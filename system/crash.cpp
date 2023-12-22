
#include "softbus.h"
#include "sys_conf.h"
#include <cstdio>

extern "C" [[noreturn]] void UimlCrashSystem(
    const char *reason, void *param1, void *param2, void *param3, void *param4)
{
    // 首先先产生急停事件
    Bus_PublishTopic("/system/stop", {{NULL, NULL}});

    // 找到崩溃的服务，由于本函数运行在崩溃的线程里，可以直接对比
    auto threadId = osThreadGetId();
    uint32_t serviceId = serviceNum;
    for (size_t i = 0; i < serviceNum; i++)
    {
        if (serviceTaskHandle[i] == threadId)
        {
            serviceId = i;
            break;
        }
    }

    // 然后试图用 printf 打印崩溃信息，如果开了RTT，就会打进RTT上行缓冲区
    printf(" *** UIML SYSTEM CRASH\n"
           "  Crashed service: %s\n"
           "  Reason: %s\n"
           "\n"
           "  (%p, %p, %p, %p)\n\n",
           serviceNames[serviceId],
           reason,
           param1,
           param2,
           param3,
           param4);

    // 关闭整机中断
    __disable_irq();

    // 尝试产生蜂鸣声
    Bus_RemoteFuncCall("/beep/cmd", {{"freq", {.U32 = 880}}});

    // 死循环
    while (true)
        ;
}
