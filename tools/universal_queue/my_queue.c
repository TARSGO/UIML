/****************ͨ��ѭ������*****************/

#include "my_queue.h"
#include <string.h>
#include <stdlib.h>

//��ʼ������
void Queue_Init(Queue *queue,uint16_t maxSize)
{
	queue->data=malloc(maxSize*sizeof(void*));
	if(queue->data)
		queue->initialized=1;
	queue->maxSize=maxSize;
}

//������һ�����ݱ�����(���ʱ�Ὣ���ݸ��Ƶ���������Ӧλ��)
//ע�Ᵽ������С����С�ڶ��г��ȣ������Խ��
void Queue_AttachBuffer(Queue *queue,void *buffer,uint8_t elemSize)
{
	if(!queue->initialized) return;
	queue->buffer=buffer;
	queue->bufElemSize=elemSize;
}

//����һ������
void Queue_Destroy(Queue *queue)
{
	if(!queue->initialized) return;
	free(queue->data);
	memset(queue,0,sizeof(Queue));
}

//��ȡ���г���
uint16_t Queue_Size(Queue *queue)
{
	if(!queue->initialized) return 0;
	return (queue->rear+queue->maxSize-queue->front)%queue->maxSize;
}

//�����Ƿ�Ϊ��
uint8_t Queue_IsFull(Queue *queue)
{
	if(!queue->initialized) return 1;
	return (queue->front==(queue->rear+1)%queue->maxSize)?1:0;
}

//�����Ƿ�Ϊ��
uint8_t Queue_IsEmpty(Queue *queue)
{
	if(!queue->initialized) return 1;
	return (queue->front==queue->rear)?1:0;
}

//��ȡ����ָ��λ�õ�Ԫ��(��ͷԪ��λ��Ϊ0)
void* Queue_GetElement(Queue *queue,uint16_t position)
{
	if(!queue->initialized) return NULL;

	if(position>=Queue_Size(queue)) return NULL;
	
	return queue->data[(queue->front+position)%queue->maxSize];
}

//��ȡ��ͷԪ��(��������)
void* Queue_Top(Queue *queue)
{
	if(!queue->initialized) return NULL;

	if(Queue_IsEmpty(queue)) return NULL;
	
	return queue->data[queue->front];
}

//����
void* Queue_Dequeue(Queue *queue)
{
	if(!queue->initialized) return NULL;

	if(Queue_IsEmpty(queue)) return NULL;
	
	void* res=queue->data[queue->front];
	queue->front=(queue->front+1)%queue->maxSize;
	return res;
}

//���
void Queue_Enqueue(Queue *queue,void* data)
{
	if(!queue->initialized) return;

	if(Queue_IsFull(queue)) return;
	
	//���������ָ��Ϊ��˵��δ���ӱ����������ԭʼָ��
	if(queue->buffer == NULL)
	{
		queue->data[queue->rear]=data;
	}
	else//�����˱����������ݸ��ƹ�������ӱ�������ָ��
	{
		memcpy((uint8_t*)(queue->buffer) + (queue->rear)*(queue->bufElemSize),
						data,
						queue->bufElemSize);
		queue->data[queue->rear]=(uint8_t*)(queue->buffer) + (queue->rear)*(queue->bufElemSize);
	}
	
	queue->rear=(queue->rear+1)%queue->maxSize;
}
