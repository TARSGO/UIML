#ifndef _SOFTBUS_H_
#define _SOFTBUS_H_

#ifdef __cplusplus
#include "softbus_cppapi.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// MSVC C++下，使用(Typename){Structure}会导致报错。此解决方法来自 https://github.com/raysan5/raygui/pull/44/files
#ifdef __cplusplus
    #define CURLY_INIT(name) name
#else
    #define CURLY_INIT(name) (name)
#endif

typedef struct _SoftBusFrame {
	const void* data;
	uint16_t size;
} SoftBusFrame; // 数据帧

typedef struct {
    char* key;
	void* data;
} SoftBusItem;//数据字段

typedef union _SoftBusGenericData {
	void* Ptr;
	const char* Str;
	uint32_t U32;
	uint16_t U16;
	uint8_t U8;
	int32_t I32;
	int16_t I16;
	int8_t I8;
	float F32;
	bool Bool;
	struct _SoftBusItemX* Child;
} SoftBusGenericData;

typedef struct _SoftBusItemX {
	const char* key;
	SoftBusGenericData data; // 字长以内的数据均可以直接指定
} SoftBusItemX;

#ifndef IM_PTR
//#define IM_PTR(type,...) (&(type){__VA_ARGS__}) //取立即数的地址
#endif

typedef void* SoftBusReceiverHandle;//软总线快速句柄
typedef void (*SoftBusBroadcastReceiver)(const char* name, SoftBusFrame* frame, void* bindData);//广播回调函数指针
typedef bool (*SoftBusRemoteFunction)(const char* name, SoftBusFrame* frame, void* bindData);//远程函数回调函数指针

// 定义函数使用的便利宏
#define BUS_REMOTEFUNC(func_name) bool func_name(const char* name, SoftBusFrame* frame, void* bindData)
#define BUS_TOPICENDPOINT(func_name) void func_name(const char* name, SoftBusFrame* frame, void* bindData)

//操作函数声明(不直接调用，应使用下方define定义的接口)
int8_t _Bus_SubscribeTopics(void* bindData, SoftBusBroadcastReceiver callback, uint16_t namesNum, const char* const * names);
void _Bus_PublishTopicMap(const char* name, uint16_t itemNum, const SoftBusItemX* items);
void _Bus_PublishTopicList(SoftBusReceiverHandle receiverHandle, uint16_t listNum, const SoftBusGenericData list[]);
bool _Bus_RemoteFuncCallMap(const char* name, uint16_t itemNum, const SoftBusItemX* items);
uint8_t _Bus_CheckMapKeysExist(SoftBusFrame* frame, uint16_t keysNum, const char* const * keys);

/*
	@brief 订阅软总线上的一个广播
	@param callback:广播发布时的回调函数
	@param name:广播名
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@note 回调函数的形式应为void callback(const char* name, SoftBusFrame* frame, void* bindData)
*/
int8_t Bus_SubscribeTopic(void* bindData, SoftBusBroadcastReceiver callback, const char* name);

/*
	@brief 订阅软总线上的多个广播
	@param bindData:绑定数据
	@param callback:广播发布时的回调函数
	@param ...:广播名称列表
	@retval 0:成功 -1:堆空间不足 -2:参数为空
	@example Bus_SubscribeTopics(NULL, callback, {"name1", "name2"});
*/
#ifndef __cplusplus
#define Bus_SubscribeTopics(bindData, callback,...) _Bus_SubscribeTopics((bindData),(callback),(sizeof(CURLY_INIT(char*[])__VA_ARGS__)/sizeof(char*)),(CURLY_INIT(char*[])__VA_ARGS__))
#endif

/*
	@brief 广播映射表数据帧
	@param name:广播名
	@param ...:映射表
	@retval void
	@example Bus_PublishTopic("name", {{"key1", {data1}}, {"key2", {data2}}});
*/
#ifndef __cplusplus
#define Bus_PublishTopic(name,...) _Bus_PublishTopicMap((name),(sizeof((SoftBusItemX[])__VA_ARGS__)/sizeof(SoftBusItemX)),((SoftBusItemX[])__VA_ARGS__))
#endif

