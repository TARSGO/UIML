#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "config.h"
#include "cmsis_os.h"
#include "pid.h"
#include "softbus.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)
//模式
typedef enum
{
	MOTOR_TORQUE_MODE,
	MOTOR_SPEED_MODE,
	MOTOR_ANGLE_MODE,
	MOTOR_STOP_MODE
} MotorCtrlMode;

// （可获取的）数据类型
enum MotorDataType
{
	CurrentAngle,
	TotalAngle,
	Speed
};

//父类，包含所有子类的方法
typedef struct _Motor
{
	void (*changeMode)(struct _Motor* motor, MotorCtrlMode mode);
	void (*setTarget)(struct _Motor* motor,float targetValue);
	
	void (*initTotalAngle)(struct _Motor* motor, float angle);
	float (*getData)(struct _Motor* motor,  const char* data);
	void (*stop)(struct _Motor* motor);
}Motor;

Motor* Motor_Init(ConfItem* dict);

class BasicMotor;
BasicMotor* Motor_New(ConfItem* dict);

class BasicMotor
{
public:
	BasicMotor() : m_mode(MOTOR_STOP_MODE) {};
	virtual void Init(ConfItem* conf);
	virtual bool SetMode(MotorCtrlMode mode) = 0;
	virtual bool SetTarget(float target);

	virtual bool SetTotalAngle(float angle);
	virtual float GetData(MotorDataType type);
	virtual void EmergencyStop();

protected:
	MotorCtrlMode m_mode;
};

class CanMotor : public BasicMotor
{
public:
	CanMotor() : BasicMotor() {};

protected:
	struct
	{
		uint16_t txID; // 向电机发送报文时使用的ID
		uint16_t rxID; // 电机向MCU反馈报文的ID
		uint8_t canX; // CAN设备号
		uint8_t bufIndex; // TX方向循环缓冲区偏移量
	} m_canInfo;
};

class M3508 final : public CanMotor
{
public:
	M3508() : CanMotor() {};
	virtual void Init(ConfItem* conf) override;
	virtual bool SetMode(MotorCtrlMode mode) override;
	virtual bool SetTarget(float target) override;

	virtual bool SetTotalAngle(float angle) override;
	virtual float GetData(MotorDataType type) override;
	virtual void EmergencyStop() override;

	// 各种电机编码值与角度的换算
	static inline int32_t DegToCode(float deg, float reductionRate) { return (int32_t)(deg * 22.7528f * reductionRate); }
	static inline float CodeToDeg(int32_t code, float reductionRate) { return (float)(code / (22.7528f * reductionRate)); }

private:
	// CAN接收中断回调
	static void CanRxCallback(const char* endpoint, SoftBusFrame* frame, void* bindData);
	// 急停回调
	static void EmergencyStopCallback(const char* endpoint, SoftBusFrame* frame, void* bindData);
	// 定时器回调
	static void TimerCallback(const void* argument);

	PID m_speedPid; // 单级速度PID
	CascadePID m_anglePid; // 串级（内速度外角度）PID
	
	// （CAN回调时接收的）电机参数
	int16_t m_motorAngle, m_motorSpeed, m_motorTorque; 
	uint8_t m_motorTemperature;

	float m_target; // 目标值（视模式可以为速度、角度（单位：度）或扭矩）
	float m_reductionRatio; // 减速比

	uint16_t m_lastAngle; // 上一计算帧的角度（编码器值）
	uint16_t m_totalAngle; // 电机累积转过的角度（编码器值）

	uint16_t m_stallTime; // 堵转时间
	SoftBusReceiverHandle m_stallTopic; // 堵转事件话题句柄

	// 以CAN接收数据更新成员变量
	void CanRxUpdate(uint8_t* rxData);
	// 统计角度（定时器回调调用）
	void UpdateAngle();
	// 控制器执行一次运算（PID等）
	void ControllerUpdate(float target);
	// 堵转检测逻辑
	void StallDetection();
};

#endif
