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

#ifndef IM_PTR
#define IM_PTR(type,...) (&(type){__VA_ARGS__}) //ȡ�������ĵ�ַ
#endif

typedef void* SoftBusReceiverHandle;//�����߿��پ��
typedef void (*SoftBusBroadcastReceiver)(const char* name, SoftBusFrame* frame, void* bindData);//����ص�����ָ��
typedef bool (*SoftBusRemoteFunction)(const char* name, SoftBusFrame* request, void* bindData);//����ص�����ָ��

//������������(��ֱ�ӵ��ã�Ӧʹ���·�define����Ľӿ�)
int8_t _Bus_MultiRegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, uint16_t namesNum, char** names);
void _Bus_BroadcastSendMap(const char* name, uint16_t itemNum, SoftBusItem* items);
void _Bus_BroadcastSendList(SoftBusReceiverHandle receiverHandle, uint16_t listNum, void** list);
bool _Bus_RemoteCallMap(const char* name, uint16_t itemNum, SoftBusItem* items);
uint8_t _Bus_CheckMapKeys(SoftBusFrame* frame, uint16_t keysNum, char** keys);

/*
	@brief �����������ϵ�һ������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param name:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@note �ص���������ʽӦΪvoid callback(const char* name, SoftBusFrame* frame, void* bindData)
*/
int8_t Bus_RegisterReceiver(void* bindData, SoftBusBroadcastReceiver callback, const char* name);

/*
	@brief �����������ϵĶ������
	@param bindData:������
	@param callback:���ⷢ��ʱ�Ļص�����
	@param ...:�����ַ����б�
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ��
	@example Bus_MultiRegisterReceiver(NULL, callback, {"name1", "name2"});
*/
#define Bus_MultiRegisterReceiver(bindData, callback,...) _Bus_MultiRegisterReceiver((bindData),(callback),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ����ͨ��ʽ����ӳ�������֡
	@param name:�����ַ���
	@param ...:ӳ���
	@retval void
	@example Bus_BroadcastSend("name", {{"key1", data1}, {"key2", data2}});
*/
#define Bus_BroadcastSend(name,...) _Bus_BroadcastSendMap((name),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief �Կ��ٷ�ʽ�����б�����֡
	@param handle:���پ��
	@param ...:����ָ���б�
	@retval void
	@example float value1,value2; Bus_FastBroadcastSend(handle, {&value1, &value2});
*/
#define Bus_FastBroadcastSend(handle,...) _Bus_BroadcastSendList((handle),(sizeof((void*[])__VA_ARGS__)/sizeof(void*)),((void*[])__VA_ARGS__))

/*
	@brief �����������ϵ�һ������
	@param callback:��Ӧ����ʱ�Ļص�����
	@param name:������
	@retval 0:�ɹ� -1:�ѿռ䲻�� -2:����Ϊ�� -3:�����Ѵ���
	@note �ص���������ʽӦΪbool callback(const char* name, SoftBusFrame* request, void* bindData)
*/
int8_t Bus_RegisterRemoteFunc(void* bindData, SoftBusRemoteFunction callback, const char* name);

/*
	@brief ͨ��ӳ�������֡�������
	@param name:������
	@param ...:��������֡
	@retval true:�ɹ� false:ʧ��
	@example Bus_RemoteCall("name", {{"key1", data1}, {"key2", data2}});
*/
#define Bus_RemoteCall(name,...) _Bus_RemoteCallMap((name),(sizeof((SoftBusItem[])__VA_ARGS__)/sizeof(SoftBusItem)),((SoftBusItem[])__VA_ARGS__))

/*
	@brief ����ӳ�������֡�е������ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ��ָ�������ֶε�constָ��,����ѯ����key�򷵻�NULL
	@note ��Ӧ�Է��ص�����֡�����޸�
*/
const SoftBusItem* Bus_GetMapItem(SoftBusFrame* frame, char* key);

/*
	@brief �ж�ӳ�������֡���Ƿ����ָ���ֶ�
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval 0:������ 1:����
*/
#define Bus_IsMapKeyExist(frame,key) (Bus_GetMapItem((frame),(key)) != NULL)

/*
	@brief �жϸ���key�б��Ƿ�ȫ��������ӳ�������֡��
	@param frame:����֡��ָ��
	@param ...:Ҫ�жϵ�key�б�
	@retval 0:����һ��key������ 1:����key������
	@example if(Bus_CheckMapKeys(frame, {"key1", "key2", "key3"})) { ... }
*/
#define Bus_CheckMapKeys(frame,...) _Bus_CheckMapKeys((frame),(sizeof((char*[])__VA_ARGS__)/sizeof(char*)),((char*[])__VA_ARGS__))

/*
	@brief ��ȡӳ�������֡��ָ���ֶε�ֵ
	@param frame:����֡��ָ��
	@param key:�����ֶε�����
	@retval ָ��ֵ��(void*)��ָ��
	@note ����ȷ�������key����������֡�У�Ӧ������ؽӿڽ��м��
	@note ��Ӧͨ�����ص�ָ���޸�ָ�������
	@example float value = *(float*)Bus_GetMapValue(frame, "key");
*/
#define Bus_GetMapValue(frame,key) (Bus_GetMapItem((frame),(key))->data)

/*
	@brief ͨ�������ַ����������پ��
	@param name:�����ַ���
	@retval �������Ŀ��پ��
	@example SoftBusReceiverHandle handle = Bus_CreateReceiverHandle("name");
	@note Ӧ���ڳ����ʼ��ʱ����һ�Σ�������ÿ�η���ǰ����
*/
SoftBusReceiverHandle Bus_CreateReceiverHandle(const char* name);

/*
	@brief ��ȡ�б�����֡��ָ������������
	@param frame:����֡��ָ��
	@param pos:�������б��е�λ��
	@retval ָ�����ݵ�(void*)��ָ�룬���������򷵻�NULL
	@note ��Ӧͨ�����ص�ָ���޸�ָ�������
	@example float value = *(float*)Bus_GetListValue(frame, 0); //��ȡ�б��е�һ��ֵ
*/
#define Bus_GetListValue(frame,pos) (((pos) < (frame)->size)?((void**)(frame)->data)[(pos)]:NULL)

#endif
