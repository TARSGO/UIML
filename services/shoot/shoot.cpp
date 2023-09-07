#include "config.h"
#include "softbus.h"
#include "motor.h"
#include "cmsis_os.h"

typedef enum
{
	SHOOTER_MODE_IDLE,
	SHOOTER_MODE_ONCE,
	SHOOTER_MODE_CONTINUE,
	SHOOTER_MODE_BLOCK
}ShooterMode;

typedef struct
{
	BasicMotor *fricMotors[2],*triggerMotor;
	bool fricEnable;
	float fricSpeed; //摩擦轮速度
	float triggerAngle,targetTrigAngle; //拨弹一次角度及累计角度
	uint16_t intervalTime; //连发间隔ms
	uint8_t mode;
	uint16_t taskInterval;

	char* settingName;
	char* changeModeName;
	char* triggerStallName;
}Shooter;

void Shooter_Init(Shooter* shooter, ConfItem* dict);
bool Shooter_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData);
bool Shoot_ChangeModeCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData);
void Shoot_StopCallback(const char* name, SoftBusFrame* frame, void* bindData);

void Shooter_TaskCallback(void const * argument)
{
	portENTER_CRITICAL();
	float totalAngle; //为解决warning，C语言中标签的下一条语句不能是定义变量的表达式，而case恰好就是标签
	Shooter shooter={0};
	Shooter_Init(&shooter, (ConfItem*)argument);
	portEXIT_CRITICAL();

	osDelay(2000);
	while(1)
	{
		switch(shooter.mode)
		{
			case SHOOTER_MODE_IDLE:
				osDelay(shooter.taskInterval);
				break;
			case SHOOTER_MODE_BLOCK:
				totalAngle = shooter.triggerMotor->GetData(TotalAngle); //重置电机角度为当前累计角度
				shooter.targetTrigAngle = totalAngle - shooter.triggerAngle*0.5f;  //反转
				shooter.triggerMotor->SetTarget(shooter.targetTrigAngle);
				osDelay(500);   //等待电机堵转恢复
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_ONCE:   //单发
				if(shooter.fricEnable == false)   //若摩擦轮未开启则先开启
				{
					Bus_RemoteCall("/shooter/setting", {{"fric-enable",{.Bool = true}}});
					osDelay(200);     //等待摩擦轮转速稳定
				}
				shooter.targetTrigAngle += shooter.triggerAngle; 
				shooter.triggerMotor->SetTarget(shooter.targetTrigAngle);
				shooter.mode = SHOOTER_MODE_IDLE;
				break;
			case SHOOTER_MODE_CONTINUE:  //以一定的时间间隔连续发射 
				if(shooter.fricEnable == false)   //若摩擦轮未开启则先开启
				{
					Bus_RemoteCall("/shooter/setting",{{"fric-enable",{.Bool = true}}});
					osDelay(200);   //等待摩擦轮转速稳定
				}
				shooter.targetTrigAngle += shooter.triggerAngle;  //增加拨弹电机目标角度
				shooter.triggerMotor->SetTarget(shooter.targetTrigAngle);
				osDelay(shooter.intervalTime);  
				break;
			default:
				break;
		}
	}	
}

