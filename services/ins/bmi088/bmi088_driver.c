#include "bmi088_driver.h"
#include "BMI088reg.h"
#include "softbus.h"
#include "cmsis_os.h"

#define spiWriteAddr(addr) ((addr)&0x7f)
#define spiReadAddr(addr) ((addr)|0x80)

#define BMI088_ACCEL_SEN BMI088_ACCEL_6G_SEN;
#define BMI088_GYRO_SEN  BMI088_GYRO_2000_SEN;

//���ٶȼơ���������λ����(ֻд2���ֽ�Ŀǰ���bug������3���ֽھ�û�����ˣ�Ŀǰû�ҵ�ԭ�����Ҵ�3���ֽ�)
uint8_t accSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
uint8_t gyroSoftResetCmd[] = {spiWriteAddr(BMI088_ACC_SOFTRESET), BMI088_ACC_SOFTRESET_VALUE, 0};
//���ٶȼơ������Ƕ�id����
uint8_t accIdCmd[] = {spiReadAddr(BMI088_ACC_CHIP_ID), 0, 0};
uint8_t gyroIdCmd[] = {spiReadAddr(BMI088_GYRO_CHIP_ID), 0};

//���ٶȼ���������
uint8_t accPwrCtrlCmd[]={spiWriteAddr(BMI088_ACC_PWR_CTRL), BMI088_ACC_ENABLE_ACC_ON};
uint8_t accPwrConfCmd[]={spiWriteAddr(BMI088_ACC_PWR_CONF), BMI088_ACC_PWR_ACTIVE_MODE};
uint8_t accConfCmd[]={spiWriteAddr(BMI088_ACC_CONF),  BMI088_ACC_NORMAL| BMI088_ACC_800_HZ | BMI088_ACC_CONF_MUST_Set};
uint8_t accRangeCmd[]={spiWriteAddr(BMI088_ACC_RANGE), BMI088_ACC_RANGE_6G};
uint8_t accIOConfCmd[]={spiWriteAddr(BMI088_INT1_IO_CTRL), BMI088_ACC_INT1_IO_ENABLE | BMI088_ACC_INT1_GPIO_PP | BMI088_ACC_INT1_GPIO_LOW};
uint8_t accIOMapCmd[]={spiWriteAddr(BMI088_INT_MAP_DATA), BMI088_ACC_INT1_DRDY_INTERRUPT};

//��������������
uint8_t gyroRangeCmd[]={spiWriteAddr(BMI088_GYRO_RANGE), BMI088_GYRO_2000};
uint8_t gyroBandWidthCmd[]={spiWriteAddr(BMI088_GYRO_BANDWIDTH), BMI088_GYRO_1000_116_HZ | BMI088_GYRO_BANDWIDTH_MUST_Set};
uint8_t gyroPwrConfCmd[]={spiWriteAddr(BMI088_GYRO_LPM1), BMI088_GYRO_NORMAL_MODE};
uint8_t gyroPwrCtrlCmd[]={spiWriteAddr(BMI088_GYRO_CTRL), BMI088_DRDY_ON};
uint8_t gyroIOConfCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_CONF), BMI088_GYRO_INT3_GPIO_PP | BMI088_GYRO_INT3_GPIO_LOW};
uint8_t gyroIOMapCmd[]={spiWriteAddr(BMI088_GYRO_INT3_INT4_IO_MAP), BMI088_GYRO_DRDY_IO_INT3};

//���ٶȼ������ǻ�ȡ��������
uint8_t accGetDataCmd[8]={spiReadAddr(BMI088_ACCEL_XOUT_L)};
uint8_t gyroGetDataCmd[7]={spiReadAddr(BMI088_GYRO_X_L)};
uint8_t tmpGetDataCmd[4]={spiReadAddr(BMI088_TEMP_M)};


bool BMI088_AccelInit(uint8_t spiX)
{
	uint8_t accId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //�����ٶȼ�id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���ý���ģʽ
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //�򿪼��ٶȼƵ�Դ
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIdCmd}, 
	                              {"rxData", accId}, 
	                              {"len", IM_PTR(uint16_t, 3)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //�����ٶȼ�id
	osDelay(1);

	if(accId[2] != BMI088_ACC_CHIP_ID_VALUE) //������ٶȼ�id����
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"txData", accSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"csName", "acc"}, 
		                              {"isBlock", IM_PTR(bool, true)}}); //��λ
		osDelay(50);
		return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���ü��ٶȼ��������800Hz, Normal���ģʽ
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���ü��ٶȼ�����+-3g
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���ý�������ģʽ
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accPwrCtrlCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); 
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	return false;
}

bool BMI088_GyroInit(uint8_t spiX)
{
	uint8_t gyroId[3]= {0};
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIdCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //��������id
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIdCmd}, 
	                              {"rxData", gyroId}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //��������id
	osDelay(1);

	if(gyroId[1] != BMI088_GYRO_CHIP_ID_VALUE) //���������id����
	{
		Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
		                              {"txData", gyroSoftResetCmd}, 
		                              {"len", IM_PTR(uint16_t, 3)}, 
		                              {"timeout", IM_PTR(uint32_t, 1000)}, 
		                              {"csName", "gyro"}, 
		                              {"isBlock", IM_PTR(bool, true)}}); //��λ
		osDelay(50);
		return true;
	}

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroBandWidthCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //�����������������1000Hz, ����116Hz
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroRangeCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���������������Χ+-2000��/s
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroPwrConfCmd},
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //���ý���ģʽ
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIOConfCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroIOMapCmd}, 
	                              {"len", IM_PTR(uint16_t, 2)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //
	osDelay(1);
	return false;
}

void BMI088_ReadData(uint8_t spiX, float gyro[3], float accel[3], float *temperate)
{
	uint8_t buf[8] = {0};
	int16_t temp;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", accGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 8)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //��ȡ���ٶȼ�����
	temp = (int16_t)((buf[3]) << 8) | buf[2];
	accel[0] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[5]) << 8) | buf[4];
	accel[1] = temp * BMI088_ACCEL_SEN;
	temp = (int16_t)((buf[7]) << 8) | buf[6];
	accel[2] = temp * BMI088_ACCEL_SEN;

	Bus_RemoteCall("/spi/block", {{"spi-x", &spiX}, 
	                              {"txData", gyroGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 7)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "gyro"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //��ȡ����������
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
	                              {"txData", tmpGetDataCmd}, 
	                              {"rxData", buf}, 
	                              {"len", IM_PTR(uint16_t, 4)}, 
	                              {"timeout", IM_PTR(uint32_t, 1000)}, 
	                              {"csName", "acc"}, 
	                              {"isBlock", IM_PTR(bool, true)}}); //��ȡ����������
	temp = (int16_t)((buf[2] << 3) | (buf[3] >> 5));

	if (temp > 1023)
	{
		temp -= 2048;
	}

	*temperate = temp * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET;
}
