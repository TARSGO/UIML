#ifndef _M6020_H_
#define _M6020_H_
#include "Motor.h"
#include "PID.h"
#include "config.h"

//���ֵ������ֵ��ǶȵĻ���
#define M6020_DGR2CODE(dgr) ((int32_t)(dgr*22.7528f)) //8191/360
#define M6020_CODE2DGR(code) ((float)(code/22.7528f))

typedef struct _M6020
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t id[2];
		char* canX[2];
		uint8_t sendBits;
	}canInfo;
	MotorCtrlMode mode;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//��¼��һ�εõ��ĽǶ�
	
	int32_t totalAngle;//�ۼ�ת���ı�����ֵ	
	
	PID speedPID;//�ٶ�pid(����)
	CascadePID anglePID;//�Ƕ�pid������
	
}M6020;

#endif
