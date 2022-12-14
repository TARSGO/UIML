#include "softbus.h"
#include "motor.h"
#include "config.h"

typedef struct _Servo
{
	Motor motor;
	struct
	{
		uint8_t timX;
		uint8_t	channelX;
	}timInfo;
	float  targetAngle;//Ŀ��Ƕ�(��λ��)
	float  maxAngle;
	float  maxDuty,minDuty;
}Servo;

Motor* Servo_Init(ConfItem* dict);

void Servo_SetTarget(Motor* motor,float targetValue);

Motor* Servo_Init(ConfItem* dict)
{
	//���������ڴ�ռ�
	Servo* servo = MOTOR_MALLOC_PORT(sizeof(Servo));
	memset(servo,0,sizeof(Servo));
	//�����̬
	servo->motor.setTarget = Servo_SetTarget;
	//��ʼ�������TIM��Ϣ
	servo->timInfo.timX = Conf_GetValue(dict,"timX",uint8_t,0);
	servo->timInfo.channelX = Conf_GetValue(dict,"channelX",uint8_t,0);
	servo->maxAngle = Conf_GetValue(dict,"maxAngle",float,180);
	servo->maxDuty = Conf_GetValue(dict,"maxDuty",float,0.125f);
	servo->minDuty = Conf_GetValue(dict,"minDuty",float,0.025f);

	return (Motor*)servo;
}

//���ö���Ƕ�
void Servo_SetTarget(Motor* motor,float targetValue)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle = targetValue; //��ʵ����;��������debug
	float pwmDuty = servo->targetAngle / servo->maxAngle * (servo->maxDuty - servo->minDuty) + servo->minDuty;
	Bus_BroadcastSend("/tim/pwm/set-duty", {{"timX", &servo->timInfo.timX}, {"channelX", &servo->timInfo.channelX}, {"duty", &pwmDuty}});
}


