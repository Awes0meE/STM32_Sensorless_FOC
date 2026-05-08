/**********************************

**********************************/
#include "main.h"
#include "adc.h"



int ia_test,ib_test,ic_test;
//float err;


double Ia,Ib,Ic;
float Ia_test,Ib_test,Ic_test;
float Vbus;
uint16_t ADC1ConvertedValue[5];
uint16_t i = 0;
uint32_t A_offset,B_offset;
uint8_t get_offset_flag = 0;
uint8_t get_offset_sample_cnt = 0;
u8 oled_display_sample_freq = 0;
u8 speed_close_loop_flag;
float Iq_ref;
float EKF_Hz;

float theta_add;
float theta;

extern float Rs;
extern float Ls;
extern float flux;

void get_offset(uint32_t *a_offset,uint32_t *b_offset)
{
  if(get_offset_sample_cnt<128)
  {
    *a_offset += ADC1->JDR2;
    *b_offset += ADC1->JDR3;
    get_offset_sample_cnt++;
  }
  else
  {
    *a_offset >>= 7;
    *b_offset >>= 7;
    get_offset_sample_cnt=0;
    TIM_CtrlPWMOutputs(PWM_TIM,DISABLE);
    get_offset_flag = 2;
  }
}

void motor_run(void)
{
  float vbus_temp;
  double ia_temp,ib_temp;
  vbus_temp = (float)(ADC1->JDR1);                                //得到母线电压 adc转换值
  ia_temp = (int16_t)((int16_t)A_offset - (int16_t)ADC1->JDR2);   //得到A相电流 adc转换值
  ib_temp = (int16_t)((int16_t)B_offset - (int16_t)ADC1->JDR3);   //得到B相电流 adc转换值
  Vbus = vbus_temp*VBUS_CONVERSION_FACTOR;                        //通过电压转换因子（通过分压电阻得到）把adc转换值 转化为 真实电压
  Ia = ia_temp*SAMPLE_CURR_CON_FACTOR;                            //通过电流转换因子（通过采样电阻和运算放大倍数得到）把adc采样值转化为真实电流值
  Ib = ib_temp*SAMPLE_CURR_CON_FACTOR;                            //通过电流转换因子（通过采样电阻和运算放大倍数得到）把adc采样值转化为真实电流值
  Ic = -Ia-Ib;                                                    //基于基尔霍夫电流定律，根据AB相电流去计算C相电流
  Ia_test = Ia;
  Ib_test = Ib;
  Ic_test = Ic;

  int_test2 = ADC1ConvertedValue[0];

  if(speed_close_loop_flag==0)         //速度环闭环切换控制，电机刚启动时速度环不闭环
  {                                    //并且电流参考值缓慢增加（防冲击），速度达到一定值
    if((Iq_ref<MOTOR_STARTUP_CURRENT)) //速度切入闭环
    {                                  //电流环在电机运行过程中全程闭环
      Iq_ref += 0.00003f;              //角度在电机刚启动时就闭环运行，无需强拖，得益于卡尔曼滤波做
    }                                  //状态观测器低速性能比教好
    else
    {
      speed_close_loop_flag=1;
    }
  }
  else
  {
    if(speed_close_loop_flag==1)
    {
      if(Iq_ref>(MOTOR_STARTUP_CURRENT/2.0f))
      {
        Iq_ref -= 0.001f;
      }
      else
      {
        speed_close_loop_flag=2;
      }
    }
  }


  float_test3 = Speed_Ref*2.0f*PI;

//切换有感或无感方式运行，也就是选择从无感状态观测器方式得到角度和速度信息还是从有感方式得到角度和速度信息
#ifdef  HALL_FOC_SELECT            //通过条件编译选择有感FOC运行，

  if((hall_speed*2.0f*PI)>SPEED_LOOP_CLOSE_RAD_S)     //有感方式运行，霍尔传感器得到角度和速度信息
  {
    FOC_Input.Id_ref = 0.0f;
    Speed_Fdk = hall_speed*2.0f*PI;
    FOC_Input.Iq_ref = Speed_Pid_Out;
  }
  else
  {
    FOC_Input.Id_ref = 0.0f;
    FOC_Input.Iq_ref = Iq_ref;
    Speed_Pid.I_Sum = Iq_ref;
  }
  FOC_Input.theta = hall_angle;
  FOC_Input.speed_fdk = hall_speed*2.0f*PI;

#endif

#ifdef  SENSORLESS_FOC_SELECT            //通过条件编译选择无感FOC运行

  if(FOC_Output.EKF[2]>SPEED_LOOP_CLOSE_RAD_S)      //无感方式运行，状态观测器得到角度和速度信息
  {
    FOC_Input.Id_ref = 0.0f;
    Speed_Fdk = FOC_Output.EKF[2];
    FOC_Input.Iq_ref = Speed_Pid_Out;
  }
  else
  {
    FOC_Input.Id_ref = 0.0f;
    FOC_Input.Iq_ref = Iq_ref;
    Speed_Pid.I_Sum = Iq_ref;
  }
  FOC_Input.theta = FOC_Output.EKF[3];
  FOC_Input.speed_fdk = FOC_Output.EKF[2];

#endif



  EKF_Hz = FOC_Output.EKF[2]/(2.0f*PI);
  FOC_Input.Id_ref = 0.0f;
  FOC_Input.Tpwm = PWM_TIM_PULSE_TPWM;         //FOC运行函数需要用到的输入信息
  FOC_Input.Udc = Vbus;
  FOC_Input.Rs = Rs;
  FOC_Input.Ls = Ls;
  FOC_Input.flux = flux;

  FOC_Input.ia = Ia;
  FOC_Input.ib = Ib;
  FOC_Input.ic = Ic;
  foc_algorithm_step();       //整个FOC运行函数（包括无感状态观测器，电流环，SVPWM，坐标变换，电机参数识别）

  if(motor_start_stop==1)
  {
    PWM_TIM->CCR1 = (u16)(FOC_Output.Tcmp1);     //通过SVPWM得到的占空比赋值给定时器的寄存器
    PWM_TIM->CCR2 = (u16)(FOC_Output.Tcmp2);
    PWM_TIM->CCR3 = (u16)(FOC_Output.Tcmp3);
  }
  else
  {
    PWM_TIM->CCR1 = PWM_TIM_PULSE>>1;
    PWM_TIM->CCR2 = PWM_TIM_PULSE>>1;
    PWM_TIM->CCR3 = PWM_TIM_PULSE>>1;
  }

  drv8301_protection();

  #if PC_COMMUNICATION_ENABLE
  communication_task();
  #endif
  oled_display_sample_freq++;
  if(oled_display_sample_freq == 10)         //电路板自带OLED显示屏 模拟示波器显示波形功能
  {
    if(display_data_flag==0)
    {
      display_data_buff[display_data_buff_cnt]= (s8)(FOC_Output.EKF[3]*15.0f);
      display_data_buff_cnt++;
      if(display_data_buff_cnt==127)
      {
        display_data_buff_cnt=0;
        display_data_flag=1;
      }
    }
    oled_display_sample_freq=0;
  }
}


void ADC_IRQHandler(void)
{

  if((SAMPLE_ADC->SR & ADC_FLAG_JEOC) == ADC_FLAG_JEOC)
  {
    if(get_offset_flag==2)
    {
      hall_angle += hall_angle_add;
      if(hall_angle<0.0f)
      {
        hall_angle += 2.0f*PI;
      }
      else if(hall_angle>(2.0f*PI))
      {
        hall_angle -= 2.0f*PI;
      }
      motor_run();

    }
    else
    {
      if(get_offset_flag==1)
      {
        get_offset(&A_offset,&B_offset);
      }
    }
    ADC_ClearFlag(SAMPLE_ADC, ADC_FLAG_JEOC);
  }
}

