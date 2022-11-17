#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//���ֵ������ֵ��ǶȵĻ���	
#define M2006_DGR2CODE(dgr) ((int32_t)(dgr*819.1f)) //36*8191/360
#define M2006_CODE2DGR(code) ((float)(code/819.1f))

typedef struct _M2006
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t sendID,recvID;
		uint8_t canX;
		uint8_t bufIndex;
	}canInfo;
	MotorCtrlMode mode;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//��¼��һ�εõ��ĽǶ�
	
	int32_t totalAngle;//�ۼ�ת���ı�����ֵ
	
	float  targetValue;//Ŀ��ֵ(�ٶ�/�Ƕ�(������ֵ))
	
	PID speedPID;//�ٶ�pid(����)
	CascadePID anglePID;//�Ƕ�pid������
	
}M2006;

Motor* M2006_Init(ConfItem* dict);

void M2006_StartStatAngle(Motor *motor);
void M2006_StatAngle(Motor* motor);
void M2006_SetTarget(Motor* motor, float targetValue);
void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode);

void M2006_Update(M2006* m2006,uint8_t* data);
void M2006_PIDInit(M2006* m2006, ConfItem* dict);
void M2006_CtrlerCalc(M2006* m2006, float reference);

void M2006_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M2006* m2006 = (M2006*)bindData;

	const SoftBusItem* item = NULL;

	item = SoftBus_GetItem(frame, "can-x");
	if(!item || *(uint8_t*)item->data != m2006->canInfo.canX)
		return;

	item = SoftBus_GetItem(frame, "id");
	if(!item || *(uint16_t*)item->data != m2006->canInfo.recvID)
		return;
		
	item = SoftBus_GetItem(frame, "data");
	if(item)
		M2006_Update(m2006, item->data);
}

void M2006_TimerCallback(void const *argument)
{
	M2006* m2006 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M2006_CtrlerCalc(m2006, m2006->targetValue);
}

Motor* M2006_Init(ConfItem* dict)
{
	M2006* m2006 = MOTOR_MALLOC_PORT(sizeof(M2006));
	memset(m2006,0,sizeof(M2006));
	
	m2006->motor.setTarget = M2006_SetTarget;
	m2006->motor.changeMode = M2006_ChangeMode;
	m2006->motor.startStatAngle = M2006_StartStatAngle;
	m2006->motor.statAngle = M2006_StatAngle;
	
	m2006->reductionRatio = 36;
	
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m2006->canInfo.recvID = id + 0x200;
	m2006->canInfo.sendID = (id <= 4) ? 0x200 : 0x1FF;
	m2006->canInfo.bufIndex =  (id - 1) * 2;
	m2006->canInfo.canX = Conf_GetValue(dict, "canX", uint8_t, 0);
	
	m2006->mode = MOTOR_TORQUE_MODE;
	M2006_PIDInit(m2006, dict);
	SoftBus_Subscribe(m2006, M2006_SoftBusCallback, "/can/recv");
	
	osTimerDef(M2006, M2006_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M2006), osTimerPeriodic, m2006), 2);

	return (Motor*)m2006;
}

void M2006_PIDInit(M2006* m2006, ConfItem* dict)
{
	PID_Init(&m2006->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m2006->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m2006->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m2006->anglePID.outer, m2006->anglePID.outer.maxOutput*m2006->reductionRatio);
}

//��ʼͳ�Ƶ���ۼƽǶ�
void M2006_StartStatAngle(Motor *motor)
{
	M2006* m2006 = (M2006*)motor;
	
	m2006->totalAngle=0;
	m2006->lastAngle=m2006->angle;
}

//ͳ�Ƶ���ۼ�ת����Ȧ��
void M2006_StatAngle(Motor* motor)
{
	M2006* m2006 = (M2006*)motor;
	
	int32_t dAngle=0;
	if(m2006->angle-m2006->lastAngle<-4000)
		dAngle=m2006->angle+(8191-m2006->lastAngle);
	else if(m2006->angle-m2006->lastAngle>4000)
		dAngle=-m2006->lastAngle-(8191-m2006->angle);
	else
		dAngle=m2006->angle-m2006->lastAngle;
	//���Ƕ��������������
	m2006->totalAngle+=dAngle;
	//��¼�Ƕ�
	m2006->lastAngle=m2006->angle;
}

void M2006_CtrlerCalc(M2006* m2006, float reference)
{
	int16_t output;
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m2006->speedPID, reference*m2006->reductionRatio, m2006->speed);
		output = m2006->speedPID.output;
	}
	else if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&m2006->anglePID, reference, m2006->totalAngle, m2006->speed);
		output = m2006->anglePID.output;
	}
	else if(m2006->mode == MOTOR_TORQUE_MODE)
	{
		output = (int16_t)reference;
	}
	SoftBus_PublishMap("/can/set-buf",{
		{"can-x", &m2006->canInfo.canX, sizeof(uint8_t)},
		{"id", &m2006->canInfo.sendID, sizeof(uint16_t)},
		{"pos", &m2006->canInfo.bufIndex, sizeof(uint8_t)},
		{"len", &(uint8_t){2}, sizeof(uint8_t)},
		{"data", &output, sizeof(int16_t)}
	});
}

void M2006_SetTarget(Motor* motor, float targetValue)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		m2006->targetValue = M2006_DGR2CODE(targetValue);
	}
	else 
	{
		m2006->targetValue = targetValue;
	}
}

void M2006_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&m2006->speedPID);
	}
	else if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&m2006->anglePID.inner);
		PID_Clear(&m2006->anglePID.outer);
	}
	m2006->mode = mode;
}

//���µ������(���ܽ����˲�)
void M2006_Update(M2006* m2006,uint8_t* data)
{
	m2006->angle = (data[0]<<8 | data[1]);
	m2006->speed = (data[2]<<8 | data[3]);
}
