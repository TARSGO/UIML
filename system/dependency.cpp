
#include <cmsis_os.h>
#include <etl/bitset.h>
#include <etl/vector.h>
#include <stddef.h>
#include <string.h>
#include <sys_conf.h>

#include "dependency.h"
#include "portmacro.h"

// 依赖位图，每个任务都会拥有一个依赖位图单元，各个依赖位图一个单元中至多有serviceNum+1个有效的位，
// 最低的部分当某个位为1时，表示该任务还没有向依赖单元告知其已经完成初始化；
// 最高位表示是否为此任务启用依赖初始化完毕的通知。
etl::vector<etl::bitset<serviceNum + 1, uint32_t>, serviceNum> DependencyBitmap;

// 互斥信号量，维护位图一致性
static SemaphoreHandle_t DependencyDataMutex;

// 未完成初始化的服务的个数，清零后会进行一系列系统清理
static size_t ServicesAwaitStartupCount;

static inline void Depends_EnableNotificationForService(Module module)
{
    // 为此任务启用依赖初始化完毕的通知
    DependencyBitmap[module].set(serviceNum);
}

static inline bool Depends_ServiceRequestedNotification(Module module)
{
    // 检查此任务是否启用了依赖初始化完毕的通知
    return DependencyBitmap[module].test(serviceNum);
}

static inline bool Depends_HasAllDependencyFinished(Module module)
{
    // 如果此任务没有启用依赖初始化完毕的通知，那么直接返回false，不要再试图唤醒对应线程
    if (!Depends_ServiceRequestedNotification(module))
        return false;

    // 检查某任务的依赖是否已经全部初始化完成
    // 复制初始化一个
    auto testUnit = etl::bitset(DependencyBitmap[module]);

    // 去掉最高位（启用通知标志）
    testUnit.reset(serviceNum);

    // 如果置位个数为0，说明所有依赖项都已经初始化完成
    return testUnit.none();
}

extern "C" void Depends_Init()
{
    // 初始化互斥信号量
    DependencyDataMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(DependencyDataMutex);

    // 初始化服务唤醒计数
    ServicesAwaitStartupCount = serviceNum;

    // 为所有有效位置1（初始状态没有任务完成初始化）
    for (auto unit : DependencyBitmap)
    {
        unit.set();
        unit.reset(serviceNum); // 取消掉最高位（启用通知）
    }
}

extern "C" void _Depends_WaitFor(Module waitingModule, Module *modules, size_t count)
{
    // 为其指定的所有依赖项置1，但在调用到这之前已经初始化过的（为0）除外
    // 先制作一个指定的所有依赖项都为1的掩码
    etl::bitset<serviceNum + 1, uint32_t> maskUnit;
    for (size_t i = 0; i < count; i++)
        maskUnit.set(modules[i]);

    // 获取信号量
    xSemaphoreTake(DependencyDataMutex, portMAX_DELAY);

    // 将其与等待者在库中的依赖项位图按位相与，即完成上述要求
    DependencyBitmap[waitingModule] &= maskUnit;

    // 为此任务启用依赖初始化完毕的通知
    Depends_EnableNotificationForService(waitingModule);

    // 归还信号量
    xSemaphoreGive(DependencyDataMutex);

    // 先检查一次是否已经全部初始化完成
    if (Depends_HasAllDependencyFinished(waitingModule))
        return;

    // 处理完之后暂停当前线程，等待依赖加载完成后被唤醒
    vTaskSuspend(NULL);
}

extern "C" void Depends_SignalFinished(Module module)
{
    // 获取信号量
    xSemaphoreTake(DependencyDataMutex, portMAX_DELAY);

    for (size_t i = 0; i < serviceNum; i++)
    {
        // 清掉自己在每个其他任务的依赖项中的位
        DependencyBitmap[i].reset(module);

        // 如果此时发现某个任务的依赖已经全部初始化完成，那么就唤醒它
        if (Depends_HasAllDependencyFinished((Module)i))
        {
            vTaskResume(serviceTaskHandle[i]);
            // 清除该任务的通知flag
            DependencyBitmap[i].reset(serviceNum);
            // 减去一个没有完成启动的服务
            ServicesAwaitStartupCount--;
        }
    }

    // 归还信号量
    xSemaphoreGive(DependencyDataMutex);

    // 如果所有定义了的服务都声称完成了启动，就开始系统清理
    // TODO:
    if (!ServicesAwaitStartupCount)
        ;
}
