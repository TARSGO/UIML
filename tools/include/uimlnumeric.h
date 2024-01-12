
#pragma once

#include <math.h>

#ifdef __cplusplus

#include <type_traits>

template <typename T, T lowerBound, T upperBound> inline T BoundCrossDelta(T last, T current)
{
    static_assert(lowerBound < upperBound, "Invalid zero crossing range");
    static_assert(std::is_signed<T>(), "Cannot determine zero crossing delta for unsigned types");
    constexpr T range = upperBound - lowerBound;
    constexpr T halfRange = range / 2;
    const T delta = current - last;

    if (delta < -halfRange)
    {
        // 向上过零
        return range + delta;
    }
    else if (delta > halfRange)
    {
        // 向下过零
        return -range + delta;
    }
    else
    {
        // 没有过零
        return delta;
    }
}

#endif

inline float BoundCrossDeltaF32(float lowerBound, float upperBound, float last, float current)
{
    const auto range = upperBound - lowerBound;
    const auto halfRange = range / 2;
    const auto delta = current - last;

    if (delta < -halfRange)
    {
        // 向上过零
        return range + delta;
    }
    else if (delta > halfRange)
    {
        // 向下过零
        return -range + delta;
    }
    else
    {
        // 没有过零
        return delta;
    }
}

/**
 * @brief 将累积角度值转换为正负180度范围
 *
 * @param deg 累积角度值，单位：度
 * @return float 正负180度范围内的输出
 */
inline float AccumulatedDegTo180(float deg)
{
    deg = fmodf(deg, 360.0f);

    if (deg > 180.0f)
        return deg - 360.0f;
    else
        return deg;
}

/**
 * @brief 将角度转为弧度
 *
 * @param deg 角度值
 * @return float 弧度值
 */
inline float Degree2Radian(float deg) { return deg * 0.0174532f; }

/**
 * @brief 将弧度转为角度
 *
 * @param rad 弧度值
 * @return float 角度值
 */

inline float Radian2Degree(float rad) { return rad * 57.2958f; }

/**
 * @brief 将RPM转为弧度每秒
 *
 * @param rpm RPM值
 * @return float 弧度每秒值
 */
inline float Rpm2Radps(float rpm) { return rpm * 0.10472f; }

/**
 * @brief 将弧度每秒转为RPM
 *
 * @param radps 弧度每秒值
 * @return float RPM值
 */
inline float Radps2Rpm(float radps) { return radps * 9.5493f; }
