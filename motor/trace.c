/**********************************

**********************************/
#include "main.h"

volatile TRACE_RECORD_DEF trace_buffer[TRACE_BUFFER_SIZE];
volatile TRACE_META_DEF trace_meta = {
  TRACE_MAGIC,
  0u,
  0u,
  0u,
  0u,
  TRACE_BUFFER_SIZE,
  sizeof(TRACE_RECORD_DEF),
  TRACE_SAMPLE_PERIOD_MS
};

static uint32_t trace_time_ms = 0u;
static uint8_t trace_divider = 0u;

static int16_t trace_float_to_i16(float value,float scale)
{
  float scaled;

  scaled = value * scale;
  if(scaled > 32767.0f)
  {
    return 32767;
  }
  if(scaled < -32768.0f)
  {
    return -32768;
  }
  if(scaled >= 0.0f)
  {
    return (int16_t)(scaled + 0.5f);
  }
  return (int16_t)(scaled - 0.5f);
}

static int16_t trace_diag_flags(void)
{
  int16_t flags = 0;

  if(motor_control_ready != 0u)
  {
    flags |= 0x0001;
  }
  if(compressor_aligning_flag != 0u)
  {
    flags |= 0x0002;
  }
  if((compressor_state == COMPRESSOR_STATE_STARTING) ||
     ((COMPRESSOR_OPEN_LOOP_HOLD_ENABLE != 0) && (compressor_state == COMPRESSOR_STATE_RUNNING)))
  {
    flags |= 0x0004;
  }
  if(foc_ekf_update_enable != 0u)
  {
    flags |= 0x0008;
  }
  return flags;
}

void trace_reset(void)
{
  trace_meta.magic = TRACE_MAGIC;
  trace_meta.write_index = 0u;
  trace_meta.wrapped = 0u;
  trace_meta.sample_count = 0u;
  trace_meta.active = 1u;
  trace_meta.buffer_size = TRACE_BUFFER_SIZE;
  trace_meta.record_size = sizeof(TRACE_RECORD_DEF);
  trace_meta.sample_period_ms = TRACE_SAMPLE_PERIOD_MS;
  trace_time_ms = 0u;
  trace_divider = 0u;
}

void trace_sample_10ms(void)
{
  uint32_t index;

  if(trace_meta.active == 0u)
  {
    return;
  }

  trace_divider++;
  if(trace_divider < TRACE_SAMPLE_DIV)
  {
    return;
  }
  trace_divider = 0u;

  index = trace_meta.write_index;
  trace_buffer[index].time_ms = trace_time_ms;
  trace_buffer[index].ol_hz_x10 = trace_float_to_i16(compressor_open_loop_speed_hz,10.0f);
  trace_buffer[index].ekf_hz_x10 = trace_float_to_i16(EKF_Hz,10.0f);
  trace_buffer[index].iq_ref_x100 = trace_float_to_i16(FOC_Input.Iq_ref,100.0f);
  trace_buffer[index].iq_fb_x100 = trace_float_to_i16(Current_Idq.Iq,100.0f);
  trace_buffer[index].id_fb_x100 = trace_float_to_i16(Current_Idq.Id,100.0f);
  trace_buffer[index].ia_x100 = trace_float_to_i16((float)Ia,100.0f);
  trace_buffer[index].ib_x100 = trace_float_to_i16((float)Ib,100.0f);
  trace_buffer[index].ic_x100 = trace_float_to_i16((float)Ic,100.0f);
  trace_buffer[index].vbus_x100 = trace_float_to_i16(Vbus,100.0f);
  trace_buffer[index].vd_x100 = trace_float_to_i16(Voltage_DQ.Vd,100.0f);
  trace_buffer[index].vq_x100 = trace_float_to_i16(Voltage_DQ.Vq,100.0f);
  trace_buffer[index].angle_x1000 = trace_float_to_i16(hall_angle,1000.0f);
  trace_buffer[index].target_hz_x10 = trace_float_to_i16(compressor_open_loop_target_hz,10.0f);
  trace_buffer[index].foc_theta_x1000 = trace_float_to_i16(FOC_Input.theta,1000.0f);
  trace_buffer[index].ekf_angle_x1000 = trace_float_to_i16(FOC_Output.EKF[3],1000.0f);
  trace_buffer[index].valpha_x100 = trace_float_to_i16(FOC_Interface_states.EKF_Interface[0],100.0f);
  trace_buffer[index].vbeta_x100 = trace_float_to_i16(FOC_Interface_states.EKF_Interface[1],100.0f);
  trace_buffer[index].ialpha_x100 = trace_float_to_i16(FOC_Interface_states.EKF_Interface[2],100.0f);
  trace_buffer[index].ibeta_x100 = trace_float_to_i16(FOC_Interface_states.EKF_Interface[3],100.0f);
  trace_buffer[index].diag_flags = trace_diag_flags();
  trace_buffer[index].state = (uint8_t)compressor_state;
  trace_buffer[index].fault = compressor_fault_code;
  trace_buffer[index].motor = motor_start_stop;
  trace_buffer[index].speed_flag = speed_close_loop_flag;

  trace_time_ms += TRACE_SAMPLE_PERIOD_MS;
  trace_meta.sample_count++;
  index++;
  if(index >= TRACE_BUFFER_SIZE)
  {
    index = 0u;
    trace_meta.wrapped = 1u;
  }
  trace_meta.write_index = index;

  if((compressor_state == COMPRESSOR_STATE_FAULT) || (motor_start_stop == 0u))
  {
    trace_meta.active = 0u;
  }
}
