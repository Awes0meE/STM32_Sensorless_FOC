/**********************************

**********************************/
#ifndef __FOC_DEFINE_PARAMETER_H_
#define __FOC_DEFINE_PARAMETER_H_


#define MOTOR_STARTUP_CURRENT   3.0f    //压缩机低压试机启动电流，先保守限制
#define SPEED_LOOP_CLOSE_RAD_S  120.0f  //速度环切入闭环的速度，单位: rad/s

// Compressor safe-start profile for 12V bench testing.
#define COMPRESSOR_SAFE_START_ENABLE       1
#define COMPRESSOR_POLE_PAIRS              4u
#define COMPRESSOR_CONTROL_PERIOD_MS       10u
#define COMPRESSOR_MIN_SPEED_HZ            30.0f   //1800 rpm
#define COMPRESSOR_DEFAULT_SPEED_HZ        30.0f   //1800 rpm
#define COMPRESSOR_MAX_SPEED_HZ            50.0f   //3000 rpm
#define COMPRESSOR_SPEED_STEP_HZ           5.0f    //300 rpm per key press
#define COMPRESSOR_RAMP_RATE_HZ_S          5.0f
#define COMPRESSOR_RAMP_STEP_HZ            (COMPRESSOR_RAMP_RATE_HZ_S * 0.01f)
#define COMPRESSOR_VBUS_MIN_V              9.0f
#define COMPRESSOR_VBUS_MAX_V              15.5f
#define COMPRESSOR_PHASE_CURRENT_LIMIT_A   6.0f
#define COMPRESSOR_IQ_LIMIT_A              4.0f
#define COMPRESSOR_REGEN_IQ_LIMIT_A        2.0f
#define COMPRESSOR_STARTUP_CHECK_MS        12000u
#define COMPRESSOR_STALL_CONFIRM_MS        2000u
#define COMPRESSOR_STALL_SPEED_HZ          12.0f
#define COMPRESSOR_RESTART_DELAY_MS        10000u
#define COMPRESSOR_VBUS_FAULT_COUNT        20u
#define COMPRESSOR_CURRENT_FAULT_COUNT     50u

//有感FOC 或 无感FOC选择，总得注释掉其中一个
//#define HALL_FOC_SELECT          //此行注释掉就不使用有感FOC运行
#define SENSORLESS_FOC_SELECT    //此行注释掉就不使用无感感FOC运行


//电机参数配置（电阻，电感，磁链）
#define RS_PARAMETER     0.376f          //6MD030Z 数据手册电阻值，后续实测后再修正
#define LS_PARAMETER     0.00020f        //6MD030Z Ld/Lq 估算等效值
#define FLUX_PARAMETER   0.00650f        //由 Ke/Kt 估算的磁链初值

#endif
