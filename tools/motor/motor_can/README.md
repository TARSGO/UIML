# CAN�����

## ģ����

1. ����Ŀ�Գ�������ľ���ʵ�֣����ڴ󽮵����������ڼ��ٱȡ�����can����֡��id�Լ��������������в�ͬ�⣬�����������ƣ����3�ֵ����ʵ�ִ�������
2. �������Ψһһ����Ҫ��������ģ��ͨ��������ͨ�ŵĹ����࣬���ඩ��can������Ϣ���µ�����ݣ�ͬʱ���������ʱ������pid��ɱջ����ƣ�ͬʱҲ����ж�ת��⣬�����������ת�ǻ�㲥���¼�(6020���δ��Ӷ�ת���)��Ĭ�ϻ�����Ϊ`"/motor/stall"`�����趩�ĸ��¼�ǿ�ҽ��������õ��ʱ���`name`�����ʹ�仰������ӳ��Ϊ`"/name/stall"`

## ģ��������

1. �ļ�����

    - ����Ŀ�ļ�
      	- `softbus.c��h`��`config.c/h`��`sys_conf.h`��`motor.c/h`��`pid.h`
  	- ��׼���ļ�
    	- `stdint.h`��`stdlib.h`��`string.h`
    - hal���ļ� 
        - `cmsis_os.h`

## ģ��������

1. ģ��������
    
    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `type`            | (char*)NULL | ������ͣ������У�[>>](../README.md) |
    | `id`              | (uint16_t)0 | ���id(����������¾��Ǽ�) |
	| `can-x`           | (uint16_t)0 | ����������can������ |
	| `name`            | (char*)"motor" | ����Ҫ���ĵ����ת�㲥����Ҫ���ô���Ϣ |
	| `reduction-ratio` | (float)Ĭ��ԭװ������ٱ� | ������ٱȣ�ʹ��ԭװ������������ô˲��� |
	| `speed-pid`       | [>>](../../controller/README.md) | �ٶȵ���pid |
	| `angle-pid`       | [>>](#motor2) | �Ƕȴ���pid |

2. <span id='motor2'>`angle-pid`������</span>

    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `inner` | [>>](../../controller/README.md) | �ڻ�pid |
    | `outer` | [>>](../../controller/README.md) | �⻷pid |

### ʾ����

```c
{"motor", CF_DICT{
	{"type", "M3508"},
	{"id", IM_PTR(uint16_t, 1)},				//���id(����������¾��Ǽ�)
	{"can-x", IM_PTR(uint8_t, 1)},
	{"name", "xxx-motor"}						//����Ҫ���ĵ����ת�㲥�¼�ʱ����Ӹ�����ʹ��ת�㲥����ӳ��Ϊ"/xxxMotor/stall"
	{"reduction-ratio", IM_PTR(float, 19.2)},   //��ʹ�ø�װ��������߲��������ĵ�����޸Ĵ˲�������ʹ��ԭװ������������ô˲���
	{"speed-pid", CF_DICT{                  //�ٶȵ���pidʾ��������Ҫ�ٶ�ģʽ�������ٶ�pid����Ҫ�Ƕ�ģʽ�����ýǶ�pid��������ģʽ��Ҫ�����л���������������
		{"p", IM_PTR(float, 10)},
		{"i", IM_PTR(float, 1)},
		{"d", IM_PTR(float, 0)},
		{"max-i", IM_PTR(float, 10000)},
		{"max-out", IM_PTR(float, 20000)},
		CF_DICT_END
	}},
	{"angle-pid", CF_DICT{                  //�Ƕȴ���pidʾ��������ʹ�ô���pid�մ�ģ�����ü���
		{"inner", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		{"outer", CF_DICT{
			{"p", IM_PTR(float, 0.5)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 25)},
			{"max-out", IM_PTR(float, 50)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	CF_DICT_END
}},
```
