#include "cmsis_os.h"
#include "softbus.h"
#include "config.h"
#include "pid.h"
#include "main.h"

typedef enum
{
	SYS_FOLLOW_MODE,
	SYS_SPIN_MODE,
	SYS_SEPARATE_MODE
}SysCtrlMode;

typedef struct
{
	struct
	{
		float vx,vy,vw;
		float ax,ay;
	}chassisData; //��������

	struct
	{
		float yaw,pitch;
		float relativeAngle; //��̨ƫ��Ƕ�
	}gimbalData; //��̨����

	uint8_t mode;
	bool rockerCtrl; // ң�������Ʊ�־λ
	bool errFlag;  // ��ͣ��־λ
	PID rotatePID;
}SysControl;

SysControl sysCtrl={0};

//��������
void Sys_InitInfo(ConfItem *dict);
void Sys_InitReceiver(void);
void Sys_Broadcast(void);

void Sys_Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Chassis_MoveCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Mode_ChangeCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Gimbal_RotateCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_Shoot_Callback(const char* name, SoftBusFrame* frame, void* bindData);
void Sys_ErrorHandle(void);

//��ʼ��������Ϣ
void Sys_InitInfo(ConfItem *dict)
{
	sysCtrl.mode = Conf_GetValue(dict, "InitMode", uint8_t, SYS_FOLLOW_MODE); //Ĭ�ϸ���ģʽ
	sysCtrl.rockerCtrl = Conf_GetValue(dict, "rockerCtrl", bool, false);  //Ĭ�ϼ������
	PID_Init(&sysCtrl.rotatePID, Conf_GetPtr(dict, "rotatePID", ConfItem)); 
}

//��ʼ������
void Sys_InitReceiver()
{
	//����
	Bus_MultiRegisterReceiver(NULL, Sys_Chassis_MoveCallback, {"/rc/key/on-pressing","/rc/left-stick"});
	Bus_RegisterReceiver(NULL, Sys_Chassis_StopCallback, "/rc/key/on-up");
	//��̨
	Bus_MultiRegisterReceiver(NULL, Sys_Gimbal_RotateCallback, {"/rc/mouse-move",
																"/rc/right-stick",
																"/gimbal/yaw/relative-angle"});	
	//ģʽ�л�
	Bus_MultiRegisterReceiver(NULL, Sys_Mode_ChangeCallback, {"/rc/key/on-click","rc/switch"});
	//����  
	Bus_MultiRegisterReceiver(NULL, Sys_Shoot_Callback, {"/rc/key/on-click",
														"/rc/key/on-long-press",
														"/rc/key/on-up",
														"/rc/wheel"});
}

void SYS_CTRL_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Sys_InitInfo((ConfItem *)argument);
	Sys_InitReceiver();
	portEXIT_CRITICAL();
	while(1)
	{
		if(sysCtrl.errFlag==1)
			Sys_ErrorHandle();

		if(sysCtrl.mode==SYS_FOLLOW_MODE)//����ģʽ
		{
			PID_SingleCalc(&sysCtrl.rotatePID, 0, sysCtrl.gimbalData.relativeAngle);
			sysCtrl.chassisData.vw = sysCtrl.rotatePID.output;
		}
		else if(sysCtrl.mode==SYS_SPIN_MODE)//С����ģʽ
		{
			sysCtrl.chassisData.vw = 240;
		}
		else if(sysCtrl.mode==SYS_SEPARATE_MODE)// ����ģʽ
		{
			sysCtrl.chassisData.vw = 0;
		}
		Sys_Broadcast();
		osDelay(10);
	}
}

//���͹㲥
void Sys_Broadcast()
{
	Bus_RemoteCall("/chassis/speed", {{"vx", &sysCtrl.chassisData.vx},
										{"vy", &sysCtrl.chassisData.vy},
										{"vw", &sysCtrl.chassisData.vw}});
	Bus_RemoteCall("/chassis/relativeAngle", {{"angle", &sysCtrl.gimbalData.relativeAngle}});
	Bus_RemoteCall("/gimbal/setting", {{"yaw", &sysCtrl.gimbalData.yaw},{"pitch", &sysCtrl.gimbalData.pitch}});
}

//�����˶���ֹͣ�ص�����
void Sys_Chassis_MoveCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	float speedRatio=0;
	if(!strcmp(name,"/rc/key/on-pressing") && !sysCtrl.rockerCtrl) //�������
	{
		if(!Bus_CheckMapKeys(frame,{"combine-key","key"}))
			return;
		if(!strcmp(Bus_GetMapValue(frame,"combine-key"), "none"))  //����
			speedRatio=1; 
		else if(!strcmp(Bus_GetMapValue(frame,"combine-key"), "shift")) //����
			speedRatio=5; 
		else if(!strcmp(Bus_GetMapValue(frame,"combine-key"), "ctrl")) //����
			speedRatio=0.2;
		switch(*(char*)Bus_GetMapValue(frame,"key"))
		{
			case 'A': 
				sysCtrl.chassisData.vx = 200*speedRatio;
				break;
			case 'D': 
				sysCtrl.chassisData.vx = -200*speedRatio;
				break;
			case 'W': 
				sysCtrl.chassisData.vy = 200*speedRatio;
				break;
			case 'S': 
				sysCtrl.chassisData.vy = -200*speedRatio;
				break;
		}
	}
	else if(!strcmp(name,"/rc/left-stick") && sysCtrl.rockerCtrl) //ң��������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
			return;
		sysCtrl.chassisData.vx = *(int16_t*)Bus_GetMapValue(frame,"x");
		sysCtrl.chassisData.vy = *(int16_t*)Bus_GetMapValue(frame,"y");
	}
}
void Sys_Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{

	if(sysCtrl.rockerCtrl || !Bus_IsMapKeyExist(frame, "key"))
		return;
	switch(*(char*)Bus_GetMapValue(frame,"key"))
	{
		case 'A': 
		case 'D': 
			sysCtrl.chassisData.vx = 0;
			break;
		case 'W': 
		case 'S': 
			sysCtrl.chassisData.vy = 0;
			break;
	}
}

