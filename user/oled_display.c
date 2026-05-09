/**********************************

**********************************/
#include "main.h"
#include "oled_display.h"

u8 usb_open_display_flag;
u8 data_upload_display_flag;
u8 motor_run_display_flag;
u8 motor_run_display_flag_pre=1;
u8 display_static_flag;

u8 init_dispaly_flag;
u8 display_index;
u8 display_index_key;
u8 display_cnt;

u8 display_flag;
u8 clear_display_flag=0;
u8 drv8301_fault_flag = 0;

static void OLED_ShowSignedCurrent(u8 x,u8 y,float value)
{
  float abs_value;
  u32 integer_part;
  u32 decimal_part;

  if(value < 0.0f)
  {
    OLED_ShowString(x,y,"-");
    abs_value = -value;
  }
  else
  {
    OLED_ShowString(x,y," ");
    abs_value = value;
  }

  integer_part = (u32)abs_value;
  decimal_part = (u32)((abs_value - (float)integer_part) * 100.0f + 0.5f);
  if(decimal_part >= 100u)
  {
    integer_part++;
    decimal_part -= 100u;
  }

  OLED_ShowNum(x+1*8,y,integer_part,1,16);
  OLED_ShowString(x+2*8,y,".");
  OLED_ShowNum(x+3*8,y,decimal_part,2,16);
}

static void OLED_ShowSignedHz(u8 x,u8 y,float value)
{
  float abs_value;
  u32 integer_part;

  if(value < 0.0f)
  {
    OLED_ShowString(x,y,"-");
    abs_value = -value;
  }
  else
  {
    OLED_ShowString(x,y," ");
    abs_value = value;
  }

  integer_part = (u32)(abs_value + 0.5f);
  if(integer_part > 999u)
  {
    integer_part = 999u;
  }
  OLED_ShowNum(x+1*8,y,integer_part,3,16);
}

static void OLED_ShowSignedDeg(u8 x,u8 y,float value_rad)
{
  float value_deg;

  value_deg = value_rad * 57.29578f;
  OLED_ShowSignedHz(x,y,value_deg);
}

void oled_display_handle(void)
{
  if(drv8301_fault_flag == 0)
  {
    if(display_flag==0)
    {
      clear_display_flag=0;
      motor_run_display_flag_pre = !motor_run_display_flag;
      OLED_DrawBMP(0,0,128,8,Logo);
    }
    else if(display_flag==1)
    {
      if(clear_display_flag==0)
      {
        OLED_Clear();
        OLED_ShowString(0,2,"ol:    cap:");
        OLED_ShowString(0,4,"ekf:    ph:");
        OLED_ShowString(0,6,"r:      q:     ");
        clear_display_flag=1;
      }
      if((motor_run_display_flag_pre!=motor_run_display_flag) ||
         (compressor_state == COMPRESSOR_STATE_FAULT))
      {
        if(compressor_state == COMPRESSOR_STATE_FAULT)
        {
          OLED_ShowString(0,0,"C:fault ");
        }
        else if(motor_run_display_flag==1)
        {
          OLED_ShowString(0,0,"C:run   ");
        }
        else
        {
          OLED_ShowString(0,0,"C:stop  ");
        }
        motor_run_display_flag_pre = motor_run_display_flag;
      }
      
      OLED_ShowNum(3*8,2,(u32)compressor_open_loop_speed_hz,3,16);
      OLED_ShowNum(11*8,2,(u32)compressor_open_loop_target_hz,3,16);
      
      OLED_ShowSignedHz(4*8,4,EKF_Hz);
      OLED_ShowSignedDeg(12*8,4,ekf_angle_error_rad);
      
      OLED_ShowSignedCurrent(2*8,6,FOC_Input.Iq_ref);
      OLED_ShowSignedCurrent(10*8,6,Current_Idq.Iq);
      
    }
    else if(display_flag==2)
    {
      oled_display();
    }
  }
  else
  {
    OLED_DrawBMP(0,0,128,8,fault);
  }
}
