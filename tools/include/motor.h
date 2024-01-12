#ifndef _MOTOR_H_
#define _MOTOR_H_
#include "cmsis_os.h"
#include "config.h"
#include "pid.h"
#include "softbus.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MOTOR_MALLOC_PORT(len) pvPortMalloc(len)
#define MOTOR_FREE_PORT(ptr) vPortFree(ptr)
// 模式
typedef enum __packed
{
    ModeTorque,
    ModeSpeed,
    ModeAngle,
    ModeStop
} MotorCtrlMode;

// （可获取的）数据类型
enum MotorDataType
{
    // 电机当前的角度反馈值
    CurrentAngle,

    // 电机反馈角度的累积值
    TotalAngle,

    // 电机反馈角度的原始数值
    RawAngle,

    // 电机反馈的速度值
    Speed,

    // 电机反馈的速度值，经过某减速比换算后的值。仅对减速电机有意义
    SpeedReduced,

    // 电机配置的“方向”参数，1为正转，-1为反转。
    Direction,

    // 电机自主进行角度环控制时的速度上限（在DamiaoJ6006中加入）
    SpeedLimit,
};

class BasicMotor
{
  public:
    BasicMotor() : m_mode(ModeStop) { m_feedbackValid = false; }
    virtual void Init(ConfItem *conf) = 0;
    virtual bool SetMode(MotorCtrlMode mode) = 0;
    virtual void EmergencyStop() = 0;

    virtual bool SetTarget(float target) = 0;
    virtual bool SetData(MotorDataType type, float angle) = 0;
    virtual float GetData(MotorDataType type);

    /**
     * @brief 等待直到电机至少获得了一次反馈，内部每至多10ms轮询一次。对于开环控制的电机，
     *        实现中应在构造函数将m_feedbackValid置true。
     *
     * @param ms 等待毫秒数。会使用osDelay等待。如果为-1（默认值），则会永久等待下去。
     * @return 是否等到了电机反馈。
     */
    bool WaitUntilFeedbackValid(int32_t ms = 0)
    {
        while (true)
        {
            int32_t delayDuration = (ms > 10 || ms < 0) ? 10 : ms;
            if (m_feedbackValid)
                return true;
            else if (ms == 0)
                return false;
            else
                ms -= delayDuration;
            osDelay(delayDuration);
        }
    }

    static BasicMotor *Create(ConfItem *conf);

  protected:
    MotorCtrlMode m_mode;

    bool m_feedbackValid;
    int8_t m_direction; // 电机转动方向，为1或-1，乘在输出值上
};

class CanMotor : public BasicMotor
{
  public:
    CanMotor() : BasicMotor(){};

  protected:
    struct
    {
        uint16_t txID;    // 向电机发送报文时使用的ID
        uint16_t rxID;    // 电机向MCU反馈报文的ID
        uint8_t canX;     // CAN设备号
        uint8_t bufIndex; // TX方向循环缓冲区偏移量
    } m_canInfo;

    void RegisterCanMotor(const char *name); // 将电机注册到Manager
};

class DummyMotor : public BasicMotor
{
  public:
    DummyMotor() : BasicMotor(){};
    virtual void Init(ConfItem *conf) override{};
    virtual bool SetMode(MotorCtrlMode mode) override { return false; }
    virtual void EmergencyStop() override{};

    virtual bool SetTarget(float target) override { return false; }
    virtual bool SetData(MotorDataType type, float angle) override { return false; }
    virtual float GetData(MotorDataType type) override { return 0.0f; }
};

class DjiCanMotor : public CanMotor
{
  public:
    DjiCanMotor() : CanMotor(){};
    virtual void Init(ConfItem *conf) override;
    virtual bool SetMode(MotorCtrlMode mode) override;
    virtual void EmergencyStop() override;

    virtual bool SetTarget(float target) override;
    virtual bool SetData(MotorDataType type, float angle) override;
    virtual float GetData(MotorDataType type) override;

