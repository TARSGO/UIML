
#include "byteorder.h"
#include "extendio.h"
#include "motor.h"
#include "portable.h"

void M3508::Init(ConfItem *dict)
{
    // 电机减速比；如果未配置电机减速比参数，则使用原装电机默认减速比
    m_reductionRatio = Conf_GetValue(dict, "reduction-ratio", float, 19.2f);

    // 初始化电机绑定CAN信息
    uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
    m_canInfo.rxID = id + 0x200;                // 从电机收到报文的ID
    m_canInfo.txID = (id <= 4) ? 0x200 : 0x1FF; // 向电机发送报文的ID
    m_canInfo.bufIndex = ((id - 1) % 4) * 2;
    m_canInfo.canX = Conf_GetValue(dict, "can-x", uint8_t, 0);

    // 调用公用部分
    DjiCanMotor::Init(dict);

    // 读取电机名称，创建堵转话题
    const char *motorName = Conf_GetValue(dict, "name", const char *, "m3508");
    char *stallName = alloc_sprintf(pvPortMalloc, "/%s/stall", motorName);
    m_stallTopic = Bus_GetFastTopicHandle(stallName); // 创建话题句柄
    MOTOR_FREE_PORT(stallName);                       // 释放内存

    // 软件定时器
    osTimerDef(M3508, M3508::TimerCallback);
    osTimerStart(osTimerCreate(osTimer(M3508), osTimerPeriodic, this), 2);
}

void M3508::CanTransmit(uint16_t output)
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

void M3508::CanRxUpdate(uint8_t *rxData)
{
    struct
    {
        uint16_t Angle;
        uint16_t Speed;
        uint16_t Current;
        uint8_t Temperature;
        uint8_t Reserved;
    } __attribute((__packed__)) * C620Feedback;
    C620Feedback = reinterpret_cast<decltype(C620Feedback)>(rxData);

    // 将反馈报文数据读入类成员
    m_motorAngle = SwapByteorder16Bit(C620Feedback->Angle);
    m_motorSpeed = SwapByteorder16Bit(C620Feedback->Speed);
    m_motorTorque = SwapByteorder16Bit(C620Feedback->Current);
    m_motorTemperature = C620Feedback->Temperature;

    m_motorReducedRpm = m_motorSpeed / m_reductionRatio;
    m_feedbackValid = true;
}
