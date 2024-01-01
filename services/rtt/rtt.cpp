
#include "stm32f4xx_hal.h"
#include "config.h"
#include "dependency.h"
#include "portmacro.h"
#include "rtt/SEGGER_RTT.h"
#include "softbus.h"
#include "softbus_cppapi.h"
#include <cmsis_os.h>
#include <cstddef>
#include <cstring>

class RTTReceiver
{
  public:
    RTTReceiver(){};
    [[noreturn]] void Loop()
    {
        // 默认每15ms检测一次下行链路（上位机到下位机）是否有数据
        while (true)
        {
            if (SEGGER_RTT_HasData(downlinkBufferIndex))
            {
                // 调用私有方法连续从RTT下行链路取出数据
                ContinuousConsumeRtt();
            }
            osDelay(detectInterval);
        }
    }

    void Init(ConfItem *conf)
    {
        U_C(conf);

        detectInterval = Conf["detect-interval"].get<uint16_t>(15);
        continueTimeout = Conf["cont-timeout"].get<uint16_t>(5);
        downlinkBufferIndex = Conf["down-buf"].get<uint16_t>(0);

        broadcastBufferSize = Conf["broadcast-buf-size"].get<uint16_t>(16);
        broadcastBuffer = (char *)pvPortMalloc(broadcastBufferSize);

        // 下行链路需要保证不丢数据，配置为缓冲区满则阻塞
        SEGGER_RTT_ConfigDownBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);

        receiveTopicHandle = Bus_GetFastTopicHandle("/rtt/recv");
        Bus_RemoteFuncRegister(this, RttSendFuncCallback, "/rtt/send");
    }

  private:
    /**
     * @brief
     * 在下行链路有数据时被自动调用，于任务线程内循环，直到HAL计时超出continueTimeout。
     * 在等待期间如果有新的数据产生，则重置计时。以此达到最多等待上位机5ms继续填充缓冲区的目的。
     */
    void ContinuousConsumeRtt()
    {
        auto beginTime = HAL_GetTick();
        while (beginTime + continueTimeout > HAL_GetTick())
        {
            if (SEGGER_RTT_HasData(downlinkBufferIndex))
            {
                auto bytesRead =
                    SEGGER_RTT_Read(downlinkBufferIndex, broadcastBuffer, broadcastBufferSize);
                Bus_PublishTopicFast(receiveTopicHandle, {{broadcastBuffer}, {.U32 = bytesRead}});
                beginTime = HAL_GetTick();
            }
        }
    }

    static BUS_REMOTEFUNC(RttSendFuncCallback); // RTT发送远程函数

  private:
    uint16_t detectInterval;
    uint16_t continueTimeout;
    uint16_t downlinkBufferIndex;
    uint16_t broadcastBufferSize;
    char *broadcastBuffer;

    SoftBusReceiverHandle receiveTopicHandle;

} RttReceiver;

/**
 * @brief RTT发送远程函数。
 *        data [.Str] 要发送的数据
 *        len [.U16] 要发送的数据长度
 *        sent [.Ptr (uint16_t *)，可选] 发送出去的数据长度
 */
BUS_REMOTEFUNC(RTTReceiver::RttSendFuncCallback)
{
    if (!Bus_CheckMapKeysExist(frame, {"data", "len"}))
        return false;

    auto data = Bus_GetMapValue(frame, "data").Str;
    auto len = Bus_GetMapValue(frame, "len").U16;

    // RTT线程不安全，为防止中断打断，直接使用临界区保护
    portENTER_CRITICAL();
    uint32_t bytesSent = SEGGER_RTT_Write(0, data, len);
    portEXIT_CRITICAL();

    if (Bus_CheckMapKeyExist(frame, "sent"))
        *((uint16_t *)Bus_GetMapValue(frame, "sent").Ptr) = (uint16_t)bytesSent;
    return true;
}

extern "C" void RTT_TaskCallback(void *argument)
{
    RttReceiver.Init((ConfItem *)argument);
    Depends_SignalFinished(svc_rtt);

    RttReceiver.Loop();
}
