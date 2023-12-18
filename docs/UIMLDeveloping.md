# UIML 开发说明

**本节讲述如何自行创建模块并添加到框架中。**

## 添加工具类模块

- 工具类模块不与 RTOS 任务相关，也无需接收用户的系统配置，因此创建普通工具类模块**没有特殊要求**，只需建立对应的 .c/h 文件后编写相关接口函数即可

- 对于一些集合多种相似子模块于一体的特殊模块，可以使用**继承和多态**的思想，编写父类模块和子类模块并用 C++ 类创建继承关系，并使用虚函数实现运行时多态，可以参考典型案例 [电机模块](../tools/motor)。[IMU 模块](../tools/imu)亦采用了这样的设计。

## 添加服务类模块

1. **创建一个 .c/.cpp 文件**

   - 如果使用 C++，请将服务入口点标记为 `extern "C"`。
   - 如果要将其包含在 UIML 中使之成为公共功能，可以在 UIML 的 `services` 目录中添加。如果为机器人个性化逻辑，请将其添加到父项目的某处。父项目在链接 UIML 后会继承 UIML 的包含文件，可以使用 UIML 的软总线和其他工具类 API，这使得在统一底层之上再添加个性化逻辑变得更加可行：不影响 UIML 代码，同一大版本内可以直接更新整个 UIML 目录以更新新的统一底层。

2. 在其中**添加一个 FreeRTOS 任务函数**，其中应该使用`while(1)`形成死循环，或在结束时使用`vTaskDelete`销毁自身

   ```c
   void Example_TaskCallback(void *argument)
   {
       Example_Init((ConfItem*)argument);

       while (1)
           Example_DoSomething();
   }
   ```

3. 在进入任务函数后，可使用系统配置模块中提供的接口从参数中**读取配置表**

   - 如果某模块依赖其他模块，需要在其他模块均启动后才能启动，则需要使用 `dependency.h` 提供的依赖启动功能。语法如下：

   ```cpp
   void Example_TaskCallback(void *argument)
   {
       Depends_WaitFor(svc_example,       // 自身
                       {svc_spi, svc_can} // 需要等待的模块
                      );

       // <-- 依赖项全部启动后才会运行到此处

       Example_Init((ConfItem*)argument);
       Depends_SignalFinished(svc_example); // 告知系统自身已初始化完毕

       while (1)
           Example_DoSomething();
   }
   ```

   - 对于 C++ 实现的模块，提供了一个便利宏 `U_C(dict)`，需要传入一个 `ConfItem*` 指针，然后便产生一个 `UimlYamlNodeObject Conf` 对象，可对此对象使用方括号下标语法取出子键，使用 `.get<T>(T defval)` 方法获得值。

   ```cpp
   // 其他实现细节见 system/yamlparser.h 中 UimlYamlNodeObject 实现
   void ExampleCpp_Init(ConfItem *conf)
   {
       U_C(conf);

       auto taskInterval = Conf["task-interval"].get<uint16_t>(10);
       auto exampleName = Conf["name"].get("DefaultName");

       ConfItem* subDict = Conf["subdict"];
   }
   ```

4. 根据需求**编写模块逻辑**，包括服务任务逻辑、注册广播接收器、注册远程函数等

   - 定义远程函数时，可使用 `BUS_REMOTEFUNC` 宏；定义话题回调函数时，可使用 `BUS_TOPICENDPOINT` 宏。C/C++ 均支持。

   ```cpp
   // C
   BUS_REMOTEFUNC(Example_RemoteFuncCallback);
   BUS_TOPICENDPOINT(Example_TopicCallback);

   // C++
   class Example
   {
     public:
       static BUS_REMOTEFUNC(RemoteFuncCallback);
       static BUS_TOPICENDPOINT(TopicCallback);
   }
   ```

   - 对于 C++ 实现的模块，如果要将类中的方法注册为远程函数或者话题回调函数，必须使用静态方法。可以在初始化时将 `this` 存入 `bindData`。

   - 对远程函数和话题回调函数，提供了一个便利宏 `U_S(selfType)`，需要指定一个类型参数，然后宏将会把 `void* bindData` 转换为该类型的指针，名称为 `self`。与上一条中存入 `this` 结合，可以使用此方法用 `self` 代替 `this`。

   ```cpp
   // 取自发射机构 shoot.cpp
   // 开关摩擦轮
   BUS_REMOTEFUNC(Shooter::ToggleFrictionMotorFuncCallback)
   {
       U_S(Shooter);

       if (Bus_CheckMapKeyExist(frame, "enable"))
       {
           bool enable = Bus_GetMapValue(frame, "enable").Bool;
           self->ToggleFrictionMotor(enable);
       }
       return true;
   }
   ```

