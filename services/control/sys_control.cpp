#include "cmsis_os.h"
#include "config.h"
#include "main.h"
#include "pid.h"
#include "rcapi.h"
#include "shooterapi.h"
#include "softbus.h"
#include "softbus_cppapi.h"
#include <string.h>

typedef enum
{
    SYS_FOLLOW_MODE,
    SYS_SPIN_MODE,
    SYS_SEPARATE_MODE
} SysCtrlMode;

typedef struct
{
    struct
    {
        float vx, vy, vw;
        float ax, ay;
    } chassisData; // 底盘数据

    struct
    {
        float targetYaw, targetPitch; // 操纵云台欲达到的目标偏离角度
        float relativeAngle;          // 云台偏离角度
    } gimbalData;                     // 云台数据

    uint8_t mode;
    bool rockerCtrl; // 遥控器控制标志位
    bool errFlag;    // 急停标志位
    PID rotatePID;
} SysControl;

SysControl sysCtrl = {0};

// 函数声明
void Sys_InitInfo(ConfItem *dict);
void Sys_InitReceiver(void);
void Sys_Broadcast(void);

void Sys_Chassis_StopCallback(const char *name, SoftBusFrame *frame, void *bindData);
void Sys_Chassis_MoveCallback(const char *name, SoftBusFrame *frame, void *bindData);
void Sys_Mode_ChangeCallback(const char *name, SoftBusFrame *frame, void *bindData);
void Sys_Gimbal_RotateCallback(const char *name, SoftBusFrame *frame, void *bindData);
void Sys_Shoot_Callback(const char *name, SoftBusFrame *frame, void *bindData);
BUS_TOPICENDPOINT(Sys_LeftSwitchCallback);
BUS_TOPICENDPOINT(Sys_RightSwitchCallback);
void Sys_ErrorHandle(void);

// 初始化控制信息
void Sys_InitInfo(ConfItem *dict)
{
    sysCtrl.mode = Conf_GetValue(dict, "init-mode", uint8_t, SYS_FOLLOW_MODE); // 默认跟随模式
    sysCtrl.rockerCtrl = Conf_GetValue(dict, "rocker-ctrl", bool, false);      // 默认键鼠控制

    sysCtrl.rotatePID.Init(Conf_GetNode(dict, "rotate-pid")); // 初始化小陀螺PID
}

// 初始化接收
void Sys_InitReceiver()
{
    // 底盘
    Bus_SubscribeTopics(NULL, Sys_Chassis_MoveCallback, {"/rc/key/on-pressing", "/rc/left-stick"});
    Bus_SubscribeTopic(NULL, Sys_Chassis_StopCallback, "/rc/key/on-up");
    // 云台
    Bus_SubscribeTopics(NULL,
                        Sys_Gimbal_RotateCallback,
                        {"/rc/mouse-move", "/rc/right-stick", "/gimbal/yaw/relative-angle"});
    // 模式切换
    Bus_SubscribeTopic(NULL, Sys_Mode_ChangeCallback, "/rc/key/on-click");
    // 拨杆动作
    Bus_SubscribeTopic(NULL, Sys_LeftSwitchCallback, "/rc/switch");
    Bus_SubscribeTopic(NULL, Sys_RightSwitchCallback, "/rc/switch");
    // 发射
    Bus_SubscribeTopics(NULL,
                        Sys_Shoot_Callback,
                        {"/rc/key/on-click",
                         "/rc/key/on-long-press",
                         "/rc/key/on-up",
                         "/rc/wheel"});
}

extern "C" void SYS_CTRL_TaskCallback(void const *argument)
{
    // 进入临界区
    portENTER_CRITICAL();
    Sys_InitInfo((ConfItem *)argument);
    Sys_InitReceiver();
    portEXIT_CRITICAL();
    while (1)
    {
        if (sysCtrl.errFlag == 1)
            Sys_ErrorHandle();

        if (sysCtrl.mode == SYS_FOLLOW_MODE) // 跟随模式
        {
            sysCtrl.rotatePID.SingleCalc(0, sysCtrl.gimbalData.relativeAngle);
            sysCtrl.chassisData.vw = sysCtrl.rotatePID.output;
        }
        else if (sysCtrl.mode == SYS_SPIN_MODE) // 小陀螺模式
        {
            sysCtrl.chassisData.vw = 240;
        }
        else if (sysCtrl.mode == SYS_SEPARATE_MODE) // 分离模式
        {
            sysCtrl.chassisData.vw = 0;
        }
        Sys_Broadcast();
        osDelay(10);
    }
}

