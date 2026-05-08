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
        OLED_ShowString(0,2,"tgt:");
        OLED_ShowString(0,4,"ekf:");
        OLED_ShowString(0,6,"Iq: . F:");
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
      
      OLED_ShowNum(4*8,2,(u32)(compressor_target_speed_hz*60.0f),4,16);
      
      OLED_ShowNum(4*8,4,(u32)EKF_Hz,3,16);
      
      
      
      OLED_ShowNum(3*8,6,(u32)FOC_Input.Iq_ref,1,16);
      OLED_ShowNum(5*8,6,(u32)(FOC_Input.Iq_ref*100),2,16);
      OLED_ShowNum(10*8,6,(u32)compressor_fault_code,1,16);
      
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
