/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dali_config.h"
#include "dali.h"
#include "lite_timer_8bit.h"
#include "dali_cmd.h"
#include "dali_pub.h"
#include "dali_regs.h"
#include "eeprom.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay = 0;
__IO uint16_t RandomTimingCnt = 0;
static __IO uint32_t LedShowStatus = 0;
static __IO ErrorStatus HSEStartUpStatus = SUCCESS;
static __IO uint32_t SELStatus = 0;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;

/* Extern variables ----------------------------------------------------------*/
extern GPIO_InitTypeDef GPIO_InitStructure;

#ifdef USE_ARC_TABLE
#define ARC_TYPE "T"
#else
#define ARC_TYPE "D"
#endif

/* -- Debug option --
If the debuging mode is enable, a D is to be displayed.
For more information about he arc table mode, please read the documentation */
#ifdef DEBUG
#define DEBUGSTRING "D"
#else
#define DEBUGSTRING " "
#endif
/* -- Version number -- */
#define _VERSION_ "Version V1.0"

const uint8_t version[3][20]={
  "DALIbySTM8DALI " ARC_TYPE,
  _VERSION_,
  "(c)2010 by ST  " DEBUGSTRING
};

#define LOW_POWER_TIMEOUT      2000  // 2 seconds to go to sleep/halt
volatile u16 LEDlight;                // current light level
u16 HALTtimer;                       // timeout counter for low power mode

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void SysTick_Configuration(void);
void PWM_LED(u16 lightlevel);

/* Private function ----------------------------------------------------------*/
/**
* @brief   main program body
* @param   None
* @retval  None
*/
int main(void)
{
  /* Dummy access to avoid removal by compiler optimisation */
  __IO u8 t;
  u8 s = 0;
  s=version[s][s];
  t=s;
  GPIO_InitTypeDef GPIOInitStruct;
  RCC_Configuration();

  /* Initialisation of DALI */
  DALI_Init(PWM_LED);
  /* End of initialisation */

  /* sleep/halt coudown counter */
  HALTtimer = LOW_POWER_TIMEOUT;
  LEDlight = 0;

  GPIOInitStruct.GPIO_Pin = GPIO_Pin_0;
  GPIOInitStruct.GPIO_Speed = GPIO_Speed_10MHz;
  GPIOInitStruct.GPIO_Mode =GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIOInitStruct);



  /* main program loop */
  while(1)
  {

    /* -------------------------------------------------------------------------------- */
    if (DALI_TimerStatus()) // must be checked at least each 1ms
    {
      if (HALTtimer) // countdown timeout if no activity in timer
        HALTtimer--;
      if (DALI_CheckAndExecuteTimer())  // need to call this function under 1ms interval periodically (fading function)
        HALTtimer = LOW_POWER_TIMEOUT;  // restart 10seconds timeout if some activity in timer
    }
    /* -------------------------------------------------------------------------------- */
    if (DALI_CheckAndExecuteReceivedCommand()) //need to call this function periodically (receive and process DALI command)
    {
      HALTtimer = LOW_POWER_TIMEOUT;    // restart 10seconds timeout if received and executed command
      Physically_Selected = !(DALI_BUTTON_PORT->IDR & (1<<DALI_BUTTON_PIN));// physical selection = pushbutton in GND
    }
    /* -------------------------------------------------------------------------------- */
    if (!HALTtimer) // go to power save state (WFI or HALT)
    {
      if (LEDlight) // go to sleep or halt according light level (level "0" = power off = halt)
      {
        __WFI();       // enable sleep only: PWM function requires continuous run and/or interrupts
      }
      else
      {
        DALI_halt();     // enable halt: PWM function is off - not requires continuous run and/or not uses interrupts
        HALTtimer = 600; // wake-up = DALI bus changed - command is receiving, 600ms to receive command and check bus errors
      }
    }
  } /* while(1) loop */
}

/**
* @brief   PWM for LED light control on STM32 discovery board
*                : control of light level callback function - must be type
*                : TLightControlCallback - see dali.h
* @param   intensity value(mapped inside)
* @retval  None
*/
void PWM_LED(u16 lightlevel)
{
  /* Initialize the PWM signals for the intensity control */
  /* SEt some default values for PWM to start with        */

  /* PA6 pin - TIM3_CH1 */
  /* GPIOA Configuration:TIM3 Channel1 */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = lightlevel;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(TIM3, &TIM_OCInitStructure);

  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

  TIM_ARRPreloadConfig(TIM3, ENABLE);

  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE);

  LEDlight = lightlevel;       // store current light level to global variable (for entering into halt if zero)
}

/**
* @brief   RCC configuration
* @param   None
* @retval  None
*/
void RCC_Configuration()
{
  SysTick_Configuration();
  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  /* GPIOA and GPIOB clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC |
                         RCC_APB2Periph_AFIO, ENABLE);
}

/**
* @brief   Configure a SysTick Base time
* @param   None
* @retval  None
*/
void SysTick_Configuration(void)
{
  if (SysTick_Config(SystemCoreClock / 9600))
  {
    /* Capture error */
    while (1);
  }

 }


/**
* @brief   Inserts a delay time.
* @param   specifies the delay time length (time base 10 ms
* @retval  None
*/
void Delay(__IO uint32_t nCount)
{
  TimingDelay = nCount;

  /* Enable the SysTick Counter */
  SysTick->CTRL |= SysTick_CTRL_ENABLE;

  while(TimingDelay != 0);

  /* Disable the SysTick Counter */
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE;

  /* Clear the SysTick Counter */
  SysTick->VAL = (uint32_t)0x0;
}

/**
* @brief   Decrements the TimingDelay variable.
* @param   None
* @retval  None
*/
void Decrement_TimingDelay(void)
{
  if (TimingDelay != 0x00)
  {
    TimingDelay--;
  }
  RandomTimingCnt++;
}

/**
* @brief   Returns the HSE StartUp Status
* @param   None
* @retval  HSE StartUp Status.
*/
ErrorStatus Get_HSEStartUpStatus(void)
{
  return (HSEStartUpStatus);
}

#ifdef USE_FULL_ASSERT
/**
* @brief   Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* @param    file: pointer to the source file name
* @param    line: assert error line source number
* @retval   None
*/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif



/**
* @}
*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
