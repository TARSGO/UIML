/* ����C���Ե�ͨ�����Ͷ�̬����ģ�飬������C++�е�STLģ���vector */

#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdint.h>

//Vector���ݽṹ��
typedef struct{
	uint8_t *data;
	uint8_t elementSize;
	uint32_t size,capacity;
}Vector;

//������������(��ֱ�ӵ��ã�Ӧʹ���·�define����Ľӿ�)
int _Vector_Init(Vector *vector,int _elementSize);
Vector _Vector_Create(int _elementSize);
void _Vector_Destroy(Vector *vector);
int _Vector_Insert(Vector *vector, uint32_t index, void *element);
int _Vector_Remove(Vector *vector, uint32_t index);
void *_Vector_GetByIndex(Vector *vector, uint32_t index);
int _Vector_SetValue(Vector *vector, uint32_t index, void *element);
int _Vector_TrimCapacity(Vector *vector);

/*
	@brief ��ʼ��һ��ָ�����͵�Vector
	@param vector:��Ҫ��ʼ����Vector����
	       type:��Vector�н����ŵ�Ԫ������
	@retval 0:�ɹ� -1:ʧ��
*/
#define Vector_Init(vector,type) (_Vector_Init(&(vector),sizeof(type)))

/*
	@brief ����һ��ָ�����͵�Vector
	@param type:��Vector�н����ŵ�Ԫ������
	@retval �ѳ�ʼ���õ�Vector���� (���ڴ����ʧ��������data=NULL)
*/
#define Vector_Create(type) (_Vector_Create(sizeof(type)))

/*
	@brief ����һ��Vector
	@param vector:��Ҫ���ٵ�Vector
	@retval void
*/
#define Vector_Destroy(vector) (_Vector_Destroy(&(vector)))

/*
	@brief ��ȡ�Ѳ����Ԫ������
	@param vector:Ŀ��Vector����
	@retval ��vector�е�Ԫ������
*/
#define Vector_Size(vector) ((vector).size)

/*
	@brief ��ȡ��ǰ�ѷ��������
	@param vector:Ŀ��Vector����
	@retval ��Ϊ��vector����Ŀռ����ܴ洢��Ԫ������
*/
#define Vector_Capacity(vector) ((vector).capacity)

/*
	@brief ɾ����������
	@param vector:Ŀ��Vector����
	@retval 0:�ɹ� -1:ʧ��
*/
#define Vector_TrimCapacity(vector) (_Vector_TrimCapacity(&(vector)))

/*
	@brief ��VectorתΪԪ������
	@param vector:Ŀ��vector����
	       type:Ԫ������
	@retval type*���͵�ָ�룬ָ�������׵�ַ����ֱ����Ϊ������ʹ��
	@example int num = Vector_ToArray(vector,int)[0];
*/
#define Vector_ToArray(vector,type) ((type*)((vector).data))

/*
	@brief ��ȡָ���±괦Ԫ�ص�ָ��
	@param vector:Ŀ��vector����
	       index:ָ�����±�
	       type:Ԫ������
	@retval ָ��ָ���±괦Ԫ�ص�(type*)��ָ�룬���±겻�����򷵻�NULL
*/
#define Vector_GetByIndex(vector,index,type) ((type*)_Vector_GetByIndex(&(vector),(index)))

/*
	@brief ��ȡ��Ԫ��ָ��
	@param vector:Ŀ��vector����
	       type:Ԫ������
	@retval ָ����Ԫ�ص�(type*)��ָ�룬���������򷵻�NULL
*/
#define Vector_GetFront(vector,type) Vector_GetByIndex(vector,0,type)

/*
	@brief ��ȡβԪ��ָ��
	@param vector:Ŀ��vector����
	       type:Ԫ������
	@retval ָ��βԪ�ص�(type*)��ָ�룬���������򷵻�NULL
*/
#define Vector_GetBack(vector,type) Vector_GetByIndex(vector,(vector).size-1,type)

/*
	@brief �޸�ָ���±��ֵ
	@param vector:Ŀ��vector����
	       index:ָ�����±�
	       val:��Ҫ�޸ĵ���ֵ
	@retval 0:�ɹ� -1:ʧ��
	@notice ��ϣ����val��������������Ӧʹ�����������紫��100Ӧд"(int){100}"
*/
#define Vector_SetValue(vector,index,val) (_Vector_SetValue(&(vector),(index),&(val)))

/*
	@brief ɾ��ָ���±��Ԫ��
	@param vector:Ҫ������vector����
	       index:ָ�����±�
	@retval 0:�ɹ� -1:ʧ��
*/
#define Vector_Remove(vector,index) (_Vector_Remove(&(vector),(index)))

/*
	@brief ɾ��βԪ��
	@param vector:Ҫ������vector����
	@retval 0:�ɹ� -1:ʧ��
*/
#define Vector_PopBack(vector) Vector_Remove(vector,(vector).size-1)

/*
	@brief ɾ����Ԫ��
	@param vector:Ҫ������vector����
	@retval 0:�ɹ� -1:ʧ��
*/
#define Vector_PopFront(vector) Vector_Remove(vector,0)

/*
	@brief ɾ������Ԫ��
	@param vector:Ҫ������vector����
	@retval 0
*/
#define Vector_Clear(vector) ((vector).size=0)

/*
	@brief ��ָ���±괦����Ԫ�أ����±괦ԭ��Ԫ�ؼ������Ԫ������ƶ�
	@param vector:Ŀ��vector����
	       index:ָ�����±�
	       val:��Ҫ�����ֵ
	@retval 0:�ɹ� -1:ʧ��
	@notice ��ϣ����val��������������Ӧʹ�����������紫��100Ӧд"(int){100}"
*/
#define Vector_Insert(vector,index,val) (_Vector_Insert(&(vector),(index),&(val)))

/*
	@brief ��β������Ԫ��
	@param vector:Ŀ��vector����
	       val:��Ҫ�����ֵ
	@retval 0:�ɹ� -1:ʧ��
	@notice ��ϣ����val��������������Ӧʹ�����������紫��100Ӧд"(int){100}"
*/
#define Vector_PushBack(vector,val) Vector_Insert(vector,(vector).size,(val))

/*
	@brief ��ͷ������Ԫ��
	@param vector:Ŀ��vector����
	       val:��Ҫ�����ֵ
	@retval 0:�ɹ� -1:ʧ��
	@notice ��ϣ����val��������������Ӧʹ�����������紫��100Ӧд"(int){100}"
*/
#define Vector_PushFront(vector,val) Vector_Insert(vector,0,(val))

/*
	@brief ����������䣬������������е�for���
	@param vector:Ŀ��vector����
	       iter:����ʱ�������ı���������ָ������������Ԫ�ص�(type*)��ָ��
	       type:Ԫ�ص�����
	@example Vector_ForEach(vector,ptr,int) { printf("%d",*ptr); }
*/
#define Vector_ForEach(vector,iter,type) for(type *iter=(type*)(vector).data; iter<((type*)(vector).data+(vector).size); iter++)

#endif
