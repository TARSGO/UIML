
#include "managerapi.h"
#include "motor.h"
#include "softbus.h"
#include "softbus_cppapi.h"

void CanMotor::RegisterCanMotor(const char *name)
{
    Bus_RemoteFuncCall("/manager/motor/register",
                       {{"name", {.Str = name}},
                        {"bus-type", {.U32 = BusCan}},
                        {"bus-id", {.U32 = m_canInfo.canX}},
                        {"motor-id", {.U32 = m_canInfo.rxID}},
                        {"ptr", {this}}});
}
