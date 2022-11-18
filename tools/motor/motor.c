#include "motor.h"

//X-MACRO
//�����б�ÿһ���ʽΪ(������,��ʼ��������)
#define MOTOR_CHILD_LIST \
	MOTOR_TYPE("M3508",M3508_Init) \
	MOTOR_TYPE("M2006",M2006_Init) \
	MOTOR_TYPE("M6020",M6020_Init)

//�ڲ���������
void Motor_SetTarget(Motor* motor, float targetValue);
void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode ctrlerType);
void Motor_StartStatAngle(Motor* motor);
void Motor_StatAngle(Motor* motor);
void Motor_InitDefault(Motor* motor);

//���������ʼ������
#define MOTOR_TYPE(name,initFunc) extern Motor* initFunc(ConfItem*);
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
	//������δ����ķ�������Ϊ�պ���
	if(motor)
		Motor_InitDefault(motor);

	return motor;
}

void Motor_InitDefault(Motor* motor)
{
	if(!motor->changeMode)
		motor->changeMode = Motor_ChangeCtrler;
	if(!motor->setTarget)
		motor->setTarget = Motor_SetTarget;
	if(!motor->startStatAngle)
		motor->startStatAngle = Motor_StartStatAngle;
	if(!motor->statAngle)
		motor->statAngle = Motor_StatAngle;
}

//���麯��
void Motor_SetTarget(Motor* motor, float targetValue) { }

void Motor_ChangeCtrler(Motor* motor, MotorCtrlMode mode) { }

void Motor_StartStatAngle(Motor* motor) { }

void Motor_StatAngle(Motor* motor) { }