// 发送广播
void Sys_Broadcast()
{
    Bus_PublishTopic("/chassis/speed",
                     {{"vx", {.F32 = sysCtrl.chassisData.vx}},
                      {"vy", {.F32 = sysCtrl.chassisData.vy}},
                      {"vw", {.F32 = sysCtrl.chassisData.vw}}});
    Bus_PublishTopic("/chassis/relative-angle",
                     {{"angle", {.F32 = sysCtrl.gimbalData.relativeAngle}}});
    Bus_PublishTopic("/gimbal/setting",
                     {{"yaw", {.F32 = sysCtrl.gimbalData.targetYaw}},
                      {"pitch", {.F32 = sysCtrl.gimbalData.targetPitch}}});
}

// 底盘运动及停止回调函数
void Sys_Chassis_MoveCallback(const char *name, SoftBusFrame *frame, void *bindData)
{
    float speedRatio = 0;
    if (!strcmp(name, "/rc/key/on-pressing") && !sysCtrl.rockerCtrl) // 键鼠控制
    {
        if (!Bus_CheckMapKeysExist(frame, {"combine-key", "key"}))
            return;

        if (!strcmp(Bus_GetMapValue(frame, "combine-key").Str, "none")) // 正常
            speedRatio = 1;
        else if (!strcmp(Bus_GetMapValue(frame, "combine-key").Str, "shift")) // 快速
            speedRatio = 5;
        else if (!strcmp(Bus_GetMapValue(frame, "combine-key").Str, "ctrl")) // 慢速
            speedRatio = 0.2;

        switch (Bus_GetMapValue(frame, "key").Str[0])
        {
        case 'A':
            sysCtrl.chassisData.vx = 200 * speedRatio;
            break;
        case 'D':
            sysCtrl.chassisData.vx = -200 * speedRatio;
            break;
        case 'W':
            sysCtrl.chassisData.vy = 200 * speedRatio;
            break;
        case 'S':
            sysCtrl.chassisData.vy = -200 * speedRatio;
            break;
        }
    }
    else if (!strcmp(name, "/rc/left-stick") && sysCtrl.rockerCtrl) // 遥控器控制
    {
        if (!Bus_CheckMapKeysExist(frame, {"x", "y"}))
            return;
        sysCtrl.chassisData.vx = Bus_GetMapValue(frame, "x").F32;
        sysCtrl.chassisData.vy = Bus_GetMapValue(frame, "y").F32;
    }
}

void Sys_Chassis_StopCallback(const char *name, SoftBusFrame *frame, void *bindData)
{

    if (sysCtrl.rockerCtrl || !Bus_CheckMapKeyExist(frame, "key"))
        return;
    switch (Bus_GetMapValue(frame, "key").U8)
    {
    case 'A':
    case 'D':
        sysCtrl.chassisData.vx = 0;
        break;
    case 'W':
    case 'S':
        sysCtrl.chassisData.vy = 0;
        break;
    }
}

// 云台旋转回调函数
void Sys_Gimbal_RotateCallback(const char *name, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(name, "/rc/mouse-move") && !sysCtrl.rockerCtrl) // 键鼠控制
    {
        if (!Bus_CheckMapKeysExist(frame, {"x", "y"}))
            return;
        // FIXME:鼠标能打多少量未知
        // sysCtrl.gimbalData.targetYaw +=*(int16_t*)Bus_GetMapValue(frame,"x");
        // sysCtrl.gimbalData.targetPitch +=*(int16_t*)Bus_GetMapValue(frame,"y");
    }
    else if (!strcmp(name, "/rc/right-stick") && sysCtrl.rockerCtrl) // 遥控器控制
    {
        if (!Bus_CheckMapKeysExist(frame, {"x", "y"}))
            return;

        sysCtrl.gimbalData.targetYaw = Bus_GetMapValue(frame, "x").F32 * 1.5f;
        sysCtrl.gimbalData.targetPitch = Bus_GetMapValue(frame, "y").F32 * 1.5f;
    }
    else if (!strcmp(name, "/gimbal/yaw/relative-angle"))
    {
        if (!Bus_CheckMapKeyExist(frame, "angle"))
            return;
        sysCtrl.gimbalData.relativeAngle = Bus_GetMapValue(frame, "angle").F32;
    }
}

