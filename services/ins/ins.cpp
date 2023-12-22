#include "AHRS.h"
#include "cmsis_os.h"
#include "config.h"
#include "dependency.h"
#include "extendio.h"
#include "filter.h"
#include "imu.h"
#include "pid.h"
#include "softbus.h"
#include "strict.h"
#include "sys_conf.h"
#include <cstddef>
#include <cstring>
#include <stdio.h>

typedef struct
{
    struct RemapData
    {
        // 重映射关系：
        // 有一个remap[3]对应XYZ三个轴（以及RPY）
        // 字段index究竟应该为从IMU中取出的数据的哪一个（base为0）
        // 字段sign表示是否应该取反（其值为应乘进去的符号）
        uint8_t index;
        int8_t sign;
    } remap[3];
    struct
    {
        float quat[4];
        float accel[3];
        float gyro[3];
        float mag[3];
        float gyroOffset[3];
        float tmp;
    } imuData;
    uint8_t spiX;
    float yaw, pitch, roll;
    float targetTmp;
    uint8_t timX;
    uint8_t channelX;

    BasicImu *imu;
    Filter *filter;

    uint16_t taskInterval; // 任务执行间隔

    char *eulerAngleName;
} INS;

void INS_Init(INS *ins, ConfItem *dict);
void INS_Remap(INS *ins);
void INS_TmpPIDTimerCallback(void const *argument);

void INS_Init_RemapRelations(INS *ins, const char *fwdAxis, const char *upAxis);

static INS *GlobalINS;

extern "C" void INS_TaskCallback(void const *argument)
{
    Depends_WaitFor(svc_ins, {svc_spi});

    /* USER CODE BEGIN IMU */
    INS ins = {0};
    GlobalINS = &ins;

    osDelay(50);
    INS_Init(&ins, (ConfItem *)argument);
    AHRS_init(ins.imuData.quat, ins.imuData.accel, ins.imuData.mag);
    // 校准零偏
    //  for(int i=0;i<10000;i++)
    //  {
    //  	BMI088_ReadData(ins.spiX, ins.imu.gyro,ins.imu.accel, &ins.imu.tmp);
    //  	ins.imu.gyroOffset[0] +=ins.imu.gyro[0];
    //  	ins.imu.gyroOffset[1] +=ins.imu.gyro[1];
    //  	ins.imu.gyroOffset[2] +=ins.imu.gyro[2];
    //  	HAL_Delay(1);
    //  }
    //  ins.imu.gyroOffset[0] = ins.imu.gyroOffset[0]/10000.0f;
    //  ins.imu.gyroOffset[1] = ins.imu.gyroOffset[1]/10000.0f;
    //  ins.imu.gyroOffset[2] = ins.imu.gyroOffset[2]/10000.0f;

    Depends_SignalFinished(svc_ins);

    /* Infinite loop */
    while (1)
    {
        ins.imu->Acquire(); // 采样
        ins.imu->GetGyroscopeData(ins.imuData.gyro[0], ins.imuData.gyro[1], ins.imuData.gyro[2]);
        ins.imu->GetAccelerometerData(ins.imuData.accel[0],
                                      ins.imuData.accel[1],
                                      ins.imuData.accel[2]);
        ins.imuData.tmp = ins.imu->GetTemperature();
        INS_Remap(&ins);

        for (uint8_t i = 0; i < 3; i++)
            ins.imuData.gyro[i] -= ins.imuData.gyroOffset[i];

        // 滤波
        //  for(uint8_t i=0;i<3;i++)
        //  	ins.imu.accel[i] = ins.filter->cala(ins.filter , ins.imu.accel[i]);
        // 数据融合

        AHRS_update(ins.imuData.quat,
                    ins.taskInterval / 1000.0f,
                    ins.imuData.gyro,
                    ins.imuData.accel,
                    ins.imuData.mag);
        get_angle(ins.imuData.quat, &ins.yaw, &ins.pitch, &ins.roll);
        ins.yaw = ins.yaw / PI * 180;
        ins.pitch = ins.pitch / PI * 180;
        ins.roll = ins.roll / PI * 180;
        // 发布数据
        Bus_PublishTopic(ins.eulerAngleName,
                         {{"yaw", {.F32 = ins.yaw}},
                          {"pitch", {.F32 = ins.pitch}},
                          {"roll", {.F32 = ins.roll}}});
        osDelay(ins.taskInterval);
    }
    /* USER CODE END IMU */
}

