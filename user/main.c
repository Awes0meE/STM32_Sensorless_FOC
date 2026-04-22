/**********************************
           
**********************************/
#include "main.h"

static __IO uint32_t uwTimingDelay;



int main(void)
{
  hard_init();
  OLED_Init();
  drv8301_init();
  pc_communication_init();
  foc_algorithm_initialize();
  if(get_offset_flag==0)
  {
    get_offset_flag = 1;
    TIM_CtrlPWMOutputs(PWM_TIM,ENABLE);
  }
  
  while (1)
  {
    drv8301_protection();
    oled_display_handle();
  }
}


void Delay(__IO uint32_t nTime)
{ 
  uwTimingDelay = nTime;
  
  while(uwTimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
  if (uwTimingDelay != 0x00)
  { 
    uwTimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT


void assert_failed(uint8_t* file, uint32_t line)
{ 
  
  while (1)
  {
  }
}
#endif
