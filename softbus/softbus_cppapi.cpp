
#include "softbus.h"

/*
    SoftBus 模块 C++ 实现部分。
    
    SoftBus 原模块中存在部分用 C 宏实现的变长数组调用，由于 C++ 语法严格，无法移植，需要使用
    C++ 语法实现这几个 API。
*/

bool Bus_SubscribeTopics(void* bindData,
                               void(*callback)(const char*, _SoftBusFrame*, void*),
                               std::initializer_list<const char*> endpoints)
{
    return _Bus_SubscribeTopics(bindData, callback, endpoints.size(), &(*endpoints.begin()));
}


void Bus_PublishTopicFast(void* endpointHandle, std::initializer_list<SoftBusGenericData> data)
{
    _Bus_PublishTopicList(endpointHandle, data.size(), &(*data.begin()));
}

void Bus_PublishTopic(const char* endpoint, std::initializer_list<SoftBusItemX> parameters)
{
    _Bus_PublishTopicMap(endpoint, parameters.size(), &(*parameters.begin()));
}

bool Bus_RemoteFuncCall(const char* endpoint, std::initializer_list<SoftBusItemX> parameters)
{
    return _Bus_RemoteFuncCallMap(endpoint, parameters.size(), &(*parameters.begin()));
}

bool Bus_CheckMapKeysExist(_SoftBusFrame* frame, std::initializer_list<const char *> keys)
{
    return _Bus_CheckMapKeysExist(frame, keys.size(), &(*keys.begin()));
}
