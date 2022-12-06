#include "filter.h"

//X-MACRO
//�����б�ÿһ���ʽΪ(������,��ʼ��������)
#define FILTER_CHILD_LIST \
	FILTER_TYPE("kalman",Kalman_Init) \
	FILTER_TYPE("mean",Mean_Init) \
	FILTER_TYPE("low-pass",LowPass_Init)

//���������ʼ������
#define FILTER_TYPE(name,initFunc) extern Filter* initFunc(ConfItem*);
FILTER_CHILD_LIST
#undef FILTER_TYPE

//�ڲ���������
void Filter_InitDefault(Filter* filter);
float Filter_Cala(Filter *filter, float data);

Filter* Filter_Init(ConfItem* dict)
{
	char* filterType = Conf_GetPtr(dict, "type", char);

	Filter *filter = NULL;
	//�ж������ĸ�����
	#define FILTER_TYPE(name,initFunc) \
	if(!strcmp(filterType, name)) \
		filter = initFunc(dict);
	FILTER_CHILD_LIST
	#undef MOTOR_TYPE
	if(!filter)
		filter = FILTER_MALLOC_PORT(sizeof(Filter));
	
	//������δ����ķ�������Ϊ�պ���
	Filter_InitDefault(filter);

	return filter;
}

void Filter_InitDefault(Filter* filter)
{
	if(!filter->cala)
		filter->cala = Filter_Cala;
}

//���麯��
float Filter_Cala(Filter *filter, float data) {return 0;}
