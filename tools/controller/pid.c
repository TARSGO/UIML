/****************PID����****************/

#include "pid.h"

//��ʼ��pid����
void PID_Init(PID *pid, ConfItem* conf)
{
	pid->kp=Conf_GetValue(conf, "p", float, 0);
	pid->ki=Conf_GetValue(conf, "i", float, 0);
	pid->kd=Conf_GetValue(conf, "d", float, 0);
	pid->maxIntegral=Conf_GetValue(conf, "max-i", float, 0);
	pid->maxOutput=Conf_GetValue(conf, "max-out", float, 0);
	pid->deadzone=0;
}

//����pid����
void PID_SingleCalc(PID *pid,float reference,float feedback)
{
	//��������
	pid->lastError=pid->error;
	if(ABS(reference-feedback) < pid->deadzone)//���������������errorֱ����0
		pid->error=0;
	else
		pid->error=reference-feedback;
	//����΢��
	pid->output=(pid->error-pid->lastError)*pid->kd;
	//�������
	pid->output+=pid->error*pid->kp;
	//�������
	pid->integral+=pid->error*pid->ki;
	LIMIT(pid->integral,-pid->maxIntegral,pid->maxIntegral);//�����޷�
	pid->output+=pid->integral;
	//����޷�
	LIMIT(pid->output,-pid->maxOutput,pid->maxOutput);
}

//����pid����
void PID_CascadeCalc(CascadePID *pid,float angleRef,float angleFdb,float speedFdb)
{
	PID_SingleCalc(&pid->outer,angleRef,angleFdb);//�����⻷(�ǶȻ�)
	PID_SingleCalc(&pid->inner,pid->outer.output,speedFdb);//�����ڻ�(�ٶȻ�)
	pid->output=pid->inner.output;
}

//���һ��pid����ʷ����
void PID_Clear(PID *pid)
{
	pid->error=0;
	pid->lastError=0;
	pid->integral=0;
	pid->output=0;
}

//�����趨pid����޷�
void PID_SetMaxOutput(PID *pid,float maxOut)
{
	pid->maxOutput=maxOut;
}

//����PID����
void PID_SetDeadzone(PID *pid,float deadzone)
{
	pid->deadzone=deadzone;
}
