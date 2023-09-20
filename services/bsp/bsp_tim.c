#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "tim.h"
#include "strict.h"

#ifndef LIMIT
#define LIMIT(x,min,max) (x)=(((x)<=(min))?(min):(((x)>=(max))?(max):(x)))
#endif

//TIM句柄信息
typedef struct 
{
	TIM_HandleTypeDef *htim;
	uint8_t number;
	SoftBusReceiverHandle fastHandle;
}TIMInfo;

//TIM服务数据
typedef struct 
{
	TIMInfo *timList;
	uint8_t timNum;
	uint8_t initFinished;
}TIMService;

//函数声明
void BSP_TIM_Init(ConfItem* dict);
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict);
void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict);
bool BSP_TIM_SetDutyCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_TIM_GetEncodeCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool BSP_TIM_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);

TIMService timService={0};

void BSP_TIM_UpdateCallback(TIM_HandleTypeDef *htim)
{
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(htim == timService.timList[num].htim) //找到对应的TIM
		{
			TIMInfo* timInfo = &timService.timList[num];
			Bus_FastBroadcastSend(timInfo->fastHandle,{{""}});
		}
	}
}

//TIM任务回调函数
void BSP_TIM_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	BSP_TIM_Init((ConfItem*)argument);
	portEXIT_CRITICAL();

	vTaskDelete(NULL);
}

//TIM初始化
void BSP_TIM_Init(ConfItem* dict)
{
	//计算用户配置的tim数量
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
		BSP_TIM_InitInfo(&timService.timList[num], Conf_GetNode(dict, confName));
		BSP_TIM_StartHardware(&timService.timList[num], Conf_GetNode(dict, confName));
	}
	//注册远程服务
	Bus_RegisterRemoteFunc(NULL,BSP_TIM_SettingCallback,"/tim/setting");
	Bus_RegisterRemoteFunc(NULL,BSP_TIM_SetDutyCallback,"/tim/pwm/set-duty");
	Bus_RegisterRemoteFunc(NULL,BSP_TIM_GetEncodeCallback,"/tim/encode");
	timService.initFinished=1;
}

//初始化TIM信息
void BSP_TIM_InitInfo(TIMInfo* info,ConfItem* dict)
{
	info->number = Conf_GetValue(dict,"number",uint8_t,0);

	char timName[6] = {0}; // "tim13\0"
	sprintf(timName, "tim%d", info->number);
	info->htim = Conf_GetPeriphHandle(timName, TIM_HandleTypeDef);
	UIML_FATAL_ASSERT(info->htim != NULL, "Missing TIM Device");
	
	char name[17] = {0};
	sprintf(name, "/tim%d/update", info->number);
	info->fastHandle=Bus_CreateReceiverHandle(name);
}

//开启TIM硬件
void BSP_TIM_StartHardware(TIMInfo* info,ConfItem* dict)
{
	if(!strcmp(Conf_GetValue(dict,"mode", const char*, NULL),"encode"))
	{
		HAL_TIM_Encoder_Start(info->htim,TIM_CHANNEL_ALL);
	}
	else if(!strcmp(Conf_GetValue(dict,"mode", const char*, NULL),"pwm"))
	{
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_3);
		HAL_TIM_PWM_Start(info->htim, TIM_CHANNEL_4);
	}
	else if(!strcmp(Conf_GetValue(dict,"mode", const char*, NULL),"update-interrupted"))
	{
		//清除定时器中断标志位，避免一开启就中断
		__HAL_TIM_CLEAR_IT(info->htim, TIM_IT_UPDATE);
		HAL_TIM_Base_Start_IT(info->htim);
	}
}

//TIM设置配置远程函数回调
bool BSP_TIM_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_IsMapKeyExist(frame,"tim-x"))
		return false;
	uint8_t timX = Bus_GetMapValue(frame,"tim-x").U8;
	TIMInfo* timInfo = NULL;
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number) //找到对应的TIM
		{
			timInfo = &timService.timList[num];
			break;
		}
	}
	if(!timInfo)
		return false;
	if (Bus_CheckMapKeys(frame,{"channel-x","compare-value"}))
	{
		uint8_t	channelX = Bus_GetMapValue(frame,"channel-x").U8;
		uint32_t compVal = Bus_GetMapValue(frame,"compare-value").U32;
		switch (channelX)
		{
			case 1:
				__HAL_TIM_SetCompare(timInfo->htim, TIM_CHANNEL_1, compVal);
				break;
			case 2:
				__HAL_TIM_SetCompare(timInfo->htim, TIM_CHANNEL_2, compVal);
				break;
			case 3:
				__HAL_TIM_SetCompare(timInfo->htim, TIM_CHANNEL_3, compVal);
				break;
			case 4:
				__HAL_TIM_SetCompare(timInfo->htim, TIM_CHANNEL_4, compVal);
				break;
			default:
				break;
		}
	}
	if(Bus_IsMapKeyExist(frame,"auto-reload"))
	{
		uint32_t autoReload = Bus_GetMapValue(frame,"auto-reload").U32;
		__HAL_TIM_SetAutoreload(timInfo->htim,autoReload);
	}
	if(Bus_IsMapKeyExist(frame,"enable"))
	{
		bool enable = Bus_GetMapValue(frame,"enable").Bool;
		if(enable)
			HAL_TIM_Base_Start_IT(timInfo->htim);
		else
			HAL_TIM_Base_Stop_IT(timInfo->htim);
	}

	return true;
}

//TIM设置占空比远程函数回调
bool BSP_TIM_SetDutyCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"tim-x","channel-x","duty"}))
		return false;
	uint8_t timX = Bus_GetMapValue(frame,"tim-x").U8;
	uint8_t	channelX = Bus_GetMapValue(frame,"channel-x").U8;
	float duty = Bus_GetMapValue(frame,"duty").F32;
	LIMIT(duty,0,1);
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number) //找到对应的TIM
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
		}
	}
	return false;
}

//TIM获取编码器值远程服务回调
bool BSP_TIM_GetEncodeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!Bus_CheckMapKeys(frame,{"tim-x","count"}))
		return false;
	uint8_t timX = Bus_GetMapValue(frame,"tim-x").U8;
	uint32_t *count = (uint32_t*)Bus_GetMapValue(frame,"count").Ptr;
	uint32_t *autoReload=NULL; 
	if(Bus_IsMapKeyExist(frame,"auto-reload"))
		autoReload = (uint32_t *)Bus_GetMapValue(frame,"auto-reload").Ptr;
	for(uint8_t num = 0;num<timService.timNum;num++)
	{
		if(timX==timService.timList[num].number) //找到对应的TIM
		{
			*count=__HAL_TIM_GetCounter(timService.timList[num].htim); //返回计数器值
			if(autoReload)
				*autoReload=__HAL_TIM_GetAutoreload(timService.timList[num].htim); //如果提供了该变量则，返回自动重装载值
			return true;
		}
	}
	return false;
}

