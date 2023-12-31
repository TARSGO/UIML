
#include "../system/yamlparser.h"
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>

constexpr const char *yaml_simple =
    R"(
sys:
  rotate-pid: # 底盘跟随PID
    p: 1.5
    i: 0.0
    d: 0.0
    max-i: 100.0
    max-out: 200.0

chassis:
  task-interval: 2

  info:
    wheelbase: 100.0 # 轴距
    wheeltrack: 100.0 # 轮距
    wheel-radius: 76.0 # 轮子半径
    offset-x: 0.0
    offset-y: 0.0

  move:
    max-vx: 2000.0 # mm/s
      #进一级插入注释测试
    max-vy: 2000.0
    max-vw: 180.0 # deg/s
    x-acc: 1000.0
    y-acc: 1000.0

  motor-fl: # 左前
    type: "M3508"
    id: 1
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

  motor-fr: # 右前
    type: "M3508"
    id: 2
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

  motor-bl: # 左后
    type: "M3508"
    id: 3
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

  motor-br: # 右后
    type: "M3508"
    id: 4
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

gimbal:
  # Yaw Pitch机械零点时电机编码值
  zero-yaw: 4010
  zero-pitch: 5300
  task-interval: 10

  yaw-imu-pid:
    p: -90.0
    i: 0.0
    d: 0.0
    max-i: 50.0
    max-out: 1000.0
  motor-yaw:
    type: "M6020"
    id: 1
    can-x: 1
    speed-pid:
      p: 15.0
      i: 0.0
      d: 0.0
      max-i: 500.0
      max-out: 20000.0

  pitch-imu-pid:
    p: 63.0
    i: 0.0
    d: 0.0
    max-i: 10000.0
    max-out: 20000.0
  motor-pitch:
    type: "M6020"
    id: 4
    can-x: 2
    speed-pid:
      p: 15.0
      i: 0.0
      d: 0.0
      max-i: 500.0
      max-out: 20000.0

shooter:
  task-interval: 10

  trigger-angle: 45.0 # 拨一发弹丸的角度
  trigger-motor:
    type: "M2006"
    name: "trigger-motor"
    id: 6
    can-x: 1
    angle-pid:
      inner:
        p: 10.0
        i: 0.0
        d: 0.0
        max-i: 10000.0
        max-out: 20000.0
      outer:
        p: 10.0
        i: 0.0
        d: 0.0
        max-i: 10000.0
        max-out: 20000.0

  fric-motor-left:
    type: "M3508"
    id: 2
    can-x: 2
    speed-pid:
      p: 10.0
      i: 0.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

  fric-motor-right:
    type: "M3508"
    id: 1
    can-x: 2
    speed-pid:
      p: 10.0
      i: 0.0
      d: 0.0
      max-i: 10000.0
      max-out: 20000.0

rc:
  type: "DT7"
  uart-x: 3

judge:
  uart-x: 6
  task-interval: 150

ins:
  imu:
    type: "BMI088"
    spi-x: 1
    tim-x: 10
    channel-x: 1
    tmp-pid:
      p: 0.15
      i: 0.01
      d: 0.00
      max-i: 0.15
      max-out: 1

# BSP配置
can:
  cans:
    0:
      number: 1
    1:
      number: 2
  repeat-buffers:
    0:
      can-x: 1
      id: 512
      interval: 2
    1:
      can-x: 1
      id: 0x1FF
      interval: 2
    2:
      can-x: 2
      id: 0x200
      interval: 2
    3:
      can-x: 2
      id: 0x1ff
      interval: 2

uart:
  uarts:
    0:
      number: 3
      max-recv-size: 18
    1:
      number: 6
      max-recv-size: 300

spi:
  spis:
    0:
      number: 1
      max-recv-size: 10
      cs:
        0:
          gpio-x: "B"
          pin: 0
          name: "gyro"
        1:
          gpio-x: "A"
          pin: 4
          name: "acc"
 #
testcase:
  minus:
    int: -65537
    float: -123.456
  positive:
    int: 65536
    float: 12.3456
  string:
    hello: "world"
  #
)";

extern size_t BuiltKeyCount;
extern size_t RemovedNamerefCount;

