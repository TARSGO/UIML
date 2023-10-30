#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "usart.h"
#include "strict.h"
#include "dependency.h"

#include <string.h>

#define UART_IRQ \
	IRQ_FUN(USART1_IRQHandler, 1) \
	IRQ_FUN(USART2_IRQHandler, 2) \
	IRQ_FUN(USART3_IRQHandler, 3) \
	IRQ_FUN(UART4_IRQHandler, 4) \
	IRQ_FUN(UART5_IRQHandler, 5) \
	IRQ_FUN(USART6_IRQHandler, 6) \
	IRQ_FUN(UART7_IRQHandler, 7) \
	IRQ_FUN(UART8_IRQHandler, 8)
	
#define UART_TOTAL_NUM 8

//UART句柄信息
typedef struct {
	UART_HandleTypeDef* huart;
	uint8_t number; //uartX中的X
	struct 
	{
		uint8_t *data;
		uint16_t maxBufSize;
		uint16_t pos;
	}recvBuffer;
	SoftBusReceiverHandle fastHandle;
}UARTInfo;

//UART服务数据
typedef struct {
	UARTInfo uartList[UART_TOTAL_NUM];
	uint8_t uartNum;
	uint8_t initFinished;
}UARTService;

UARTService uartService = {0};
//函数声明
void BSP_UART_Init(ConfItem* dict);
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict);
bool BSP_UART_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_UART_ItCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_UART_DMACallback(const char* name, SoftBusFrame* frame, void* bindData);
void BSP_UART_InitRecvBuffer(UARTInfo* info);
void BSP_UART_RecvConf(UARTInfo* info);
void BSP_UART_RecvConf_DMA(UARTInfo* info, bool isDoubleBuffer);
void BSP_UART_RecvHanlder(UARTInfo* uartInfo);
void BSP_UART_RecvHanlder_DMA(UARTInfo* uartInfo);

//uart接收中断回调函数
void BSP_UART_IRQCallback(uint8_t huartX)
{
	UARTInfo* uartInfo = &uartService.uartList[huartX - 1];
	
	//如果初始化未完成则清除标志位
	if(!uartService.initFinished)
	{              
		(void)uartInfo->huart->Instance->SR; 
		(void)uartInfo->huart->Instance->DR;
		return;
	}

	//判断是否为DMA模式
	if(!(uartInfo->huart->Instance->CR3 & USART_CR3_DMAR))
		BSP_UART_RecvHanlder(uartInfo);
	else	
		BSP_UART_RecvHanlder_DMA(uartInfo);
}

//uart任务回调函数
void BSP_UART_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_UART_Init((ConfItem*)argument);
	portEXIT_CRITICAL();

	Depends_SignalFinished(svc_uart);
	
	vTaskDelete(NULL);
}

void BSP_UART_Init(ConfItem* dict)
{
	//计算用户配置的uart数量
	uartService.uartNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		if(Conf_ItemExist(dict, confName))
		{
			//uartService.uartNum++;
			// 序号存在，读取UART外设信息
			ConfItem* uartDict = Conf_GetNode(dict, confName);
			uint8_t uartX = Conf_GetValue(uartDict, "number", uint8_t, 0);

			// 初始化各UART外设信息
			BSP_UART_InitInfo(&uartService.uartList[uartX - 1], Conf_GetNode(dict, confName));
		}
		else
			break;
	}
	//初始化各uart信息
	// for(uint8_t i = 0; i < uartService.uartNum; i++)
	// {
	// 	char confName[] = "uarts/_";
	// 	confName[6] = i + '0';
	// 	BSP_UART_InitInfo(&uartService.uartList[i], Conf_GetNode(dict, confName));
	// }

	//注册远程函数
	Bus_RemoteFuncRegister(NULL, BSP_UART_BlockCallback, "/uart/trans/block");
	Bus_RemoteFuncRegister(NULL, BSP_UART_ItCallback, "/uart/trans/it");
	Bus_RemoteFuncRegister(NULL, BSP_UART_DMACallback, "/uart/trans/dma");
	uartService.initFinished = 1;
}

//初始化uart信息
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict)
{
	uint8_t number = Conf_GetValue(dict, "number", uint8_t, 0);
	info->number = number;

	char uartName[] = "uart_";
	uartName[4] = number + '0';
	info->huart = Conf_GetPeriphHandle(uartName, UART_HandleTypeDef);
	UIML_FATAL_ASSERT((info->huart != NULL), "Missing UART Device");

	info->recvBuffer.maxBufSize = Conf_GetValue(dict, "max-recv-size", uint16_t, 1);
	char name[] = "/uart_/recv";
	name[5] = info->number + '0';
	info->fastHandle = Bus_GetFastTopicHandle(name);
	//初始化接收缓冲区
	BSP_UART_InitRecvBuffer(info);
	//开启uart空闲中断
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_IDLE);

	//判断是否为DMA接收
	if(Conf_ItemExist(dict, "dma"))		//若为DMA接收，进入DMA配置函数
		BSP_UART_RecvConf_DMA(info, Conf_GetValue(dict, "dma/double-buffer", bool, 0));
	else								//若否，则使能RXNE中断
		__HAL_UART_ENABLE_IT(info->huart, UART_IT_RXNE);
}

