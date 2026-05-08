#ifndef __BOARD_CONFIG_H_
#define __BOARD_CONFIG_H_

// Board feature switches
#define PC_COMMUNICATION_ENABLE 0

//UVW PWM define start
#define PWM_TIM             TIM1
#define PWM_TIM_CLOCK       180000000

#define PWM_TIM_FREQ        10000         //HZ
#define PWM_TIM_PULSE       (PWM_TIM_CLOCK/(2*PWM_TIM_FREQ))
#define PWM_TIM_PULSE_TPWM  (PWM_TIM_CLOCK/(PWM_TIM_FREQ))

#define FOC_PERIOD          0.0001F

#define DEAD_TIME         ((u16) 200)
#define PWM_DEAD_TIME     (u16)((unsigned long long)PWM_TIM_CLOCK/2*(unsigned long long)DEAD_TIME/1000000000uL)

#define PWM_TIM_CLK           RCC_APB2Periph_TIM1

#define PWM_AH_PIN            GPIO_Pin_8
#define PWM_AH_GPIO_PORT      GPIOA
#define PWM_AH_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define PWM_AH_SOURCE         GPIO_PinSource8
#define PWM_AH_AF             GPIO_AF_TIM1

#define PWM_AL_PIN            GPIO_Pin_13
#define PWM_AL_GPIO_PORT      GPIOB
#define PWM_AL_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define PWM_AL_SOURCE         GPIO_PinSource13
#define PWM_AL_AF             GPIO_AF_TIM1

#define PWM_BH_PIN            GPIO_Pin_9
#define PWM_BH_GPIO_PORT      GPIOA
#define PWM_BH_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define PWM_BH_SOURCE         GPIO_PinSource9
#define PWM_BH_AF             GPIO_AF_TIM1

#define PWM_BL_PIN            GPIO_Pin_14
#define PWM_BL_GPIO_PORT      GPIOB
#define PWM_BL_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define PWM_BL_SOURCE         GPIO_PinSource14
#define PWM_BL_AF             GPIO_AF_TIM1

#define PWM_CH_PIN            GPIO_Pin_10
#define PWM_CH_GPIO_PORT      GPIOA
#define PWM_CH_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define PWM_CH_SOURCE         GPIO_PinSource10
#define PWM_CH_AF             GPIO_AF_TIM1

#define PWM_CL_PIN            GPIO_Pin_15
#define PWM_CL_GPIO_PORT      GPIOB
#define PWM_CL_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define PWM_CL_SOURCE         GPIO_PinSource15
#define PWM_CL_AF             GPIO_AF_TIM1
//UVW PWM define end

//DMA define start
#define DMA2_CLK        RCC_AHB1Periph_DMA2
#define DMA_CHANNEL0             DMA_Channel_0
#define DMA2_STREAM0              DMA2_Stream0
//DMA define end

//ADC sample define start
#define ADC1_DR_ADDRESS          ((uint32_t)0x4001204C)

#define SAMPLE_ADC                  ADC1
#define SAMPLE_ADC_CLK              RCC_APB2Periph_ADC1

#define VBUS_ADC_PIN                GPIO_Pin_0
#define VBUS_ADC_GPIO_PORT          GPIOC
#define VBUS_ADC_GPIO_CLK           RCC_AHB1Periph_GPIOC
#define VBUS_ADC_SOURCE             GPIO_PinSource0
#define VBUS_ADC_CHANNEL            ADC_Channel_10

#define A_CURRENT_ADC_PIN           GPIO_Pin_1
#define A_CURRENT_ADC_GPIO_PORT     GPIOC
#define A_CURRENT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOC
#define A_CURRENT_ADC_SOURCE        GPIO_PinSource1
#define A_CURRENT_ADC_CHANNEL            ADC_Channel_11

#define B_CURRENT_ADC_PIN           GPIO_Pin_2
#define B_CURRENT_ADC_GPIO_PORT     GPIOC
#define B_CURRENT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOC
#define B_CURRENT_ADC_SOURCE        GPIO_PinSource2
#define B_CURRENT_ADC_CHANNEL       ADC_Channel_12

#define TOTAL_CURRENT_ADC_PIN           GPIO_Pin_6
#define TOTAL_CURRENT_ADC_GPIO_PORT     GPIOA
#define TOTAL_CURRENT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOA
#define TOTAL_CURRENT_ADC_SOURCE        GPIO_PinSource6
#define TOTAL_CURRENT_ADC_CHANNEL       ADC_Channel_6

