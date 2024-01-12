
#include "../lettershell/shell_cpp.h"
#include "softbus.h"
#include "softbus_cppapi.h"
#include <stdio.h>
#include <string.h>

static inline void PidPrintUsage()
{

    puts("Usage:\n"
         "  Query all PID controllers:\n"
         "  -> pid ?\n"
         "  Query specific PID controller:\n"
         "  -> pid <id>\n"
         "  Set PID parameter:\n"
         "  -> pid <id> [inner/outer] [<p/i/d> <number>] ...\n"
         "     inner/outer required for cascade PID\n"
         "     Example: pid 2 p 5 d 2.5 (Sets kp=5 kd=2.5 on PID 2)");
}

namespace ShellCmd
{
    int Pid(int argc, char **argv)
    {
        if (argc <= 1)
            goto printUsage;

        if (argc == 2)
        {
            int id;
            if (!strcmp(argv[1], "?"))
            {
                // 查询所有
                return !Bus_RemoteFuncCall("/manager/pid/query", {{}});
            }
            else if (sscanf(argv[1], "%d", &id) && id >= 0)
            {
                // 查询单个
                return !Bus_RemoteFuncCall("/manager/pid/query", {{"id", {.I32 = id}}});
            }
            else
            {
                goto printUsage;
            }
        }

        // 参数够多，认为是在设置PID参数
        if (argc >= 4)
        {
            int id, setterBegin = 3, paramBegin = 1;
            char *topology = nullptr;

            // 解析参数，ID，拓扑
            if (sscanf(argv[1], "%d", &id) == 0 || id < 0)
                goto printUsage;
            if (!strcmp(argv[2], "inner") || !strcmp(argv[2], "outer"))
                topology = argv[2];
            else
                setterBegin = 2;

            // 暂时准备一个能传7个参数的参数缓冲区
            static SoftBusItemX paramBuffer[7];
            paramBuffer[0] = {"id", {.I32 = id}};
            if (topology != nullptr)
            {
                paramBuffer[1] = {"topology", {.Str = topology}};
                paramBegin = 2;
            }

            // 剩下的必须是成对的，为2的倍数，而且必须最多为5个赋值
            if ((argc - setterBegin) % 2 || (argc - setterBegin) > 10)
                goto printUsage;

            // 解析设置的值
            for (int i = setterBegin; i < argc; i += 2)
            {
                paramBuffer[paramBegin].key = argv[i];
                if (!sscanf(argv[i + 1], "%f", &paramBuffer[paramBegin].data.F32))
                    goto printUsage;
                paramBegin++;
            }

            // 设置PID参数
            return !_Bus_RemoteFuncCallMap("/manager/pid/set", paramBegin, paramBuffer);
        }

    printUsage:
        PidPrintUsage();
        return 1;
    }
} // namespace ShellCmd

// clang-format off
SHELL_EXPORT_CMD(SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), pid, ShellCmd::Pid, Query and control registered PID controllers);