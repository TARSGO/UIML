#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "main.h"

//�ٷ�������ƶ��������ƶ������˵�λ��������Ϣ

#define KEY_NUM 18
//X-MACRO
#define KEY_TPYE \
	KEY(W) \
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
	MOUSE_KEY(Left) \
	MOUSE_KEY(Right) 

//����
char* keyType[] = {
	#define KEY(x) #x,
	#define MOUSE_KEY(x) #x,
	KEY_TPYE
	#undef KEY
	#undef MOUSE_KEY
};

//ң�����ݽṹ��
typedef struct {
	//ҡ������ ȡֵ[-660,660] 
	int16_t ch1;
	int16_t ch2;
	int16_t ch3;
	int16_t ch4;
	//���Ҳ��뿪�� ��1/��3/��2 
	uint8_t left;
	uint8_t right;
	//�����Ϣ 
	struct {
		//�ƶ��ٶ� 
		int16_t x;
		int16_t y;
		int16_t z;
		//���Ҽ� 
		uint8_t l;
		uint8_t r;
	} mouse;
	//������Ϣ 
	union {
		uint16_t key_code;//�������� 
		struct {
			#define KEY(x) uint16_t (x) : 1;
			#define MOUSE_KEY(x) 
			KEY_TPYE
			#undef KEY
			#undef MOUSE_KEY 
		} bit;//����λ�����Ӧ�ļ�λ 
	} kb;
	//�������� ȡֵ[-660,660]
	int16_t wheel;
}RC_TypeDef;

//�����ṹ�壬���ڼ������/���İ����¼�
typedef struct{
	//��Ҫ���õĲ���
	uint16_t clickDelayTime;//���¶�ò��㵥��һ��
	uint16_t longPressTime;//���¶�ò��㳤��
	
	//����ʹ�õĲ��������ڶ�Ӧ������Ч��һ˲��Ϊ1
	uint8_t isClicked;
	uint8_t isLongPressed;
	uint8_t isUp;
	uint8_t isPressing;
	
	//�м����
	uint8_t lastState;//1/0Ϊ����/�ɿ�
	uint32_t startPressTime;
}Key;
typedef struct 
{
	RC_TypeDef rcInfo;//ң��������
	Key keyList[KEY_NUM];//�����б�(�������п��ü��̰�����������Ҽ�)
	uint8_t uartX;
}RC;

//�ڲ�����
//��ʼ���ж�ʱ��
void RC_InitKeys(RC* rc);
void RC_InitKeyJudgeTime(RC* rc, uint32_t key,uint16_t clickDelay,uint16_t longPressDelay);
void RC_UpdateKeys(RC* rc);
void RC_ParseData(RC* rc, uint8_t* buff);
void RC_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData);

void RC_TaskCallback(void const * argument)
{
	RC rc;
	rc.uartX = Conf_GetValue((ConfItem*)argument, "uart-x", uint8_t, 0);
	RC_InitKeys(&rc);
	SoftBus_Subscribe(&rc, RC_SoftBusCallback, "/uart/recv");
	TickType_t tick = xTaskGetTickCount();
	while(1)
	{
		RC_UpdateKeys(&rc);
		osDelayUntil(&tick,14);
	}
}

//��ʼ�����а������ж�ʱ��
void RC_InitKeys(RC* rc)
{
	//��ʼ��ȫ������
	RC_InitKeyJudgeTime(rc,0x3ffff,50,500);
}

//��ʼ��һ���������ж�ʱ��(��λID�������ж�ʱ�䣬�����ж�ʱ��)
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

