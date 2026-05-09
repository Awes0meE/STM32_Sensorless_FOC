#ifndef __TRACE_H_
#define __TRACE_H_

#include <stdint.h>

#define TRACE_MAGIC            0x54434f46u
#define TRACE_BUFFER_SIZE      1792u
#define TRACE_SAMPLE_DIV       1u
#define TRACE_SAMPLE_PERIOD_MS (10u * TRACE_SAMPLE_DIV)

typedef struct
{
  uint32_t time_ms;
  int16_t ol_hz_x10;
  int16_t ekf_hz_x10;
  int16_t iq_ref_x100;
  int16_t iq_fb_x100;
  int16_t id_fb_x100;
  int16_t ia_x100;
  int16_t ib_x100;
  int16_t ic_x100;
  int16_t vbus_x100;
  int16_t vd_x100;
  int16_t vq_x100;
  int16_t angle_x1000;
  int16_t target_hz_x10;
  int16_t foc_theta_x1000;
  int16_t ekf_angle_x1000;
  int16_t valpha_x100;
  int16_t vbeta_x100;
  int16_t ialpha_x100;
  int16_t ibeta_x100;
  int16_t ekf_angle_err_x1000;
  int16_t ekf_speed_ratio_x1000;
  int16_t diag_flags;
  uint8_t state;
  uint8_t fault;
  uint8_t motor;
  uint8_t speed_flag;
  int16_t handoff_blend_x1000;
  int16_t trip_blend_x1000;
  int16_t trip_angle_err_x1000;
  int16_t trip_speed_ratio_x1000;
  uint8_t trip_reason;
  uint8_t handoff_state;
  int16_t handoff_offset_x1000;
} TRACE_RECORD_DEF;

typedef struct
{
  uint32_t magic;
  uint32_t write_index;
  uint32_t wrapped;
  uint32_t sample_count;
  uint32_t active;
  uint32_t buffer_size;
  uint32_t record_size;
  uint32_t sample_period_ms;
} TRACE_META_DEF;

extern volatile TRACE_RECORD_DEF trace_buffer[TRACE_BUFFER_SIZE];
extern volatile TRACE_META_DEF trace_meta;

void trace_reset(void);
void trace_sample_10ms(void);

#endif