void Shooter_Init(Shooter* shooter, ConfItem* dict)
{
	//任务间隔
	shooter->taskInterval = Conf_GetValue(dict, "task-interval", uint16_t, 20);
	//初始弹速
	shooter->fricSpeed = Conf_GetValue(dict,"fric-speed",float,5450);
	//拨弹轮拨出一发弹丸转角
	shooter->triggerAngle = Conf_GetValue(dict,"trigger-angle",float,1/7.0*360);
	//发射机构电机初始化
	shooter->fricMotors[0] = BasicMotor::Create(Conf_GetPtr(dict, "fric-motor-left", ConfItem));
	shooter->fricMotors[1] = BasicMotor::Create(Conf_GetPtr(dict, "fric-motor-right", ConfItem));
	shooter->triggerMotor = BasicMotor::Create(Conf_GetPtr(dict, "trigger-motor", ConfItem));
	//设置发射机构电机模式
	for(uint8_t i = 0; i<2; i++)
	{
		shooter->fricMotors[i]->SetMode(MOTOR_SPEED_MODE);
	}
	shooter->triggerMotor->SetMode(MOTOR_ANGLE_MODE);

	const char* temp = Conf_GetValue(dict, "name", const char*, NULL);
	temp = temp ? temp : "shooter";
	uint8_t len = strlen(temp);
	shooter->settingName = (char*)pvPortMalloc(len + 9+ 1); //9为"/   /setting"的长度，1为'\0'的长度
	sprintf(shooter->settingName, "/%s/setting", temp);

	shooter->changeModeName = (char*)pvPortMalloc(len + 6+ 1); //6为"/   /mode"的长度，1为'\0'的长度
	sprintf(shooter->changeModeName, "/%s/mode", temp);
	
	temp = Conf_GetPtr(dict, "trigger-motor/name", char);
	temp = temp ? temp : "trigger-motor";
	len = strlen(temp);
	shooter->triggerStallName = (char*)pvPortMalloc(len + 7+ 1); //7为"/   /stall"的长度，1为'\0'的长度
	sprintf(shooter->triggerStallName, "/%s/stall", temp);

	//注册回调函数
	Bus_RegisterRemoteFunc(shooter,Shooter_SettingCallback, shooter->settingName);
	Bus_RegisterRemoteFunc(shooter,Shoot_ChangeModeCallback, shooter->changeModeName);
	Bus_RegisterReceiver(shooter,Shoot_StopCallback,"/system/stop");
	Bus_RegisterReceiver(shooter,Shooter_BlockCallback, shooter->triggerStallName);
}

//射击模式
bool Shooter_SettingCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData ;
	if(Bus_IsMapKeyExist(frame,"fric-speed"))
	{
		shooter->fricSpeed = Bus_GetMapValue(frame,"fric-speed").F32;
	}

	if(Bus_IsMapKeyExist(frame,"trigger-angle"))
	{
		shooter->triggerAngle = Bus_GetMapValue(frame,"trigger-angle").F32;
	}

	if(Bus_IsMapKeyExist(frame,"fric-enable"))
	{
		shooter->fricEnable = Bus_GetMapValue(frame,"fric-enable").Bool;
		if(shooter->fricEnable == false)
		{
			shooter->fricMotors[0]->SetTarget(0);
			shooter->fricMotors[1]->SetTarget(0);
		}
		else
		{
			shooter->fricMotors[0]->SetTarget(-shooter->fricSpeed);
			shooter->fricMotors[1]->SetTarget( shooter->fricSpeed);
		}
	}
	return true;
}
bool Shoot_ChangeModeCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	if(Bus_IsMapKeyExist(frame,"mode"))
	{
		const char* mode = Bus_GetMapValue(frame,"mode").Str;
		if(!strcmp(mode, "once") && shooter->mode == SHOOTER_MODE_IDLE)  //空闲时才允许修改模式
		{
			shooter->mode = SHOOTER_MODE_ONCE;
			return true;
		}
		else if(!strcmp(mode,"continue") && shooter->mode == SHOOTER_MODE_IDLE)
		{
			if(!Bus_IsMapKeyExist(frame,"interval-time"))
				return false;
			shooter->intervalTime = Bus_GetMapValue(frame,"interval-time").U16;
			shooter->mode = SHOOTER_MODE_CONTINUE;
			return true;
		}
		else if(!strcmp(mode,"idle") && shooter->mode != SHOOTER_MODE_BLOCK)
		{
			shooter->mode = SHOOTER_MODE_IDLE;
			return true;
		}
	}
	return false;
}

//堵转
void Shooter_BlockCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	shooter->mode = SHOOTER_MODE_BLOCK;
}
//急停
void Shoot_StopCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	Shooter *shooter = (Shooter*)bindData;
	for(uint8_t i = 0; i<2; i++)
	{
		shooter->fricMotors[i]->EmergencyStop();
	}
	shooter->triggerMotor->EmergencyStop();
}

