#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

#ifndef PI
#define PI 3.1415926535f
#endif

typedef enum
{
	GIMBAL_ZERO_FORCE = 0, 		//����ģʽ
	GIMBAL_ECD_CONTROL = 1, 	//ECDģʽ
	GIMBAL_IMU_GIMBAL = 2, 		//IMUģʽ
	
}GIMBAL_MODE_e; //��̨ģʽ

typedef struct _Gimbal
{

	float ins_angle[3];
	float target_angle[2];

	//2�����
	Motor* motors[2];
	
	struct _Yaw
	{		
		float yaw_velocity;				//��ǰ�ٶ� mm/s			
		float maxV; 							//����ٶ�				
		float Target_Yaw;		
	}yaw;
		
	struct _Pitch
	{		
		float pitch_velocity;			//��ǰ�ٶ� mm/s			
		float maxV; 							//����ٶ�
		float Target_Pitch;							
	}pitch;
				
	uint8_t taskInterval;
	uint8_t GIMBAL_MODE;
		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_Limit(Gimbal* gimbal);
float Gimbal_angle_zero(float angle, float offset_angle);

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

void Gimbal_TaskCallback(void const * argument)
{
	
	//�����ٽ���
	portENTER_CRITICAL();
	Gimbal gimbal={0};
	Gimbal_Init(&gimbal, (ConfItem*)argument);
	portEXIT_CRITICAL();
	
	osDelay(2000);
	TickType_t tick = xTaskGetTickCount();

	while(1)
	{
		//�Ƕ�����
		Gimbal_Limit(&gimbal);		
		
		//���㴦��
		gimbal.yaw.Target_Yaw 		= Gimbal_angle_zero(gimbal.target_angle[0], gimbal.ins_angle[0]);		
		gimbal.pitch.Target_Pitch = Gimbal_angle_zero(gimbal.target_angle[1], gimbal.ins_angle[1]);		
	
		//�������
		gimbal.motors[0]->setTarget(gimbal.motors[0], gimbal.yaw.Target_Yaw);
		gimbal.motors[1]->setTarget(gimbal.motors[1], gimbal.pitch.Target_Pitch);

		osDelayUntil(&tick,gimbal.taskInterval);
	}

}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{

	//������
	gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));

	//�ƶ�������ʼ��
	gimbal->yaw.maxV 	 = Conf_GetValue(dict, "moveYaw/maxSpeed", float, 2000);
	gimbal->pitch.maxV = Conf_GetValue(dict, "movePitch/maxSpeed", float, 2000);	

	//��̨�����ʼ��
	gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
	gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));

	for(uint8_t i = 0; i<2; i++)
	{
		gimbal->motors[i]->changeMode(gimbal->motors[i], MOTOR_ANGLE_MODE);
	}
			
	//��ʼ����̨ģʽΪ ��̨����ģʽ
	gimbal->GIMBAL_MODE = GIMBAL_ZERO_FORCE;
	
	Bus_RegisterReceiver(gimbal, Gimbal_SoftBusCallback, "/gimbal/ins_angle");		
}

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	Gimbal* gimbal = (Gimbal*)bindData;

	if(!strcmp(topic, "/gimbal/ins_angle"))
	{
		if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
			gimbal->ins_angle[0] = *(float*)Bus_GetMapValue(frame, "ins_angle[0]");
		if(Bus_IsMapKeyExist(frame, "ins_angle[1]"))
			gimbal->ins_angle[1] = *(float*)Bus_GetMapValue(frame, "ins_angle[1]");
		if(Bus_IsMapKeyExist(frame, "ins_angle[2]"))
			gimbal->ins_angle[2] = *(float*)Bus_GetMapValue(frame, "ins_angle[2]");				
	}
}

void Gimbal_Limit(Gimbal* gimbal)
{
	
	//yaw�Ƕ�����
	if(gimbal->target_angle[0] > PI)
		gimbal->target_angle[0] = -PI;
	if(gimbal->target_angle[0] < -PI)
		gimbal->target_angle[0] = PI;	

	//pitch�Ƕ�����
	if(gimbal->target_angle[1] > PI)
		gimbal->target_angle[1] = -PI;
	if(gimbal->target_angle[1] < -PI)
		gimbal->target_angle[1] = PI;	
			
}
	
float Gimbal_angle_zero(float angle, float offset_angle)
{
	float relative_angle = angle - offset_angle;

	if(relative_angle >  1.25f * PI)
	{
		relative_angle -= 2*PI;
	}
	else if(relative_angle < - 1.25f * PI)
	{
		relative_angle += 2*PI;		
	}

	return relative_angle;
}

