# 发射模块

---

## 简介

这是整个代码库的发射模块，根据用户设置发射单发、连射、停止，完成相应的动作，同时接收拨弹电机的堵转广播，若发生堵弹，会及时退弹。


## 项目文件及依赖项

- 本项目文件
	- `softbus.c/h`、`config.c/h`、`sys_conf.h`、`motor.c/h`(及其使用到的电机子类)
- hal库文件 
    - `cmsis_os.h`
- 系统广播
    - `/system/stop`：在监听到该广播后会设置该模块下所有电机进入急停模式
- 电机堵转广播
    - "/triggerMotor/stall"：在监听到拨弹电机堵转后会做出退弹操作

---

> 注：下面远程函数所写的数据类型为指针的项仅强调该项传递的应该是数组，实际传递的参数只需数组名即可，不需要传递数组名的地址。广播也是如此，所写的数据类型若为指针的仅强调该项传递的应该是数组，获取该项的值是仅需要强制类型转换成相应的指针即可，无需额外解引用

---

## 说明

由于监听了拨弹电机堵转广播，因此需要在配置拨弹电机时，需要将拨弹电机命名`{"name", "triggerMotor"},`

---

## 在`sys_conf.h`中的配置

```c
{"shooter", CF_DICT{
	//任务循环周期
	{"taskInterval", IM_PTR(uint8_t, 10)},
	//拨一发弹丸的角度
	{"triggerAngle",IM_PTR(float,45)},
	//发射机构电机配置
	{"fricMotorLeft", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 2)},
		{"canX", IM_PTR(uint8_t, 2)},
		{"reductionRatio", IM_PTR(float, 1)},
		{"speedPID", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 10000)},
			{"maxOut", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},		
	{"fricMotorRight", CF_DICT{
		{"type", "M3508"},
		{"id", IM_PTR(uint16_t, 1)},
		{"canX", IM_PTR(uint8_t, 2)},
		{"reductionRatio", IM_PTR(float, 1)},
		{"speedPID", CF_DICT{
			{"p", IM_PTR(float, 10)},
			{"i", IM_PTR(float, 1)},
			{"d", IM_PTR(float, 0)},
			{"maxI", IM_PTR(float, 10000)},
			{"maxOut", IM_PTR(float, 20000)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	{"triggerMotor", CF_DICT{
		{"type", "M2006"},
		{"id", IM_PTR(uint16_t, 6)},
		{"name", "triggerMotor"},
		{"canX", IM_PTR(uint8_t, 1)},
		{"anglePID", CF_DICT{                  //串级pid
			{"inner", CF_DICT{								//内环pid参数设置
				{"p", IM_PTR(float, 10)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 10000)},
				{"maxOut", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			{"outer", CF_DICT{								//外环pid参数设置
				{"p", IM_PTR(float, 0.3)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"maxI", IM_PTR(float, 2000)},
				{"maxOut", IM_PTR(float, 200)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},	
	CF_DICT_END		
}},
```

## 模块接口

> 注：name重映射只需要在配置表中配置名写入原本name字符串，在配置值处写入重映射后的name字符串，就完成了name的重映射。例如：`{"old-name", "new-name"},`

- 广播：无

- 远程函数
  
    1. `/shooter/setting`

        说明：设置拨弹电机的一些属性

        **是否允许name重映射：允许**

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `fric-speed`    | `float` | × | 可选 | 设置摩擦轮转速(单位：rpm) |
        | `trigger-angle` | `float` | × | 可选 | 设置拨一发弹丸旋转的角度(单位：°) |
        | `fric-enable`   | `bool`  | × | 可选 | 使能摩擦轮 |
    
    2. `/shooter/mode`

        说明：修改发射机构运行模式

		`once`：单发弹丸 

		`continue`：连发弹丸直到修改模式为idle才停止
		
		`idle`：停止发射弹丸

        **是否允许name重映射：允许**

        传入参数数据：

        | 数据字段名 | 数据类型 | 是否为返回值 | 是否必须传输 | 说明 |
        | :---: | :---: | :---: | :---: | :---: |
        | `mode`         | `char*`    | × | 必须 | 设置拨弹模式(`"once","continue","idle"`) |
        | `intervalTime` | `uint16_t` | × | 可选 | 仅在连发时需要设置，表示连发弹丸时两次发射的间隔时间 |
