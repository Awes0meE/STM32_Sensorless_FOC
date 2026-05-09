/**********************************

**********************************/
#ifndef __FOC_DEFINE_PARAMETER_H_
#define __FOC_DEFINE_PARAMETER_H_

#define MOTOR_STARTUP_CURRENT   3.0f
#define COMPRESSOR_STARTUP_ID_CURRENT   3.0f
#define COMPRESSOR_STARTUP_RUN_ID_CURRENT 3.0f
#define COMPRESSOR_STARTUP_HOLD_CURRENT 3.0f
#define COMPRESSOR_STARTUP_IQ_RAMP_A_S  0.375f
#define COMPRESSOR_STARTUP_IQ_STEP_A    (COMPRESSOR_STARTUP_IQ_RAMP_A_S * FOC_PERIOD)
#define SPEED_LOOP_CLOSE_RAD_S  120.0f

// Compressor safe-start profile for 12V bench testing.
#define COMPRESSOR_SAFE_START_ENABLE       1
#define COMPRESSOR_POLE_PAIRS              4u
#define COMPRESSOR_CONTROL_PERIOD_MS       10u
#define COMPRESSOR_MIN_SPEED_HZ            120.0f
#define COMPRESSOR_DEFAULT_SPEED_HZ        120.0f
#define COMPRESSOR_MAX_SPEED_HZ            200.0f
#define COMPRESSOR_SPEED_STEP_HZ           20.0f
#define COMPRESSOR_RAMP_RATE_HZ_S          20.0f
#define COMPRESSOR_RAMP_STEP_HZ            (COMPRESSOR_RAMP_RATE_HZ_S * 0.01f)
#define COMPRESSOR_FORCE_START_ENABLE      1
#define COMPRESSOR_OPEN_LOOP_HOLD_ENABLE   1
#define COMPRESSOR_OPEN_LOOP_ALIGN_MS      1000u
#define COMPRESSOR_OPEN_LOOP_START_HZ      1.0f
#define COMPRESSOR_OPEN_LOOP_MAX_HZ        45.0f
#define COMPRESSOR_OPEN_LOOP_CAL_HZ_LOW    30.0f
#define COMPRESSOR_OPEN_LOOP_CAL_HZ_MID    45.0f
#define COMPRESSOR_OPEN_LOOP_CAL_HZ_HIGH   60.0f
#define COMPRESSOR_OPEN_LOOP_DEFAULT_HZ    COMPRESSOR_OPEN_LOOP_CAL_HZ_LOW
#define COMPRESSOR_OPEN_LOOP_RAMP_HZ_S     1.5f
#define COMPRESSOR_OPEN_LOOP_DIRECTION     1.0f
#define COMPRESSOR_VBUS_MIN_V              9.0f
#define COMPRESSOR_VBUS_MAX_V              15.5f
#define COMPRESSOR_PHASE_CURRENT_LIMIT_A   6.0f
#define COMPRESSOR_IQ_LIMIT_A              4.0f
#define COMPRESSOR_REGEN_IQ_LIMIT_A        2.0f
#define COMPRESSOR_STARTUP_CHECK_MS        40000u
#define COMPRESSOR_STALL_CONFIRM_MS        3000u
#define COMPRESSOR_STALL_SPEED_HZ          8.0f
#define COMPRESSOR_RESTART_DELAY_MS        10000u
#define COMPRESSOR_VBUS_FAULT_COUNT        20u
#define COMPRESSOR_CURRENT_FAULT_COUNT     50u

// FOC mode selection. Enable exactly one mode.
//#define HALL_FOC_SELECT
#define SENSORLESS_FOC_SELECT

// Panasonic 6MD030Z EKF seed values. Rs is per phase; datasheet line-line R is 0.376 ohm.
#define RS_PARAMETER     0.188f
#define LS_PARAMETER     0.00036f
#define FLUX_PARAMETER   0.00580f

// EKF input signs. These only affect the observer, not FOC actuation.
#define EKF_V_ALPHA_SIGN  1.0f
#define EKF_V_BETA_SIGN   1.0f
#define EKF_I_ALPHA_SIGN  1.0f
#define EKF_I_BETA_SIGN   1.0f

#endif
