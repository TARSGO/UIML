#include "config.h"
#include "sys_conf.h"
#include "cmsis_os.h"

//��������ģ��ص�����
#define SERVICE(service,callback,priority,stackSize) extern void callback(void const *);
SERVICE_LIST
#undef SERVICE

//�����б�ö��
typedef enum
{
	#define SERVICE(service,callback,priority,stackSize) service,
	SERVICE_LIST
	#undef SERVICE
	serviceNum
}Module;

//����������
osThreadId serviceTaskHandle[serviceNum];

//ȡ���������ñ��и����������ֵ��û�ҵ��򷵻�NULL
void* _Conf_GetValue(ConfItem* dict, const char* name)
{
	if(!dict)
		return NULL;

	char *sep=NULL;

	do{
		sep=strchr(name,'/');
		if(sep==NULL)
			sep=(char*)name+strlen(name);

		while(dict->name)
		{
			if(strlen(dict->name)==sep-name && strncmp(dict->name,name,sep-name)==0)
			{
				if(*sep=='\0')
					return dict->value;
				dict=dict->value;
				break;
			}
			dict++;
		}

		name=sep+1;

	}while(*sep!='\0');
	return NULL;
}

//FreeRTOSĬ������
void StartDefaultTask(void const * argument)
{
	//�������з������񣬽����ñ�ֱ���Ϊ��������
	#define SERVICE(service,callback,priority,stackSize) \
		osThreadDef(service, callback, priority, 0, stackSize); \
		serviceTaskHandle[service] = osThreadCreate(osThread(service), Conf_GetPtr(systemConfig,#service,void));
	SERVICE_LIST
	#undef SERVICE
	//�����Լ�
	vTaskDelete(NULL);
}
