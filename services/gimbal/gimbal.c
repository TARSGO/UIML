#include "config.h"
#include "softbus.h"
#include "pid.h"
#include "motor.h"
#include "cmsis_os.h"

#ifndef PI
#define PI 3.1415926535f
#endif

typedef struct _Gimbal
{
	//yaw��pitch���
	Motor* motors[2];

	uint16_t zeroAngle[2];	//���
	float relativeAngle;	//��̨ƫ��Ƕ�
	struct 
	{
		// float eulerAngle[3];	//ŷ����
		float lastEulerAngle[3];
		float totalEulerAngle[3];
		PID pid[2];
	}imu;
	float angle[2];	//��̨�Ƕ�
	uint16_t taskInterval;	
	//�����߹㲥��Զ�̺���name
	char* yawRelAngleName;	
	char* imuEulerAngleName;
	char* settingName;
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_TotalAngleInit(Gimbal* gimbal);
void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll);

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Gimbal_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	Gimbal_TotalAngleInit(&gimbal); //������̨���
	//�������̨���󣬸��ĵ��ģʽ��imu�������Ƕ��⻷����ٶȷ������ڻ�
	gimbal.motors[0]->changeMode(gimbal.motors[0], MOTOR_SPEED_MODE);
	gimbal.motors[1]->changeMode(gimbal.motors[1], MOTOR_SPEED_MODE);
	while(1)
	{
		//����Ƕȴ���pid
		PID_SingleCalc(&gimbal.imu.pid[0], gimbal.angle[0], gimbal.imu.totalEulerAngle[0]);
		PID_SingleCalc(&gimbal.imu.pid[1], gimbal.angle[1], gimbal.imu.totalEulerAngle[1]);
		gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.imu.pid[0].output);
		gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.imu.pid[1].output);
		//������̨�������ĽǶ�
		gimbal.relativeAngle = gimbal.motors[0]->getData(gimbal.motors[0], "totalAngle");
		int16_t turns = (int32_t)gimbal.relativeAngle / 360; //ת��
		turns = turns < 0 ? turns - 1 : turns; //����Ǹ������һȦʹƫ��Ǳ������
		gimbal.relativeAngle -= turns*360; //0-360��
		Bus_BroadcastSend(gimbal.yawRelAngleName, {{"angle", &gimbal.relativeAngle}}); //�㲥��̨ƫ���
		osDelay(gimbal.taskInterval);
	}
}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{
	//������
	gimbal->taskInterval = Conf_GetValue(dict, "task-interval", uint16_t, 2);

	//��̨���
	gimbal->zeroAngle[0] = Conf_GetValue(dict, "zero-yaw", uint16_t, 0);
	gimbal->zeroAngle[1] = Conf_GetValue(dict, "zero-pitch", uint16_t, 0);

	//��̨�����ʼ��
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motor-yaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motor-pitch", ConfItem));

	PID_Init(&gimbal->imu.pid[0], Conf_GetPtr(dict, "yaw-imu-pid", ConfItem));
	PID_Init(&gimbal->imu.pid[1], Conf_GetPtr(dict, "pitch-imu-pid", ConfItem));
	//�㲥��Զ�̺���name��ӳ��
	char* temp = Conf_GetPtr(dict, "name", char);
	temp = temp ? temp : "gimbal";
	uint8_t len = strlen(temp);
	gimbal->settingName = pvPortMalloc(len + 9+ 1); //9Ϊ"/   /setting"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(gimbal->settingName, "/%s/setting", temp);

	gimbal->yawRelAngleName = pvPortMalloc(len + 20+ 1); //20Ϊ"/   /yaw/relative-angle"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(gimbal->yawRelAngleName, "/%s/yaw/relative-angle", temp);

	temp = Conf_GetPtr(dict, "ins-name", char);
	temp = temp ? temp : "ins";
	len = strlen(temp);
	gimbal->imuEulerAngleName = pvPortMalloc(len + 13+ 1); //13Ϊ"/   /euler-angle"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(gimbal->imuEulerAngleName, "/%s/euler-angle", temp);

	//�����������õ��ģʽ����Ϊ��δ���ú����ǰ��pid����ʹ����ﵽ��������������imu�ĳ�ʼ�����

	//ע��㲥��Զ�̺����ص�����
	Bus_RegisterReceiver(gimbal, Gimbal_BroadcastCallback, gimbal->imuEulerAngleName);
	Bus_RegisterRemoteFunc(gimbal, Gimbal_SettingCallback, gimbal->settingName);
	Bus_RegisterReceiver(gimbal, Gimbal_StopCallback, "/system/stop"); //��ͣ
}

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(name, "/ins/euler-angle"))
	{
		if(!Bus_CheckMapKeys(frame, {"yaw", "pitch", "roll"}))
			return;
		float yaw = *(float*)Bus_GetMapValue(frame, "yaw");
		float pitch = *(float*)Bus_GetMapValue(frame, "pitch");
		float roll = *(float*)Bus_GetMapValue(frame, "roll");
		Gimbal_StatAngle(gimbal, yaw, pitch, roll); //ͳ����̨�Ƕ�
	}
}
bool Gimbal_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(Bus_IsMapKeyExist(frame, "yaw"))
	{
		gimbal->angle[0] = *(float*)Bus_GetMapValue(frame, "yaw");
	}
	if(Bus_IsMapKeyExist(frame, "pitch"))
	{
		gimbal->angle[1] = *(float*)Bus_GetMapValue(frame, "pitch");
	}
	return true;
}

void Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData) //��ͣ
{
	Gimbal* gimbal = (Gimbal*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->stop(gimbal->motors[i]);
	}
}

void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll)
{
	float eulerAngle[3] = {yaw, pitch, roll};
	for (uint8_t i = 0; i < 3; i++)
	{
		float dAngle;
		if(eulerAngle[i] - gimbal->imu.lastEulerAngle[i] < -180)
			dAngle = eulerAngle[i] + (360 - gimbal->imu.lastEulerAngle[i]);
		else if(eulerAngle[i] - gimbal->imu.lastEulerAngle[i] > 180)
			dAngle = -gimbal->imu.lastEulerAngle[i] - (360 - eulerAngle[i]);
		else
			dAngle = eulerAngle[i] - gimbal->imu.lastEulerAngle[i];
		//���Ƕ��������������
		gimbal->imu.totalEulerAngle[i] += dAngle;
		//��¼�Ƕ�
		gimbal->imu.lastEulerAngle[i] = eulerAngle[i];
	}
}

void Gimbal_TotalAngleInit(Gimbal* gimbal)
{
	for(uint8_t i = 0; i<2; i++)
	{
		float angle = 0;
		angle = gimbal->motors[i]->getData(gimbal->motors[i], "angle");
		angle = angle - (float)gimbal->zeroAngle[i]*360/8191;  //����������ĽǶ�  
		if(angle < -180)  //���Ƕ�ת����-180~180�ȣ���������ʹ��̨�����������ת�����
			angle += 360;
		else if(angle > 180)
			angle -= 360;
		gimbal->imu.totalEulerAngle[i] = angle;
		gimbal->motors[i]->initTotalAngle(gimbal->motors[i], angle); //���õ������ʼ�Ƕ�
	}	
}
