#ifndef _M3508_H_
#define _M3508_H_
#include "Motor.h"
#include "PID.h"
#include "config.h"

//���ֵ������ֵ��ǶȵĻ���
#define M3508_DGR2CODE(dgr) ((int32_t)(dgr*436.9263f)) //3591/187*8191/360
#define M3508_CODE2DGR(code) ((float)(code/436.9263f))

typedef struct _M3508
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t id[2];
		char* canX[2];
		uint8_t sendBits;
	}canInfo;
	Ctrler ctrler;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//��¼��һ�εõ��ĽǶ�
	
	int32_t totalAngle;//�ۼ�ת���ı�����ֵ
	
	PID speedPID;//�ٶ�pid(����)
	CascadePID anglePID;//�Ƕ�pid������
	
}M3508;

void M3508_Init(Motor* motor, ConfItem* dict);

#endif
