
#include "stm32f4xx.h"
#include "arm_math.h"
#include "beepapi.h"
#include "cmsis_os.h"
#include "config.h"
#include "dependency.h"
#include "extendio.h"
#include "motor.h"
#include "slope.h"
#include "softbus.h"
#include "sys_conf.h"
#include <stdio.h>
#include <string.h>

#ifndef LIMIT
#define LIMIT(x, min, max) (x) = (((x) <= (min)) ? (min) : (((x) >= (max)) ? (max) : (x)))
#endif
#define CHASSIS_ACC2SLOPE(taskInterval, acc) ((taskInterval) * (acc) / 1000) // mm/s2

typedef struct _Chassis
{
    // 底盘尺寸信息
    struct Info
    {
        float wheelbase;   // 轴距
        float wheeltrack;  // 轮距
        float wheelRadius; // 轮半径
        float offsetX;     // 重心在xy轴上的偏移
        float offsetY;
    } info;
    // 4个电机
    BasicMotor *motors[4];
    // 底盘移动信息
    struct Move
    {
        float vx; // 当前左右平移速度 mm/s
        float vy; // 当前前后移动速度 mm/s
        float vw; // 当前旋转速度 rad/s

        float maxVx, maxVy, maxVw; // 三个分量最大速度
        Slope xSlope, ySlope;      // 斜坡
    } move;

    float relativeAngle; // 与底盘的偏离角，单位度

    uint16_t taskInterval;

    char *speedName;
    char *accName;
    char *relAngleName;

} Chassis;

void Chassis_Init(Chassis *chassis, ConfItem *dict);
void Chassis_UpdateSlope(Chassis *chassis);
BUS_TOPICENDPOINT(Chassis_SetSpeedCallback);
BUS_TOPICENDPOINT(Chassis_SetAccCallback);
BUS_TOPICENDPOINT(Chassis_SetRelativeAngleCallback);
void Chassis_StopCallback(const char *name, SoftBusFrame *frame, void *bindData);

static Chassis *GlobalChassis;

// 底盘任务回调函数
extern "C" void Chassis_TaskCallback(void const *argument)
{
    Depends_WaitFor(svc_chassis, {svc_can});

    // 进入临界区
    portENTER_CRITICAL();
    Chassis chassis = {0};
    Chassis_Init(&chassis, (ConfItem *)argument);
    portEXIT_CRITICAL();

    Depends_SignalFinished(svc_chassis);

    // 启动音效
    BeepCmd startupBeep = {3, BEEP_C5, 2, 0, 3, BEEP_F5, 0};
    Bus_RemoteFuncCall("/beep/urgent", {{"cmd", &startupBeep}});

    GlobalChassis = &chassis;

    while (1)
    {
        /*************计算底盘平移速度**************/

        Chassis_UpdateSlope(&chassis); // 更新运动斜坡函数数据

        // 将云台坐标系下平移速度解算到底盘平移速度(根据云台偏离角)
        float gimbalAngleSin = arm_sin_f32(chassis.relativeAngle * PI / 180);
        float gimbalAngleCos = arm_cos_f32(chassis.relativeAngle * PI / 180);
        chassis.move.vx = Slope_GetVal(&chassis.move.xSlope) * gimbalAngleCos +
                          Slope_GetVal(&chassis.move.ySlope) * gimbalAngleSin;
        chassis.move.vy = -Slope_GetVal(&chassis.move.xSlope) * gimbalAngleSin +
                          Slope_GetVal(&chassis.move.ySlope) * gimbalAngleCos;
        float vw = chassis.move.vw / 180 * PI;

        /*************解算各轮子转速**************/

        float rotateRatio[4];
        rotateRatio[0] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f -
                         chassis.info.offsetY + chassis.info.offsetX;
        rotateRatio[1] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f -
                         chassis.info.offsetY - chassis.info.offsetX;
        rotateRatio[2] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f +
                         chassis.info.offsetY + chassis.info.offsetX;
        rotateRatio[3] = (chassis.info.wheelbase + chassis.info.wheeltrack) / 2.0f +
                         chassis.info.offsetY - chassis.info.offsetX;
        float wheelRPM[4];
        wheelRPM[0] = (chassis.move.vx + chassis.move.vy - vw * rotateRatio[0]) * 60 /
                      (2 * PI * chassis.info.wheelRadius); // FL
        wheelRPM[1] = -(-chassis.move.vx + chassis.move.vy + vw * rotateRatio[1]) * 60 /
                      (2 * PI * chassis.info.wheelRadius); // FR
        wheelRPM[2] = (-chassis.move.vx + chassis.move.vy - vw * rotateRatio[2]) * 60 /
                      (2 * PI * chassis.info.wheelRadius); // BL
        wheelRPM[3] = -(chassis.move.vx + chassis.move.vy + vw * rotateRatio[3]) * 60 /
                      (2 * PI * chassis.info.wheelRadius); // BR

        for (uint8_t i = 0; i < 4; i++)
        {
            chassis.motors[i]->SetTarget(wheelRPM[i]);
        }

        osDelay(chassis.taskInterval);
    }
}

