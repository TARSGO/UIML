#include "config.h"
#include "motor.h"
#include "pid.h"
#include "softbus.h"
#include "string.h"

void DjiCanMotor::Init(ConfItem *dict)
{
    U_C(dict);
    // 公共初始化部分
    // 注意：必须已经由子类设定好了减速比与CAN信息才能到达这里

    // 注册CAN电机到管理器
    auto motorName = Conf["name"].get<const char *>(nullptr);
    if (motorName != nullptr)
        RegisterCanMotor(motorName);

    // 默认模式为扭矩模式
    m_mode = ModeTorque;

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

    m_target = 0.0f;
    m_stallTime = 0;
    m_totalAngle = 0;
    m_motorReducedRpm = 0;
}

// 切换电机模式
bool DjiCanMotor::SetMode(MotorCtrlMode mode)
{
    if (m_mode == ModeStop) // 急停模式下不允许切换模式
        return false;

    m_mode = mode;
    switch (mode)
    {
    case ModeSpeed:
        m_speedPid.Clear();
        break;
    case ModeAngle:
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
    m_mode = ModeStop;
    CanTransmit(0);
}

// 设置电机期望值
bool DjiCanMotor::SetTarget(float target)
{
    switch (m_mode)
    {
    case ModeSpeed:
        m_target = target * m_reductionRatio;
        break;
    case ModeAngle:
        m_target = DegToCode(target, m_reductionRatio);
        break;
    case ModeTorque:
        m_target = target;
        break;
    default:
        return false;
    }

    return true;
}

// 开始统计电机累计角度
bool DjiCanMotor::SetData(MotorDataType type, float data)
{
    switch (type)
    {
    case TotalAngle:
        m_totalAngle = DegToCode(data, m_reductionRatio);
        m_lastAngle = m_motorAngle;
        return true;

    default:
        return false;
    }
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
        return CanMotor::GetData(type);
    }
}

void DjiCanMotor::CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData)
{
    // 校验绑定电机对象
    if (!bindData)
        return;

    U_S(DjiCanMotor);

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
    U_S(DjiCanMotor);
    self->EmergencyStop();
}

// 统计电机累计转过的圈数
void DjiCanMotor::UpdateAngle()
{
    int32_t angleDiff = 0;
    // 计算角度增量（加过零检测）
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
    case ModeSpeed:
        output = m_speedPid.SingleCalc(target, m_motorSpeed);
        break;
    case ModeAngle:
        output = m_anglePid.CascadeCalc(target, m_totalAngle, m_motorSpeed);
        break;
    case ModeTorque:
        output = (int16_t)target;
        break;
    case ModeStop:
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
