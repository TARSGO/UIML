
// FIXME: UIML TIM BSP接口不好用，先不用了

#include <cmsis_os.h>
#include "config.h"
#include "my_queue.h"
#include "dependency.h"
#include "portable.h"
#include "softbus.h"
#include "stm32f4xx_hal_tim.h"
#include "sys_conf.h"
#include "yamlparser.h"
#include "beepapi.h"

uint16_t BeepFrequencyTable[] = {
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311,
    330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784,
    831, 880, 932, 988, 1046, 1108, 1174, 1244, 1318, 1396, 1479, 1567, 1661, 1760,
    1864, 1975, 2093, 2217, 2349, 2489, 2637, 2793, 2959, 3135, 3322, 3520, 3729, 3951,
};

void Beep_Init(ConfItem* conf);

void Beep_TimerCallback(const void* arg);

BUS_REMOTEFUNC(Beep_UrgentCommand);
BUS_REMOTEFUNC(Beep_NormalCommand);
BUS_REMOTEFUNC(Beep_ForceCommand);

struct _Beep {
    // Queue urgentCmdQueue, normalCmdQueue;
    QueueHandle_t urgentCmdQueue, normalCmdQueue;
    void *urgentCmdQueueBuffer, *normalCmdQueueBuffer;

    // 输出TIM
    TIM_HandleTypeDef *htim;

    // （规划音符的）软件定时器
    osTimerId Timer;
    uint8_t urgentCmdDueTime, normalCmdDueTime;
    bool isTimerRunning;

    // 低优先级被覆盖的音符
    uint16_t normalPriorityPitch;
} Beep;

struct BeepCommand {
    uint8_t Duration;
    uint8_t Note;
};

// 重启软件定时器便利函数
inline void Beep_StartTimer()
{
    osTimerStart(Beep.Timer, 10);
    Beep.isTimerRunning = true;
    Beep.htim->Instance->CNT = 0;
}

// 命令入队便利函数
inline bool Beep_InteralEnqueueCommands(const char* cmdLine, QueueHandle_t queue)
{
    // 传入的命令行为<长度,音高>的两个字节为一组、有不定长个组的数据，如果遇到长度为0的，则认为命令行在此处结束
    for (int i = 0; cmdLine[i] != 0 && uxQueueSpacesAvailable(queue); i += 2)
    {
        BeepCommand cmd { cmdLine[i], cmdLine[i + 1] };
        xQueueSend(queue, &cmd, 0);
    }

    // （如果需要）重启定时器
    if (!Beep.isTimerRunning)
        Beep_StartTimer();

    return true;
}

// 蜂鸣器设置频率便利函数
inline void Beep_SetFrequency(uint16_t freq)
{
    Beep.htim->Instance->ARR = 100000 / freq;
    Beep.htim->Instance->CCR3 = Beep.htim->Instance->ARR / 2;
    Beep.htim->Instance->CNT = 0;
}

inline void Beep_StopBeep()
{
    Beep_SetFrequency(0);
}

extern "C" void Beep_TaskCallback(void* argument)
{
    // Depends_WaitFor(svc_beep, { svc_tim });
    Beep_Init((ConfItem*)argument);
    // Depends_SignalFinished(svc_beep);

    vTaskDelete(NULL);
}

void Beep_Init(ConfItem* conf)
{
    auto queueSize = Conf_GetValue(conf, "queue-size", uint32_t, 8);

    // 初始化状态变量
    Beep.normalCmdDueTime = UINT8_MAX;
    Beep.urgentCmdDueTime = UINT8_MAX;
    Beep.normalPriorityPitch = 0;

    // 初始化命令队列
    Beep.urgentCmdQueue = xQueueCreate(queueSize, sizeof(BeepCommand));
    Beep.normalCmdQueue = xQueueCreate(queueSize, sizeof(BeepCommand));

    // 读取时钟信息
    auto timerNode = Conf_GetNode(conf, "timer");
    auto htim = Conf_GetPeriphHandle(Conf_GetValue(timerNode, "name", const char*, nullptr), TIM_HandleTypeDef);
    auto timerClock = Conf_GetValue(timerNode, "clockkhz", int32_t, -1);
    if (!htim || timerClock == -1)
        return;
    Beep.htim = htim;
    // 初始化定时器外设
    // 配置文件中给出TIM的时钟输入（以kHz为单位），C板蜂鸣器为TIM4 CH3，属于时钟树的APB2 Timer路时钟
    // 我队配置中这路时钟一般为168MHz，预计不会低到100kHz精度以下，故直接先设置PSC将时钟细分到100kHz量级，此时ARR每1个点等于0.00001s
    // API中给出最高音高为B5(988Hz)周期为0.00101214s，按当前精度取整到0.00101s后实际频率为990Hz，误差约在可接受范围内
    htim->Instance->PSC = timerClock / 100;

    // 启动0.01s定时器
    osTimerDef(BeepTimer, Beep_TimerCallback);
    Beep.Timer = osTimerCreate(osTimer(BeepTimer), osTimerPeriodic, nullptr);
    Beep_StartTimer();

    // 启动TIM输出
    HAL_TIM_PWM_Start(Beep.htim, TIM_CHANNEL_3);

    // 注册远程函数
    Bus_RemoteFuncRegister(nullptr, Beep_UrgentCommand, "/beep/urgent");
    Bus_RemoteFuncRegister(nullptr, Beep_NormalCommand, "/beep/normal");
    Bus_RemoteFuncRegister(nullptr, Beep_ForceCommand, "/beep/cmd");
}

