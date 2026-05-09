#ifndef __LOW_TASK_H_
#define __LOW_TASK_H_

#include "stm32f4xx.h"

#define COMPRESSOR_FAULT_NONE         0u
#define COMPRESSOR_FAULT_UNDERVOLTAGE 1u
#define COMPRESSOR_FAULT_OVERVOLTAGE  2u
#define COMPRESSOR_FAULT_OVERCURRENT  3u
#define COMPRESSOR_FAULT_STALL        4u
#define COMPRESSOR_FAULT_RESTART_WAIT 5u

typedef enum
{
  COMPRESSOR_STATE_IDLE = 0,
  COMPRESSOR_STATE_STARTING,
  COMPRESSOR_STATE_RUNNING,
  COMPRESSOR_STATE_FAULT
} COMPRESSOR_STATE_DEF;

extern uint8_t motor_start_stop;
extern volatile uint8_t motor_control_ready;
extern COMPRESSOR_STATE_DEF compressor_state;
extern uint8_t compressor_fault_code;
extern float compressor_target_speed_hz;
extern float compressor_ec11_target_rpm;
extern float compressor_ec11_target_hz;
extern uint32_t compressor_restart_wait_ms;

void ec11_encoder_sample_isr(void);
void compressor_clear_fault(void);
void compressor_fault_trip(uint8_t fault_code);

#endif
