
#pragma once
/*
    字节序调换。

    C620等电机通讯报文中使用了大端序的16位整数。在小端序的STM32上使用其数值前，需要调换字节序。
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 调换16位字节序
inline uint16_t SwapByteorder16Bit(uint16_t input)
{
    return ((input & 0xFF00) >> 8) | ((input & 0x00FF) << 8);
}

#ifdef __cplusplus
} // extern "C" {
#endif
