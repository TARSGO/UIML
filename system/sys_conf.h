/*
	sys_conf.h
	系统配置文件
*/

#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"
#include "cmsis_os.h"

/****************** 外设配置 ******************/
//可使用Configuration Wizard配置

//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Config
//<q0>CAN
//<i>Select to include "can.h"
//<q1>UART
//<i>Select to include "usart.h"
//<q2>EXTI
//<i>Select to include "gpio.h"
//<q3>TIM
//<i>Select to include "tim.h"
//<q4>SPI
//<i>Select to include "spi.h"
#define CONF_CAN_ENABLE		1
#define CONF_USART_ENABLE	1
#define CONF_EXTI_ENABLE 1
#define CONF_TIM_ENABLE 1
#define CONF_SPI_ENABLE 1
//</h>
#if CONF_CAN_ENABLE
#include "can.h"
#endif
#if CONF_USART_ENABLE
#include "usart.h"
#endif
#if CONF_EXTI_ENABLE
#include "gpio.h"
#endif
#if CONF_TIM_ENABLE
#include "tim.h"
#endif
#if CONF_SPI_ENABLE
#include "spi.h"
#endif
// <<< end of configuration section >>>

/****************** 服务列表配置 (X-MACRO) ******************/

//每项格式(服务名,服务任务函数,任务优先级,任务栈大小)
#define SERVICE_LIST \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime,1024) \
	SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal,1024) \
	SERVICE(spi, BSP_SPI_TaskCallback, osPriorityNormal,1024) \
	SERVICE(tim, BSP_TIM_TaskCallback, osPriorityNormal,1024)	\
	/*SERVICE(ins, INS_TaskCallback, osPriorityNormal,128)*/\
	/*SERVICE(gimbal, Gimbal_TaskCallback, osPriorityNormal,256)*/\
	/*SERVICE(shooter, Shooter_TaskCallback, osPriorityNormal,256)*/\
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal,1024) \
	SERVICE(rc, RC_TaskCallback, osPriorityNormal,1024)\
	/*SERVICE(judge, Judge_TaskCallback, osPriorityNormal,128) */\
	/*SERVICE(sys, SYS_CTRL_TaskCallback, osPriorityNormal,256)*/
//SERVICE(exti, BSP_EXTI_TaskCallback, osPriorityNormal,256)

//服务列表枚举
typedef enum
{
	#define SERVICE(service,callback,priority,stackSize) service,
	SERVICE_LIST
	#undef SERVICE
	serviceNum
} Module;

/****************** 全局配置数据 ******************/

extern const char* configYaml;
extern PeriphHandle* peripheralHandles;
extern ConfItem* systemConfig;
extern osThreadId serviceTaskHandle[];

#endif