/*
	@brief 通过快速句柄广播列表数据帧
	@param handle:快速句柄
	@param ...:数据指针列表
	@retval void
	@example float value1,value2; Bus_PublishTopicFast(handle, {{.F32 = value1}, {.F32 = value2}});
*/
#ifndef __cplusplus
#define Bus_PublishTopicFast(handle,...) _Bus_PublishTopicList((handle),(sizeof((SoftBusGenericData[])__VA_ARGS__)/sizeof(SoftBusGenericData)),((SoftBusGenericData[])__VA_ARGS__))
#endif

/*
	@brief 创建软总线上的一个远程函数
	@param callback:响应远程函数时的回调函数
	@param name:远程函数名
	@retval 0:成功 -1:堆空间不足 -2:参数为空 -3:远程函数已存在
	@note 回调函数的形式应为bool callback(const char* name, SoftBusFrame* frame, void* bindData)
*/
int8_t Bus_RemoteFuncRegister(void* bindData, SoftBusRemoteFunction callback, const char* name);

/*
	@brief 通过映射表数据帧调用远程函数
	@param name:远程函数名
	@param ...:远程函数参数列表(包含参数和返回值)
	@retval true:成功 false:失败
	@example Bus_RemoteFuncCall("name", {{"key1", {data1}}, {"key2", {data2}}});
*/
#ifndef __cplusplus
#define Bus_RemoteFuncCall(name,...) _Bus_RemoteFuncCallMap((name),(sizeof(__VA_ARGS__)/sizeof(SoftBusItemX)),(CURLY_INIT(SoftBusItemX[])__VA_ARGS__))
#endif

/*
	@brief 查找映射表数据帧中的数据字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向指定数据字段的const指针,若查询不到key则返回NULL
	@note 不应对返回的数据帧进行修改
*/
const SoftBusItemX* Bus_GetMapItem(SoftBusFrame* frame, const char* key);

/*
	@brief 判断映射表数据帧中是否存在指定字段
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 0:不存在 1:存在
*/
#define Bus_CheckMapKeyExist(frame,key) (Bus_GetMapItem((frame),(key)) != NULL)

/*
	@brief 判断给定key列表是否全部存在于映射表数据帧中
	@param frame:数据帧的指针
	@param ...:要判断的key列表
	@retval 0:任意一个key不存在 1:所有key都存在
	@example if(Bus_CheckMapKeysExist(frame, {"key1", "key2", "key3"})) { ... }
*/
#ifndef __cplusplus
#define Bus_CheckMapKeysExist(frame,...) _Bus_CheckMapKeysExist((frame),(sizeof(CURLY_INIT(char*[])__VA_ARGS__)/sizeof(char*)),(CURLY_INIT(const char* const[])__VA_ARGS__))
#endif

/*
	@brief 获取映射表数据帧中指定字段的值
	@param frame:数据帧的指针
	@param key:数据字段的名字
	@retval 指向值的(void*)型指针
	@note 必须确保传入的key存在于数据帧中，应先用相关接口进行检查
	@note 不应通过返回的指针修改指向的数据
	@example float value = *(float*)Bus_GetMapValue(frame, "key");
*/
#define Bus_GetMapValue(frame,key) (Bus_GetMapItem((frame),(key))->data)

/*
	@brief 通过广播名创建快速广播句柄
	@param name:广播名
	@retval 创建出的快速句柄
	@example SoftBusReceiverHandle handle = Bus_GetFastTopicHandle("name");
	@note 应仅在程序初始化时创建一次，而不是每次发布前创建
*/
SoftBusReceiverHandle Bus_GetFastTopicHandle(const char* name);

/*
	@brief 获取列表数据帧中指定索引的数据
	@param frame:数据帧的指针
	@param pos:数据在列表中的位置
	@retval 包含数据的SoftBusGenericData联合体，若不存在则返回一个空的联合体
	@note 不应通过返回的指针修改指向的数据
	@example float value = Bus_GetListValue(frame, 0).F32; //获取列表中第一个值
*/
#define Bus_GetListValue(frame,pos) (((pos) < (frame)->size)?(((SoftBusGenericData*)(frame)->data))[(pos)]:CURLY_INIT(SoftBusGenericData){NULL})

#ifdef __cplusplus
} // extern "C" {
#endif

#endif
