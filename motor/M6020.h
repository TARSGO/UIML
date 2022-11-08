#ifndef _M6020_H_
#define _M6020_H_
#include "Motor.h"
#include "PID.h"
#include "config.h"

//���ֵ������ֵ��ǶȵĻ���
#define MOTO_M6020_DGR2CODE(dgr) ((int32_t)(dgr*22.7528f)) //8191/360
#define MOTO_M6020_CODE2DGR(code) ((float)(code/22.7528f))

typedef struct _M6020
{
	Motor motor;
	
	float reductionRatio;
	uint16_t id;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//��¼��һ�εõ��ĽǶ�
	
	int32_t totalAngle;//�ۼ�ת���ı�����ֵ	
	
	PID speedPID;//�ٶ�pid(����)
	CascadePID anglePID;//�Ƕ�pid������
	
}M6020;

void M6020_Init(Motor* motor, ConfItem* dict);

#endif
