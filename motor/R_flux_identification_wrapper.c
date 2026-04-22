/**********************************
          
**********************************/
#if defined(MATLAB_MEX_FILE)
#include "tmwtypes.h"
#include "simstruc_types.h"
#else
#include "rtwtypes.h"
#endif



#include <math.h>
float theta0_1_1;
float theta0_2_1;
float Pn0_1_1;
float Pn0_1_2;
float Pn0_2_1;
float Pn0_2_2;
float x_1_1;
float x_1_2;
float x_1_3;
float x_2_1;
float x_2_2;
float x_2_3;
float h_1_1;
float h_2_1;
float r_K_1_1;
float r_K_2_1;
float Pn1_1_1;
float Pn1_1_2;
float Pn1_2_1;
float Pn1_2_2;
float theta1_1_1;
float theta1_2_1;
float r_temp_1_1;
float r_temp_1_2;
float r_temp_2_1;
float r_temp_2_2;
float r_Uq;

#define u_width 3
#define y_width 1


void R_flux_identification_Start_wrapper(real_T *xD)
{

theta0_1_1 = 0.05f;
theta0_2_1 = 0.01f;

Pn0_1_1 = 0.0008f*2.0f;
Pn0_1_2 = 0.0008f*0.0f;
Pn0_2_1 = 0.0008f*0.0f;
Pn0_2_2 = 0.0008f*2.0f;

x_1_1 = theta0_1_1;
x_2_1 = theta0_2_1;
x_1_2 = Pn0_1_1;
x_1_3 = Pn0_1_2;
x_2_2 = Pn0_2_1;
x_2_3 = Pn0_2_2;

}

void R_flux_identification_Outputs_wrapper(const real32_T *u,
			real32_T *y,
			const real_T *xD)
{

y[0] = x_1_1;
y[1] = x_2_1;

}


void R_flux_identification_Update_wrapper(const real32_T *u,
			real32_T *y,
			real_T *xD)
{


h_1_1 = u[0];
h_2_1 = u[1];
r_Uq = u[2];
Pn0_1_1 = x_1_2;
Pn0_1_2 = x_1_3;
Pn0_2_1 = x_2_2;
Pn0_2_2 = x_2_3;

r_temp_1_1 = 1.0f + (((h_1_1*Pn0_1_1 + h_2_1* Pn0_2_1)*h_1_1) + ((h_1_1*Pn0_1_2 + h_2_1* Pn0_2_2)*h_2_1));
r_temp_1_1 = 1.0f/r_temp_1_1;
r_K_1_1 = (Pn0_1_1*h_1_1+Pn0_1_2*h_2_1)*r_temp_1_1;
r_K_2_1 = (Pn0_2_1*h_1_1+Pn0_2_2*h_2_1)*r_temp_1_1;

r_temp_1_1 = r_K_1_1*h_1_1;
r_temp_1_2 = r_K_1_1*h_2_1;
r_temp_2_1 = r_K_2_1*h_1_1;
r_temp_2_2 = r_K_2_1*h_2_1;
Pn1_1_1 = Pn0_1_1 - (r_temp_1_1*Pn0_1_1+r_temp_1_2*Pn0_2_1);
Pn1_1_2 = Pn0_1_2 - (r_temp_1_1*Pn0_1_2+r_temp_1_2*Pn0_2_2);
Pn1_2_1 = Pn0_2_1 - (r_temp_2_1*Pn0_1_1+r_temp_2_2*Pn0_2_1);
Pn1_2_2 = Pn0_2_2 - (r_temp_2_1*Pn0_1_2+r_temp_2_2*Pn0_2_2);

theta0_1_1 = x_1_1;
theta0_2_1 = x_2_1;

theta1_1_1 = theta0_1_1 + r_K_1_1*(r_Uq - (h_1_1*theta0_1_1*0.999f+h_2_1*theta0_2_1*0.999f));
theta1_2_1 = theta0_2_1 + r_K_2_1*(r_Uq - (h_1_1*theta0_1_1*0.999f+h_2_1*theta0_2_1*0.999f));

x_1_1 = theta1_1_1;
x_2_1 = theta1_2_1;
x_1_2 = Pn1_1_1;
x_1_3 = Pn1_1_2;
x_2_2 = Pn1_2_1;
x_2_3 = Pn1_2_2;

}

