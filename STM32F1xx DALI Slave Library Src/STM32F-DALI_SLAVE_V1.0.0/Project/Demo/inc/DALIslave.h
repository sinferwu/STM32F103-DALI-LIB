/**
  ******************************************************************************
  * @file    dalislave.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali IO pin driver implementation - header
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


#include "stm32f10x.h"

#define NO_ACTION 0
#define SENDING_DATA 1
#define RECEIVING_DATA 2
#define ERR 3

#define CPU_CLK           (16000000)  //16MHz
#define PRESCALLER   (0x03) // divide by 8
#define DIVIDER      (CPU_CLK/(1<<PRESCALLER)/9600) //8 x 1200 DALI baudrate
#define TICKS_PER_ONE_MS  (9600/1000)

#define US_PER_TICK       (1000000/(CPU_CLK/(1<<PRESCALLER)/DIVIDER))
#define US_PER_MS         (1000000/1000)

extern __IO uint16_t RandomTimingCnt;

//callback function type
typedef void TDataReceivedCallback(u8 address,u8 dataByte);
typedef void TRTC_1ms_Callback(void);
typedef void TErrorCallback(u8 code);

// Receiving procedures
void receive_data(void);
void receive_tick(void);

// Common procedures
void init_DALI(GPIO_TypeDef* port_out, u16 pin_out, u8 invert_out, GPIO_TypeDef* port_in, u16 pin_in, u8 invert_in,
               TDataReceivedCallback DataReceivedFunction, TErrorCallback ErrorFunction, TRTC_1ms_Callback RTC_1ms_Function);
u8 get_flag(void);

// Sending procedures
void send_data(u8 byteToSend);
void send_tick(void);
void check_interface_failure(void);

void DALI_InterruptConfig(void);
void receive_tick(void) ;

// Timer procedures
u8 get_timer_count(void);


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
