#include "vector.h"

//���ñ�׼�⺯�������ڴ��������ͨ���޸ĺ궨�������ֲ
#include <stdlib.h>
#include <string.h>
#include "cmsis_os.h"
#define VECTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define VECTOR_FREE_PORT(ptr) vPortFree(ptr)
#define VECTOR_MEMCPY_PORT(dst,src,len) memcpy(dst,src,len)

//��ʼ��, �ɹ�����0ʧ�ܷ���-1
int _Vector_Init(Vector *vector, int _elementSize)
{
	vector->elementSize=_elementSize;
	vector->capacity=1;
	vector->size=0;
	vector->data=VECTOR_MALLOC_PORT(_elementSize);
	if(!vector->data)
		return -1;
	return 0;
}

//��������ʼ��һ��vector
Vector _Vector_Create(int _elementSize)
{
	Vector vector;
	_Vector_Init(&vector, _elementSize);
	return vector;
}

//����һ��vector
void _Vector_Destroy(Vector *vector)
{
	if(vector->data)
		VECTOR_FREE_PORT(vector->data);
}

//����Ԫ��, �ɹ�����0ʧ�ܷ���-1
int _Vector_Insert(Vector *vector, uint32_t index, void *element)
{
	if(!vector->data || index > vector->size)
		return -1;
	if(vector->size+1 > vector->capacity) //��ʣ��ռ䲻�㣬���·��������ռ䲢��������
	{
		uint8_t *newBuf=VECTOR_MALLOC_PORT(vector->capacity * 2 * vector->elementSize);
		if(!newBuf)
			return -1;
		vector->capacity*=2;
		VECTOR_MEMCPY_PORT(newBuf, vector->data, vector->size * vector->elementSize);
		VECTOR_FREE_PORT(vector->data);
		vector->data=newBuf;
	}
	if(vector->size > 0 && index <= vector->size-1) //������������ݺ���
	{
		for(int32_t pos=vector->size-1; pos>=(int32_t)index; pos--)
			VECTOR_MEMCPY_PORT(vector->data + (pos+1) * vector->elementSize, vector->data + pos * vector->elementSize, vector->elementSize);
	}
	VECTOR_MEMCPY_PORT(vector->data + index * vector->elementSize, element, vector->elementSize); //�����û������ֵ
	vector->size++;
	return 0;
}

//ɾ��Ԫ��, �ɹ�����0ʧ�ܷ���-1
int _Vector_Remove(Vector *vector, uint32_t index)
{
	if(!vector->data || vector->size==0 || index >= vector->size)
		return -1;
	for(uint32_t pos=index; pos<(vector->size-1); pos++) //��ɾ����������ǰ��
		VECTOR_MEMCPY_PORT(vector->data + pos * vector->elementSize, vector->data + (pos+1) * vector->elementSize, vector->elementSize);
	vector->size--;
	return 0;
}

//��ȡָ��λ���ϵ�Ԫ��ָ��, ��ʧ�ܷ���NULL
void *_Vector_GetByIndex(Vector *vector, uint32_t index)
{
	if(index >= vector->size)
		return NULL;
	return vector->data + index * vector->elementSize; //����ƫ�ƣ����ص�ַ
}

//�޸�ָ��λ���ϵ�Ԫ��, �ɹ�����0ʧ�ܷ���-1
int _Vector_SetValue(Vector *vector, uint32_t index, void *element)
{
	if(index >= vector->size || !vector->data)
		return -1;
	VECTOR_MEMCPY_PORT(vector->data + index * vector->elementSize, element, vector->elementSize); //�����û������ֵ��Ŀ���ַ
	return 0;
}

//ɾ������Ŀռ�, �ɹ�����0ʧ�ܷ���-1
int _Vector_TrimCapacity(Vector *vector)
{
	if(!vector->data)
		return -1;
	if(vector->capacity <= vector->size)
		return 0;
	int newCapacity=(vector->size ? vector->size : 1); //�����¿ռ��С
	uint8_t *newBuf=VECTOR_MALLOC_PORT(newCapacity * vector->elementSize); //�����¿ռ�
	if(!newBuf)
		return -1;
	if(vector->size)
		VECTOR_MEMCPY_PORT(newBuf, vector->data, vector->size * vector->elementSize); //����ԭ������
	VECTOR_FREE_PORT(vector->data);
	vector->data=newBuf;
	vector->capacity=newCapacity;
	return 0;
}
