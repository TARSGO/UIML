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
    void* bindData;
    void* callback;
}CallbackNode;//�ص������ڵ�

typedef struct{
    char* topic;
    Vector callbackNodes;
}TopicNode;//topic�ڵ�

typedef struct 
{
	char* service;
	CallbackNode callbackNode;
}ServiceNode;//service�ڵ�

typedef struct{
    uint32_t hash;
    Vector topicNodes;
	Vector serviceNodes;
}HashNode;//hash�ڵ�

int8_t SoftBus_Init(void);//��ʼ��������,����0:�ɹ� -1:ʧ��
uint32_t SoftBus_Str2Hash_8(const char* str);//8λ�����hash���������ַ�������С��20���ַ�ʱʹ��
uint32_t SoftBus_Str2Hash_32(const char* str);//32λ�����hash���������ַ�������С��20���ַ�ʱʹ��
void _SoftBus_Publish(const char* topic, SoftBusFrame* frame);//������Ϣ
bool _SoftBus_Request(const char* service, SoftBusFrame* request, void* response);//�������
void SoftBus_EmptyTopicCallback(const char* topic, SoftBusFrame* frame, void* bindData);//�ջص�����
bool SoftBus_EmptyServiceCallback(const char* topic, SoftBusFrame* request, void* bindData, void* response);//�ջص�����

Vector hashList={0};

int8_t SoftBus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t SoftBus_Subscribe(void* bindData, SoftBusTopicCallback callback, const char* topic)
{
	if(!topic || !callback)
		return -2;
	if(hashList.data == NULL)//���������δ��ʼ�����ʼ��������
	{
		if(SoftBus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(topic);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//������hash�ڵ�������topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//ƥ�䵽����topicע��ص�����
				{
					if(Vector_GetFront(topicNode->callbackNodes, CallbackNode)->callback == SoftBus_EmptyTopicCallback)//�����topic���пջص�����
					{
						CallbackNode* callbackNode = Vector_GetFront(topicNode->callbackNodes, CallbackNode);
						callbackNode->bindData = bindData;//���°�����
						callbackNode->callback = callback;//���»ص�����
						return 0;
					}
					return Vector_PushBack(topicNode->callbackNodes, ((CallbackNode){bindData, callback}));
				}
			}
			Vector callbackNodes = Vector_Create(CallbackNode);//δƥ�䵽topic����hash��ͻ���ڸ�hash�ڵ㴦���һ��topic�ڵ���hash��ͻ
			Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
			char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic)+1);
			SOFTBUS_MEMCPY_PORT(topicCpy, topic, SOFTBUS_STRLEN_PORT(topic)+1);
			return Vector_PushBack(hashNode->topicNodes,((TopicNode){topicCpy, callbackNodes}));
		}
	}
	Vector callbackNodes = Vector_Create(CallbackNode);//�µ�hash�ڵ�
	Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
	Vector topicV = Vector_Create(TopicNode);
	Vector serviceV = Vector_Create(ServiceNode);
	char* topicCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(topic)+1);
	SOFTBUS_MEMCPY_PORT(topicCpy, topic, SOFTBUS_STRLEN_PORT(topic)+1);
	char* serviceCpy = topicCpy;
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackNodes}));
	Vector_PushBack(serviceV, ((ServiceNode){serviceCpy, ((CallbackNode){NULL, SoftBus_EmptyServiceCallback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV, serviceV}));
}

int8_t _SoftBus_MultiSubscribe(void* bindData, SoftBusTopicCallback callback, uint16_t topicsNum, char** topics)
{
	if(!topics || !topicsNum || !callback)
		return -2;
	for (uint16_t i = 0; i < topicsNum; i++)
	{
		uint8_t retval = SoftBus_Subscribe(bindData, callback, topics[i]); //������Ļ���
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
						(*((SoftBusTopicCallback)callbackNode->callback))(topic, frame, callbackNode->bindData);
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

void _SoftBus_PublishList(SoftBusFastTopicHandle topicHandle, uint16_t listNum, void** list)
{
	if(!hashList.data || !listNum || !list)
		return;
	TopicNode* topicNode = (TopicNode*)topicHandle;
	SoftBusFrame frame = {list, listNum};
	Vector_ForEach(topicNode->callbackNodes, callbackNode, CallbackNode)
	{
		(*((SoftBusTopicCallback)callbackNode->callback))(topicNode->topic, &frame, callbackNode->bindData);
	}
}

SoftBusFastTopicHandle SoftBus_CreateFastTopicHandle(const char* topic)
{
	if(!topic)
		return NULL;
	uint32_t hash = SoftBus_Str2Hash(topic);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->topicNodes, topicNode, TopicNode)//������hash�ڵ�������topic
			{
				if(strcmp(topic, topicNode->topic) == 0)//ƥ�䵽����topicע��ص�����
				{
					return topicNode;
				}
			}
		}
	}
	SoftBus_Subscribe(NULL, SoftBus_EmptyTopicCallback, topic);//δƥ�䵽topic,ע��һ���ջص�����
	return SoftBus_CreateFastTopicHandle(topic);//�ݹ����
}

