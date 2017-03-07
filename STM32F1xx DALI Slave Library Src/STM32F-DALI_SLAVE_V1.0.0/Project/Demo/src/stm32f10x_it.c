/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    29-June-2012
  * @brief   Main Interrupt Service Routines.
  *                      This file provides template for all exceptions handler
  *                      and peripherals interrupt service routine.
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
#include "DALIslave.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t Index = 0;
static __IO uint32_t AlarmStatus = 0;
static __IO uint32_t LedCounter = 0;
extern GPIO_TypeDef* DALIIN_port;
extern TRTC_1ms_Callback * RTC_1ms_Callback;
__IO uint16_t oneMScounter;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/


/**
* @brief  This function handles NMI exception.
* @param  None
* @retval None
*/
void NMI_Handler(void)
{

}

/**
* @brief  This function handles Hard Fault exception.
* @param  None
* @retval None
*/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles Memory Manage exception.
* @param  None
* @retval None
*/
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles Bus Fault exception.
* @param  None
* @retval None
*/
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles Usage Fault exception.
* @param  None
* @retval None
*/
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
* @brief  This function handles SVCall exception.
* @param  None
* @retval None
*/
void SVC_Handler(void)
{
}

/**
* @brief  This function handles Debug Monitor exception.
* @param  None
* @retval None
*/
void DebugMon_Handler(void)
{
}

/**
* @brief   This function handles PendSVC exception.
* @param  None
* @retval None
*/
void PendSV_Handler(void)
{
}

/**
* @brief  This function handles SysTick Handler.
* @param  None
* @retval None
*/
void SysTick_Handler(void)
{
  /* Decrement the TimingDelay variable */
  Decrement_TimingDelay();
  oneMScounter += US_PER_TICK;
  if (oneMScounter >= US_PER_MS)
  {
    oneMScounter -= US_PER_MS;
    RTC_1ms_Callback();
  }
  if(get_flag()==RECEIVING_DATA)
  {
    receive_tick();
  }
  else if(get_flag()==SENDING_DATA)
  {
    send_tick();
  }
  if(get_flag()==NO_ACTION)
  {
    check_interface_failure(); //check idle voltage on bus
  }
}

/**
* @brief  This function handles RTC global interrupt request.
* @param  None
* @retval None
*/
void RTC_IRQHandler(void)
{

}
/**
* @brief  This function handles External interrupt Line 3 request.
* @param  None
* @retval None
*/
void EXTI4_IRQHandler(void)
{

}

/**
* @brief  This function handles External lines 0 interrupt request.
* @param  None
* @retval None
*/
void EXTI0_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line0) != RESET)
  {
    receive_data();
    /* Clear the EXTI Line 3 */
    EXTI_ClearITPendingBit(EXTI_Line0);
  }
}
/**
* @brief  This function handles External lines 9 to 5 interrupt request.
* @param  None
* @retval None
*/
void EXTI9_5_IRQHandler(void)
{

}
/**
* @brief  This function handles TIM1 overflow and update interrupt
*          request.
* @param  None
* @retval None
*/
void TIM1_UP_IRQHandler(void)
{
  /* Clear the TIM1 Update pending bit */
  TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}
/**
* @brief  This function handles SPI2 global interrupt request.
* @param  None
* @retval None
*/
void SPI2_IRQHandler(void)
{

}
/**
* @brief  This function handles USART3 global interrupt request.
* @param  None
* @retval None
*/
void USART3_IRQHandler(void)
{

}
/**
* @brief  This function handles External lines 15 to 10 interrupt request.
* @param  None
* @retval None
*/
void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line15) != RESET)
  {

    /* Clear the EXTI Line 15 */
    EXTI_ClearITPendingBit(EXTI_Line15);
  }

}
/**
* @brief  This function handles RTC Alarm interrupt request.
* @param  None
* @retval None
*/
void RTCAlarm_IRQHandler(void)
{

}
/**
* @brief  This function handles SDIO global interrupt request.
* @param  None
* @retval None
*/
void SDIO_IRQHandler(void)
{

}
/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
* @brief  This function handles PPP interrupt request.
* @param  None
* @retval None
*/
void PPP_IRQHandler(void)
{
}



/**
* @}
*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
