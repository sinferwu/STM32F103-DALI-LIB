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
#include "dali_cmd.h"
#include "dali_regs.h"
#include "eeprom.h"

/* Private macro -------------------------------------------------------------*/
#define DALIR_IsRAMReg(a) (a<DALIREG_RAM_END)
#define DALIR_IsEEPROMReg(a) ((a>=DALIREG_EEPROM_START) && (a<DALIREG_EEPROM_END))
#define DALIR_IsROMReg(a)  ((a>=DALIREG_ROM_START) && (a<DALIREG_ROM_END))
#define DALIR_IsValid(a) (a < DALI_NUMBER_REGS)

/* Private variables ---------------------------------------------------------*/
uint8_t RAMRegs[DALIREG_RAM_END - DALIREG_RAM_START];
uint8_t short_addr;
uint8_t randbuf[2];

/* Extern variables ----------------------------------------------------------*/
extern const uint8_t ROMRegs[2]; /* Declared in DALI_PUB.C */
extern const uint8_t DaliRegDefaults[];

/* Private function prototypes -----------------------------------------------*/
/**
* @brief  Initialize RAM registers.
* @param  None
* @retval None
*/
void DALIR_Init(void)
{
  uint8_t i;
  for (i = 0; i<5; i++) RAMRegs[i]=0;
}

/**
* @brief  Reports the name of the source file and the source line number
*   where the assert_param error has occurred.
* @param  file: pointer to the source file name
* @param  line: assert_param error line source number
* @retval None
*/
void DALIR_WriteEEPROMReg(uint8_t idx, uint8_t val)
{

  if (idx == DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START) short_addr = val;
  E2_WriteMem((u16)eeprom_variable[idx],(u16)val);
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
uint8_t DALIR_ReadEEPROMReg(uint8_t idx)
{ u16 data;
if (idx == DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START) return short_addr;
E2_ReadMem((u16)eeprom_variable[idx],&data);
return (u8)data;
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
uint8_t DALIR_ReadReg(uint8_t idx)
{
  if (!DALIR_IsValid(idx)) return 0;
  if (DALIR_IsRAMReg(idx)) { return RAMRegs[idx - DALIREG_RAM_START];}
  if (DALIR_IsROMReg(idx)) { return ROMRegs[idx - DALIREG_ROM_START];}
  return DALIR_ReadEEPROMReg(idx - DALIREG_EEPROM_START);
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
void DALIR_WriteReg(uint8_t idx, uint8_t newval)
{
  uint8_t i;
  i=newval;
  if (!DALIR_IsValid(idx)) return;
  if (DALIR_IsROMReg(idx)) return;
  if (DALIR_IsRAMReg(idx))
  {
    RAMRegs[idx - DALIREG_RAM_START] = newval;
  }
  else
  {
    DALIR_WriteEEPROMReg(idx - DALIREG_EEPROM_START, newval);
  }

  if (DALIR_ReadStatusBit(DALIREG_STATUS_FADE_READY)==0)
  {

    DALIR_WriteStatusBit(DALIREG_STATUS_RESET_STATE,1);   /*set reset State*/

    for (i=0; i<DALI_NUMBER_REGS; i++)
    {           //to refresh "reset state" bit
      switch (i)
      {
      case DALIREG_SHORT_ADDRESS:
        break;
      case DALIREG_STATUS_INFORMATION:
        break;
      case DALIREG_MIN_LEVEL:
        if (DALIR_ReadReg(DALIREG_MIN_LEVEL)!= DALIR_ReadReg(DALIREG_PHYS_MIN_LEVEL))
          DALIR_WriteStatusBit(DALIREG_STATUS_RESET_STATE,0); /*clear reset State*/;
          break;
      default:
        if(DALIR_IsEEPROMReg(i) || DALIR_IsRAMReg(i))
          if (DALIR_ReadReg(i) != DaliRegDefaults[i])
            DALIR_WriteStatusBit(DALIREG_STATUS_RESET_STATE,0); /*clear reset State*/
        break;
      }
    }
  }
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
void DALIR_WriteStatusBit(uint8_t bit_nbr,uint8_t val)
{
  if (val == 0)
  {
    ClrBit(RAMRegs[DALIREG_STATUS_INFORMATION - DALIREG_RAM_START],bit_nbr);
  }
  else
  {
    SetBit(RAMRegs[DALIREG_STATUS_INFORMATION - DALIREG_RAM_START],bit_nbr);
  }
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
uint8_t DALIR_ReadStatusBit(uint8_t bit_nbr)
{
  return ValBit(RAMRegs[DALIREG_STATUS_INFORMATION - DALIREG_RAM_START],bit_nbr);
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
void DALIR_ResetRegs(void)
{
  uint8_t i;

  E2_WriteBurst(0,(u16)(DALIREG_EEPROM_END-DALIREG_EEPROM_START),(u16*)(&(DaliRegDefaults[DALIREG_EEPROM_START])));
  E2_WriteMem((u16)eeprom_variable[(DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START)],(u16)short_addr);
  for (i=0; i<DALI_NUMBER_REGS; i++)
  {
    switch (i)
    {
    case DALIREG_SHORT_ADDRESS:
      break;
    case DALIREG_STATUS_INFORMATION:
      break;
    case DALIREG_MIN_LEVEL:
      DALIR_WriteReg(DALIREG_MIN_LEVEL,DALIR_ReadReg(DALIREG_PHYS_MIN_LEVEL));
      break;
    default:
      if(DALIR_IsEEPROMReg(i) || DALIR_IsRAMReg(i))
      {
        DALIR_WriteReg(i,DaliRegDefaults[i]);
      }
      break;
    }
  }
  i=DALIR_ReadReg(DALIREG_STATUS_INFORMATION);
  i&=0x47;
  DALIR_WriteReg(DALIREG_STATUS_INFORMATION, i);
  DALIR_WriteStatusBit(DALIREG_STATUS_RESET_STATE,1); /*Set reset State*/
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
void DALIR_LoadRegsFromE2(void)
{   uint16_t data;
E2_ReadMem((u16)eeprom_variable[(DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START)],&data);
short_addr = (u8)data;
}

/**
* @brief  Main program.
* @param  None
* @retval None
*/
void DALIR_DeleteShort(void)
{
  short_addr = 0xFF;
  E2_WriteMem((u16)eeprom_variable[DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START],(u16)0xFF);
  DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,1);
}
/**
* @}
*/




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/