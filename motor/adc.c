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
float ekf_angle_error_rad;
float ekf_speed_ratio;
float ekf_handoff_blend;
uint8_t ekf_handoff_ready;
uint8_t ekf_handoff_speed_ok;
uint8_t ekf_handoff_angle_ok;
uint8_t ekf_handoff_state = EKF_HANDOFF_STATE_OPEN_LOOP;
float compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_DEFAULT_HZ;
uint8_t compressor_aligning_flag = 0u;

float theta_add;
float theta;

extern float Rs;
extern float Ls;
extern float flux;

static uint16_t compressor_vbus_fault_cnt = 0;
static uint16_t compressor_current_fault_cnt = 0;
float compressor_open_loop_speed_hz = COMPRESSOR_OPEN_LOOP_START_HZ;
static uint32_t compressor_open_loop_ticks = 0;
static uint8_t compressor_prev_aligning = 0u;
static uint16_t ekf_handoff_ready_ticks = 0u;

static float adc_absf(float value)
{
  return (value >= 0.0f) ? value : -value;
}

static float adc_wrap_pm_pi(float value)
{
  while(value > PI)
  {
    value -= 2.0f * PI;
  }
  while(value < -PI)
  {
    value += 2.0f * PI;
  }
  return value;
}

static float adc_wrap_0_2pi(float value)
{
  while(value >= (2.0f * PI))
  {
    value -= 2.0f * PI;
  }
  while(value < 0.0f)
  {
    value += 2.0f * PI;
  }
  return value;
}

static float adc_blend_angle(float base,float target,float blend)
{
  return adc_wrap_0_2pi(base + adc_wrap_pm_pi(target - base) * blend);
}

static void adc_update_vbus_from_jdr(void)
{
  Vbus = (float)ADC1->JDR1 * VBUS_CONVERSION_FACTOR;
}

static float compressor_limit_iq(float value)
{
  if(value > COMPRESSOR_IQ_LIMIT_A)
  {
    return COMPRESSOR_IQ_LIMIT_A;
  }
  if(value < -COMPRESSOR_REGEN_IQ_LIMIT_A)
  {
    return -COMPRESSOR_REGEN_IQ_LIMIT_A;
  }
  return value;
}

static uint8_t compressor_is_aligning(void)
{
  return (compressor_open_loop_ticks < (COMPRESSOR_OPEN_LOOP_ALIGN_MS * 10u)) ? 1u : 0u;
}

void compressor_open_loop_reset(void)
{
  compressor_open_loop_speed_hz = COMPRESSOR_OPEN_LOOP_START_HZ;
  compressor_open_loop_ticks = 0;
  compressor_aligning_flag = 0u;
  compressor_prev_aligning = 0u;
  ekf_angle_error_rad = 0.0f;
  ekf_speed_ratio = 0.0f;
  ekf_handoff_blend = 0.0f;
  ekf_handoff_ready = 0u;
  ekf_handoff_speed_ok = 0u;
  ekf_handoff_angle_ok = 0u;
  ekf_handoff_state = EKF_HANDOFF_STATE_OPEN_LOOP;
  ekf_handoff_ready_ticks = 0u;
  hall_angle = 0.0f;
  hall_angle_add = 0.0f;
}

