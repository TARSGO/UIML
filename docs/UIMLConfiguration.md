# UIML 配置文件

## 简介

UIMLv2 的配置项由三部分组成：

- `PeriphHandle* peripheralHandles` 提供的 **外设列表**

  弱符号定义于 [system/config_peripheral.c](../system/config_peripheral.c) 中；

  由用户重新定义于自行复制的 [example/config_peripheral.c](../example/config_peripheral.c) 中。

- `const char *configYaml` 提供的 **YAML 格式配置参数**

  弱符号定义于 [system/config_yaml.cpp](../system/config_yaml.cpp) 中；

  由用户重新定义于自行复制的 [example/config_yaml.cpp](../example/config_yaml.cpp) 中。

- `SERVICE_LIST` **服务列表** X-MACRO 宏

  由于宏没有 \_\_weak 符号覆盖的机制，此宏仅由用户定义于自行复制的 [example/config_services.h](../example/config_services.h) 中，需在 CMakeLists.txt 中指定其路径，然后在编译时，UIML 自行将其拷贝到 `build/UimlAutogen` 目录中一个已知的文件中。

使用覆盖符号、编译前生成头文件等机制是为了：

1. 在开发过程中不必修改 UIML 本体的源码，以便在需要更新 UIML 时只将整个文件夹替换即可完成。
2. 可以让所有机器人的仓库都使用 submodule 机制或者直接引用相同的 UIML 源码，而不需要担心配置项不同导致 push/pull 时产生冲突。

## 外设列表

外设列表编写为形如以下的列表：

```c
{
    {"can1", &hcan1},
    {"can2", &hcan2},

    {"spi1", &hspi1},

    {"uart3", &huart3},
    // {"uart6", &huart6},

    {"gpioA", GPIOA},
    {"gpioB", GPIOB},
    {"gpioC", GPIOC},

    // {"tim4", &htim4},

    {NULL, NULL} // 结束元素
}
```

外设列表为一个个 `PeriphHandle` 类型的结构体组成的数组，第一个成员为 UIML 检索该外设时使用的名称字符串，第二个成员为该外设的 STM32 HAL 句柄指针。对于 GPIO 这样的直接利用宏定义出外设地址的则直接写 `GPIOA` 等。

```c
typedef struct
{
    const char *name;
    void *handle;
} PeriphHandle;
```

## YAML 配置参数

UIMLv2 相比 v1 使用了更加人性化的类 YAML 式配置文件为每个服务指定一些参数。使用了 C++ 11 的原始字符串语法将 YAML 直接嵌入源码中。

使用一个自制的 YAML 解析器解析配置文件内容，此实现中可使用的语法相较 YAML 1.0 作了极大的裁剪。

能够使用的数据类型有：int32，uint32, float, 字符串；字典。一个键只能占用一行。

字符串不支持任何形式的转义，而且必须以双引号包围（不可以使用单引号或者跨行语法）。

浮点数只支持使用 float 型，且不会处理额外的精度问题。如果要将一个数字以 float 型读入，必须写成小数形式（如 100.0）。不能向某个预期浮点数的键中写入整数，否则读取得到的数字将是错误的，且无法检测。

其他实现细节不作展开，请参考 `system/yamlparser.cpp`。

具体使用细节请参考 `example/config_yaml.cpp` 中的样例。对于不同服务使用的不同的配置项格式，参阅对应模块的 README，或者参考样例进行修改。

## 服务列表

服务列表使用 X-MACRO 格式编写，并在不同的地方通过定义不同的 `SERVICE` 宏将其展开成所需的形式。示例如下：

```c
#define SERVICE_LIST                                                 \
  SERVICE(can, BSP_CAN_TaskCallback, osPriorityRealtime, 512)        \
  SERVICE(uart, BSP_UART_TaskCallback, osPriorityNormal, 1024)       \
  SERVICE(spi, BSP_SPI_TaskCallback, osPriorityNormal, 512)          \
  SERVICEX(tim, BSP_TIM_TaskCallback, osPriorityNormal, 512)         \
  SERVICEX(exti, BSP_EXTI_TaskCallback, osPriorityNormal, 256)
```

`SERVICE` 宏的参数含义：

- 服务名称。此名称会用来从 YAML 配置参数中取出根节点下名称相同的配置字典，以及在 `Module` 枚举中生成形如 `svc_服务名称` 的索引值，可用于从 `osThreadId serviceTaskHandle[]` 数组中取出对应服务的线程句柄以及进行依赖项等待。
- 服务入口点。符号需具有 C 链接性（在 C++ 源码中使用 `extern "C"` 修饰）。可以启动多个使用同一入口点的服务。
- FreeRTOS 优先级。
- 栈空间。以字节计。

如需将一个服务禁用，将 `SERVICE` 改为 `SERVICEX`。此后，该服务不会计入 `Module::serviceNum` 计数，不会被分配栈空间。
