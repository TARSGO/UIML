
#pragma once

// 设置发射机构的发射模式
//
// Singleshot一次之后，不能再通过调用API的方式返回Idle，但可以提升至Continuous
// Continuous一次之后，不能再返回Singleshot，但可以返回Idle
enum ShooterMode
{
    Idle,       // 回到不发射弹丸的空闲状态
    Singleshot, // 设置后会发出一发弹丸
    Continuous, // 设置后会连续发出弹丸，直到再次设置为Idle
};
