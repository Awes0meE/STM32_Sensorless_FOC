#ifndef __OLED_H_
#define __OLED_H_



#define OLED_SCLK_Clr() GPIO_ResetBits(OLED_SPIx_SCK_GPIO_PORT,OLED_SPIx_SCK_PIN)//CLK
#define OLED_SCLK_Set() GPIO_SetBits(OLED_SPIx_SCK_GPIO_PORT,OLED_SPIx_SCK_PIN)

#define OLED_SDIN_Clr() GPIO_ResetBits(OLED_SPIx_MOSI_GPIO_PORT,OLED_SPIx_MOSI_PIN)//DIN
#define OLED_SDIN_Set() GPIO_SetBits(OLED_SPIx_MOSI_GPIO_PORT,OLED_SPIx_MOSI_PIN)

#define OLED_RST_Clr() GPIO_ResetBits(OLED_RESET_GPIO_PORT,OLED_RESET_PIN)//RES
#define OLED_RST_Set() GPIO_SetBits(OLED_RESET_GPIO_PORT,OLED_RESET_PIN)

#define OLED_DC_Clr() GPIO_ResetBits(OLED_DC_GPIO_PORT,OLED_DC_PIN)//DC
#define OLED_DC_Set() GPIO_SetBits(OLED_DC_GPIO_PORT,OLED_DC_PIN)
 		     

#define OLED_CMD  0	//Ð´ÃüÁî
#define OLED_DATA 1	//Ð´Êý¾Ý
#define SIZE 16
#define Max_Column	128
#define Max_Row		64

extern s8 display_data_buff[128];
extern u8 display_data_buff_cnt;
extern u8 display_data_flag;
extern void OLED_Init(void);
extern void OLED_Clear(void);
extern void oled_display(void);
extern void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[]);
extern void OLED_ShowCHinese(u8 x,u8 y,u8 no);
extern void OLED_ShowCHinese_32(u8 x,u8 y,u8 no);
extern void OLED_ShowChar_32(u8 x,u8 y,u8 chr);
extern void OLED_ShowString(u8 x,u8 y,u8 *chr);
extern void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);

#endif
