#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"
#include "arm_math.h"

#ifndef LIMIT
#define LIMIT(x,min,max) (x)=(((x)<=(min))?(min):(((x)>=(max))?(max):(x)))
#endif
#define CHASSIS_ACC2SLOPE(taskInterval,acc) ((taskInterval)*(acc)/1000) //mm/s2

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
	
	float relativeAngle; //����̵�ƫ��ǣ���λ��
	
	uint8_t taskInterval;

	char* speedName;
	char* accName;
	char* relAngleName;
	
}Chassis;

void Chassis_Init(Chassis* chassis, ConfItem* dict);
void Chassis_UpdateSlope(Chassis* chassis);
bool Chassis_SetSpeedCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Chassis_SetAccCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Chassis_SetRelativeAngleCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

//��������ص�����
void Chassis_TaskCallback(void const * argument)
{
	//�����ٽ���
	portENTER_CRITICAL();
	Chassis chassis={0};
	Chassis_Init(&chassis, (ConfItem*)argument);
	portEXIT_CRITICAL();
	
	osDelay(2000);
	while(1)
	{		
		/*************�������ƽ���ٶ�**************/
		
		Chassis_UpdateSlope(&chassis);//�����˶�б�º�������

		//����̨����ϵ��ƽ���ٶȽ��㵽����ƽ���ٶ�(������̨ƫ���)
		float gimbalAngleSin=arm_sin_f32(chassis.relativeAngle*PI/180);
		float gimbalAngleCos=arm_cos_f32(chassis.relativeAngle*PI/180);
		chassis.move.vx=Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
		chassis.move.vy=-Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
		float vw = chassis.move.vw/180*PI;
		
		/*************���������ת��**************/
		
		float rotateRatio[4];
		rotateRatio[0]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[1]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY-chassis.info.offsetX;
		rotateRatio[2]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[3]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY-chassis.info.offsetX;
		float wheelRPM[4];
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius);//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius);//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius);//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius);//BR
		
		for(uint8_t i = 0; i<4; i++)
		{
			chassis.motors[i]->setTarget(chassis.motors[i], wheelRPM[i]);
		}
		
		osDelay(chassis.taskInterval);
	}
}

void Chassis_Init(Chassis* chassis, ConfItem* dict)
{
	//������
	chassis->taskInterval = Conf_GetValue(dict, "task-interval", uint8_t, 2);
	//���̳ߴ���Ϣ�����ڽ������٣�
	chassis->info.wheelbase = Conf_GetValue(dict, "info/wheelbase", float, 0);
	chassis->info.wheeltrack = Conf_GetValue(dict, "info/wheeltrack", float, 0);
	chassis->info.wheelRadius = Conf_GetValue(dict, "info/wheel-radius", float, 76);
	chassis->info.offsetX = Conf_GetValue(dict, "info/offset-x", float, 0);
	chassis->info.offsetY = Conf_GetValue(dict, "info/offset-y", float, 0);
	//�ƶ�������ʼ��
	chassis->move.maxVx = Conf_GetValue(dict, "move/max-vx", float, 2000);
	chassis->move.maxVy = Conf_GetValue(dict, "move/max-vy", float, 2000);
	chassis->move.maxVw = Conf_GetValue(dict, "move/max-vw", float, 180);
	//���̼��ٶȳ�ʼ��
	float xAcc = Conf_GetValue(dict, "move/x-acc", float, 1000);
	float yAcc = Conf_GetValue(dict, "move/y-acc", float, 1000);
	Slope_Init(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, xAcc),0);
	Slope_Init(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, yAcc),0);
	//���̵����ʼ��
	chassis->motors[0] = Motor_Init(Conf_GetPtr(dict, "motor-fl", ConfItem));
	chassis->motors[1] = Motor_Init(Conf_GetPtr(dict, "motor-fr", ConfItem));
	chassis->motors[2] = Motor_Init(Conf_GetPtr(dict, "motor-bl", ConfItem));
	chassis->motors[3] = Motor_Init(Conf_GetPtr(dict, "motor-br", ConfItem));
	//���õ��̵��Ϊ�ٶ�ģʽ
	for(uint8_t i = 0; i<4; i++)
	{
		chassis->motors[i]->changeMode(chassis->motors[i], MOTOR_SPEED_MODE);
	}
	//�����߹㲥��Զ�̺���name��ӳ��
	char* temp = Conf_GetPtr(dict, "chassis", char);
	temp = temp ? temp : "chassis";
	uint8_t len = strlen(temp);
	chassis->speedName = pvPortMalloc(len + 7+ 1); //7Ϊ"/   /speed"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(chassis->speedName, "/%s/speed", temp);
	
	chassis->accName = pvPortMalloc(len + 5+ 1); //5Ϊ"/   /acc"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(chassis->accName, "/%s/acc", temp);
	
	chassis->relAngleName = pvPortMalloc(len + 16+ 1); //16Ϊ"/   /relative-angle"�ĳ��ȣ�1Ϊ'\0'�ĳ���
	sprintf(chassis->relAngleName, "/%s/relative-angle", temp);
	
	//ע��Զ�̺���
	Bus_RegisterRemoteFunc(chassis, Chassis_SetSpeedCallback, chassis->speedName);
	Bus_RegisterRemoteFunc(chassis, Chassis_SetAccCallback, chassis->accName);
	Bus_RegisterRemoteFunc(chassis, Chassis_SetRelativeAngleCallback, chassis->relAngleName);
	Bus_RegisterReceiver(chassis, Chassis_StopCallback, "/system/stop");
}


