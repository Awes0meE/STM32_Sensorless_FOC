/**********************************
           
**********************************/
#if defined(MATLAB_MEX_FILE)
#include "tmwtypes.h"
#include "simstruc_types.h"
#else
#include "rtwtypes.h"
#endif



#include <math.h>
float l_h;
float l_Pn0;
float l_Pn1;
float l_x1;
float l_x2;
float l_K;
float l_theta0;
float l_theta1;
float l_Ud;

#define u_width 2
#define y_width 1


void L_identification_Start_wrapper(real_T *xD)
{

l_theta0 = 0.01f;
l_Pn0 = 0.0008f*2.0f;

l_x1 = l_theta0;
l_x2 = l_Pn0;

}

void L_identification_Outputs_wrapper(const real32_T *u,
			real32_T *y,
			const real_T *xD)
{

l_theta1 = l_x1;
y[0] = l_theta1;

}

void L_identification_Update_wrapper(const real32_T *u,
			real32_T *y,
			real_T *xD)
{

l_h = u[0];
l_Pn0 = l_x2;
l_K = l_Pn0*l_h/(1+l_h*l_Pn0*l_h);
l_Pn1 = (l_Pn0 - l_K*l_h*l_Pn0);
l_theta0 = l_x1;
l_Ud = u[1];
l_theta1 = l_theta0 + l_K*(l_Ud - l_h*0.99f*l_theta0);
l_x1 = l_theta1;
l_x2 = l_Pn1;
}

