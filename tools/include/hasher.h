
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t Hasher_UIML32(const uint8_t* data, size_t length);
uint32_t Hasher_FNV1A(const uint8_t* data, size_t length);

#ifdef __cplusplus
} // extern "C" {
#endif
