/**
  ******************************************************************************
  * @file    dali_regs.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali Registers management
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
#include "stm32f10x.h"
#include "dali.h"
#include "DALIslave.h"
#include "dali_config.h"
#include "lite_timer_8bit.h"
#include "dali_cmd.h"
#include "dali_pub.h"
#include "dali_regs.h"
#include "eeprom.h"

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
volatile u8 dali_address;
volatile u8 dali_data;
volatile u8 dali_receive_status;
volatile u8 dali_error;
u8 dali_state;

/* Extern variables ----------------------------------------------------------*/
extern EXTI_InitTypeDef   EXTI_InitStructure;

/* Private function prototypes -----------------------------------------------*/
/**
* @brief  DALI receiving callback
* @param  address byte
* @param  data byte
* @retval None
*/
void DALI_Interrupt(u8 address,u8 dataByte)
{
  dali_address = address; 		// read DALI forward address
  dali_data = dataByte;		// read DALI forward data;
  dali_receive_status = DALI_NEW_FRAME_RECEIVED;
}

/**
* @brief  DALI Error callback
* @param   error code
* @retval None
*/
void DALI_Error(u8 code_val)
{
  switch (code_val)
  {
  case 1:  dali_error = DALI_INTERFACE_FAILURE_ERROR; break;
  default: dali_error = DALI_NO_ERROR; break;
  }
}

/**
* @brief  Initialize DALI
* @param   LightControlFunction for ballast management
* @retval None
*/
void DALI_Init(TDLightControlCallback LightControlFunction)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Pull-up Vdd pin for data output */
  GPIO_InitStructure.GPIO_Pin =  DALI_PULLUP_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DALI_PULLUP_PORT, &GPIO_InitStructure);

  GPIO_WriteBit(DALI_PULLUP_PORT, DALI_PULLUP_PIN, Bit_SET);

  /* button pin for data input */
  GPIO_InitStructure.GPIO_Pin =  DALI_BUTTON_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DALI_BUTTON_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin =  OUT_DALI_PIN ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(OUT_DALI_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin =  IN_DALI_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(IN_DALI_PORT, &GPIO_InitStructure);

  /* dali flags init */
  dali_state = DALI_IDLE;
  dali_receive_status = DALI_READY_TO_RECEIVE;

  /* Initialisation of DALI stack modules*/
  Timer_Lite_Init();

  /* Unlock the Flash Program Erase controller */
  FLASH_Unlock();

  /* RAM Init */
  EEPROM_Init();
  DALIR_Init();
  DALIP_Init(LightControlFunction);
  DALIC_Init();
  DALIR_ResetRegs();
  DALI_InterruptConfig ();

  /* Initialisation of DALI IO driver */
  init_DALI(OUT_DALI_PORT, OUT_DALI_PIN, INVERT_OUT_DALI, IN_DALI_PORT, IN_DALI_PIN, INVERT_IN_DALI, DALI_Interrupt, DALI_Error, Lite_timer_Interrupt);
}

/**
* @brief  DALI_InterruptConfig
* @param   None
* @retval None
*/
void DALI_InterruptConfig(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Set the Vector Table base address at 0x08000000 */
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x00);
  /* Configure the Priority Group to 2 bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);

  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable the EXTI3 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the RTC Alarm Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  EXTI_ClearITPendingBit(EXTI_Line0);
}


/**
* @brief   checks if 1ms timer is elapsed
* @param   None
* @retval  Timer state
*/
u8 DALI_TimerStatus(void)
{
  return lite_timer_IT_state;
}

/**
* @brief   checks if user timer is active and performs timer action if necessary
* @param   None
* @retval  Timer state
*/
u8 DALI_CheckAndExecuteTimer(void)
{
  static u8 TimerActive = 0;

  if(lite_timer_IT_state==1) //set every 1 ms by timer
  {
    TimerActive = Process_Lite_timer_IT(); //manage fade effects each 1ms (fade time and fade rate)
  }
  return TimerActive;
}

/**
* @brief   checks if some received command is pending and if then execute it
* @param   None
* @retval  DALI Receive  state
*/
u8 DALI_CheckAndExecuteReceivedCommand(void)
{
  /*check received data*/
  if(dali_receive_status == DALI_NEW_FRAME_RECEIVED)
  {
    if (DALIC_isTalkingToMe())
    {
      DALIC_ProcessCommand();
      dali_receive_status = DALI_READY_TO_RECEIVE;
      return 1;
    }
    dali_receive_status = DALI_READY_TO_RECEIVE;
  }

  /*check error*/
  if(dali_error == DALI_INTERFACE_FAILURE_ERROR)
  {
    /*error management*/
    DALIC_Process_System_Failure();
    dali_error = DALI_NO_ERROR;
    return 2;
  }

  return 0;
}

/**
* @brief   checks if DALI packed receiving/sending is not in progress and go to halt if not
* @param   None
* @retval  None
*/
void DALI_halt(void)
{
  __disable_irq();
  if ((dali_receive_status != DALI_NEW_FRAME_RECEIVED) && (get_flag() == NO_ACTION) )  //if DALI frame receiving in progress
  {
    __WFI(); /* Not able to find suitable command for halt command */
  }
  __enable_irq();
}

/**
* @brief   Initializes Sending of backward frame
* @param   data byte
* @retval  None
*/
/*
************************************************
* Initializes Sending but returns immediately!	 *
* Sending continues asynchronously.             *
************************************************/
void Send_DALI_Frame(u8 data_val)
{
  send_data(data_val);
  dali_state = DALI_SEND_START;
}

/**
* @brief   Gets a Random address using timer
* @param   None
* @retval  u8 random address
*/
u8 Get_DALI_Random(void)
{
  static u8 RandomCounter;
  u8 TimerCount;

  RandomCounter ++;
  TimerCount = get_timer_count();
  switch (RandomCounter & 0x03)
  {
  case 0:  return (TimerCount); break;
  case 1:  return (~TimerCount); break;
  case 2:  return ((TimerCount >> 1)^(TimerCount   )); break;
  default: return ((TimerCount     )^(RandomCounter)); break;
  }
}

/**
* @brief   Notification that Lamp has hardware error
* @param   failure code
* @retval  None
*/
void DALI_Set_Lamp_Failure(u8 failure)
{
  DALIP_SetLampFailureFlag(failure);
}

/**
* @}
*/


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/