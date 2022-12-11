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
		uint32_t pwmValue;
		
	}timInfo;
	float  targetAngle;//Ŀ��ֵ(�����Ť�ؾ�/�ٶ�/�Ƕ�(��λ��))
	float  maxAngle;
	
}Servo;

Motor* Servo_Init(ConfItem* dict);

void Servo_SetTarget(struct _Motor* motor,float targetValue);
void Servo_SetStartAngle(Motor *motor, float startAngle);
void Servo_TimerCallback(void const *argument);

//�����ʱ���ص�����
void Servo_TimerCallback(void const *argument)
{
	Servo *servo=(Servo*) argument;
	float pwmDuty=(servo->targetAngle/servo->maxAngle+1)/20.0f;
	Bus_BroadcastSend("/tim/set-pwm-duty",{{"timX",&servo->timInfo.timX},{"channelX",&servo->timInfo.channelX},{"duty",&pwmDuty}});
}

Motor* Servo_Init(ConfItem* dict)
{
	//���������ڴ�ռ�
	Servo* servo = MOTOR_MALLOC_PORT(sizeof(Servo));
	memset(servo,0,sizeof(Servo));
	//�����̬
	servo->motor.setTarget = Servo_SetTarget;
	servo->motor.setStartAngle = Servo_SetStartAngle;
	//��ʼ�������TIM��Ϣ
	servo->timInfo.timX=Conf_GetValue(dict,"timX",uint8_t,0);
	servo->timInfo.channelX=Conf_GetValue(dict,"channelX",uint8_t,0);
	servo->maxAngle=Conf_GetValue(dict,"maxAngle",float,180);
	//���������ʱ��
	osTimerDef(Servo, Servo_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(Servo), osTimerPeriodic,servo), 2);

	return (Motor*)servo;
}

//���ö����ʼ�Ƕ�
void Servo_SetStartAngle(Motor *motor, float startAngle)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle=startAngle;
	//���Ƕ�תΪռ�ձ�
	float pwmDuty=(servo->targetAngle/servo->maxAngle+1)/20.0f;
	Bus_BroadcastSend("/tim/set-pwm-duty",{{"timX",&servo->timInfo.timX},{"channelX",&servo->timInfo.channelX},{"duty",&pwmDuty}});
}

//���ö���Ƕ�
void Servo_SetTarget(struct _Motor* motor,float targetValue)
{
	Servo* servo=(Servo*) motor;
	servo->targetAngle=targetValue;
}