// 模式切换回调
void Sys_Mode_ChangeCallback(const char *name, SoftBusFrame *frame, void *bindData)
{
    if (!strcmp(name, "/rc/key/on-click") && !sysCtrl.rockerCtrl) // 键鼠控制
    {
        if (!Bus_CheckMapKeyExist(frame, "key"))
            return;
        switch (Bus_GetMapValue(frame, "key").U8)
        {
        case 'Q':
            sysCtrl.mode = SYS_SPIN_MODE; // 小陀螺模式
            break;
        case 'E':
            sysCtrl.mode = SYS_FOLLOW_MODE; // 跟随模式
            break;
        case 'R':
            sysCtrl.mode = SYS_SEPARATE_MODE; // 分离模式
            break;
        }
    }
}

// 左拨杆功能
BUS_TOPICENDPOINT(Sys_LeftSwitchCallback)
{
    if (Bus_CheckMapKeyExist(frame, "left"))
    {
        switch (Bus_GetMapValue(frame, "left").U8)
        {
        case SwitchUp:
            sysCtrl.rockerCtrl = true; // 切换至遥控器控制
            sysCtrl.errFlag = 0;
            break;
        case SwitchMiddle:
            sysCtrl.rockerCtrl = false; // 切换至键鼠控制
            sysCtrl.errFlag = 0;
            break;
        case SwitchDown:
            sysCtrl.errFlag = 1; // 急停
            break;
        }
    }
}

// 右拨杆功能
BUS_TOPICENDPOINT(Sys_RightSwitchCallback)
{
    if (Bus_CheckMapKeyExist(frame, "right") && sysCtrl.rockerCtrl)
    {
        auto switchPos = RcSwitch(Bus_GetMapValue(frame, "right").U8);

        switch (switchPos)
        {
        case SwitchUp:
            // 向上为开启摩擦轮，但开启摩擦轮还需要能关闭，需要单独逻辑判断
            break;
        case SwitchMiddle:
            sysCtrl.mode = SYS_FOLLOW_MODE; // 跟随模式
            break;
        case SwitchDown:
            sysCtrl.mode = SYS_SPIN_MODE; // 小陀螺模式
            break;
        }

        // 向上为开启摩擦轮，向下为关闭摩擦轮
        Bus_RemoteFuncCall("/shooter/toggle-fric", {{"enable", {.Bool = (switchPos == SwitchUp)}}});
    }
}

// 发射回调函数
void Sys_Shoot_Callback(const char *name, SoftBusFrame *frame, void *bindData)
{
    // if (!strcmp(name, "/rc/key/on-click") && !sysCtrl.rockerCtrl) // 键鼠控制
    // {
    //     if (!Bus_CheckMapKeyExist(frame, "left"))
    //         return;
    //     Bus_RemoteFuncCall("/shooter/shoot", {{"mode", {.U32 = Singleshot}}}); // 点射
    // }
    // else if (!strcmp(name, "/rc/key/on-long-press") && !sysCtrl.rockerCtrl)
    // {
    //     if (!Bus_CheckMapKeyExist(frame, "left"))
    //         return;
    //     Bus_RemoteFuncCall("/shooter/shoot",
    //                        {{"mode", {.Str = "continue"}},
    //                         {"interval-time", {.U32 = 100}}}); // 连发
    // }
    // else if (!strcmp(name, "/rc/key/on-up") && !sysCtrl.rockerCtrl)
    // {
    //     if (!Bus_CheckMapKeyExist(frame, "left"))
    //         return;
    //     Bus_RemoteFuncCall("/shooter/shoot", {{"mode", {.U32 = Idle}}}); // 连发
    // }
    // else
    if (!strcmp(name, "/rc/wheel") && sysCtrl.rockerCtrl) // 遥控器控制
    {
        if (!Bus_CheckMapKeyExist(frame, "value"))
            return;
        int16_t wheel = Bus_GetMapValue(frame, "value").I16;

        if (wheel > 600)
            Bus_RemoteFuncCall("/shooter/shoot", {{"mode", {.U32 = Singleshot}}}); // 点射
    }
}

// 急停
void Sys_ErrorHandle(void)
{
    Bus_PublishTopic("/system/stop", {{"", {NULL}}});
    while (1)
    {
        if (!sysCtrl.errFlag)
        {
            __disable_irq();
            NVIC_SystemReset();
        }
        osDelay(2);
    }
}
