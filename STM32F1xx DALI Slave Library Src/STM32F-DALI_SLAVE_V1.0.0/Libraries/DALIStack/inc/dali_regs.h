/**
  ******************************************************************************
  * @file    dali_reg.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali Registers management - header
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

#ifndef DALI_REGS_H
#define DALI_REGS_H

/* The DALI-Registers */



/* RAM-Regs */
#define DALIREG_ACTUAL_DIM_LEVEL      0
#define DALIREG_SEARCH_ADDRESS        1
#define DALIREG_STATUS_INFORMATION    4

/* E²PROM-Regs */
#define DALIREG_POWER_ON_LEVEL        5
#define DALIREG_SYSTEM_FAILURE_LEVEL  6
#define DALIREG_MIN_LEVEL             7
#define DALIREG_MAX_LEVEL             8
#define DALIREG_FADE_RATE             9
#define DALIREG_FADE_TIME            10
#define DALIREG_SHORT_ADDRESS        11
#define DALIREG_RANDOM_ADDRESS       12
#define DALIREG_GROUP_0_7            15
#define DALIREG_GROUP_8_15           16
#define DALIREG_SCENE                17

/* ROM_Regs */
#define DALIREG_VERSION_NUMBER       33
#define DALIREG_PHYS_MIN_LEVEL       34


#define DALIREG_RAM_START     0
#define DALIREG_RAM_END       5

#define DALIREG_EEPROM_START  0
#define DALIREG_EEPROM_END   33

#define DALIREG_ROM_START    33
#define DALIREG_ROM_END      35

#define DALI_NUMBER_REGS     35

extern uint8_t RAMRegs[DALIREG_RAM_END - DALIREG_RAM_START];

/********************************************************************
 * Name      : DALIR_ReadReg
 * Purpose   : Reads the Content of one of the DALI-Registers
 * Parameters: Name of the Register to be read
 * Returns   : Value of the Register
 * Example   : minlevel = DALIP_ReadRegister(DALIREG_MIN_LEVEL);
 * Remarks   : -
 *******************************************************************/
u8 DALIR_ReadReg(u8 idx);

/********************************************************************
 * Name      : DALIR_WriteReg
 * Purpose   : Writes to one of the DALI-Registers
 * Parameters: Name of the Register to be write, New Value
 * Returns   : None
 * Example   : DALIP_WriteRegister(DALIREG_MIN_LEVEL,1);
 * Remarks   : Some of the Regs are read-only. in that case,
 *             the write attempt will be ignored
 *******************************************************************/
void DALIR_WriteReg(u8 idx, u8 newval);

/********************************************************************
 * Name      : DALIR_WriteStatusBit
 * Purpose   : Writes to the status reg
 * Parameters: Bit index, see constants, New Value
 * Returns   : None
 * Example   : DALIP_WriteStatusBit(DALIREG_STATUS_BALLAST,1);
 * Remarks   : Some of the Regs are read-only. in that case,
 *             the write attempt will be ignored
 *******************************************************************/
#define DALIREG_STATUS_BALLAST           0
#define DALIREG_STATUS_LAMP_FAILURE      1
#define DALIREG_STATUS_LAMP_ARC_POWER_ON 2
#define DALIREG_STATUS_LIMIT_ERROR       3
#define DALIREG_STATUS_FADE_READY        4
#define DALIREG_STATUS_RESET_STATE       5
#define DALIREG_STATUS_MISSING_SHORT     6
#define DALIREG_STATUS_POWER_FAILURE     7

void          DALIR_WriteStatusBit(u8, u8);
u8 DALIR_ReadStatusBit(u8);

void DALIR_ResetRegs(void);
void DALIR_LoadRegsFromE2(void);
void DALIR_DeleteShort(void);
void DALIR_Init(void);

#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
