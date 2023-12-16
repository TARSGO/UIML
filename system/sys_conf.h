/*
    sys_conf.h
    系统配置文件
*/

#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "cmsis_os.h"
#include "config.h"

/****************** 外设配置 ******************/
#if UIML_ENABLE_BSP_CAN
#include "can.h"
#endif
#if UIML_ENABLE_BSP_UART
#include "usart.h"
#endif
#if UIML_ENABLE_BSP_GPIO
#include "gpio.h"
#endif
#if UIML_ENABLE_BSP_TIM
#include "tim.h"
#endif
#if UIML_ENABLE_BSP_SPI
#include "spi.h"
#endif
// <<< end of configuration section >>>

// 服务列表枚举（自动生成于构建目录中）
#include "uiml_service_list_header.h"

typedef enum
{
#define SERVICE(service, callback, priority, stackSize) svc_##service,
#define SERVICEX(service, callback, priority, stackSize)
    SERVICE_LIST
#undef SERVICEX
#undef SERVICE
        serviceNum,

    // 禁用的任务。为了让用到禁用的任务的枚举值的代码通过编译，依然需要在这里定义它们的枚举值。
    // 但它们位于serviceNum之后，不影响其他基于启用服务数量的逻辑。
#define SERVICE(service, callback, priority, stackSize)
#define SERVICEX(service, callback, priority, stackSize) svc_##service,
    SERVICE_LIST
#undef SERVICEX
#undef SERVICE

} Module;

// 在以后用到服务列表X-Macro时将禁用的服务除名，这样它们不会被链接。
#define SERVICEX(service, callback, priority, stackSize)
/****************** 全局配置数据 ******************/

extern const char *configYaml;
extern PeriphHandle *peripheralHandles;
extern ConfItem *systemConfig;
extern osThreadId serviceTaskHandle[];

#endif
