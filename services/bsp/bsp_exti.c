#include "softbus.h"
#include  "cmsis_os.h"
#include "config.h"
#include "gpio.h"
#include "stdio.h"

//EXTI GPIO��Ϣ
typedef struct
{
	GPIO_TypeDef* GPIOX;
	uint16_t pin;
	SoftBusReceiverHandle fastHandle;
}EXTIInfo;

//EXTI��������
typedef struct {
	EXTIInfo extiList[16];
	uint8_t extiNum;
	uint8_t initFinished;
}EXTIService;

//��������
void BSP_EXTI_Init(ConfItem* dict);
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict);

EXTIService extiService={0};

//�ⲿ�жϷ�����
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(!extiService.initFinished)
		return;  
	uint8_t pin = 31 - __clz((uint32_t)GPIO_Pin);//ʹ���ں˺���__clz����GPIO_Pinǰ��0�ĸ������Ӷ��õ��ж��ߺ�
	EXTIInfo* extiInfo = &extiService.extiList[pin];
	GPIO_PinState state = HAL_GPIO_ReadPin(extiInfo->GPIOX, GPIO_Pin);
	Bus_FastBroadcastSend(extiInfo->fastHandle,{&state});
}
//EXTI����ص�����
void BSP_EXTI_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_EXTI_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}
//EXTI��ʼ��
void BSP_EXTI_Init(ConfItem* dict)
{
	//�����û����õ�exit����
	extiService.extiNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[9];
		sprintf(confName,"extis/%d",num);
		if(Conf_ItemExist(dict, confName))
			extiService.extiNum++;
		else
			break;
	}

	for(uint8_t num = 0; num < extiService.extiNum; num++)
	{
		char confName[9] = "extis/_";
		sprintf(confName,"extis/%d",num);
		BSP_EXIT_InitInfo(extiService.extiList, Conf_GetPtr(dict, confName, ConfItem));
	}
	extiService.initFinished=1;
}

//��ʼ��EXTI��Ϣ
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict)
{
	uint8_t pin = Conf_GetValue(dict, "pin-x", uint8_t, 0);
	info[pin].GPIOX = Conf_GetPtr(dict, "gpio-x", GPIO_TypeDef);
	char name[12];
	sprintf(name,"/exti/pin%d",pin);
	//����ӳ����GPIO_PIN=2^pin
	info[pin].pin = 1 << pin;
	info[pin].fastHandle = Bus_CreateReceiverHandle(name);
}


