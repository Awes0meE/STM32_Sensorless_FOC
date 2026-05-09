/**********************************
         
**********************************/
#include "main.h"
#include "exti.h"


//u8 usb_open_flag=0;

u8 key1_flag;
u8 key2_flag;
u8 key3_flag;
void EXTI2_IRQHandler(void)
{
  if(EXTI_GetITStatus(KEY_3_EXTI_LINE) != RESET)
  {
    key3_flag=1;
    EXTI_ClearITPendingBit(KEY_3_EXTI_LINE);
  }
}


void EXTI9_5_IRQHandler(void)
{
  if(EXTI_GetITStatus(KEY_2_EXTI_LINE) != RESET)
  {
    key2_flag=1;
    EXTI_ClearITPendingBit(KEY_2_EXTI_LINE);
  }
#if EC11_ENCODER_ENABLE
  if((EXTI_GetITStatus(EC11_A_EXTI_LINE) != RESET) ||
     (EXTI_GetITStatus(EC11_B_EXTI_LINE) != RESET))
  {
    ec11_encoder_sample_isr();
    EXTI_ClearITPendingBit(EC11_A_EXTI_LINE|EC11_B_EXTI_LINE);
  }
#endif
}




void EXTI4_IRQHandler(void)
{
  if(EXTI_GetITStatus(KEY_1_EXTI_LINE) != RESET)
  {
    
    key1_flag=1;
    EXTI_ClearITPendingBit(KEY_1_EXTI_LINE);
  }
}
