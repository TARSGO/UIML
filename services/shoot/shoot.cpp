#include "cmsis_os.h"
#include "config.h"
#include "dependency.h"
#include "extendio.h"
#include "motor.h"
#include "portable.h"
#include "shooterapi.h"
#include "softbus.h"
#include "sys_conf.h"

static constexpr uint32_t SignalTriggerStall = 1;

class Shooter
{
  public:
    Shooter(){};

    void Init(ConfItem *conf)
    {
        U_C(conf);

        threadId = osThreadGetId();
        taskInterval = Conf["task-interval"].get<uint16_t>(20); // 任务间隔
        fricSpeed = Conf["fric-speed"].get(5450.0f);            // 初始弹速

        // 拨弹轮拨出一发弹丸转角
        triggerAngle = Conf["trigger-angle"].get<float>(1 / 7.0 * 360);

        // 发射机构电机初始化
        fricMotors[0] = BasicMotor::Create(Conf["fric-motor-left"]);
        fricMotors[1] = BasicMotor::Create(Conf["fric-motor-right"]);
        triggerMotor = BasicMotor::Create(Conf["trigger-motor"]);

        // 设置摩擦轮速度模式，拨弹轮角度模式
        for (uint8_t i = 0; i < 2; i++)
            fricMotors[i]->SetMode(MOTOR_SPEED_MODE);
        triggerMotor->SetMode(MOTOR_ANGLE_MODE);

        auto shooterName = Conf["name"].get("shooter");
        configFuncName = alloc_sprintf(pvPortMalloc, "/%s/config", shooterName);
        shootFuncName = alloc_sprintf(pvPortMalloc, "/%s/shoot", shooterName);
        toggleFricMotorFuncName = alloc_sprintf(pvPortMalloc, "/%s/toggle-fric", shooterName);

        auto trigMotorName = Conf["trigger-motor"]["name"].get("trigger-motor");
        triggerStallName = alloc_sprintf(pvPortMalloc, "/%s/stall", trigMotorName);

        // 注册回调函数
        Bus_RemoteFuncRegister(this, ConfigFuncCallback, configFuncName);
        Bus_RemoteFuncRegister(this, ToggleFrictionMotorFuncCallback, toggleFricMotorFuncName);
        Bus_RemoteFuncRegister(this, ShootFuncCallback, shootFuncName);
        Bus_SubscribeTopic(this, EmergencyStopCallback, "/system/stop");
        Bus_SubscribeTopic(this, TriggerMotorStallCallback, triggerStallName);
    }

    [[noreturn]] void Exec()
    {
        while (true)
        {
            Tick();
        }
    }

  private:
    inline void Tick();
    inline void ToggleFrictionMotor(bool enabled);

    static BUS_REMOTEFUNC(ConfigFuncCallback); // [远程函数] 更改发射参数回调
    static BUS_REMOTEFUNC(ToggleFrictionMotorFuncCallback); // [远程函数] 开关摩擦轮回调
    static BUS_REMOTEFUNC(ShootFuncCallback);            // [远程函数] 更改发射模式回调
    static BUS_TOPICENDPOINT(EmergencyStopCallback);     // [订阅] 急停回调
    static BUS_TOPICENDPOINT(TriggerMotorStallCallback); // [订阅] 氢弹电机卡弹回调

  private:
    // 线程ID，用于通知卡弹时立即开始反转
    osThreadId threadId;

    // 目前只是固定2个摩擦轮电机 + 1个拨弹电机，全队步兵发射机构统一所以不需要担心这个
    BasicMotor *fricMotors[2], *triggerMotor;

    bool fricEnable;           // 摩擦轮是否已开始旋转
    enum __packed ShooterState // 发射机构当前工作状态
    {
        StateIdle,               // 空闲状态不发射弹丸
        StateShootingSingleshot, // 设为射出单发弹丸
        StateShootingContinuous, // 设为射出连续弹丸
        StateRecoveringStall     // 正在从卡弹中反转恢复
    } state, stateBeforeStall;

    float fricSpeed;                     // 摩擦轮速度
    float triggerAngle, targetTrigAngle; // 拨弹一次角度及累计角度

    uint16_t bulletInterval; // 连发间隔ms
    uint16_t taskInterval;

    char *configFuncName;
    char *shootFuncName;
    char *toggleFricMotorFuncName;
    char *triggerStallName;
} shooter;

