
#include "config.h"

extern "C"
{
// 配置文件YAML字符串内容
__weak const char *configYaml = R"(
sys:
  rotate-pid: # 底盘跟随PID
    p: 4.0
    i: 0.0
    d: 0.0
    max-i: 100.0
    max-out: 200.0

chassis:
  task-interval: 2

  info:
    wheelbase: 466.0 # 轴距
    wheeltrack: 461.0 # 轮距
    wheel-radius: 76.0 # 轮子半径
    offset-x: 0.0
    offset-y: 0.0

  move:
    max-vx: 2000.0 # mm/s
    max-vy: 2000.0
    max-vw: 180.0 # deg/s
    x-acc: 3000.0
    y-acc: 3000.0

  motor-fl: # 左前
    type: "M3508"
    id: 1
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 15000.0
      max-out: 20000.0

  motor-fr: # 右前
    type: "M3508"
    id: 2
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 15000.0
      max-out: 20000.0

  motor-bl: # 左后
    type: "M3508"
    id: 3
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 15000.0
      max-out: 20000.0

  motor-br: # 右后
    type: "M3508"
    id: 4
    can-x: 1
    speed-pid:
      p: 10.0
      i: 1.0
      d: 0.0
      max-i: 15000.0
      max-out: 20000.0

gimbal:
  task-interval: 10

  yaw-zero: 260.9
  yaw-imu-pid:
    p: 90.0
    i: 0.0
    d: 0.0
    max-i: 50.0
    max-out: 1000.0
  motor-yaw:
    type: "M6020"
    id: 2
    can-x: 1
    direction: -1
    speed-pid:
      p: 15.0
      i: 0.0
      d: 0.0
      max-i: 500.0
      max-out: 20000.0

  pitch-limit:
    up: 40.0
    down: 10.0
  pitch-imu-pid:
    p: 63.0
    i: 0.0
    d: 0.0
    max-i: 10000.0
    max-out: 20000.0
  motor-pitch:
    type: "M6020"
    id: 2
    can-x: 2
    direction: -1
    speed-pid:
      p: 20.0
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
  stick-deadzone: 5

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
  orientation: # IMU安装方向
    fwd: "+x" # 正前向轴
    up: "+z" # 正上方轴

beep:
  timer:
    name: "tim4"
    clockkhz: 168000

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
      id: 511
      interval: 2
    2:
      can-x: 2
      id: 512
      interval: 2
    3:
      can-x: 2
      id: 511
      interval: 2

uart:
  uarts:
    0:
      name: "uart3"
      max-recv-size: 18
    1:
      name: "uart6"
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

#tim:
#  tims:
#    0:
#      number: 4
#      mode: pwm
)";
}