static void ekf_handoff_diag_update(void)
{
  uint8_t running_open_loop;

  running_open_loop = ((motor_start_stop == 1) &&
                       (foc_ekf_update_enable != 0u) &&
                       ((compressor_state == COMPRESSOR_STATE_STARTING) ||
                        ((COMPRESSOR_OPEN_LOOP_HOLD_ENABLE != 0) &&
                         (compressor_state == COMPRESSOR_STATE_RUNNING)))) ? 1u : 0u;

  if(running_open_loop == 0u)
  {
    ekf_angle_error_rad = 0.0f;
    ekf_speed_ratio = 0.0f;
    ekf_handoff_blend = 0.0f;
    ekf_handoff_ready = 0u;
    ekf_handoff_speed_ok = 0u;
    ekf_handoff_angle_ok = 0u;
    ekf_handoff_state = EKF_HANDOFF_STATE_OPEN_LOOP;
    ekf_handoff_ready_ticks = 0u;
    return;
  }

  ekf_angle_error_rad = adc_wrap_pm_pi(FOC_Output.EKF[3] - hall_angle);
  if(adc_absf(compressor_open_loop_speed_hz) > 0.1f)
  {
    ekf_speed_ratio = EKF_Hz / compressor_open_loop_speed_hz;
  }
  else
  {
    ekf_speed_ratio = 0.0f;
  }

  ekf_handoff_speed_ok =
    ((compressor_open_loop_speed_hz >= EKF_HANDOFF_MIN_HZ) &&
     (ekf_speed_ratio >= EKF_HANDOFF_SPEED_RATIO_MIN) &&
     (ekf_speed_ratio <= EKF_HANDOFF_SPEED_RATIO_MAX)) ? 1u : 0u;
  ekf_handoff_angle_ok =
    (adc_absf(ekf_angle_error_rad) <= EKF_HANDOFF_ANGLE_ERR_MAX_RAD) ? 1u : 0u;

  if((ekf_handoff_speed_ok != 0u) && (ekf_handoff_angle_ok != 0u))
  {
    if(ekf_handoff_ready_ticks < EKF_HANDOFF_READY_TICKS)
    {
      ekf_handoff_ready_ticks++;
    }
  }
  else
  {
    ekf_handoff_ready_ticks = 0u;
  }
  ekf_handoff_ready = (ekf_handoff_ready_ticks >= EKF_HANDOFF_READY_TICKS) ? 1u : 0u;

#if EKF_HANDOFF_BLEND_ENABLE
  if(((ekf_handoff_state == EKF_HANDOFF_STATE_BLEND) ||
      (ekf_handoff_state == EKF_HANDOFF_STATE_EKF)) &&
     ((ekf_handoff_speed_ok == 0u) || (ekf_handoff_angle_ok == 0u)))
  {
    ekf_handoff_blend = 0.0f;
    ekf_handoff_state = EKF_HANDOFF_STATE_FALLBACK;
    ekf_handoff_ready_ticks = 0u;
    ekf_handoff_ready = 0u;
    return;
  }

  if(((ekf_handoff_state == EKF_HANDOFF_STATE_OPEN_LOOP) ||
      (ekf_handoff_state == EKF_HANDOFF_STATE_FALLBACK)) &&
     (ekf_handoff_ready != 0u))
  {
    ekf_handoff_state = EKF_HANDOFF_STATE_BLEND;
  }

  if(ekf_handoff_state == EKF_HANDOFF_STATE_BLEND)
  {
    ekf_handoff_blend += 1.0f / (float)EKF_HANDOFF_BLEND_TICKS;
    if(ekf_handoff_blend >= 1.0f)
    {
      ekf_handoff_blend = 1.0f;
      ekf_handoff_state = EKF_HANDOFF_STATE_EKF;
    }
  }
  else if(ekf_handoff_state == EKF_HANDOFF_STATE_EKF)
  {
    ekf_handoff_blend = 1.0f;
  }
  else
  {
    ekf_handoff_blend = 0.0f;
  }
#else
  ekf_handoff_blend = 0.0f;
  ekf_handoff_state = EKF_HANDOFF_STATE_OPEN_LOOP;
#endif
}

static void ekf_handoff_apply_angle(void)
{
  if(ekf_handoff_blend <= 0.0f)
  {
    FOC_Input.theta = hall_angle;
    return;
  }

  FOC_Input.theta = adc_blend_angle(hall_angle,FOC_Output.EKF[3],ekf_handoff_blend);
  FOC_Input.speed_fdk =
    (1.0f - ekf_handoff_blend) * (2.0f * PI * compressor_open_loop_speed_hz) +
    ekf_handoff_blend * FOC_Output.EKF[2];
}

