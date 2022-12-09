#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"

#include "can.h"

//CAN�����Ϣ
typedef struct {
	CAN_HandleTypeDef* hcan;
	uint8_t number; //canX�е�X
	SoftBusReceiverHandle fastHandle;
}CANInfo;

//ѭ�����ͻ�����
typedef struct {
	CANInfo* canInfo; //ָ�����󶨵�CANInfo
	uint16_t frameID;
	uint8_t* data;
}CANRepeatBuffer;

//��CAN��������
typedef struct {
	CANInfo* canList;
	uint8_t canNum;
	CANRepeatBuffer* repeatBuffers;
	uint8_t bufferNum;
	uint8_t initFinished;
}CANService;

CANService canService = {0};
//��������
void BSP_CAN_Init(ConfItem* dict);
void BSP_CAN_InitInfo(CANInfo* info, ConfItem* dict);
void BSP_CAN_InitHardware(CANInfo* info);
void BSP_CAN_InitRepeatBuffer(CANRepeatBuffer* buffer, ConfItem* dict);
void BSP_CAN_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
void BSP_CAN_TimerCallback(void const *argument);
uint8_t BSP_CAN_SendFrame(CAN_HandleTypeDef* hcan,uint16_t StdId,uint8_t* data);

//can���ս����ж�
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	CAN_RxHeaderTypeDef header;
	uint8_t rx_data[8];
	
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, rx_data);
	
	if(!canService.initFinished)
		return;
	
	for(uint8_t i = 0; i < canService.canNum; i++)
	{
		CANInfo* canInfo = &canService.canList[i];
		if(hcan == canInfo->hcan)
		{
			uint16_t frameID = header.StdId;
			Bus_FastBroadcastSend(canInfo->fastHandle, {&frameID, rx_data});
		}
	}
}

//can����ص�����
void BSP_CAN_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	BSP_CAN_Init((ConfItem*)argument);
	portEXIT_CRITICAL();
	
	vTaskDelete(NULL);
}

