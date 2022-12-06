#include "filter.h"

//��ֵ�˲��ṹ��
typedef struct
{
	Filter filter;

	uint8_t index;
	float* array;
	uint8_t size;
	float sum;
}MeanFilter;

float Mean_Cala(Filter* filter, float data);

/**
  * @brief  ��ʼ��һ����ֵ�˲���
  * @param  �˲����ṹ��
  * @param  ��Ҫ�������˲����ṹ�������
  * @param  �˲�����������С���˲����ڲ����ڴ洢���ݵ�����Ĵ�С��
  * @retval None
  */
Filter* Mean_Init(ConfItem* dict)
{
	MeanFilter* mean = (MeanFilter*)FILTER_MALLOC_PORT(sizeof(MeanFilter));
	memset(mean, 0, sizeof(MeanFilter));

	mean->filter.cala = Mean_Cala;
	mean->size = Conf_GetValue(dict, "size", uint8_t, 1);;
	mean->index = 0;
	mean->sum = 0;
	mean->array = (float*)FILTER_MALLOC_PORT(mean->size * sizeof(float));
	memset(mean->array, 0, mean->size * sizeof(float));

	return (Filter*)mean;
}

/**
  * @brief  ��ֵ�˲�����
  * @param  �˲����ṹ�壬��Ҫ�˲�������
  * @retval �˲������
  * @attention None
  */
float Mean_Cala(Filter* filter, float data)
{
	MeanFilter *mean = (MeanFilter*)filter;
	
	mean->sum -= mean->array[mean->index];
	mean->sum += data;
	float retval = mean->sum / (float)mean->size;
	
	mean->array[mean->index++] = data;
	if(mean->index >= mean->size)
		mean->index = 0;

	return retval;
}