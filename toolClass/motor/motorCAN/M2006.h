#ifndef _M2006_H_
#define _M2006_H_
#include "Motor.h"
#include "PID.h"
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
	
}M2006;

#endif
