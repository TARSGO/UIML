
#include "gyroscope.h"
#include "bmi088reg.h"
#include "bmi088const.h"
#include <cmsis_os.h>

constexpr uint8_t spiWriteAddr(uint8_t addr) { return addr & (~0x80); }
constexpr uint8_t spiReadAddr(uint8_t addr) { return addr | 0x80; }

#define BMI088_ACCEL_SEN BMI088_ACCEL_6G_SEN;
#define BMI088_GYRO_SEN  BMI088_GYRO_2000_SEN;

//加速度计、陀螺仪软复位命令(只写2个字节目前测出bug，但传3个字节就没问题了，目前没找到原因暂且传3个字节)
static uint8_t accSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
static uint8_t gyroSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
//加速度计、陀螺仪读id命令
static uint8_t accIdCmd[] = {spiReadAddr(BMI088_ACC_CHIP_ID), 0, 0};
static uint8_t gyroIdCmd[] = {spiReadAddr(BMI088_GYRO_CHIP_ID), 0};

//加速度计配置命令
static uint8_t accPwrCtrlCmd[]={spiWriteAddr(BMI088_ACC_PWR_CTRL), BMI088_ACC_ENABLE_ACC_ON};
static uint8_t accPwrConfCmd[]={spiWriteAddr(BMI088_ACC_PWR_CONF), BMI088_ACC_PWR_ACTIVE_MODE};
static uint8_t accConfCmd[]={spiWriteAddr(BMI088_ACC_CONF),  BMI088_ACC_NORMAL| BMI088_ACC_800_HZ | BMI088_ACC_CONF_MUST_Set};
static uint8_t accRangeCmd[]={spiWriteAddr(BMI088_ACC_RANGE), BMI088_ACC_RANGE_6G};
static uint8_t accIOConfCmd[]={spiWriteAddr(BMI088_INT1_IO_CTRL), BMI088_ACC_INT1_IO_ENABLE | BMI088_ACC_INT1_GPIO_PP | BMI088_ACC_INT1_GPIO_LOW};
static uint8_t accIOMapCmd[]={spiWriteAddr(BMI088_INT_MAP_DATA), BMI088_ACC_INT1_DRDY_INTERRUPT};

//陀螺仪配置命令
static uint8_t gyroRangeCmd[]={spiWriteAddr(BMI088_GYRO_RANGE), BMI088_GYRO_2000};
static uint8_t gyroBandWidthCmd[]={spiWriteAddr(BMI088_GYRO_BANDWIDTH), BMI088_GYRO_1000_116_HZ | BMI088_GYRO_BANDWIDTH_MUST_Set};
static uint8_t gyroPwrConfCmd[]={spiWriteAddr(BMI088_GYRO_LPM1), BMI088_GYRO_NORMAL_MODE};
static uint8_t gyroPwrCtrlCmd[]={spiWriteAddr(BMI088_GYRO_CTRL), BMI088_DRDY_ON};
static uint8_t gyroIOConfCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_CONF), BMI088_GYRO_INT3_GPIO_PP | BMI088_GYRO_INT3_GPIO_LOW};
static uint8_t gyroIOMapCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_MAP), BMI088_GYRO_DRDY_IO_INT3};

//加速度计陀螺仪获取数据命令
static uint8_t accGetDataCmd[8]={spiReadAddr(BMI088_ACCEL_XOUT_L)};
static uint8_t gyroGetDataCmd[7]={spiReadAddr(BMI088_GYRO_X_L)};
static uint8_t tmpGetDataCmd[4]={spiReadAddr(BMI088_TEMP_M)};

void Bmi088::Init(ConfItem* conf)
{
	m_spiX = Conf_GetValue(conf, "spi-x", uint8_t, 0);
	m_timX = Conf_GetValue(conf, "tim-x", uint8_t, 10);
	m_targetTemp = Conf_GetValue(conf, "target-temperature", float, 40);
	m_channelX = Conf_GetValue(conf, "channel-x", uint8_t, 1);

    m_heatingPid.Init(Conf_GetPtr(conf, "tmp-pid", ConfItem));
}

void Bmi088::DeviceStartup()
{
	bool result = false;

	do
	{
		result = DeviceStartupAccelerometer();
	} while (!result);

	do
	{
		result = DeviceStartupGyroscope();
	} while (!result);
}

