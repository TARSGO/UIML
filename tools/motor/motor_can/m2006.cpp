
#include "byteorder.h"
#include "extendio.h"
#include "motor.h"

void M2006::Init(ConfItem *dict)
{
    U_C(dict);
    // 电机减速比；如果未配置电机减速比参数，则使用原装电机默认减速比
    m_reductionRatio = Conf["reduction-ratio"].get<float>(19.2f);

    // 初始化电机绑定CAN信息
    uint16_t id = Conf["id"].get<uint16_t>(0);
    m_canInfo.rxID = id + 0x200;                // 从电机收到报文的ID
    m_canInfo.txID = (id <= 4) ? 0x200 : 0x1FF; // 向电机发送报文的ID
    m_canInfo.bufIndex = ((id - 1) % 4) * 2;
    m_canInfo.canX = Conf["can-x"].get<uint8_t>(0);

    // 调用公用部分
    DjiCanMotor::Init(dict);

    // 读取电机名称，创建堵转话题
    const char *motorName = Conf["name"].get<const char *>("m2006");
    char *stallName = alloc_sprintf(pvPortMalloc, "/%s/stall", motorName);
    m_stallTopic = Bus_GetFastTopicHandle(stallName); // 创建话题句柄
    MOTOR_FREE_PORT(stallName);                       // 释放内存

    // 软件定时器
    osTimerDef(M2006, M2006::TimerCallback);
    osTimerStart(osTimerCreate(osTimer(M2006), osTimerPeriodic, this), 2);
}

void M2006::CanTransmit(uint16_t output)
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

void M2006::CanRxUpdate(uint8_t *rxData)
{
    struct
    {
        uint16_t Angle;
        uint16_t Speed;
        uint16_t Current;
        uint16_t Reserved;
    } __attribute((__packed__)) * C610Feedback;
    C610Feedback = reinterpret_cast<decltype(C610Feedback)>(rxData);

    // 将反馈报文数据读入类成员
    m_motorAngle = SwapByteorder16Bit(C610Feedback->Angle);
    m_motorSpeed = SwapByteorder16Bit(C610Feedback->Speed);
    m_motorTorque = SwapByteorder16Bit(C610Feedback->Current);

    m_motorReducedRpm = m_motorSpeed / m_reductionRatio;
    m_feedbackValid = true;
}
