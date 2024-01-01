
#pragma once
#include <string.h>

#define CREATE_FOR_BEGIN()                                                                         \
    do                                                                                             \
    {

#define CREATE_FOR(compare, given, type)                                                           \
    if (strcmp((compare), (given)) == 0)                                                           \
    {                                                                                              \
        ret = (type *)CREATE_FOR_MALLOC(sizeof(type));                                             \
        new (ret) type(); /* 用定位new来调用构造函数 */                                  \
        ret->Init(conf);                                                                           \
        break;                                                                                     \
    }

#define CREATE_FOR_END()                                                                           \
    }                                                                                              \
    while (false)                                                                                  \
        ;
