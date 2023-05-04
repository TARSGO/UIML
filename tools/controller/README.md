# PID��

## ģ����

1. ����Ŀʵ����һ��PID�࣬�ܹ����е���������PID���㡢���PID���ݡ����������ȹ���

## ģ��������

1. �ļ�����

    - ����Ŀ�ļ�
      	- `config.c/h`��`sys_conf.h`
  	- ��׼���ļ�
    	- `stdint.h`

## ģ��������

1. ģ��������
    
    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `p`       | (float)0  | kp |
	| `i`       | (float)0  | ki |
	| `d`       | (float)0  | kd |
	| `max-i`   | (float)0  | �����޷� |
	| `max-out` | (float)0  | ����޷� |

#### ʾ����

```c
{"pid", CF_DICT{
    {"p", IM_PTR(float, 0.15)},
    {"i", IM_PTR(float, 0.01)},
    {"d", IM_PTR(float, 0)},
    {"max-i", IM_PTR(float, 0.15)},
    {"max-out", IM_PTR(float, 1)},
    CF_DICT_END
}},
```

## �ӿ�˵��

1. `void PID_Init(PID *pid, ConfItem* conf)`
   
    ����ݴ�������ó�ʼ��pid������ʹ��ʾ����

	```c
	PID pid;//����pid
    PID_Init(&pid, dict); 
    CascadePID pidC;//����pid
    PID_Init(&pidC.inner, dict); //��ʼ���ڻ�
    PID_Init(&pidC.outer, dict); //��ʼ���⻷
	```

2. `void PID_SingleCalc(PID *pid,float reference,float feedback)`

	ͨ���˺������Լ��㵥��pid������`reference`ΪĿ��ֵ��`feedback`Ϊ����ֵ��ʹ��ʾ����

	```c
	PID_SingleCalc(&pid, ref, fd);
	```

3. `void PID_CascadeCalc(CascadePID *pid,float angleRef,float angleFdb,float speedFdb)`

	ͨ���˺������Լ��㵥��pid������`angleRef`Ϊ�Ƕ�Ŀ��ֵ��`angleFdb`Ϊ�Ƕȷ���ֵ��`speedFdb`Ϊ�ٶȷ���ֵ��ʹ��ʾ����

	```c
	PID_SingleCalc(&pidC, angleRef, angleFdb, speedFdb);
	```

4. `void PID_Clear(PID *pid)`

	ͨ���˺����������pid����ʷ���ݡ�ʹ��ʾ����

	```c
	PID_Clear(&pid);
	```
5. `void PID_SetMaxOutput(PID *pid,float maxOut)`

	ͨ���˺�����������pid������޷���ʹ��ʾ����

	```c
	PID_SetMaxOutput(&pid);
	```

6. `void PID_SetDeadzone(PID *pid,float deadzone)`

	ͨ���˺�����������pid��������ʹ��ʾ����

	```c
	PID_SetDeadzone(&pid, 5);
	```

