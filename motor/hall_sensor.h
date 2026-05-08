#ifndef __HALL_SENSOR_H_
#define __HALL_SENSOR_H_


#define HALL_TIM_CLOCK (u32)90000000
#define HALL_SAMPLE_FREQ (u32)10000

#define PHASE_SHIFT_ANGLE (float)(60.0f/360.0f*2.0f*PI)         //单位 角度

#define HALL_ANGLE_FACTOR (float)((float)HALL_TIM_CLOCK/(float)HALL_SAMPLE_FREQ*PI/3.0f)

#define HALL_SPEED_FACTOR (float)((float)HALL_TIM_CLOCK/6.0f)


extern float hall_angle;
extern float hall_angle_add;
extern float hall_speed;
#endif
