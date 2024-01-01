
/****************** 服务列表配置 (X-MACRO) ******************/

// 每项格式(服务名,服务任务函数,任务优先级,任务栈大小)
#define SERVICE_LIST                                                                         \
  /* VVVVV 系统 BSP 服务，请不要在此处禁用，而应在 CMake 中配置 VVVVV */ \
  SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime, 512)                                \
  SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal, 1024)                               \
  SERVICE(spi, BSP_SPI_TaskCallback, osPriorityNormal, 512)                                  \
  SERVICEX(tim, BSP_TIM_TaskCallback, osPriorityNormal, 512)                                 \
  SERVICEX(exti, BSP_EXTI_TaskCallback, osPriorityNormal, 256)                               \
  /* ^^^^^                                                 ^^^^^ */                          \
  SERVICEX(beep, Beep_TaskMain, osPriorityNormal, 256)                                       \
  SERVICE(referee, Referee_TaskMain, osPriorityNormal, 512)                                  \
  SERVICE(ins, INS_TaskCallback, osPriorityNormal, 1024)                                     \
  SERVICE(gimbal, Gimbal_TaskCallback, osPriorityNormal, 256)                                \
  SERVICEX(shooter, Shooter_TaskCallback, osPriorityNormal, 256)                             \
  SERVICE(chassis, Chassis_TaskCallback, osPriorityNormal, 1024)                             \
  SERVICE(rc, RC_TaskCallback, osPriorityNormal, 1024)                                       \
  SERVICEX(judge, Judge_TaskCallback, osPriorityNormal, 128)                                 \
  SERVICE(sys, SYS_CTRL_TaskCallback, osPriorityNormal, 256)                                 \
  SERVICE(rtt, RTT_TaskCallback, osPriorityLow, 128)                                         \
  SERVICE(shell, Shell_TaskCallback, osPriorityLow, 512)
