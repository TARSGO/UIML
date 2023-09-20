
#include <stddef.h>
#include <string.h>
#include <sys_conf.h>
#include <cmsis_os.h>

#include "dependency.h"

// 依赖位图，每个任务都会拥有一个依赖位图单元，各个依赖位图的大小为((总任务数+1)/8+1)字节。
// 一个单元中至多有serviceNum+1个有效的位，最低的部分当某个位为1时，表示该任务还没有向依赖单元告知其已经完成初始化；
// 最高位表示是否为此任务启用依赖初始化完毕的通知。
static uint8_t *DependencyBitmap = NULL;
#define DEPENDENCY_BITMAP_UNIT_SIZE (serviceNum / 8 + 1) // 依赖位图的单位、即每个位图单元的大小，单位为字节。

static inline void Depends_EnableNotificationForService(Module module)
{
    // 为此任务启用依赖初始化完毕的通知
    DependencyBitmap[module * DEPENDENCY_BITMAP_UNIT_SIZE + DEPENDENCY_BITMAP_UNIT_SIZE - 1] |= 0x80;
}

static inline bool Depends_ServiceRequestedNotification(Module module)
{
    // 检查此任务是否启用了依赖初始化完毕的通知
    return DependencyBitmap[module * DEPENDENCY_BITMAP_UNIT_SIZE + DEPENDENCY_BITMAP_UNIT_SIZE - 1] & 0x80;
}

static inline void Depends_SetBitInDependencyUnit(uint8_t* unit, Module module)
{
    // 为指定的依赖项对应的位上置1
    unit[module / 8] |= (0x01 << (module % 8));
}

static inline bool Depends_HasAllDependencyFinished(Module module)
{
    // 如果此任务没有启用依赖初始化完毕的通知，那么直接返回false，不要再试图唤醒对应线程
    if (!Depends_ServiceRequestedNotification(module))
        return false;

    // 检查某任务的依赖是否已经全部初始化完成
    uint8_t andTemplate = 0;

    // 先按位与所有非最高字节的依赖位图单元
    for (size_t i = 0; i < DEPENDENCY_BITMAP_UNIT_SIZE - 1; i++)
        andTemplate |= DependencyBitmap[module * DEPENDENCY_BITMAP_UNIT_SIZE + i];

    // 去掉最高字节的最高位然后与进去
    andTemplate |= (DependencyBitmap[module * DEPENDENCY_BITMAP_UNIT_SIZE + DEPENDENCY_BITMAP_UNIT_SIZE - 1] & 0x7F);

    // 如果结果为0，说明所有依赖项都已经初始化完成
    return andTemplate == 0;
}

void Depends_Init()
{
    // 分配依赖位图
    DependencyBitmap = (uint8_t*)pvPortMalloc(DEPENDENCY_BITMAP_UNIT_SIZE * serviceNum);

    // 为所有有效位置1（初始状态没有任务完成初始化）
    // 一个单元占有多于一个字节才去全填充
    if (DEPENDENCY_BITMAP_UNIT_SIZE > 1)
        memset(DependencyBitmap, 0xFF, DEPENDENCY_BITMAP_UNIT_SIZE * serviceNum);
    // 把每个任务的单元最高那个字节里不包含依赖项位的部分置0
    // 用uint32是因为编译器警告
    uint32_t topByteMask = ~(0xFF << (serviceNum % 8));
    for (size_t i = 0; i < serviceNum; i++)
        DependencyBitmap[i * DEPENDENCY_BITMAP_UNIT_SIZE + DEPENDENCY_BITMAP_UNIT_SIZE - 1] &= topByteMask;
}

void _Depends_WaitFor(Module waitingModule, Module* modules, size_t count)
{
    // AND到该任务的依赖位图单元中的mask
    uint8_t mask[DEPENDENCY_BITMAP_UNIT_SIZE];
    memset(mask, 0x00, DEPENDENCY_BITMAP_UNIT_SIZE);

    // 为其指定的所有依赖项置1
    for (size_t i = 0; i < count; i++)
        Depends_SetBitInDependencyUnit(mask, modules[i]);

    // 把mask和依赖位图单元做AND运算
    for (size_t i = 0; i < DEPENDENCY_BITMAP_UNIT_SIZE; i++)
        DependencyBitmap[waitingModule * DEPENDENCY_BITMAP_UNIT_SIZE + i] &= mask[i];

    // 为此任务启用依赖初始化完毕的通知
    Depends_EnableNotificationForService(waitingModule);

    // 先检查一次是否已经全部初始化完成
    if (Depends_HasAllDependencyFinished(waitingModule))
        return;

    // 处理完之后暂停当前线程，等待依赖加载完成后被唤醒
    vTaskSuspend(NULL);
}

void Depends_SignalFinished(Module module)
{
    // 在位图中把自己的依赖项对应的位清零
    uint8_t mask = ~(0x01 << (module % 8));

    for (size_t i = 0; i < serviceNum; i++)
    {
        DependencyBitmap[i * DEPENDENCY_BITMAP_UNIT_SIZE + module / 8] &= mask;
        // 如果此时发现某个任务的依赖已经全部初始化完成，那么就唤醒它
        if (Depends_HasAllDependencyFinished(i))
            vTaskResume(serviceTaskHandle[i]);
    }
}
