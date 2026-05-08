#ifndef __ADC_H_
#define __ADC_H_

#define ADC_REF_V                   (float)(3.3f)
#define VBUS_UP_RES                 (float)(100.0f)
#define VBUS_DOWN_RES               (float)(4.99f)
#define VBUS_CONVERSION_FACTOR      (float)(ADC_REF_V*(VBUS_UP_RES+VBUS_DOWN_RES)/VBUS_DOWN_RES/4095.0f)

#define SAMPLE_RES                  (double)(0.020f)
#define AMP_GAIN                    (double)(10.0f)
#define SAMPLE_CURR_CON_FACTOR      (double)(ADC_REF_V/4095.0f/AMP_GAIN/SAMPLE_RES)



extern uint8_t get_offset_flag;
extern float theta;
extern float angle;
extern float Iq_ref;
extern float EKF_Hz;
extern u8 speed_close_loop_flag;
extern uint16_t ADC1ConvertedValue[5];

#endif
