#include "softbus.h"
#include "vector.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdlib.h>

#define SOFTBUS_MALLOC_PORT(len) pvPortMalloc(len)
#define SOFTBUS_FREE_PORT(ptr) vPortFree(ptr)
#define SOFTBUS_MEMCPY_PORT(dst,src,len) memcpy(dst,src,len)
#define SOFTBUS_STRLEN_PORT(str) strlen(str)

#define SoftBus_Str2Hash(str) SoftBus_Str2Hash_8(str)

typedef struct{
    uint32_t hash;
    Vector topicNodes;
}HashNode;//hash�ڵ�

typedef struct{
    char* topic;
    Vector callbackNodes;
}TopicNode;//topic�ڵ�

typedef struct{
    void* userData;
    SoftBusCallback callback;
}CallbackNode;//hash�ڵ�

int8_t SoftBus_Init(void);//��ʼ��������,����0:�ɹ� -1:ʧ��
uint32_t SoftBus_Str2Hash_8(const char* str);//8λ�����hash���������ַ�������С��20���ַ�ʱʹ��
uint32_t SoftBus_Str2Hash_32(const char* str);//32λ�����hash���������ַ�������С��20���ַ�ʱʹ��

Vector hashList={0};

int8_t SoftBus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t SoftBus_Subscribe(void* userData, SoftBusCallback callback, const char* topic)
{
	if(!topic || !callback)
		return -2;
	if(hashList.data == NULL)//���������δ��ʼ�����ʼ��������
	{
		return SoftBus_Init();
	}
	char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic));
	uint32_t hash = SoftBus_Str2Hash(topicCpy);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//������hash�ڵ�������topic
			{
				if(strcmp(topicCpy, topicNode->topic) == 0)//ƥ�䵽����topicע��ص�����
				{
					return Vector_PushBack(topicNode->callbackNodes, ((CallbackNode){userData, callback}));
				}
			}
			Vector callbackV = Vector_Create(CallbackNode);//δƥ�䵽topic����hash��ͻ���ڸ�hash�ڵ㴦���һ��topic�ڵ���hash��ͻ
			Vector_PushBack(callbackV, ((CallbackNode){userData, callback}));
			return Vector_PushBack(hashNode->topicNodes,((TopicNode){topicCpy, callbackV}));
		}
	}
	Vector callbackV = Vector_Create(CallbackNode);//�µ�hash�ڵ�
	Vector_PushBack(callbackV, ((CallbackNode){userData, callback}));
	Vector topicV = Vector_Create(TopicNode);
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackV}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV}));
}

int8_t _SoftBus_MultiSubscribe(void* userData, SoftBusCallback callback, uint16_t topicsNum, char** topics)
{
	if(!topics || !topicsNum || !callback)
		return -2;
	for (uint16_t i = 0; i < topicsNum; i++)
	{
		uint8_t retval = SoftBus_Subscribe(userData, callback, topics[i]); //������Ļ���
		if(retval)
			return retval;
	}
	return 0;
}

uint32_t SoftBus_Str2Hash_8(const char* str)  
{
    uint32_t h = 0;  
	for(uint16_t i = 0; str[i] != '\0'; ++i)  
		h = (h << 5) - h + str[i];  
    return h;  
}

uint32_t SoftBus_Str2Hash_32(const char* str)  
{
	uint32_t h = 0;  
	uint16_t strLength = strlen(str),alignedLen = strLength/sizeof(uint32_t);
	for(uint16_t i = 0; i < alignedLen; ++i)  
		h = (h << 5) - h + ((uint32_t*)str)[i]; 
	for(uint16_t i = alignedLen << 2; i < strLength; ++i)
		h = (h << 5) - h + str[i]; 
    return h; 
}

void _SoftBus_Publish(const char* topic, SoftBusFrame* frame)
{
	if(!hashList.data ||!topic || !frame)
		return;
	uint32_t hash = SoftBus_Str2Hash(topic);
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)//ƥ�䵽hashֵ
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//������hash�ڵ������topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//ƥ�䵽topic���׳���topic���лص�����
				{
					Vector_ForEach(topicNode->callbackNodes, callbackNode, CallbackNode)
					{
						(*(callbackNode->callback))(topic, frame, callbackNode->userData);
					}
					break;
				}
			}
			break;
		}
	}
}

void _SoftBus_PublishMap(const char* topic, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!topic || !itemNum || !items)
		return;
	SoftBusFrame frame = {items, itemNum};
	_SoftBus_Publish(topic, &frame);
}

const SoftBusItem* SoftBus_GetItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->length; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data;
		if(strcmp(key, item->key) == 0)//���keyֵ������֡����Ӧ���ֶ�ƥ�����򷵻���
			return item;
	}
	return NULL;
}
