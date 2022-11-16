#include "chassis_mecanum.h"
#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "Motor.h"
#include "cmsis_os.h"
#include "arm_math.h"

#define CHASSIS_ACC2SLOPE(acc) (chassis.taskInterval*(acc)/1000) //mm/s2

typedef struct _Chassis
{
	//���̳ߴ���Ϣ
	struct Info
	{
		float wheelbase;//���
		float wheeltrack;//�־�
		float wheelRadius;//�ְ뾶
		float offsetX;//������xy���ϵ�ƫ��
		float offsetY;
	}info;
	//4�����
	Motor* motors[4];
	//�����ƶ���Ϣ
	struct Move
	{
		float vx;//��ǰ����ƽ���ٶ� mm/s
		float vy;//��ǰǰ���ƶ��ٶ� mm/s
		float vw;//��ǰ��ת�ٶ� rad/s
		
		float maxVx,maxVy,maxVw; //������������ٶ�
		Slope xSlope,ySlope; //б��
	}move;
	
	float relativeAngle; //��̨����̵�ƫ��ǣ���λ��
	
	uint8_t taskInterval;
	
}Chassis;

void Chassis_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

Chassis chassis={0};

void Chassis_Init(ConfItem* dict)
{
	//������
	chassis.taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 2);
	//���̳ߴ���Ϣ�����ڽ������٣�
	chassis.info.wheelbase = Conf_GetValue(dict, "info/wheelbase", float, 0);
	chassis.info.wheeltrack = Conf_GetValue(dict, "info/wheeltrack", float, 0);
	chassis.info.wheelRadius = Conf_GetValue(dict, "info/wheelRadius", float, 76);
	chassis.info.offsetX = Conf_GetValue(dict, "info/offsetX", float, 0);
	chassis.info.offsetY = Conf_GetValue(dict, "info/offsetY", float, 0);
	//�ƶ�������ʼ��
	chassis.move.maxVx = Conf_GetValue(dict, "move/offsetX", float, 2000);
	chassis.move.maxVy = Conf_GetValue(dict, "move/offsetX", float, 2000);
	chassis.move.maxVw = Conf_GetValue(dict, "move/offsetX", float, 2);
	float xAcc = Conf_GetValue(dict, "move/xAcc", float, 1000);
	float yAcc = Conf_GetValue(dict, "move/yAcc", float, 1000);
	Slope_Init(&chassis.move.xSlope,CHASSIS_ACC2SLOPE(xAcc),0);
	Slope_Init(&chassis.move.ySlope,CHASSIS_ACC2SLOPE(yAcc),0);
	chassis.motors[0] = Motor_Init(Conf_GetPtr(dict, "motorFL", ConfItem));
	chassis.motors[1] = Motor_Init(Conf_GetPtr(dict, "motorFR", ConfItem));
	chassis.motors[2] = Motor_Init(Conf_GetPtr(dict, "motorBL", ConfItem));
	chassis.motors[3] = Motor_Init(Conf_GetPtr(dict, "motorBR", ConfItem));
	for(uint8_t i = 0; i<4; i++)
	{
		chassis.motors[i]->changeMode(chassis.motors[i], MOTOR_SPEED_MODE);
	}
	SoftBus_Subscribe(NULL, Chassis_SoftBusCallback, "chassis");
}


//����б�¼����ٶ�
void Chassis_UpdateSlope()
{
	Slope_NextVal(&chassis.move.xSlope);
	Slope_NextVal(&chassis.move.ySlope);
}

//��������ص�����
void Chassis_TaskCallback(void const * argument)
{
	Chassis_Init((ConfItem*)argument);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{		
		/*************�������ƽ���ٶ�**************/
		
		Chassis_UpdateSlope();//�����˶�б�º�������

		//����̨����ϵ��ƽ���ٶȽ��㵽����ƽ���ٶ�(������̨ƫ���)
		float gimbalAngleSin=arm_sin_f32(chassis.relativeAngle*PI/180);
		float gimbalAngleCos=arm_cos_f32(chassis.relativeAngle*PI/180);
		chassis.move.vx=Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
		chassis.move.vy=-Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
		
		/*************���������ת��**************/
		
		float rotateRatio[4];
		rotateRatio[0]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[1]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY-chassis.info.offsetX;
		rotateRatio[2]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[3]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY-chassis.info.offsetX;
		float wheelRPM[4];
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius);//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius);//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius);//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius);//BR
		
		for(uint8_t i = 0; i<4; i++)
		{
			chassis.motors[i]->setTarget(chassis.motors[i], wheelRPM[i]);
		}
		
		osDelayUntil(&tick,chassis.taskInterval);
	}
}

void Chassis_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
//	if(!strcmp(topic, "chassis"))
//	{
		const SoftBusItem* item = SoftBus_GetItem(frame, "relativeAngle");
		if(!item)
		{
			chassis.relativeAngle = *(float*)item->data;
		}
		item = SoftBus_GetItem(frame, "vx");
		if(!item)
		{
			Slope_SetTarget(&chassis.move.xSlope, *(float*)item->data);
		}
		item = SoftBus_GetItem(frame, "vy");
		if(!item)
		{
			Slope_SetTarget(&chassis.move.ySlope, *(float*)item->data);
		}
		item = SoftBus_GetItem(frame, "vw");
		if(!item)
		{
			chassis.move.vw = *(float*)item->data;
		}
		item = SoftBus_GetItem(frame, "ax");
		if(!item)
		{
			Slope_SetStep(&chassis.move.xSlope, CHASSIS_ACC2SLOPE(*(float*)item->data));
		}
		item = SoftBus_GetItem(frame, "ay");
		if(!item)
		{
			Slope_SetStep(&chassis.move.ySlope, CHASSIS_ACC2SLOPE(*(float*)item->data));
		}
//	}
}
