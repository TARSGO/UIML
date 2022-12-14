#include "softbus.h"
#include "motor.h"
#include "config.h"
#include "pid.h"
#include <stdio.h>

//���ֵ������ֵ��ǶȵĻ���
#define DCMOTOR_DGR2CODE(dgr,rdcr,encode) ((int32_t)((dgr)*encode/360.0f*(rdcr))) //���ٱ�*������һȦֵ/360
#define DCMOTOR_CODE2DGR(code,rdcr,encode) ((float)((code)/(encode/360.0f*(rdcr))))

typedef struct
{
	uint8_t timX;
	uint8_t	channelX;
}TimInfo;

typedef struct _DCmotor
{
	Motor motor;
	TimInfo encodeTim,posRotateTim,negRotateTim;
	float reductionRatio;
	float circleEncode;

	MotorCtrlMode mode;
	
	uint32_t angle,lastAngle;//��¼��һ�εõ��ĽǶ�
	
	int16_t speed;
	
	int32_t totalAngle;//�ۼ�ת���ı�����ֵ
	
	float  targetValue;//Ŀ��ֵ(�����/�ٶ�/�Ƕ�(��λ��))
	
	PID speedPID;//�ٶ�pid(����)
	CascadePID anglePID;//�Ƕ�pid������
	
}DcMotor;

void DcMotor_TimInit(DcMotor* dcMotor, ConfItem* dict);
void DcMotor_PIDInit(DcMotor* dcMotor, ConfItem* dict);
void DcMotor_SetStartAngle(Motor *motor, float startAngle);
void DcMotor_SetTarget(Motor* motor, float targetValue);
void DcMotor_ChangeMode(Motor* motor, MotorCtrlMode mode);

void DcMotor_StatAngle(DcMotor* dcMotor);
void DcMotor_CtrlerCalc(DcMotor* dcMotor, float reference);

//�����ʱ���ص�����
void DcMotor_TimerCallback(void const *argument)
{
	DcMotor* dcMotor = pvTimerGetTimerID((TimerHandle_t)argument); 
	DcMotor_StatAngle(dcMotor);
	DcMotor_CtrlerCalc(dcMotor, dcMotor->targetValue);
}

Motor* DcMotor_Init(ConfItem* dict)
{
	//���������ڴ�ռ�
	DcMotor* dcMotor = MOTOR_MALLOC_PORT(sizeof(DcMotor));
	memset(dcMotor,0,sizeof(DcMotor));
	//�����̬
	dcMotor->motor.setTarget = DcMotor_SetTarget;
	dcMotor->motor.changeMode = DcMotor_ChangeMode;
	dcMotor->motor.setStartAngle = DcMotor_SetStartAngle;
	//������ٱ�
	dcMotor->reductionRatio = Conf_GetValue(dict, "reductionRatio", float, 19.0f);//���δ���õ�����ٱȲ�������ʹ��ԭװ���Ĭ�ϼ��ٱ�
	dcMotor->circleEncode = Conf_GetValue(dict, "maxEncode", float, 48.0f); //��Ƶ�������תһȦ�����ֵ
	//��ʼ�������tim��Ϣ
	DcMotor_TimInit(dcMotor, dict);
	//���õ��Ĭ��ģʽΪ�ٶ�ģʽ
	dcMotor->mode = MOTOR_TORQUE_MODE;
	//��ʼ�����pid
	DcMotor_PIDInit(dcMotor, dict);
	
	//���������ʱ��
	osTimerDef(DCMOTOR, DcMotor_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(DCMOTOR), osTimerPeriodic,dcMotor), 2);

	return (Motor*)dcMotor;
}

//��ʼ��pid
void DcMotor_PIDInit(DcMotor* dcMotor, ConfItem* dict)
{
	PID_Init(&dcMotor->speedPID, Conf_GetPtr(dict, "speedPID", ConfItem));
	PID_Init(&dcMotor->anglePID.inner, Conf_GetPtr(dict, "anglePID/inner", ConfItem));
	PID_Init(&dcMotor->anglePID.outer, Conf_GetPtr(dict, "anglePID/outer", ConfItem));
	PID_SetMaxOutput(&dcMotor->anglePID.outer, dcMotor->anglePID.outer.maxOutput*dcMotor->reductionRatio);//��������ٶ��޷��Ŵ�ת����
}

