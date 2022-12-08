#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct{
	void* data;
	uint16_t size;
}SoftBusFrame;//����֡

typedef struct{
    char* key;
	void* data;
}SoftBusItem;//�����ֶ�

typedef void* SoftBusFastTopicHandle;//�����߿��پ��
typedef void (*SoftBusTopicCallback)(const char* topic, SoftBusFrame* frame, void* bindData);//����ص�����ָ��
typedef bool (*SoftBusServiceCallback)(const char* topic, SoftBusFrame* request, void* bindData, void* response);//����ص�����ָ��

//������������(��ֱ�ӵ��ã�Ӧʹ���·�define����Ľӿ�)
int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusTopicCallback callback, uint16_t topicsNum, char** topics);
void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items);
void _SoftBus_PublishList(SoftBusFastTopicHandle topicHandle, uint16_t listNum, void** list);
bool _SoftBus_RequestMap(const char* service, void* response, uint16_t itemNum, SoftBusItem* items);
uint8_t _SoftBus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys);

/*
	@brief �����������ϵ�һ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param topic:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@note �ص���������ʽӦΪvoid callback(const char* topic, SoftBusFrame* frame, void* bindData)
*/
int8_t SoftBus_Subscribe(void* bindData, SoftBusTopicCallback callback, const char* topic);

/*
	@brief �����������ϵĶ������
	@param bindData:������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param ...:�����ַ����б�
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@example SoftBus_MultiSubscribe(NULL, callback, {"topic1", "topic2"});
*/
#define SoftBus_MultiSubscribe(bindData, callback,...) _SoftBus_MultiSubscribe((bindData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ����ͨ��ʽ����ӳ�������֡
	@param topic:�����ַ���
	@param ...:ӳ���
	@retval void
	@example SoftBus_PublishMap("topic", {{"key1", data1}, {"key2", data2}});
*/
#define SoftBus_Publish(topic,...) _SoftBus_PublishMap((topic),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief �Կ��ٷ�ʽ�����б�����֡
	@param handle:���پ��
	@param ...:����ָ���б�
	@retval void
	@example float value1,value2; SoftBus_FastPublish(handle, {&value1, &value2});
*/
#define SoftBus_FastPublish(handle,...) _SoftBus_PublishList((handle),(sizeof((void*[])__VA_ARGS__)/sizeof(void*)),((void*[])__VA_ARGS__))

/*
	@brief �����������ϵ�һ������
	@param callback:��Ӧ����ʱ�Ļص�����
	@param topic:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ�� -3:�����Ѵ���
	@note �ص���������ʽӦΪbool callback(const char* topic, SoftBusFrame* request, void* bindData, void* response)
*/
int8_t SoftBus_CreateServer(void* bindData, SoftBusServiceCallback callback, const char* service);

/*
	@brief ͨ��ӳ�������֡�������
	@param service:������
	@param response:��Ӧ���ݵ�ָ��
	@param ...:��������֡
	@retval true:�ɹ� false:ʧ��
	@example SoftBus_Request("service", &response, {{"key1", data1}, {"key2", data2}});
*/
#define SoftBus_Request(service, response,...) _SoftBus_RequestMap((service),(response),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief ����ӳ�������֡�е������ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ��ָ�������ֶε�constָ��,����ѯ����key�򷵻�NULL
	@note ��Ӧ�Է��ص�����֡�����޸�
*/
const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key);

/*
	@brief �ж�ӳ�������֡���Ƿ����ָ���ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval 0:������ 1:����
*/
#define SoftBus_IsMapKeyExist(frame,key) (SoftBus_GetMapItem((frame),(key)) != NULL)

/*
	@brief �жϸ���key�б��Ƿ�ȫ��������ӳ�������֡��
	@param frame:����֡��ָ��
	@param ...:Ҫ�жϵ�key�б�
	@retval 0:����һ��key������ 1:����key������
	@example if(SoftBus_CheckMapKeys(frame, {"key1", "key2", "key3"})) { ... }
*/
#define SoftBus_CheckMapKeys(frame,...) _SoftBus_CheckMapKeys((frame),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ��ȡӳ�������֡��ָ���ֶε�ֵ
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ��ֵ��(void*)��ָ��
	@note ����ȷ�������key����������֡�У�Ӧ������ؽӿڽ��м��
	@note ��Ӧͨ�����ص�ָ���޸�ָ�������
	@example float value = *(float*)SoftBus_GetMapValue(frame, "key");
*/
#define SoftBus_GetMapValue(frame,key) (SoftBus_GetMapItem((frame),(key))->data)

/*
	@brief ͨ�������ַ����������پ��
	@param topic:�����ַ���
	@retval �������Ŀ��پ��
	@example SoftBusFastTopicHandle handle = SoftBus_CreateFastTopicHandle("topic");
	@note Ӧ���ڳ����ʼ��ʱ����һ�Σ�������ÿ�η���ǰ����
*/
SoftBusFastTopicHandle SoftBus_CreateFastTopicHandle(const char* topic);

/*
	@brief ��ȡ�б�����֡��ָ������������
	@param frame:����֡��ָ��
	@param pos:�������б��е�λ��
	@retval ָ�����ݵ�(void*)��ָ�룬���������򷵻�NULL
	@note ��Ӧͨ�����ص�ָ���޸�ָ�������
	@example float value = *(float*)SoftBus_GetListValue(frame, 0); //��ȡ�б��е�һ��ֵ
*/
#define SoftBus_GetListValue(frame,pos) (((pos) < (frame)->size)?((void**)(frame)->data)[(pos)]:NULL)

#endif
