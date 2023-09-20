
#include "can.h"
#include "gpio.h"
#include "spi.h"
#include "usart.h"
#include "config.h"
#include <string.h>

__weak PeriphHandle* peripheralHandles = (PeriphHandle[]){
    {"can1", &hcan1},
    {"can2", &hcan2},

    {"spi1", &hspi1},

    {"uart3", &huart3},
    {"uart6", &huart6},

    {"gpioA", GPIOA},
    {"gpioB", GPIOB},
    {"gpioC", GPIOC},

    {NULL, NULL} // 结束元素，必须位于结尾
};