void Chassis_Init(Chassis *chassis, ConfItem *dict)
{
    // 任务间隔
    chassis->taskInterval = Conf_GetValue(dict, "task-interval", uint16_t, 2);
    // 底盘尺寸信息（用于解算轮速）
    chassis->info.wheelbase = Conf_GetValue(dict, "info/wheelbase", float, 0);
    chassis->info.wheeltrack = Conf_GetValue(dict, "info/wheeltrack", float, 0);
    chassis->info.wheelRadius = Conf_GetValue(dict, "info/wheel-radius", float, 76);
    chassis->info.offsetX = Conf_GetValue(dict, "info/offset-x", float, 0);
    chassis->info.offsetY = Conf_GetValue(dict, "info/offset-y", float, 0);
    // 移动参数初始化
    chassis->move.maxVx = Conf_GetValue(dict, "move/max-vx", float, 2000);
    chassis->move.maxVy = Conf_GetValue(dict, "move/max-vy", float, 2000);
    chassis->move.maxVw = Conf_GetValue(dict, "move/max-vw", float, 180);
    // 底盘加速度初始化
    float xAcc = Conf_GetValue(dict, "move/x-acc", float, 1000);
    float yAcc = Conf_GetValue(dict, "move/y-acc", float, 1000);
    Slope_Init(&chassis->move.xSlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, xAcc), 0);
    Slope_Init(&chassis->move.ySlope, CHASSIS_ACC2SLOPE(chassis->taskInterval, yAcc), 0);
    // 底盘电机初始化
    chassis->motors[0] = BasicMotor::Create(Conf_GetNode(dict, "motor-fl"));
    chassis->motors[1] = BasicMotor::Create(Conf_GetNode(dict, "motor-fr"));
    chassis->motors[2] = BasicMotor::Create(Conf_GetNode(dict, "motor-bl"));
    chassis->motors[3] = BasicMotor::Create(Conf_GetNode(dict, "motor-br"));

    // 设置底盘电机为速度模式
    for (uint8_t i = 0; i < 4; i++)
    {
        chassis->motors[i]->SetMode(ModeSpeed);
    }

    // 软总线广播、远程函数name重映射
    const char *chassisName = Conf_GetValue(dict, "name", const char *, NULL);
    chassisName = chassisName ? chassisName : "chassis";

    chassis->speedName = alloc_sprintf(pvPortMalloc, "/%s/speed", chassisName);
    chassis->accName = alloc_sprintf(pvPortMalloc, "/%s/acc", chassisName);
    chassis->relAngleName = alloc_sprintf(pvPortMalloc, "/%s/relative-angle", chassisName);

    // 注册远程函数
    Bus_SubscribeTopic(chassis, Chassis_SetSpeedCallback, chassis->speedName);
    Bus_SubscribeTopic(chassis, Chassis_SetAccCallback, chassis->accName);
    Bus_SubscribeTopic(chassis, Chassis_SetRelativeAngleCallback, chassis->relAngleName);
    Bus_SubscribeTopic(chassis, Chassis_StopCallback, "/system/stop");
}

// 更新斜坡计算速度
void Chassis_UpdateSlope(Chassis *chassis)
{
    Slope_NextVal(&chassis->move.xSlope);
    Slope_NextVal(&chassis->move.ySlope);
}
// 设置底盘速度回调
BUS_TOPICENDPOINT(Chassis_SetSpeedCallback)
{
    Chassis *chassis = (Chassis *)bindData;
    if (Bus_CheckMapKeyExist(frame, "vx"))
    {
        float vx = Bus_GetMapValue(frame, "vx").F32 * 2000;
        LIMIT(vx, -chassis->move.maxVx, chassis->move.maxVx);
        Slope_SetTarget(&chassis->move.xSlope, vx);
    }
    if (Bus_CheckMapKeyExist(frame, "vy"))
    {
        float vy = Bus_GetMapValue(frame, "vy").F32 * 2000;
        LIMIT(vy, -chassis->move.maxVy, chassis->move.maxVy);
        Slope_SetTarget(&chassis->move.ySlope, vy);
    }
    if (Bus_CheckMapKeyExist(frame, "vw"))
    {
        chassis->move.vw = Bus_GetMapValue(frame, "vw").F32;
        LIMIT(chassis->move.vw, -chassis->move.maxVw, chassis->move.maxVw);
    }
}
// 设置底盘加速度回调
BUS_TOPICENDPOINT(Chassis_SetAccCallback)
{
    Chassis *chassis = (Chassis *)bindData;
    if (Bus_CheckMapKeyExist(frame, "ax"))
        Slope_SetStep(&chassis->move.xSlope,
                      CHASSIS_ACC2SLOPE(chassis->taskInterval, Bus_GetMapValue(frame, "ax").F32));
    if (Bus_CheckMapKeyExist(frame, "ay"))
        Slope_SetStep(&chassis->move.ySlope,
                      CHASSIS_ACC2SLOPE(chassis->taskInterval, Bus_GetMapValue(frame, "ay").F32));
}

// 设置底盘坐标系分离角度回调
BUS_TOPICENDPOINT(Chassis_SetRelativeAngleCallback)
{
    Chassis *chassis = (Chassis *)bindData;
    if (Bus_CheckMapKeyExist(frame, "angle"))
        chassis->relativeAngle = Bus_GetMapValue(frame, "angle").F32;
}

// 底盘急停回调
void Chassis_StopCallback(const char *name, SoftBusFrame *frame, void *bindData)
{
    Chassis *chassis = (Chassis *)bindData;
    for (uint8_t i = 0; i < 4; i++)
    {
        chassis->motors[i]->EmergencyStop();
    }

    // 立即使电机动作
    Bus_RemoteFuncCall("/can/flush-buf", {{nullptr, nullptr}});
}
