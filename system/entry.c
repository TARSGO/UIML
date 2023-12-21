
#include <cmsis_os.h>
#include "sys_conf.h"
#include "dependency.h"
#include "yamlparser.h"

ConfItem* systemConfig = NULL;

//声明服务模块回调函数
#define SERVICE(service,callback,priority,stackSize) extern void callback(void const *);
SERVICE_LIST
#undef SERVICE

//服务任务句柄表
osThreadId serviceTaskHandle[serviceNum];

//FreeRTOS默认任务
void StartDefaultTask(void const * argument)
{
	// 执行YAML解析
	UimlYamlParse(configYaml, (struct UimlYamlNode**)&systemConfig);

	// 初始化依赖启动模块
	Depends_Init();

	//创建所有服务任务，将配置表分别作为参数传入
	#define SERVICE(service,callback,priority,stackSize) \
		osThreadDef(service, callback, priority, 0, stackSize); \
		serviceTaskHandle[svc_##service] = osThreadCreate(osThread(service), (void*)UimlYamlGetValue(systemConfig->Children,#service));
	SERVICE_LIST
	#undef SERVICE
	
	//销毁自己
	vTaskDelete(NULL);
}
