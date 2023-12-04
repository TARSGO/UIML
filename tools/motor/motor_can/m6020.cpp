
#include "byteorder.h"
#include "extendio.h"
#include "motor.h"

void M6020::Init(ConfItem *dict)
{
    // 电机减速比；如果未配置电机减速比参数，则使用原装电机默认减速比
    m_reductionRatio = Conf_GetValue(dict, "reduction-ratio", float, 1.0f);

    // 初始化电机绑定CAN信息
    uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
    m_canInfo.rxID = id + 0x204;                // 从电机收到报文的ID
    m_canInfo.txID = (id <= 4) ? 0x1FF : 0x2FF; // 向电机发送报文的ID
    m_canInfo.bufIndex = ((id - 1) % 4) * 2;
    m_canInfo.canX = Conf_GetValue(dict, "can-x", uint8_t, 0);

    // 调用公用部分
    DjiCanMotor::Init(dict);

    // 读取电机名称，创建堵转话题
    const char *motorName = Conf_GetValue(dict, "name", const char *, "m6020");
    char *stallName = alloc_sprintf(pvPortMalloc, "/%s/stall", motorName);
    m_stallTopic = Bus_GetFastTopicHandle(stallName); // 创建话题句柄
    MOTOR_FREE_PORT(stallName);                       // 释放内存

    // 软件定时器
    osTimerDef(M6020, M6020::TimerCallback);
    osTimerStart(osTimerCreate(osTimer(M6020), osTimerPeriodic, this), 2);
}

bool M6020::SetMode(MotorCtrlMode mode)
{
    if (mode == MOTOR_TORQUE_MODE)
        return false;

    return DjiCanMotor::SetMode(mode);
}

void M6020::CanTransmit(uint16_t output)
{
    // 变换方向
    output *= m_direction;

    // 交换成大端
    output = SwapByteorder16Bit(output);

    Bus_RemoteFuncCall("/can/set-buf",
                       {{"can-x", {.U8 = m_canInfo.canX}},
                        {"id", {.U16 = m_canInfo.txID}},
                        {"pos", {.U8 = m_canInfo.bufIndex}},
                        {"len", {.U8 = 2}},
                        {"data", {&output}}});
}

void M6020::CanRxUpdate(uint8_t *rxData)
{
    struct
    {
        uint16_t Angle;
        uint16_t Speed;
        uint16_t Current;
        uint8_t Temperature;
        uint8_t Reserved;
    } __attribute((__packed__)) * M6020Feedback;
    M6020Feedback = reinterpret_cast<decltype(M6020Feedback)>(rxData);

    // 将反馈报文数据读入类成员
    m_motorAngle = SwapByteorder16Bit(M6020Feedback->Angle);
    m_motorSpeed = SwapByteorder16Bit(M6020Feedback->Speed);
    m_motorTorque = M6020Feedback->Current;
    m_motorTemperature = M6020Feedback->Temperature;

    m_motorReducedRpm = m_motorSpeed / m_reductionRatio;
    m_feedbackValid = true;
}
