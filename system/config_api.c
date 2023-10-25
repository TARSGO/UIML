
#include <string.h>
#include <stddef.h>
#include "config.h"
#include "cmsis_os.h"
#include "sys_conf.h"

// 取出给定配置表中给定配置项的值，没找到则返回NULL
const ConfItem* _Conf_GetValue(const ConfItem* dict, const char* name)
{
	if(!dict)
		return NULL;

	return UimlYamlGetValueByPath(dict, name);
}

// 取出定义的外设句柄，没找到则返回NULL
void* _Conf_GetPeriphHandle(const char* name)
{
    for (PeriphHandle* handle = peripheralHandles; handle->name != NULL; handle++)
    {
        if (strcmp(handle->name, name) == 0)
        {
            return handle->handle;
        }
    }
    return NULL;
}
