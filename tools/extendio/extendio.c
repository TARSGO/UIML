
#include "extendio.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char *alloc_sprintf(void *(*alloc_method)(size_t), const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t length = vsnprintf(NULL, 0, format, args);

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

char *alloc_strcpy(void *(*alloc_method)(size_t), const char *from)
{
    size_t length = strlen(from);
    char *ret = (char *)alloc_method(length);

    strcpy(ret, from);

    return ret;
}