void BSP_CAN_Init(ConfItem* dict)
{
	//�����û����õ�can����
	canService.canNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "cans/_";
		confName[5] = num + '0';
		if(Conf_ItemExist(dict, confName))
			canService.canNum++;
		else
			break;
	}
	//��ʼ����can��Ϣ
	canService.canList = pvPortMalloc(canService.canNum * sizeof(CANInfo));
	for(uint8_t num = 0; num < canService.canNum; num++)
	{
		char confName[] = "cans/_";
		confName[5] = num + '0';
		BSP_CAN_InitInfo(&canService.canList[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	//��ʼ��CANӲ������
	for(uint8_t num = 0; num < canService.canNum; num++)
	{
		BSP_CAN_InitHardware(&canService.canList[num]);
	}
	//�����û����õ�ѭ�����ͻ���������
	canService.bufferNum = 0;
	for(uint8_t num = 0; ; num++)
	{
		char confName[] = "repeat-buffers/_";
		confName[15] = num + '0';
		if(Conf_ItemExist(dict, confName))
			canService.bufferNum++;
		else
			break;
	}
	//��ʼ����ѭ��������
	canService.repeatBuffers = pvPortMalloc(canService.bufferNum * sizeof(CANRepeatBuffer));
	for(uint8_t num = 0; num < canService.bufferNum; num++)
	{
		char confName[] = "repeat-buffers/_";
		confName[15] = num + '0';
		BSP_CAN_InitRepeatBuffer(&canService.repeatBuffers[num], Conf_GetPtr(dict, confName, ConfItem));
	}
	//���Ļ���
	Bus_RegisterReceiver(NULL, BSP_CAN_SoftBusCallback, "/can/set-buf");
	Bus_RegisterReceiver(NULL, BSP_CAN_SoftBusCallback, "/can/send-once");

	canService.initFinished = 1;
}

//��ʼ��CAN��Ϣ
void BSP_CAN_InitInfo(CANInfo* info, ConfItem* dict)
{
	info->hcan = Conf_GetPtr(dict, "hcan", CAN_HandleTypeDef);
	info->number = Conf_GetValue(dict, "number", uint8_t, 0);
	char name[] = "/can_/recv";
	name[4] = info->number + '0';
	info->fastHandle = Bus_CreateReceiverHandle(name);
}

//��ʼ��Ӳ������
void BSP_CAN_InitHardware(CANInfo* info)
{
	//CAN��������ʼ��
	CAN_FilterTypeDef canFilter = {0};
	canFilter.FilterActivation = ENABLE;
	canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
	canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
	canFilter.FilterIdHigh = 0x0000;
	canFilter.FilterIdLow = 0x0000;
	canFilter.FilterMaskIdHigh = 0x0000;
	canFilter.FilterMaskIdLow = 0x0000;
	canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
	if(info->number != 1)
	{
		canFilter.SlaveStartFilterBank=14;
		canFilter.FilterBank = 14;
	}
	else
	{
		canFilter.FilterBank = 0;
	}
	HAL_CAN_ConfigFilter(info->hcan, &canFilter);
	//����CAN
	HAL_CAN_Start(info->hcan);
	HAL_CAN_ActivateNotification(info->hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

//��ʼ��ѭ�����ͻ�����
void BSP_CAN_InitRepeatBuffer(CANRepeatBuffer* buffer, ConfItem* dict)
{
	//�ظ�֡��can
	uint8_t canX = Conf_GetValue(dict, "can-x", uint8_t, 0);
	for(uint8_t i = 0; i < canService.canNum; ++i)
		if(canService.canList[i].number == canX)
			buffer->canInfo = &canService.canList[i];
	//�����ظ�֡can��id��
	buffer->frameID = Conf_GetValue(dict, "id", uint16_t, 0x00);
	buffer->data = pvPortMalloc(8);
	memset(buffer->data, 0, 8);
		//���������ʱ����ʱ�����ظ�֡
	uint16_t sendInterval = Conf_GetValue(dict, "interval", uint16_t, 100);
	osTimerDef(CAN, BSP_CAN_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(CAN), osTimerPeriodic, buffer), sendInterval);
}

//ϵͳ��ʱ���ص�
void BSP_CAN_TimerCallback(void const *argument)
{
	if(!canService.initFinished)
		return;
	CANRepeatBuffer* buffer = pvTimerGetTimerID((TimerHandle_t)argument); 
	BSP_CAN_SendFrame(buffer->canInfo->hcan, buffer->frameID, buffer->data);
}

//CAN��������֡
uint8_t BSP_CAN_SendFrame(CAN_HandleTypeDef* hcan,uint16_t stdId,uint8_t data[8])
{
	CAN_TxHeaderTypeDef txHeader;
	uint32_t canTxMailBox;

	txHeader.StdId = stdId;
	txHeader.IDE   = CAN_ID_STD;
	txHeader.RTR   = CAN_RTR_DATA;
	txHeader.DLC   = 8;
	
	uint8_t retVal=HAL_CAN_AddTxMessage(hcan, &txHeader, data, &canTxMailBox);

	return retVal;
}

//�����߻ص�
void BSP_CAN_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!strcmp(name, "/can/set-buf"))
	{
		if(!Bus_CheckMapKeys(frame, {"can-x", "id", "pos", "len", "data"}))
			return;
		
		uint8_t canX = *(uint8_t*)Bus_GetMapValue(frame, "can-x");
		uint16_t frameID = *(uint16_t*)Bus_GetMapValue(frame, "id");
		uint8_t startIndex = *(uint8_t*)Bus_GetMapValue(frame, "pos");
		uint8_t length = *(uint8_t*)Bus_GetMapValue(frame, "len");
		uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data");
		
		for(uint8_t i = 0; i < canService.bufferNum; i++)
		{
			CANRepeatBuffer* buffer = &canService.repeatBuffers[i];
			if(buffer->canInfo->number == canX && buffer->frameID == frameID)
				memcpy(buffer->data + startIndex, data, length);
		}
	}
	else if(!strcmp(name, "/can/send-once"))
	{
		
		if(!Bus_CheckMapKeys(frame, {"can-x", "id", "data"}))
			return;

		uint8_t canX = *(uint8_t*)Bus_GetMapValue(frame, "can-x");
		uint16_t frameID = *(uint16_t*)Bus_GetMapValue(frame, "id");
		uint8_t* data = (uint8_t*)Bus_GetMapValue(frame, "data");

		for(uint8_t i = 0; i < canService.canNum; i++)
		{
			CANInfo* info = &canService.canList[i];
			if(info->number == canX)
				BSP_CAN_SendFrame(info->hcan, frameID, data);
		}
	}
}
