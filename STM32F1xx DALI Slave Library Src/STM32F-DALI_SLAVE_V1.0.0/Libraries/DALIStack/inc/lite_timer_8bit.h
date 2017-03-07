/**
  ******************************************************************************
  * @file    lite_timer_8bit.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Timer routines for DALI timing management - header
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

#ifndef LITE_TIM2_H
#define LITE_TIM2_H

extern volatile u8 lite_timer_IT_state;

/*---FUNCTIONS---*/

void Timer_Lite_Init(void);
void PowerOnTimerReset(void);
void RTC_LaunchBigTimer(u8);
void RTC_LaunchTimer(u16);
void RTC_LaunchUserTimer(u8);
void RTC_DoneUserTimer(void);
void RTC_LaunchDAPCTimer(void);
void RTC_DoneDAPCTimer(void);
u8 Process_Lite_timer_IT(void); //SESE
void Lite_timer_Interrupt(void);


/*---Variables---*/

extern u8  RealTimeClock_BigTimer;
extern u16 RealTimeClock_TimerCountDown;

#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