#define TEMPERATURE_ADC_PIN           GPIO_Pin_3
#define TEMPERATURE_ADC_GPIO_PORT     GPIOC
#define TEMPERATURE_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOC
#define TEMPERATURE_ADC_SOURCE        GPIO_PinSource3
#define TEMPERATURE_ADC_CHANNEL       ADC_Channel_13

#define U_VOLT_ADC_PIN           GPIO_Pin_1
#define U_VOLT_ADC_GPIO_PORT     GPIOB
#define U_VOLT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOB
#define U_VOLT_ADC_SOURCE        GPIO_PinSource1
#define U_VOLT_ADC_CHANNEL       ADC_Channel_9

#define V_VOLT_ADC_PIN           GPIO_Pin_0
#define V_VOLT_ADC_GPIO_PORT     GPIOB
#define V_VOLT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOB
#define V_VOLT_ADC_SOURCE        GPIO_PinSource0
#define V_VOLT_ADC_CHANNEL       ADC_Channel_8

#define W_VOLT_ADC_PIN           GPIO_Pin_7
#define W_VOLT_ADC_GPIO_PORT     GPIOA
#define W_VOLT_ADC_GPIO_CLK      RCC_AHB1Periph_GPIOA
#define W_VOLT_ADC_SOURCE        GPIO_PinSource7
#define W_VOLT_ADC_CHANNEL       ADC_Channel_7
//ADC sample define end

//communication task timer define start
#define COMMUNICATION_TASK_TIM_CLOCK     90000000
#define COMMUNICATION_TASK_TIM_PRESCALER 899
#define COMMUNICATION_TASK_FREQ          10         //HZ
#define COMMUNICATION_TASK_TIM           TIM4
#define COMMUNICATION_TASK_TIM_CLK       RCC_APB1Periph_TIM4
#define COM_TASK_TIM_IRQn                TIM4_IRQn
#define COM_TASK_TIM_IRQHandler          TIM4_IRQHandler
#define COM_TASK_TIM_PERIOD              (COMMUNICATION_TASK_TIM_CLOCK\
                                          /(COMMUNICATION_TASK_TIM_PRESCALER+1)\
                                          /COMMUNICATION_TASK_FREQ)
//communication task timer define end

//hall sensor define
#define HALL_TIM                TIM2
#define HALL_TIM_CLK            RCC_APB1Periph_TIM2

#define HALL_CH1_PIN            GPIO_Pin_0
#define HALL_CH1_GPIO_PORT      GPIOA
#define HALL_CH1_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define HALL_CH1_SOURCE         GPIO_PinSource0
#define HALL_CH1_AF             GPIO_AF_TIM2

#define HALL_CH2_PIN            GPIO_Pin_1
#define HALL_CH2_GPIO_PORT      GPIOA
#define HALL_CH2_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define HALL_CH2_SOURCE         GPIO_PinSource1
#define HALL_CH2_AF             GPIO_AF_TIM2

#define HALL_CH3_PIN            GPIO_Pin_10
#define HALL_CH3_GPIO_PORT      GPIOB
#define HALL_CH3_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define HALL_CH3_SOURCE         GPIO_PinSource10
#define HALL_CH3_AF             GPIO_AF_TIM2
//hall sensor define end

//DRV8301 define start
#define DRV8301_ENGATE_PIN             GPIO_Pin_9
#define DRV8301_ENGATE_GPIO_PORT       GPIOB
#define DRV8301_ENGATE_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define DRV8301_ENGATE_SOURCE          GPIO_PinSource9

#define DRV8301_FAULT_PIN             GPIO_Pin_2
#define DRV8301_FAULT_GPIO_PORT       GPIOD
#define DRV8301_FAULT_GPIO_CLK        RCC_AHB1Periph_GPIOD
#define DRV8301_FAULT_SOURCE          GPIO_PinSource2

#define DRV8301_OCTW_PIN              GPIO_Pin_4
#define DRV8301_OCTW_GPIO_PORT        GPIOB
#define DRV8301_OCTW_GPIO_CLK         RCC_AHB1Periph_GPIOB
#define DRV8301_OCTW_SOURCE           GPIO_PinSource4

#define DRV8301_SPIx_SCK_PIN           GPIO_Pin_10
#define DRV8301_SPIx_SCK_GPIO_PORT     GPIOC
#define DRV8301_SPIx_SCK_GPIO_CLK      RCC_AHB1Periph_GPIOC
#define DRV8301_SPIx_SCK_SOURCE        GPIO_PinSource10

