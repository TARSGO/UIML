# PWM�����

## ģ����

1. ��ģ��ʱ�Գ�������ľ���ʵ��
2. �������Ψһһ����Ҫ��������ģ��ͨ��������ͨ�ŵĹ����ֱ࣬�����ͨ�����������ʱ����ʱ��ȡ���������ݼ����ٶȣ�ͬʱ����pid���ƣ���������û��趨�ĽǶ�����pwmռ�ձ�

## ģ��������

1. �ļ�����

    - ����Ŀ�ļ�
      	- `softbus.c��h`��`config.c/h`��`sys_conf.h`��`motor.c/h`��`pid.h`
  	- ��׼���ļ�
    	- `stdint.h`��`stdio.h`��`string.h`
    - hal���ļ� 
        - `cmsis_os.h`

## ģ��������

- ֱ�����

1. ģ��������
    
    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `type`            | (char*)NULL   | ������ͣ������У�[>>](../README.md) |
    | `max-encode`      | (float)48     | ��Ƶ�������תһȦ�����ֵ |
	| `reduction-ratio` | (float)19     | ������ٱ� |
	| `pos-rotate-tim`  | [>>](#motor2) | ��תpwm������Ϣ |
	| `neg-rotate-tim`  | [>>](#motor3) | ��תpwm������Ϣ |
	| `encode-tim`      | [>>](#motor4) | ������������Ϣ |
	| `speed-pid`       | [>>](../../controller/README.md) | �ٶȵ���pid |
	| `angle-pid`       | [>>](#motor5) | �Ƕȴ���pid |

2. <span id='motor2'>`pos-rotate-tim`������</span>

    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `tim-x`     | (uint8_t)0 | ��תpwmʹ�õĶ�ʱ����   |
    | `channel-x` | (uint8_t)0 | ��תpwmʹ�õĶ�ʱ��ͨ�� |

3. <span id='motor3'>`neg-rotate-tim`������</span>

    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `tim-x`     | (uint8_t)0 | ��תpwmʹ�õĶ�ʱ����   |
    | `channel-x` | (uint8_t)0 | ��תpwmʹ�õĶ�ʱ��ͨ�� |

4. <span id='motor4'>`encode-tim`������</span>

    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `tim-x` | (uint8_t)0 | ������ʹ�õĶ�ʱ���� |

5. <span id='motor5'>`angle-pid`������</span>

    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `inner` | [>>](../../controller/README.md) | �ڻ�pid |
    | `outer` | [>>](../../controller/README.md) | �⻷pid |

	```c
	{"dc-motor", CF_DICT{
		{"type", "DcMotor"},
		{"max-encode", IM_PTR(float, 48)},		//��Ƶ�������תһȦ�����ֵ
		{"reduction-ratio", IM_PTR(float, 18)},	//���ٱ�
		{"pos-rotate-tim", CF_DICT{       		//��תpwm������Ϣ
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
		{"neg-rotate-tim", CF_DICT{				//��תתpwm������Ϣ      
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 2)},
			CF_DICT_END
		}},
		{"encode-tim", CF_DICT{					//������������Ϣ
			{"tim-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
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
- ���

1. ģ��������
    
    | ������ | (��ֵ����)Ĭ��ֵ | ˵�� |
    | :---: | :---: | :---: |
    | `type`      | (char*)NULL  | ������ͣ������У�[>>](../README.md) |
    | `tim-x`     | (uint8_t)0   | ���ʹ�õĶ�ʱ���� |
	| `channel-x` | (uint8_t)0   | ���ʹ�õĶ�ʱ��ͨ�� |
	| `max-angle` | (float)180   | ������ת�� |
	| `max-duty`  | (float)0.125 | ������ת�Ƕ�Ӧ��ռ�ձ� |
	| `min-duty`  | (float)0.025 | ���0���Ӧ��ռ�ձ� |

	```c
	{"servo", CF_DICT{
		{"type", "Servo"},
		{"tim-x", IM_PTR(uint8_t, 1)},
		{"channel-x", IM_PTR(uint8_t, 2)},
		{"max-angle", IM_PTR(float, 180)},   //������ת��
		{"max-duty", IM_PTR(float, 0.125f)},	//������ת�Ƕ�Ӧ��ռ�ձ�
		{"min-duty", IM_PTR(float, 0.025f)},	//���0���Ӧ��ռ�ձ�
		CF_DICT_END
	}},
	```
