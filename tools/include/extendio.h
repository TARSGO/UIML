
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 使用指定的分配方法，分配一个恰好足够某格式化后字符串长度的内存，并将其格式化到内存中。
 *
 * @param alloc_method 分配方法。应返回 void*，接受一个size_t型变量来指定字节数。
 * @param format 格式字符串。
 * @param ... 格式化的参数。
 * @return char* 返回分配后并格式化后的字符串。
 */
char *alloc_sprintf(void *(*alloc_method)(size_t), const char *format, ...);

#ifdef __cplusplus
} // extern "C" {
#endif