BUS_REMOTEFUNC(Beep_UrgentCommand)
{
    if (!uxQueueSpacesAvailable(Beep.urgentCmdQueue)  || !Bus_CheckMapKeyExist(frame, "cmd"))
        return false;

    const char* cmdLine = Bus_GetMapValue(frame, "cmd").Str;

    // 加入命令队列中
    return Beep_InteralEnqueueCommands(cmdLine, Beep.urgentCmdQueue);
}

BUS_REMOTEFUNC(Beep_NormalCommand)
{
    if (!uxQueueSpacesAvailable(Beep.normalCmdQueue) || !Bus_CheckMapKeyExist(frame, "cmd"))
        return false;

    const char* cmdLine = Bus_GetMapValue(frame, "cmd").Str;

    // 加入命令队列中
    return Beep_InteralEnqueueCommands(cmdLine, Beep.normalCmdQueue);
}

BUS_REMOTEFUNC(Beep_ForceCommand)
{
    Beep_SetFrequency(Bus_GetMapValue(frame, "freq").U16);
    return true;
}

void Beep_TimerCallback(const void* arg)
{
    // 遇到任一队列被排空时置true
    bool checkEndOfNotes = false;
    
    // 记录当前播放音符时长-1，如果为UINT8_MAX则当前没播放音符，跳过
    if (Beep.normalCmdDueTime != UINT8_MAX) Beep.normalCmdDueTime--;
    if (Beep.urgentCmdDueTime != UINT8_MAX) Beep.urgentCmdDueTime--;

    // 如果高优先命令做完了，尝试再取一个，如果取完之后没有了，再让低优先级的出声
    if (Beep.urgentCmdDueTime == 0 || Beep.urgentCmdDueTime == UINT8_MAX)
    {
        if (uxQueueMessagesWaiting(Beep.urgentCmdQueue))
        {
            BeepCommand cmd;
            xQueueReceive(Beep.urgentCmdQueue, &cmd, 0);
            Beep.urgentCmdDueTime = cmd.Duration;
            Beep_SetFrequency(BeepFrequencyTable[cmd.Note - BEEP_LOWEST_PITCH]);

            if (!uxQueueMessagesWaiting(Beep.urgentCmdQueue))
            {
                checkEndOfNotes = true;
                Beep.urgentCmdDueTime = UINT8_MAX;
                // 让低优先级响，如果低优先级没有东西（为0）会停下来
                Beep_SetFrequency(Beep.normalPriorityPitch);
            }
        }
    }

    // 再检查低优先命令
    if (Beep.normalCmdDueTime == 0 || Beep.normalCmdDueTime == UINT8_MAX)
    {
        if (uxQueueMessagesWaiting(Beep.normalCmdQueue))
        {
            BeepCommand cmd;
            xQueueReceive(Beep.normalCmdQueue, &cmd, 0);

            // 不能抢占高优先级
            if (Beep.urgentCmdDueTime == UINT8_MAX)
                Beep_SetFrequency((Beep.normalPriorityPitch = BeepFrequencyTable[cmd.Note - BEEP_LOWEST_PITCH]));
            Beep.normalCmdDueTime = cmd.Duration;
        }
        else
        {
            checkEndOfNotes = true;
            Beep.normalCmdDueTime = UINT8_MAX;
        }
    }

    // 如果队列已空，暂停定时器
    if (checkEndOfNotes && !uxQueueMessagesWaiting(Beep.normalCmdQueue) && !uxQueueMessagesWaiting(Beep.urgentCmdQueue))
    {
        // 停止蜂鸣
        Beep_StopBeep();
        osTimerStop(Beep.Timer);
        Beep.isTimerRunning = false;
    }
}
