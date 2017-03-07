/**
  ******************************************************************************
  * @file    dali.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   High level functions to init and execute DALI stack - header
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

#ifndef DALI_H
#define DALI_H

extern volatile u8 dali_address;
extern volatile u8 dali_data;
extern volatile u8 dali_receive_status;
extern volatile u8 dali_error;
extern u8 dali_state;

/*callback function type for light control*/
typedef void TDLightControlCallback(u16 lighvalue);

/*---CONSTANTS---*/
/* Constants for dali_state*/
#define DALI_IDLE		0	/* DALI sender: Idle mode */
#define DALI_SEND_START		1	/* DALI sender: send Start Condition */
#define DALI_SEND_ADDRESS	2	/* DALI sender: Send Address Bits*/
#define DALI_SEND_DATA		3	/* DALI sender: Send Data Bits */
#define DALI_SEND_STOP		4	/* DALI sender: Send Stop Bits */
#define DALI_SEND_SETTLING	5       /* DALI sender: Wait Settling Time */
#define DALI_WAIT		6	/* Waiting for Answerframe */

/* Constants for dali_receive_status */
#define DALI_READY_TO_RECEIVE	0
#define DALI_NEW_FRAME_RECEIVED	1
#define DALI_RECEIVE_OVERFLOW	2

/* Constants for dali_error */
#define DALI_NO_ERROR 0
#define DALI_INTERFACE_FAILURE_ERROR 1

#define DALI_REPETITION_WAIT 	120  /*Command repetition timeout (ms)*/

/** public functions **/
void DALI_Init(TDLightControlCallback LightControlFunction);
u8 DALI_TimerStatus(void);
u8 DALI_CheckAndExecuteTimer(void);
u8 DALI_CheckAndExecuteReceivedCommand(void);
void DALI_halt(void);
void DALI_Set_Lamp_Failure(u8 failure);

void Send_DALI_Frame(u8);
u8 Get_DALI_Random(void);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
