#include "config.h"
#include "motor.h"
#include "pid.h"
#include "softbus.h"
#include "string.h"

void DjiCanMotor::Init(ConfItem *dict)
{
    U_C(dict);
    // 公共初始化部分
    // 注意：必须设定好减速比与CAN信息

    // 默认模式为扭矩模式
    m_mode = MOTOR_TORQUE_MODE;

    // 初始化电机pid
    m_speedPid.Init(Conf["speed-pid"]);
    m_anglePid.Init(Conf["angle-pid"]);
    m_anglePid.outer.maxOutput *= m_reductionRatio; // 将输出轴速度限幅放大到转子上

    // 读取电机方向，为1正向为-1反向
    m_direction = Conf["direction"].get<int8_t>(1);
    if (m_direction != -1 && m_direction != 1)
        m_direction = 1;

    // 订阅can信息
    char name[] = "/can_/recv";
    name[4] = m_canInfo.canX + '0';
    Bus_SubscribeTopic(this, DjiCanMotor::CanRxCallback, name);

    m_stallTime = 0;
}

// 切换电机模式
bool DjiCanMotor::SetMode(MotorCtrlMode mode)
{
    if (m_mode == MOTOR_STOP_MODE) // 急停模式下不允许切换模式
        return false;

    m_mode = mode;
    switch (mode)
    {
    case MOTOR_SPEED_MODE:
        m_speedPid.Clear();
        break;
    case MOTOR_ANGLE_MODE:
        m_anglePid.Clear();
        break;
    default:
        break;
    }

    return true;
}

// 急停函数
void DjiCanMotor::EmergencyStop()
{
    m_target = 0;
    m_mode = MOTOR_STOP_MODE;
}

// 设置电机期望值
bool DjiCanMotor::SetTarget(float target)
{
    switch (m_mode)
    {
    case MOTOR_SPEED_MODE:
        m_target = target * m_reductionRatio;
        break;
    case MOTOR_ANGLE_MODE:
        m_target = DegToCode(target, m_reductionRatio);
        break;
    case MOTOR_TORQUE_MODE:
        m_target = target;
        break;
    default:
        return false;
    }

    return true;
}

// 开始统计电机累计角度
bool DjiCanMotor::SetTotalAngle(float angle)
{
    m_totalAngle = DegToCode(angle, m_reductionRatio);
    m_lastAngle = m_motorAngle;

    return true;
}

// 获取电机数据
float DjiCanMotor::GetData(MotorDataType type)
{
    switch (type)
    {
    case CurrentAngle:
        // 转子当前角度
        return CodeToDeg(m_motorAngle, 1);

    case TotalAngle:
        // 输出轴总转过角度
        return CodeToDeg(m_totalAngle, m_reductionRatio);

    case RawAngle:
        // 转子当前角度，原始值
        return m_motorAngle;

    case Speed:
        // 输出轴转速（RPM）
        return m_motorSpeed;

    case SpeedReduced:
        // 减速后的转速（RPM）
        return m_motorReducedRpm;

    default:
        return 0;
    }
}

void DjiCanMotor::CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData)
{
    // 校验绑定电机对象
    if (!bindData)
        return;

    auto self = reinterpret_cast<DjiCanMotor *>(bindData);

    // 校验电机ID
    if (self->m_canInfo.rxID != Bus_GetListValue(frame, 0).U16)
        return;

    auto data = reinterpret_cast<uint8_t *>(Bus_GetListValue(frame, 1).Ptr);
    if (data)
        self->CanRxUpdate(data); // 更新电机数据
}

void DjiCanMotor::TimerCallback(const void *argument)
{
    auto self = reinterpret_cast<DjiCanMotor *>(pvTimerGetTimerID((TimerHandle_t)argument));

    // 计算前一帧至今转过的角度
    self->UpdateAngle();
    // 调用控制器计算电机下一步行动
    self->ControllerUpdate(self->m_target);

    // 判断堵转情况
}

// 电机急停回调函数
void DjiCanMotor::EmergencyStopCallback(const char *endpoint, SoftBusFrame *frame, void *bindData)
{
    DjiCanMotor *self = reinterpret_cast<DjiCanMotor *>(bindData);
    self->m_target = 0;
    self->m_mode = MOTOR_STOP_MODE;
}

// 统计电机累计转过的圈数
void DjiCanMotor::UpdateAngle()
{
    int32_t angleDiff = 0;
    // FIXME: 无注释，逻辑存疑
    if (m_motorAngle - m_lastAngle < -4000)
        angleDiff = m_motorAngle + (8191 - m_lastAngle);
    else if (m_motorAngle - m_lastAngle > 4000)
        angleDiff = -m_lastAngle - (8191 - m_motorAngle);
    else
        angleDiff = m_motorAngle - m_lastAngle;
    // 将角度增量加入计数器
    m_totalAngle += angleDiff;
    // 记录角度
    m_lastAngle = m_motorAngle;
}

// 控制器根据模式计算输出
void DjiCanMotor::ControllerUpdate(float target)
{
    int16_t output = 0;
    switch (m_mode)
    {
    case MOTOR_SPEED_MODE:
        output = m_speedPid.SingleCalc(target, m_motorSpeed);
        break;
    case MOTOR_ANGLE_MODE:
        output = m_anglePid.CascadeCalc(target, m_totalAngle, m_motorSpeed);
        break;
    case MOTOR_TORQUE_MODE:
        output = (int16_t)target;
        break;
    case MOTOR_STOP_MODE:
        return;
    }

    CanTransmit(output);
}

// 堵转检测逻辑
void DjiCanMotor::StallDetection()
{
    m_stallTime = (m_motorSpeed == 0 && abs(m_motorTorque) > 7000) ? m_stallTime + 2 : 0;
    if (m_stallTime > 500)
    {
        Bus_PublishTopicFast(m_stallTopic, {{nullptr}});
        m_stallTime = 0;
    }
}
