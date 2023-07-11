#include "motor.h"
#include <string.h>

//X-MACRO
//�����б�ÿһ���ʽΪ(������,��ʼ��������)
#define MOTOR_CHILD_LIST \
	MOTOR_TYPE("M3508",M3508_Init) \
	MOTOR_TYPE("M2006",M2006_Init) \
	MOTOR_TYPE("M6020",M6020_Init) \
	MOTOR_TYPE("Servo",Servo_Init) \
	MOTOR_TYPE("DcMotor",DcMotor_Init) 

//�ڲ���������
void Motor_SetTarget(Motor* motor, float targetValue);
void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode ctrlerType);
void Motor_InitTotalAngle(Motor* motor, float angle);
float Motor_GetData(Motor* motor, const char* data);
void Motor_Stop(Motor* motor);
void Motor_InitDefault(Motor* motor);

//���������ʼ������
#define MOTOR_TYPE(name,initFunc) __weak Motor* initFunc(ConfItem* dict) {return NULL;}
MOTOR_CHILD_LIST
#undef MOTOR_TYPE

Motor* Motor_Init(ConfItem* dict)
{
	char* motorType = Conf_GetPtr(dict, "type", char);

	Motor *motor = NULL;
	//�ж������ĸ�����
	#define MOTOR_TYPE(name,initFunc) \
	if(!strcmp(motorType, name)) \
		motor = initFunc(dict);
	MOTOR_CHILD_LIST
	#undef MOTOR_TYPE
	if(!motor)
	{
		motor = MOTOR_MALLOC_PORT(sizeof(Motor));
		memset(motor, 0, sizeof(Motor));
	}
	
	//������δ����ķ�������Ϊ�պ���
	Motor_InitDefault(motor);

	return motor;
}

void Motor_InitDefault(Motor* motor)
{
	if(!motor->changeMode)
		motor->changeMode = Motor_ChangeCtrler;
	if(!motor->setTarget)
		motor->setTarget = Motor_SetTarget;
	if(!motor->initTotalAngle)
		motor->initTotalAngle = Motor_InitTotalAngle;
	if(!motor->getData)
		motor->getData = Motor_GetData;
	if(!motor->stop)
		motor->stop = Motor_Stop;
}

//���麯��
void Motor_SetTarget(Motor* motor, float targetValue) { }

void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode mode) { }

void Motor_InitTotalAngle(Motor* motor, float angle) { }

float Motor_GetData(Motor* motor, const char* data) {return 0;}

void Motor_Stop(Motor* motor) { }