static void compressor_update_open_loop_start(void)
{
  if((motor_start_stop != 1) || (compressor_state == COMPRESSOR_STATE_FAULT))
  {
    compressor_open_loop_reset();
    return;
  }

  compressor_open_loop_ticks++;
  compressor_aligning_flag = compressor_is_aligning();
  if(compressor_is_aligning() != 0u)
  {
    hall_angle_add = 0.0f;
    return;
  }

  if(compressor_open_loop_speed_hz < compressor_open_loop_target_hz)
  {
    compressor_open_loop_speed_hz += COMPRESSOR_OPEN_LOOP_RAMP_HZ_S * FOC_PERIOD;
    if(compressor_open_loop_speed_hz > compressor_open_loop_target_hz)
    {
      compressor_open_loop_speed_hz = compressor_open_loop_target_hz;
    }
  }
  else if(compressor_open_loop_speed_hz > compressor_open_loop_target_hz)
  {
    compressor_open_loop_speed_hz = compressor_open_loop_target_hz;
  }

  hall_angle_add = COMPRESSOR_OPEN_LOOP_DIRECTION * 2.0f * PI * compressor_open_loop_speed_hz * FOC_PERIOD;
}

static void compressor_adc_safety_check(void)
{
  if((motor_start_stop != 1) || (compressor_state == COMPRESSOR_STATE_FAULT))
  {
    compressor_vbus_fault_cnt = 0;
    compressor_current_fault_cnt = 0;
    return;
  }

  if((Vbus < COMPRESSOR_VBUS_MIN_V) || (Vbus > COMPRESSOR_VBUS_MAX_V))
  {
    compressor_vbus_fault_cnt++;
    if(compressor_vbus_fault_cnt >= COMPRESSOR_VBUS_FAULT_COUNT)
    {
      if(Vbus < COMPRESSOR_VBUS_MIN_V)
      {
        compressor_fault_trip(COMPRESSOR_FAULT_UNDERVOLTAGE);
      }
      else
      {
        compressor_fault_trip(COMPRESSOR_FAULT_OVERVOLTAGE);
      }
      return;
    }
  }
  else
  {
    compressor_vbus_fault_cnt = 0;
  }

  if((adc_absf((float)Ia) > COMPRESSOR_PHASE_CURRENT_LIMIT_A) ||
     (adc_absf((float)Ib) > COMPRESSOR_PHASE_CURRENT_LIMIT_A) ||
     (adc_absf((float)Ic) > COMPRESSOR_PHASE_CURRENT_LIMIT_A))
  {
    compressor_current_fault_cnt++;
    if(compressor_current_fault_cnt >= COMPRESSOR_CURRENT_FAULT_COUNT)
    {
      compressor_fault_trip(COMPRESSOR_FAULT_OVERCURRENT);
    }
  }
  else
  {
    compressor_current_fault_cnt = 0;
  }
}