int8_t SoftBus_CreateServer(void* bindData, SoftBusServiceCallback callback, const char* service)
{
	if(!service || !callback)
		return -2;
	if(hashList.data == NULL)//���������δ��ʼ�����ʼ��������
	{
		if(SoftBus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(service);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->serviceNodes, serviceNode, ServiceNode)//������hash�ڵ�������service
			{
				if(strcmp(service, serviceNode->service) == 0)//ƥ�䵽����serviceע��ص�����
				{
					if(serviceNode->callbackNode.callback == SoftBus_EmptyServiceCallback)//�����service��Ϊ�ջص�����
					{
						serviceNode->callbackNode = ((CallbackNode){bindData, callback});//���»ص�����
						return 0;
					}
					return -3; //�÷�����ע�����������������һ�������ж��������
				}
			}
			CallbackNode callbackNode = {bindData, callback};//δƥ�䵽service����hash��ͻ���ڸ�hash�ڵ㴦���һ��service�ڵ���hash��ͻ
			char* serviceCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(service)+1);
			SOFTBUS_MEMCPY_PORT(serviceCpy, service, SOFTBUS_STRLEN_PORT(service)+1);
			return Vector_PushBack(hashNode->serviceNodes,((ServiceNode){serviceCpy, callbackNode}));
		}
	}

	Vector serviceV = Vector_Create(ServiceNode);//�µ�hash�ڵ�
	char* serviceCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(service)+1);
	SOFTBUS_MEMCPY_PORT(serviceCpy, service, SOFTBUS_STRLEN_PORT(service)+1);
	Vector callbackNodes = Vector_Create(CallbackNode);
	Vector_PushBack(callbackNodes, ((CallbackNode){NULL, SoftBus_EmptyTopicCallback}));
	Vector topicV = Vector_Create(TopicNode);
	char* topicCpy = serviceCpy;
	Vector_PushBack(topicV, ((TopicNode){topicCpy, callbackNodes}));
	Vector_PushBack(serviceV, ((ServiceNode){serviceCpy, ((CallbackNode){bindData, callback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, topicV, serviceV}));
}

bool _SoftBus_Request(const char* service, SoftBusFrame* request, void* response)
{
	if(!hashList.data ||!service || !request)
		return false;
	uint32_t hash = SoftBus_Str2Hash(service);
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)//ƥ�䵽hashֵ
		{
			Vector_ForEach(hashNode->serviceNodes, serviceNode, ServiceNode)//������hash�ڵ������service
			{
				if(strcmp(service, serviceNode->service) == 0)//ƥ�䵽service���׳���service�Ļص�����
				{
					CallbackNode callbackNode = serviceNode->callbackNode;
					return (*((SoftBusServiceCallback)callbackNode.callback))(service, request, callbackNode.bindData, response);
				}
			}
			return false;
		}
	}
	return false;
}

bool _SoftBus_RequestMap(const char* service, void* response, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!service || !response || !itemNum || !items)
		return false;
	SoftBusFrame frame = {items, itemNum};
	return _SoftBus_Request(service, &frame, response);
}

uint8_t _SoftBus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys)
{
	if(!frame || !keys || !keysNum)
		return 0;
	for(uint16_t i = 0; i < keysNum; ++i)
	{
		if(!SoftBus_GetMapItem(frame, keys[i]))
			return 0;
	}
	return 1;
}

const SoftBusItem* SoftBus_GetMapItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->size; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data + i;
		if(strcmp(key, item->key) == 0)//���keyֵ������֡����Ӧ���ֶ�ƥ�����򷵻���
			return item;
	}
	return NULL;
}

void SoftBus_EmptyTopicCallback(const char* topic, SoftBusFrame* frame, void* bindData) { }
bool SoftBus_EmptyServiceCallback(const char* topic, SoftBusFrame* request, void* bindData, void* response) {return false;}
