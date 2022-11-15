#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>
#include <stdarg.h>

typedef struct{
	void* data;
	uint16_t length;
}SoftBusFrame;//����֡

typedef struct{
    char* key;
	void* data;
	uint16_t length;
}SoftBusItem;//�����ֶ�

typedef void (*SoftBusCallback)(const char* topic, SoftBusFrame* frame, void* userData);//�ص�����ָ��

//������������(��ֱ�ӵ��ã�Ӧʹ���·�define����Ľӿ�)
int8_t _SoftBus_MultiSubscribe(void* userData, SoftBusCallback callback, uint16_t topicsNum, char** topics);
void _SoftBus_Publish(const char* topic, SoftBusFrame* frame);
void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items);

/*
	@brief �����������ϵ�һ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param topic:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@note �ص���������ʽӦΪvoid callback(const char* topic, SoftBusFrame* frame)
*/
int8_t SoftBus_Subscribe(void* userData, SoftBusCallback callback, const char* topic);

/*
	@brief �����������ϵĶ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param ...:����������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@example SoftBus_MultiSubscribe(callback,{"topic1","topic2"});
*/
#define SoftBus_MultiSubscribe(userData, callback,...) _SoftBus_MultiSubscribe((userData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ���������Ϸ���һ�������Զ����ʽ���ݵĻ���
	@param topic:������
	@param data:�Զ����ʽ����
	@param len:�Զ����ʽ���ݵĳ���(�ֽ�)
	@retval void
*/
#define SoftBus_Publish(topic,data,len) _SoftBus_Publish((topic),&(SoftBusFrame){(data),(len)})

/*
	@brief ���������Ϸ���һ������ӳ���Ļ���
	@param topic:������
	@param ...:ӳ���
	@retval void
	@example SoftBus_PublishMap("topic",{{"data1",data1,len},{"data2",data2,len}});
*/
#define SoftBus_PublishMap(topic,...) _SoftBus_PublishMap((topic),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief ��ȡ����֡��������ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ�������ֶε�constָ��,��Ӧ��ͨ��ָ���޸�����,������֡�в�ѯ����key��Ӧ�������ֶ��򷵻�NULL
*/
const SoftBusItem* SoftBus_GetItem(SoftBusFrame* frame, char* key);

#endif
