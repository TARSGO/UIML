
#include "can.h"
#include "config.h"
#include "spi.h"
#include "usart.h"
#include <string.h>

__weak PeriphHandle* peripheralHandles = (PeriphHandle[]){
    {"can1", &hcan1},
    {"can2", &hcan2},

    {"spi1", &hspi1},

    {"uart3", &huart3},
    {"uart6", &huart6},

    {NULL, NULL} // 结束元素，必须位于结尾
};

void* _Conf_GetPeriphHandle(const char* name)
{
    for (PeriphHandle* handle = peripheralHandles; handle->name != NULL; handle++)
    {
        if (strcmp(handle->name, name) == 0)
        {
            return handle->handle;
        }
    }
    return NULL;
}