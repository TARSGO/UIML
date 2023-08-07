#include "bmi088_driver.h"
#include "BMI088reg.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "spi.h"
#include "stm32f4xx_hal_spi.h"
#include <string.h>

#define spiWriteAddr(addr) ((addr) & (~0x80))
#define spiReadAddr(addr) ((addr) | 0x80)

#define BMI088_ACCEL_SEN BMI088_ACCEL_6G_SEN;
#define BMI088_GYRO_SEN  BMI088_GYRO_2000_SEN;

//加速度计、陀螺仪软复位命令(只写2个字节目前测出bug，但传3个字节就没问题了，目前没找到原因暂且传3个字节)
uint8_t accSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
uint8_t gyroSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
//加速度计、陀螺仪读id命令
uint8_t accIdCmd[] = {spiReadAddr(BMI088_ACC_CHIP_ID), 0, 0};
uint8_t gyroIdCmd[] = {spiReadAddr(BMI088_GYRO_CHIP_ID), 0};

//加速度计配置命令
uint8_t accPwrCtrlCmd[]={spiWriteAddr(BMI088_ACC_PWR_CTRL), BMI088_ACC_ENABLE_ACC_ON};
uint8_t accPwrConfCmd[]={spiWriteAddr(BMI088_ACC_PWR_CONF), BMI088_ACC_PWR_ACTIVE_MODE};
uint8_t accConfCmd[]={spiWriteAddr(BMI088_ACC_CONF),  BMI088_ACC_NORMAL| BMI088_ACC_800_HZ | BMI088_ACC_CONF_MUST_Set};
uint8_t accRangeCmd[]={spiWriteAddr(BMI088_ACC_RANGE), BMI088_ACC_RANGE_6G};
uint8_t accIOConfCmd[]={spiWriteAddr(BMI088_INT1_IO_CTRL), BMI088_ACC_INT1_IO_ENABLE | BMI088_ACC_INT1_GPIO_PP | BMI088_ACC_INT1_GPIO_LOW};
uint8_t accIOMapCmd[]={spiWriteAddr(BMI088_INT_MAP_DATA), BMI088_ACC_INT1_DRDY_INTERRUPT};

//陀螺仪配置命令
uint8_t gyroRangeCmd[]={spiWriteAddr(BMI088_GYRO_RANGE), BMI088_GYRO_2000};
uint8_t gyroBandWidthCmd[]={spiWriteAddr(BMI088_GYRO_BANDWIDTH), BMI088_GYRO_1000_116_HZ | BMI088_GYRO_BANDWIDTH_MUST_Set};
uint8_t gyroPwrConfCmd[]={spiWriteAddr(BMI088_GYRO_LPM1), BMI088_GYRO_NORMAL_MODE};
uint8_t gyroPwrCtrlCmd[]={spiWriteAddr(BMI088_GYRO_CTRL), BMI088_DRDY_ON};
uint8_t gyroIOConfCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_CONF), BMI088_GYRO_INT3_GPIO_PP | BMI088_GYRO_INT3_GPIO_LOW};
uint8_t gyroIOMapCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_MAP), BMI088_GYRO_DRDY_IO_INT3};

//加速度计陀螺仪获取数据命令
uint8_t accGetDataCmd[8]={spiReadAddr(BMI088_ACCEL_XOUT_L)};
uint8_t gyroGetDataCmd[7]={spiReadAddr(BMI088_GYRO_X_L)};
uint8_t tmpGetDataCmd[4]={spiReadAddr(BMI088_TEMP_M)};


bool BMI088_AccelInit(uint8_t spiX)
{
	uint8_t accId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //读加速度计id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置进入活动模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //打开加速度计电源
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accIdCmd}, 
	                              {"rx-data", accId}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //读加速度计id
	osDelay(1);

	if(accId[2] != BMI088_ACC_CHIP_ID_VALUE) //如果加速度计id不对
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"tx-data", accSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"cs-name", "acc"}, 
		                              {"is-block", IM_PTR(bool, true)}}); //软复位
		osDelay(50);
		return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置加速度计输出速率800Hz, Normal输出模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置加速度计量程+-3g
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置进入正常模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}});  //打开加速度计电源
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //
	osDelay(1);
	return false;
}

bool BMI088_GyroInit(uint8_t spiX)
{
	uint8_t gyroId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //读陀螺仪id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroIdCmd}, 
	                              {"rx-data", gyroId}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //读陀螺仪id
	osDelay(1);

	if(gyroId[1] != BMI088_GYRO_CHIP_ID_VALUE) //如果陀螺仪id不对
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"tx-data", gyroSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"cs-name", "gyro"}, 
		                              {"is-block", IM_PTR(bool, true)}}); //软复位
		osDelay(50);
		return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroBandWidthCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置陀螺仪输出速率1000Hz, 带宽116Hz
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置陀螺仪输出范围+-2000°/s
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroPwrConfCmd},
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //配置进入活动模式
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //
	osDelay(1);
	return false;
}

void BMI088_ReadData(uint8_t spiX, float gyro[3], float accel[3], float *temperate)
{
	uint8_t buf[8] = {0};
	int16_t temp;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", accGetDataCmd}, 
	                              {"rx-data", buf}, 
	                              {"len", IM_PTR(uint16_t, 8)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //获取加速度计数据
	temp = (int16_t)((buf[3]) << 8) | buf[2];
	accel[0] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[5]) << 8) | buf[4];
	accel[1] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[7]) << 8) | buf[6];
	accel[2] = temp * BMI088_ACCEL_SEN;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", gyroGetDataCmd}, 
	                              {"rx-data", buf}, 
	                              {"len", IM_PTR(uint16_t, 7)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "gyro"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //获取陀螺仪数据
	// if(buf[0] == BMI088_GYRO_CHIP_ID_VALUE)
	// {
	temp = (int16_t)((buf[2]) << 8) | buf[1];
	gyro[0] = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((buf[4]) << 8) | buf[3];
	gyro[1] = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((buf[6]) << 8) | buf[5];
	gyro[2] = temp * BMI088_GYRO_SEN;
	// }

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"tx-data", tmpGetDataCmd}, 
	                              {"rx-data", buf}, 
	                              {"len", IM_PTR(uint16_t, 4)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"cs-name", "acc"}, 
	                              {"is-block", IM_PTR(bool, true)}}); //获取温度数据
	temp = (int16_t)((buf[2] << 3) | (buf[3] >> 5));

	if (temp > 1023)
	{
		temp -= 2048;
	}

	*temperate = temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
}
