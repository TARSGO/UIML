#include "cmsis_os.h"
#include "config.h"
#include "dependency.h"
#include "extendio.h"
#include "motor.h"
#include "pid.h"
#include "portable.h"
#include "softbus.h"
#include "softbus_cppapi.h"
#include "sys_conf.h"
#include "uimlnumeric.h"
#include <cmath>

#ifndef PI
#define PI 3.1415926535f
#endif

class Gimbal
{
  public:
    Gimbal(){};
    void Init(ConfItem *dict)
    {
        U_C(dict);

        // 加载配置项，创建电机与角度外环PID
        taskInterval = Conf["task-interval"].get<uint8_t>(2);
        motorYaw = BasicMotor::Create(Conf["motor-yaw"]);
        motorPitch = BasicMotor::Create(Conf["motor-pitch"]);
        pidYaw.Init(Conf["yaw-imu-pid"]);
        pidPitch.Init(Conf["pitch-imu-pid"]);

        auto pitchLimit = Conf["pitch-limit"];
        pitchUbound = -std::abs(pitchLimit["up"].get(10.0f));
        pitchLbound = std::abs(pitchLimit["down"].get(10.0f));

        auto gimbalName = Conf["name"].get("gimbal");

        // 创建话题字符串
        incomingImuEulerAngleTopic =
            alloc_sprintf(pvPortMalloc, "/%s/euler-angle", Conf["ins-name"].get("ins"));
        gimbalActionCommandTopic = alloc_sprintf(pvPortMalloc, "/%s/setting", gimbalName);
        relativeAngleTopic = alloc_sprintf(pvPortMalloc, "/%s/yaw/relative-angle", gimbalName);
        queryGimbalStateFunc = alloc_sprintf(pvPortMalloc, "/%s/query-state", gimbalName);

        // 等待电机编码器收到反馈后，设定云台电机Yaw轴累积角度起始值
        yawAngleAtZero = Conf["yaw-zero"].get<float>(0); // Yaw零点时电机输出的角度值
        motorYaw->WaitUntilFeedbackValid();
        auto yawAngleNow = motorYaw->GetData(CurrentAngle); // 取出反馈绝对角度值
        relativeAngle = AccumulatedDegTo180(motorYaw->GetData(TotalAngle)); // 初始化底盘相对角

        // 当前角度与零点角度之差为（虚拟的）已从零点转过的角度值
        // 如上电时为30度，设置零点为-30度，那么可以视为已从零点逆时针转动60度，故如此设置累计值
        motorYaw->SetTotalAngle(AccumulatedDegTo180(yawAngleNow - yawAngleAtZero));

        // 外环角度PID在Gimbal任务中，内环速度PID在电机内
        motorYaw->SetMode(MOTOR_SPEED_MODE);
        motorPitch->SetMode(MOTOR_SPEED_MODE);

        // 初始化其他变量
        systemTargetYaw = targetPitch = 0.0f;
        currentYaw = 0.0f;
        lastImuYaw = currentImuYaw = NAN;

        // 订阅相关事件
        Bus_SubscribeTopic(this, ImuEulerAngleReceiveCallback, incomingImuEulerAngleTopic);
        Bus_SubscribeTopic(this, GimbalActionCommandCallback, gimbalActionCommandTopic);
        Bus_SubscribeTopic(this, EmergencyStopCallback, "/system/stop");
        Bus_RemoteFuncRegister(this, QueryGimbalStateFuncCallback, queryGimbalStateFunc);
    }

    // 初始化后控制流交接到此函数，不再返回，此函数中开始循环并定时执行云台任务。
    [[noreturn]] void Exec();

  private:
    // 私有方法
    // 任务间隔到期后调用，执行云台解算。
    void Tick();

    // 回调函数
    static BUS_TOPICENDPOINT(ImuEulerAngleReceiveCallback); // IMU欧拉角收到数据回调
    static BUS_TOPICENDPOINT(GimbalActionCommandCallback);  // 云台转动指令回调
    static BUS_TOPICENDPOINT(EmergencyStopCallback);        // 急停事件回调
    static BUS_REMOTEFUNC(QueryGimbalStateFuncCallback);    // 查询云台角度远程函数

  private:
    // 私有成员
    uint8_t taskInterval; // 任务执行间隔

    BasicMotor *motorYaw, *motorPitch;
    PID pidYaw, pidPitch;                   // 角度外环（内环复用电机内的）
    const char *incomingImuEulerAngleTopic, // [订阅] IMU欧拉角来源话题
        *gimbalActionCommandTopic,          // [订阅] 云台操纵动作话题
        *relativeAngleTopic,                // [发布] 云台与底盘偏离角话题
        *queryGimbalStateFunc;              // [远程函数] 查询云台角度

