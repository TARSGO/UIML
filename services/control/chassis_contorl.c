#include "slope.h"
#include "pid.h"
#include "config.h"
#include "softbus.h"
#include "cmsis_os.h"
#include "motor.h"

#include <stdbool.h>
// ����ģʽ
typedef enum
{
    ChassisMode_Follow,   // ���̸�����̨ģʽ
    ChassisMode_Spin,     // С����ģʽ
    ChassisMode_Snipe_10, // �ѻ�ģʽ10m����������̨��45�ȼнǣ����ٴ������,���DPS�������
    ChassisMode_Snipe_20  // 20m
} ChassisMode;

typedef struct Chassis
{
    // �����ƶ���Ϣ
    struct Move
    {
        float vx; // ��ǰ����ƽ���ٶ� mm/s
        float vy; // ��ǰǰ���ƶ��ٶ� mm/s
        float vw; // ��ǰ��ת�ٶ� rad/s

        float maxVx, maxVy, maxVw; // ������������ٶ�
        Slope xSlope, ySlope;      // б��
    } move;
    float speedRto; // �����ٶȰٷֱ� ���ڵ����˶�
    // ��ת��Ϣ
    struct
    {
        PID pid;             // ��תPID����relativeAngle���������ת�ٶ�
        float relativeAngle; // ��̨����̵�ƫ��� ��λ��
        int16_t InitAngle;   // ��̨����̶���ʱ�ı�����ֵ
        int16_t nowAngle;    // ��ʱ��̨�ı�����ֵ
        ChassisMode mode;    // ����ģʽ
    } rotate;
    bool RockerCtrl; //�Ƿ�Ϊң��������

} Chassis;



//WASD�ƶ�
void Chassis_MoveCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
    Chassis chassis = *(Chassis*) bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char *key_type = SoftBus_GetMapValue(frame, "key");
        switch (*key_type) {
            case 'W':
                Slope_SetTarget(&chassis.move.ySlope, -chassis.move.maxVy * chassis.speedRto);
                break;
            case 'S':
                Slope_SetTarget(&chassis.move.ySlope, chassis.move.maxVy * chassis.speedRto);
                break;
            case 'A':
                Slope_SetTarget(&chassis.move.xSlope, -chassis.move.maxVy * chassis.speedRto);
                break;
            case 'D':
                Slope_SetTarget(&chassis.move.xSlope, chassis.move.maxVy * chassis.speedRto);
                break;
            default:
                break;
        }
    }
}
//ֹͣ�ƶ�
void Chassis_StopCallback(const char* topic, SoftBusFrame* frame, void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = SoftBus_GetMapValue(frame,"key");
        switch (*key_type)
        {
            case 'W':
            case 'S':
                Slope_SetTarget(&chassis.move.ySlope,0);
                break;
            case 'A':
            case 'D':
                Slope_SetTarget(&chassis.move.xSlope,0);
                break;
            default:
                break;
        }
    }
}
//����ģʽ�л�
void Chassis_SwitchModeCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    float speedRto = 1;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = SoftBus_GetMapValue(frame,"key");
        if(chassis.rotate.mode != ChassisMode_Follow && \
        !strcmp(key_type,"Q") || !strcmp(key_type,"E") || !strcmp(key_type,"R"))
        {
            chassis.rotate.mode = ChassisMode_Follow;

        }
        else
        {
            switch (*key_type)
            {
                case 'Q': //Spin
                    PID_Clear(&chassis.rotate.pid);
                    chassis.rotate.mode = ChassisMode_Spin;
                    break;
                case 'E': //snipe-10m
                    chassis.rotate.mode = ChassisMode_Snipe_10;
                    //gimbal&shooter
                    speedRto = 0.2;
                    break;
                case 'R': //snipe-20m
                    chassis.rotate.mode = ChassisMode_Snipe_20;
                    //gimbal&shooter
                    speedRto = 0.1;
                    break;
                default:
                    break;
            }
        }
    }
    SoftBus_Publish("chassis",{
    {"speedRto",&speedRto},
    {"mode",&chassis.rotate.mode}
    });
}
//��/�����ƶ��л�
void Chassis_SwitchSpeedCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/key"))
    {
        char* key_type = (char *) SoftBus_GetMapValue(frame, "key");
        if(chassis.speedRto == 1 && !strcmp(key_type,"C") )
            chassis.speedRto = 0.5;
        else
            chassis.speedRto = 1;
    }
}

//Spin
//void Chassis_RotateCallback(const char* topic,SoftBusFrame* frame,void* bindData)
//{
//    if(!strcmp(topic,"chassis/rotate/mode"))
//    {
//        char* mode_type = (char*) SoftBus_GetMapValue(frame,"mode");
//        if(strcmp(mode_type,ChassisMode_Follow))
//    }
//}

//���Ʒ�ʽ�Ƿ�Ϊң����
void Chassis_isRockerCtrlCallback(const char* topic,SoftBusFrame* frame,void* bindData)
{
    Chassis chassis = *(Chassis*)bindData;
    if(!strcmp(topic,"rc/wheel"))
    {
        int16_t wheel_type = *(int16_t*) SoftBus_GetMapValue(frame,"wheel");
        if(wheel_type < -600)
            chassis.RockerCtrl = false;
        else if(wheel_type > 600)
            chassis.RockerCtrl = true;
    }
}

//���̹�������
void Chassis_ControlTaskCallback(const void* argument)
{
    SoftBus_MultiSubscribe(NULL,Chassis_MoveCallback,{"rc/key/on-pressing"}); //WASD���µ����ƶ�
    SoftBus_MultiSubscribe(NULL,Chassis_StopCallback,{"rc/key/on-up"}); //WASD�ɿ�����ֹͣ
    SoftBus_MultiSubscribe(NULL,Chassis_SwitchModeCallback,{"rc/key/on-down"}); //QER�����л�����ģʽ
    SoftBus_MultiSubscribe(NULL,Chassis_isRockerCtrlCallback,{"rc/wheel"});  //���ֲ���-ң��������
    SoftBus_Subscribe(NULL,Chassis_SwitchSpeedCallback,"rc/key/on-down"); //C����"�¶�"
}

