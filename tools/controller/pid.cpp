/****************PID运算****************/

#include "pid.h"
#include "config.h"
#include "managerapi.h"
#include "softbus.h"

// 默认构造函数
PID::PID()
{
    kp = ki = kd = 0;
    maxIntegral = maxOutput = 0;
    deadzone = 0;
}

// 以配置项构造
PID::PID(ConfItem *conf)
{
    // 调用初始化函数
    Init(conf);
}

// 初始化PID参数
void PID::Init(ConfItem *conf)
{
    U_C(conf);
    kp = Conf["p"].get(0.0f);
    ki = Conf["i"].get(0.0f);
    kd = Conf["d"].get(0.0f);
    maxIntegral = Conf["max-i"].get(0.0f);
    maxOutput = Conf["max-out"].get(0.0f);
    deadzone = 0;
    error = 0;
    lastError = 0;
    integral = 0;
    output = 0;

    // 如果PID对象设置了名称就注册到管理器
    auto name = Conf["name"].get<const char *>(nullptr);
    if (name != nullptr)
    {
        Bus_RemoteFuncCall("/manager/register/pid",
                           {{"name", {.Str = name}},
                            {"type", {.U32 = PidSimple}},
                            {"ptr", {this}}});
    }
}

// 单级PID计算
float PID::SingleCalc(float reference, float feedback)
{
    // 更新数据
    lastError = error;
    if (ABS(reference - feedback) < deadzone) // 若误差在死区内则error直接置0
        error = 0;
    else
        error = reference - feedback;
    // 计算微分
    output = (error - lastError) * kd;
    // 计算比例
    output += error * kp;
    // 计算积分
    integral += error * ki;
    LIMIT(integral, -maxIntegral, maxIntegral); // 积分限幅
    output += integral;
    // 输出限幅
    LIMIT(output, -maxOutput, maxOutput);

    return output;
}

// 前馈pid计算
/*
当逻辑不复杂，没必要或不想给函数起名的时候，我们会推荐用Lambda表达式来代替函数指针Lambda表达式的结构
[capture-list] (params) -> ret(optional) { body }
captures: 捕捉子句，作用是捕捉外部变量
[]，啥都不捕捉，也就是Lambda表达式body中不能出现没在body中声明的变量
[=]，按值捕捉所有变里，也就是lambda表达式前的所有变里，都可以以值传递(拷贝)的方式出现在body中（性能问题，不建议使用）
[&]，按引用捕捉所有变量，也就是Lambda表达式前的所有变量，都可以以引用传递的方式出现在body中（性能问题，不建议使用）
[&a]，按引用捕捉变量a，按值捕捉其他变量
[&，a]，按值捕捉变量a，按引用捕捉其他变量
[=，a]，[&，&a]这样的写法是错误的
params: 和普通函数一样的参数(设置本地参数)
ret: 返回值类型，也可以省略，让编译器通过 return 语句自动推导
body: 函数的具体逻辑
样例：
 [] (int a) ->float { return (a * 10);};
 a * 10可以改成自己所需要的数学模型，然后将lambda表达式整体传入FeedbackCalc函数中，
*/
float PID::FeedforwardCalc(float reference,
                           float feedback,
                           const std::function<float(float)> feedforward)
{
    // 更新数据
    lastError = error;
    if (ABS(reference - feedback) < deadzone) // 若误差在死区内则error直接置0
        error = 0;
    else
        error = reference - feedback;
    // 计算微分
    output = (error - lastError) * kd;
    // 计算比例
    output += error * kp;
    // 计算积分
    integral += error * ki;
    LIMIT(integral, -maxIntegral, maxIntegral); // 积分限幅
    output += integral;
    // 加入前馈
    output += feedforward(reference);
    // 输出限幅
    LIMIT(output, -maxOutput, maxOutput);

    return output;
}

// 清空一个pid的历史数据
void PID::Clear()
{
    error = 0;
    lastError = 0;
    integral = 0;
    output = 0;
}

// 以配置项构造
CascadePID::CascadePID(ConfItem *conf) { Init(conf); }

// 用配置项初始化内外两环。默认内环配置键名"inner"，外环键名"outer"。
void CascadePID::Init(ConfItem *conf)
{
    U_C(conf);

    inner.Init(Conf["inner"]);
    outer.Init(Conf["outer"]);

    // 如果PID对象有名称，注册到管理器
    auto name = Conf["name"].get<const char *>(nullptr);
    if (name != nullptr)
    {
        Bus_RemoteFuncCall("/manager/register/pid",
                           {{"name", {.Str = name}},
                            {"type", {.U32 = PidCascade}},
                            {"ptr", {this}}});
    }
}

// 串级pid计算
float CascadePID::CascadeCalc(float angleRef, float angleFdb, float speedFdb)
{
    outer.SingleCalc(angleRef, angleFdb);     // 计算外环(角度环)
    inner.SingleCalc(outer.output, speedFdb); // 计算内环(速度环)
    output = inner.output;

    return output;
}

// 清空串级PID
void CascadePID::Clear()
{
    outer.Clear();
    inner.Clear();
}