void Shooter::Tick()
{
    switch (state)
    {
    case StateIdle:
        osDelay(taskInterval);
        break;

    case StateShootingSingleshot: { // 单发
        bool fricMotorGuard = false;
        if (fricEnable == false) // 若摩擦轮未开启则先开启
        {
            fricMotorGuard = true;
            ToggleFrictionMotor(true);
            osDelay(200); // 等待摩擦轮转速稳定
        }
        targetTrigAngle += triggerAngle;
        triggerMotor->SetTarget(targetTrigAngle);
        osDelay(300);       // 等待弹被打出去
        if (fricMotorGuard) // 如果打之前摩擦轮没开，就关掉
            ToggleFrictionMotor(false);
        state = StateIdle;
        break;
    }

    case StateShootingContinuous: { // 以一定的时间间隔连续发射
        bool fricMotorGuard = false;
        if (fricEnable == false) // 若摩擦轮未开启则先开启
        {
            fricMotorGuard = true;
            ToggleFrictionMotor(true);
            osDelay(200); // 等待摩擦轮转速稳定
        }
        targetTrigAngle += triggerAngle; // 增加拨弹电机目标角度
        triggerMotor->SetTarget(targetTrigAngle);

        // 等待卡弹信号或等待时间过完，尽快在卡弹时解决
        osSignalWait(SignalTriggerStall, bulletInterval);
        break;
    }

    case StateRecoveringStall: {
        osSignalWait(SignalTriggerStall, 0); // 清理卡弹标志位

        float totalAngle = triggerMotor->GetData(TotalAngle); // 重置电机角度为当前累计角度
        targetTrigAngle = totalAngle - triggerAngle * 0.5f; // 反转
        triggerMotor->SetTarget(targetTrigAngle);
        osDelay(500); // 等待电机堵转恢复

        state = stateBeforeStall;
        break;
    }
    }
}

// 内部开关摩擦轮电机函数
void Shooter::ToggleFrictionMotor(bool enable)
{
    if (fricEnable == enable)
        return;

    if (enable)
    {
        fricMotors[0]->SetTarget(-fricSpeed);
        fricMotors[1]->SetTarget(fricSpeed);
    }
    else
    {
        fricMotors[0]->SetTarget(0);
        fricMotors[1]->SetTarget(0);
    }

    fricEnable = enable;
}

// 更改射击参数（摩擦轮目标转速、拨一发弹丸的角度）
BUS_REMOTEFUNC(Shooter::ConfigFuncCallback)
{
    U_S(Shooter);

    if (Bus_CheckMapKeyExist(frame, "fric-speed"))
        self->fricSpeed = Bus_GetMapValue(frame, "fric-speed").F32;

    if (Bus_CheckMapKeyExist(frame, "trigger-angle"))
        self->triggerAngle = Bus_GetMapValue(frame, "trigger-angle").F32;

    return true;
}

// 开关摩擦轮
BUS_REMOTEFUNC(Shooter::ToggleFrictionMotorFuncCallback)
{
    U_S(Shooter);

    if (Bus_CheckMapKeyExist(frame, "enable"))
    {
        bool enable = Bus_GetMapValue(frame, "enable").Bool;
        self->ToggleFrictionMotor(enable);
    }
    return true;
}

// 射击动作
BUS_REMOTEFUNC(Shooter::ShootFuncCallback)
{
    U_S(Shooter);

    if (!Bus_CheckMapKeyExist(frame, "mode"))
        return false;

    // 状态转移逻辑见 shooterapi.h
    auto newMode = (ShooterMode)Bus_GetMapValue(frame, "mode").U32;
    switch (newMode)
    {
    case Idle:
        if (self->state == StateShootingContinuous)
        {
            self->ToggleFrictionMotor(false); // 从连续模式回到空闲时，关掉摩擦轮
            self->state = StateIdle;
        }

        break;

    case Singleshot:
        if (self->state == StateIdle)
            self->state = StateShootingSingleshot;
        break;

    case Continuous:
        self->state = StateShootingContinuous;
        break;

    default:
        return false;
    }

    return true;
}

// 拨弹电机堵转
BUS_TOPICENDPOINT(Shooter::TriggerMotorStallCallback)
{
    U_S(Shooter);

    self->stateBeforeStall = self->state; // 保留当前状态
    self->state = StateRecoveringStall;
    osSignalSet(self->threadId, SignalTriggerStall); // 让主线程立即处理卡弹
}

// 急停
BUS_TOPICENDPOINT(Shooter::EmergencyStopCallback)
{
    U_S(Shooter);

    for (uint8_t i = 0; i < 2; i++)
    {
        self->fricMotors[i]->EmergencyStop();
    }
    self->triggerMotor->EmergencyStop();
}

extern "C" void Shooter_TaskCallback(void const *argument)
{
    Depends_WaitFor(svc_shooter, {svc_can});
    shooter.Init((ConfItem *)argument);
    Depends_SignalFinished(svc_shooter);

    shooter.Exec();
}
