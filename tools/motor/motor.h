#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "config.h"
#include "cmsis_os.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)
//ģʽ
typedef enum
{
	MOTOR_TORQUE_MODE,
	MOTOR_SPEED_MODE,
	MOTOR_ANGLE_MODE
}MotorCtrlMode;
//���࣬������������ķ���
typedef struct _Motor
{
	void (*changeMode)(struct _Motor* motor, MotorCtrlMode mode);
	void (*setTarget)(struct _Motor* motor,float targetValue);
	
	void (*setStartAngle)(struct _Motor* motor, float angle);
}Motor;

Motor* Motor_Init(ConfItem* dict);

#endif
