#ifndef __DRV8301_H_
#define __DRV8301_H_


#define DRV8301_REG0    0
#define DRV8301_REG1    1
#define DRV8301_REG2    2
#define DRV8301_REG3    3

//reg2
#define GATE_CURRENT_1_7_A        0x0000
#define GATE_CURRENT_0_7_A        0x0001
#define GATE_CURRENT_0_25_A       0x0002

#define GATE_RESET_NOMAL          0x0000
#define GATE_RESET_FAULTS         0x0004

#define PWM_MODE_6_INPUTS         0x0000
#define PWM_MODE_3_INPUTS         0x0008

#define OCP_CURR_LIMIT            0x0000
#define OCP_LATCH_SHUT_DOWN       0x0010
#define OCP_REPORT_ONLY           0x0020
#define OCP_DISABLED              0x0030

#define OC_ADJ_SET_0              0x0000      //0.060V
#define OC_ADJ_SET_1              0x0040      //0.068V
#define OC_ADJ_SET_2              0x0080      //0.076V
#define OC_ADJ_SET_3              0x00c0      //0.086V
#define OC_ADJ_SET_4              0x0100      //0.097V
#define OC_ADJ_SET_5              0x0140      //0.109V
#define OC_ADJ_SET_6              0x0180      //0.123V
#define OC_ADJ_SET_7              0x01c0      //0.138V
#define OC_ADJ_SET_8              0x0200      //0.155V
#define OC_ADJ_SET_9              0x0240      //0.175V
#define OC_ADJ_SET_10             0x0280      //0.197V
#define OC_ADJ_SET_11             0x02c0      //0.222V
#define OC_ADJ_SET_12             0x0300      //0.250V
#define OC_ADJ_SET_13             0x0340      //0.282V
#define OC_ADJ_SET_14             0x0380      //0.317V
#define OC_ADJ_SET_15             0x03c0      //0.358V
#define OC_ADJ_SET_16             0x0400      //0.403V
#define OC_ADJ_SET_17             0x0440      //0.454V
#define OC_ADJ_SET_18             0x0480      //0.511V
#define OC_ADJ_SET_19             0x04c0      //0.576V
#define OC_ADJ_SET_20             0x0500      //0.648V
#define OC_ADJ_SET_21             0x0540      //0.730V
#define OC_ADJ_SET_22             0x0580      //0.822V
#define OC_ADJ_SET_23             0x05c0      //0.926V
#define OC_ADJ_SET_24             0x0600      //1.043V
#define OC_ADJ_SET_25             0x0640      //1.175V
#define OC_ADJ_SET_26             0x0680      //1.324V
#define OC_ADJ_SET_27             0x06c0      //1.491V
#define OC_ADJ_SET_28             0x0700      //1.679V
#define OC_ADJ_SET_29             0x0740      //1.892V
#define OC_ADJ_SET_30             0x0780      //2.131V
#define OC_ADJ_SET_31             0x07c0      //2.400V

//reg3
#define OCTW_OT_AND_OC            0x0000
#define OCTW_OT_ONLY              0x0001
#define OCTW_OC_ONLY              0x0002

#define GAIN_AMP_10               0x0000              
#define GAIN_AMP_20               0x0004
#define GAIN_AMP_40               0x0008
#define GAIN_AMP_80               0x000C

#define DC_CAL_CH1_CON            0x0000
#define DC_CAL_CH1_DIS            0x0010

#define DC_CAL_CH2_CON            0x0000
#define DC_CAL_CH2_DIS            0x0020

#define OC_TOFF_CYCLE             0x0000
#define OC_TOFF_OFF_TIME          0x0040

#define DRV8301_SCLK_Clr() GPIO_ResetBits(DRV8301_SPIx_SCK_GPIO_PORT,DRV8301_SPIx_SCK_PIN)
#define DRV8301_SCLK_Set() GPIO_SetBits(DRV8301_SPIx_SCK_GPIO_PORT,DRV8301_SPIx_SCK_PIN)

#define DRV8301_MOSI_Clr() GPIO_ResetBits(DRV8301_SPIx_MOSI_GPIO_PORT,DRV8301_SPIx_MOSI_PIN)
#define DRV8301_MOSI_Set() GPIO_SetBits(DRV8301_SPIx_MOSI_GPIO_PORT,DRV8301_SPIx_MOSI_PIN)

#define DRV8301_CS_Clr() GPIO_ResetBits(DRV8301_SPIx_CS_GPIO_PORT,DRV8301_SPIx_CS_PIN)
#define DRV8301_CS_Set() GPIO_SetBits(DRV8301_SPIx_CS_GPIO_PORT,DRV8301_SPIx_CS_PIN)

#define DRV8301_MISO_Clr() GPIO_ResetBits(DRV8301_SPIx_MISO_GPIO_PORT,DRV8301_SPIx_MISO_PIN)
#define DRV8301_MISO_Set() GPIO_SetBits(DRV8301_SPIx_MISO_GPIO_PORT,DRV8301_SPIx_MISO_PIN)
#define DRV8301_MISO_GET() GPIO_ReadInputDataBit(DRV8301_SPIx_MISO_GPIO_PORT,DRV8301_SPIx_MISO_PIN)

#define DRV8301_ENGATE_Clr() GPIO_ResetBits(DRV8301_ENGATE_GPIO_PORT,DRV8301_ENGATE_PIN)
#define DRV8301_ENGATE_Set() GPIO_SetBits(DRV8301_ENGATE_GPIO_PORT,DRV8301_ENGATE_PIN)



extern u8 drv8301_init_ok_flag;
extern u8 drv8301_octw_flag;
extern uint16_t drv8301_reg_read[4];
extern void drv8301_init(void);
extern void drv8301_protection(void);


#endif
