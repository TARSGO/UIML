#include "gyroscope.h"
#include "cmsis_os.h"
#include "softbus.h"
#include "config.h"
#include "AHRS.h"
#include "pid.h"
#include "filter.h"
#include <stdio.h>

typedef struct 
{
	struct 
	{
		float quat[4];
		float accel[3];
		float gyro[3];
		float mag[3];
		float gyroOffset[3];
		float tmp;
	}imu;
	uint8_t spiX;
	float yaw,pitch,roll;
	float targetTmp;
	uint8_t timX;
	uint8_t channelX;

	Bmi088 gyro; // TODO: 工厂创建
	Filter *filter;

	uint16_t taskInterval; //任务执行间隔

	char* eulerAngleName;
}INS;

void INS_Init(INS* ins, ConfItem* dict);
void INS_TmpPIDTimerCallback(void const *argument);

void INS_TaskCallback(void const * argument)
{ 

	/* USER CODE BEGIN IMU */ 
	INS ins = {0};
	osDelay(50);
	INS_Init(&ins, (ConfItem*)argument);
	AHRS_init(ins.imu.quat,ins.imu.accel,ins.imu.mag);
	//校准零偏
	// for(int i=0;i<10000;i++)
	// {
	// 	BMI088_ReadData(ins.spiX, ins.imu.gyro,ins.imu.accel, &ins.imu.tmp);
	// 	ins.imu.gyroOffset[0] +=ins.imu.gyro[0];
	// 	ins.imu.gyroOffset[1] +=ins.imu.gyro[1];
	// 	ins.imu.gyroOffset[2] +=ins.imu.gyro[2];
	// 	HAL_Delay(1);
	// }
	// ins.imu.gyroOffset[0] = ins.imu.gyroOffset[0]/10000.0f;
	// ins.imu.gyroOffset[1] = ins.imu.gyroOffset[1]/10000.0f;
	// ins.imu.gyroOffset[2] = ins.imu.gyroOffset[2]/10000.0f;

	ins.imu.gyroOffset[0] = -0.000767869;   //10次校准取均值
	ins.imu.gyroOffset[1] = 0.000771033;  
	ins.imu.gyroOffset[2] = 0.001439746;
	
  /* Infinite loop */
	while(1)
	{
		ins.gyro.Acquire(); // 采样
		ins.gyro.GetGyroscopeData(ins.imu.gyro[0], ins.imu.gyro[1], ins.imu.gyro[2]);
		ins.gyro.GetAccelerometerData(ins.imu.accel[0], ins.imu.accel[1], ins.imu.accel[2]);
		ins.imu.tmp = ins.gyro.GetTemperature();

		for(uint8_t i=0;i<3;i++)
			ins.imu.gyro[i] -= ins.imu.gyroOffset[i];

		//滤波
		// for(uint8_t i=0;i<3;i++)
		// 	ins.imu.accel[i] = ins.filter->cala(ins.filter , ins.imu.accel[i]);
		//数据融合	
		AHRS_update(ins.imu.quat,ins.taskInterval/1000.0f,ins.imu.gyro,ins.imu.accel,ins.imu.mag);
		get_angle(ins.imu.quat,&ins.yaw,&ins.pitch,&ins.roll);
		ins.yaw = ins.yaw/PI*180;
		ins.pitch = ins.pitch/PI*180;
		ins.roll = ins.roll/PI*180;
		//发布数据
		Bus_BroadcastSend(ins.eulerAngleName, {{"yaw", {.F32 = ins.yaw}}, {"pitch", {.F32 = ins.pitch}}, {"roll", {.F32 = ins.roll}}});
		osDelay(ins.taskInterval);
	}
  /* USER CODE END IMU */
}

void INS_Init(INS* ins, ConfItem* dict)
{
	ins->taskInterval = Conf_GetValue(dict,"task-interval",uint16_t,10);

	// ins->filter = Filter_Init(Conf_GetPtr(dict, "filter", ConfItem));
	//ins->tmpPID.Init(Conf_GetPtr(dict, "tmp-pid", ConfItem));

	auto temp = Conf_GetValue(dict, "name", const char *, nullptr);
	temp = temp ? temp : "ins";
	uint8_t len = strlen(temp);
	ins->eulerAngleName = (char*)pvPortMalloc(len + 13 + 1); //13为"/   /euler-angle"的长度，1为'\0'的长度
	sprintf(ins->eulerAngleName, "/%s/euler-angle", temp);

	ins->gyro.Init(dict);
	ins->gyro.Acquire(); // 采样
	ins->gyro.GetGyroscopeData(ins->imu.gyro[0], ins->imu.gyro[1], ins->imu.gyro[2]);
	ins->gyro.GetAccelerometerData(ins->imu.accel[0], ins->imu.accel[1], ins->imu.accel[2]);
	ins->imu.tmp = ins->gyro.GetTemperature();

	//创建定时器进行温度pid控制
	osTimerDef(tmp, INS_TmpPIDTimerCallback);
	osTimerStart(osTimerCreate(osTimer(tmp), osTimerPeriodic, ins), 2);
}

//软件定时器回调函数
void INS_TmpPIDTimerCallback(void const *argument)
{
	INS* ins = (INS*)pvTimerGetTimerID((TimerHandle_t)argument);
	ins->gyro.TemperatureControlTick();
}
