#include "main.h"
#include "oled.h"

uint8_t display_buff[8][128] = {0};
s8 display_data_buff[128];
u8 display_data_buff_cnt=0;
u8 display_data_flag=0;
void OLED_WR_Byte(u8 dat)
{	
  u8 i;			    
  for(i=0;i<8;i++)
  {			  
    OLED_SCLK_Clr();
    if(dat&0x80)
      OLED_SDIN_Set();
    else 
      OLED_SDIN_Clr();
    dat<<=1;
    OLED_SCLK_Set();	   
  }				 		  
} 

void OLED_Clear(void)  
{  
  u8 i,n;
  OLED_DC_Clr();
  for(i=0;i<8;i++)  
  {  
    OLED_WR_Byte (0xb0+i);   
    OLED_WR_Byte (0x02);     
    OLED_WR_Byte (0x10);     
    OLED_DC_Set();
    for(n=0;n<128;n++)OLED_WR_Byte(0); 
    OLED_DC_Clr();
  } 
}

void OLED_Init(void)
{
  
  OLED_RST_Set();
  //Delay(100);
  OLED_RST_Clr();
  Delay(10);
  OLED_RST_Set();
  OLED_WR_Byte(0xAE);//--turn off oled panel
  OLED_WR_Byte(0x02);//---set low column address
  OLED_WR_Byte(0x10);//---set high column address
  OLED_WR_Byte(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
  OLED_WR_Byte(0x81);//--set contrast control register
  OLED_WR_Byte(0xff); // Set SEG Output Current Brightness
  OLED_WR_Byte(0xA1);//--Set SEG/Column Mapping     0xa0
  OLED_WR_Byte(0xC8);//Set COM/Row Scan Direction   0xc0
  OLED_WR_Byte(0xA6);//--set normal display
  OLED_WR_Byte(0xA8);//--set multiplex ratio(1 to 64)
  OLED_WR_Byte(0x3f);//--1/64 duty
  OLED_WR_Byte(0xD3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
  OLED_WR_Byte(0x00);//-not offset
  OLED_WR_Byte(0xd5);//--set display clock divide ratio/oscillator frequency
  OLED_WR_Byte(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
  OLED_WR_Byte(0xD9);//--set pre-charge period
  OLED_WR_Byte(0xF1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
  OLED_WR_Byte(0xDA);//--set com pins hardware configuration
  OLED_WR_Byte(0x12);
  OLED_WR_Byte(0xDB);//--set vcomh
  OLED_WR_Byte(0x40);//Set VCOM Deselect Level
  OLED_WR_Byte(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
  OLED_WR_Byte(0x02);//
  OLED_WR_Byte(0x8D);//--set Charge Pump enable/disable
  OLED_WR_Byte(0x14);//--set(0x10) disable
  OLED_WR_Byte(0xA4);// Disable Entire Display On (0xa4/0xa5)
  OLED_WR_Byte(0xA6);// Disable Inverse Display On (0xa6/a7) 
  OLED_WR_Byte(0xAF);//--turn on oled panel
  OLED_Clear();
  
  //OLED_Clear();
  usb_open_display_flag = 1;
  data_upload_display_flag = 0;
  
  motor_run_display_flag = 0;
  display_static_flag=1;
  init_dispaly_flag = 1;
}

void oled_display(void)
{
  uint32_t display_temp;
  uint8_t i,j;
  int8_t pos;
  if(display_data_flag==1)
  {
    for(i = 0;i<127;i++)
    {
      pos = (display_data_buff[i]/4);
      if(pos>0)
      {
        display_temp = 0x80000000;
        display_temp = display_temp >> pos;
        display_buff[3][i] = (uint8_t)((display_temp&0xff000000)>>24);
        display_buff[2][i] = (uint8_t)((display_temp&0x00ff0000)>>16);
        display_buff[1][i] = (uint8_t)((display_temp&0x0000ff00)>>8);
        display_buff[0][i] = (uint8_t)(display_temp&0x000000ff);
        display_buff[4][i] = 0;
        display_buff[5][i] = 0;
        display_buff[6][i] = 0;
        display_buff[7][i] = 0;
      }
      else
      {
        uint8_t temp;
        temp = -pos;
        display_temp = 1;
        display_temp = display_temp << temp;
        display_buff[7][i] = (uint8_t)((display_temp&0xff000000)>>24);
        display_buff[6][i] = (uint8_t)((display_temp&0x00ff0000)>>16);
        display_buff[5][i] = (uint8_t)((display_temp&0x0000ff00)>>8);
        display_buff[4][i] = (uint8_t)(display_temp&0x000000ff);
        display_buff[0][i] = 0;
        display_buff[1][i] = 0;
        display_buff[2][i] = 0;
        display_buff[3][i] = 0;
      }
    }
    for(i=0;i<8;i++)  
    {  
      OLED_WR_Byte (0xb0+i);    
      OLED_WR_Byte (0x02);      
      OLED_WR_Byte (0x10);     
      OLED_DC_Set();
      for(j=0;j<128;j++)OLED_WR_Byte(display_buff[i][j]); 
      OLED_DC_Clr();
    } 
    display_data_flag=0;
  }
}

void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 
  OLED_WR_Byte(0xb0+y);
  OLED_WR_Byte((((x+2)&0xf0)>>4)|0x10);
  OLED_WR_Byte(((x+2)&0x0f)); 
} 
/***********功能描述：显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7*****************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[])
{ 	
  unsigned int j=0;
  unsigned char x,y;
  
  if(y1%8==0) y=y1/8;      
  else y=y1/8+1;
  for(y=y0;y<y1;y++)
  {
    OLED_Set_Pos(x0,y);
    OLED_DC_Set();
    for(x=x0;x<x1;x++)
    {      
      OLED_WR_Byte(BMP[j++]);	    	
    }
    OLED_DC_Clr();
  }
} 
//显示汉字
void OLED_ShowCHinese(u8 x,u8 y,u8 no)
{      			    
  u8 t,adder=0;
  OLED_Set_Pos(x,y);
  OLED_DC_Set();
  for(t=0;t<16;t++)
  {
    OLED_WR_Byte(Hzk[2*no][t]);
    adder+=1;
  }
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+1);
  OLED_DC_Set();
  for(t=0;t<16;t++)
  {	
    OLED_WR_Byte(Hzk[2*no+1][t]);
    adder+=1;
  }
  OLED_DC_Clr();
}

void OLED_ShowCHinese_32(u8 x,u8 y,u8 no)
{
  u8 t;
  OLED_Set_Pos(x,y);
  OLED_DC_Set();
  for(t=0;t<32;t++)
  {
    OLED_WR_Byte(Hzk_32[4*no][t]);
  }
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+1);
  OLED_DC_Set();
  for(t=0;t<32;t++)
  {
    OLED_WR_Byte(Hzk_32[4*no+1][t]);
  }
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+2);
  OLED_DC_Set();
  for(t=0;t<32;t++)
  {
    OLED_WR_Byte(Hzk_32[4*no+2][t]);
  }
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+3);
  OLED_DC_Set();
  for(t=0;t<32;t++)
  {
    OLED_WR_Byte(Hzk_32[4*no+3][t]);
  }
  OLED_DC_Clr();
}

void OLED_ShowChar_32(u8 x,u8 y,u8 chr)
{
  unsigned char c=0,i=0;
  c=chr-'0';//得到偏移；测试是显示几个数字
  OLED_Set_Pos(x,y);
  OLED_DC_Set();
  for(i=0;i<16;i++)
    OLED_WR_Byte(F8X16[c*64+i]);
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+1);
  OLED_DC_Set();
  for(i=0;i<16;i++)
    OLED_WR_Byte(F8X16[c*64+i+16]);
  OLED_DC_Clr();
  
  OLED_Set_Pos(x,y+2);
  OLED_DC_Set();
  for(i=0;i<16;i++)
    OLED_WR_Byte(F8X16[c*64+i+32]);
  OLED_DC_Clr();
  OLED_Set_Pos(x,y+3);
  OLED_DC_Set();
  for(i=0;i<16;i++)
    OLED_WR_Byte(F8X16[c*64+i+48]);
  OLED_DC_Clr();
  
}
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 16/12 
void OLED_ShowChar(u8 x,u8 y,u8 chr)
{      	
  unsigned char c=0,i=0;	
  c=chr-' ';//得到偏移后的值			
  if(x>Max_Column-1){x=0;y=y+2;}
  if(SIZE ==16)
  {
    OLED_Set_Pos(x,y);	
    OLED_DC_Set();
    for(i=0;i<8;i++)
      OLED_WR_Byte(F8X16[c*16+i]);
    OLED_DC_Clr();
    OLED_Set_Pos(x,y+1);
    OLED_DC_Set();
    for(i=0;i<8;i++)
      OLED_WR_Byte(F8X16[c*16+i+8]);
    OLED_DC_Clr();
  }
  else {	
    OLED_Set_Pos(x,y+1);
    OLED_DC_Set();
    for(i=0;i<6;i++)
      OLED_WR_Byte(F6x8[c][i]);
    OLED_DC_Clr();
    
  }
}
void OLED_ShowString(u8 x,u8 y,u8 *chr)
{
  unsigned char j=0;
  while (chr[j]!='\0')
  {		OLED_ShowChar(x,y,chr[j]);
  x+=8;
  if(x>120){x=0;y+=2;}
  j++;
  }
}
//m^n函数
u32 oled_pow(u8 m,u8 n)
{
  u32 result=1;	
  u8 i;
  for(i=n;i>0;i--)
  {
    result*=m; 
  }
  //while(n--)result*=m;    
  return result;
}		
//显示2个数字
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);	 		  
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size)
{         	
  u8 t,temp;
  u8 enshow=0;						   
  for(t=0;t<len;t++)
  {
    temp=(num/oled_pow(10,len-t-1))%10;
    if(enshow==0&&t<(len-1))
    {
      if(temp==0)
      {
        OLED_ShowChar(x+(size/2)*t,y,' ');
        continue;
      }else enshow=1; 
      
    }
    OLED_ShowChar(x+(size/2)*t,y,temp+'0'); 
  }
} 