    // 各种电机编码值与角度的换算
    // 减速比*8191/360，适用于当前体系下所有大疆的RM电机
    static inline int32_t DegToCode(float deg, float reductionRate)
    {
        return (int32_t)(deg * 22.7528f * reductionRate);
    }
    static inline float CodeToDeg(int32_t code, float reductionRate)
    {
        return (float)(code / (22.7528f * reductionRate));
    }

  protected:
    // CAN接收中断回调
    static void CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData);
    // 急停回调
    static void EmergencyStopCallback(const char *endpoint, SoftBusFrame *frame, void *bindData);
    // 定时器回调
    static void TimerCallback(const void *argument);

    // CAN发送函数
    virtual void CanTransmit(uint16_t output) = 0;

    PID m_speedPid;        // 单级速度PID
    CascadePID m_anglePid; // 串级（内速度外角度）PID

    // （CAN回调时接收的）电机参数
    int16_t m_motorAngle, m_motorSpeed, m_motorTorque;
    uint8_t m_motorTemperature;

    // （CAN回调时计算的）电机转速RPM
    float m_motorReducedRpm;

    float m_target; // 目标值（视模式可以为速度(RPM)、角度（单位：度）或扭矩）
    float m_reductionRatio; // 减速比

    SoftBusReceiverHandle m_stallTopic; // 堵转事件话题句柄
    uint16_t m_stallTime;               // 堵转时间

    uint16_t m_lastAngle; // 上一计算帧的角度（编码器值）
    int32_t m_totalAngle; // 电机累积转过的角度（编码器值）

    // 以CAN接收数据更新成员变量
    virtual void CanRxUpdate(uint8_t *rxData) = 0;
    // 统计角度（定时器回调调用）
    void UpdateAngle();
    // 控制器执行一次运算（PID等）
    void ControllerUpdate(float target);
    // 堵转检测逻辑
    void StallDetection();
};

class M3508 final : public DjiCanMotor
{
  public:
    M3508() : DjiCanMotor(){};
    virtual void Init(ConfItem *conf) override;

  private:
    virtual void CanTransmit(uint16_t output) override;
    virtual void CanRxUpdate(uint8_t *rxData) override;
};

class M6020 final : public DjiCanMotor
{
  public:
    M6020() : DjiCanMotor(){};
    virtual void Init(ConfItem *conf) override;
    // 6020没有电流反馈，拒绝使用扭矩模式
    virtual bool SetMode(MotorCtrlMode mode) override;

  private:
    virtual void CanTransmit(uint16_t output) override;
    virtual void CanRxUpdate(uint8_t *rxData) override;
};

class M2006 final : public DjiCanMotor
{
  public:
    M2006() : DjiCanMotor(){};
    virtual void Init(ConfItem *conf) override;

  private:
    virtual void CanTransmit(uint16_t output) override;
    virtual void CanRxUpdate(uint8_t *rxData) override;
};

class DamiaoJ6006 : public CanMotor
{
  public:
    virtual void Init(ConfItem *conf) override;
    virtual bool SetMode(MotorCtrlMode mode) override;
    virtual void EmergencyStop() override;

    virtual bool SetTarget(float target) override;
    virtual bool SetData(MotorDataType type, float angle) override;
    virtual float GetData(MotorDataType type) override;

