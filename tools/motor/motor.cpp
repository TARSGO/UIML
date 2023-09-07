
#include "motor.h"
#include <string.h>

#define MOTOR_CREATE_FOR(given, type) \
    if (strcmp((conf->name), (given)) == 0) \
    { \
        ret = (type*)MOTOR_MALLOC_PORT(sizeof(type)); \
        ret->Init(conf); \
        break; \
    } 

BasicMotor* BasicMotor::Create(ConfItem* conf)
{
    BasicMotor* ret = NULL;

    do
    {
        MOTOR_CREATE_FOR("M3508", M3508);
        MOTOR_CREATE_FOR("M2006", M2006);
        MOTOR_CREATE_FOR("M6020", M6020);
        // TODO: Servo, DcMotor
    } while (false);

    // 一个都没创建出来
    if (ret == NULL)
        ret = (DummyMotor*)MOTOR_MALLOC_PORT(sizeof(DummyMotor));

    return ret;
}
