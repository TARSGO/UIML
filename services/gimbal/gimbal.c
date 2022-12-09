#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

#define PI 3.1415926535f

#define GIMBAL_TASK_INIT_TIME 201			//�����ʼ�� ����һ��ʱ��
#define GIMBAL_MODE_CHANNEL 0					//״̬����ͨ��
#define GIMBAL_CONTROL_TIME 1					//��̨������ʱ��

//�����ǿ��ƽṹ��
typedef struct
{
		float measure_angle_360;    //�����Ƕ�ֵ ��λ360 ��ԽǶ� ���������
		float measure_speed_360;    //�������ٶ�ֵ ��λ360/s ��ԽǶ� ���������
		float target_angle_360;			//Ŀ��Ƕ�ֵ ��λ360 ��ԽǶ�
		float target_speed_360;			//Ŀ����ٶ�ֵ ��λ360/s ��ԽǶ�
		float error_angle_360;			//���Ƕ�ֵ ��λ360 ��ԽǶ�
		float error_speed_360;			//�����ٶ�ֵ ��λ360/s ��ԽǶ�
		
//		//�������˲��ṹ��
//		struct
//		{
//			float speed_error_kalman, angle_error_kalman;
//			KalmanFilter kalman_speed, kalman_angle;
//		}filter;
//		
//		PidThreeLayer pid;    		//����pid�ṹ��
//		
} GimbalGyroControlStruct;

typedef struct _Gimbal
{
		
							//  �ⲿ���ݽ���  //

		float INS_angle[3];
		float Target_angle[2];
		//2�����
		Motor* motors[2];
		
		struct _Yaw
		{		
				float yaw_velocity;				//��ǰ�ٶ� mm/s			
				float maxV; 							//����ٶ�
			
				struct
				{
						float angle_middle_8192;    						//yaw���м�ʱ��8192ֵ
						float angle_left_360, angle_right_360;  //yaw�������ת��
						int16_t measure_relative_angle_8192;    //�����Ƕ�ֵ ��λ8192 ��ԽǶ�
						float measure_angle_360;    						//�����Ƕ�ֵ ��λ360 ��ԽǶ�
						float measure_speed_360;    						//�������ٶ�ֵ ��λ360/s ��ԽǶ�
						int16_t output;    											//����������ֵ
				} info;
				
//				const motor_measure_t *motor_measure;		//���ڽ��յ�����ݵ�ַ
				GimbalGyroControlStruct gyro;							//�����ǿ��ƽṹ��				
		}yaw;
		
		struct _Pitch
		{		
				float pitch_velocity;			//��ǰ�ٶ� mm/s			
				float maxV; 							//����ٶ�
			
				struct
				{
						float angle_middle_8192;    						//pitch���м�ʱ��8192ֵ
						float angle_up_360, angle_down_360;  		//pitch�������ת��
						int16_t measure_relative_angle_8192;    //�����Ƕ�ֵ ��λ8192 ��ԽǶ�
						float measure_angle_360;    						//�����Ƕ�ֵ ��λ360 ��ԽǶ�
						float measure_speed_360;    						//�������ٶ�ֵ ��λ360/s ��ԽǶ�
						int16_t output;    											//����������ֵ��ֱ�ӷ��;���
				} info;
				
//				const motor_measure_t *motor_measure;		//���ڽ��յ�����ݵ�ַ
				GimbalGyroControlStruct gyro;							//�����ǿ��ƽṹ��			
		}pitch;
				
		uint8_t taskInterval;
	
		
}Gimbal;

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict);
void Gimbal_MotoAnalysis(Gimbal *gimbal);
void Gimbal_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Gimbal_MoveBusCallback(const char* name, SoftBusFrame* frame, void* bindData);
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
			
				angle_zero(gimbal.Target_angle[0],gimbal.INS_angle[0]);		//yaw
				angle_zero(gimbal.Target_angle[1],gimbal.INS_angle[1]);		//pitch
			
				osDelayUntil(&tick,gimbal.taskInterval);
		}

}

void Gimbal_Init(Gimbal* gimbal, ConfItem* dict)
{

		//������
		gimbal->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);

		gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
		gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));
		
		//pitch��ʼ������
		gimbal->pitch.info.angle_middle_8192 = 124.f;
		gimbal->pitch.info.angle_up_360 		 = 10.f;
		gimbal->pitch.info.angle_down_360  	 = -40.f;
	
		//yaw��ʼ������
		gimbal->yaw.info.angle_middle_8192 = 6144.f;
		gimbal->yaw.info.angle_left_360 	 = -180.f;
		gimbal->yaw.info.angle_right_360 	 = 180.f;
	
		//�ƶ�������ʼ��
		gimbal->yaw.maxV 	 = Conf_GetValue(dict, "moveYaw/maxSpeed", float, 2000);
		gimbal->pitch.maxV = Conf_GetValue(dict, "movePitch/maxSpeed", float, 2000);	
	
		//��̨�����ʼ��
		gimbal->motors[0] = Motor_Init(Conf_GetPtr(dict, "motorYaw", ConfItem));
		gimbal->motors[1] = Motor_Init(Conf_GetPtr(dict, "motorPitch", ConfItem));
	
		for(uint8_t i = 0; i<2; i++)
		{
			gimbal->motors[i]->changeMode(gimbal->motors[i], MOTOR_SPEED_MODE);
		}
		
		
		Bus_MultiRegisterReceiver(gimbal, Gimbal_SoftBusCallback, {"/gimbal/INS_angle"});
		Bus_MultiRegisterReceiver(gimbal, Gimbal_MoveBusCallback, {"rc/mouse-move"});		
}

void Gimbal_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
		Gimbal* gimbal = (Gimbal*)bindData;

		if(!strcmp(name, "/gimbal/INS_angle"))
		{
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[0] = *(float*)Bus_GetMapValue(frame, "INS_angle[0]");
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[1] = *(float*)Bus_GetMapValue(frame, "INS_angle[1]");
				if(Bus_IsMapKeyExist(frame, "ins_angle[0]"))
					gimbal->INS_angle[2] = *(float*)Bus_GetMapValue(frame, "INS_angle[2]");				
		}
}

void Gimbal_MoveBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{	
		Gimbal* gimbal = (Gimbal*)bindData;
	
		if(!strcmp(name, "rc/mouse-move"))
		{
				if(Bus_IsMapKeyExist(frame, "x"))
					gimbal->Target_angle[0] += *(float*)Bus_GetMapValue(frame, "x");
				
				if(Bus_IsMapKeyExist(frame, "y"))
					gimbal->Target_angle[1] += *(float*)Bus_GetMapValue(frame, "y");		
		}
	
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

