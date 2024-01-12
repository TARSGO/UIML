# 电机

依据 UIML 的原始设计，电机类是设计成面向对象、具有多态性、通过稳定 API 交流的对象。
电机类的设计准则可以理解成能够实现这样的功能：只要在配置文件里设置好参数，可以不需要动代码，就能兼容使用不同云台电机的云台。

在应用中，控制一个电机转动到某个指定位置，应当以以下步骤进行：

1. 服务创建电机对象。电机对象会将自己绑定到某个外设上，以和电机硬件进行通信；
2. 服务按需设置电机对象的控制方式，并根据需要设置其目标值（速度，角度等）；
3. 服务可以调用一些方法设置电机对象的属性，也可以读取。部分属性的值可以改变电机的控制特性，也可以改变反馈值的行为。
4. 如果电机具有反馈（如 CAN 反馈报文），可以从电机对象的属性值中读出反馈信息。

## 数据单位与格式约定

角度：以 **角度（360 度为圆周）** 为准。

速度：以 **RPM** 为准。

## 类继承关系

当前 UIML 内置了一些电机类的实现，在此简要说明。

- BasicMotor （电机基类）
  - DummyMotor （无法生成配置字典中 `type` 对应种类电机时，产生的不实现任何功能的对象）
  - CanMotor （连接在 CAN 总线上的电机，以 CAN 报文交换指令。对象内含 CAN 端点信息结构体
    ）
    - DjiCanMotor （大疆的通用 RoboMaster 电机，它们具有类似的控制逻辑，提取到此类中）
      - M3508
      - M6020
      - M2006
    - DamiaoJ6006 （达妙科技 DM-J6006）
  - DcMotor **（FIXME）**
  - Servo **（FIXME）**

## API

电机基类定义了 6 个纯虚函数，你的电机类需要实现全部的函数。

### `void Init(ConfItem *conf)`

初始化本电机对象。UIML 不对电机类使用构造函数。`conf` 通常应当指向一个电机的配置字典，由拥有该电机对象的服务提供。

如果电机硬件需要特殊序列或其他操作完成初始化，请在此函数内完成。

特别注意：请务必在此调用上一级电机类的构造函数。

### `bool SetMode(MotorCtrlMode mode)`

设置电机的控制模式。定义了以下四种模式：

- 扭矩模式（目标值为扭矩值/电流）

  扭矩模式可以实现为直接向电机发送目标电流值的形式。

  目标值的单位是电机相关的，并没有定义。

- 速度模式（目标值为转速，RPM）

  速度模式可以实现为 PID 速度环（参见 `DjiCanMotor`）。

- 角度模式（目标值为电机上传感器返回的角度值的累积量，度）

  角度模式可以实现为 PID 串级角度环（参见 `DjiCanMotor`）。

- 急停模式（进入后不可再退出，电机会被强制停止输出。被急停后只能通过重置单片机恢复运行）

如果设置了不能使用的模式，此方法应返回 `false`。

### `bool SetTarget(float target)`

设置电机应当执行的目标值。其唯一一个浮点数参数的含义随当前模式不同而改变。见 `bool SetMode(MotorCtrlMode mode)`。

### `bool SetData(MotorDataType type, float angle)`

设置电机的公开属性的值。不是所有电机都应当实现所有的属性。当试图设置未实现的属性时，此方法应返回 `false`。

### `float GetData(MotorDataType type)`

获取电机的公开属性的值。不是所有电机都应当实现所有的属性。当试图获取未实现的属性时，此方法返回 `0.0f`。

### `void EmergencyStop()`

使电机进入急停状态。效果同 `SetMode(ModeStop)`。

## 新增电机

以使用最为广泛的 CAN 总线电机为例。实现可参考 `DjiCanMotor` 与其子类 `M3508`。

CAN 电机一般都具有反馈报文。收到的 CAN 报文会通过软总线广播，我们为了让电机接收到报文，需要先定义一个软总线广播的接收端点。

```cpp
void DjiCanMotor::CanRxCallback(const char *endpoint, SoftBusFrame *frame, void *bindData)
{
    // 校验绑定电机对象
    if (!bindData)
        return;

    U_S(DjiCanMotor); // [ 作用是将bindData转换为(DjiCanMotor*)型指针，名称为self ]

    // 校验电机ID
    if (self->m_canInfo.rxID != Bus_GetListValue(frame, 0).U16)
        return;

    auto data = reinterpret_cast<uint8_t *>(Bus_GetListValue(frame, 1).Ptr);
    if (data)
        self->CanRxUpdate(data); // 更新电机数据
}
```

然后，实现 `Init` 函数。需要在 `Init` 函数中首先给出 CAN 的发送和接收报文的 ID。
当然对于任何电机对象，初始化的第一步都总是应该先从配置字典中读取该电机硬件通信的外设与总线上的地址。

然后，设置默认的电机工作模式变量，初始化私有的状态变量，并订阅 CAN 反馈报文。

```cpp
void DjiCanMotor::Init(ConfItem *dict)
{
    U_C(dict);
    // 公共初始化部分
    // 注意：必须已经由子类设定好了减速比与CAN信息才能到达这里

    // 默认模式为扭矩模式
    m_mode = ModeTorque;

    // 初始化电机pid
    m_speedPid.Init(Conf["speed-pid"]);
    m_anglePid.Init(Conf["angle-pid"]);
    m_anglePid.outer.maxOutput *= m_reductionRatio; // 将输出轴速度限幅放大到转子上

    // 读取电机方向，为1正向为-1反向
    m_direction = Conf["direction"].get<int8_t>(1);
    if (m_direction != -1 && m_direction != 1)
        m_direction = 1;

    // 订阅can信息
    char name[] = "/can_/recv";
    name[4] = m_canInfo.canX + '0';
    Bus_SubscribeTopic(this, DjiCanMotor::CanRxCallback, name);

    m_target = 0.0f;
    m_stallTime = 0;
    m_totalAngle = 0;
    m_motorReducedRpm = 0;
}
```

如果电机需要在单片机上计算 PID 控制，请在此处完成 PID 控制器的初始化。

接下来实现控制部分。如果电机需要单片机对其进行角度反馈控制，那么电机必须有角度反馈能力，且电机类或者电调本身实现了角度累积功能。
UIML 中累积角度值一般称为“Total angle”。`DjiCanMotor` 的实现为：

```cpp
// 统计电机累计转过的圈数
void DjiCanMotor::UpdateAngle()
{
    int32_t angleDiff = 0;
    // 计算角度增量（加过零检测）
    if (m_motorAngle - m_lastAngle < -4000)
        angleDiff = m_motorAngle + (8191 - m_lastAngle);
    else if (m_motorAngle - m_lastAngle > 4000)
        angleDiff = -m_lastAngle - (8191 - m_motorAngle);
    else
        angleDiff = m_motorAngle - m_lastAngle;
    // 将角度增量加入计数器
    m_totalAngle += angleDiff;
    // 记录角度
    m_lastAngle = m_motorAngle;
}
```

此处 `m_totalAngle` 将作为 PID 串级角度控制的反馈值。

如果是自带控制算法的电机，可以不在单片机上实现控制算法，但有一个使用约定：
若电机支持设置串级控制时的速度上限，应通过电机类的 `SpeedLimit` 属性设置。见 `DamiaoJ6006`：

```cpp
bool DamiaoJ6006::SetData(MotorDataType type, float data)
{
    switch (type)
    {
// ...

    case SpeedLimit:
        m_autonomousAngleModeSpeedLimit = data;
        return true;
// ...
```
