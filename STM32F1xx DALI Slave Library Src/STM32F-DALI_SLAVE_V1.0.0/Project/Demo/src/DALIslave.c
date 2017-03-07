/**
  ******************************************************************************
  * @file    dalislave.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali IO pin driver implementation
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
#include "stm32f10x_it.h"
#include "DALIslave.h"
#include "dali_config.h"

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// Communication ports and pins
NVIC_InitTypeDef NVIC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;

GPIO_TypeDef* DALIOUT_port = GPIOB; //default
u16 DALIOUT_pin = 1<<1; // default pin B1
u8 DALIOUT_invert = 0;

GPIO_TypeDef* DALIIN_port = GPIOB; //default
u8 DALIIN_pin = 1<<0; // default pin B0
u8 DALIIN_invert = 0;

// Data variables
u8 answer;    // data to send to controller device
u8 address;   // address byte from controller device
u8 dataByte;  // data byte from controller device

// Processing variables
u8 flag;        // status flag
u8 bit_count;   // nr of rec/send bits
u16 tick_count; // nr of ticks of the timer
u16 InterfaceFailureCounter; //nr of ticks when interface voltage is low

bool bit_value;   // value of actual bit
bool actual_val;  // bit value in this tick of timer
bool former_val;  // bit value in previous tick of timer


/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
//callback function
void DataReceived(u8 address, u8 dataByte);
TDataReceivedCallback *DataReceivedCallback = DataReceived;

void RTC1msFnc(void);
TRTC_1ms_Callback * RTC_1ms_Callback = RTC1msFnc;

void ErrorFnc(u8 code);
TErrorCallback *ErrorCallback = ErrorFnc;


/***********************************************************/
/*************** R E C E I V E * P R O C E D U R E S *******/
/***********************************************************/
/**
* @brief  detects the edge of start bit
* @param  None
* @retval None
*/
void receive_data()
{

  // null variables
  address = 0;
  dataByte = 0;
  bit_count = 0;
  tick_count = 0;
  former_val = TRUE;

  // setup flag
  flag = RECEIVING_DATA;
  // disable external interrupt on DALI in port
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure);
}

/**
* @brief  Get state of DALIIN pin
* @param  None
* @retval bool status
*/
bool get_DALIIN(void) {
  if (DALIIN_invert)
  {
    if(GPIO_ReadInputDataBit(DALIIN_port, DALIIN_pin))
      return FALSE;
    else
      return TRUE;
  }
  else
  {
    if(GPIO_ReadInputDataBit(DALIIN_port, DALIIN_pin))
      return TRUE;
    else
      return FALSE;
  }
}

/**
* @brief   receiving data for slave device
* @param  None
* @retval None
*/
void receive_tick(void)
{
  // Because of the structure of current amplifier, input has
  // to be negated
  actual_val = get_DALIIN();
  tick_count++;

  // edge detected
  if(actual_val != former_val)
  {
    switch(bit_count)
    {
    case 0:
      if (tick_count > 2)
      {
        tick_count = 0;
        bit_count  = 1; // start bit
      }
      break;
    case 17:      // 1st stop bit
      if(tick_count > 6) // stop bit error, no edge should exist
        flag = ERR;
      break;
    default:      // other bits
      if(tick_count > 6)
      {
        if(bit_count < 9) // store bit in address byte
        {
          address |= (actual_val << (8-bit_count));
        }
        else             // store bit in data byte
        {
          dataByte |= (actual_val << (16-bit_count));
        }
        bit_count++;
        tick_count = 0;
      }
      break;
    }
  }else // voltage level stable
  {
    switch(bit_count)
    {
    case 0:
      if(tick_count==8)  // too long start bit
        flag = ERR;
      break;
    case 17:
      // First stop bit
      if (tick_count==8)
      {
        if (actual_val==0) // wrong level of stop bit
        {
          flag = ERR;
        }
        else
        {
          bit_count++;
          tick_count = 0;
        }
      }
      break;
    case 18:
      // Second stop bit
      if (tick_count==8)
      {
        flag = NO_ACTION;

        //enable EXTI
        EXTI_InitStructure.EXTI_Line = EXTI_Line0;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        DataReceivedCallback(address,dataByte);
      }
      break;
    default: // normal bits
      if(tick_count==10)
      { // too long delay before edge
        flag = ERR;
      }
      break;
    }
  }
  former_val = actual_val;
  if(flag==ERR)
  {
    flag = NO_ACTION;
    //enable EXTI
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
  }
}

