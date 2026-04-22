/**********************************
      
**********************************/
#ifndef __FOC_DEFINE_PARAMETER_H_
#define __FOC_DEFINE_PARAMETER_H_


#define MOTOR_STARTUP_CURRENT   2.0f   //电机启动电流，根据自己实际负载设置 
#define SPEED_LOOP_CLOSE_RAD_S  50.0f  //速度环切入闭环的速度  单位: rad/s

//有感FOC 或 无感FOC选择，总得注释掉其中一个
//#define HALL_FOC_SELECT          //此行注释掉就不使用有感FOC运行
#define SENSORLESS_FOC_SELECT    //此行注释掉就不使用无感感FOC运行


//电机参数配置（电阻，电感，磁链）
#define RS_PARAMETER     0.59f           //电阻
#define LS_PARAMETER     0.001f          //电感
#define FLUX_PARAMETER   0.01150f        //磁链

#endif