#define DRV8301_SPIx_MOSI_PIN          GPIO_Pin_12
#define DRV8301_SPIx_MOSI_GPIO_PORT    GPIOC
#define DRV8301_SPIx_MOSI_GPIO_CLK     RCC_AHB1Periph_GPIOC
#define DRV8301_SPIx_MOSI_SOURCE       GPIO_PinSource12

#define DRV8301_SPIx_MISO_PIN          GPIO_Pin_11
#define DRV8301_SPIx_MISO_GPIO_PORT    GPIOC
#define DRV8301_SPIx_MISO_GPIO_CLK     RCC_AHB1Periph_GPIOC
#define DRV8301_SPIx_MISO_SOURCE       GPIO_PinSource11

#define DRV8301_SPIx_CS_PIN            GPIO_Pin_15
#define DRV8301_SPIx_CS_GPIO_PORT      GPIOA
#define DRV8301_SPIx_CS_GPIO_CLK       RCC_AHB1Periph_GPIOA
#define DRV8301_SPIx_CS_SOURCE         GPIO_PinSource15
//DRV8301 define end

//OLED display define start
#define OLED_SPIx_SCK_PIN            GPIO_Pin_3
#define OLED_SPIx_SCK_GPIO_PORT      GPIOB
#define OLED_SPIx_SCK_GPIO_CLK       RCC_AHB1Periph_GPIOB
#define OLED_SPIx_SCK_SOURCE         GPIO_PinSource3

#define OLED_SPIx_MOSI_PIN           GPIO_Pin_5
#define OLED_SPIx_MOSI_GPIO_PORT     GPIOB
#define OLED_SPIx_MOSI_GPIO_CLK      RCC_AHB1Periph_GPIOB
#define OLED_SPIx_MOSI_SOURCE        GPIO_PinSource5

#define OLED_DC_PIN                  GPIO_Pin_13
#define OLED_DC_GPIO_PORT            GPIOC
#define OLED_DC_GPIO_CLK             RCC_AHB1Periph_GPIOC
#define OLED_DC_SOURCE               GPIO_PinSource13

#define OLED_RESET_PIN               GPIO_Pin_8
#define OLED_RESET_GPIO_PORT         GPIOC
#define OLED_RESET_GPIO_CLK          RCC_AHB1Periph_GPIOC
#define OLED_RESET_SOURCE            GPIO_PinSource8

#define OLED_CS_PIN                  GPIO_Pin_7
#define OLED_CS_GPIO_PORT            GPIOC
#define OLED_CS_GPIO_CLK             RCC_AHB1Periph_GPIOC
#define OLED_CS_SOURCE               GPIO_PinSource7
//OLED display define end

//USER LED define start
#define USER_LED_PIN                GPIO_Pin_9
#define USER_LED_GPIO_PORT          GPIOC
#define USER_LED_GPIO_CLK           RCC_AHB1Periph_GPIOC
#define USER_LED_SOURCE             GPIO_PinSource9
//USER LED define end

//KEY define start
#define KEY_1_PIN                   GPIO_Pin_4
#define KEY_1_GPIO_PORT             GPIOC
#define KEY_1_GPIO_CLK              RCC_AHB1Periph_GPIOC
#define KEY_1_EXTI_GPIO_PORT        EXTI_PortSourceGPIOC
#define KEY_1_EXTI_SOURCE           EXTI_PinSource4
#define KEY_1_EXTI_LINE             EXTI_Line4

#define KEY_2_PIN                   GPIO_Pin_5
#define KEY_2_GPIO_PORT             GPIOC
#define KEY_2_GPIO_CLK              RCC_AHB1Periph_GPIOC
#define KEY_2_EXTI_GPIO_PORT        EXTI_PortSourceGPIOC
#define KEY_2_EXTI_SOURCE           EXTI_PinSource5
#define KEY_2_EXTI_LINE             EXTI_Line5

#define KEY_3_PIN                   GPIO_Pin_2
#define KEY_3_GPIO_PORT             GPIOB
#define KEY_3_GPIO_CLK              RCC_AHB1Periph_GPIOB
#define KEY_3_EXTI_GPIO_PORT        EXTI_PortSourceGPIOB
#define KEY_3_EXTI_SOURCE           EXTI_PinSource2
#define KEY_3_EXTI_LINE             EXTI_Line2
//key define end



extern void hard_init(void);
#endif

