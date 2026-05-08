/**********************************

**********************************/
#ifndef __FOC_DEFINE_PARAMETER_H_
#define __FOC_DEFINE_PARAMETER_H_

#define MOTOR_STARTUP_CURRENT   3.0f
#define SPEED_LOOP_CLOSE_RAD_S  120.0f

// Compressor safe-start profile for 12V bench testing.
#define COMPRESSOR_SAFE_START_ENABLE       1
#define COMPRESSOR_POLE_PAIRS              4u
#define COMPRESSOR_CONTROL_PERIOD_MS       10u
#define COMPRESSOR_MIN_SPEED_HZ            30.0f
#define COMPRESSOR_DEFAULT_SPEED_HZ        30.0f
#define COMPRESSOR_MAX_SPEED_HZ            50.0f
#define COMPRESSOR_SPEED_STEP_HZ           5.0f
#define COMPRESSOR_RAMP_RATE_HZ_S          5.0f
#define COMPRESSOR_RAMP_STEP_HZ            (COMPRESSOR_RAMP_RATE_HZ_S * 0.01f)
#define COMPRESSOR_FORCE_START_ENABLE      1
#define COMPRESSOR_OPEN_LOOP_START_HZ      2.0f
#define COMPRESSOR_OPEN_LOOP_MAX_HZ        18.0f
#define COMPRESSOR_OPEN_LOOP_RAMP_HZ_S     3.0f
#define COMPRESSOR_VBUS_MIN_V              9.0f
#define COMPRESSOR_VBUS_MAX_V              15.5f
#define COMPRESSOR_PHASE_CURRENT_LIMIT_A   6.0f
#define COMPRESSOR_IQ_LIMIT_A              4.0f
#define COMPRESSOR_REGEN_IQ_LIMIT_A        2.0f
#define COMPRESSOR_STARTUP_CHECK_MS        20000u
#define COMPRESSOR_STALL_CONFIRM_MS        2000u
#define COMPRESSOR_STALL_SPEED_HZ          6.0f
#define COMPRESSOR_RESTART_DELAY_MS        10000u
#define COMPRESSOR_VBUS_FAULT_COUNT        20u
#define COMPRESSOR_CURRENT_FAULT_COUNT     50u

// FOC mode selection. Enable exactly one mode.
//#define HALL_FOC_SELECT
#define SENSORLESS_FOC_SELECT

// Motor parameter seed values for Panasonic 6MD030Z. Recalibrate on real hardware.
#define RS_PARAMETER     0.376f
#define LS_PARAMETER     0.00020f
#define FLUX_PARAMETER   0.00650f

#endif
