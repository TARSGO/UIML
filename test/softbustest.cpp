
#include "softbus.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>

int SlowBroadcast = 0, FastBroadcast = 0;

void SlowBroadcastEndpoint(const char* name, SoftBusFrame* frame, void* bindData) {
    SlowBroadcast = Bus_GetMapValue(frame, "value").I32;
}

void FastBroadcastEndpoint(const char* name, SoftBusFrame* frame, void* bindData) {
    FastBroadcast = Bus_GetListValue(frame, 0).I32;
}

void TestSoftBusFunctionality() {
    std::cerr << __FUNCTION__ << std::endl;

    // Create fast broadcast handle
    auto fastBroadcastHandle = Bus_CreateReceiverHandle("/test/fast_broadcast");

    // Connect sources to sinks
    Bus_RegisterReceiver(nullptr, FastBroadcastEndpoint, "/test/fast_broadcast");
    Bus_RegisterReceiver(nullptr, SlowBroadcastEndpoint, "/test/slow_broadcast");

    // Make broadcast
    srand((uint32_t)time(NULL));
    int slowExpect = rand(), fastExpect = rand();

    SoftBusItemX sendData[] {
        {"value", {.I32 = slowExpect}}
    };
    SoftBusGenericData FastSendData[] {
        {.I32 = fastExpect}
    };
    Bus_BroadcastSend("/test/slow_broadcast", sendData);
    Bus_FastBroadcastSend(fastBroadcastHandle, FastSendData);

    // Check result
    if (SlowBroadcast != slowExpect) {
        std::cerr << "FAIL: Slow broadcast, expect " << slowExpect << ", got " << SlowBroadcast << std::endl;
    }

    if (FastBroadcast != fastExpect) {
        std::cerr << "FAIL: Fast broadcast, expect " << fastExpect << ", got " << FastBroadcast << std::endl;
    }
}
