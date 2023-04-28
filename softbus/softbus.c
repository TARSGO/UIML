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
    char* name;
    Vector callbackNodes;
}ReceiverNode;//receiver�ڵ�

typedef struct 
{
	char* name;
	CallbackNode callbackNode;
}RemoteNode;//remote�ڵ�

typedef struct{
    uint32_t hash;
    Vector receiverNodes;
	Vector remoteNodes;
}HashNode;//hash�ڵ�

int8_t Bus_Init(void);//��ʼ��������,����0:�ɹ� -1:ʧ��
uint32_t SoftBus_Str2Hash_8(const char* str);//8λ�����hash���������ַ�������С��20���ַ�ʱʹ��
uint32_t SoftBus_Str2Hash_32(const char* str);//32λ�����hash���������ַ�������С��20���ַ�ʱʹ��
void _Bus_BroadcastSend(const char* name, SoftBusFrame* frame);//������Ϣ
bool _Bus_RemoteCall(const char* name, SoftBusFrame* frame);//�������
void Bus_EmptyBroadcastReceiver(const char* name, SoftBusFrame* frame, void* bindData);//�ջص�����
bool Bus_EmptyRemoteFunction(const char* name, SoftBusFrame* frame, void* bindData);//�ջص�����

Vector hashList={0};
//��ʼ��hash��
int8_t Bus_Init()
{
    return Vector_Init(hashList,HashNode);
}

int8_t Bus_RegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, const char* name)
{
	if(!name || !callback)
		return -2;
	if(hashList.data == NULL)//���������δ��ʼ�����ʼ��������
	{
		if(Bus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(name);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//������hash�ڵ�������receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//ƥ�䵽����receiverע��ص�����
				{
					if(Vector_GetFront(receiverNode->callbackNodes, CallbackNode)->callback == Bus_EmptyBroadcastReceiver)//�����receiver���пջص�����
					{
						CallbackNode* callbackNode = Vector_GetFront(receiverNode->callbackNodes, CallbackNode);
						callbackNode->bindData = bindData;//���°�����
						callbackNode->callback = callback;//���»ص�����
						return 0;
					}
					return Vector_PushBack(receiverNode->callbackNodes, ((CallbackNode){bindData, callback}));
				}
			}
			Vector callbackNodes = Vector_Create(CallbackNode);//δƥ�䵽receiver����hash��ͻ���ڸ�hash�ڵ㴦���һ��receiver�ڵ���hash��ͻ
			Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
			char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);//��ֹname�Ǿֲ�����������ռ䱣�浽hash����
			SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
			return Vector_PushBack(hashNode->receiverNodes,((ReceiverNode){nameCpy, callbackNodes}));
		}
	}
	Vector callbackNodes = Vector_Create(CallbackNode);//�µ�hash�ڵ�
	Vector_PushBack(callbackNodes, ((CallbackNode){bindData, callback}));
	Vector receiverV = Vector_Create(ReceiverNode);
	Vector remoteV = Vector_Create(RemoteNode);
	char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
	SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
	Vector_PushBack(receiverV, ((ReceiverNode){nameCpy, callbackNodes}));
	Vector_PushBack(remoteV, ((RemoteNode){nameCpy, ((CallbackNode){NULL, Bus_EmptyRemoteFunction})}));
	return Vector_PushBack(hashList, ((HashNode){hash, receiverV, remoteV}));
}

int8_t _Bus_MultiRegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, uint16_t namesNum, char** names)
{
	if(!names || !namesNum || !callback)
		return -2;
	for (uint16_t i = 0; i < namesNum; i++)
	{
		uint8_t retval = Bus_RegisterReceiver(bindData, callback, names[i]); //������Ļ���
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

void _Bus_BroadcastSend(const char* name, SoftBusFrame* frame)
{
	if(!hashList.data ||!name || !frame)
		return;
	uint32_t hash = SoftBus_Str2Hash(name);
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)//ƥ�䵽hashֵ
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//������hash�ڵ������receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//ƥ�䵽receiver���׳���receiver���лص�����
				{
					Vector_ForEach(receiverNode->callbackNodes, callbackNode, CallbackNode)
					{
						(*((SoftBusBroadcastReceiver)callbackNode->callback))(name, frame, callbackNode->bindData);
					}
					break;
				}
			}
			break;
		}
	}
}

void _Bus_BroadcastSendMap(const char* name, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!name || !itemNum || !items)
		return;
	SoftBusFrame frame = {items, itemNum};
	_Bus_BroadcastSend(name, &frame);
}

void _Bus_BroadcastSendList(SoftBusReceiverHandle receiverHandle, uint16_t listNum, void** list)
{
	if(!hashList.data || !listNum || !list)
		return;
	ReceiverNode* receiverNode = (ReceiverNode*)receiverHandle;
	SoftBusFrame frame = {list, listNum};
	Vector_ForEach(receiverNode->callbackNodes, callbackNode, CallbackNode)//�׳��ÿ��پ�������лص�����
	{
		(*((SoftBusBroadcastReceiver)callbackNode->callback))(receiverNode->name, &frame, callbackNode->bindData);
	}
}

