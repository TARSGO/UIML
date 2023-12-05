
#pragma once

#include "config.h"
#include "pid.h"

class BasicImu
{
  public:
    BasicImu(){};

    virtual void Init(ConfItem *conf) = 0;
    virtual bool Acquire() = 0;

    virtual void GetAccelerometerData(float &ax, float &ay, float &az) = 0;
    virtual void GetGyroscopeData(float &vx, float &vy, float &vz) = 0;
    virtual float GetTemperature() = 0;

    virtual void TemperatureControlTick() = 0;

    static BasicImu *Create(ConfItem *conf);
};

class DummyImu final : public BasicImu
{
  public:
    DummyImu() : BasicImu() {}

    virtual void Init(ConfItem *conf) override {}
    virtual bool Acquire() override { return false; }

    virtual void GetAccelerometerData(float &ax, float &ay, float &az) override {}
    virtual void GetGyroscopeData(float &vx, float &vy, float &vz) override {}
    virtual float GetTemperature() override { return 25.0f; }

    virtual void TemperatureControlTick() override {}
};

class Bmi088 final : public BasicImu
{
  public:
    Bmi088() : BasicImu() {}

    virtual void Init(ConfItem *conf) override;
    virtual bool Acquire() override;

    virtual void GetAccelerometerData(float &ax, float &ay, float &az) override;
    virtual void GetGyroscopeData(float &vx, float &vy, float &vz) override;
    virtual float GetTemperature() override;

    virtual void TemperatureControlTick() override;

  private:
    enum ChipSelect
    {
        Accelerometer = 0,
        Gyroscope = 1
    };

    // 发送SPI命令。接收会自动存入内置缓冲区。
    void DeviceIssueCommandSpi(uint8_t *TxBuffer, size_t TxLength, ChipSelect Cs);

    // 启动BMI088。只应该由Init函数调用一次。
    void DeviceStartup();

    // 启动加速度计/陀螺仪。只应该由DeviceStartup调用。返回值表示是否成功。
    bool DeviceStartupAccelerometer();
    bool DeviceStartupGyroscope();

  private:
    uint8_t m_spiX; // BMI088连接的SPI设备号

    uint8_t m_timX;     // 恒温加热TIM设备号
    uint8_t m_channelX; // 恒温加热TIM设备输出通道
    float m_targetTemp; // 恒温加热目标温度值（摄氏度）
    PID m_heatingPid;   // 恒温加热PID

    uint8_t m_spiTxBuffer[8],
        m_spiRxBuffer[8]; // SPI通信接收缓冲区（需要一次读的最大数据就是8字节）
    float m_ax, m_ay, m_az; // 加速度计
    float m_vx, m_vy, m_vz; // 陀螺仪（角速度计）
    float m_temperature;    // 温度
};
