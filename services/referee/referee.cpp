
/*
 * 裁判系统信息交互程序，负责：
 *      接收裁判系统串口包、解码、广播裁判系统事件
 *      发送向裁判系统上行传输的包
 *
 */

#include "config.h"
#include "dependency.h"
#include "portable.h"
#include "softbus.h"
#include "staticstr.h"
#include "sys_conf.h"

static struct _Referee
{

    // 接收缓冲区
    uint8_t *recvPacketBuf;
    uint16_t recvPacketBufSize;

    // 接收状态机
    struct
    {
        enum __packed PacketIdentifyState
        {
            WaitingSOF,    // 在等待帧头
            GettingHeader, // 在接收帧头
            Skipping,      // 在跳过指定的丢弃CmdId
            GettingBody,   // 在接收整个帧（直到收完CRC16为止）
        } state;
        uint16_t bytesToSkip; // （遇到了需要整个跳过的包的情况下）还有多少字节需要跳过
        uint16_t
            bytesReceived; // （收到不完整帧时）已经收到的字节数。定义包含帧头、CmdId、data、帧尾CRC16等。
        uint16_t dataLength;
        ; // （收到不完整帧时）帧头指定的data段字节数，如果已收字节数不足帧头，此字段未定义
    } recvFsm;
} Referee;

BUS_TOPICENDPOINT(Referee_UartReceive);

void Referee_Init(ConfItem *conf)
{
    // 初始化接收缓冲区
    auto recvNode = Conf_GetNode(conf, "receive");
    // 接收缓冲区长度由于除了车间通信没有什么很大变化的可能，所以暂时没有放进配置文件
    Referee.recvPacketBufSize = 64;
    Referee.recvPacketBuf = (uint8_t *)pvPortMalloc(Referee.recvPacketBufSize);
    auto recvUartStr = Conf_GetValue(conf, "uart", const char *, nullptr);

    // 订阅UART接收话题
    {
        StaticString<16> recvUartTopic;
        recvUartTopic << '/' << recvUartStr << "/recv";
        Bus_SubscribeTopic(nullptr, Referee_UartReceive, recvUartTopic);
    }

    // TODO: UI发送、多车通信
}

extern "C" void Referee_TaskMain(void *argument)
{
    Depends_WaitFor(svc_referee, {svc_uart});
    Referee_Init((ConfItem *)argument);
    Depends_SignalFinished(svc_referee);

    vTaskDelete(NULL);
}

BUS_TOPICENDPOINT(Referee_UartReceive)
{
}
