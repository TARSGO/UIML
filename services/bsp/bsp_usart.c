#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "usart.h"

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

//UART�����Ϣ
typedef struct {
	UART_HandleTypeDef* huart;
	uint8_t number; //uartX�е�X
	struct 
	{
		uint8_t *data;
		uint16_t maxBufSize;
		uint16_t pos;
	}recvBuffer;
	SoftBusFastTopicHandle fastHandle;
}UARTInfo;

//UART��������
typedef struct {
	UARTInfo uartList[UART_TOTAL_NUM];
	uint8_t uartNum;
	uint8_t initFinished;
}UARTService;

UARTService uartService = {0};
//��������
void BSP_UART_Init(ConfItem* dict);
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict);
void BSP_UART_Start_IT(UARTInfo* info);
void BSP_UART_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_UART_InitRecvBuffer(UARTInfo* info);

//uart�����жϻص�����
void BSP_UART_IRQCallback(uint8_t huartX)
{
	UARTInfo* uartInfo = &uartService.uartList[huartX - 1];
	
	if(!uartService.initFinished)//�����ʼ��δ����������־λ
	{              
		(void)uartInfo->huart->Instance->SR; 
		(void)uartInfo->huart->Instance->DR;
		return;
	}
	
	if (__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_RXNE))
	{
		//��ֹ����Խ��
		if(uartInfo->recvBuffer.pos < uartInfo->recvBuffer.maxBufSize)
			uartInfo->recvBuffer.data[uartInfo->recvBuffer.pos++] = uartInfo->huart->Instance->DR;
	}
		
	if (__HAL_UART_GET_FLAG(uartInfo->huart, UART_FLAG_IDLE))
	{
		/* clear idle it flag avoid idle interrupt all the time */
		__HAL_UART_CLEAR_IDLEFLAG(uartInfo->huart);
		uint16_t recSize=uartInfo->recvBuffer.pos;
		SoftBus_FastPublish(uartInfo->fastHandle, {uartInfo->recvBuffer.data, &recSize});
		uartInfo->recvBuffer.pos = 0;
	}
}

//uart����ص�����
void BSP_UART_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_UART_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}

void BSP_UART_Init(ConfItem* dict)
{
	//�����û����õ�uart����
	uartService.uartNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		if(Conf_ItemExist(dict, confName))
			uartService.uartNum++;
		else
			break;
	}
	//��ʼ����uart��Ϣ
	for(uint8_t num = 0; num < uartService.uartNum; num++)
	{
		char confName[] = "uarts/_";
		confName[6] = num + '0';
		BSP_UART_InitInfo(uartService.uartList, Conf_GetPtr(dict, confName, ConfItem));
	}

	//���Ļ���
	SoftBus_MultiSubscribe(NULL, BSP_UART_SoftBusCallback, {"/uart/trans/it","/uart/trans/dma","/uart/trans/block"});
	uartService.initFinished = 1;
}

//��ʼ��uart��Ϣ
void BSP_UART_InitInfo(UARTInfo* info, ConfItem* dict)
{
	uint8_t number = Conf_GetValue(dict, "uart-x", uint8_t, 0);
	info[number-1].huart = Conf_GetPtr(dict, "huart", UART_HandleTypeDef);
	info[number-1].number = number;
	info[number-1].recvBuffer.maxBufSize = Conf_GetValue(dict, "maxRecvSize", uint16_t, 1);
	char topic[] = "/uart_/recv";
	topic[5] = info[number-1].number + '0';
	info[number-1].fastHandle = SoftBus_CreateFastTopicHandle(topic);
	//��ʼ�����ջ�����
	BSP_UART_InitRecvBuffer(&info[number-1]);
	//����uart�ж�
	BSP_UART_Start_IT(&info[number-1]);
}

//����uart�ж�
void BSP_UART_Start_IT(UARTInfo* info)
{
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(info->huart, UART_IT_IDLE);
}
//��ʼ�����ջ�����
void BSP_UART_InitRecvBuffer(UARTInfo* info)
{
   	info->recvBuffer.pos=0;
	info->recvBuffer.data = pvPortMalloc(info->recvBuffer.maxBufSize);
    memset(info->recvBuffer.data,0,info->recvBuffer.maxBufSize);
}

//�����߻ص�
void BSP_UART_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	if(!SoftBus_CheckMapKeys(frame, {"uart-x","data","transSize"}))
		return;

	uint8_t uartX = *(uint8_t*)SoftBus_GetMapValue(frame, "uart-x");
	uint8_t* data = (uint8_t*)SoftBus_GetMapValue(frame, "data");
	uint16_t transSize = *(uint16_t*)SoftBus_GetMapValue(frame, "transSize");
	for(uint8_t i = 0; i < uartService.uartNum; i++)
	{
		UARTInfo* info = &uartService.uartList[i];
		if(info->number == uartX)
		{
			if(!strcmp(topic, "/uart/trans/it")) //�жϷ���
				HAL_UART_Transmit_IT(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/dma")) //dma����
				HAL_UART_Transmit_DMA(info->huart,data,transSize);
			else if(!strcmp(topic, "/uart/trans/block") && SoftBus_IsMapKeyExist(frame, "timeout"))//����ʽ����
			{
				uint32_t timeout = *(uint32_t*)SoftBus_GetMapValue(frame, "timeout");
				HAL_UART_Transmit(info->huart,data,transSize,timeout);
			}
		}
	}
}

//�����жϷ�����
#define IRQ_FUN(irq, number) \
void irq(void) \
{ \
	BSP_UART_IRQCallback(number); \
	HAL_UART_IRQHandler(uartService.uartList[number-1].huart); \
}

UART_IRQ
#undef IRQ_FUN
