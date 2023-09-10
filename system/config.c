
#include "stddef.h"
#include "string.h"
#include "config.h"
#include "cmsis_os.h"

//取出给定配置表中给定配置项的值，没找到则返回NULL
ConfItem* _Conf_GetValue(ConfItem* dict, const char* name)
{
	if(!dict)
		return NULL;

	return UimlYamlGetValue(dict, name);
}
