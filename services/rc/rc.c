#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdlib.h>

#define KEY_NUM 18
//X-MACRO
#define KEY_TPYE \
	KEY(W) \
	KEY(S) \
	KEY(A) \
	KEY(D) \
	KEY(Shift) \
	KEY(Ctrl) \
	KEY(Q) \
	KEY(E) \
	KEY(R) \
	KEY(F) \
	KEY(G) \
	KEY(Z) \
	KEY(X) \
	KEY(C) \
	KEY(V) \
	KEY(B) \
	MOUSE_KEY(left) \
	MOUSE_KEY(right)

//主键
char* keyType[] = {
	#define KEY(x) #x,
	#define MOUSE_KEY(x) #x,
	KEY_TPYE
	#undef KEY
	#undef MOUSE_KEY
};


//遥控数据结构体
typedef struct {
	//摇杆数据 取值[-660,660] 
	int16_t ch1;
	int16_t ch2;
	int16_t ch3;
	int16_t ch4;
	//左右拨码开关 上1/中3/下2 
	uint8_t left;
	uint8_t right;
	//鼠标信息 
	struct {
		//移动速度 
		int16_t x;
		int16_t y;
		int16_t z;
		//左右键 
		uint8_t l;
		uint8_t r;
	} mouse;
	//键盘信息 
	union {
		uint16_t key_code;//按键编码 
		struct {
			#define KEY(x) uint16_t x : 1;
			#define MOUSE_KEY(x)
			KEY_TPYE
			#undef KEY
			#undef MOUSE_KEY
		} bit;//各个位代表对应的键位 
	} kb;
	//拨轮数据 取值[-660,660]
	int16_t wheel;
}RC_TypeDef;

//按键结构体，用于计算键盘/鼠标的按键事件
typedef struct{
	//需要配置的参数
	uint16_t clickDelayTime;//按下多久才算单击一次
	uint16_t longPressTime;//按下多久才算长按
	
	//用来使用的参数，仅在对应条件有效的一瞬间为1
	uint8_t isClicked;
	uint8_t isLongPressed;
	uint8_t isUp;
	uint8_t isPressing;
	
	//中间变量
	uint8_t lastState;//1/0为按下/松开
	uint32_t startPressTime;
}Key;
typedef struct 
{
	RC_TypeDef rcInfo;//遥控器数据
	Key keyList[KEY_NUM];//按键列表(包含所有可用键盘按键和鼠标左右键)
	uint8_t uartX;
}RC;

//内部函数
//初始化判定时间
void RC_InitKeys(RC* rc);
void RC_InitKeyJudgeTime(RC* rc, uint32_t key,uint16_t clickDelay,uint16_t longPressDelay);
void RC_PublishData(RC *rc);
void RC_UpdateKeys(RC* rc);
void RC_ParseData(RC* rc, uint8_t* buff);
void RC_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData);

static RC* GlobalRC; // 调试用

void RC_TaskCallback(void const * argument)
{
	//进入临界区
	portENTER_CRITICAL();
	RC rc={0};
	rc.uartX = Conf_GetValue((ConfItem*)argument, "uart-x", uint8_t, 0);
	RC_InitKeys(&rc);
	char name[] = "/uart_/recv";
	name[5] = rc.uartX + '0';
	Bus_RegisterReceiver(&rc, RC_SoftBusCallback, name);
	portEXIT_CRITICAL();

	GlobalRC = &rc;
	
	osDelay(2000);
	while(1)
	{
		RC_PublishData(&rc);
		osDelay(14);
	}
}

//初始化所有按键的判定时间
void RC_InitKeys(RC* rc)
{
	//初始化全部按键(0x3ffff)
	RC_InitKeyJudgeTime(rc,0x3ffff,50,500);
}

//初始化一个按键的判定时间(键位ID，单击判定时间，长按判定时间)
void RC_InitKeyJudgeTime(RC* rc, uint32_t key,uint16_t clickDelay,uint16_t longPressDelay)
{
	for(uint8_t i=0;i<18;i++)
	{
		if(key&(0x01<<i))
		{
			rc->keyList[i].clickDelayTime=clickDelay;
			rc->keyList[i].longPressTime=longPressDelay;
		}
	}
}


