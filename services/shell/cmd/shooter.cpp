
#include "../lettershell/shell_cpp.h"
#include "softbus.h"
#include <cstring>
#include <stdio.h>

namespace ShellCmd
{
    static int ShooterHelp()
    {
        puts("Usage:\n"
             "  Set friction wheels motor speed (in RPM):\n"
             "   -> shooter set-fric-rpm 100.0\n"
             "  Set trigger motor stepping angle (in Degrees):\n"
             "   -> shooter set-trig-step 45.0\n");
        return 1;
    }

    static inline int ShooterInvalidFloat()
    {
        puts("Invalid floating point number");
        return 1;
    }

    static inline int ShooterSetFrictionMotorRpm(const char *speedStr)
    {
        float speed = 0.0;
        auto ret = sscanf(speedStr, "%f", &speed);
        if (ret != 1)
            return ShooterInvalidFloat();

        printf("Sets friction motor speed to %.2g RPMs.\n", speed);
        ret = Bus_RemoteFuncCall("/shooter/config", {{"fric-speed", {.F32 = speed}}});
        return !ret;
    }

    static inline int ShooterSetTriggerMotorStep(const char *angleStr)
    {
        float angle = 0.0;
        auto ret = sscanf(angleStr, "%f", &angle);
        if (ret != 1)
            return ShooterInvalidFloat();

        printf("Sets trigger motor stepping to %.2g deg/bullet.\n", angle);
        ret = Bus_RemoteFuncCall("/shooter/config", {{"trigger-angle", {.F32 = angle}}});
        return !ret;
    }

    int Shooter(int argc, char **argv)
    {
        int ret = 0;
        if (argc != 3)
            return ShooterHelp();

        if (!strcmp(argv[1], "set-fric-rpm"))
            return ShooterSetFrictionMotorRpm(argv[2]);
        else if (!strcmp(argv[1], "set-trig-step"))
            return ShooterSetTriggerMotorStep(argv[2]);
        else
            return ShooterHelp();
    }
} // namespace ShellCmd

// clang-format off
SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), shooter, ShellCmd::Shooter, Shooter parameter control);