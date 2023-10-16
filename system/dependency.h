
#pragma once

#include "sys_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化模块启动依赖系统。只应该在默认线程中调用一次。
 * 
 */
void Depends_Init();

/**
 * @brief 向依赖系统告知某模块自身已经启动完成，并唤醒所有等待条件满足的线程。
 * 
 * @param module 调用方自身的模块枚举值
 */
void Depends_SignalFinished(Module module);

/**
 * @brief Depends_WaitFor API的实现。
 * 
 * @param waitingModule 调用方自身的模块枚举值
 * @param modules 等待的模块枚举值数组
 * @param count 模块枚举值数组长度
 *
 * @note 请使用Depends_WaitFor API。
 */
void _Depends_WaitFor(Module waitingModule, Module* modules, size_t count);

/**
 * @brief 等待多个模块启动完成。
 * 
 * @param waitingModule 调用方自身的模块枚举值
 * @param Dependencies 依赖的模块枚举值数组
 *
 * @note 该函数会阻塞调用方线程，直到所有依赖的模块都启动完成。
 * @note 该API会在C中展开为宏调用确定数组长度，在C++中展开为函数调用并使用initializer_list。
 */
#ifndef __cplusplus
#define Depends_WaitFor(waitingModule,Dependencies) _Depends_WaitFor(waitingModule,Dependencies,sizeof(Dependencies)/sizeof(Module))
#else
#include <initializer_list>
inline void Depends_WaitFor(Module waitingModule, std::initializer_list<Module> modules)
{
    _Depends_WaitFor(waitingModule, (Module*)modules.begin(), modules.size());
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif
