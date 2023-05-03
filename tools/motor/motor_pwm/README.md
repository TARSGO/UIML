# PWM�����

## ���

����Ŀ�Գ�������ľ���ʵ��.

## ������

��Ŀ��freertos��`cmsis_os.h`��������ʹ����freertos�������ʱ�����Լ��Ա���Ŀ��`config.h`��`softbus.h`��`motor.h`��`pid.h`����������ȡ���ñ���Ĳ�����ʼ������Լ���ʹ��pid�Ե�����п��ƺ�ʹ�������߸�tim����ͨ��

## ������˵��

�������Ψһһ����Ҫ��������ģ��ͨ��������ͨ�ŵĹ����ֱ࣬�����ͨ�����������ʱ����ʱ��ȡ���������ݼ����ٶȣ�ͬʱ����pid���ƣ���������û��趨�ĽǶ�����pwmռ�ձ�

## ��`sys_conf.h`�е�����

- ֱ�����

	```c
	{"dc-motor", CF_DICT{
		{"type", "DcMotor"},
		{"maxEncode", IM_PTR(float, 48)},		//��Ƶ�������תһȦ�����ֵ
		{"reductionRatio", IM_PTR(float, 18)},	//���ٱ�
		{"posRotateTim", CF_DICT{       		//��תpwm������Ϣ
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
		{"negRotateTim", CF_DICT{				//��תתpwm������Ϣ      
			{"tim-x", IM_PTR(uint8_t, 8)},
			{"channel-x", IM_PTR(uint8_t, 2)},
			CF_DICT_END
		}},
		{"encodeTim", CF_DICT{					//������������Ϣ
			{"tim-x", IM_PTR(uint8_t, 1)},
			CF_DICT_END
		}},
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
- ���

	```c
	{"servo", CF_DICT{
		{"type", "Servo"},
		{"timX", IM_PTR(uint8_t, 1)},
		{"channelX", IM_PTR(uint8_t, 2)},
		{"maxAngle", IM_PTR(float, 180)},   //������ת��
		{"maxDuty", IM_PTR(float, 0.125f)},	//������ת�Ƕ�Ӧ��ռ�ձ�
		{"minDuty", IM_PTR(float, 0.025f)},	//���0���Ӧ��ռ�ձ�
		CF_DICT_END
	}},
	```