bool Bmi088::DeviceStartupAccelerometer()
{
	// 读加速度计ID
	DeviceIssueCommandSpi(accIdCmd, sizeof(accIdCmd));
	osDelay(1);

	// 配置进入活动模式
	DeviceIssueCommandSpi(accPwrConfCmd, sizeof(accPwrConfCmd));
	osDelay(1);

	// 打开加速度计电源
	DeviceIssueCommandSpi(accPwrCtrlCmd, sizeof(accPwrCtrlCmd));
	osDelay(1);

	// 读加速度计ID
	DeviceIssueCommandSpi(accIdCmd, sizeof(accIdCmd));
	osDelay(1);

	// 检验加速度计的ID
	if (m_spiRxBuffer[2] != BMI088_ACC_CHIP_ID_VALUE)
	{
		// 如不能通过检验则命令软重启
		DeviceIssueCommandSpi(accSoftResetCmd, sizeof(accSoftResetCmd));
		osDelay(50);
		return false;
	}

	// 设置加速度计输出速率
	DeviceIssueCommandSpi(accConfCmd, sizeof(accConfCmd));
	osDelay(1);

	// 设置加速度计量程
	DeviceIssueCommandSpi(accRangeCmd, sizeof(accRangeCmd));
	osDelay(1);

	// 配置进入活动模式
	DeviceIssueCommandSpi(accPwrConfCmd, sizeof(accPwrConfCmd));
	osDelay(1);

	// 打开加速度计电源
	DeviceIssueCommandSpi(accPwrCtrlCmd, sizeof(accPwrCtrlCmd));
	osDelay(1);

	// 设置IO映射
	DeviceIssueCommandSpi(accIOConfCmd, sizeof(accIOConfCmd));
	osDelay(1);

	DeviceIssueCommandSpi(accIOMapCmd, sizeof(accIOMapCmd));
	osDelay(1);

	return true;
}

bool Bmi088::DeviceStartupGyroscope()
{
	//
	DeviceIssueCommandSpi(gyroIdCmd, sizeof(gyroIdCmd));
	osDelay(1);

	//
	DeviceIssueCommandSpi(gyroIdCmd, sizeof(gyroIdCmd));
	osDelay(1);

	//
	if (m_spiRxBuffer[1] != BMI088_GYRO_CHIP_ID_VALUE)
	{
		//
		DeviceIssueCommandSpi(gyroSoftResetCmd, sizeof(gyroSoftResetCmd));
		osDelay(50);
		return false;
	}

	//
	DeviceIssueCommandSpi(gyroBandWidthCmd, sizeof(gyroBandWidthCmd));
	osDelay(1);

	// 
	DeviceIssueCommandSpi(gyroRangeCmd, sizeof(gyroRangeCmd));
	osDelay(1);

	//
	DeviceIssueCommandSpi(gyroPwrConfCmd, sizeof(gyroPwrConfCmd));
	osDelay(1);

	//
	DeviceIssueCommandSpi(gyroPwrCtrlCmd, sizeof(gyroPwrCtrlCmd));
	osDelay(1);

	//
	DeviceIssueCommandSpi(gyroIOConfCmd, sizeof(gyroIOConfCmd));
	osDelay(1);

	//
	DeviceIssueCommandSpi(gyroIOMapCmd, sizeof(gyroIOMapCmd));
	osDelay(1);

	return true;
}

bool Bmi088::Acquire()
{
	int16_t temp;

	// 加速度
	DeviceIssueCommandSpi(accGetDataCmd, sizeof(accGetDataCmd));
	temp = (int16_t)((m_spiRxBuffer[3]) << 8) | m_spiRxBuffer[2];
	m_ax = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((m_spiRxBuffer[5]) << 8) | m_spiRxBuffer[4];
	m_ay = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((m_spiRxBuffer[7]) << 8) | m_spiRxBuffer[6];
	m_az = temp * BMI088_ACCEL_SEN;

	// 陀螺仪
	DeviceIssueCommandSpi(gyroGetDataCmd, sizeof(gyroGetDataCmd));
	temp = (int16_t)((m_spiRxBuffer[2]) << 8) | m_spiRxBuffer[1];
	m_vx = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((m_spiRxBuffer[4]) << 8) | m_spiRxBuffer[3];
	m_vy = temp * BMI088_GYRO_SEN;
	temp = (int16_t)((m_spiRxBuffer[6]) << 8) | m_spiRxBuffer[5];
	m_vz = temp * BMI088_GYRO_SEN;

	// 温度
	DeviceIssueCommandSpi(tmpGetDataCmd, sizeof(tmpGetDataCmd));
	temp = (int16_t)((m_spiRxBuffer[2] << 3) | (m_spiRxBuffer[3] >> 5));
	if (temp > 1023) temp -= 2048;

	m_temperature = temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;

	return true;
}

void Bmi088::GetAccelerometerData(float& ax, float& ay, float& az)
{
	ax = m_ax;
	ay = m_ay;
	az = m_az;
}

void Bmi088::GetGyroscopeData(float& vx, float& vy, float& vz)
{
	vx = m_vx;
	vy = m_vy;
	vz = m_vz;
}

float Bmi088::GetTemperature()
{
	return m_temperature;
}
