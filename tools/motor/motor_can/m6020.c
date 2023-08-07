#include "softbus.h"
#include "motor.h"
#include "pid.h"
#include "config.h"

//各种电机编码值与角度的换算
#define M6020_DGR2CODE(dgr,rdcr) ((int32_t)((dgr)*22.7528f*(rdcr))) //减速比*8191/360
#define M6020_CODE2DGR(code,rdcr) ((float)((code)/(22.7528f*(rdcr))))

typedef struct _M6020
{
	Motor motor;
	
	float reductionRatio;
	struct
	{
		uint16_t sendID,recvID;
		uint8_t canX;
		uint8_t bufIndex;
	}canInfo;
	MotorCtrlMode mode;
	
	int16_t angle,speed;
	
	int16_t lastAngle;//记录上一次得到的角度
	
	int32_t totalAngle;//累计转过的编码器值	
	
	float  targetValue;//目标值(输出轴扭矩矩/速度/角度(单位度))
	
	PID speedPID;//速度pid(单级)
	CascadePID anglePID;//角度pid，串级
}M6020;

Motor* M6020_Init(ConfItem* dict);

void M6020_InitTotalAngle(Motor *motor, float startAngle);
void M6020_SetTarget(Motor* motor, float targetValue);
void M6020_ChangeMode(Motor* motor, MotorCtrlMode mode);
float M6020_GetData(Motor* motor, const char* data);
void M6020_Stop(Motor* motor);
void M6020_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);

void M6020_Update(M6020* m6020,uint8_t* data);
void M6020_PIDInit(M6020* m6020, ConfItem* dict);
void M6020_StatAngle(M6020* m6020);
void M6020_CtrlerCalc(M6020* m6020, float reference);

//软件定时器回调函数
void M6020_TimerCallback(void const *argument)
{
	M6020* m6020 = pvTimerGetTimerID((TimerHandle_t)argument); 
	M6020_StatAngle(m6020);
	M6020_CtrlerCalc(m6020, m6020->targetValue);
}

Motor* M6020_Init(ConfItem* dict)
{
	//分配子类内存空间
	M6020* m6020 = MOTOR_MALLOC_PORT(sizeof(M6020));
	memset(m6020,0,sizeof(M6020));
	//子类多态
	m6020->motor.setTarget = M6020_SetTarget;
	m6020->motor.changeMode = M6020_ChangeMode;
	m6020->motor.initTotalAngle = M6020_InitTotalAngle;
	m6020->motor.getData = M6020_GetData;
	m6020->motor.stop = M6020_Stop;
	//电机减速比
	m6020->reductionRatio = Conf_GetValue(dict, "reduction-ratio", float, 1);//如果未配置电机减速比参数，则使用原装电机默认减速比
	//初始化电机绑定can信息
	uint16_t id = Conf_GetValue(dict, "id", uint16_t, 0);
	m6020->canInfo.recvID = id + 0x204;
	m6020->canInfo.sendID = (id <= 4) ? 0x1FF : 0x2FF;
	m6020->canInfo.bufIndex =  ((id - 1)%4) * 2;
	m6020->canInfo.canX = Conf_GetValue(dict, "can-x", uint8_t, 0);
	//设置电机默认模式为扭矩模式
	m6020->mode = MOTOR_TORQUE_MODE;
	//初始化电机pid
	M6020_PIDInit(m6020, dict);
	//订阅can信息
	char name[] = "/can_/recv";
	name[4] = m6020->canInfo.canX + '0';
	Bus_RegisterReceiver(m6020, M6020_SoftBusCallback, name);
	//开启软件定时器
	osTimerDef(M6020, M6020_TimerCallback);
	osTimerStart(osTimerCreate(osTimer(M6020), osTimerPeriodic, m6020), 2);

	return (Motor*)m6020;
}
//初始化pid
void M6020_PIDInit(M6020* m6020, ConfItem* dict)
{
	PID_Init(&m6020->speedPID, Conf_GetPtr(dict, "speed-pid", ConfItem));
	PID_Init(&m6020->anglePID.inner, Conf_GetPtr(dict, "angle-pid/inner", ConfItem));
	PID_Init(&m6020->anglePID.outer, Conf_GetPtr(dict, "angle-pid/outer", ConfItem));
	PID_SetMaxOutput(&m6020->anglePID.outer, m6020->anglePID.outer.maxOutput*m6020->reductionRatio);//将输出轴速度限幅放大到转子上
}
//软总线回调函数
void M6020_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	M6020* m6020 = (M6020*)bindData;

	uint16_t id = *(uint16_t*)Bus_GetListValue(frame, 0);
	if(id != m6020->canInfo.recvID)
		return;
		
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 1);
	if(data)
		M6020_Update(m6020, data);
}

