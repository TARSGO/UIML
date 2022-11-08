#include "chassisMcNamm.h"
#include "slope.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "arm_math.h"

#define CHASSIS_ACC2SLOPE(acc) (chassis.taskInterval*(acc)/1000)

typedef enum{
	ChassisMode_Follow, //���̸�����̨ģʽ
	ChassisMode_Alone, //����ģʽ�����̲���ת
	ChassisMode_Spin, //С����ģʽ
	ChassisMode_45 //45��ģʽ����������̨��45�ȼн�
}ChassisMode;

typedef struct _Chassis
{
	//���̳ߴ���Ϣ
	struct Info
	{
		float wheelbase;//���
		float wheeltrack;//�־�
		float wheelRadius;//�ְ뾶
		float offsetX;//������xy���ϵ�ƫ��
		float offsetY;
	}info;
//	//4�����
//	struct Motor
//	{
//		int16_t targetSpeed;//Ŀ���ٶ�
//		int32_t targetAngle;//Ŀ��Ƕ�
//	}motors[4];
	//�����ƶ���Ϣ
	struct Move
	{
		float vx;//��ǰ����ƽ���ٶ� mm/s
		float vy;//��ǰǰ���ƶ��ٶ� mm/s
		float vw;//��ǰ��ת�ٶ� rad/s
		
		float maxVx,maxVy,maxVw; //������������ٶ�
		Slope xSlope,ySlope; //б��
	}move;
	
	float relativeAngle; //��̨����̵�ƫ��ǣ���λ��
	
	uint8_t taskInterval;
	
}Chassis;

Chassis chassis={0};

void Chassis_Init(ConfItem* dict)
{
	//������
	chassis.taskInterval = Conf_GetValue(dict, chassis/taskInterval, uint8_t, 2);
	//���̳ߴ���Ϣ�����ڽ������٣�
	chassis.info.wheelbase = Conf_GetValue(dict, chassis/info/wheelbase, float, 0);
	chassis.info.wheeltrack = Conf_GetValue(dict, chassis/info/wheeltrack, float, 0);
	chassis.info.wheelRadius = Conf_GetValue(dict, chassis/info/wheelRadius, float, 76);
	chassis.info.offsetX = Conf_GetValue(dict, chassis/info/offsetX, float, 0);
	chassis.info.offsetY = Conf_GetValue(dict, chassis/info/offsetY, float, 0);
	//�ƶ�������ʼ��
	chassis.move.maxVx = Conf_GetValue(dict, chassis/move/offsetX, float, 2000);
	chassis.move.maxVy = Conf_GetValue(dict, chassis/move/offsetX, float, 2000);
	chassis.move.maxVw = Conf_GetValue(dict, chassis/move/offsetX, float, 2);
	float xAcc = Conf_GetValue(dict, chassis/move/xAcc, float, 1000);
	float yAcc = Conf_GetValue(dict, chassis/move/yAcc, float, 1000);
	Slope_Init(&chassis.move.xSlope,CHASSIS_ACC2SLOPE(xAcc),0);
	Slope_Init(&chassis.move.ySlope,CHASSIS_ACC2SLOPE(yAcc),0);
//	//���pid������ʼ��
//	Chassis_InitPID();
//	//��ʼ����ת��Ϣ
//	Chassis_InitRotate();
}

////����pid������ʼ��
//void Chassis_InitPID()
//{
//	PID_Init(&chassis.rotate.pid,0.05,0,0,0,1);
//}

////��ʼ����ת����
//void Chassis_InitRotate()
//{
//	chassis.rotate.angle=IMU_GetYaw();
//	chassis.rotate.lastAngle=chassis.rotate.angle;
//	chassis.rotate.zeroAdjust=chassis.rotate.angle;
//	chassis.rotate.totalAngle=0;
//	chassis.rotate.lastTotalAngle=0;
//	chassis.rotate.totalRound=0;
//	chassis.rotate.normalOffsetAngle=0;
//	chassis.rotate.relativeAngle=0;
//}


//����б�¼����ٶ�
void Chassis_UpdateSlope()
{
	Slope_NextVal(&chassis.move.xSlope);
	Slope_NextVal(&chassis.move.ySlope);
}

//��������ص�����
void Chassis_TaskCallback(void const * argument)
{
	Chassis_Init((ConfItem*)argument);
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{		
		/*************�������ƽ���ٶ�**************/
		
		Chassis_UpdateSlope();//�����˶�б�º�������

		//����̨����ϵ��ƽ���ٶȽ��㵽����ƽ���ٶ�(������̨ƫ���)
		float gimbalAngleSin=arm_sin_f32(chassis.relativeAngle*PI/180);
		float gimbalAngleCos=arm_cos_f32(chassis.relativeAngle*PI/180);
		chassis.move.vx=Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
		chassis.move.vy=-Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin
									 +Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
		
		/*************���������ת��**************/
		
		float rotateRatio[4];
		rotateRatio[0]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[1]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f-chassis.info.offsetY-chassis.info.offsetX;
		rotateRatio[2]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY+chassis.info.offsetX;
		rotateRatio[3]=(chassis.info.wheelbase+chassis.info.wheeltrack)/2.0f+chassis.info.offsetY-chassis.info.offsetX;
		float wheelRPM[4];
		wheelRPM[0]=(chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[0])*60/(2*PI*chassis.info.wheelRadius)*19;//FL
		wheelRPM[1]=-(-chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[1])*60/(2*PI*chassis.info.wheelRadius)*19;//FR
		wheelRPM[2]=(-chassis.move.vx+chassis.move.vy-chassis.move.vw*rotateRatio[2])*60/(2*PI*chassis.info.wheelRadius)*19;//BL
		wheelRPM[3]=-(chassis.move.vx+chassis.move.vy+chassis.move.vw*rotateRatio[3])*60/(2*PI*chassis.info.wheelRadius)*19;//BR
		
//		SoftBus_PublishMap("Motor",{{"chassis/motorFL",&wheelRPM[0],sizeof(float)},
//																{"chassis/motorFR",&wheelRPM[1],sizeof(float)},
//																{"chassis/motorBL",&wheelRPM[2],sizeof(float)},
//																{"chassis/motorBR",&wheelRPM[3],sizeof(float)}});
//		
		osDelayUntil(&tick,chassis.taskInterval);
	}
}
