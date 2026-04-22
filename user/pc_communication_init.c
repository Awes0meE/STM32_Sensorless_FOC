/**********************************
       
**********************************/
#include "main.h"
#include "pc_communication_init.h"





int int_test1 = 1;
int int_test2 = 2;
int int_test3 = 3;
int int_test4 = 4;
int int_test5 = 5;
int int_test6 = 6;
int int_test7 = 7;
int int_test8 = 8;
int int_test9 = 9;
int int_test10 = 10;
float float_test1 = 1.0f;
float float_test2 = 2.0f;
float float_test3 = 3.0f;
float float_test4 = 4.0f;
float float_test5 = 5.0f;
float float_test6 = 6.0f;
float float_test7 = 7.0f;
float float_test8 = 8.0f;
float float_test9 = 9.0f;
float float_test10 = 0.0f;

extern float Q_0_0;
extern float Q_1_1;
extern float Q_2_2;
extern float Q_3_3;
extern float R_0_0;
extern float R_1_1;

extern float Rs;
extern float Ls;
extern float flux;
extern int ia_test,ib_test,ic_test;

extern float theta_add;
extern float Ia_test,Ib_test,Ic_test;
void pc_communication_init(void)
{
  communication_init();
  float_wave_upload_init(0,&FOC_Input.theta);
  float_wave_upload_init(1,&FOC_Output.EKF[0]);
  float_wave_upload_init(2,&FOC_Output.EKF[1]);
  float_wave_upload_init(3,&FOC_Output.EKF[2]);
  float_wave_upload_init(4,&Current_Ialpha_beta.Ibeta);
  float_wave_upload_init(5,&FOC_Input.Iq_ref);
  int_wave_upload_init(0,0x06,&ia_test);
  int_wave_upload_init(1,0x06,&int_test2);
  int_wave_upload_init(2,0x06,&int_test3);
  int_wave_upload_init(3,0x06,&int_test4);
  float_data_upload_init(0,&FOC_Output.L_RF[0]);
  float_data_upload_init(1,&FOC_Output.L_RF[1]);
  float_data_upload_init(2,&FOC_Output.L_RF[2]);
  float_data_upload_init(3,&Current_Ialpha_beta.Ibeta);
  float_data_upload_init(4,&float_test2);
  float_data_upload_init(5,&Current_Ialpha_beta.Ialpha);
  float_data_upload_init(6,&Current_Ialpha_beta.Ibeta);
  float_data_upload_init(7,&Q_3_3);
  float_data_upload_init(8,&Q_0_0);
  float_data_upload_init(9,&Q_0_0);
  int_data_upload_init(0,0x06,&int_test1);
  int_data_upload_init(1,0x06,&int_test2);
  int_data_upload_init(2,0x06,&int_test3);
  int_data_upload_init(3,0x06,&int_test4);
  int_data_upload_init(4,0x06,&int_test5);
  int_data_upload_init(5,0x06,&int_test6);
  int_data_upload_init(6,0x06,&int_test7);
  int_data_upload_init(7,0x06,&int_test8);
  int_data_upload_init(8,0x06,&int_test9);
  int_data_upload_init(9,0x06,&int_test10);
  
  float_data_download_init(0,&SPEED_PI_P);
  float_data_download_init(1,&SPEED_PI_I);
  float_data_download_init(2,&R_0_0);
  float_data_download_init(3,&R_1_1);
  float_data_download_init(4,&Q_0_0);
  float_data_download_init(5,&Q_1_1);
  float_data_download_init(6,&Q_2_2);
  float_data_download_init(7,&Q_3_3);
  float_data_download_init(8,&flux);
  float_data_download_init(9,&Q_0_0);
  
  speed_ref_data_download_init(&Speed_Ref);
  motor_start_stop_data_download_init(&motor_start_stop);
}