//若有数据更新则发布rc数据
void RC_PublishData(RC *rc)
{
	static RC_TypeDef lastData={0};

	if(abs(lastData.ch1) > 5 || abs(lastData.ch2) > 5)
		Bus_BroadcastSend("/rc/right-stick",{{"x",&rc->rcInfo.ch1},{"y",&rc->rcInfo.ch2}}); 
	
	if(abs(lastData.ch3) > 5 || abs(lastData.ch4) > 5)
		Bus_BroadcastSend("/rc/left-stick",{{"x",&rc->rcInfo.ch3},{"y",&rc->rcInfo.ch4}});

	if(lastData.mouse.x != rc->rcInfo.mouse.x || lastData.mouse.y != rc->rcInfo.mouse.y)
		Bus_BroadcastSend("/rc/mouse-move",{{"x",&rc->rcInfo.mouse.x},{"y",&rc->rcInfo.mouse.y}});
	
	if(lastData.left != rc->rcInfo.left)
		Bus_BroadcastSend("/rc/switch",{{"left",&rc->rcInfo.left}});

	if(lastData.right != rc->rcInfo.right)
		Bus_BroadcastSend("/rc/switch",{{"right",&rc->rcInfo.right}});

	if(lastData.wheel != rc->rcInfo.wheel)
		Bus_BroadcastSend("/rc/wheel",{{"value",&rc->rcInfo.wheel}});
	
	//更新按键状态并发布
	RC_UpdateKeys(rc);

	lastData =rc->rcInfo;
}
//更新按键状态，需要定时调用，建议间隔为14ms
void RC_UpdateKeys(RC* rc)
{
	static uint32_t lastUpdateTime;
	uint32_t presentTime=osKernelSysTick();//获取系统时间戳
	
	//检查组合键
	char* combineKey="none";
	if(rc->rcInfo.kb.bit.Ctrl) combineKey="ctrl";
	else if(rc->rcInfo.kb.bit.Shift) combineKey="shift";
	
	for(uint8_t key=0;key<18;key++)
	{
		//读取按键状态
		uint8_t thisState=0;
		if(key==4||key==5) continue;
		char name[22] = "/rc/key/";
		if(key<16) 
		{
			thisState=(rc->rcInfo.kb.key_code&(0x01<<key))?1:0;//取出键盘对应位
		}
		else if(key==16) 
		{
			thisState=rc->rcInfo.mouse.l;
		}
		else if(key==17)
		{
			 thisState=rc->rcInfo.mouse.r;
		}
		
		uint16_t lastPressTime=lastUpdateTime-rc->keyList[key].startPressTime;//上次更新时按下的时间
		uint16_t pressTime=presentTime-rc->keyList[key].startPressTime;//当前按下的时间
		
		//按键状态判定
		/*******按下的一瞬间********/
		if(!rc->keyList[key].lastState && thisState)
		{
			rc->keyList[key].startPressTime=presentTime;//记录按下时间
			rc->keyList[key].isPressing=1;
			//发布topic
			memcpy(name+8, "on-down", 8);
			Bus_BroadcastSend(name, {
				{"key", keyType[key]},
				{"combine-key", combineKey}
			});
		}
		/*******松开的一瞬间********/
		else if(rc->keyList[key].lastState && !thisState)
		{
			rc->keyList[key].isLongPressed=0;
			rc->keyList[key].isPressing=0;
			
			//按键抬起
			rc->keyList[key].isUp=1;
			//发布topic
			memcpy(name+8, "on-up", 6);
			Bus_BroadcastSend(name, {
				{"key", keyType[key]},
				{"combine-key", combineKey}
			});
				
			//单击判定
			if(pressTime>rc->keyList[key].clickDelayTime && pressTime<rc->keyList[key].longPressTime)
			{
				rc->keyList[key].isClicked=1;
				//发布topic
				memcpy(name+8, "on-click", 9);
				Bus_BroadcastSend(name, {
					{"key", keyType[key]},
					{"combine-key", combineKey}
				});
			}
		}
		/*******按键持续按下********/
		else if(rc->keyList[key].lastState && thisState)
		{
			//执行一直按下的事件回调
			//发布topic
			memcpy(name+8, "on-pressing", 12);
			Bus_BroadcastSend(name, {
				{"key", keyType[key]},
				{"combine-key", combineKey}
			});
			
			//长按判定
			if(lastPressTime<=rc->keyList[key].longPressTime && pressTime>rc->keyList[key].longPressTime)
			{
				rc->keyList[key].isLongPressed=1;
				//发布topic
				memcpy(name+8, "on-long-press", 14);
				Bus_BroadcastSend(name, {
					{"key", keyType[key]},
					{"combine-key", combineKey}
				});
			}
			else rc->keyList[key].isLongPressed=0;
		}
		/*******按键持续松开********/
		else
		{
			rc->keyList[key].isClicked=0;
			rc->keyList[key].isLongPressed=0;
			rc->keyList[key].isUp=0;
		}
		
		rc->keyList[key].lastState=thisState;//记录按键状态
	}
	
	lastUpdateTime=presentTime;//记录更新事件
}


void RC_SoftBusCallback(const char* name, SoftBusFrame* frame, void* bindData)
{
	RC* rc = (RC*)bindData;
	
	uint8_t* data = (uint8_t*)Bus_GetListValue(frame, 0).Ptr;
	uint16_t len = Bus_GetListValue(frame, 1).U16;

	if(data && len)
		RC_ParseData(rc, data);
}

//解析串口数据 
void RC_ParseData(RC* rc, uint8_t* buff)
{
	rc->rcInfo.ch1 = (buff[0] | buff[1] << 8) & 0x07FF;
	rc->rcInfo.ch1 -= 1024;
	rc->rcInfo.ch2 = (buff[1] >> 3 | buff[2] << 5) & 0x07FF;
	rc->rcInfo.ch2 -= 1024;
	rc->rcInfo.ch3 = (buff[2] >> 6 | buff[3] << 2 | buff[4] << 10) & 0x07FF;
	rc->rcInfo.ch3 -= 1024;
	rc->rcInfo.ch4 = (buff[4] >> 1 | buff[5] << 7) & 0x07FF;
	rc->rcInfo.ch4 -= 1024;

	rc->rcInfo.left = ((buff[5] >> 4) & 0x000C) >> 2;
	rc->rcInfo.right = (buff[5] >> 4) & 0x0003;
	
	rc->rcInfo.mouse.x = buff[6] | (buff[7] << 8);
	rc->rcInfo.mouse.y = buff[8] | (buff[9] << 8);
	rc->rcInfo.mouse.z = buff[10] | (buff[11] << 8);

	rc->rcInfo.mouse.l = buff[12];
	rc->rcInfo.mouse.r = buff[13];

	rc->rcInfo.kb.key_code = buff[14] | buff[15] << 8;

	rc->rcInfo.wheel = (buff[16] | buff[17] << 8) - 1024;
}
