#ifndef _SYSCONF_H_
#define _SYSCONF_H_

#include "config.h"

//<<< Use Configuration Wizard in Context Menu >>>
//<h>BSP Config
//<q0>CAN
//<i>Select to include "can.h"
//<q1>UART
//<i>Select to include "usart.h"
//<q2>EXTI
//<i>Select to include "gpio.h"
//<q3>TIM
//<i>Select to include "tim.h"
//<q4>SPI
//<i>Select to include "spi.h"
#define CONF_CAN_ENABLE		1
#define CONF_USART_ENABLE	1
#define CONF_EXTI_ENABLE 1
#define CONF_TIM_ENABLE 1
#define CONF_SPI_ENABLE 1
//</h>
#if CONF_CAN_ENABLE
#include "can.h"
#endif
#if CONF_USART_ENABLE
#include "usart.h"
#endif
#if CONF_EXTI_ENABLE
#include "gpio.h"
#endif
#if CONF_TIM_ENABLE
#include "tim.h"
#endif
#if CONF_SPI_ENABLE
#include "spi.h"
#endif
// <<< end of configuration section >>>

//���������б�ÿ���ʽ(������,����������,�������ȼ�,����ջ��С)
#define SERVICE_LIST \
	SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime,128) \
	SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal,128) \
	SERVICE(spi, BSP_SPI_TaskCallback, osPriorityNormal,512) \
	SERVICE(tim, BSP_TIM_TaskCallback, osPriorityNormal,256)	\
	SERVICE(ins, INS_TaskCallback, osPriorityNormal,128)\
	SERVICE(gimbal, Gimbal_TaskCallback, osPriorityNormal,256)\
	SERVICE(shooter, Shooter_TaskCallback, osPriorityNormal,256)\
	SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal,256) \
	SERVICE(rc, RC_TaskCallback, osPriorityNormal,256)\
	SERVICE(judge, Judge_TaskCallback, osPriorityNormal,128) \
	SERVICE(sys, SYS_CTRL_TaskCallback, osPriorityNormal,256)
	