//电机急停回调函数
void M6020_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	M6020* m6020 = (M6020*)bindData;
	m6020->targetValue = 0;
	m6020->mode = MOTOR_STOP_MODE;
}

//开始统计电机累计角度
void M6020_InitTotalAngle(Motor *motor, float startAngle)
{
	M6020* m6020 = (M6020*)motor;
	
	m6020->totalAngle=M6020_DGR2CODE(startAngle, m6020->reductionRatio);
	m6020->lastAngle=m6020->angle;
}

//统计电机累计转过的圈数
void M6020_StatAngle(M6020* m6020)
{
	int32_t dAngle=0;
	if(m6020->angle-m6020->lastAngle<-4000)
		dAngle=m6020->angle+(8191-m6020->lastAngle);
	else if(m6020->angle-m6020->lastAngle>4000)
		dAngle=-m6020->lastAngle-(8191-m6020->angle);
	else
		dAngle=m6020->angle-m6020->lastAngle;
	//将角度增量加入计数器
	m6020->totalAngle+=dAngle;
	//记录角度
	m6020->lastAngle=m6020->angle;
}
//控制器根据模式计算输出
void M6020_CtrlerCalc(M6020* m6020, float reference)
{
	int16_t output=0;
	uint8_t buffer[2]={0};
	if(m6020->mode == MOTOR_SPEED_MODE)
	{
		PID_SingleCalc(&m6020->speedPID, reference, m6020->speed);
		output = m6020->speedPID.output;
	}
	else if(m6020->mode == MOTOR_ANGLE_MODE)
	{
		PID_CascadeCalc(&m6020->anglePID, reference, m6020->totalAngle, m6020->speed);
		output = m6020->anglePID.output;
	}
	else if(m6020->mode == MOTOR_TORQUE_MODE)
	{
		output = (int16_t)reference;
	}
	buffer[0] = (output>>8)&0xff;
	buffer[1] = (output)&0xff;
	Bus_RemoteCall("/can/set-buf",{
		{"can-x", &m6020->canInfo.canX},
		{"id", &m6020->canInfo.sendID},
		{"pos", &m6020->canInfo.bufIndex},
		{"len", &(uint8_t){2}},
		{"data", buffer}
	});
}
//设置电机期望值
void M6020_SetTarget(Motor* motor, float targetValue)
{
	M6020* m6020 = (M6020*)motor;
	if(m6020->mode == MOTOR_ANGLE_MODE)
	{
		m6020->targetValue = M6020_DGR2CODE(targetValue, m6020->reductionRatio);
	}
	else if(m6020->mode == MOTOR_SPEED_MODE)
	{
		m6020->targetValue = targetValue*m6020->reductionRatio;
	}
	else 
	{
		m6020->targetValue = targetValue;
	}
}
//切换电机模式
void M6020_ChangeMode(Motor* motor, MotorCtrlMode mode)
{
	M6020* m6020 = (M6020*)motor;
	if(m6020->mode == MOTOR_STOP_MODE) //急停模式下不允许切换模式
		return;
	
	if(m6020->mode == MOTOR_SPEED_MODE)
	{
		PID_Clear(&m6020->speedPID);
	}
	else if(m6020->mode == MOTOR_ANGLE_MODE)
	{
		PID_Clear(&m6020->anglePID.inner);
		PID_Clear(&m6020->anglePID.outer);
	}
	m6020->mode = mode;
}

//获取电机数据
float M6020_GetData(Motor* motor, const char* data)
{
	M6020* m6020 = (M6020*)motor;

	if(!strcmp(data, "angle"))
	{
		return M6020_CODE2DGR(m6020->angle, 1);
	}
	else if(!strcmp(data, "totalAngle"))
	{
		return M6020_CODE2DGR(m6020->totalAngle, m6020->reductionRatio);
	}
	return 0;
}

//电机急停函数
void M6020_Stop(Motor* motor)
{
	M6020* m6020 = (M6020*)motor;
	m6020->targetValue = 0;
	m6020->mode = MOTOR_STOP_MODE;
}

//更新电机数据(可能进行滤波)
void M6020_Update(M6020* m6020,uint8_t* data)
{
	m6020->angle = (data[0]<<8 | data[1]);
	m6020->speed = (data[2]<<8 | data[3]);
}
