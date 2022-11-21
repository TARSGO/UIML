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
void M2006_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

void M2006_Update(M2006* m2006,uint8_t* data);
void M2006_PIDInit(M2006* m2006, ConfItem* dict);
void M2006_CtrlerCalc(M2006* m2006, float reference);

//�����ʱ���ص�����
void M2006_TimerCallback(void const *argument)
{
	M2006* m2006 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M2006_CtrlerCalc(m2006, m2006->targetValue);
}

Motor* M2006_Init(ConfItem* dict)
{
	//���������ڴ�ռ�
	M2006* m2006 = MOTOR_MALLOC_PORT(sizeof(M2006));
	memset(m2006,0,sizeof(M2006));
	//�����̬
	m2006->motor.setTarget = M2006_SetTarget;
	m2006->motor.changeMode = M2006_ChangeMode;
	m2006->motor.startStatAngle = M2006_StartStatAngle;
	m2006->motor.statAngle = M2006_StatAngle;
	//������ٱ�
	m2006->reductionRatio = 36;
	//��ʼ�������can��Ϣ
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m2006->canInfo.recvID = id + 0x200;
	m2006->canInfo.sendID = (id <= 4) ? 0x200 : 0x1FF;
	m2006->canInfo.bufIndex =  (id - 1) * 2;
	m2006->canInfo.canX = Conf_GetValue(dict, "canX", uint8_t, 0);
	//���õ��Ĭ��ģʽΪŤ��ģʽ
	m2006->mode = MOTOR_TORQUE_MODE;
	//��ʼ�����pid
	M2006_PIDInit(m2006, dict);
	//����can��Ϣ
	char topic[] = "/can_/recv";
	topic[4] = m2006->canInfo.canX + '0';
	SoftBus_Subscribe(m2006, M2006_SoftBusCallback, topic);
	//���������ʱ��
	osTimerDef(M2006, M2006_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M2006), osTimerPeriodic, m2006), 2);

	return (Motor*)m2006;
}
//��ʼ��pid
void M2006_PIDInit(M2006* m2006, ConfItem* dict)
{
	PID_Init(&m2006->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&m2006->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&m2006->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&m2006->anglePID.outer, m2006->anglePID.outer.maxOutput*m2006->reductionRatio);
}
//�����߻ص�����
void M2006_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	M2006* m2006 = (M2006*)bindData;

	uint16_t id = *(uint16_t*)SoftBus_GetListValue(frame, 0);
	if(id != m2006->canInfo.recvID)
		return;
		
	uint8_t* data = (uint8_t*)SoftBus_GetListValue(frame, 1);
	if(data)
		M2006_Update(m2006, data);
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
//����������ģʽ�������
void M2006_CtrlerCalc(M2006* m2006, float reference)
{
	int16_t output;
	if(m2006->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m2006->speedPID, reference, m2006->speed);
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
	//����can��Ϣ
	SoftBus_Publish("/can/set-buf",{
		{"can-x", &m2006->canInfo.canX},
		{"id", &m2006->canInfo.sendID},
		{"pos", &m2006->canInfo.bufIndex},
		{"len", &(uint8_t){2}},
		{"data", &output}
	});
}
//���õ������ֵ
void M2006_SetTarget(Motor* motor, float targetValue)
{
	M2006* m2006 = (M2006*)motor;
	if(m2006->mode == MOTOR_ANGLE_MODE)
	{
		m2006->targetValue = M2006_DGR2CODE(targetValue);
	}
	else if(m2006->mode == MOTOR_SPEED_MODE)
	{
		m2006->targetValue = targetValue*m2006->reductionRatio;
	}
	else
	{
		m2006->targetValue = targetValue;
	}
}
//�л����ģʽ
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