    float systemTargetYaw; // 整车的目标 Yaw 角度（云台主动、底盘随动）累积值，单位：度
    float targetPitch; // 目标 Pitch 角度，物理上不可累积，单位：度

    float pitchUbound, pitchLbound; // Pitch角目标值上下界，钳位目标值使用

    float lastImuYaw;      // 上一次Tick时IMU的Yaw值，用于维护整车朝向
    float currentImuYaw;   // 当前的IMU的Yaw值
    float currentImuPitch; // 当前的IMU的Pitch值，直接供给PID使用，不需要累积
    float currentYaw; // 云台真实朝向Yaw值，Tick产生时计算，累积值，单位：度

    float yawAngleAtZero; // 云台Yaw轴电机在云台与底盘前向方向对齐时，底盘电机的角度值
    float relativeAngle; // 云台方向与底盘前向角度之差，广播至总线，范围-180~180，单位：度。

} gimbal;

extern "C" void Gimbal_TaskCallback(void const *argument)
{
    Depends_WaitFor(svc_gimbal, {svc_can, svc_ins, svc_chassis});
    gimbal.Init((ConfItem *)argument);
    Depends_SignalFinished(svc_gimbal);

    gimbal.Exec();
}

void Gimbal::Tick()
{
    // 在数值有效时，开始更新IMU累积Yaw值
    if (!std::isnan(lastImuYaw))
    {
        currentYaw += BoundCrossDeltaF32(-180.0f, 180.0f, lastImuYaw, currentImuYaw);
    }
    lastImuYaw = currentImuYaw;

    pidYaw.SingleCalc(systemTargetYaw, currentYaw);
    pidPitch.SingleCalc(targetPitch, currentImuPitch);

    motorYaw->SetTarget(pidYaw.output);
    motorPitch->SetTarget(pidPitch.output);

    relativeAngle = AccumulatedDegTo180(motorYaw->GetData(TotalAngle));
    Bus_PublishTopic(relativeAngleTopic, {{"angle", {.F32 = relativeAngle}}});
}

void Gimbal::Exec()
{
    while (true)
    {
        Tick();
        osDelay(taskInterval);
    }
}

BUS_TOPICENDPOINT(Gimbal::ImuEulerAngleReceiveCallback)
{
    auto self = (Gimbal *)bindData;
    if (!Bus_CheckMapKeysExist(frame, {"yaw", "pitch"}))
        return;

    float yaw = Bus_GetMapValue(frame, "yaw").F32;
    float pitch = Bus_GetMapValue(frame, "pitch").F32;

    self->currentImuYaw = yaw;
    self->currentImuPitch = pitch;
}

/**
 * @brief [订阅] SysControl发布的云台动作指令回调。
 * @note yaw与pitch取值正负均与ROS右手系中旋转方式相同。
 */
BUS_TOPICENDPOINT(Gimbal::GimbalActionCommandCallback)
{
    auto self = (Gimbal *)bindData;

    // 向世界 yaw 目标值增减
    if (Bus_CheckMapKeyExist(frame, "yaw"))
    {
        float dYaw = Bus_GetMapValue(frame, "yaw").F32;
        self->systemTargetYaw -= dYaw;
    }

    // pitch 值设置
    if (Bus_CheckMapKeyExist(frame, "pitch"))
    {
        float dPitch = Bus_GetMapValue(frame, "pitch").F32;
        float newPitch = self->targetPitch - dPitch;
        self->targetPitch = std::clamp(newPitch, self->pitchUbound, self->pitchLbound);
    }
}

BUS_TOPICENDPOINT(Gimbal::EmergencyStopCallback)
{
    auto self = (Gimbal *)bindData;

    self->motorPitch->EmergencyStop();
    self->motorYaw->EmergencyStop();
}

BUS_REMOTEFUNC(Gimbal::QueryGimbalStateFuncCallback)
{
    auto self = (Gimbal *)bindData;

    if (!Bus_CheckMapKeysExist(frame, {"pitch", "yaw"}))
        return false;

    *((float *)Bus_GetMapValue(frame, "pitch").Ptr) = self->motorPitch->GetData(CurrentAngle);
    *((float *)Bus_GetMapValue(frame, "yaw").Ptr) = self->motorYaw->GetData(CurrentAngle);

    return true;
}
