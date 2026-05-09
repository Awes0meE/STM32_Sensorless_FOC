/**********************************

**********************************/
#include "main.h"
#include "low_task.h"



u16 hz_100_cnt = 0;
uint8_t motor_start_stop = 0;
uint8_t motor_start_stop_pre = 1;
volatile uint8_t motor_control_ready = 0u;
COMPRESSOR_STATE_DEF compressor_state = COMPRESSOR_STATE_IDLE;
uint8_t compressor_fault_code = COMPRESSOR_FAULT_NONE;
float compressor_target_speed_hz = COMPRESSOR_DEFAULT_SPEED_HZ;
float compressor_ec11_target_rpm = COMPRESSOR_EC11_DEFAULT_RPM;
float compressor_ec11_target_hz = COMPRESSOR_DEFAULT_SPEED_HZ;
uint32_t compressor_restart_wait_ms = 0;

static uint32_t compressor_state_ms = 0;
static uint32_t compressor_stall_ms = 0;
static uint8_t ec11_last_state = 0xffu;
static int8_t ec11_substep_accum = 0;
static volatile int16_t ec11_detent_delta = 0;
static uint32_t compressor_auto_test_delay_ms = 0u;
static uint32_t compressor_auto_test_run_ms = 0u;
static uint8_t compressor_auto_test_done = 0u;

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

static float compressor_limit_rpm(float rpm)
{
  if(rpm < COMPRESSOR_EC11_MIN_RPM)
  {
    return COMPRESSOR_EC11_MIN_RPM;
  }
  if(rpm > COMPRESSOR_EC11_MAX_RPM)
  {
    return COMPRESSOR_EC11_MAX_RPM;
  }
  return rpm;
}

static uint8_t ec11_encoder_read_state(void)
{
  uint8_t state = 0u;

  if(GPIO_ReadInputDataBit(EC11_A_GPIO_PORT, EC11_A_PIN) != Bit_RESET)
  {
    state |= 0x01u;
  }
  if(GPIO_ReadInputDataBit(EC11_B_GPIO_PORT, EC11_B_PIN) != Bit_RESET)
  {
    state |= 0x02u;
  }
  return state;
}

void ec11_encoder_sample_isr(void)
{
#if EC11_ENCODER_ENABLE
  static const int8_t quad_table[16] =
  {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };
  uint8_t state;
  uint8_t index;
  int8_t step;

  state = ec11_encoder_read_state();
  if(ec11_last_state == 0xffu)
  {
    ec11_last_state = state;
    return;
  }

  index = (uint8_t)((ec11_last_state << 2) | state);
  step = quad_table[index];
  if(step != 0)
  {
    ec11_substep_accum += (int8_t)(step * COMPRESSOR_EC11_DIRECTION);
    if(ec11_substep_accum >= 4)
    {
      ec11_detent_delta++;
      ec11_substep_accum = 0;
    }
    else if(ec11_substep_accum <= -4)
    {
      ec11_detent_delta--;
      ec11_substep_accum = 0;
    }
  }
  else if(state == ec11_last_state)
  {
    ec11_substep_accum = 0;
  }
  ec11_last_state = state;
#endif
}

static int16_t ec11_encoder_take_delta(void)
{
  int16_t delta;

  __disable_irq();
  delta = ec11_detent_delta;
  ec11_detent_delta = 0;
  __enable_irq();
  return delta;
}

static void compressor_update_ec11_target(void)
{
#if EC11_ENCODER_ENABLE
  int16_t delta;

  if(ec11_last_state == 0xffu)
  {
    ec11_last_state = ec11_encoder_read_state();
  }

  delta = ec11_encoder_take_delta();
  if(delta != 0)
  {
    compressor_ec11_target_rpm += (float)delta * COMPRESSOR_EC11_STEP_RPM;
    compressor_ec11_target_rpm = compressor_limit_rpm(compressor_ec11_target_rpm);
  }
#endif
  compressor_ec11_target_hz = COMPRESSOR_RPM_TO_EHZ(compressor_ec11_target_rpm);
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
  motor_control_ready = 0u;
  compressor_fault_code = COMPRESSOR_FAULT_NONE;
  if(compressor_state == COMPRESSOR_STATE_FAULT)
  {
    compressor_state = COMPRESSOR_STATE_IDLE;
  }
  motor_start_stop = 0;
  motor_start_stop_pre = 0;
  motor_run_display_flag = 0;
  compressor_open_loop_reset();
}

