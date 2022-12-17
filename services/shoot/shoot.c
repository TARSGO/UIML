#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"
#include "main.h"

typedef enum
{
	SHOOTER_MODE_IDLE,
	SHOOTER_MODE_ONCE,
	SHOOTER_MODE_CONTINUE,
	SHOOTER_MODE_BLOCK
}ShooterMode;

typedef struct
{
	Motor *fricMotors[2],*triggerMotor;
	bool fricEnable;
	float fricSpeed; //Ħ�����ٶ�
	float triggerAngle,totalTrigAngle; //����һ�νǶȼ��ۼƽǶ�
	uint16_t continueNum;  //��������
	uint16_t intervalTime; //�������ms
	uint8_t mode;
	uint8_t taskInterval;
}Shooter;

void Shooter_Init(Shooter* shooter, ConfItem* dict);
void Shooter_shootCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shooter_ChangeArgumentCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shooter_FricCtrlCallback(const char* name, SoftBusFrame* frame, void* bindData);
  
void Shooter_TaskCallback(void const * argument)
{
	portENTER_CRITICAL();
	Shooter shooter={0};
	Shooter_Init(&shooter, (ConfItem*)argument);
	TickType_t tick = xTaskGetTickCount();
	portEXIT_CRITICAL();

  osDelay(2000);
	while(1)
	{
		switch(shooter.mode)
		{
			case SHOOTER_MODE_IDLE:
				break;
			case SHOOTER_MODE_BLOCK:
				shooter.triggerMotor->setStartAngle(shooter.triggerMotor,shooter.totalTrigAngle); //���õ���Ƕ�Ϊ��ǰ�ۼƽǶ�
				shooter.totalTrigAngle -= shooter.triggerAngle*0.5f;  //��ת
				shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.totalTrigAngle);
				osDelay(500);   //�ȴ������ת�ָ�
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_ONCE:   //����
			
				if(shooter.fricEnable == false)   //��Ħ����δ�������ȿ���
				{
					Bus_BroadcastSend("/shooter/fricCtrl",{"enable",IM_PTR(bool,true)});
					osDelay(200);     //�ȴ�Ħ����ת���ȶ�
				}

				shooter.totalTrigAngle += shooter.triggerAngle; 
				shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.totalTrigAngle);
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_CONTINUE:  //��һ����ʱ������������n�� 
			
				if(shooter.fricEnable == false)   //��Ħ����δ�������ȿ���
				{
					Bus_BroadcastSend("/shooter/fricCtrl",{"enable",IM_PTR(bool,true)});
					osDelay(200);   //�ȴ�Ħ����ת���ȶ�
				}
				//����
				for(uint16_t num;num<shooter.continueNum;num++)
				{
					shooter.totalTrigAngle += shooter.triggerAngle; 
					shooter.triggerMotor->setTarget(shooter.triggerMotor,shooter.totalTrigAngle);
					osDelay(shooter.intervalTime);
				}
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			default:
				break;
		}
		osDelayUntil(&tick,shooter.taskInterval);
	}	
}

void Shooter_Init(Shooter* shooter, ConfItem* dict)
{
	//������
	shooter->taskInterval = Conf_GetValue(dict, "taskInterval", uint8_t, 5);
	//��ʼ����
	shooter->fricSpeed = Conf_GetValue(dict,"fricSpeed",float,5450);
	//�����ֲ���һ������ת��
	shooter->triggerAngle = Conf_GetValue(dict,"triggerAngle",float,1/7.0*360);
	//������������ʼ��
	shooter->fricMotors[0] = Motor_Init(Conf_GetPtr(dict, "fricMotorLeft", ConfItem));
	shooter->fricMotors[1] = Motor_Init(Conf_GetPtr(dict, "fricMotorRight", ConfItem));
	shooter->triggerMotor = Motor_Init(Conf_GetPtr(dict, "triggerMotor", ConfItem));
	//���÷���������ģʽ
	for(uint8_t i = 0; i<2; i++)
	{
		shooter->fricMotors[i]->changeMode(shooter->fricMotors[i], MOTOR_SPEED_MODE);
	}
	shooter->triggerMotor->changeMode(shooter->triggerMotor,MOTOR_ANGLE_MODE);
  
	Bus_MultiRegisterReceiver(shooter, Shooter_shootCallback, {"/shooter/once","/shooter/continue"});
	Bus_MultiRegisterReceiver(shooter, Shooter_ChangeArgumentCallback, {"/shooter/fricSpeed","/shooter/triggerAngle"});
	Bus_RegisterReceiver(shooter,Shooter_BlockCallback,"/motor/stall");
	Bus_RegisterReceiver(shooter,Shooter_FricCtrlCallback,"/shooter/fricCtrl");
}

//���ģʽ
void Shooter_shootCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData ;
	if(!strcmp(name,"/shooter/once")&&shooter->mode == SHOOTER_MODE_IDLE)  //����ʱ�������޸�ģʽ
	{
		shooter->mode = SHOOTER_MODE_ONCE;
	}
	else if(!strcmp(name,"/shooter/continue")&&shooter->mode == SHOOTER_MODE_IDLE)
	{
		if(!Bus_CheckMapKeys(frame,{"num","intervalTime"}))
			return;
		shooter->continueNum = *(uint16_t*)Bus_GetMapValue(frame,"num");
		shooter->intervalTime = *(uint16_t*)Bus_GetMapValue(frame,"intervalTime");
		shooter->mode = SHOOTER_MODE_CONTINUE;
	}
}

//�޸�Ħ����ת�ٻ򲦵��Ƕ�
void Shooter_ChangeArgumentCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	if(!strcmp(name,"/shooter/fricSpeed"))
	{
		if(!Bus_IsMapKeyExist(frame,"speed"))
			return;
		shooter->fricSpeed = *(float*)Bus_GetMapValue(frame,"speed");
	}
	else if(!strcmp(name,"/shooter/triggerAngle"))
	{
		if(!Bus_IsMapKeyExist(frame,"angle"))
			return;
		shooter->triggerAngle = *(float*)Bus_GetMapValue(frame,"angle");
	}
}

//��ת
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	if(!Bus_IsMapKeyExist(frame,"motor"))
		return;
	Motor *motor = (Motor *)Bus_GetMapValue(frame,"motor");
	if(motor == shooter->triggerMotor)
	{
		shooter->mode = SHOOTER_MODE_BLOCK;
	}
}

//Ħ���ֿ���
void Shooter_FricCtrlCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
		if(!Bus_IsMapKeyExist(frame,"enable"))
		return;
	shooter->fricEnable = *(bool*)Bus_GetMapValue(frame,"enable");
	if(shooter->fricEnable == false)
	{
		shooter->fricMotors[0]->setTarget(shooter->fricMotors[0],0);
		shooter->fricMotors[1]->setTarget(shooter->fricMotors[0],0);
	}
	else
	{
		shooter->fricMotors[0]->setTarget(shooter->fricMotors[0],-shooter->fricSpeed);
		shooter->fricMotors[1]->setTarget(shooter->fricMotors[1], shooter->fricSpeed);
	}
	}

