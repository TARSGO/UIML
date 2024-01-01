
#pragma once
#include <stddef.h>

// clang-format off

#ifdef __cplusplus
// 调用后产生系统崩溃
extern "C" [[noreturn]]
void UimlCrashSystem(const char *reason = "",
                     void *param1 = NULL,
                     void *param2 = NULL,
                     void *param3 = NULL,
                     void *param4 = NULL);
#else
void UimlCrashSystem(const char *reason,
                     void *param1,
                     void *param2,
                     void *param3,
                     void *param4);
#endif

#ifdef UIML_STRICT_MODE

#define UIML_CRASH_STRICT UimlCrashSystem
#define UIML_FATAL_ASSERT(condition,reason) if(!(condition)) { UimlCrashSystem(reason, NULL, NULL, NULL, NULL); }

#else

#define UIML_CRASH_STRICT(...)
#define UIML_FATAL_ASSERT(condition,reason)

#endif