SoftBusReceiverHandle Bus_CreateReceiverHandle(const char* name)
{
	if(!name)
		return NULL;
	uint32_t hash = SoftBus_Str2Hash(name);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->receiverNodes, receiverNode, ReceiverNode)//������hash�ڵ�������receiver
			{
				if(strcmp(name, receiverNode->name) == 0)//ƥ�䵽����receiverע��ص�����
				{
					return receiverNode;
				}
			}
		}
	}
	Bus_RegisterReceiver(NULL, Bus_EmptyBroadcastReceiver, name);//δƥ�䵽receiver,ע��һ���ջص�����
	return Bus_CreateReceiverHandle(name);//�ݹ����
}

int8_t Bus_RegisterRemoteFunc(void* bindData, SoftBusRemoteFunction callback, const char* name)
{
	if(!name || !callback)
		return -2;
	if(hashList.data == NULL)//���������δ��ʼ�����ʼ��������
	{
		if(Bus_Init())
			return -1;
	}
	uint32_t hash = SoftBus_Str2Hash(name);//�����ַ���hashֵ
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)
		{
			Vector_ForEach(hashNode->remoteNodes, remoteNode, RemoteNode)//������hash�ڵ�������remote
			{
				if(strcmp(name, remoteNode->name) == 0)//ƥ�䵽����remoteע��ص�����
				{
					if(remoteNode->callbackNode.callback == Bus_EmptyRemoteFunction)//�����remote��Ϊ�ջص�����
					{
						remoteNode->callbackNode = ((CallbackNode){bindData, callback});//���»ص�����
						return 0;
					}
					return -3; //�÷�����ע�����������������һ�������ж��������
				}
			}
			CallbackNode callbackNode = {bindData, callback};//δƥ�䵽remote����hash��ͻ���ڸ�hash�ڵ㴦���һ��remote�ڵ���hash��ͻ
			char* remoteCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
			SOFTBUS_MEMCPY_PORT(remoteCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
			return Vector_PushBack(hashNode->remoteNodes,((RemoteNode){remoteCpy, callbackNode}));
		}
	}

	Vector remoteV = Vector_Create(RemoteNode);//�µ�hash�ڵ�
	char* nameCpy = SOFTBUS_MALLOC_PORT(SOFTBUS_STRLEN_PORT(name)+1);
	SOFTBUS_MEMCPY_PORT(nameCpy, name, SOFTBUS_STRLEN_PORT(name)+1);
	Vector callbackNodes = Vector_Create(CallbackNode);
	Vector_PushBack(callbackNodes, ((CallbackNode){NULL, Bus_EmptyBroadcastReceiver}));
	Vector receiverV = Vector_Create(ReceiverNode);
	Vector_PushBack(receiverV, ((ReceiverNode){nameCpy, callbackNodes}));
	Vector_PushBack(remoteV, ((RemoteNode){nameCpy, ((CallbackNode){bindData, callback})}));
	return Vector_PushBack(hashList, ((HashNode){hash, receiverV, remoteV}));
}

bool _Bus_RemoteCall(const char* name, SoftBusFrame* frame)
{
	if(!hashList.data ||!name || !frame)
		return false;
	uint32_t hash = SoftBus_Str2Hash(name);
	Vector_ForEach(hashList, hashNode, HashNode)//��������hash�ڵ�
	{
		if(hash == hashNode->hash)//ƥ�䵽hashֵ
		{
			Vector_ForEach(hashNode->remoteNodes, remoteNode, RemoteNode)//������hash�ڵ������remote
			{
				if(strcmp(name, remoteNode->name) == 0)//ƥ�䵽remote���׳���remote�Ļص�����
				{
					CallbackNode callbackNode = remoteNode->callbackNode;
					return (*((SoftBusRemoteFunction)callbackNode.callback))(name, frame, callbackNode.bindData);
				}
			}
			return false;
		}
	}
	return false;
}

bool _Bus_RemoteCallMap(const char* name, uint16_t itemNum, SoftBusItem* items)
{
	if(!hashList.data ||!name || !itemNum || !items)
		return false;
	SoftBusFrame frame = {items, itemNum};
	return _Bus_RemoteCall(name, &frame);
}

uint8_t _Bus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys)
{
	if(!frame || !keys || !keysNum)
		return 0;
	for(uint16_t i = 0; i < keysNum; ++i)
	{
		if(!Bus_GetMapItem(frame, keys[i]))
			return 0;
	}
	return 1;
}

const SoftBusItem* Bus_GetMapItem(SoftBusFrame* frame, char* key)
{
	for(uint16_t i = 0; i < frame->size; ++i)
	{
		SoftBusItem* item = (SoftBusItem*)frame->data + i;
		if(strcmp(key, item->key) == 0)//���keyֵ������֡����Ӧ���ֶ�ƥ�����򷵻���
			return item;
	}
	return NULL;
}

void Bus_EmptyBroadcastReceiver(const char* name, SoftBusFrame* frame, void* bindData) { }
bool Bus_EmptyRemoteFunction(const char* name, SoftBusFrame* frame, void* bindData) {return false;}
