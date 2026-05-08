/**********************************

**********************************/
#include "main.h"
#include "low_task.h"



u16 hz_100_cnt = 0;
uint8_t motor_start_stop = 0;
uint8_t motor_start_stop_pre = 1;
COMPRESSOR_STATE_DEF compressor_state = COMPRESSOR_STATE_IDLE;
uint8_t compressor_fault_code = COMPRESSOR_FAULT_NONE;
float compressor_target_speed_hz = COMPRESSOR_DEFAULT_SPEED_HZ;
uint32_t compressor_restart_wait_ms = 0;

static uint32_t compressor_state_ms = 0;
static uint32_t compressor_stall_ms = 0;

static float compressor_absf(float value)
{
  return (value >= 0.0f) ? value : -value;
}

static float compressor_limit_speed(float speed_hz)
{
  if(speed_hz < COMPRESSOR_MIN_SPEED_HZ)
  {
    return COMPRESSOR_MIN_SPEED_HZ;
  }
  if(speed_hz > COMPRESSOR_MAX_SPEED_HZ)
  {
    return COMPRESSOR_MAX_SPEED_HZ;
  }
  return speed_hz;
}

static uint8_t compressor_vbus_fault_code(void)
{
  if(Vbus < COMPRESSOR_VBUS_MIN_V)
  {
    return COMPRESSOR_FAULT_UNDERVOLTAGE;
  }
  if(Vbus > COMPRESSOR_VBUS_MAX_V)
  {
    return COMPRESSOR_FAULT_OVERVOLTAGE;
  }
  return COMPRESSOR_FAULT_NONE;
}

static void compressor_reset_control(float speed_hz)
{
  Speed_Ref = compressor_limit_speed(speed_hz);
  Speed_Fdk = 0.0f;
  Speed_Pid_Out = 0.0f;
  Speed_Pid.I_Sum = 0.0f;
  speed_close_loop_flag = 0;
  Iq_ref = 0.0f;
  hall_angle_add = 0.0005f;
  hall_speed = 0.0f;
}

void compressor_clear_fault(void)
{
  compressor_fault_code = COMPRESSOR_FAULT_NONE;
  if(compressor_state == COMPRESSOR_STATE_FAULT)
  {
    compressor_state = COMPRESSOR_STATE_IDLE;
  }
  motor_start_stop = 0;
  motor_start_stop_pre = 0;
  motor_run_display_flag = 0;
}

void compressor_fault_trip(uint8_t fault_code)
{
  if(fault_code == COMPRESSOR_FAULT_NONE)
  {
    return;
  }

  compressor_fault_code = fault_code;
  compressor_state = COMPRESSOR_STATE_FAULT;
  compressor_state_ms = 0;
  compressor_stall_ms = 0;
  compressor_restart_wait_ms = COMPRESSOR_RESTART_DELAY_MS;
  motor_start_stop = 0;
  motor_start_stop_pre = 0;
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);

  PWM_TIM->CCR1 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR2 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR3 = PWM_TIM_PULSE>>1;
  TIM_CtrlPWMOutputs(PWM_TIM,DISABLE);
  GPIO_ResetBits(GPIOC,GPIO_Pin_9);
  motor_run_display_flag = 0;
}

static void compressor_update_speed_reference(void)
{
  float target_hz;

  target_hz = compressor_limit_speed(compressor_target_speed_hz);
  compressor_target_speed_hz = target_hz;

  if(Speed_Ref + COMPRESSOR_RAMP_STEP_HZ < target_hz)
  {
    Speed_Ref += COMPRESSOR_RAMP_STEP_HZ;
  }
  else if(Speed_Ref > target_hz + COMPRESSOR_RAMP_STEP_HZ)
  {
    Speed_Ref -= COMPRESSOR_RAMP_STEP_HZ;
  }
  else
  {
    Speed_Ref = target_hz;
  }
}

static void compressor_control_10ms(void)
{
  uint8_t fault_code;

  if(compressor_restart_wait_ms > COMPRESSOR_CONTROL_PERIOD_MS)
  {
    compressor_restart_wait_ms -= COMPRESSOR_CONTROL_PERIOD_MS;
  }
  else
  {
    compressor_restart_wait_ms = 0;
  }

  if(motor_start_stop != 1)
  {
    return;
  }

  fault_code = compressor_vbus_fault_code();
  if(fault_code != COMPRESSOR_FAULT_NONE)
  {
    compressor_fault_trip(fault_code);
    return;
  }

  compressor_state_ms += COMPRESSOR_CONTROL_PERIOD_MS;
  compressor_update_speed_reference();

  if(compressor_state == COMPRESSOR_STATE_STARTING)
  {
    if(compressor_state_ms >= COMPRESSOR_STARTUP_CHECK_MS)
    {
      if(compressor_absf(EKF_Hz) < COMPRESSOR_STALL_SPEED_HZ)
      {
        compressor_fault_trip(COMPRESSOR_FAULT_STALL);
      }
      else
      {
        compressor_state = COMPRESSOR_STATE_RUNNING;
        compressor_state_ms = 0;
        compressor_stall_ms = 0;
      }
    }
  }
  else if(compressor_state == COMPRESSOR_STATE_RUNNING)
  {
    if(compressor_absf(EKF_Hz) < COMPRESSOR_STALL_SPEED_HZ)
    {
      compressor_stall_ms += COMPRESSOR_CONTROL_PERIOD_MS;
      if(compressor_stall_ms >= COMPRESSOR_STALL_CONFIRM_MS)
      {
        compressor_fault_trip(COMPRESSOR_FAULT_STALL);
      }
    }
    else
    {
      compressor_stall_ms = 0;
    }
  }
}

