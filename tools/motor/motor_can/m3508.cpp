#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "byteorder.h"
#include "config.h"

void M3508::Init(ConfItem* dict)
{
	// 电机减速比；如果未配置电机减速比参数，则使用原装电机默认减速比
	m_reductionRatio = Conf_GetValue(dict, "reduction-ratio", float, 19.2f);

	// 初始化电机绑定CAN信息
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m_canInfo.rxID = id + 0x200; // 从电机收到报文的ID
	m_canInfo.txID = (id <= 4) ? 0x200 : 0x1FF; // 向电机发送报文的ID
	m_canInfo.bufIndex =  ((id - 1) % 4) * 2;
	m_canInfo.canX = Conf_GetValue(dict, "can-x", uint8_t, 0);

	// 默认模式为扭矩模式
	m_mode = MOTOR_TORQUE_MODE;

	// 初始化电机pid
	m_speedPid.Init(Conf_GetPtr(dict, "speed-pid", ConfItem));
	m_anglePid.Init(Conf_GetPtr(dict, "angle-pid", ConfItem));
	m_anglePid.outer.maxOutput *= m_reductionRatio; // 将输出轴速度限幅放大到转子上

	// 订阅can信息
	char name[] = "/can_/recv";
	name[4] = m_canInfo.canX + '0';
	Bus_RegisterReceiver(this, M3508::CanRxCallback, name);

	// 读取电机名称，创建堵转话题
	constexpr const char* stallFormat = "/%s/stall"; // 堵转话题名称格式字符串

	const char* motorName = Conf_GetValue(dict, "name", const char*, nullptr);
	motorName = motorName ? motorName : "m3508";
	char* stallName = (char*)MOTOR_MALLOC_PORT(strlen(stallFormat) - 2 + strlen(motorName));
	sprintf(stallName, stallFormat, motorName); // 格式化字符串
	m_stallTopic = Bus_CreateReceiverHandle(stallName); // 创建话题句柄
	MOTOR_FREE_PORT(stallName); // 释放内存

	m_stallTime = 0;

	// 开启软件定时器
	osTimerDef(M3508, M3508::TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M3508), osTimerPeriodic, this), 2);
}

// 切换电机模式
bool M3508::SetMode(MotorCtrlMode mode)
{
	if (m_mode == MOTOR_STOP_MODE) // 急停模式下不允许切换模式
		return false;

	m_mode = mode;
	switch (mode)
	{
	case MOTOR_SPEED_MODE:
		m_speedPid.Clear();
		break;
	case MOTOR_ANGLE_MODE:
		m_anglePid.Clear();
		break;
	default:
		break;
	}

	return true;
}

// 设置电机期望值
bool M3508::SetTarget(float target)
{
	switch (m_mode)
	{
	case MOTOR_SPEED_MODE:
		m_target = target * m_reductionRatio;
		break;
	case MOTOR_ANGLE_MODE:
		m_target = DegToCode(target, m_reductionRatio);
		break;
	case MOTOR_TORQUE_MODE:
		m_target = target;
		break;
	default:
		return false;
	}

	return true;
}

// 开始统计电机累计角度
bool M3508::SetTotalAngle(float angle)
{
	m_totalAngle = DegToCode(angle, m_reductionRatio);
	m_lastAngle = m_motorAngle;

	return true;
}

// 获取电机数据
float M3508::GetData(MotorDataType type)
{
	switch (type)
	{
        case CurrentAngle:
			// 转子当前角度
			return CodeToDeg(m_motorAngle, 1);

        case TotalAngle:
			// 输出轴总转过角度
			return CodeToDeg(m_totalAngle, m_reductionRatio);

        case Speed:
			// 输出轴转速（RPM）
			return m_motorSpeed;
		
		default:
			return 0;
	}
}

// 急停函数
void M3508::EmergencyStop()
{
	m_target = 0;
	m_mode = MOTOR_STOP_MODE;
}

