
#include "../lettershell/shell_cpp.h"
#include "softbus.h"
#include <stdio.h>

namespace ShellCmd
{
    int GimbalState()
    {
        float pitch, yaw;

        Bus_RemoteFuncCall("/gimbal/query-state", {{"pitch", {&pitch}}, {"yaw", {&yaw}}});
        printf("/gimbal motors:\nPitch %.2f\nYaw %.2f\n", pitch, yaw);

        return 0;
    }
} // namespace ShellCmd

// clang-format off
SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), gimbalstate, ShellCmd::GimbalState, Get gimbal angle state);