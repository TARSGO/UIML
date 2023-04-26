#include "config.h"
#include "softbus.h"
#include "pid.h"
#include "motor.h"
#include "cmsis_os.h"

#ifndef PI
#define PI 3.1415926535f
#endif

typedef enum
{
	GIMBAL_ECD_MODE, 	//������ģʽ
	GIMBAL_IMU_MODE		//IMUģʽ
}GimbalCtrlMode; //��̨ģʽ

typedef struct _Gimbal
{
	//yaw��pitch���
	Motor* motors[2];

	GimbalCtrlMode mode;

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
	uint8_t taskInterval;		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_StartAngleInit(Gimbal* gimbal);
void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll);

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Gimbal_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	osDelay(2000);
	Gimbal_StartAngleInit(&gimbal); //������̨���
	//�������̨���󣬸��ĵ��ģʽ
	if(gimbal.mode == GIMBAL_ECD_MODE)
	{
		gimbal.motors[0]->changeMode(gimbal.motors[0], MOTOR_ANGLE_MODE);
		gimbal.motors[1]->changeMode(gimbal.motors[1], MOTOR_ANGLE_MODE);
	}
	else if(gimbal.mode == GIMBAL_IMU_MODE)
	{
		gimbal.motors[0]->changeMode(gimbal.motors[0], MOTOR_SPEED_MODE);
		gimbal.motors[1]->changeMode(gimbal.motors[1], MOTOR_SPEED_MODE);
	}
	while(1)
	{
		if(gimbal.mode == GIMBAL_ECD_MODE)
		{
			gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.angle[0]);
			gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.angle[1]);
		}
		else if(gimbal.mode == GIMBAL_IMU_MODE)
		{
			PID_SingleCalc(&gimbal.imu.pid[0], gimbal.angle[0], gimbal.imu.totalEulerAngle[0]);
			PID_SingleCalc(&gimbal.imu.pid[1], gimbal.angle[1], gimbal.imu.totalEulerAngle[1]);
			gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.imu.pid[0].output);
			gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.imu.pid[1].output);
		}
		gimbal.relativeAngle = gimbal.motors[0]->getData(gimbal.motors[0], "totalAngle");
		int16_t turns = (int32_t)gimbal.relativeAngle / 360; //ת��
		turns = turns < 0 ? turns - 1 : turns; //����Ǹ������һȦʹƫ��Ǳ������
		gimbal.relativeAngle -= turns*360; //0-360��
		Bus_BroadcastSend("/gimbal/yaw/relative-angle", {{"angle", &gimbal.relativeAngle}});
		osDelay(gimbal.taskInterval);
	}
}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{
	//������
	gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

	//��̨���
	gimbal->zeroAngle[0] = Conf_GetValue(dict, "zero-yaw", uint16_t, 0);
	gimbal->zeroAngle[1] = Conf_GetValue(dict, "zero-pitch", uint16_t, 0);

	//��̨�����ʼ��
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motor-yaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motor-pitch", ConfItem));

	PID_Init(&gimbal->imu.pid[0], Conf_GetPtr(dict, "motor-yaw/imu", ConfItem));
	PID_Init(&gimbal->imu.pid[1], Conf_GetPtr(dict, "motor-pitch/imu", ConfItem));

	//��ʼ����̨ģʽΪ ������ģʽ
	gimbal->mode = Conf_GetValue(dict, "mode", GimbalCtrlMode, GIMBAL_ECD_MODE);
	//������������ģʽ����Ϊ��δ���ú����ǰ��pid����ʹ����ﵽ��������������imu�ĳ�ʼ�����

	Bus_RegisterReceiver(gimbal, Gimbal_BroadcastCallback, "/imu/euler-angle");
	Bus_RegisterRemoteFunc(gimbal, Gimbal_SettingCallback, "/gimbal");
	Bus_RegisterRemoteFunc(gimbal, Gimbal_StopCallback, "/system/stop"); //��ͣ
}

void Gimbal_BroadcastCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(name, "/imu/euler-angle"))
	{
		if(!Bus_CheckMapKeys(frame, {"yaw", "pitch", "roll"}))
			return;
		float yaw = *(float*)Bus_GetMapValue(frame, "yaw");
		float pitch = *(float*)Bus_GetMapValue(frame, "pitch");
		float roll = *(float*)Bus_GetMapValue(frame, "roll");
		Gimbal_StatAngle(gimbal, yaw, pitch, roll);
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

bool Gimbal_StopCallback(const char* name, SoftBusFrame* frame, void* bindData) //��ͣ
{
	Gimbal* gimbal = (Gimbal*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->stop(gimbal->motors[i]);
	}
	return true;
}

void Gimbal_StatAngle(Gimbal* gimbal, float yaw, float pitch, float roll)
{
	float dAngle=0;
	float eulerAngle[3] = {yaw, pitch, roll};
	for (uint8_t i = 0; i < 3; i++)
	{
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

void Gimbal_StartAngleInit(Gimbal* gimbal)
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
		gimbal->motors[i]->setStartAngle(gimbal->motors[i], angle);
	}	
}