void get_offset(uint32_t *a_offset,uint32_t *b_offset)
{
  adc_update_vbus_from_jdr();
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
  double ia_temp,ib_temp;
  uint8_t aligning;

  adc_update_vbus_from_jdr();
  ia_temp = (int16_t)((int16_t)A_offset - (int16_t)ADC1->JDR2);   //得到A相电流 adc转换值
  ib_temp = (int16_t)((int16_t)B_offset - (int16_t)ADC1->JDR3);   //得到B相电流 adc转换值
  Ia = ia_temp*SAMPLE_CURR_CON_FACTOR;                            //通过电流转换因子（通过采样电阻和运算放大倍数得到）把adc采样值转化为真实电流值
  Ib = ib_temp*SAMPLE_CURR_CON_FACTOR;                            //通过电流转换因子（通过采样电阻和运算放大倍数得到）把adc采样值转化为真实电流值
  Ic = -Ia-Ib;                                                    //基于基尔霍夫电流定律，根据AB相电流去计算C相电流
  Ia_test = Ia;
  Ib_test = Ib;
  Ic_test = Ic;

  int_test2 = ADC1ConvertedValue[0];
  compressor_update_open_loop_start();
  compressor_adc_safety_check();
  if(compressor_state == COMPRESSOR_STATE_FAULT)
  {
    return;
  }

  aligning = compressor_is_aligning();
  if((compressor_prev_aligning != 0u) && (aligning == 0u))
  {
    foc_ekf_reset();
  }
  compressor_prev_aligning = aligning;
  foc_ekf_update_enable = (aligning == 0u) ? 1u : 0u;

  if(aligning != 0u)
  {
    Iq_ref = 0.0f;
    speed_close_loop_flag = 0;
  }
  else if(speed_close_loop_flag==0)    //速度环闭环切换控制，电机刚启动时速度环不闭环
  {                                    //并且电流参考值缓慢增加（防冲击），速度达到一定值
    if((Iq_ref<MOTOR_STARTUP_CURRENT)) //速度切入闭环
    {                                  //电流环在电机运行过程中全程闭环
      Iq_ref += COMPRESSOR_STARTUP_IQ_STEP_A; // Strong-start diagnostic ramp.
      if(Iq_ref > MOTOR_STARTUP_CURRENT)
      {
        Iq_ref = MOTOR_STARTUP_CURRENT;
      }
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
      if(Iq_ref>COMPRESSOR_STARTUP_HOLD_CURRENT)
      {
        Iq_ref -= 0.001f;
      }
      else
      {
        Iq_ref = COMPRESSOR_STARTUP_HOLD_CURRENT;
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
    FOC_Input.Iq_ref = compressor_limit_iq(Speed_Pid_Out);
  }
  else
  {
    FOC_Input.Id_ref = 0.0f;
    FOC_Input.Iq_ref = compressor_limit_iq(Iq_ref);
    Speed_Pid.I_Sum = Iq_ref;
  }
  FOC_Input.theta = hall_angle;
  FOC_Input.speed_fdk = hall_speed*2.0f*PI;

#endif

#ifdef  SENSORLESS_FOC_SELECT            //通过条件编译选择无感FOC运行

#if COMPRESSOR_FORCE_START_ENABLE
  if((compressor_state == COMPRESSOR_STATE_STARTING) ||
     ((COMPRESSOR_OPEN_LOOP_HOLD_ENABLE != 0) && (compressor_state == COMPRESSOR_STATE_RUNNING)))
  {
    if(compressor_is_aligning() != 0u)
    {
      FOC_Input.Id_ref = COMPRESSOR_STARTUP_ID_CURRENT;
      FOC_Input.Iq_ref = 0.0f;
      Speed_Pid.I_Sum = 0.0f;
      FOC_Input.speed_fdk = 0.0f;
    }
    else
    {
      FOC_Input.Id_ref = COMPRESSOR_STARTUP_RUN_ID_CURRENT;
      FOC_Input.Iq_ref = compressor_limit_iq(Iq_ref);
      Speed_Pid.I_Sum = Iq_ref;
      FOC_Input.speed_fdk = 2.0f * PI * compressor_open_loop_speed_hz;
    }
    FOC_Input.theta = hall_angle;
  }
  else
#endif
  if(FOC_Output.EKF[2]>SPEED_LOOP_CLOSE_RAD_S)      //无感方式运行，状态观测器得到角度和速度信息
  {
    FOC_Input.Id_ref = 0.0f;
    Speed_Fdk = FOC_Output.EKF[2];
    FOC_Input.Iq_ref = compressor_limit_iq(Speed_Pid_Out);
    FOC_Input.theta = FOC_Output.EKF[3];
    FOC_Input.speed_fdk = FOC_Output.EKF[2];
  }
  else
  {
    FOC_Input.Id_ref = 0.0f;
    FOC_Input.Iq_ref = compressor_limit_iq(Iq_ref);
    Speed_Pid.I_Sum = Iq_ref;
    FOC_Input.theta = FOC_Output.EKF[3];
    FOC_Input.speed_fdk = FOC_Output.EKF[2];
  }

#endif



  EKF_Hz = FOC_Output.EKF[2]/(2.0f*PI);
  ekf_handoff_diag_update();
  ekf_handoff_apply_angle();
  FOC_Input.Tpwm = PWM_TIM_PULSE_TPWM;         //FOC运行函数需要用到的输入信息
  FOC_Input.Udc = Vbus;
  FOC_Input.Rs = Rs;
  FOC_Input.Ls = Ls;
  FOC_Input.flux = flux;

  FOC_Input.ia = Ia;
  FOC_Input.ib = Ib;
  FOC_Input.ic = Ic;
  foc_algorithm_step();       //整个FOC运行函数（包括无感状态观测器，电流环，SVPWM，坐标变换，电机参数识别）
  EKF_Hz = FOC_Output.EKF[2]/(2.0f*PI);

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
    if((get_offset_flag==2) && (motor_control_ready != 0u))
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