void compressor_fault_trip(uint8_t fault_code)
{
  if(fault_code == COMPRESSOR_FAULT_NONE)
  {
    return;
  }

  compressor_fault_code = fault_code;
  motor_control_ready = 0u;
  compressor_state = COMPRESSOR_STATE_FAULT;
  compressor_state_ms = 0;
  compressor_stall_ms = 0;
  compressor_restart_wait_ms = COMPRESSOR_RESTART_DELAY_MS;
  motor_start_stop = 0;
  motor_start_stop_pre = 0;
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);
  compressor_open_loop_reset();

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

static void compressor_cycle_open_loop_target(void)
{
  if(compressor_open_loop_target_hz < COMPRESSOR_OPEN_LOOP_CAL_HZ_MID)
  {
    compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_CAL_HZ_MID;
  }
  else if(compressor_open_loop_target_hz < COMPRESSOR_OPEN_LOOP_CAL_HZ_HIGH)
  {
    compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_CAL_HZ_HIGH;
  }
  else
  {
    compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_CAL_HZ_LOW;
  }
}

static void compressor_auto_test_10ms(void)
{
#if COMPRESSOR_AUTO_TEST_ENABLE
  if(compressor_auto_test_done != 0u)
  {
    return;
  }

  if(compressor_state == COMPRESSOR_STATE_FAULT)
  {
    compressor_auto_test_done = 1u;
    motor_start_stop = 0u;
    return;
  }

  if(motor_start_stop == 0u)
  {
    compressor_auto_test_run_ms = 0u;
    if((compressor_state == COMPRESSOR_STATE_IDLE) &&
       (compressor_restart_wait_ms == 0u))
    {
      if(compressor_auto_test_delay_ms < COMPRESSOR_AUTO_TEST_START_DELAY_MS)
      {
        compressor_auto_test_delay_ms += COMPRESSOR_CONTROL_PERIOD_MS;
      }
      else
      {
        motor_start_stop = 1u;
      }
    }
    return;
  }

  compressor_auto_test_run_ms += COMPRESSOR_CONTROL_PERIOD_MS;
  if(compressor_auto_test_run_ms >= COMPRESSOR_AUTO_TEST_RUN_MS)
  {
    motor_start_stop = 0u;
    compressor_auto_test_done = 1u;
  }
#endif
}

static void compressor_open_loop_stall_check(void)
{
#if COMPRESSOR_OPEN_LOOP_STALL_DETECT_ENABLE
  float ekf_abs_hz;
  float min_ekf_hz;

  if((compressor_state != COMPRESSOR_STATE_STARTING) &&
     (compressor_state != COMPRESSOR_STATE_RUNNING))
  {
    compressor_stall_ms = 0u;
    return;
  }

  if((speed_close_loop_flag != 2u) ||
     (compressor_open_loop_speed_hz < COMPRESSOR_OPEN_LOOP_STALL_MIN_HZ))
  {
    compressor_stall_ms = 0u;
    return;
  }

  ekf_abs_hz = compressor_absf(EKF_Hz);
  min_ekf_hz = compressor_open_loop_speed_hz * COMPRESSOR_OPEN_LOOP_STALL_RATIO_MIN;
  if(ekf_abs_hz < min_ekf_hz)
  {
    compressor_stall_ms += COMPRESSOR_CONTROL_PERIOD_MS;
    if(compressor_stall_ms >= COMPRESSOR_OPEN_LOOP_STALL_CONFIRM_MS)
    {
      compressor_fault_trip(COMPRESSOR_FAULT_STALL);
    }
  }
  else
  {
    compressor_stall_ms = 0u;
  }
#endif
}