void M3508::CanRxCallback(const char* endpoint, SoftBusFrame* frame, void* bindData)
{
	// 校验绑定电机对象
	if (!bindData) return;

	auto self = reinterpret_cast<M3508*>(bindData);

	// 校验电机ID
	if (self->m_canInfo.rxID != Bus_GetListValue(frame, 0).U16)
		return;

	auto data = reinterpret_cast<uint8_t*>(Bus_GetListValue(frame, 1).Ptr);
	if (data)
		self->CanRxUpdate(data); // 更新电机数据
}

void M3508::TimerCallback(const void* argument)
{
	auto self = reinterpret_cast<M3508*>(pvTimerGetTimerID((TimerHandle_t)argument));

	// 计算前一帧至今转过的角度
	self->UpdateAngle();
	// 调用控制器计算电机下一步行动
	self->ControllerUpdate(self->m_target);

	// 判断堵转情况
}

// 电机急停回调函数
void M3508::EmergencyStopCallback(const char* endpoint, SoftBusFrame* frame, void* bindData)
{
	M3508* m3508 = reinterpret_cast<M3508*>(bindData);
	m3508->m_target = 0;
	m3508->m_mode = MOTOR_STOP_MODE;
}

void M3508::CanRxUpdate(uint8_t* rxData)
{
	struct {
		uint16_t Angle;
		uint16_t Speed;
		uint16_t Current;
		uint8_t Temperature;
		uint8_t Reserved;
	} __attribute((__packed__)) *C620Feedback;
	C620Feedback = reinterpret_cast<decltype(C620Feedback)>(rxData);

	// 将反馈报文数据读入类成员
	m_motorAngle = SwapByteorder16Bit(C620Feedback->Angle);
	m_motorSpeed = SwapByteorder16Bit(C620Feedback->Speed);
	m_motorTorque = SwapByteorder16Bit(C620Feedback->Current);
	m_motorTemperature = C620Feedback->Temperature;
}

// 统计电机累计转过的圈数
void M3508::UpdateAngle()
{
	int32_t angleDiff = 0;
	// FIXME: 无注释，逻辑存疑
	if (m_motorAngle - m_lastAngle < -4000) 
		angleDiff = m_motorAngle + (8191 - m_lastAngle);
	else if (m_motorAngle - m_lastAngle > 4000)
		angleDiff = -m_lastAngle - (8191 - m_motorAngle);
	else
		angleDiff = m_motorAngle - m_lastAngle;
	// 将角度增量加入计数器
	m_totalAngle += angleDiff;
	// 记录角度
	m_lastAngle = m_motorAngle;
}

// 控制器根据模式计算输出
void M3508::ControllerUpdate(float target)
{
	int16_t output = 0;
	switch (m_mode)
	{
	case MOTOR_SPEED_MODE:
		output = m_speedPid.SingleCalc(target, m_motorSpeed);
		break;
	case MOTOR_ANGLE_MODE:
		output = m_anglePid.CascadeCalc(target, m_totalAngle, m_motorSpeed);
		break;
	case MOTOR_TORQUE_MODE:
		output = (int16_t) target;
		break;
	case MOTOR_STOP_MODE:
		return;
	}
	
	// 交换成大端
	output = SwapByteorder16Bit(output);

	Bus_RemoteCall("/can/set-buf", {
					{"can-x", {.U8 = m_canInfo.canX}},
		 			{"id", {.U16 = m_canInfo.txID}},
		 			{"pos", {.U8 = m_canInfo.bufIndex}},
		 			{"len", {.U8 = 2}},
		 			{"data", {&output}}
	});
}

// 堵转检测逻辑
void M3508::StallDetection()
{
	m_stallTime = (m_motorSpeed == 0 && abs(m_motorTorque) > 7000) ? m_stallTime + 2 : 0;
	if (m_stallTime > 500)
	{
		Bus_FastBroadcastSend(m_stallTopic, {{nullptr}});
		m_stallTime = 0;
	}
}
