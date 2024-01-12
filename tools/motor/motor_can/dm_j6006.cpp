#include "config.h"
#include "motor.h"
#include "pid.h"
#include "softbus.h"
#include "string.h"
#include "uimlnumeric.h"
#include <stdint.h>

void DamiaoJ6006::Init(ConfItem *dict)
{
    U_C(dict);

    // 读取CAN信息
    // 由于达妙电机不像大疆的RM电机一样在一个ID里塞多个电机的控制量，
    // 所以不使用循环缓冲区。另外，ID也是由调参软件设置的，要直接从配置文件读
    m_canInfo.rxID = Conf["rx-id"].get<uint16_t>(0);
    m_canInfo.txID = Conf["tx-id"].get<uint16_t>(0);
    m_canInfo.canX = Conf["can-x"].get<uint8_t>(0);

    // 默认模式为扭矩模式
    m_mode = ModeTorque;

    // 读取电机方向，为1正向为-1反向
    m_direction = Conf["direction"].get<int8_t>(1);
    if (m_direction != -1 && m_direction != 1)
        m_direction = 1;

    // 订阅can信息
    char name[] = "/can_/recv";
    name[4] = char(m_canInfo.canX) + '0';
    Bus_SubscribeTopic(this, DamiaoJ6006::CanRxCallback, name);

    m_target = 0.0f;
    // m_stallTime = 0;
    m_totalAngle = 0;
}

// 切换电机模式
bool DamiaoJ6006::SetMode(MotorCtrlMode mode)
{
    if (m_mode == ModeStop) // 急停模式下不允许切换模式
        return false;

    // J6006可以支持UIML定义的全部三种工作模式，所以不用判断其他的东西
    // 同样因为没有在单片机上进行PID计算，所以也不用像大疆的一样清空PID
    m_mode = mode;
    m_target = 0.0f;

    return true;
}

// 急停函数
void DamiaoJ6006::EmergencyStop()
{
    m_target = 0;
    m_mode = ModeStop;
    CommandSpeedMode(0);
}

// 设置电机期望值
bool DamiaoJ6006::SetTarget(float target)
{
    switch (m_mode)
    {
    case ModeSpeed:
        m_target = target;
        CommandSpeedMode(target);
        break;
    case ModeAngle:
        m_target = target;
        CommandAngleMode(target, m_autonomousAngleModeSpeedLimit);
        break;
    case ModeTorque:
        m_target = target;
        CommandMitMode(0, 0, 0, 0, target);
        break;
    default:
        return false;
    }

    return true;
}

// 开始统计电机累计角度
bool DamiaoJ6006::SetData(MotorDataType type, float data)
{
    switch (type)
    {
    case TotalAngle:
        m_totalAngle = data;
        return true;

    case SpeedLimit:
        m_autonomousAngleModeSpeedLimit = data;
        return true;

    default:
        return false;
    }
}

// 获取电机数据
float DamiaoJ6006::GetData(MotorDataType type)
{
    switch (type)
    {
    case CurrentAngle:
        // 转子当前角度
        return CodeToDeg(m_motorAngle);

    case TotalAngle:
        // 输出轴总转过角度
        return m_totalAngle;

    case RawAngle:
        // 转子当前角度，原始值
        return m_motorAngle;

    case SpeedReduced:
        // 减速后的转速（RPM）
    case Speed:
        // 输出轴转速（RPM）
        return Radps2Rpm(m_motorSpeed);

    case SpeedLimit:
        // 电机自主进行角度环控制时的速度上限（RPM）
        return m_autonomousAngleModeSpeedLimit;

    default:
        return CanMotor::GetData(type);
    }
}

void DamiaoJ6006::CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData)
{
    // 校验绑定电机对象
    if (!bindData)
        return;

    U_S(DamiaoJ6006);

    // 校验电机ID
    if (self->m_canInfo.rxID != Bus_GetListValue(frame, 0).U16)
        return;

    auto data = reinterpret_cast<uint8_t *>(Bus_GetListValue(frame, 1).Ptr);
    if (data) // 更新电机数据
    {
        struct __packed
        {
            uint8_t ErrorBits : 4;
            uint8_t IdLSByte : 4;
            uint16_t Angle;
            uint8_t SpeedTorque[3]; // 必须手动移位
            uint8_t DriverTemperature;
            uint8_t CoilTemperature;
        } *J6006Feedback;

        self->m_motorAngle = J6006Feedback->Angle;
        self->m_motorSpeed =
            uint16_t(J6006Feedback->SpeedTorque[0] << 4) | (J6006Feedback->SpeedTorque[1] >> 4);
        self->m_motorTorque =
            uint16_t((J6006Feedback->SpeedTorque[1] & 0x0F) << 8) | J6006Feedback->SpeedTorque[2];
        self->m_motorCoilTemp = J6006Feedback->CoilTemperature;
        self->m_motorDriverTemp = J6006Feedback->DriverTemperature;
    }
}