void INS_Init(INS *ins, ConfItem *dict)
{
    U_C(dict);

    ins->taskInterval = Conf["task-interval"].get(10);

    // TODO: 温控
    // ins->filter = Filter_Init(Conf_GetNode(dict, "filter"));
    // ins->tmpPID.Init(Conf_GetNode(dict, "tmp-pid"));

    auto insName = Conf["name"].get("ins");
    ins->eulerAngleName = alloc_sprintf(pvPortMalloc, "/%s/euler-angle", insName);

    // 读取安装轴
    auto orientationNode = Conf["orientation"];
    auto fwdAxis = orientationNode["fwd"].get<const char *>(nullptr);
    auto upAxis = orientationNode["up"].get<const char *>(nullptr);
    // 根据安装轴推算重映射关系
    INS_Init_RemapRelations(ins, fwdAxis, upAxis);

    // 读取陀螺仪零漂参数
    auto gyroStaticDriftNode = Conf["gyro-static"];
    ins->imuData.gyroOffset[0] = gyroStaticDriftNode["0"].get<float>(0.0f);
    ins->imuData.gyroOffset[1] = gyroStaticDriftNode["1"].get<float>(0.0f);
    ins->imuData.gyroOffset[2] = gyroStaticDriftNode["2"].get<float>(0.0f);

    ins->imu = BasicImu::Create(Conf["imu"]); // 创建陀螺仪对象
    ins->imu->Acquire();                      // 先初始化采样
    ins->imu->GetGyroscopeData(ins->imuData.gyro[0], ins->imuData.gyro[1], ins->imuData.gyro[2]);
    ins->imu->GetAccelerometerData(ins->imuData.accel[0],
                                   ins->imuData.accel[1],
                                   ins->imuData.accel[2]);
    ins->imuData.tmp = ins->imu->GetTemperature();
    INS_Remap(ins);

    // 创建定时器进行温度pid控制
    osTimerDef(tmp, INS_TmpPIDTimerCallback);
    osTimerStart(osTimerCreate(osTimer(tmp), osTimerPeriodic, ins), 2);
}

// 执行重映射操作
void INS_Remap(INS *ins)
{
    float mappedAccel[3] = {0.0f, 0.0f, 0.0f};
    float mappedGyro[3] = {0.0f, 0.0f, 0.0f};

    for (int i = 0; i < 3; i++)
    {
        auto &remap = ins->remap[i];
        mappedAccel[i] = ins->imuData.accel[remap.index] * remap.sign;
        mappedGyro[i] = ins->imuData.gyro[remap.index] * remap.sign;
        // TODO: 磁力计 Mag
    }

    memcpy(ins->imuData.accel, mappedAccel, sizeof(mappedAccel));
    memcpy(ins->imuData.gyro, mappedGyro, sizeof(mappedGyro));
}

// 软件定时器回调函数
void INS_TmpPIDTimerCallback(void const *argument)
{
    INS *ins = (INS *)pvTimerGetTimerID((TimerHandle_t)argument);
    ins->imu->TemperatureControlTick();
}

// 推算重映射关系
void INS_Init_RemapRelations(INS *ins, const char *fwdAxis, const char *upAxis)
{
    if (fwdAxis == nullptr || upAxis == nullptr || strlen(fwdAxis) != 2 || strlen(upAxis) != 2 ||
        fwdAxis[1] == upAxis[1])
    {
        // 检查非法输入：必须为两个字符长的字符串且两个轴不能相同，否则停机
        UIML_CRASH_STRICT("Invalid INS Remap configuration", (void *)upAxis, (void *)fwdAxis);
    }

    // 向量子函数
    // clang-format off
    // 从轴向字符串转换为向量
    auto AxisToVec = [&](const char *axis, int8_t *vec) {
        vec[0] = vec[1] = vec[2] = 0;
        int8_t assign;
        switch (axis[0]) {
            case '+': assign = 1; break;
            case '-': assign = -1; break;
            default: UIML_CRASH_STRICT("Invalid INS Remap: Bad sign", (void*)axis);
        }
        switch (axis[1]) {
            case 'X': case 'x': vec[0] = assign; break;
            case 'Y': case 'y': vec[1] = assign; break;
            case 'Z': case 'z': vec[2] = assign; break;
            default: UIML_CRASH_STRICT("Invalid INS Remap: Bad axis", (void*)axis);
        }
    };
    // vec1 X vec2
    auto CrossProduct = [&](int8_t *vec1, int8_t* vec2, int8_t* result) {
        //  [(a2 * b3 – a3 * b2), (a3 * b1 – a1 * b3), (a1 * b2 – a2 * b1)] 
        result[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
        result[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
        result[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
    };
    // 从向量转换为重映射关系
    auto VecToRemap = [&](int8_t *vec, INS::RemapData &remap) {
        for (int8_t i = 0; i < 3; i++)
        {
            if (vec[i] == 0) continue;
            remap.index = i;
            remap.sign = vec[i];
            break;
        }
    };
    // clang-format on

    // 上方向轴 X 前向轴 得到Pitch轴轴向
    int8_t fwdVec[3]{0}, upVec[3]{0}, pitchVec[3]{0};
    AxisToVec(fwdAxis, fwdVec);
    AxisToVec(upAxis, upVec);
    CrossProduct(upVec, fwdVec, pitchVec);

    // 写入重映射关系数据
    VecToRemap(fwdVec, ins->remap[0]);
    VecToRemap(pitchVec, ins->remap[1]);
    VecToRemap(upVec, ins->remap[2]);
}
