#include "config.h"
#include "motor.h"
#include "pid.h"
#include "softbus.h"
#include <stdio.h>


// 软件定时器回调函数
void DcMotor::TimerCallback(void const *argument)
{
    auto self = reinterpret_cast<DcMotor *>(pvTimerGetTimerID((TimerHandle_t)argument));
    self->UpdateAngle();
    self->ControllerUpdate(self->m_target);
}

void DcMotor::Init(ConfItem *dict)
{
    // 电机减速比
    m_reductionRatio = Conf_GetValue(dict, "reduction-ratio", float, 19.0f); // 电机减速比参数
    m_circleEncode = Conf_GetValue(dict, "max-encode", float, 48.0f); // 倍频后编码器转一圈的最大值
    // 初始化电机绑定tim信息
    TimerInit(dict);
    // 设置电机默认模式为速度模式
    SetMode(MOTOR_SPEED_MODE);
    // 初始化电机pid
    m_speedPID.Init(Conf_GetNode(dict, "speed-pid"));
    m_anglePID.Init(Conf_GetNode(dict, "angle-pid"));
    m_anglePID.outer.maxOutput =
        m_speedPID.maxOutput / m_reductionRatio; // 将输出轴速度限幅放大到转子上

    // 开启软件定时器
    osTimerDef(DCMOTOR, DcMotor::TimerCallback);
    osTimerStart(osTimerCreate(osTimer(DCMOTOR), osTimerPeriodic, this), 2);
}

// 初始化tim
void DcMotor::TimerInit(ConfItem *dict)
{
    m_posRotateTim.timX = Conf_GetValue(dict, "pos-rotate-tim/tim-x", uint8_t, 0);
    m_posRotateTim.channelX = Conf_GetValue(dict, "pos-rotate-tim/channel-x", uint8_t, 0);
    m_negRotateTim.timX = Conf_GetValue(dict, "neg-rotate-tim/tim-x", uint8_t, 0);
    m_negRotateTim.channelX = Conf_GetValue(dict, "neg-rotate-tim/channel-x", uint8_t, 0);
    m_encodeTim.channelX = Conf_GetValue(dict, "encode-tim/tim-x", uint8_t, 0);
}

// 开始统计电机累计角度
bool DcMotor::SetData(MotorDataType type, float data)
{
    switch (type)
    {
    case TotalAngle:
        m_totalAngle = Degree2Code(data, m_reductionRatio, m_circleEncode);
        m_lastAngle = data;
        return true;

    default:
        return false;
    }

    return true;
}

// 统计电机累计转过的圈数
void DcMotor::UpdateAngle()
{
    int32_t dAngle = 0;
    uint32_t autoReload = 0;

    Bus_RemoteFuncCall("/tim/encode",
                       {{"tim-x", {.U8 = m_encodeTim.timX}},
                        {"count", {.U16 = m_angle}},
                        {"auto-reload", {&autoReload}}});

    if (m_angle - m_lastAngle < -(autoReload / 2.0f))
        dAngle = m_angle + (autoReload - m_lastAngle);
    else if (m_angle - m_lastAngle > (autoReload / 2.0f))
        dAngle = -m_lastAngle - (autoReload - m_angle);
    else
        dAngle = m_angle - m_lastAngle;
    // 将角度增量加入计数器
    m_totalAngle += dAngle;
    // 计算速度
    m_speed = (float)dAngle / (m_circleEncode * m_reductionRatio) * 500 * 60; // rpm
    // 记录角度
    m_lastAngle = m_angle;
}

// 控制器根据模式计算输出
void DcMotor::ControllerUpdate(float reference)
{
    float output = 0;
    if (m_mode == MOTOR_SPEED_MODE)
    {
        m_speedPID.SingleCalc(reference, m_speed);
        output = m_speedPID.output;
    }
    else if (m_mode == MOTOR_ANGLE_MODE)
    {
        m_anglePID.CascadeCalc(reference, m_totalAngle, m_speed);
        output = m_anglePID.output;
    }
    else if (m_mode == MOTOR_TORQUE_MODE)
    {
        output = reference;
    }

    if (output > 0)
    {
        Bus_RemoteFuncCall("/tim/pwm/set-duty",
                           {{"tim-x", {.U8 = m_posRotateTim.timX}},
                            {"channel-x", {.U8 = m_posRotateTim.channelX}},
                            {"duty", {.F32 = output}}});
        Bus_RemoteFuncCall("/tim/pwm/set-duty",
                           {{"tim-x", {.U8 = m_posRotateTim.timX}},
                            {"channel-x", {.U8 = m_negRotateTim.channelX}},
                            {"duty", {.F32 = 0.0f}}});
    }
    else
    {
        output = ABS(output);
        Bus_RemoteFuncCall("/tim/pwm/set-duty",
                           {{"tim-x", {.U8 = m_posRotateTim.timX}},
                            {"channel-x", {.U8 = m_posRotateTim.channelX}},
                            {"duty", {.F32 = 0.0f}}});
        Bus_RemoteFuncCall("/tim/pwm/set-duty",
                           {{"tim-x", {.U8 = m_posRotateTim.timX}},
                            {"channel-x", {.U8 = m_negRotateTim.channelX}},
                            {"duty", {.F32 = output}}});
    }
}

// 设置电机期望值
bool DcMotor::SetTarget(float targetValue)
{
    // FIXME: Else分支真的可以直接设置吗
    if (m_mode == MOTOR_ANGLE_MODE)
    {
        m_target = Degree2Code(targetValue, m_reductionRatio, m_circleEncode);
    }
    else if (m_mode == MOTOR_SPEED_MODE)
    {
        m_target = targetValue * m_reductionRatio;
    }
    else
    {
        m_target = targetValue;
    }

    return true;
}

// 切换电机模式
bool DcMotor::SetMode(MotorCtrlMode mode)
{
    // 检测模式是否合法并设置
    if (mode != MOTOR_SPEED_MODE && mode != MOTOR_ANGLE_MODE)
        return false;
    m_mode = mode;

    // 清 PID
    switch (mode)
    {
    case MOTOR_SPEED_MODE:
        m_speedPID.Clear();
    case MOTOR_ANGLE_MODE:
        m_anglePID.Clear();
    default:
        break;
    }

    return true;
}

void DcMotor::EmergencyStop()
{
    m_mode = MOTOR_STOP_MODE;

    Bus_RemoteFuncCall("/tim/pwm/set-duty",
                       {{"tim-x", {.U8 = m_posRotateTim.timX}},
                        {"channel-x", {.U8 = m_posRotateTim.channelX}},
                        {"duty", {.F32 = 0.0f}}});
    Bus_RemoteFuncCall("/tim/pwm/set-duty",
                       {{"tim-x", {.U8 = m_posRotateTim.timX}},
                        {"channel-x", {.U8 = m_negRotateTim.channelX}},
                        {"duty", {.F32 = 0.0f}}});
}

float DcMotor::GetData(MotorDataType type)
{
    switch (type)
    {
    case CurrentAngle:
        return Code2Degree(m_totalAngle, m_reductionRatio, m_circleEncode);
    case TotalAngle:
        return Code2Degree(m_totalAngle, m_reductionRatio, m_circleEncode);
    case Speed:
        return m_speed;
    default:
        return 0;
    }
}
