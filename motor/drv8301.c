/**********************************

**********************************/
#include "main.h"
#include "drv8301.h"


uint16_t drv8301_reg_read[4];
u8 drv8301_init_ok_flag=0;
u8 drv8301_octw_flag=0;
uint16_t drv8301_spi_read(uint8_t address);

static void drv8301_status_update(void)
{
  drv8301_reg_read[0] = drv8301_spi_read(DRV8301_REG0);
  drv8301_reg_read[1] = drv8301_spi_read(DRV8301_REG1);
}

void drv8301_spi_write(uint8_t address,uint16_t data)
{
  uint16_t send_data = 0;
  uint8_t i;
  send_data = (uint16_t)(address&0x03);
  send_data <<= 11;
  send_data |= data;
  DRV8301_CS_Clr();
  for(i=0;i<16;i++)
  {
    DRV8301_SCLK_Set();
    if(send_data&0x8000)
    {
      DRV8301_MOSI_Set();
    }
    else
    {
      DRV8301_MOSI_Clr();
    }
    DRV8301_SCLK_Clr();
    send_data <<= 1;
  } 
  DRV8301_SCLK_Clr();
  DRV8301_MOSI_Clr();
  
  DRV8301_CS_Set();
}

uint16_t drv8301_spi_read(uint8_t address)
{
  uint16_t send_data = 0;
  uint16_t receive_data = 0;
  uint8_t i;
  send_data = (uint16_t)(address&0x03);
  send_data <<= 11;
  send_data |= 0x8000;
  DRV8301_CS_Clr();
  for(i=0;i<16;i++)
  {
    DRV8301_SCLK_Set();
    if(send_data&0x8000)
    {
      DRV8301_MOSI_Set();
    }
    else
    {
      DRV8301_MOSI_Clr();
    }
    send_data <<= 1;
    DRV8301_SCLK_Clr();
  }
  DRV8301_CS_Set();
  send_data = (uint16_t)(address&0x03);
  send_data <<= 11;
  send_data |= 0x8000;
  send_data = 0;
  DRV8301_CS_Clr();
  for(i=0;i<16;i++)
  {
    DRV8301_SCLK_Set();
    if(send_data&0x8000)
    {
      DRV8301_MOSI_Set();
    }
    else
    {
      DRV8301_MOSI_Clr();
    }
    send_data <<= 1;
    receive_data <<= 1;
    DRV8301_SCLK_Clr();
    if(DRV8301_MISO_GET()==1)
    {
      receive_data |= 0x0001;
    }
  }
  DRV8301_CS_Set();
  return receive_data;
}


void drv8301_init(void)
{
  Delay(10);
  drv8301_spi_write(DRV8301_REG2,GATE_CURRENT_0_7_A|GATE_RESET_NOMAL|PWM_MODE_6_INPUTS|OCP_CURR_LIMIT|OC_ADJ_SET_18);
  drv8301_spi_write(DRV8301_REG3,OCTW_OT_AND_OC|GAIN_AMP_10|DC_CAL_CH1_CON|DC_CAL_CH2_CON|OC_TOFF_CYCLE);
  drv8301_status_update();
  drv8301_reg_read[2] = drv8301_spi_read(DRV8301_REG2);
  drv8301_reg_read[3] = drv8301_spi_read(DRV8301_REG3);
  drv8301_init_ok_flag = 1;
  drv8301_octw_flag = 0;
}

void drv8301_protection(void)
{
  if(GPIO_ReadInputDataBit(DRV8301_FAULT_GPIO_PORT,DRV8301_FAULT_PIN)!=1)
  {
    GPIO_ResetBits(DRV8301_ENGATE_GPIO_PORT,DRV8301_ENGATE_PIN);
    drv8301_status_update();
    drv8301_fault_flag = 1;
    return;
  }
  
  if(GPIO_ReadInputDataBit(DRV8301_OCTW_GPIO_PORT,DRV8301_OCTW_PIN)!=1)
  {
    drv8301_status_update();
    drv8301_octw_flag = 1;
  }
  else
  {
    drv8301_octw_flag = 0;
  }
}


