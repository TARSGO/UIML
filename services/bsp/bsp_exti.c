#include "softbus.h"
#include  "cmsis_os.h"
#include "config.h"
#include "gpio.h"
#include "arm_math.h"
//EXTI GPIO��Ϣ
typedef struct
{
	uint16_t pin;
	SoftBusFastHandle fastHandle;
}EXTIInfo;

//EXTI��������
typedef struct {
	EXTIInfo* extiList;
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
	for(uint16_t i=0;i<extiService.extiNum;i++)
	{
		EXTIInfo* extiInfo = &extiService.extiList[i];
		if(GPIO_Pin==extiInfo ->pin)
		{
			SoftBus_FastPublish(extiInfo->fastHandle,{""});
			break;
		}
	}
}
//EXTI����ص�����
void BSP_EXTI_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_EXTI_Init((ConfItem *)argument);
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
		char confName[] = "extis/_";
		confName[6] = num + '0';
		if(Conf_ItemExist(dict, confName))
			extiService.extiNum++;
		else
			break;
	}
	extiService.extiList=pvPortMalloc(extiService.extiNum * sizeof(EXTIInfo));
	for(uint8_t num = 0; num < extiService.extiNum; num++)
	{
		char confName[] = "extis/_";
		confName[6] = num + '0';
		BSP_EXIT_InitInfo(&extiService.extiList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	extiService.initFinished=1;
}

//��ʼ��EXTI��Ϣ
void BSP_EXIT_InitInfo(EXTIInfo* info, ConfItem* dict)
{
	info->pin = Conf_GetValue(dict, "pin-x", uint16_t, 0);
	char topic[] = "/exti/pin_";
	topic[9] = info->pin + '0';
	//����ӳ����GPIO_PIN=2^pin
	info->pin=pow(2,info->pin);
	info->fastHandle = SoftBus_CreateFastHandle(topic);
}