//���°���״̬����Ҫ��ʱ���ã�������Ϊ14ms
void RC_UpdateKeys(RC* rc)
{
	static uint32_t lastUpdateTime;
	uint32_t presentTime=HAL_GetTick();//��ȡϵͳʱ���
	
	//�����ϼ�
	char* combineKey="none";
	if(rc->rcInfo.kb.bit.Ctrl) combineKey="ctrl";
	else if(rc->rcInfo.kb.bit.Shift) combineKey="shift";
	
	for(uint8_t key=0;key<18;key++)
	{
		//��ȡ����״̬
		uint8_t thisState=0;
		if(key==4||key==5) continue;
		char* topic = NULL;
		if(key<16) 
		{
			thisState=(rc->rcInfo.kb.key_code&(0x01<<key))?1:0;//ȡ�����̶�Ӧλ
			topic = "rc/key";
		}
		else if(key==16) 
		{
			thisState=rc->rcInfo.mouse.l;
			topic = "rc/mouse-key";
		}
		else if(key==17)
		{
			 thisState=rc->rcInfo.mouse.r;
			 topic = "rc/mouse-key";
		}
		
		uint16_t lastPressTime=lastUpdateTime-rc->keyList[key].startPressTime;//�ϴθ���ʱ���µ�ʱ��
		uint16_t pressTime=presentTime-rc->keyList[key].startPressTime;//��ǰ���µ�ʱ��
		
		//����״̬�ж�
		/*******���µ�һ˲��********/
		if(!rc->keyList[key].lastState && thisState)
		{
			rc->keyList[key].startPressTime=presentTime;//��¼����ʱ��
			rc->keyList[key].isPressing=1;
			//����topic
			SoftBus_PublishMap(topic, {
				{"event", "on-down", 8},
				{"key", keyType[key], 1},
				{"combine-key", combineKey, 5}
			});
		}
		/*******�ɿ���һ˲��********/
		else if(rc->keyList[key].lastState && !thisState)
		{
			rc->keyList[key].isLongPressed=0;
			rc->keyList[key].isPressing=0;
			
			//����̧��
			rc->keyList[key].isUp=1;
			//����topic
			SoftBus_PublishMap(topic, {
				{"event", "on-up", 6},
				{"key", keyType[key], 1},
				{"combine-key", combineKey, 5}
			});
				
			//�����ж�
			if(pressTime>rc->keyList[key].clickDelayTime && pressTime<rc->keyList[key].longPressTime)
			{
				rc->keyList[key].isClicked=1;
				//����topic
			SoftBus_PublishMap(topic, {
				{"event", "on-click", 9},
				{"key", keyType[key], 1},
				{"combine-key", combineKey, 5}
			});
			}
		}
		/*******������������********/
		else if(rc->keyList[key].lastState && thisState)
		{
			//ִ��һֱ���µ��¼��ص�
			//����topic
			SoftBus_PublishMap(topic, {
				{"event", "on-pressing", 12},
				{"key", keyType[key], 1},
				{"combine-key", combineKey, 5}
			});
			
			//�����ж�
			if(lastPressTime<=rc->keyList[key].longPressTime && pressTime>rc->keyList[key].longPressTime)
			{
				rc->keyList[key].isLongPressed=1;
				//����topic
				SoftBus_PublishMap(topic, {
					{"event", "on-long-press", 14},
					{"key", keyType[key], 1},
					{"combine-key", combineKey, 5}
				});
			}
			else rc->keyList[key].isLongPressed=0;
		}
		/*******���������ɿ�********/
		else
		{
			rc->keyList[key].isClicked=0;
			rc->keyList[key].isLongPressed=0;
			rc->keyList[key].isUp=0;
		}
		
		rc->keyList[key].lastState=thisState;//��¼����״̬
	}
	
	lastUpdateTime=presentTime;//��¼�����¼�
}


void RC_SoftBusCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
	RC* rc = (RC*)bindData;
	const SoftBusItem* item = NULL;

	item = SoftBus_GetItem(frame, "can-x");
	if(!item || *(uint8_t*)item->data != rc->uartX)
		return;
	item = SoftBus_GetItem(frame, "data");
	if(item)
		RC_ParseData(rc, item->data);
}

//������������ 
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
