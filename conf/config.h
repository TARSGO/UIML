#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//����������
typedef struct {
	char *name; //������
	void *value; //����ֵ
} ConfItem;

//���û������ļ��õĺ��װ
#define CF_DICT (ConfItem[])
#define CF_DICT_END {NULL,NULL}

#ifndef IM_PTR
#define IM_PTR(type,...) (&(type){__VA_ARGS__}) //ȡ�������ĵ�ַ
#endif

//��ȡ����ֵ����Ӧֱ�ӵ��ã�Ӧʹ���·��ķ�װ��
void* _Conf_GetValue(ConfItem* dict, const char* name);

/*
	@brief �ж��ֵ����������Ƿ����
	@param dict:Ŀ���ֵ�
	@param name:Ҫ���ҵ�����������ͨ��'/'�ָ��Բ����ڲ��ֵ�
	@retval 0:������ 1:����
*/
#define Conf_ItemExist(dict,name) (_Conf_GetValue((dict),(name))!=NULL)

/*
	@brief ��ȡ����ֵָ��
	@param dict:Ŀ���ֵ�
	@param name:Ҫ���ҵ�����������ͨ��'/'�ָ��Բ����ڲ��ֵ�
	@param type:����ֵ������
	@retval ָ������ֵ��(type*)��ָ�룬���������������򷵻�NULL
*/
#define Conf_GetPtr(dict,name,type) ((type*)_Conf_GetValue((dict),(name)))

/*
	@brief ��ȡ����ֵ
	@param dict:Ŀ���ֵ�
	@param name:Ҫ���ҵ�����������ͨ��'/'�ָ��Բ����ڲ��ֵ�
	@param type:����ֵ������
	@param def:Ĭ��ֵ
	@retval type������ֵ���ݣ����������������򷵻�def
*/
#define Conf_GetValue(dict,name,type,def) (_Conf_GetValue((dict),(name))?(*(type*)(_Conf_GetValue((dict),(name)))):(def))

#endif
