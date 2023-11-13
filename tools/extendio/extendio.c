
#include "extendio.h"
#include <stdarg.h>
#include <stdio.h>

char *alloc_sprintf(void *(*alloc_method)(size_t), const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t length = snprintf(NULL, 0, format, args);

    char *ret = alloc_method(length + 1); // 补充结尾空字符
    if (ret == NULL)
    {
        va_end(args);
        return NULL;
    }

    vsnprintf(ret, length + 1, format, args);

    va_end(args);
    return ret;
}