//��ʼ��tim
void DcMotor_TimInit(DcMotor* dcMotor, ConfItem* dict)
{
	dcMotor->posRotateTim.timX = Conf_GetValue(dict,"posRotateTim/tim-x",uint8_t,0);
	dcMotor->posRotateTim.channelX = Conf_GetValue(dict,"posRotateTim/channel-x",uint8_t,0);
	dcMotor->negRotateTim.timX = Conf_GetValue(dict,"negRotateTim/tim-x",uint8_t,0);
	dcMotor->negRotateTim.channelX = Conf_GetValue(dict,"negRotateTim/channel-x",uint8_t,0);
	dcMotor->encodeTim.channelX = Conf_GetValue(dict,"encodeTim/tim-x",uint8_t,0);
}

//��ʼͳ�Ƶ���ۼƽǶ�
void DcMotor_SetStartAngle(Motor *motor, float startAngle)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	
	dcMotor->totalAngle=DCMOTOR_DGR2CODE(startAngle, dcMotor->reductionRatio,dcMotor->circleEncode);
	dcMotor->lastAngle=dcMotor->angle;
}

//ͳ�Ƶ���ۼ�ת����Ȧ��
void DcMotor_StatAngle(DcMotor* dcMotor)
{
	int32_t dAngle=0;
	uint32_t autoReload=0;
	
	Bus_RemoteCall("/tim/encode",{{"tim-x",&dcMotor->encodeTim.timX},{"count",&dcMotor->angle},{"auto-reload",&autoReload}});
	
	if(dcMotor->angle - dcMotor->lastAngle < -(autoReload/2.0f))
		dAngle = dcMotor->angle+(autoReload-dcMotor->lastAngle);
	else if(dcMotor->angle - dcMotor->lastAngle > (autoReload/2.0f))
		dAngle = -dcMotor->lastAngle - (autoReload - dcMotor->angle);
	else
		dAngle = dcMotor->angle - dcMotor->lastAngle;
	//���Ƕ��������������
	dcMotor->totalAngle += dAngle;
	//�����ٶ�
	dcMotor->speed = (float)dAngle/(dcMotor->circleEncode*dcMotor->reductionRatio)*500*60;//rpm  
	//��¼�Ƕ�
	dcMotor->lastAngle=dcMotor->angle;
}

//����������ģʽ�������
void DcMotor_CtrlerCalc(DcMotor* dcMotor, float reference)
{
	float output=0;
	if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&dcMotor->speedPID, reference, dcMotor->speed);
		output = dcMotor->speedPID.output;
	}
	else if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&dcMotor->anglePID, reference, dcMotor->totalAngle, dcMotor->speed);
		output = dcMotor->anglePID.output;
	}
	else if(dcMotor->mode == MOTOR_TORQUE_MODE)
	{
		output = reference;
	}
  
	if(output>0)
	{
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->posRotateTim.channelX},{"duty",&output}});
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->negRotateTim.channelX},{"duty",IM_PTR(float, 0)}});
	}
	else
	{
		output = ABS(output);
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->posRotateTim.channelX},{"duty",IM_PTR(float, 0)});
		Bus_BroadcastSend("/tim/pwm/set-duty",{{"tim-x",&dcMotor->posRotateTim.timX},{"channel-x",&dcMotor->negRotateTim.channelX},{"duty",&output}});
	}

}

//���õ������ֵ
void DcMotor_SetTarget(Motor* motor, float targetValue)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		dcMotor->targetValue = DCMOTOR_DGR2CODE(targetValue, dcMotor->reductionRatio,dcMotor->circleEncode);
	}
	else if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		dcMotor->targetValue = targetValue*dcMotor->reductionRatio;
	}
	else
	{
		dcMotor->targetValue = targetValue;
	}
}

//�л����ģʽ
void DcMotor_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	DcMotor* dcMotor = (DcMotor*)motor;
	if(dcMotor->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&dcMotor->speedPID);
	}
	else if(dcMotor->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&dcMotor->anglePID.inner);
		PID_Clear(&dcMotor->anglePID.outer);
	}
	dcMotor->mode = mode;
}





