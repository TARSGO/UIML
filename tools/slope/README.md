# б�º�����

## ģ����

��ģ���������ģ��б�º������û������ò�����ͨ����ģ�����ʹ��Ծ�������б�º���

## ģ��������

��

## ģ��������

��

## �ӿ�˵��

1. `void Slope_Init(Slope *slope,float step,float deadzone)`
   
    б�º�����ʼ����`step`Ϊ��������ÿ�θ���ʱ���ӵ�ֵ��`deadzone`Ϊ����������ǰֵ������ֵС������ʱ���ٸ��¡�ʹ��ʾ����

	```c
	Slope slope;//����pid
    Slope_Init(&slope, 2, 0); 
	```

2. `void Slope_SetTarget(Slope *slope,float target)`

	ͨ���˺�����������б�º�������ֵ��ʹ��ʾ����

	```c
	Slope_SetTarget(&slope, 10);
	```

3. `void Slope_SetStep(Slope *slope,float step)`

	ͨ���˺�������б�º���������ʹ��ʾ����

	```c
	Slope_SetStep(&slope, 2);
	```

4. `float Slope_NextVal(Slope *slope)`

	�˺������ڸ���б��ֵ�������ظ��º��ֵ��ʹ��ʾ����

	```c
	float value = Slope_NextVal(&slope);
	```
5. `float Slope_GetVal(Slope *slope)`

	ͨ���˺������Ի�ȡ��ǰб��ֵ��ʹ��ʾ����

	```c
	float value = Slope_GetVal(&slope);
	```
