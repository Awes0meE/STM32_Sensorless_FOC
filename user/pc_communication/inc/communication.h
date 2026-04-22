#ifndef __COMMUICATION_H_
#define __COMMUICATION_H_





extern void communication_init(void);
extern void communication_handle(void);
extern void communication_task(void);
extern void float_wave_upload_init(u8 index,float* data);
extern void int_wave_upload_init(u8 index,u8 type,int* data);
extern void float_data_upload_init(u8 index,float* data);
extern void int_data_upload_init(u8 index,u8 type,int* data);
extern void float_data_download_init(u8 index,float* data);
extern void speed_ref_data_download_init(float* data);
extern void motor_start_stop_data_download_init(u8* data);
#endif
