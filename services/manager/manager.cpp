
#include "FreeRTOS.h"
#include "extendio.h"
#include "managerapi.h"
#include "motor.h"
#include "pid.h"
#include "portmacro.h"
#include "softbus.h"
#include "softbus_cppapi.h"
#include "strict.h"
#include "uimlraii.h"
#include "vector.h"

class Manager
{
  public:
    Manager() {}
    void Init(ConfItem *conf);

  private:
    struct RegisteredMotor // 注册过的电机
    {
        const char *Name;
        union MotorMeta {
            struct
            {
                uint8_t BusType;  // 总线类型
                uint8_t BusId;    // 该类型总线的编号
                uint16_t MotorId; // 该条总线上电机的编号
            };
            uint32_t WordType; // 用一个字宽的变量把上面那几个打包起来方便对比
        } Meta;
        BasicMotor *Motor;
    } __packed;

    struct RegisteredPid // 注册过的PID控制器
    {
        const char *Name;
        union {
            PID *PidSimple;
            CascadePID *CascadePid;
            void *PidPtr;
        };
        ManagerPidType Type;
    };

    // 注册过的各种元素
    Vector motorRegistry;
    Vector pidRegistry;

    // 修改数组时的锁
    xSemaphoreHandle managerMutex;

    // 实用函数
    static void printPid(PID *pid);

    // 应用API
    static BUS_REMOTEFUNC(RegisterPidControllerFuncCallback); // [远程函数]注册PID控制器
    static BUS_REMOTEFUNC(RegisterMotorFuncCallback);         // [远程函数]注册电机

    // 命令行API
    static BUS_REMOTEFUNC(PidQueryFuncCallback); // [远程函数]查询所有PID控制器或单个详情
    static BUS_REMOTEFUNC(PidSetParamFuncCallback); // [远程函数]设置PID控制器参数
    static BUS_REMOTEFUNC(MotorQueryFuncCallback); // [远程函数]查询所有电机或单个详情
} manager;

extern "C" void Manager_TaskCallback(void *argument)
{
    manager.Init((ConfItem *)argument);
    vTaskDelete(NULL);
}

void Manager::Init(ConfItem *conf)
{
    // 初始化动态数组
    Vector_Init(motorRegistry, RegisteredMotor);
    Vector_Init(pidRegistry, RegisteredPid);

    // 初始化锁
    managerMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(managerMutex);

    // 连接话题
    Bus_RemoteFuncRegister(this, RegisterMotorFuncCallback, "/manager/register/motor");
    Bus_RemoteFuncRegister(this, RegisterPidControllerFuncCallback, "/manager/register/pid");
    Bus_RemoteFuncRegister(this, PidQueryFuncCallback, "/manager/query/pid");
}

BUS_REMOTEFUNC(Manager::RegisterPidControllerFuncCallback)
{
    U_S(Manager);

    if (!Bus_CheckMapKeysExist(frame, {"name", "type", "ptr"}))
        return false;

    auto pidName = Bus_GetMapValue(frame, "name").Str;
    auto pidType = (ManagerPidType)Bus_GetMapValue(frame, "type").U32;
    auto pidPtr = Bus_GetMapValue(frame, "ptr").Ptr;

    RegisteredPid pidEntry;
    pidEntry.Name = alloc_strcpy(pvPortMalloc, pidName);
    pidEntry.PidPtr = pidPtr;
    pidEntry.Type = pidType;

    FreertosSemaphoreLocker lock(self->managerMutex, portMAX_DELAY);
    Vector_PushBack(self->pidRegistry, pidEntry);
    return true;
}

BUS_REMOTEFUNC(Manager::RegisterMotorFuncCallback)
{
    U_S(Manager);

    if (!Bus_CheckMapKeysExist(frame, {"name", "bus-type", "bus-id", "motor-id", "ptr"}))
        return false;

    auto motorName = Bus_GetMapValue(frame, "name").Str;
    auto busType = (ManagerBusType)Bus_GetMapValue(frame, "bus-type").U32;
    auto busId = Bus_GetMapValue(frame, "bus-id").U32;
    auto motorId = Bus_GetMapValue(frame, "motor-id").U32;
    auto motorPtr = (BasicMotor *)Bus_GetMapValue(frame, "ptr").Ptr;

    RegisteredMotor motorEntry;
    motorEntry.Meta.WordType = 0;
    motorEntry.Meta.BusType = busType;
    motorEntry.Meta.BusId = busId;
    motorEntry.Meta.MotorId = motorId;

#ifdef UIML_STRICT_MODE
    // 检查是否已经把某个总线位置分配给了某个电机，如果有，则使系统崩溃
    // 这也是写这个模块的用意之一，为了防止把一个CAN位置分给了多个电机对象，导致疯车
    Vector_ForEach(self->motorRegistry, it, RegisteredMotor)
    {
        if (it->Meta.WordType == motorEntry.Meta.WordType)
        {
            UimlCrashSystem("Motor registry conflict", it, &motorEntry);
        }
    }
#endif

    motorEntry.Motor = motorPtr;
    motorEntry.Name = alloc_strcpy(pvPortMalloc, motorName);

    FreertosSemaphoreLocker lock(self->managerMutex, portMAX_DELAY);
    Vector_PushBack(self->motorRegistry, motorEntry);
    return true;
}

