
#include "hasher.h"
#include <stdint.h>

uint32_t Hasher_UIML32(const uint8_t *data, size_t length)
{
	uint32_t h = 0;  
	uint16_t strLength = length, alignedLen = strLength / sizeof(uint32_t);
	for(uint16_t i = 0; i < alignedLen; ++i)  
		h = (h << 5) - h + ((uint32_t*)data)[i]; 
	for(uint16_t i = alignedLen << 2; i < strLength; ++i)
		h = (h << 5) - h + data[i]; 
    return h; 
}

uint32_t Hasher_FNV1A(const uint8_t *data, size_t length)
{
    uint32_t prime = 16777619U;
    uint32_t ret = 2166136261U;
    for (size_t i = 0; i < length; i++) {
        ret ^= (uint32_t)(data[i]);
        ret *= prime;
    }
    return ret;
}