  protected:
    // 编码器与角度换算
    // *16384/360
    static inline int32_t DegToCode(float deg) { return (int32_t)(deg * 45.5111f); }
    static inline float CodeToDeg(int32_t code) { return (float)(code / 45.5111f); }
    // CAN接收中断回调
    static void CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData);
    // 急停回调
    static void EmergencyStopCallback(const char *endpoint, SoftBusFrame *frame, void *bindData);

    // 电机自主角度环时的速度上限
    float m_autonomousAngleModeSpeedLimit;
    // 累积角度值（角度）
    float m_totalAngle;
    // 目标值
    float m_target;
    // 角度(16位)、速度(12位)、扭矩(12位)
    uint16_t m_motorAngle, m_motorSpeed, m_motorTorque;
    // 驱动温度、线圈温度
    uint8_t m_motorDriverTemp, m_motorCoilTemp;

    // 定时器
    osTimerId m_timer;

  private:
    // 尝试解除错误状态
    // 因为常见的错误状态是电机过热，所以这里只尝试解除过热错误，温度阈值为60度
    void AttemptResolveError();

    // 下面的指令属于控制指令
    // 对电机发送使能/失能指令
    void CommandEnable(bool enable);

    // 保存位置零点
    void CommandSetZero();

    // 清除错误标志
    void CommandClearErrorFlag();

    // 下面的三个命令方式用的是UIML规定的单位制，发送函数封包的时候再转成达妙的弧度或者映射单位
    // 向MIT模式发送命令，目前只用在了扭矩模式
    void CommandMitMode(uint16_t targetAngle,
                        uint16_t targetSpeed,
                        uint16_t kp,
                        uint16_t kd,
                        uint16_t targetTorque);

    // 向角度模式发送命令
    void CommandAngleMode(float targetAngle, float targetSpeed);

    // 向速度模式发送命令
    void CommandSpeedMode(float targetSpeed);

    // 发送CAN报文
    void CanTransmitFrame(uint16_t txId, uint8_t length, uint8_t *data);
};

class DcMotor : public BasicMotor
{
  public:
    DcMotor() : BasicMotor(){};
    virtual void Init(ConfItem *conf);
    virtual bool SetMode(MotorCtrlMode mode);
    virtual void EmergencyStop();

    virtual bool SetTarget(float target);
    virtual bool SetData(MotorDataType type, float angle);
    virtual float GetData(MotorDataType type);

    // 编码值与角度的换算（减速比*编码器一圈值/360）
    static inline uint32_t Degree2Code(float deg, float reductionRatio, float circleEncode)
    {
        return deg * reductionRatio * circleEncode / 360.0f;
    }
    static inline float Code2Degree(uint32_t code, float reductionRatio, float circleEncode)
    {
        return code / (reductionRatio * circleEncode / 360.0f);
    }

  protected:
    PID m_speedPID;        // 单级速度PID
    CascadePID m_anglePID; // 串级（内速度外角度）PID

    float m_reductionRatio; // 减速比
    float m_circleEncode;   // 倍频后编码器转一圈的最大值
    float m_target;         // 目标值（视模式可以为速度、角度（单位：度)）

    uint16_t m_lastAngle;  // 上一计算帧的角度（编码器值）
    uint16_t m_totalAngle; // 电机累积转过的角度（编码器值）

    uint16_t m_angle, m_speed;

    struct TimInfo
    {
        uint8_t timX;
        uint8_t channelX;
    } m_posRotateTim, m_negRotateTim, m_encodeTim;

    // 定时器回调
    static void TimerCallback(const void *argument);

    // 统计角度（定时器回调调用）
    void UpdateAngle();

    // 控制器执行一次运算（PID等）
    void ControllerUpdate(float target);

  private:
    void TimerInit(ConfItem *dict); // 定时器初始化
};

class Servo : public BasicMotor
{
  public:
    Servo() : BasicMotor(){};
    virtual void Init(ConfItem *conf);
    virtual bool SetMode(MotorCtrlMode mode) { return false; }
    virtual void EmergencyStop() {}

    virtual bool SetTarget(float target);
    virtual bool SetData(MotorDataType type, float angle) { return false; }
    virtual float GetData(MotorDataType type) { return 0.0f; }

  protected:
    struct TimInfo
    {
        uint8_t timX;
        uint8_t channelX;
    } m_timInfo;
    float m_maxAngle;
    float m_maxDuty, m_minDuty;
    float m_target;
};

#endif
