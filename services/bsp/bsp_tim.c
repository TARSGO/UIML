#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "tim.h"
#include "stdio.h"

//TIM�����Ϣ
typedef struct 
{
	TIM_HandleTypeDef *htim;
	uint8_t number;
	SoftBusReceiverHandle fastHandle;
}TIMInfo;

//TIM��������
typedef struct 
{
	TIMInfo *timList;
	uint8_t timNum;
	uint8_t initFinished;
}TIMService;

//��������
void BSP_TIM_Init(ConfItem* dict);
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict);
void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict);
void BSP_TIM_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void BSP_TIM_TimerCallback(void const *argument);

TIMService timService={0};

//TIM����ص�����
void BSP_TIM_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_TIM_Init((ConfItem*)argument);
	portEXIT_CRITICAL();

	vTaskDelete(NULL);
}

//TIM��ʼ��
void BSP_TIM_Init(ConfItem* dict)
{
	//�����û����õ�tim����
	timService.timNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "tims/_";
		confName[5] = num + '0';
		if(Conf_ItemExist(dict, confName))
			timService.timNum++;
		else
			break;
	}
	timService.timList=pvPortMalloc(timService.timNum * sizeof(TIMInfo));
	for(uint8_t num = 0; num < timService.timNum; num++)
	{
		char confName[] = "tims/_";
		confName[5] = num + '0';
		BSP_TIM_InitInfo(&timService.timList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	for(uint8_t num = 0; num < timService.timNum; num++)
	{
		char confName[] = "tims/_";
		confName[5] = num + '0';
		BSP_TIM_StartHardware(&timService.timList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	//���Ļ���
	Bus_RegisterReceiver(NULL,BSP_TIM_SoftBusCallback,"/tim/set-pwm");
	timService.initFinished=1;
}

//��ʼ��TIM��Ϣ
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict)
{
	info->htim = Conf_GetPtr(dict,"htim",TIM_HandleTypeDef);
	info->number = Conf_GetValue(dict,"number",uint8_t,0);
	if(!strcmp(Conf_GetPtr(dict,"mode",char),"encode"))
	{
		char topic[14];
		sprintf(topic,"/tim%d/encode",info->number);
		info->fastHandle = Bus_CreateReceiverHandle(topic);
		uint32_t sendInterval = Conf_GetValue(dict,"interval",uint32_t,100);  
		//���������ʱ��
		osTimerDef(TIM, BSP_TIM_TimerCallback);
		osTimerStart(osTimerCreate(osTimer(TIM), osTimerPeriodic, info), sendInterval);
	}
}

//TIM��ʱ���ص�����
void BSP_TIM_TimerCallback(void const *argument)
{
	if(!timService.initFinished)
		return;
	TIMInfo* info = pvTimerGetTimerID((TimerHandle_t)argument); 
	uint32_t count=__HAL_TIM_GET_COUNTER(info->htim);
	Bus_FastBroadcastSend(info->fastHandle,{&count});
}

//����TIMӲ��
void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict)
{
	if(!strcmp(Conf_GetPtr(dict,"mode",char),"encode"))
	{
		HAL_TIM_Encoder_Start(info->htim,TIM_CHANNEL_ALL);
	}
	else if(!strcmp(Conf_GetPtr(dict,"mode",char),"pwm"))
	{
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_3);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_4);
	}
}


//TIM�����߻ص�
void BSP_TIM_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{

	if(!Bus_CheckMapKeys(frame,{"tim-x","channel-x","value"}))
		return;
 uint8_t timX = *(uint8_t *)Bus_GetMapValue(frame,"tim-x");
	uint8_t	channelX=*(uint8_t*)Bus_GetMapValue(frame,"channel-x");
	uint32_t pwmValue=*(uint32_t*)Bus_GetMapValue(frame,"value");
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number)
		{
			switch (channelX)
			{
			case 1:
				__HAL_TIM_SetCompare(timService.timList[num].htim, TIM_CHANNEL_1, pwmValue);
				break;
			case 2:
				__HAL_TIM_SetCompare(timService.timList[num].htim, TIM_CHANNEL_2, pwmValue);
				break;
			case 3:
				__HAL_TIM_SetCompare(timService.timList[num].htim, TIM_CHANNEL_3, pwmValue);
				break;
			case 4:
				__HAL_TIM_SetCompare(timService.timList[num].htim, TIM_CHANNEL_4, pwmValue);
				break;
			default:
				break;
			}
			
			break;
		}
	}
}