//	SERVICE(exti, BSP_EXTI_TaskCallback, osPriorityNormal,256) 

	
//������������
ConfItem* systemConfig = CF_DICT{
	{"sys",CF_DICT{
		{"rotate-pid",CF_DICT{
			{"p", IM_PTR(float, 1.5)},
			{"i", IM_PTR(float, 0)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 100)},
			{"max-out", IM_PTR(float, 200)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	//���̷�������
	{"chassis", CF_DICT{
		//����ѭ������
		{"task-interval", IM_PTR(uint16_t, 2)},
		//���̳ߴ���Ϣ
		{"info", CF_DICT{
			{"wheelbase", IM_PTR(float, 100)},
			{"wheeltrack", IM_PTR(float, 100)},
			{"wheel-radius", IM_PTR(float, 76)},
			{"offset-x", IM_PTR(float, 0)},
			{"offset-y", IM_PTR(float, 0)},
			CF_DICT_END
		}},
		//�����ƶ��ٶ�/���ٶ�����
		{"move", CF_DICT{
			{"max-vx", IM_PTR(float, 2000)},
			{"max-vy", IM_PTR(float, 2000)},
			{"max-vw", IM_PTR(float, 180)},
			{"x-acc", IM_PTR(float, 1000)},
			{"y-acc", IM_PTR(float, 1000)},
			CF_DICT_END
		}},
		//�ĸ��������
		{"motor-fl", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 1)},
			{"can-x", IM_PTR(uint8_t, 1)},
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
		{"motor-fr", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 2)},
			{"can-x", IM_PTR(uint8_t, 1)},
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
		{"motor-bl", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 3)},
			{"can-x", IM_PTR(uint8_t, 1)},
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
		{"motor-br", CF_DICT{
			{"type", "M3508"},
			{"id", IM_PTR(uint16_t, 4)},
			{"can-x", IM_PTR(uint8_t, 1)},
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
		CF_DICT_END
	}},
	//��̨��������
	{"gimbal", CF_DICT{
		//yaw pitch ��е���
		{"zero-yaw",IM_PTR(uint16_t,4010)},
		{"zero-pitch",IM_PTR(uint16_t,5300)},
		//����ѭ������
		{"task-interval", IM_PTR(uint16_t, 10)},
		//��̨�������
		{"motor-yaw", CF_DICT{
			{"type", "M6020"},
			{"id", IM_PTR(uint16_t, 1)},
			{"can-x", IM_PTR(uint8_t, 1)},
			{"imu",CF_DICT{								//������pid��������
				{"p", IM_PTR(float, -90)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 500)},
				{"max-out", IM_PTR(float, 1000)},
				CF_DICT_END
			}},
			{"speed-pid", CF_DICT{						//���������Ƕ��⻷���ִ���ٶ��ڻ�
				{"p", IM_PTR(float, 15)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 500)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},			
		{"motor-pitch", CF_DICT{
			{"type", "M6020"},
			{"id", IM_PTR(uint16_t, 4)},
			{"can-x", IM_PTR(uint8_t, 2)},
			{"imu",CF_DICT{								//������pid��������
				{"p", IM_PTR(float, 63)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 10000)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
				}},
			{"speed-pid", CF_DICT{						//���������Ƕ��⻷���ִ���ٶ��ڻ�
				{"p", IM_PTR(float, 15)},
				{"i", IM_PTR(float, 0)},
				{"d", IM_PTR(float, 0)},
				{"max-i", IM_PTR(float, 500)},
				{"max-out", IM_PTR(float, 20000)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},	
		CF_DICT_END		
	}},
	//�����������
	{"shooter", CF_DICT{
		//����ѭ������
		{"task-interval", IM_PTR(uint16_t, 10)},
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
	//CAN��������
	{"can", CF_DICT{
		//CAN��������Ϣ
		{"cans", CF_DICT{
			{"0", CF_DICT{
				{"hcan", &hcan1},
				{"number", IM_PTR(uint8_t, 1)},
				CF_DICT_END
			}},
			{"1", CF_DICT{
				{"hcan", &hcan2},
				{"number", IM_PTR(uint8_t, 2)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		//��ʱ֡����
		{"repeat-buffers", CF_DICT{
			{"0", CF_DICT{
				{"can-x", IM_PTR(uint8_t, 1)},
				{"id", IM_PTR(uint16_t, 0x200)},
				{"interval", IM_PTR(uint16_t, 2)},
				CF_DICT_END
			}},
			{"1",CF_DICT{
				{"can-x", IM_PTR(uint8_t, 1)},
				{"id", IM_PTR(uint16_t, 0x1FF)},
				{"interval", IM_PTR(uint16_t, 2)},          
				CF_DICT_END
			}},
			{"2",CF_DICT{
				{"can-x", IM_PTR(uint8_t, 2)},
				{"id", IM_PTR(uint16_t, 0x200)},
				{"interval", IM_PTR(uint16_t, 2)},          
				CF_DICT_END
			}},		
			{"3", CF_DICT{
				{"can-x", IM_PTR(uint8_t, 2)},
				{"id", IM_PTR(uint16_t, 0x1FF)},
				{"interval", IM_PTR(uint16_t, 2)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	//ң�ط�������
	{"rc",CF_DICT{
		{"uart-x",IM_PTR(uint8_t, 3)},
		CF_DICT_END
	}},
	//����ϵͳ��������
	{"judge",CF_DICT{
		{"max-tx-queue-length",IM_PTR(uint16_t,20)},
		{"task-interval",IM_PTR(uint16_t,150)},
		{"uart-x",IM_PTR(uint8_t,6)},
		CF_DICT_END
	}},
	//���ڷ�������
	{"uart",CF_DICT{
		{"uarts",CF_DICT{
			{"0",CF_DICT{
				{"huart",&huart3},
				{"number",IM_PTR(uint8_t,3)},
				{"max-recv-size",IM_PTR(uint16_t,18)},
				CF_DICT_END
			}},
			{"1",CF_DICT{
				{"huart",&huart6},
				{"number",IM_PTR(uint8_t,6)},
				{"max-recv-size",IM_PTR(uint16_t,300)},
				CF_DICT_END
			}},
			CF_DICT_END
		}},	
		CF_DICT_END
	}},
	{"ins",CF_DICT{
		{"spi-x",IM_PTR(uint8_t,1)},
		{"tim-x",IM_PTR(uint8_t,10)},
		{"channel-x",IM_PTR(uint8_t,1)},
		{"tmp-pid", CF_DICT{
			{"p", IM_PTR(float, 0.15)},
			{"i", IM_PTR(float, 0.01)},
			{"d", IM_PTR(float, 0)},
			{"max-i", IM_PTR(float, 0.15)},
			{"max-out", IM_PTR(float, 1)},
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	{"spi",CF_DICT{
		{"spis",CF_DICT{
			{"0",CF_DICT{
				{"hspi", &hspi1},
				{"number",IM_PTR(uint8_t,1)},
				{"max-recv-size",IM_PTR(uint8_t,10)},
				{"cs",CF_DICT{
					{"0",CF_DICT{
						{"pin", IM_PTR(uint8_t,0)},
						{"name", "gyro"},
						{"gpio-x", GPIOB},
						CF_DICT_END
					}},
					{"1",CF_DICT{
						{"pin", IM_PTR(uint8_t,4)},
						{"name", "acc"},
						{"gpio-x", GPIOA},
						CF_DICT_END
					}},          
					CF_DICT_END
					}},
				CF_DICT_END
				}},
			CF_DICT_END
			}},
		CF_DICT_END
	}},
	//��ʱ����������
	{"tim",CF_DICT{
		{"tims",CF_DICT{
			{"0",CF_DICT{
				{"htim",&htim10},
				{"number",IM_PTR(uint8_t,10)},
				{"mode","pwm"},
				CF_DICT_END
			}}, 
			CF_DICT_END
		}},
		CF_DICT_END
	}},
	//	//�ⲿ�жϷ�������
//	{"exti",CF_DICT{
//		{"extis",CF_DICT{
//			{"0",CF_DICT{
//				{"gpio-x", GPIOA},
//				{"pin-x",IM_PTR(uint8_t,0)},
//				CF_DICT_END
//			}},
//			{"1",CF_DICT{
//				{"gpio-x", GPIOA},
//				{"pin-x",IM_PTR(uint8_t,4)},
//				CF_DICT_END
//			}},
//			CF_DICT_END
//		}},
//		CF_DICT_END
//	}},
	CF_DICT_END
};

#endif

