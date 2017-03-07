/**
  ******************************************************************************
  * @file    dali_cmd.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   DALI commands implementation (according DALI spec) - header
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

#ifndef DALI_CMD_H
#define DALI_CMD_H

/** public data **/
#define ValBit(VAR,Place)    (VAR & (1 << Place))
#define SetBit(VAR,Place)    (VAR |= (1 << Place))
#define ClrBit(VAR,Place)    (VAR &= ((1 << Place) ^ 255))
/** public functions **/

void  DALIC_Init(void);
u8 	  DALIC_isTalkingToMe(void);
void  DALIC_ProcessCommand(void);
void  DALIC_Process_System_Failure(void);
void  DALIC_PowerOn(void);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
