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
/*
        云台控制服务

    - 关于云台电机方向与云台偏离角方向的设置
        云台电机方向：
        IMU角度数据使用常用的ROS右手系，在+Z轴上看向原点，物体逆时针旋转，旋转角为正，
        此时假设云台右转偏离中心10度，由于PID的Error = Ref(0) - Fb(-10)，得到Error为正，
        再乘以正的Kp，角度环的输出为正。在大疆的6020电机上，速度环给出正的电流值时，
        电机逆时针旋转，在云台电机正装的机器人上，这将导致云台左转，从而达到控制效果；
        依此可知，在6020电机反装的机器人上，需要将电机配置中的direction属性改为-1，
        这将使速度环的输出量乘以-1，使电机控制云台左转。
        总结：
        云台电机控制电流值为正时，云台左转，则yaw轴电机的direction置1，反之置-1。

        底盘偏离角方向：
        底盘接收的偏离角范围：左负右正，范围[-180, 180]，单位：度
        当Yaw轴使用大疆6020电机时，电机逆时针旋转，反馈角为负；
        如果电机正装，则对应为“云台右转，反馈角减小”，
        此时云台右转，电机对象统计的累计转动角将减小。这种情况下，累计转动角是“左正右负”的，
        与底盘类期待的方向不同，需要乘以-1；
        如果电机反装，则对应为“云台右转，反馈角增大”，这种情况下，累计转动角是“左负右正”的，
        与底盘类期待的方向相同。
        总结：
        假设云台电机 位置反馈值 与 控制电流值 的正负号对应方向的关系相同；
        云台右转时，电机反馈角度减小，需要为累计角度值乘以-1；
        但为了在6020正装的情况下，让yaw-direction与yaw轴电机的direction保持一致，
        规定在此种情况下yaw-direction置1。反之置-1。
*/

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
        motorYaw->SetData(TotalAngle, AccumulatedDegTo180(yawAngleNow - yawAngleAtZero));

        // 外环角度PID在Gimbal任务中，内环速度PID在电机内
        motorYaw->SetMode(ModeSpeed);
        motorPitch->SetMode(ModeSpeed);

        // 初始化其他变量
        systemTargetYaw = targetPitch = 0.0f;
        currentYaw = 0.0f;
        lastImuYaw = currentImuYaw = NAN;
        yawDirection = ((Conf["yaw-direction"].get<int32_t>(-1) > 0) ? -1 : 1); // 取反，原因见注释

        // 禁用云台模式，用于测试云台Yaw电机零度角在哪里
        disableMode = Conf["disable"].get<uint32_t>(1) != 0;
        if (disableMode)
        {
            motorYaw->SetMode(ModeStop);
            motorPitch->SetMode(ModeStop);
        }

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

    bool disableMode; // 禁用云台模式（用于调试云台初始角，此情况下会发送偏离角为0度，电机停转）
    int8_t yawDirection; // 云台偏离角方向，被乘在广播到总线上的偏离角数值上（见文件头详细注释）

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
    Depends_WaitFor(svc_gimbal, {svc_can, svc_ins});
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

    if (disableMode)
        relativeAngle = 0.0f;
    else
        relativeAngle = AccumulatedDegTo180(motorYaw->GetData(TotalAngle));
    Bus_PublishTopic(relativeAngleTopic, {{"angle", {.F32 = relativeAngle * yawDirection}}});
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

    // 立即使电机动作
    Bus_RemoteFuncCall("/can/flush-buf", {{nullptr, nullptr}});
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
