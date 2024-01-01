
#include "motor.h"
#include "private/createfor.h"
#include <string.h>

#define CREATE_FOR_MALLOC MOTOR_MALLOC_PORT

BasicMotor *BasicMotor::Create(ConfItem *conf)
{
    BasicMotor *ret = NULL;
    auto motorTypeString = Conf_GetValue(conf, "type", const char *, "");

    CREATE_FOR_BEGIN();
    CREATE_FOR(motorTypeString, "M3508", M3508);
    CREATE_FOR(motorTypeString, "M2006", M2006);
    CREATE_FOR(motorTypeString, "M6020", M6020);
    // TODO: Servo, DcMotor
    CREATE_FOR_END();

    // 一个都没创建出来
    if (ret == NULL)
    {
        ret = (DummyMotor *)CREATE_FOR_MALLOC(sizeof(DummyMotor));
        new (ret) DummyMotor();
    }

    return ret;
}

bool BasicMotor::SetMode(MotorCtrlMode mode)
{
    if (m_mode == MOTOR_STOP_MODE)
        return false;

    m_mode = mode;
    return true;
}

float BasicMotor::GetData(MotorDataType type)
{
    switch (type)
    {
    case Direction:
        return m_direction;
        break;

    default:
        return 0.0f;
    }
}