//uart DMA接收配置
void BSP_UART_RecvConf_DMA(UARTInfo* info, bool isDoubleBuffer)
{
	//判断是否为双缓冲区模式
	if(isDoubleBuffer)		//若为双缓冲区模式，则进行相应配置
	{
		//使能DMA串口接收
		SET_BIT(info->huart->Instance->CR3, USART_CR3_DMAR);
		//配置DMA双缓冲区
		HAL_DMAEx_MultiBufferStart(info->huart->hdmarx, 
									(uint32_t)(&info->huart->Instance->DR),
									(uint32_t)(info->recvBuffer.data),
									(uint32_t)(info->recvBuffer.data + info->recvBuffer.maxBufSize / 2),
									info->recvBuffer.maxBufSize);
	}
	else
	{
		HAL_UART_Receive_DMA(info->huart, info->recvBuffer.data, info->recvBuffer.maxBufSize);
	}
}

//初始化接收缓冲区
void BSP_UART_InitRecvBuffer(UARTInfo* info)
{
   	info->recvBuffer.pos=0;
	info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
}

//阻塞回调
bool BSP_UART_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeysExist(frame, {"uart-x", "data", "trans-size", "timeout"}))
		return false;

	uint8_t uartX = Bus_GetMapValue(frame, "uart-x").U8;
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data").Ptr;
	uint16_t transSize = Bus_GetMapValue(frame, "trans-size").U16;
	uint32_t timeout = Bus_GetMapValue(frame, "timeout").U32;
	HAL_UART_Transmit(uartService.uartList[uartX - 1].huart,data,transSize,timeout);
	return true;
}

//中断发送回调
bool BSP_UART_ItCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeysExist(frame, {"uart-x","data","trans-size"}))
		return false;

	uint8_t uartX = Bus_GetMapValue(frame, "uart-x").U8;
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data").Ptr;
	uint16_t transSize = Bus_GetMapValue(frame, "trans-size").U16;
	HAL_UART_Transmit_IT(uartService.uartList[uartX - 1].huart,data,transSize);
	return true;
}

//DMA发送回调
bool BSP_UART_DMACallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeysExist(frame, {"uart-x","data","trans-size"}))
		return false;

	uint8_t uartX = Bus_GetMapValue(frame, "uart-x").U8;
	uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data").Ptr;
	uint16_t transSize = Bus_GetMapValue(frame, "trans-size").U16;
	HAL_UART_Transmit_DMA(uartService.uartList[uartX - 1].huart, data, transSize);
	return true;
}

//非DMA模式下接收处理函数
void BSP_UART_RecvHanlder(UARTInfo* uartInfo)
{
	//清除ORE标志位
	if((__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_ORE) != RESET))
	{
		__HAL_UART_CLEAR_OREFLAG(uartInfo->huart);
	}

	//清除PE标志位
	if((__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_PE) != RESET))
	{
		__HAL_UART_CLEAR_PEFLAG(uartInfo->huart);
	}

	//若为RXNE中断则接收数据
	if (__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_RXNE))
	{
		//防止数组越界
		if(uartInfo->recvBuffer.pos < uartInfo->recvBuffer.maxBufSize)
			uartInfo->recvBuffer.data[uartInfo->recvBuffer.pos++] = uartInfo->huart->Instance->DR;
	}

	//若为IDLE中断则广播数据
	if(__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_IDLE) != RESET)
	{
		__HAL_UART_CLEAR_IDLEFLAG(uartInfo->huart);
		uint16_t recSize=uartInfo->recvBuffer.pos; //此时pos值为一帧数据的长度
		Bus_PublishTopicFast(uartInfo->fastHandle, {uartInfo->recvBuffer.data, &recSize}); //空闲中断为一帧，发送一帧数据
		uartInfo->recvBuffer.pos = 0;
	}
}

//DMA模式下接收处理函数
void BSP_UART_RecvHanlder_DMA(UARTInfo* uartInfo)
{
	//判断是否为IDLE中断
	if(__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_IDLE) != RESET)
		__HAL_UART_CLEAR_IDLEFLAG(uartInfo->huart);
	else
		return;

	//失能DMA
	__HAL_DMA_DISABLE(uartInfo->huart->hdmarx);
	//重新设定数据长度
	__HAL_DMA_SET_COUNTER(uartInfo->huart->hdmarx, uartInfo->recvBuffer.maxBufSize);

	//判断是否为双缓冲区
	if(uartInfo->huart->hdmarx->Instance->CR & DMA_SxCR_DBM)
	{
		//切换缓冲区
		uartInfo->huart->hdmarx->Instance->CR ^= DMA_SxCR_CT;
		//使能DMA
		__HAL_DMA_ENABLE(uartInfo->huart->hdmarx);
		//判断当前缓冲区
		if (uartInfo->huart->hdmarx->Instance->CR & DMA_SxCR_CT)
		{
			//广播接收到的数据
			Bus_PublishTopicFast(uartInfo->fastHandle, {uartInfo->recvBuffer.data});
		}
		else
		{
			//广播接收到的数据
			Bus_PublishTopicFast(uartInfo->fastHandle, {uartInfo->recvBuffer.data + uartInfo->recvBuffer.maxBufSize / 2});
		}
	}
	else
	{
		//广播接收到的数据
		Bus_PublishTopicFast(uartInfo->fastHandle, {uartInfo->recvBuffer.data});
		//使能DMA
		__HAL_DMA_ENABLE(uartInfo->huart->hdmarx);
	}
}

//生成中断服务函数
#define IRQ_FUN(irq, number) \
void irq(void) \
{ \
	BSP_UART_IRQCallback(number); \
	HAL_UART_IRQHandler(uartService.uartList[number-1].huart); \
}

UART_IRQ
#undef IRQ_FUN