static void compressor_control_10ms(void)
{
  uint8_t fault_code;

#if COMPRESSOR_PRODUCT_OPEN_LOOP_ENABLE
  compressor_update_ec11_target();
#endif

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

#if COMPRESSOR_PRODUCT_OPEN_LOOP_ENABLE
  if(compressor_state == COMPRESSOR_STATE_STARTING)
  {
#if COMPRESSOR_AUTO_TEST_HOLD_BOOST_ENABLE
    compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
#else
    compressor_open_loop_target_hz = compressor_ec11_target_hz;
#endif
    compressor_target_speed_hz = compressor_open_loop_target_hz;
    compressor_update_speed_reference();
    if((compressor_open_loop_speed_hz >= (compressor_open_loop_target_hz - 0.5f)) ||
       (compressor_state_ms >= COMPRESSOR_STARTUP_CHECK_MS))
    {
      compressor_state = COMPRESSOR_STATE_RUNNING;
      compressor_state_ms = 0;
      compressor_stall_ms = 0;
#if COMPRESSOR_AUTO_TEST_HOLD_BOOST_ENABLE
      compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
      compressor_target_speed_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
#else
      compressor_open_loop_target_hz = compressor_ec11_target_hz;
      compressor_target_speed_hz = compressor_ec11_target_hz;
#endif
    }
  }
  else if(compressor_state == COMPRESSOR_STATE_RUNNING)
  {
#if COMPRESSOR_AUTO_TEST_HOLD_BOOST_ENABLE
    compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
    compressor_target_speed_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
#else
    compressor_open_loop_target_hz = compressor_ec11_target_hz;
    compressor_target_speed_hz = compressor_ec11_target_hz;
#endif
    compressor_update_speed_reference();
  }
  compressor_open_loop_stall_check();
#else
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
#endif
}

void motor_start(void)
{
  uint8_t fault_code;

  motor_control_ready = 0u;
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
  compressor_update_ec11_target();
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);
  compressor_open_loop_reset();
#if COMPRESSOR_PRODUCT_OPEN_LOOP_ENABLE
#if COMPRESSOR_AUTO_TEST_HOLD_BOOST_ENABLE
  compressor_open_loop_target_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
  compressor_target_speed_hz = COMPRESSOR_OPEN_LOOP_STARTUP_BOOST_HZ;
#else
  compressor_open_loop_target_hz = compressor_ec11_target_hz;
  compressor_target_speed_hz = compressor_ec11_target_hz;
#endif
#else
  compressor_target_speed_hz = compressor_limit_speed(compressor_target_speed_hz);
#endif
  trace_reset();

  compressor_state = COMPRESSOR_STATE_STARTING;
  compressor_fault_code = COMPRESSOR_FAULT_NONE;
  compressor_state_ms = 0;
  compressor_stall_ms = 0;
  motor_run_display_flag = 1;

  TIM_CtrlPWMOutputs(PWM_TIM,ENABLE);
  motor_control_ready = 1u;
}
void motor_stop(void)
{
  uint8_t was_running;

  motor_control_ready = 0u;
  was_running = (compressor_state == COMPRESSOR_STATE_STARTING) ||
                (compressor_state == COMPRESSOR_STATE_RUNNING);
  GPIO_ResetBits(GPIOC,GPIO_Pin_9);
  TIM_CtrlPWMOutputs(PWM_TIM,DISABLE);
  PWM_TIM->CCR1 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR2 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR3 = PWM_TIM_PULSE>>1;
  compressor_reset_control(COMPRESSOR_MIN_SPEED_HZ);
  compressor_open_loop_reset();
#if COMPRESSOR_PRODUCT_OPEN_LOOP_ENABLE
  compressor_update_ec11_target();
  compressor_open_loop_target_hz = compressor_ec11_target_hz;
  compressor_target_speed_hz = compressor_ec11_target_hz;
#endif
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
      display_flag=1;
    }
    key2_flag=0;
  }
  if(key3_flag==1)
  {
#if COMPRESSOR_PRODUCT_OPEN_LOOP_ENABLE
    if(motor_start_stop==0)
    {
      compressor_ec11_target_rpm = COMPRESSOR_EC11_DEFAULT_RPM;
      compressor_update_ec11_target();
      compressor_open_loop_target_hz = compressor_ec11_target_hz;
      compressor_target_speed_hz = compressor_ec11_target_hz;
    }
#else
    if(motor_start_stop==0)
    {
      compressor_cycle_open_loop_target();
    }
#endif
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
    compressor_auto_test_10ms();
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
    trace_sample_10ms();
    TimingDelay_Decrement();
    hz_100_cnt=0;
  }

}
