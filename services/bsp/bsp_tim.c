#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "tim.h"
#include <stdio.h>

#ifndef LIMIT
#define LIMIT(x,min,max) (x)=(((x)<=(min))?(min):(((x)>=(max))?(max):(x)))
#endif

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
bool BSP_TIM_SetDutyCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_TIM_GetEncodeCallback(const char* name, SoftBusFrame* frame, void* bindData);

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
		char confName[8] = {0};
		sprintf(confName, "tims/%d", num);
		if(Conf_ItemExist(dict, confName))
			timService.timNum++;
		else
			break;
	}
	timService.timList=pvPortMalloc(timService.timNum * sizeof(TIMInfo));
	for(uint8_t num = 0; num < timService.timNum; num++)
	{
		char confName[8] = {0};
		sprintf(confName, "tims/%d", num);
		BSP_TIM_InitInfo(&timService.timList[num], Conf_GetPtr(dict, confName, ConfItem));
		BSP_TIM_StartHardware(&timService.timList[num], Conf_GetPtr(dict, confName, ConfItem));
	}

	//ע�����
	Bus_RegisterRemoteFunc(NULL,BSP_TIM_SetDutyCallback,"/tim/pwm/set-duty");
	//ע��Զ�̷���
	Bus_RegisterRemoteFunc(NULL,BSP_TIM_GetEncodeCallback,"/tim/encode");
	timService.initFinished=1;
}

//��ʼ��TIM��Ϣ
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict)
{
	info->htim = Conf_GetPtr(dict,"htim",TIM_HandleTypeDef);
	info->number = Conf_GetValue(dict,"number",uint8_t,0);
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


//TIM�����߹㲥�ص�
bool BSP_TIM_SetDutyCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"tim-x","channel-x","duty"}))
		return;
	uint8_t timX = *(uint8_t *)Bus_GetMapValue(frame,"tim-x");
	uint8_t	channelX=*(uint8_t*)Bus_GetMapValue(frame,"channel-x");
	float duty=*(float*)Bus_GetMapValue(frame,"duty");
	LIMIT(duty,0,1);
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number)
		{
			uint32_t pwmValue = duty * __HAL_TIM_GetAutoreload(timService.timList[num].htim);
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
			return true;
			break;
		}
	}
	return false;
}

//TIM������Զ�̷���ص�
bool BSP_TIM_GetEncodeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"tim-x","count"}))
		return false;
	uint8_t timX = *(uint8_t *)Bus_GetMapValue(frame,"tim-x");
	uint32_t *count = (uint32_t*)Bus_GetMapValue(frame,"count");
	uint32_t *autoReload=NULL; 
	if(Bus_IsMapKeyExist(frame,"auto-reload"))
		autoReload = (uint32_t *)Bus_GetMapValue(frame,"auto-reload");
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number)
		{
			*count=__HAL_TIM_GetCounter(timService.timList[num].htim);
			if(autoReload)
				*autoReload=__HAL_TIM_GetAutoreload(timService.timList[num].htim);
			return true;
		}
	}
	return false;
}

