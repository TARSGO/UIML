
#include "lettershell/shell.h"
#include "config.h"
#include "dependency.h"
#include "portable.h"
#include "softbus.h"
#include "sys_conf.h"
#include <cmsis_os.h>
#include <cstring>

static constexpr uint32_t RxSignal = 1;

class ShellTask
{
  public:
    ShellTask(){};

    void Init(ConfItem *conf)
    {
        U_C(conf);

        shellThreadId = osThreadGetId();

        // 创建输入缓冲区，默认32字节
        inputBufferSize = Conf["input-buf-size"].get<uint16_t>(32);
        inputBuffer = (char *)pvPortMalloc(inputBufferSize);
        dataLength = 0;

        // 初始化LetterShell
        // 缓冲区，按说明，默认给512字节是可以的
        auto letterShellBufferSize = Conf["shell-buf-size"].get<uint16_t>(256);
        auto letterShellBuffer = (char *)pvPortMalloc(letterShellBufferSize);

        letterShellObj.read = NULL; // 不需要shell自行读取
        letterShellObj.write = letterShellWrite;
        shellInit(&letterShellObj, letterShellBuffer, letterShellBufferSize);

        // 从RTT获取输入
        Bus_SubscribeTopic(this, IncomingDataCallback, "/rtt/recv");
    }

    [[noreturn]] void Loop()
    {
        while (true)
        {
            osSignalWait(RxSignal, osWaitForever);

            for (size_t i = 0; i < dataLength; i++)
                shellHandler(&letterShellObj, inputBuffer[i]);
            dataLength = 0;
        }
    }

  private:
    static BUS_TOPICENDPOINT(IncomingDataCallback); // 收到下行数据回调(帧格式与UART，RTT兼容)
    static short letterShellWrite(char *data, unsigned short len); // LetterShell写入函数

  private:
    Shell letterShellObj; // LetterShell

    // 输入缓冲区
    char *inputBuffer;
    uint16_t dataLength, inputBufferSize;

    // Shell线程ID，激发线程用
    osThreadId shellThreadId;

} shell;

extern "C" void Shell_TaskCallback(void *argument)
{
    Depends_WaitFor(svc_shell, {svc_rtt});
    shell.Init((ConfItem *)argument);
    Depends_SignalFinished(svc_shell);

    shell.Loop();
}

BUS_TOPICENDPOINT(ShellTask::IncomingDataCallback)
{
    auto self = (ShellTask *)bindData;
    auto data = Bus_GetListValue(frame, 0).Ptr;
    auto len = Bus_GetListValue(frame, 1).U16;

    memcpy((void *)self->inputBuffer, data, std::min(self->inputBufferSize, len));
    self->dataLength = len;

    osSignalSet(self->shellThreadId, RxSignal); // 异步处理
}

short ShellTask::letterShellWrite(char *data, unsigned short len)
{
    uint16_t sentBytes = len;

    // 调用RTT发送
    Bus_RemoteFuncCall(
        "/rtt/send",
        {{"data", {.Str = data}}, {"len", {.U16 = len}}, {"sent", {.Ptr = &sentBytes}}});

    return sentBytes;
}