void motor_start(void)
{
  uint8_t fault_code;

  if(compressor_state == COMPRESSOR_STATE_FAULT)
  {
    motor_start_stop = 0;
    return;
  }
  if(compressor_restart_wait_ms > 0)
  {
    compressor_fault_code = COMPRESSOR_FAULT_RESTART_WAIT;
    motor_start_stop = 0;
    return;
  }

  fault_code = compressor_vbus_fault_code();
  if(fault_code != COMPRESSOR_FAULT_NONE)
  {
    compressor_fault_trip(fault_code);
    return;
  }

  GPIO_SetBits(GPIOC,GPIO_Pin_9);
  foc_algorithm_initialize();
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);
  compressor_target_speed_hz = compressor_limit_speed(compressor_target_speed_hz);

  TIM_CtrlPWMOutputs(PWM_TIM,ENABLE);

  compressor_state = COMPRESSOR_STATE_STARTING;
  compressor_fault_code = COMPRESSOR_FAULT_NONE;
  compressor_state_ms = 0;
  compressor_stall_ms = 0;
  motor_run_display_flag = 1;
}
void motor_stop(void)
{
  uint8_t was_running;

  was_running = (compressor_state == COMPRESSOR_STATE_STARTING) ||
                (compressor_state == COMPRESSOR_STATE_RUNNING);
  GPIO_ResetBits(GPIOC,GPIO_Pin_9);
  TIM_CtrlPWMOutputs(PWM_TIM,DISABLE);
  PWM_TIM->CCR1 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR2 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR3 = PWM_TIM_PULSE>>1;
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);
  compressor_state = COMPRESSOR_STATE_IDLE;
  compressor_state_ms = 0;
  compressor_stall_ms = 0;
  if((was_running != 0) && (compressor_fault_code == COMPRESSOR_FAULT_NONE))
  {
    compressor_restart_wait_ms = COMPRESSOR_RESTART_DELAY_MS;
  }
  motor_run_display_flag = 0;
}


void low_control_task(void)
{
  if(key1_flag==1)
  {
    if(compressor_state == COMPRESSOR_STATE_FAULT)
    {
      compressor_clear_fault();
    }
    else if(motor_start_stop==0)
    {
      motor_start_stop=1;
    }
    else
    {
      motor_start_stop=0;
    }
    key1_flag=0;
  }
  if(key2_flag==1)
  {
    if(display_flag==0)
    {
      display_flag=1;
    }
    else if(display_flag==1)
    {
      display_flag=2;
    }
    else if(display_flag==2)
    {
      display_flag=0;
    }
    key2_flag=0;
  }
  if(key3_flag==1)
  {
    compressor_target_speed_hz += COMPRESSOR_SPEED_STEP_HZ;
    if(compressor_target_speed_hz > COMPRESSOR_MAX_SPEED_HZ)
    {
      compressor_target_speed_hz = COMPRESSOR_MIN_SPEED_HZ;
    }
    if(motor_start_stop==0)
    {
      Speed_Ref = compressor_target_speed_hz;
    }
    key3_flag=0;
  }

  if(get_offset_flag == 2)
  {
    if(motor_start_stop_pre!=motor_start_stop)
    {
      motor_start_stop_pre=motor_start_stop;
      if(motor_start_stop == 1)
      {
        motor_start();
      }
      else
      {
        motor_stop();
      }

    }
  }

  compressor_control_10ms();
}





void SysTick_Handler(void)
{
  if(drv8301_init_ok_flag==1)
  {
    drv8301_protection();
  }
  Speed_Pid_Calc(Speed_Ref,Speed_Fdk,&Speed_Pid_Out,&Speed_Pid);
  hz_100_cnt++;
  if(hz_100_cnt==10)
  {
    #if PC_COMMUNICATION_ENABLE
    communication_handle();
    #endif
    low_control_task();
    TimingDelay_Decrement();
    hz_100_cnt=0;
  }

}