template <typename T> bool CompareYamlValue(const UimlYamlNode *node, T value)
{
    return !memcmp(&(node->I32), &value, sizeof(T));
}
template <> bool CompareYamlValue(const UimlYamlNode *node, const char *value)
{
    return !strcmp(node->Str, value);
}
template <> bool CompareYamlValue(const UimlYamlNode *node, float value)
{
    return (std::abs(node->F32 - value) <= 1e-6);
}
template <> bool CompareYamlValue(const UimlYamlNode *node, double value)
{
    return (std::abs(node->F32 - value) <= 1e-6);
}

template <typename T> void AssertYamlValue(const UimlYamlNode *root, const char *path, T value)
{
    auto node = UimlYamlGetValueByPath(root, path);
    if (node == NULL)
    {
        std::cerr << "FAIL: Requested value of \"" << path << "\" but the item does not exist!\n";
        return;
    }
    if (!CompareYamlValue(node, value))
    {
        std::cerr << "FAIL: Value of \"" << path << "\" expected: " << value
                  << ", got: " << *reinterpret_cast<const T *>(&(node->I32)) << '\n';
    }
}

template <typename T> void AssertYamlValue(UimlYamlNodeObject &&obj, T given)
{
    if (!CompareYamlValue(obj, given))
    {
        std::cerr << "FAIL: Value expected: " << given << ", got: " << obj.get<T>(given) << '\n';
    }
}

void TestYamlParser()
{
    std::cerr << __FUNCTION__ << std::endl;
    std::ofstream ofs("test.yaml");
    ofs.write(yaml_simple, strlen(yaml_simple));
    ofs.close();

    UimlYamlNode *root;

    UimlYamlParse(yaml_simple, &root);

#ifdef UIML_TESTCASE
    std::cerr << "BuiltKeyCount: " << BuiltKeyCount
              << ", RemovedNamerefCount: " << RemovedNamerefCount << std::endl;
    std::cerr << "Hash collision rate: " << (1.0 - double(RemovedNamerefCount) / BuiltKeyCount)
              << std::endl;
#endif

    auto pChassis = UimlYamlGetValue(root->Children, "chassis");
    auto pInfo = UimlYamlGetValue(pChassis->Children, "info");
    auto pOffsetX = UimlYamlGetValue(pInfo->Children, "wheel-radius");

    UimlYamlNodeObject Conf(root);

    AssertYamlValue(root, "/chassis/info/wheel-radius", 76.0f);
    AssertYamlValue(root, "/testcase/minus/int", -65537);
    AssertYamlValue(root, "/testcase/minus/float", -123.456f);
    AssertYamlValue(root, "/testcase/positive/int", 65536);
    AssertYamlValue(root, "/testcase/positive/float", 12.3456f);
    AssertYamlValue(root, "/testcase/string/hello", "world");
    AssertYamlValue(root, "/spi/spis/0/cs/0/name", "gyro");

    // Hex test
    AssertYamlValue(root, "/can/repeat-buffers/0/id", 0x200);
    AssertYamlValue(root, "/can/repeat-buffers/2/id", 0x200);
    AssertYamlValue(root, "/can/repeat-buffers/1/id", 0x1FF);
    AssertYamlValue(root, "/can/repeat-buffers/3/id", 0x1FF);

    AssertYamlValue(Conf["chassis"]["info"]["wheel-radius"], 76.0f);
    AssertYamlValue(Conf["testcase"]["minus"]["int"], -65537);
    AssertYamlValue(Conf["testcase"]["minus"]["float"], -123.456f);
    AssertYamlValue(Conf["testcase"]["positive"]["int"], 65536);
    AssertYamlValue(Conf["testcase"]["positive"]["float"], 12.3456f);
    AssertYamlValue(Conf["testcase"]["string"]["hello"], "world");
    AssertYamlValue(Conf["spi"]["spis"]["0"]["cs"]["0"]["name"], "gyro");

    auto can = UimlYamlGetValue(root->Children, "can");
    AssertYamlValue(can, "cans/0/number", 1);
    AssertYamlValue(root, "spi/spis/0/cs/1/pin", 4);

    std::cerr << "Comment test: " << UimlYamlGetValueByPath(root, "/chassis/#ThisIsAComment")
              << std::endl;
}