// 发送使能/失能控制帧
void DamiaoJ6006::CommandEnable(bool enable)
{
    uint8_t frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};
    frame[7] = enable ? 0xFC : 0xFD;
    CanTransmitFrame(m_canInfo.txID, 8, frame);
}

// 发送保存位置零点控制帧
void DamiaoJ6006::CommandSetZero()
{
    uint8_t frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
    CanTransmitFrame(m_canInfo.txID, 8, frame);
}

// 发送清除错误消息控制帧
void DamiaoJ6006::CommandClearErrorFlag()
{
    uint8_t frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB};
    CanTransmitFrame(m_canInfo.txID, 8, frame);
}

// 向MIT模式发送指令
// FIXME: MIT模式的速度需要映射到上位机设置的范围,这个东西需要先写个配置文件,
// 现在先用个扭矩控制,后面再改
void DamiaoJ6006::CommandMitMode(
    uint16_t targetAngle, uint16_t targetSpeed, uint16_t kp, uint16_t kd, uint16_t targetTorque)
{
    // TODO:

    // CanTransmitFrame(m_canInfo.rxID, 8, data);
}

// 向角度模式发送命令
void DamiaoJ6006::CommandAngleMode(float targetAngle, float targetSpeed)
{
    // 达妙用的是rad和rad/s
    targetAngle = Degree2Radian(targetAngle);
    targetSpeed = Rpm2Radps(targetSpeed);

    float data[2] = {targetAngle, targetSpeed};
    CanTransmitFrame(m_canInfo.rxID + 0x100, 8, (uint8_t *)data);
}

// 向速度模式发送命令
void DamiaoJ6006::CommandSpeedMode(float targetSpeed)
{
    // 达妙用的是rad/s
    targetSpeed = Rpm2Radps(targetSpeed);

    CanTransmitFrame(m_canInfo.rxID + 0x200, 4, (uint8_t *)&targetSpeed);
}

inline void DamiaoJ6006::CanTransmitFrame(uint16_t txId, uint8_t length, uint8_t *data)
{
    Bus_RemoteFuncCall("/can/send",
                       {{"can-x", {.U8 = m_canInfo.canX}},
                        {"id", {.U16 = txId}},
                        {"len", {.U8 = length}},
                        {"data", {data}}});
}

// void DamiaoJ6006::TimerCallback(const void *argument)
// {
//     auto self = reinterpret_cast<DamiaoJ6006 *>(pvTimerGetTimerID((TimerHandle_t)argument));

//     // 计算前一帧至今转过的角度
//     self->UpdateAngle();
//     // 调用控制器计算电机下一步行动
//     self->ControllerUpdate(self->m_target);

//     // 判断堵转情况
// }

// 电机急停回调函数
// void DamiaoJ6006::EmergencyStopCallback(const char *endpoint, SoftBusFrame *frame, void
// *bindData)
// {
//     U_S(DamiaoJ6006);
//     self->EmergencyStop();
// }

// 统计电机累计转过的圈数
// FIXME:
// void DamiaoJ6006::UpdateAngle()
// {
//     int32_t angleDiff = 0;
//     // 计算角度增量（加过零检测）
//     if (m_motorAngle - m_lastAngle < -8000)
//         angleDiff = m_motorAngle + (16383 - m_lastAngle);
//     else if (m_motorAngle - m_lastAngle > 8000)
//         angleDiff = -m_lastAngle - (16383 - m_motorAngle);
//     else
//         angleDiff = m_motorAngle - m_lastAngle;
//     // 将角度增量加入计数器
//     m_totalAngle += angleDiff;
//     // 记录角度
//     m_lastAngle = m_motorAngle;
// }

// 控制器根据模式计算输出
// void DamiaoJ6006::ControllerUpdate(float target)
// {
//     int16_t output = 0;
//     switch (m_mode)
//     {
//     case MOTOR_SPEED_MODE:
//         output = m_speedPid.SingleCalc(target, m_motorSpeed);
//         break;
//     case MOTOR_ANGLE_MODE:
//         output = m_anglePid.CascadeCalc(target, m_totalAngle, m_motorSpeed);
//         break;
//     case MOTOR_TORQUE_MODE:
//         output = (int16_t)target;
//         break;
//     case MOTOR_STOP_MODE:
//         return;
//     }

//     CanTransmit(output);
// }

// 堵转检测逻辑
// void DamiaoJ6006::StallDetection()
// {
//     m_stallTime = (m_motorSpeed == 0 && abs(m_motorTorque) > 7000) ? m_stallTime + 2 : 0;
//     if (m_stallTime > 500)
//     {
//         Bus_PublishTopicFast(m_stallTopic, {{nullptr}});
//         m_stallTime = 0;
//     }
// }
