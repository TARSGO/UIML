#include "M6020.h"
#include "softbus.h"

Motor* M6020_Init(ConfItem* dict);

void M6020_StartStatAngle(Motor *motor);
void M6020_StatAngle(Motor* motor);
void M6020_CtrlerCalc(Motor* motor, float reference);
void M6020_ChangeMode(Motor* motor, MotorCtrlMode mode);

void M6020_Update(M6020* m6020,uint8_t* data);
void M6020_PIDInit(M6020* m6020, ConfItem* dict);

void M6020_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M6020* m6020 = (M6020*)bindData;
	if(!strcmp(topic, m6020->canInfo.canX[0]))
	{
		uint8_t* data = (uint8_t*)frame->data;
		if(m6020->canInfo.id[0] == data[0])
		{
			M6020_Update(m6020, data+1);
		}
	}
}

Motor* M6020_Init(ConfItem* dict)
{
	M6020* m6020 = pvPortMalloc(sizeof(M6020));
	memset(m6020,0,sizeof(M6020));
	
	m6020->motor.ctrlerCalc = M6020_CtrlerCalc;
	m6020->motor.changeMode = M6020_ChangeMode;
	m6020->motor.startStatAngle = M6020_StartStatAngle;
	m6020->motor.statAngle = M6020_StatAngle;
	
	m6020->reductionRatio = 1;
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m6020->canInfo.id[0] = id+0x200;
	m6020->canInfo.id[1] = (m6020->canInfo.id[0]<0x205)?0x1FF:0x2FF;
	id = (id-1)%4;
	m6020->canInfo.sendBits =  1<<(2*id-2) | 1<<(2*id-1);
	char* canX = Conf_GetPtr(dict, "canX", char);
	char* send = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+4+1)*sizeof(char));
	char* receive = (char*)MOTOR_MALLOC_PORT((MOTOR_STRLEN_PORT(canX)+7+1)*sizeof(char));
	MOTOR_STRCAT_PORT(send, "Send");
	MOTOR_STRCAT_PORT(receive, "Receive");
	m6020->canInfo.canX[0] = receive;
	m6020->canInfo.canX[1] = send;
	m6020->mode = MOTOR_TORQUE_MODE;
	M6020_PIDInit(m6020, dict);
	SoftBus_Subscribe(m6020, M6020_SoftBusCallback, m6020->canInfo.canX[0]);

	return (Motor*)m6020;
}

void M6020_PIDInit(M6020* m6020, ConfItem* dict)
{
	float speedP, speedI, speedD, speedMaxI, speedMaxOut;
	speedP = Conf_GetValue(dict, "speedPID/p", uint16_t, 0);
	speedI = Conf_GetValue(dict, "speedPID/i", uint16_t, 0);
	speedD = Conf_GetValue(dict, "speedPID/d", uint16_t, 0);
	speedMaxI = Conf_GetValue(dict, "speedPID/maxI", uint16_t, 0);
	speedMaxOut = Conf_GetValue(dict, "speedPID/maxOut", uint16_t, 0);
	PID_Init(&m6020->speedPID, speedP, speedI, speedD, speedMaxI, speedMaxOut);
	float angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut;
	angleInnerP = Conf_GetValue(dict, "anglePID/inner/p", uint16_t, 0);
	angleInnerI = Conf_GetValue(dict, "anglePID/inner/i", uint16_t, 0);
	angleInnerD = Conf_GetValue(dict, "anglePID/inner/d", uint16_t, 0);
	angleInnerMaxI = Conf_GetValue(dict, "anglePID/inner/maxI", uint16_t, 0);
	angleInnerMaxOut = Conf_GetValue(dict, "anglePID/inner/maxOut", uint16_t, 0);
	PID_Init(&m6020->anglePID.inner, angleInnerP, angleInnerI, angleInnerD, angleInnerMaxI, angleInnerMaxOut);
	float angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut;
	angleOuterP = Conf_GetValue(dict, "anglePID/outer/p", uint16_t, 0);
	angleOuterI = Conf_GetValue(dict, "anglePID/outer/i", uint16_t, 0);
	angleOuterD = Conf_GetValue(dict, "anglePID/outer/d", uint16_t, 0);
	angleOuterMaxI = Conf_GetValue(dict, "anglePID/outer/maxI", uint16_t, 0);
	angleOuterMaxOut = Conf_GetValue(dict, "anglePID/outer/maxOut", uint16_t, 0);
	PID_Init(&m6020->anglePID.outer, angleOuterP, angleOuterI, angleOuterD, angleOuterMaxI, angleOuterMaxOut*m6020->reductionRatio);
}

//��ʼͳ�Ƶ���ۼƽǶ�
void M6020_StartStatAngle(Motor *motor)
{
	M6020* m6020 = (M6020*)motor;
	
	m6020->totalAngle=0;
	m6020->lastAngle=m6020->angle;
}

//ͳ�Ƶ���ۼ�ת����Ȧ��
void M6020_StatAngle(Motor* motor)
{
	M6020* m6020 = (M6020*)motor;
	
	int32_t dAngle=0;
	if(m6020->angle-m6020->lastAngle<-4000)
		dAngle=m6020->angle+(8191-m6020->lastAngle);
	else if(m6020->angle-m6020->lastAngle>4000)
		dAngle=-m6020->lastAngle-(8191-m6020->angle);
	else
		dAngle=m6020->angle-m6020->lastAngle;
	//���Ƕ��������������
	m6020->totalAngle+=dAngle;
	//��¼�Ƕ�
	m6020->lastAngle=m6020->angle;
}

void M6020_CtrlerCalc(Motor* motor, float reference)
{
	M6020* m6020 = (M6020*)motor;
	int16_t output;
	if(m6020->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m6020->speedPID, reference*m6020->reductionRatio, m6020->speed);
		output = m6020->speedPID.output;
	}
	else if(m6020->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&m6020->anglePID, reference, m6020->totalAngle, m6020->speed);
		output = m6020->anglePID.output;
	}
	else if(m6020->mode == MOTOR_TORQUE_MODE)
	{
		output = (int16_t)reference;
	}
	SoftBus_PublishMap(m6020->canInfo.canX[1],{
		{"id", &m6020->canInfo.id[1], sizeof(uint32_t)},
		{"bits", &m6020->canInfo.sendBits, sizeof(uint8_t)},
		{"data", &output, sizeof(int16_t)}
	});
}

void M6020_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	M6020* m6020 = (M6020*)motor;
	if(m6020->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&m6020->speedPID);
	}
	else if(m6020->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&m6020->anglePID.inner);
		PID_Clear(&m6020->anglePID.outer);
	}
	m6020->mode = mode;
}

//���µ������(���ܽ����˲�)
void M6020_Update(M6020* m6020,uint8_t* data)
{
	m6020->angle = (data[0]<<8 | data[1]);
	m6020->speed = (data[2]<<8 | data[3]);
}
