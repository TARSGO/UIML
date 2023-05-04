# ����ģ��

---

## ���

�������������ķ���ģ�飬�����û����÷��䵥�������䡢ֹͣ�������Ӧ�Ķ�����ͬʱ���ղ�������Ķ�ת�㲥���������µ����ἰʱ�˵���


## ��Ŀ�ļ���������

- ����Ŀ�ļ�
	- `softbus.c/h`��`config.c/h`��`sys_conf.h`��`motor.c/h`(����ʹ�õ��ĵ������)
- hal���ļ� 
    - `cmsis_os.h`
- ϵͳ�㲥
    - `/system/stop`���ڼ������ù㲥������ø�ģ�������е�����뼱ͣģʽ
- �����ת�㲥
    - "/triggerMotor/stall"���ڼ��������������ת��������˵�����

---

> ע������Զ�̺�����д����������Ϊָ������ǿ������ݵ�Ӧ�������飬ʵ�ʴ��ݵĲ���ֻ�����������ɣ�����Ҫ�����������ĵ�ַ���㲥Ҳ����ˣ���д������������Ϊָ��Ľ�ǿ������ݵ�Ӧ�������飬��ȡ�����ֵ�ǽ���Ҫǿ������ת������Ӧ��ָ�뼴�ɣ�������������

---

## ˵��

���ڼ����˲��������ת�㲥�������Ҫ�����ò������ʱ����Ҫ�������������`{"name", "triggerMotor"},`

---

## ��`sys_conf.h`�е�����

```c
{"shooter", CF_DICT{
	//����ѭ������
	{"task-interval", IM_PTR(uint8_t, 10)},
	//��һ������ĽǶ�
	{"trigger-angle",IM_PTR(float,45)},
	//��������������
	{"fric-motor-left", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 2)},
		{"can-x", IM_PTR(uint8_t, 2)},
		{"reduction-ratio", IM_PTR(float, 1)},
		{"speed-pid", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},		
	{"fric-motor-right", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 1)},
		{"canX", IM_PTR(uint8_t, 2)},
		{"reduction-ratio", IM_PTR(float, 1)},
		{"speed-pid", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 10000)},
			{"max-out", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	{"trigger-motor", CF_DICT{
		{"type", "M2006"},
		{"id", IM_PTR(uint16_t, 6)},
		{"name","trigger-motor"},
		{"can-x", IM_PTR(uint8_t, 1)},
		{"angle-pid", CF_DICT{								//����pid
			{"inner", CF_DICT{								//�ڻ�pid��������
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 10000)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{								//�⻷pid��������
				{"p", IM_PTR(float, 0.3)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 2000)},
				{"max-out", IM_PTR(float, 200)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	CF_DICT_END		
}},
```

## ģ��ӿ�

- �㲥����

- Զ�̺���
  
    1. `/<shooter_name>/setting`

        ˵�������ò��������һЩ���ԣ�`<shooter_name>`Ϊ�����滻���֣����磺�������ļ������`{"shooter", "up-shooter"},`�Ϳ��Խ�Ĭ�ϵ�`/shooter/setting`���滻��`/up-shooter/setting`

        ����������ݣ�

        | �����ֶ��� | �������� | �Ƿ�Ϊ����ֵ | �Ƿ���봫�� | ˵�� |
        | :---: | :---: | :---: | :---: | :---: |
        | `fric-speed`    | `float` | �� | ��ѡ | ����Ħ����ת��(��λ��rpm) |
        | `trigger-angle` | `float` | �� | ��ѡ | ���ò�һ��������ת�ĽǶ�(��λ����) |
        | `fric-enable`   | `bool`  | �� | ��ѡ | ʹ��Ħ���� |
    
    2. `/<shooter_name>/mode`

        ˵�����޸ķ����������ģʽ��`<shooter_name>`Ϊ�����滻���֣����磺�������ļ������`{"shooter", "up-shooter"},`�Ϳ��Խ�Ĭ�ϵ�`/shooter/mode`���滻��`/up-shooter/mode`

		`once`���������� 

		`continue`����������ֱ���޸�ģʽΪidle��ֹͣ
		
		`idle`��ֹͣ���䵯��

        ����������ݣ�

        | �����ֶ��� | �������� | �Ƿ�Ϊ����ֵ | �Ƿ���봫�� | ˵�� |
        | :---: | :---: | :---: | :---: | :---: |
        | `mode`         | `char*`    | �� | ���� | ���ò���ģʽ(`"once","continue","idle"`) |
        | `interval-time` | `uint16_t` | �� | ��ѡ | ��������ʱ��Ҫ���ã���ʾ��������ʱ���η���ļ��ʱ�� |