void Manager::printPid(PID *pid)
{
    printf("  P: %.3f\n  I: %.3f\n  D: %.3f\n  MaxOut: %.3f\n  MaxIntegral: %.3f\n",
           pid->kp,
           pid->ki,
           pid->kd,
           pid->maxOutput,
           pid->maxIntegral);
};

BUS_REMOTEFUNC(Manager::PidQueryFuncCallback)
{
    U_S(Manager);

    int id = -1;
    if (Bus_CheckMapKeyExist(frame, "id"))
        id = Bus_GetMapValue(frame, "id").I32;

    if (id < 0)
    {
        // 打印整个注册过的PID列表
        id = 0;
        Vector_ForEach(self->pidRegistry, it, RegisteredPid)
        {
            printf(" %-4d [%-7s]: %s\n",
                   id,
                   it->Type == PidSimple ? "Simple" : "Cascade",
                   it->Name);
            ++id;
        }
    }
    else
    {
        // 打印指定PID控制器的信息
        RegisteredPid *pidEntry;
        if ((pidEntry = Vector_GetByIndex(self->pidRegistry, id, RegisteredPid)))
        {
            printf("PID #%d ", id);
            switch (pidEntry->Type)
            {
            case PidSimple:
                puts("[Simple]");
                printPid(pidEntry->PidSimple);
                break;

            case PidCascade:
                puts("[Cascade]");
                puts("[Inner]");
                printPid(&pidEntry->CascadePid->inner);
                puts("[Outer]");
                printPid(&pidEntry->CascadePid->outer);
                break;
            }
        }
    }

    return true;
}

BUS_REMOTEFUNC(Manager::PidSetParamFuncCallback)
{
    U_S(Manager);
    FreertosSemaphoreLocker locker(self->managerMutex, portMAX_DELAY);
    int id = -1;

    if (Bus_CheckMapKeyExist(frame, "id"))
        id = Bus_GetMapValue(frame, "id").I32;

    auto regPid = Vector_GetByIndex(self->pidRegistry, id, RegisteredPid);
    PID *pid = nullptr;

    // 取出对应应该设置值的PID对象
    if (regPid->Type == PidCascade)
    {
        // 对串级PID先检查是取内环还是外环
        if (!Bus_CheckMapKeyExist(frame, "topology"))
        {
            puts("Topology not specified (inner, outer) for cascade PID");
            return false;
        }
        else
        {
            auto topology = Bus_GetMapValue(frame, "topology").Str;
            if (!strcmp(topology, "inner"))
                pid = &regPid->CascadePid->inner;
            else if (!strcmp(topology, "outer"))
                pid = &regPid->CascadePid->outer;
            else
            {
                puts("Invalid topology (inner, outer)");
                return false;
            }
        }
    }
    else
    {
        pid = regPid->PidSimple;
    }

    // 写改完的数值
    const SoftBusItemX *item;
    if ((item = Bus_GetMapItem(frame, "p")))
        pid->kp = item->data.F32;
    if ((item = Bus_GetMapItem(frame, "i")))
        pid->ki = item->data.F32;
    if ((item = Bus_GetMapItem(frame, "d")))
        pid->kd = item->data.F32;

    // 打出来修改完的PID数值
    printPid(pid);

    return true;
}

BUS_REMOTEFUNC(Manager::MotorQueryFuncCallback)
{
    U_S(Manager);
    FreertosSemaphoreLocker locker(self->managerMutex, portMAX_DELAY);

    const char *busName = nullptr;
    int id = 0;

    Vector_ForEach(self->motorRegistry, it, RegisteredMotor)
    {
        switch (it->Meta.BusType)
        {
        case BusCan:
            busName = "CAN";
            break;
        case BusTimerPwm:
            busName = "TIMER";
            break;
        default:
            busName = "(Unknown bus)";
        }

        printf("%-4d: %s %d - 0x%x\n", id, busName, it->Meta.BusId, it->Meta.MotorId);
        ++id;
    }
}
