#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "yamlparser.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

// 配置项类型
//  typedef struct {
//  	char *name; //配置名
//  	void *value; //配置值
//  } ConfItem;
typedef const struct UimlYamlNode ConfItem;

// 外设句柄表
typedef struct
{
    const char *name;
    void *handle;
} PeriphHandle;

// 给用户配置文件用的宏封装
#define CF_DICT (ConfItem[])
#define CF_DICT_END                                                                                \
    {                                                                                              \
        NULL, NULL                                                                                 \
    }

#ifndef IM_PTR
#define IM_PTR(type, ...) (&(type){__VA_ARGS__}) // 取立即数的地址
#endif

// 获取配置值，不应直接调用，应使用下方的封装宏
ConfItem *_Conf_GetValue(ConfItem *dict, const char *name);

// 获取外设句柄，可以直接用但是提供了封装宏
void *_Conf_GetPeriphHandle(const char *name);

/*
    @brief 判断字典中配置名是否存在
    @param dict:目标字典
    @param name:要查找的配置名，可通过'/'分隔以查找内层字典
    @retval 0:不存在 1:存在
*/
#define Conf_ItemExist(dict, name) (_Conf_GetValue((dict), (name)) != NULL)

/*
    @brief 获取配置节点指针
    @param dict:目标字典
    @param name:要查找的配置名，可通过'/'分隔以查找内层字典
    @retval 指向配置节点的ConfItem*型指针，若配置名不存在则返回NULL
*/
#define Conf_GetNode(dict, name) (_Conf_GetValue((dict), (name)))

/*
    @brief 获取配置值
    @param dict:目标字典
    @param name:要查找的配置名，可通过'/'分隔以查找内层字典
    @param type:配置值的类型
    @param def:默认值
    @retval type型配置值数据，若配置名不存在则返回def
*/
#define Conf_GetValue(dict, name, type, def)                                                       \
    (_Conf_GetValue((dict), (name)) ? (*(type *)(&(_Conf_GetValue((dict), (name))->U32))) : (def))

/*
    @brief 获取外设句柄
    @param name:外设名
    @param type:外设句柄类型
    @retval 外设句柄
*/
#define Conf_GetPeriphHandle(name, type) ((type *)_Conf_GetPeriphHandle(name))

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

#define U_C(confptr) UimlYamlNodeObject Conf(confptr)

#endif

#endif
