
#include "imu.h"
#include "private/createfor.h"
#include <cmsis_os.h>

#define CREATE_FOR_MALLOC pvPortMalloc

BasicImu *BasicImu::Create(ConfItem *conf)
{
    U_C(conf);

    BasicImu *ret = nullptr;
    auto imuTypeStr = Conf["type"].get("");

    CREATE_FOR_BEGIN();
    CREATE_FOR(imuTypeStr, "BMI088", Bmi088);
    CREATE_FOR_END();

    if (ret == NULL)
    {
        ret = (DummyImu *)CREATE_FOR_MALLOC(sizeof(DummyImu));
        new (ret) DummyImu();
    }

    return ret;
}
