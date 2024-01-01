
#include "can.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "config.h"
#include <string.h>

__weak PeriphHandle* peripheralHandles = (PeriphHandle[]){

    {"gpioA", GPIOA},

    {NULL, NULL} // 结束元素，必须位于结尾
};
