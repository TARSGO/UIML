#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

#define PI 3.1415926535f

typedef struct _Gimbal
{

		float INS_angle[3];
		float Target_angle[2];
	
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
	
		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_Limit(Gimbal* gimbal);

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);
void Gimbal_MoveBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

float angle_zero(float angle, float offset_angle);

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
				gimbal.yaw.Target_Yaw 		= angle_zero(gimbal.Target_angle[0], gimbal.INS_angle[0]);		
				gimbal.pitch.Target_Pitch = angle_zero(gimbal.Target_angle[1], gimbal.INS_angle[1]);		
			
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
		
		
		SoftBus_MultiSubscribe(gimbal, Gimbal_SoftBusCallback, {"/gimbal/INS_angle"});
		SoftBus_MultiSubscribe(gimbal, Gimbal_MoveBusCallback, {"rc/mouse-move"});		
}

void Gimbal_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
		Gimbal* gimbal = (Gimbal*)bindData;

		if(!strcmp(topic, "/gimbal/INS_angle"))
		{
				if(SoftBus_IsMapKeyExist(frame, "INS_angle[0]"))
					gimbal->INS_angle[0] = *(float*)SoftBus_GetMapValue(frame, "INS_angle[0]");
				if(SoftBus_IsMapKeyExist(frame, "INS_angle[1]"))
					gimbal->INS_angle[1] = *(float*)SoftBus_GetMapValue(frame, "INS_angle[1]");
				if(SoftBus_IsMapKeyExist(frame, "INS_angle[2]"))
					gimbal->INS_angle[2] = *(float*)SoftBus_GetMapValue(frame, "INS_angle[2]");				
		}
}

void Gimbal_MoveBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{	
		Gimbal* gimbal = (Gimbal*)bindData;
	
		if(!strcmp(topic, "rc/mouse-move"))
		{
				if(SoftBus_IsMapKeyExist(frame, "x"))
					gimbal->Target_angle[0] += *(float*)SoftBus_GetMapValue(frame, "x");
				
				if(SoftBus_IsMapKeyExist(frame, "y"))
					gimbal->Target_angle[1] += *(float*)SoftBus_GetMapValue(frame, "y");		
		}
	
}	

void Gimbal_Limit(Gimbal* gimbal)
{
	
			//yaw�Ƕ�����
			if(gimbal->Target_angle[0] > PI)
				gimbal->Target_angle[0] = -PI;
			if(gimbal->Target_angle[0] < -PI)
				gimbal->Target_angle[0] = PI;	
		
			//pitch�Ƕ�����
			if(gimbal->Target_angle[1] > PI)
				gimbal->Target_angle[1] = -PI;
			if(gimbal->Target_angle[1] < -PI)
				gimbal->Target_angle[1] = PI;	
			
}
	
float angle_zero(float angle, float offset_angle)
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

