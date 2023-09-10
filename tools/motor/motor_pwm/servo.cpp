#include "softbus.h"
#include "motor.h"
#include "config.h"

void Servo::Init(ConfItem* dict)
{
	//初始化电机绑定TIM信息
	m_timInfo.timX = Conf_GetValue(dict,"tim-x",uint8_t,0);
	m_timInfo.channelX = Conf_GetValue(dict,"channel-x",uint8_t,0);
	m_maxAngle = Conf_GetValue(dict,"max-angle",float,180);
	m_maxDuty = Conf_GetValue(dict,"max-duty",float,0.125f);
	m_minDuty = Conf_GetValue(dict,"min-duty",float,0.025f);
}

//设置舵机角度
bool Servo::SetTarget(float targetValue)
{
	m_target = targetValue; //无实际用途，可用于debug
	float pwmDuty = m_target / m_maxAngle * (m_maxDuty - m_minDuty) + m_minDuty;
	Bus_RemoteCall("/tim/pwm/set-duty", {{"tim-x", {.U8 = m_timInfo.timX}},
										 {"channel-x", {.U8 = m_timInfo.channelX}},
										 {"duty", {.F32 = pwmDuty}}});
}


