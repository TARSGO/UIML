
#pragma once

#include <initializer_list>
#include <functional>

struct _SoftBusItemX;
struct _SoftBusFrame;
union _SoftBusGenericData;

void Bus_PublishTopicFast(void* endpointHandle, std::initializer_list<_SoftBusGenericData> data);
void Bus_PublishTopic(const char* name, std::initializer_list<_SoftBusItemX> parameters);
bool Bus_RemoteFuncCall(const char* endpoint, std::initializer_list<_SoftBusItemX> parameters);
bool Bus_CheckMapKeysExist(_SoftBusFrame* frame, std::initializer_list<const char *> keys);
bool Bus_SubscribeTopics(void* bindData, void(*callback)(const char*, _SoftBusFrame*, void*), std::initializer_list<const char*> endpoints);