//��̨��ת�ص�����
void Sys_Gimbal_RotateCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!strcmp(name,"/rc/mouse-move") && !sysCtrl.rockerCtrl)  //�������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
			return;
		sysCtrl.gimbalData.yaw =*(int16_t*)Bus_GetMapValue(frame,"x");
		sysCtrl.gimbalData.pitch =*(int16_t*)Bus_GetMapValue(frame,"y"); 
	}
	else if(!strcmp(name,"/rc/right-stick") && sysCtrl.rockerCtrl)  //ң��������
	{
		if(!Bus_CheckMapKeys(frame,{"x","y"}))
			return;
		sysCtrl.gimbalData.yaw =*(int16_t*)Bus_GetMapValue(frame,"x");
		sysCtrl.gimbalData.pitch =*(int16_t*)Bus_GetMapValue(frame,"y"); 
	}
	else if(!strcmp(name,"/gimbal/yaw/relative-angle"))
	{
		if(!Bus_IsMapKeyExist(frame,"angle"))
			return;
		sysCtrl.gimbalData.relativeAngle = *(float*)Bus_GetMapValue(frame,"angle");
	}
}

//ģʽ�л��ص�
void Sys_Mode_ChangeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!strcmp(name,"/rc/key/on-click") && !sysCtrl.rockerCtrl)  //�������
	{
		if(!Bus_IsMapKeyExist(frame,"key"))
			return;
		switch(*(char*)Bus_GetMapValue(frame,"key"))
		{
			case 'Q':  
				sysCtrl.mode = SYS_SPIN_MODE;  //С����ģʽ
				break;
			case 'E':  
				sysCtrl.mode = SYS_FOLLOW_MODE;  //����ģʽ
				break;
			case 'R':
				sysCtrl.mode = SYS_SEPARATE_MODE; //����ģʽ
				break;
		}
	}
	else if(!strcmp(name,"/rc/switch") && sysCtrl.rockerCtrl)  //ң��������
	{
		if(!Bus_IsMapKeyExist(frame, "right"))
			return;
		switch(*(uint8_t*)Bus_GetMapValue(frame, "right"))
		{
			case 1:
				sysCtrl.mode = SYS_SPIN_MODE; //С����ģʽ
				break;                        
			case 2:                         
				sysCtrl.mode = SYS_FOLLOW_MODE;  //����ģʽ
				break;                        
			case 3:                         
				sysCtrl.mode = SYS_SEPARATE_MODE; //����ģʽ
				break;
		}
	}
	else if(!strcmp(name,"/rc/switch"))
	{
		if(!Bus_IsMapKeyExist(frame, "left"))
			return;
		switch(*(uint8_t*)Bus_GetMapValue(frame, "left"))
		{
			case 1:
				sysCtrl.rockerCtrl = true; //�л���ң��������
				sysCtrl.errFlag = 0;
				break;
			case 2:
				sysCtrl.rockerCtrl = false; //�л����������
				sysCtrl.errFlag = 0;
				break;
			case 3:   
				sysCtrl.errFlag = 1;
				break;
		}
	}
}

//����ص�����
void Sys_Shoot_Callback(const char* name, SoftBusFrame* frame, void* bindData)
{
	if(!strcmp(name,"/rc/key/on-click") && !sysCtrl.rockerCtrl)//�������
	{
		if(!Bus_IsMapKeyExist(frame,"left"))
			return;
		Bus_RemoteCall("/shooter/mode",{{"mode", "once"}});  //����
	}
	else if(!strcmp(name,"/rc/key/on-long-press") && !sysCtrl.rockerCtrl)
	{
		if(!Bus_IsMapKeyExist(frame,"left"))
			return;
		Bus_RemoteCall("/shooter/mode",{{"mode", "continue"}, {"intervalTime", IM_PTR(uint16_t, 100)}}); //����
	}
	else if(!strcmp(name,"/rc/key/on-up") && !sysCtrl.rockerCtrl)
	{
		if(!Bus_IsMapKeyExist(frame,"left"))
			return;
		Bus_RemoteCall("/shooter/mode",{{"mode", "idle"}}); //����
	}
	else if(!strcmp(name,"/rc/wheel") && sysCtrl.rockerCtrl)//ң��������
	{
		if(!Bus_IsMapKeyExist(frame,"value"))
			return;
		int16_t wheel = *(int16_t*)Bus_GetMapValue(frame,"value");

		if(wheel > 600)
			Bus_RemoteCall("/shooter", {{"once", IM_PTR(uint8_t,1)}}); //����
	}
}

//��ͣ
void Sys_ErrorHandle(void)
{
	Bus_BroadcastSend("/system/stop",{"",0});
	while(1)
	{
		if(!sysCtrl.errFlag)
		{
			__disable_irq();
			NVIC_SystemReset();
		}
		osDelay(2);
	}
}
