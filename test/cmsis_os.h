
// 假的CMSIS OS/FreeRTOS API，为测试而生。

#include <stdlib.h>
#define pvPortMalloc malloc
#define vPortFree free
#define portENTER_CRITICAL()
#define portEXIT_CRITICAL()
