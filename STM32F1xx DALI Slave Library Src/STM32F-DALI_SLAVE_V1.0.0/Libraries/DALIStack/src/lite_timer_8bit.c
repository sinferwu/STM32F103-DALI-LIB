/**
  ******************************************************************************
  * @file    lite_timer_8bit.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Timer routines for DALI timing management
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
#include "lite_timer_8bit.h"
#include "dali_pub.h"
#include "dali_cmd.h"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile u8 lite_timer_IT_state;
u16 timercounter;
u8  bigtimermins;
u16 bigtimertics;
u8  RealTimeClock_BigTimer;
u16 RealTimeClock_TimerCountDown;
u8  UserTimerActive;
u8  DAPCTimerActive;
u16 PowerOnTimerActive;
u16 BusFailureTimer;
u8 toggle=0;

/* Extern variables ----------------------------------------------------------*/
/* Private function ----------------------------------------------------------*/
/**
* @brief  Configure the Timer Lite
* @param  None
* @retval None
*/
void Timer_Lite_Init(void)
{
  PowerOnTimerActive = 600; //600 ms timeout after power up
  UserTimerActive = 0;
  DAPCTimerActive = 0;
}

/**
* @brief  Resets the PowerOnTimeer
* @param  None
* @retval None
*/
void PowerOnTimerReset(void)
{
  PowerOnTimerActive = 0;
}

/**
* @brief  Initiate RTC_LaunchBigTimer
* @param  minutes
* @retval None
*/
void RTC_LaunchBigTimer(u8 mins)
{
  bigtimertics = 60000; /* 60000*1ms=1mn*/
  bigtimermins = mins-1; /* Timer is launched for (mins-1)*1mn (basically 15mn, see DALI specifications)*/
  RealTimeClock_BigTimer = 1;
}

/**
* @brief  Initiate RTC_LaunchTimer
* @param  timer value
* @retval None
*/
void RTC_LaunchTimer(u16 timer_value)
{
  RealTimeClock_TimerCountDown=timer_value;
}

/**
* @brief  Initiate RTC_LaunchUserTimer
* @param  timer count
* @retval None
*/
void RTC_LaunchUserTimer(u8 TimerCount)
{
  UserTimerActive=TimerCount;
}

/**
* @brief  Clears RTC_LaunchUserTimer
* @param  None
* @retval None
*/
void RTC_DoneUserTimer(void)
{
  UserTimerActive=0;
}

/**
* @brief  launch 200ms RTC DAPC Timer
* @param  None
* @retval None
*/
void RTC_LaunchDAPCTimer(void)
{
  DAPCTimerActive=200;
}

/**
* @brief  clears 200ms RTC DAPC Timer
* @param  None
* @retval None
*/
void RTC_DoneDAPCTimer(void)
{
  DAPCTimerActive=0;
}

/**
* @brief  interrupt routine which process every 1ms
* @param  None
* @retval timer state
*/
u8 Process_Lite_timer_IT(void)
{
  if(toggle==0)
  {
    GPIO_SetBits(GPIOA, GPIO_Pin_0);
    toggle=1;
  }
  else
  {
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    toggle=0;
  }


  if (UserTimerActive)
  {


    if (UserTimerActive!=0xFF) UserTimerActive--;
    DALIP_TimerCallback();
    if (UserTimerActive==0)
      DALIP_SetFadeReadyFlag(0); /* fade is ready */
  }
  if (PowerOnTimerActive)
  {
    PowerOnTimerActive--;
    if (!PowerOnTimerActive)
    {
      DALIC_PowerOn();
    }
  }

  if (DAPCTimerActive)
  {
    DAPCTimerActive--;
    if (!DAPCTimerActive)
    {
      DALIP_Stop_DAPC_Sequence();
    }
  }

  lite_timer_IT_state=0;
  return (UserTimerActive + PowerOnTimerActive + DAPCTimerActive);
}


/**
* @brief  for calling every 1ms - callback function
* @param  None
* @retval None
*/
void Lite_timer_Interrupt(void)
{
  if (bigtimertics)
  {
    bigtimertics--;
  }
  else
  {
    if (bigtimermins)
    {
      bigtimermins--;
      bigtimertics = 60000;  /* 60000*1ms=1mn*/
    }
    else
    {
      RealTimeClock_BigTimer = 0;
    }
  }
  if (RealTimeClock_TimerCountDown)
  {
    RealTimeClock_TimerCountDown--;
  }
  lite_timer_IT_state=1;
}


/**
* @}
*/



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/