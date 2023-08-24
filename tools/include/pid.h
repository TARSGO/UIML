#ifndef _USER_PID_H_
#define _USER_PID_H_

#include "../../conf/config.h"

#include <functional>

#ifndef LIMIT
#define LIMIT(x,min,max) (x)=(((x)<=(min))?(min):(((x)>=(max))?(max):(x)))
#endif

#ifndef ABS
#define ABS(x) ((x)>=0?(x):-(x))
#endif

class PID
{
public:
    float kp,ki,kd;
    float error,lastError;//误差、上次误差
    float integral,maxIntegral;//积分、积分限幅
    float output,maxOutput;//输出、输出限幅
    float deadzone;//死区

    PID(ConfItem* conf);
    void SingleCalc(float reference,float feedback);
    void FeedbackCalc(float reference, float feedback, const std::function<float(float)>& lambda);
    void Clear();
    void SetMaxOutput(float maxOutput);
    void SetMaxIntegral(float maxIntegral);
    void SetDeadzone(float deadzone);
};

class CascadePID
{
public:
    PID inner;//内环
    PID outer;//外环
    float output;//串级输出，等于inner.output

    void CascadeCalc(float angleRef,float angleFdb,float speedFdb);
};


#endif
