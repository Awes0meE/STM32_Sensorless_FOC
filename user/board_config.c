/**********************************
    
**********************************/
#include "main.h"
#include "board_config.h"


void hard_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  TIM_TimeBaseInitTypeDef PWM_TIM_TimeBaseStructure;
  TIM_OCInitTypeDef PWM_TIM_OCInitStructure;
  TIM_BDTRInitTypeDef PWM_TIM_BDTRInitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  ADC_InitTypeDef ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  EXTI_InitTypeDef   EXTI_InitStructure;
  TIM_TimeBaseInitTypeDef TIM_HALLTimeBaseInitStructure;
  TIM_ICInitTypeDef       TIM_HALLICInitStructure;
  //TIM_TimeBaseInitTypeDef COM_TASK_TIM_TimeBaseStructure;
  RCC_ClocksTypeDef   RCC_Clocks;
  
  RCC_GetClocksFreq(&RCC_Clocks);
  RCC_AHB1PeriphClockCmd(OLED_SPIx_SCK_GPIO_CLK | OLED_SPIx_MOSI_GPIO_CLK|OLED_DC_GPIO_CLK|OLED_RESET_GPIO_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(DRV8301_SPIx_SCK_GPIO_CLK|DRV8301_SPIx_MOSI_GPIO_CLK|DRV8301_SPIx_MISO_GPIO_CLK|DRV8301_SPIx_CS_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(KEY_1_GPIO_CLK|KEY_2_GPIO_CLK|KEY_3_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(PWM_AH_GPIO_CLK|PWM_BH_GPIO_CLK|PWM_CH_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(PWM_AL_GPIO_CLK|PWM_BL_GPIO_CLK|PWM_CL_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(HALL_CH1_GPIO_CLK|HALL_CH2_GPIO_CLK|HALL_CH3_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(DRV8301_ENGATE_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(DRV8301_FAULT_GPIO_CLK|DRV8301_OCTW_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(VBUS_ADC_GPIO_CLK|A_CURRENT_ADC_GPIO_CLK|B_CURRENT_ADC_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(TOTAL_CURRENT_ADC_GPIO_CLK|TEMPERATURE_ADC_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(U_VOLT_ADC_GPIO_CLK|V_VOLT_ADC_GPIO_CLK|W_VOLT_ADC_GPIO_CLK,ENABLE);
  RCC_AHB1PeriphClockCmd(USER_LED_GPIO_CLK,ENABLE);
  RCC_APB2PeriphClockCmd(PWM_TIM_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(SAMPLE_ADC_CLK,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  RCC_APB1PeriphClockCmd(HALL_TIM_CLK, ENABLE);
  RCC_APB1PeriphClockCmd(COMMUNICATION_TASK_TIM_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(DMA2_CLK, ENABLE);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
  
  
  //hall sensor config start 
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = HALL_CH1_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_CH1_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = HALL_CH2_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_CH2_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = HALL_CH3_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(HALL_CH3_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(HALL_CH1_GPIO_PORT,HALL_CH1_SOURCE,HALL_CH1_AF);
  GPIO_PinAFConfig(HALL_CH2_GPIO_PORT,HALL_CH2_SOURCE,HALL_CH2_AF);
  GPIO_PinAFConfig(HALL_CH3_GPIO_PORT,HALL_CH3_SOURCE,HALL_CH3_AF);
  
  TIM_DeInit(HALL_TIM);
  TIM_TimeBaseStructInit(&TIM_HALLTimeBaseInitStructure); 
  TIM_HALLTimeBaseInitStructure.TIM_Period = 0xffffffff;
  TIM_HALLTimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(HALL_TIM,&TIM_HALLTimeBaseInitStructure);
  
  TIM_ICStructInit(&TIM_HALLICInitStructure);
  TIM_HALLICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM_HALLICInitStructure.TIM_ICPolarity = TIM_ICPolarity_BothEdge;
  TIM_HALLICInitStructure.TIM_ICFilter = 0x0f;
  TIM_ICInit(HALL_TIM,&TIM_HALLICInitStructure);
  
  
  TIM_PrescalerConfig(HALL_TIM, (u16)0,TIM_PSCReloadMode_Immediate);
  TIM_InternalClockConfig(HALL_TIM);
  TIM_SelectHallSensor(HALL_TIM, ENABLE);
  TIM_SelectInputTrigger(HALL_TIM, TIM_TS_TI1FP1);
  TIM_SelectSlaveMode(HALL_TIM,TIM_SlaveMode_Reset);
  TIM_ClearFlag(HALL_TIM, TIM_FLAG_Update|
                TIM_FLAG_CC1|
                  TIM_FLAG_CC2|
                    TIM_FLAG_CC3|
                      TIM_FLAG_CC4|
                        TIM_FLAG_Trigger|
                          TIM_FLAG_CC1OF|
                            TIM_FLAG_CC2OF|
                              TIM_FLAG_CC3OF|
                                TIM_FLAG_CC4OF);
  TIM_ITConfig(HALL_TIM, TIM_IT_CC1, ENABLE);
  TIM_SetCounter(HALL_TIM, 0);
  TIM_Cmd(HALL_TIM, ENABLE);
  //hall sensor config end
  
  //communication task timer config start
  //TIM_DeInit(COMMUNICATION_TASK_TIM);
  //TIM_TimeBaseStructInit(&COM_TASK_TIM_TimeBaseStructure); 
  //COM_TASK_TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  //COM_TASK_TIM_TimeBaseStructure.TIM_Prescaler = COMMUNICATION_TASK_TIM_PRESCALER;
  //COM_TASK_TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  //COM_TASK_TIM_TimeBaseStructure.TIM_Period = COM_TASK_TIM_PERIOD;
  //TIM_TimeBaseInit(COMMUNICATION_TASK_TIM,&COM_TASK_TIM_TimeBaseStructure);
  //TIM_ITConfig(COMMUNICATION_TASK_TIM, TIM_IT_Update, ENABLE);
  //TIM_Cmd(COMMUNICATION_TASK_TIM, ENABLE);
  //communication task timer config end
  
  //DRV8301 引脚  config start
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  
  GPIO_InitStructure.GPIO_Pin = DRV8301_FAULT_PIN;
  GPIO_Init(DRV8301_FAULT_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = DRV8301_OCTW_PIN;
  GPIO_Init(DRV8301_OCTW_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  
  GPIO_InitStructure.GPIO_Pin = DRV8301_ENGATE_PIN;
  GPIO_Init(DRV8301_ENGATE_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  
  GPIO_InitStructure.GPIO_Pin = DRV8301_SPIx_SCK_PIN;
  GPIO_Init(DRV8301_SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = DRV8301_SPIx_MOSI_PIN;
  GPIO_Init(DRV8301_SPIx_MOSI_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = DRV8301_SPIx_CS_PIN;
  GPIO_Init(DRV8301_SPIx_CS_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = DRV8301_SPIx_MISO_PIN;
  GPIO_Init(DRV8301_SPIx_MISO_GPIO_PORT, &GPIO_InitStructure);
  
  //DRV8301  SPI  引脚 初始电平
  GPIO_SetBits(DRV8301_ENGATE_GPIO_PORT,DRV8301_ENGATE_PIN);
  GPIO_SetBits(DRV8301_SPIx_CS_GPIO_PORT,DRV8301_SPIx_CS_PIN);
  GPIO_ResetBits(DRV8301_SPIx_SCK_GPIO_PORT,DRV8301_SPIx_SCK_PIN);
  GPIO_ResetBits(DRV8301_SPIx_MOSI_GPIO_PORT,DRV8301_SPIx_MOSI_PIN);
  //DRV8301 引脚  config end
  
  //OLED显示屏 引脚  config start
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
  
  
  GPIO_InitStructure.GPIO_Pin = OLED_SPIx_SCK_PIN;
  GPIO_Init(OLED_SPIx_SCK_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin =  OLED_SPIx_MOSI_PIN;
  GPIO_Init(OLED_SPIx_MOSI_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = OLED_DC_PIN;
  GPIO_Init(OLED_DC_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin =  OLED_RESET_PIN;
  GPIO_Init(OLED_RESET_GPIO_PORT, &GPIO_InitStructure);
  GPIO_ResetBits(OLED_RESET_GPIO_PORT,OLED_RESET_PIN);
  //OLED显示屏 引脚  config end
  
  //LED提示灯 引脚  config start
  GPIO_InitStructure.GPIO_Pin =  USER_LED_PIN;
  GPIO_Init(USER_LED_GPIO_PORT, &GPIO_InitStructure);
  //LED提示灯 引脚  config end
  
  //PWM  config start
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  
  GPIO_InitStructure.GPIO_Pin = PWM_AH_PIN;
  GPIO_Init(PWM_AH_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PWM_AL_PIN;
  GPIO_Init(PWM_AL_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PWM_BH_PIN;
  GPIO_Init(PWM_BH_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PWM_BL_PIN;
  GPIO_Init(PWM_BL_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PWM_CH_PIN;
  GPIO_Init(PWM_CH_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = PWM_CL_PIN;
  GPIO_Init(PWM_CL_GPIO_PORT, &GPIO_InitStructure);  
  
  
  GPIO_PinAFConfig(PWM_AH_GPIO_PORT,PWM_AH_SOURCE,PWM_AH_AF);
  GPIO_PinAFConfig(PWM_AL_GPIO_PORT,PWM_AL_SOURCE,PWM_AL_AF);
  GPIO_PinAFConfig(PWM_BH_GPIO_PORT,PWM_BH_SOURCE,PWM_BH_AF);
  GPIO_PinAFConfig(PWM_BL_GPIO_PORT,PWM_BL_SOURCE,PWM_BL_AF);
  GPIO_PinAFConfig(PWM_CH_GPIO_PORT,PWM_CH_SOURCE,PWM_CH_AF);
  GPIO_PinAFConfig(PWM_CL_GPIO_PORT,PWM_CL_SOURCE,PWM_CL_AF);
  TIM_DeInit(PWM_TIM);
  TIM_TimeBaseStructInit(&PWM_TIM_TimeBaseStructure);
  
  PWM_TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
  PWM_TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
  PWM_TIM_TimeBaseStructure.TIM_Period = PWM_TIM_PULSE;
  PWM_TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  PWM_TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(PWM_TIM, &PWM_TIM_TimeBaseStructure);
  
  TIM_OCStructInit(&PWM_TIM_OCInitStructure);
  PWM_TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  PWM_TIM_OCInitStructure.TIM_OutputState =  TIM_OutputState_Enable;
  PWM_TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
  PWM_TIM_OCInitStructure.TIM_Pulse = PWM_TIM_PULSE>>1;
  PWM_TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  PWM_TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
  PWM_TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  PWM_TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
  
  TIM_OC1Init(PWM_TIM, &PWM_TIM_OCInitStructure);
  TIM_OC2Init(PWM_TIM, &PWM_TIM_OCInitStructure);
  TIM_OC3Init(PWM_TIM, &PWM_TIM_OCInitStructure);
  
  PWM_TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  PWM_TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  PWM_TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
  PWM_TIM_OCInitStructure.TIM_Pulse = PWM_TIM_PULSE; 
  PWM_TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  PWM_TIM_OCInitStructure.TIM_OCNPolarity =TIM_OCNPolarity_Low;
  PWM_TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  PWM_TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;  
  TIM_OC4Init(PWM_TIM, &PWM_TIM_OCInitStructure);
  
  PWM_TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
  PWM_TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
  PWM_TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1; 
  PWM_TIM_BDTRInitStructure.TIM_DeadTime = PWM_DEAD_TIME;
  PWM_TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;
  PWM_TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
  PWM_TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;
  TIM_BDTRConfig(PWM_TIM, &PWM_TIM_BDTRInitStructure);
  
  
  //TIM_ITConfig(PWM_TIM, TIM_IT_Update, ENABLE);
  TIM_Cmd(PWM_TIM, ENABLE);
  
  PWM_TIM->CCR1 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR2 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR3 = PWM_TIM_PULSE>>1;
  PWM_TIM->CCR4 = PWM_TIM_PULSE-1;
  //PWM  config end
  
  //DMA config start
  DMA_InitStructure.DMA_Channel = DMA_CHANNEL0;  
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_ADDRESS;
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADC1ConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = 4;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_STREAM0, &DMA_InitStructure);
  DMA_Cmd(DMA2_STREAM0, ENABLE);
  //DMA config end
  
  //ADC  config start
  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  
  GPIO_InitStructure.GPIO_Pin = VBUS_ADC_PIN;
  GPIO_Init(VBUS_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = A_CURRENT_ADC_PIN;
  GPIO_Init(A_CURRENT_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = B_CURRENT_ADC_PIN;
  GPIO_Init(B_CURRENT_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = TOTAL_CURRENT_ADC_PIN;
  GPIO_Init(TOTAL_CURRENT_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = TEMPERATURE_ADC_PIN;
  GPIO_Init(TEMPERATURE_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = U_VOLT_ADC_PIN;
  GPIO_Init(U_VOLT_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = V_VOLT_ADC_PIN;
  GPIO_Init(V_VOLT_ADC_GPIO_PORT,&GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = W_VOLT_ADC_PIN;
  GPIO_Init(W_VOLT_ADC_GPIO_PORT,&GPIO_InitStructure);
  ADC_DeInit();
  
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv =ADC_ExternalTrigConv_T1_CC1;
  ADC_InitStructure.ADC_ExternalTrigConvEdge =ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;;
  ADC_InitStructure.ADC_NbrOfConversion = 4;
  
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  
  
  ADC_CommonInit(&ADC_CommonInitStructure);
  ADC_Init(SAMPLE_ADC, &ADC_InitStructure);
  
  ADC_InjectedSequencerLengthConfig(SAMPLE_ADC,4);
  
  ADC_ExternalTrigInjectedConvConfig(SAMPLE_ADC,ADC_ExternalTrigInjecConv_T1_CC4);
  ADC_ExternalTrigInjectedConvEdgeConfig(SAMPLE_ADC,ADC_ExternalTrigInjecConvEdge_Rising);
  
  ADC_InjectedChannelConfig(SAMPLE_ADC,VBUS_ADC_CHANNEL,1,ADC_SampleTime_15Cycles);
  ADC_InjectedChannelConfig(SAMPLE_ADC,A_CURRENT_ADC_CHANNEL,2,ADC_SampleTime_15Cycles);
  ADC_InjectedChannelConfig(SAMPLE_ADC,B_CURRENT_ADC_CHANNEL,3,ADC_SampleTime_15Cycles);
  ADC_InjectedChannelConfig(SAMPLE_ADC,TOTAL_CURRENT_ADC_CHANNEL,4,ADC_SampleTime_15Cycles);
  
  ADC_RegularChannelConfig(SAMPLE_ADC, U_VOLT_ADC_CHANNEL, 1, ADC_SampleTime_15Cycles);
  ADC_RegularChannelConfig(SAMPLE_ADC, V_VOLT_ADC_CHANNEL, 2, ADC_SampleTime_15Cycles);
  ADC_RegularChannelConfig(SAMPLE_ADC, W_VOLT_ADC_CHANNEL, 3, ADC_SampleTime_15Cycles);
  ADC_RegularChannelConfig(SAMPLE_ADC, TEMPERATURE_ADC_CHANNEL, 4, ADC_SampleTime_15Cycles);
  
  ADC_DMARequestAfterLastTransferCmd(SAMPLE_ADC, ENABLE);
  ADC_DMACmd(SAMPLE_ADC, ENABLE);
  
  
  ADC_ITConfig(SAMPLE_ADC, ADC_IT_JEOC, ENABLE);
  ADC_ClearFlag(SAMPLE_ADC, ADC_FLAG_JEOC);
  ADC_Cmd(SAMPLE_ADC,ENABLE);
  ADC_SoftwareStartConv(SAMPLE_ADC);
  //ADC  config end
  
  //按键  config start
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  
  GPIO_InitStructure.GPIO_Pin = KEY_1_PIN;
  GPIO_Init(KEY_1_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = KEY_2_PIN;
  GPIO_Init(KEY_2_GPIO_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = KEY_3_PIN;
  GPIO_Init(KEY_3_GPIO_PORT, &GPIO_InitStructure);
  
  SYSCFG_EXTILineConfig(KEY_1_EXTI_GPIO_PORT, KEY_1_EXTI_SOURCE);
  SYSCFG_EXTILineConfig(KEY_2_EXTI_GPIO_PORT, KEY_2_EXTI_SOURCE);
  SYSCFG_EXTILineConfig(KEY_3_EXTI_GPIO_PORT, KEY_3_EXTI_SOURCE);
  
  EXTI_InitStructure.EXTI_Line = KEY_1_EXTI_LINE|KEY_2_EXTI_LINE|KEY_3_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  //按键  config end
  
  //USB通讯配置 start
  //communication_init();
  //USB通讯配置 end
  
  //中断优先级设置 start
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  /*
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  */
  NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  /*
  NVIC_InitStructure.NVIC_IRQChannel = COM_TASK_TIM_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  */
  NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  #ifdef __ICCARM__
  NVIC_InitStructure.NVIC_IRQChannel = SysTick_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	#endif
  //中断优先级设置 end
}


