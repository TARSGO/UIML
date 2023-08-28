
#pragma once

#include <initializer_list>
#include <functional>

struct _SoftBusItemX;
struct _SoftBusFrame;
union _SoftBusGenericData;

void Bus_FastBroadcastSend(void* endpointHandle, std::initializer_list<_SoftBusGenericData> data);
void Bus_BroadcastSend(const char* name, std::initializer_list<_SoftBusItemX> parameters);
bool Bus_RemoteCall(const char* endpoint, std::initializer_list<_SoftBusItemX> parameters);
bool Bus_CheckMapKeys(_SoftBusFrame* frame, std::initializer_list<const char *> keys);
bool Bus_MultiRegisterReceiver(void* bindData, std::function<void(const char*, _SoftBusFrame*, void*)> callback, std::initializer_list<const char*> endpoints);