5. **修改系统配置文件**，将该服务加入列表，然后编写服务的配置参数
6. 编写模块的**说明文档**

# UIML 开发规范

为尽可能增强项目的可读性和可维护性，本项目指定了下述各项开发规范

## 格式规范

代码格式方面，在本队[电控组编码规范](TARS_Go电控组编码规范.md)的基础上，添加了下述规则

- 变量名规定使用**小写开头的驼峰命名法**
- 函数名规定使用**模块名+下划线+大写开头驼峰命名法**
- 代码文件名使用**小写字母+下划线命名法**
- 各模块配置项、软总线中的广播名、远程函数名、软总线映射表数据帧的键均使用**小写字母+减号分隔**

一般的代码格式化问题通过本仓库根目录附带的 `.clang-format` 格式文件为准。VSCode 可使用插件格式化代码。

## 逻辑规范

为减小模块间的耦合度，降低程序逻辑错误概率，添加了下述针对代码逻辑的规范

- 需要一对多通信时，应使用软总线广播，尽量避免多个模块发送相同广播
- 需要多对一通信时，应使用软总线远程函数
- 必须进行多对多通信时，应考虑是否将该事件升级为系统事件，以`/system`作为广播名开头
- 软总线广播中**不应修改传入的数据帧**，而远程函数中可以修改，同时必须要在说明文档中标注

## 说明文档规范

每个模块都必须配有 MarkDown 编写的说明文档`README.md`，放在该模块最上层文件夹下，且可以包含下述章节：

- `模块介绍`：对该模块的介绍，如所描述的对象、使用的算法、代码结构等
- `模块依赖项`：该模块所依赖的文件或其他模块
  - `文件依赖`：要添加该模块需要添加的文件，包含本模块文件、框架文件或库文件等
  - `模块依赖`：添加该模块前需要添加的其他模块，以及使用了这些模块的什么软总线接口
- `准备工作`：添加该模块前的准备工作，如进行 CubeMX 配置等
- `模块配置项`：使用表格展示本模块允许的系统配置项及其格式和含义，可给出配置表示例
- `函数接口`：介绍模块开放的外部函数接口
- `软总线接口`：使用表格展示本模块对接的软总线接口
  - `广播接口`：发送的广播数据帧及其格式
  - `远程函数接口`：注册的远程函数及其所需参数的格式
- `注意事项`：使用该模块时的注意事项

### 说明文档表格示例

- **服务配置表说明表格模板**

  |    配置名    |       数值类型        |     默认值     |         说明         |
  | :----------: | :-------------------: | :------------: | :------------------: |
  | `配置项名称` | 配置项类型，如`float` | 配置项的默认值 | 对该配置项意义的说明 |

- **广播数据帧格式说明表格模板**

  |   数据字段名   |      数据类型       |        说明        |
  | :------------: | :-----------------: | :----------------: |
  | `数据帧字段名` | 字段类型，如`float` | 对该字段意义的说明 |

- **远程函数参数格式说明表格模板**

  | 数据字段名 |       数据类型        | 是否为返回值 | 是否必须传输 |        说明        |
  | :--------: | :-------------------: | :----------: | :----------: | :----------------: |
  |  `参数名`  | 参数值类型，如`float` |   是 / 否    | 可选 / 必需  | 对该参数意义的说明 |
