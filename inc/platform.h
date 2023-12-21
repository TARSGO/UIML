
#pragma once

// 弱符号，定义来自STM32 HAL库
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#ifndef __packed
#define __packed __attribute__((packed))
#endif
#elif defined(__GNUC__) && !defined(__CC_ARM) /* GNU Compiler */
#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif /* __packed */
#endif /* __GNUC__ */