//����б�¼����ٶ�
void Chassis_UpdateSlope(Chassis* chassis)
{
	Slope_NextVal(&chassis->move.xSlope);
	Slope_NextVal(&chassis->move.ySlope);
}
//���õ����ٶȻص�
bool Chassis_SetSpeedCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Chassis* chassis = (Chassis*)bindData;
	if(Bus_IsMapKeyExist(frame, "vx"))
	{
		float vx = *(float*)Bus_GetMapValue(frame, "vx");
		LIMIT(vx, -chassis->move.maxVx, chassis->move.maxVx);
		Slope_SetTarget(&chassis->move.xSlope, vx);
	}
	if(Bus_IsMapKeyExist(frame, "vy"))
	{
		float vy = *(float*)Bus_GetMapValue(frame, "vy");
		LIMIT(vy, -chassis->move.maxVy, chassis->move.maxVy);
		Slope_SetTarget(&chassis->move.ySlope, vy);
	}
	if(Bus_IsMapKeyExist(frame, "vw"))
	{
		chassis->move.vw = *(float*)Bus_GetMapValue(frame, "vw");
		LIMIT(chassis->move.vw, -chassis->move.maxVw, chassis->move.maxVw);
	}
	return true;
}
//���õ��̼��ٶȻص�
bool Chassis_SetAccCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Chassis* chassis = (Chassis*)bindData;
	if(Bus_IsMapKeyExist(frame, "ax"))
		Slope_SetStep(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float*)Bus_GetMapValue(frame, "ax")));
	if(Bus_IsMapKeyExist(frame, "ay"))
		Slope_SetStep(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, *(float*)Bus_GetMapValue(frame, "ay")));
	return true;
}
//���õ�������ϵ����ǶȻص�
bool Chassis_SetRelativeAngleCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Chassis* chassis = (Chassis*)bindData;
	if(Bus_IsMapKeyExist(frame, "angle"))
		chassis->relativeAngle = *(float*)Bus_GetMapValue(frame, "angle");
	return true;
}
//���̼�ͣ�ص�
void Chassis_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Chassis* chassis = (Chassis*)bindData;
	for(uint8_t i = 0; i<4; i++)
	{
		chassis->motors[i]->stop(chassis->motors[i]);
	}
}
