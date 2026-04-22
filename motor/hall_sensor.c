/**********************************
           
**********************************/
#include "main.h"
#include "hall_sensor.h"
u8 hall_read_temp;

float hall_angle;
float hall_angle_add;
float hall_speed;
void TIM2_IRQHandler(void)
{
  float temp;
  if(TIM_GetFlagStatus(HALL_TIM,TIM_FLAG_CC1)==SET)
  {
    temp = (float)(TIM_GetCapture1(HALL_TIM));
    hall_angle_add = (float)HALL_ANGLE_FACTOR/(float)(temp);
    hall_speed = (float)HALL_SPEED_FACTOR/(float)(temp);
    hall_read_temp = GPIO_ReadInputDataBit(HALL_CH3_GPIO_PORT,HALL_CH3_PIN);
    hall_read_temp |= GPIO_ReadInputDataBit(HALL_CH2_GPIO_PORT,HALL_CH2_PIN)<<1;
    hall_read_temp |= GPIO_ReadInputDataBit(HALL_CH1_GPIO_PORT,HALL_CH1_PIN)<<2;

    if(hall_read_temp==0x05)
    {
      hall_angle = 0.0f+PHASE_SHIFT_ANGLE;
    }
    else if(hall_read_temp==0x04)
    {
      hall_angle = (PI/3.0f)+PHASE_SHIFT_ANGLE;
    }
    else if(hall_read_temp==0x06)
    {
      hall_angle = (PI*2.0f/3.0f)+PHASE_SHIFT_ANGLE;
    }
    else if(hall_read_temp==0x02)
    {
      hall_angle = PI+PHASE_SHIFT_ANGLE;
    }
    else if(hall_read_temp==0x03)
    {
      hall_angle = (PI*4.0f/3.0f)+PHASE_SHIFT_ANGLE;
    }
    else if(hall_read_temp==0x01)
    {
      hall_angle = (PI*5.0f/3.0f)+PHASE_SHIFT_ANGLE;
    }
    if(hall_angle<0.0f)
    {
      hall_angle += 2.0f*PI;
    }
    else if(hall_angle>(2.0f*PI))
    {
      hall_angle -= 2.0f*PI;
    }
    
    TIM_ClearFlag(HALL_TIM,TIM_FLAG_CC1);
  }
  
}
