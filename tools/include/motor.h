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

class BasicMotor
{
public:
	BasicMotor() : m_mode(MOTOR_STOP_MODE) {};
	virtual void Init(ConfItem* conf) = 0;
	virtual bool SetMode(MotorCtrlMode mode) = 0;
	virtual void EmergencyStop() = 0;

	virtual bool SetTarget(float target) = 0;
	virtual bool SetTotalAngle(float angle) = 0;
	virtual float GetData(MotorDataType type) = 0;

	static BasicMotor* Create(ConfItem* conf);

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

class DummyMotor : public BasicMotor
{
public:
	DummyMotor() : BasicMotor() {};
	virtual void Init(ConfItem* conf) override {};
	virtual bool SetMode(MotorCtrlMode mode) override { return false; }
	virtual void EmergencyStop() override {};

	virtual bool SetTarget(float target) override { return false; }
	virtual bool SetTotalAngle(float angle) override { return false; }
	virtual float GetData(MotorDataType type) override { return 0.0f; }
};

class DjiCanMotor : public CanMotor
{
public:
	DjiCanMotor() : CanMotor() {};
	virtual void Init(ConfItem* conf) override;
	virtual bool SetMode(MotorCtrlMode mode) override;
	virtual void EmergencyStop() override;

	virtual bool SetTarget(float target) override;
	virtual bool SetTotalAngle(float angle) override;
	virtual float GetData(MotorDataType type) override;
	
	// 各种电机编码值与角度的换算
	// 减速比*8191/360，适用于当前体系下所有大疆的RM电机
	static inline int32_t DegToCode(float deg, float reductionRate) { return (int32_t)(deg * 22.7528f * reductionRate); }
	static inline float CodeToDeg(int32_t code, float reductionRate) { return (float)(code / (22.7528f * reductionRate)); }

protected:
	// CAN接收中断回调
	static void CanRxCallback(const char* endpoint, SoftBusFrame* frame, void* bindData);
	// 急停回调
	static void EmergencyStopCallback(const char* endpoint, SoftBusFrame* frame, void* bindData);
	// 定时器回调
	static void TimerCallback(const void* argument);

	// CAN发送函数
	virtual void CanTransmit(uint16_t output) = 0;

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
	virtual void CanRxUpdate(uint8_t* rxData) = 0;
	// 统计角度（定时器回调调用）
	void UpdateAngle();
	// 控制器执行一次运算（PID等）
	void ControllerUpdate(float target);
	// 堵转检测逻辑
	void StallDetection();
};

class M3508 final : public DjiCanMotor
{
public:
	M3508() : DjiCanMotor() {};
	virtual void Init(ConfItem* conf) override;

private:
	virtual void CanTransmit(uint16_t output) override;
	virtual void CanRxUpdate(uint8_t* rxData) override;
};

class M6020 final : public DjiCanMotor
{
public:
	M6020() : DjiCanMotor() {};
	virtual void Init(ConfItem* conf) override;
	// 6020没有电流反馈，拒绝使用扭矩模式
	virtual bool SetMode(MotorCtrlMode mode) override;

private:
	virtual void CanTransmit(uint16_t output) override;
	virtual void CanRxUpdate(uint8_t* rxData) override;
};

class M2006 final : public DjiCanMotor
{
public:
	M2006() : DjiCanMotor() {};
	virtual void Init(ConfItem* conf) override;

private:
	virtual void CanTransmit(uint16_t output) override;
	virtual void CanRxUpdate(uint8_t* rxData) override;
};

class DcMotor : public BasicMotor
{
public:
	DcMotor() : BasicMotor() {};
	virtual void Init(ConfItem* conf);
	virtual bool SetMode(MotorCtrlMode mode);
	virtual void EmergencyStop();

	virtual bool SetTarget(float target);
	virtual bool SetTotalAngle(float angle);
	virtual float GetData(MotorDataType type);

	// 编码值与角度的换算（减速比*编码器一圈值/360）
	static inline uint32_t Degree2Code(float deg, float reductionRatio, float circleEncode) { return deg * reductionRatio * circleEncode / 360.0f; }
	static inline float Code2Degree(uint32_t code, float reductionRatio, float circleEncode) { return code / (reductionRatio * circleEncode / 360.0f); }

protected:
	PID m_speedPID; // 单级速度PID
	CascadePID m_anglePID; // 串级（内速度外角度）PID

	float m_reductionRatio; // 减速比
	float m_circleEncode; // 倍频后编码器转一圈的最大值
	float m_target; // 目标值（视模式可以为速度、角度（单位：度)）

	uint16_t m_lastAngle; // 上一计算帧的角度（编码器值）
	uint16_t m_totalAngle; // 电机累积转过的角度（编码器值）

	uint16_t m_angle, m_speed;

	struct TimInfo
	{
		uint8_t timX;
		uint8_t	channelX;
	} m_posRotateTim, m_negRotateTim, m_encodeTim;

	// 定时器回调
	static void TimerCallback(const void* argument);

	// 统计角度（定时器回调调用）
	void UpdateAngle();

	// 控制器执行一次运算（PID等）
	void ControllerUpdate(float target);

private:
	void TimerInit(ConfItem* dict); // 定时器初始化
};

class Servo : public BasicMotor
{
public:
	Servo() : BasicMotor() {};
	virtual void Init(ConfItem* conf);
	virtual bool SetMode(MotorCtrlMode mode) { return false; }
	virtual void EmergencyStop() { }

	virtual bool SetTarget(float target);
	virtual bool SetTotalAngle(float angle) { return false; }
	virtual float GetData(MotorDataType type) { return 0.0f; }

protected:
	struct TimInfo
	{
		uint8_t timX;
		uint8_t	channelX;
	} m_timInfo;
	float m_maxAngle;
	float m_maxDuty, m_minDuty;
	float m_target;
};

#endif
