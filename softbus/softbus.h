#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#include <stdint.h>

typedef struct{
	void* data;
	uint16_t size;
}SoftBusFrame;//����֡

typedef struct{
    char* key;
	void* data;
}SoftBusItem;//�����ֶ�

typedef void* SoftBusFastHandle;//�����߿��پ��
typedef void (*SoftBusCallback)(const char* topic, SoftBusFrame* frame, void* bindData);//�ص�����ָ��

//������������(��ֱ�ӵ��ã�Ӧʹ���·�define����Ľӿ�)
int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusCallback callback, uint16_t topicsNum, char** topics);
void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items);
void _SoftBus_PublishList(SoftBusFastHandle topicHandle, uint16_t listNum, void** list);
void* _SoftBus_GetListValue(SoftBusFrame* frame, uint16_t pos);
uint8_t _SoftBus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys);

/*
	@brief �����������ϵ�һ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param topic:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@note �ص���������ʽӦΪvoid callback(const char* topic, SoftBusFrame* frame)
*/
int8_t SoftBus_Subscribe(void* bindData, SoftBusCallback callback, const char* topic);

/*
	@brief �����������ϵĶ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param ...:����������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@example SoftBus_MultiSubscribe(callback,{"topic1","topic2"});
*/
#define SoftBus_MultiSubscribe(bindData, callback,...) _SoftBus_MultiSubscribe((bindData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ���������Ϸ���һ������ӳ���Ļ���
	@param topic:������
	@param ...:ӳ���
	@retval void
	@example SoftBus_PublishMap("topic",{{"data1",data1,len},{"data2",data2,len}});
*/
#define SoftBus_Publish(topic,...) _SoftBus_PublishMap((topic),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief ��ȡ����֡���ӳ���������ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ�������ֶε�constָ��,��Ӧ��ͨ��ָ���޸�����,������֡�в�ѯ����key��Ӧ�������ֶ��򷵻�NULL
*/
const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key);

/*
	@brief �ж�����֡�������ֶ��Ƿ����
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval 0:������ 1:����
*/
#define SoftBus_IsMapKeyExist(frame,key) (SoftBus_GetMapItem((frame),(key)) != NULL)

/*
	@brief �ж�����֡��ĳЩ�����ֶ��Ƿ����
	@param frame:����֡��ָ��
	@param ...:Ҫ�жϵ������ֶε�����
	@retval 0:������ 1:����
*/
#define SoftBus_CheckMapKeys(frame,...) _SoftBus_CheckMapKeys((frame),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ��ȡ����֡���ӳ����ֵ��ָ��
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@param type:ֵ������
	@retval ָ��ֵ��ָ��,��Ӧ��ͨ��ָ���޸�����,������֡�в�ѯ����key��Ӧ��ֵ�򷵻�NULL
*/
#define SoftBus_GetMapValue(frame,key) (SoftBus_GetMapItem((frame),(key))->data)

/*
	@brief ��ȡ��������һ������Ŀ��پ��
	@param topic:������
	@retval ���پ��
*/
SoftBusFastHandle SoftBus_CreateFastHandle(const char* topic);

/*
	@brief ����������ͨ�����پ������һ�������б����ݵĻ���
	@param handle:���پ��
	@param ...:�б�����
	@retval void
*/
#define SoftBus_FastPublish(handle,...) _SoftBus_PublishList((handle),(sizeof((void*[])__VA_ARGS__)/sizeof(void*)),((void*[])__VA_ARGS__))

/*
	@brief ��ȡ����֡���б��е�����ָ��
	@param frame:����֡��ָ��
	@param pos:�������б��е�λ��
	@retval ָ�����ݵ�(type*)��ָ�룬���������򷵻�NULL
*/
#define SoftBus_GetListValue(frame,pos) (_SoftBus_GetListValue((frame),(pos)))

#endif
