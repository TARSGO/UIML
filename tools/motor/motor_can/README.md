# CAN�����

## ���

����Ŀ�Գ�������ľ���ʵ�֣����ڴ󽮵����������ڼ��ٱȡ�����can����֡��id�Լ��������������в�ͬ�⣬�����������ƣ����3�ֵ����ʵ�ִ������ơ�������3508���ʵ�־���

## ������

��Ŀ��freertos��`cmsis_os.h`��������ʹ����freertos�������ʱ�����Լ��Ա���Ŀ��`config.h`��`softbus.h`��`motor.h`��`pid.h`����������ȡ���ñ���Ĳ�����ʼ������Լ���ʹ��pid�Ե�����п��ƺ�ʹ�������߸�can���߽���ͨ��

## ������˵��

�������Ψһһ����Ҫ��������ģ��ͨ��������ͨ�ŵĹ����࣬���ඩ��can������Ϣ���µ�����ݣ�ͬʱ���������ʱ������pid��ɱջ����ƣ�ͬʱҲ����ж�ת��⣬�����������ת�ǻ�㲥���¼�(6020���δ��Ӷ�ת���)��Ĭ�ϻ�����Ϊ`"/motor/stall"`�����趩�ĸ��¼�ǿ�ҽ��������õ��ʱ���`name`�����ʹ�仰������ӳ��Ϊ`"/name/stall"`

## ��`sys_conf.h`�е�����

```c
{"motor", CF_DICT{
	{"type", "M3508"},
	{"id", IM_PTR(uint16_t, 1)},				//���id(����������¾��Ǽ�)
	{"canX", IM_PTR(uint8_t, 1)},
	{"name", "xxxMotor"}						//����Ҫ���ĵ����ת�㲥�¼�ʱ����Ӹ�����ʹ��ת�㲥����ӳ��Ϊ"/xxxMotor/stall"
	{"reductionRatio", IM_PTR(float, 19.2)},   //��ʹ�ø�װ��������߲��������ĵ�����޸Ĵ˲�������ʹ��ԭװ������������ô˲���
	{"speedPID", CF_DICT{                  //�ٶȵ���pidʾ��������Ҫ�ٶ�ģʽ�������ٶ�pid����Ҫ�Ƕ�ģʽ�����ýǶ�pid��������ģʽ��Ҫ�����л���������������
		{"p", IM_PTR(float, 10)},
		{"i", IM_PTR(float, 1)},
		{"d", IM_PTR(float, 0)},
		{"maxI", IM_PTR(float, 10000)},
		{"maxOut", IM_PTR(float, 20000)},
		CF_DICT_END
	}},
	{"anglePID", CF_DICT{                  //�Ƕȴ���pidʾ��������ʹ�ô���pid�մ�ģ�����ü���
		{"inner", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 10000)},
			{"maxOut", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		{"outer", CF_DICT{
			{"p", IM_PTR(float, 0.5)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 25)},
			{"maxOut", IM_PTR(float, 50)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	CF_DICT_END
}},
```