/***********************************************************/
/*************** C O M M O N * P R O C E D U R E S *********/
/***********************************************************/
/**
* @brief   Iniatilize  DALIOUT/DALIIN port and pin
* @param  port_out DALIOUT Port
* @param  pin_out DALIOUT Pin
* @param  invert_out
* @param  port_in DALIIN Port
* @param  invert_in
* @param  DataReceivedFunction Callback function to verify succesful receive
* @param  ErrorFunction callback function for error management
* @param  RTC_1ms_Function  callback function for 1ms interrupt
* @retval None
*/
void init_DALI(GPIO_TypeDef* port_out, u16 pin_out, u8 invert_out, GPIO_TypeDef* port_in, u16 pin_in, u8 invert_in,
               TDataReceivedCallback DataReceivedFunction, TErrorCallback ErrorFunction, TRTC_1ms_Callback RTC_1ms_Function)
{
  DALIOUT_port = port_out;
  DALIOUT_pin = pin_out;
  DALIOUT_invert = invert_out;

  DALIIN_port = port_in;
  DALIIN_pin = pin_in;
  DALIIN_invert = invert_in;

  DataReceivedCallback = DataReceivedFunction;
  RTC_1ms_Callback = RTC_1ms_Function;
  ErrorCallback = ErrorFunction;

  /* Pin for data output */
  GPIO_InitStructure.GPIO_Pin =  DALIOUT_pin;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DALIOUT_port, &GPIO_InitStructure);

  GPIO_WriteBit(DALIOUT_port, DALIOUT_pin, Bit_SET);

  /* Pin for data input */
  GPIO_InitStructure.GPIO_Pin = DALIIN_pin;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(DALIIN_port, &GPIO_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  //set status flaf
  flag = NO_ACTION;

  //reset 500ms interface failure counter
  InterfaceFailureCounter = 0;

  return;
}

/**
* @brief   receiving flag status
* @param  None
* @retval u8 flag
*/
u8 get_flag(void)
{
  return flag;
}

/**
* @brief   returns timer counter
* @param  None
* @retval u8 value of random timer count
*/
u8 get_timer_count(void)
{
  return (RandomTimingCnt);
}

/*************** S E N D * P R O C E D U R E S *************/
/***********************************************************/

/**
* @brief   Set value to the DALIOUT pin
* @param  bool
* @retval None
*/
void set_DALIOUT(bool pin_value)
{
  if (DALIOUT_invert)
  {
    if(pin_value)
      DALIOUT_port->ODR &= ~DALIOUT_pin;
    else
      DALIOUT_port->ODR |= DALIOUT_pin;
  }
  else
  {
    if(pin_value)
      DALIOUT_port->ODR |= DALIOUT_pin;
    else
      DALIOUT_port->ODR &= ~DALIOUT_pin;
  }
}

void DataReceived(u8 address, u8 dataByte)
{
  // Data has been received from master device
  // Received data were stored in bytes address (1st byte)
  // and dataByte (2nd byte)
}

void RTC1msFnc(void)
{
  //here is called routines every 1ms
}

void ErrorFnc(u8 code_val)
{
  //here is error management
}

/**
* @brief   gets state of the DALIOUT pin
* @param   None
* @retval bool state of the DALIOUT pin
*/
bool get_DALIOUT(void)
{
  if (DALIOUT_invert)
  {
    if(DALIOUT_port->IDR & DALIOUT_pin)
      return FALSE;
    else
      return TRUE;
  }
  else
  {
    if(DALIOUT_port->IDR & DALIOUT_pin)
      return TRUE;
    else
      return FALSE;
  }
}

/**
* @brief   Send answer to the controller device
* @param   byteToSend
* @retval None
*/
// Send answer to the controller device
void send_data(u8 byteToSend)
{
  answer = byteToSend;
  bit_count = 0;
  tick_count = 0;

  // disable external interrupt - no incoming data now
  EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = DISABLE;
  EXTI_Init(&EXTI_InitStructure);

  flag = SENDING_DATA;
}

/**
* @brief   DALI protocol physical layer for slave device
* @param   None
* @retval  None
*/
void send_tick(void)
{
  //access to the routine just every 4 ticks = every half bit
  if((tick_count & 0x03)==0)
  {
    if(tick_count < 96)
    {
      // settling time between forward and backward frame
      if(tick_count < 24)
      {
        tick_count++;
        return;
      }

      // start of the start bit
      if(tick_count == 24)
      {
        set_DALIOUT(FALSE);
        tick_count++;
        return;
      }

      // edge of the start bit
      // 28 ticks = 28/9600 = 2,92ms = delay between forward and backward message frame
      if(tick_count == 28)
      {
        set_DALIOUT(TRUE);
        tick_count++;
        return;
      }

      // bit value (edge) selection
      bit_value = (bool)( (answer >> (7-bit_count)) & 0x01);

      // Every half bit -> Manchester coding
      if( !( (tick_count-24) & 0x0007) )
      { // div by 8
        if(get_DALIOUT() == bit_value ) // former value of bit = new value of bit
          set_DALIOUT((bool)(1-bit_value));
      }

      // Generate edge for actual bit
      if( !( (tick_count - 28) & 0x0007) )
      {
        set_DALIOUT(bit_value);
        bit_count++;
      }
    }else
    { // end of data byte, start of stop bits
      if(tick_count == 96)
      {
        set_DALIOUT(TRUE); // start of stop bit
      }

      // end of stop bits, no settling time
      if(tick_count == 112)
      {
        flag = NO_ACTION;
        //enable EXTI
        EXTI_InitStructure.EXTI_Line = EXTI_Line0;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);
      }
    }
  }
  tick_count++;

  return;
}

/**
* @brief   checks if DALI bus is in the error state for long time
* @param   None
* @retval  None
*/
void check_interface_failure(void)
{
  if (get_DALIIN())
  {
    InterfaceFailureCounter = 0;
    return;
  }

  InterfaceFailureCounter++;
  if (InterfaceFailureCounter > ((1000l * 500)/US_PER_TICK) )  //check 500ms timeout
  {
    ErrorCallback(1);
    InterfaceFailureCounter = 0;
  }
}

/**
* @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/