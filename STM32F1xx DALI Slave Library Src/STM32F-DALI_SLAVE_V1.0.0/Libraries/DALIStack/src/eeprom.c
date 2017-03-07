/**
  ******************************************************************************
  * @file    eeprom.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   EEPROM read/write routines
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
#include "eeprom.h"
#include "dali_regs.h"

/* Private macro -------------------------------------------------------------*/

/* Global variable used to store variable value in read sequence */
/* Private variables ---------------------------------------------------------*/
uint16_t DataVar = 0;
u16 eeprom_variable[NumbOfVar];
u16 read_eeprom_variable[128];

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void E2_ResetEEPROM(void);
uint16_t E2_WriteMem(u16 VirtAddress, u16 Data);
u16 E2_DetectMemSize(void);
uint16_t E2_ReadMem(uint16_t VirtAddress, uint16_t* Data);
static FLASH_Status EE_Format(void);
static uint16_t EE_FindValidPage(uint8_t Operation);
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, uint16_t Data);
static uint16_t EE_PageTransfer(uint16_t VirtAddress, uint16_t Data);


/* Private functions ---------------------------------------------------------*/
/**
* @brief  Initialize EEPROM
* @param  None
* @retval EEPROM status
*/
uint16_t EEPROM_Init(void)
{
  /* Virtual address defined by the user: 0xFFFF value is prohibited */
  /*Initializing an array with the virtual address(starting from 0x6666)*/
  for(int index=0,virt_address=0x6666;index<128;index++,virt_address++)
    eeprom_variable[index]=virt_address;


  /*These two lines are to be used to see the data that is stored in the flash,
  emulated as EEPROM.The data can be seen in temp_read variable*/
  for(int i=0;i<128;i++)
    E2_ReadMem(eeprom_variable[i],&read_eeprom_variable[i]);

  uint16_t PageStatus0 = 6, PageStatus1 = 6;
  uint16_t VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;
  int16_t x = -1;
  uint16_t  FlashStatus;

  /* Get Page0 status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
  /* Get Page1 status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

  /* Check for invalid header states and repair if necessary */
  switch (PageStatus0)
  {
  case ERASED:
    if (PageStatus1 == VALID_PAGE) /* Page0 erased, Page1 valid */
    {
      /* Erase Page0 */
      FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else if (PageStatus1 == RECEIVE_DATA) /* Page0 erased, Page1 receive */
    {
      /* Erase Page0 */
      FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Mark Page1 as valid */
      FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
    {
      /* Erase both Page0 and Page1 and set Page0 as valid page */
      FlashStatus = EE_Format();
      /* If erase/program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    break;

  case RECEIVE_DATA:
    if (PageStatus1 == VALID_PAGE) /* Page0 receive, Page1 valid */
    {
      /* Transfer data from Page1 to Page0 */
      for (VarIdx = 0; VarIdx < NumbOfVar; VarIdx++)
      {
        if (( *(__IO uint16_t*)(PAGE0_BASE_ADDRESS + 6)) == eeprom_variable[VarIdx])
        {
          x = VarIdx;
        }
        if (VarIdx != x)
        {
          /* Read the last variables' updates */
          ReadStatus = E2_ReadMem(eeprom_variable[VarIdx], &DataVar);
          /* In case variable corresponding to the virtual address was found */
          if (ReadStatus != 0x1)
          {
            /* Transfer the variable to the Page0 */
            EepromStatus = EE_VerifyPageFullWriteVariable(eeprom_variable[VarIdx], DataVar);
            /* If program operation was failed, a Flash error code is returned */
            if (EepromStatus != FLASH_COMPLETE)
            {
              return EepromStatus;
            }
          }
        }
      }
      /* Mark Page0 as valid */
      FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Erase Page1 */
      FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else if (PageStatus1 == ERASED) /* Page0 receive, Page1 erased */
    {
      /* Erase Page1 */
      FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Mark Page0 as valid */
      FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else /* Invalid state -> format eeprom */
    {
      /* Erase both Page0 and Page1 and set Page0 as valid page */
      FlashStatus = EE_Format();
      /* If erase/program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    break;

  case VALID_PAGE:
    if (PageStatus1 == VALID_PAGE) /* Invalid state -> format eeprom */
    {
      /* Erase both Page0 and Page1 and set Page0 as valid page */
      FlashStatus = EE_Format();
      /* If erase/program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else if (PageStatus1 == ERASED) /* Page0 valid, Page1 erased */
    {
      /* Erase Page1 */
      FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    else /* Page0 valid, Page1 receive */
    {
      /* Transfer data from Page0 to Page1 */
      for (VarIdx = 0; VarIdx < NumbOfVar; VarIdx++)
      {
        if ((*(__IO uint16_t*)(PAGE1_BASE_ADDRESS + 6)) == eeprom_variable[VarIdx])
        {
          x = VarIdx;
        }
        if (VarIdx != x)
        {
          /* Read the last variables' updates */
          ReadStatus = E2_ReadMem(eeprom_variable[VarIdx], &DataVar);
          /* In case variable corresponding to the virtual address was found */
          if (ReadStatus != 0x1)
          {
            /* Transfer the variable to the Page1 */
            EepromStatus = EE_VerifyPageFullWriteVariable(eeprom_variable[VarIdx], DataVar);
            /* If program operation was failed, a Flash error code is returned */
            if (EepromStatus != FLASH_COMPLETE)
            {
              return EepromStatus;
            }
          }
        }
      }
      /* Mark Page1 as valid */
      FlashStatus = FLASH_ProgramHalfWord(PAGE1_BASE_ADDRESS, VALID_PAGE);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Erase Page0 */
      FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);
      /* If erase operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
    }
    break;

  default:  /* Any other state -> format eeprom */
    /* Erase both Page0 and Page1 and set Page0 as valid page */
    FlashStatus = EE_Format();
    /* If erase/program operation was failed, a Flash error code is returned */
    if (FlashStatus != FLASH_COMPLETE)
    {
      return FlashStatus;
    }
    break;
  }


  uint16_t eeprom_variable_data[4];
  E2_ReadMem(eeprom_variable[0],&eeprom_variable_data[0]);
  E2_ReadMem(eeprom_variable[1],&eeprom_variable_data[1]);
  E2_ReadMem(eeprom_variable[2],&eeprom_variable_data[2]);
  E2_ReadMem(eeprom_variable[3],&eeprom_variable_data[3]);
  if ((eeprom_variable_data[0]=='T') && (eeprom_variable_data[1]=='G') && (eeprom_variable_data[2]==8) && (eeprom_variable_data[3]==0x00))
  { //test on EEPROM content validity see E2_ResetEEPROM()
    DALIR_LoadRegsFromE2();  // short_addr = E2_ReadMem(DALIREG_SHORT_ADDRESS - DALIREG_EEPROM_START);
  }
  else
  {
    DALIR_DeleteShort();
    DALIR_ResetRegs();
    E2_ResetEEPROM();
  }

  return FLASH_COMPLETE;
}

/**
* @brief  Resets the EEPROM header
* @param  None
* @retval None
*/
void E2_ResetEEPROM(void)
{
  E2_WriteMem(eeprom_variable[0], 'T');  //"TG80" is written  in the EEPROM
  E2_WriteMem(eeprom_variable[1], 'G');  //This allows to check later that the EEPROM content is valid.
  E2_WriteMem(eeprom_variable[2], 8);
  E2_WriteMem(eeprom_variable[3], 0);
}


/**
* @brief  Writes/upadtes variable data in EEPROM.
* @param  VirtAddress: Variable virtual address
* @param  Data: 16 bit data to be written
* @retval Success or error status:
*           - FLASH_COMPLETE: on success
*           - PAGE_FULL: if valid page is full
*           - NO_VALID_PAGE: if no valid page was found
*           - Flash error code: on write Flash error
*/
uint16_t E2_WriteMem(u16 VirtAddress, u16 Data)
{
  // FLASH_Unlock();
  uint16_t Status = 0;

  /* Write the variable virtual address and value in the EEPROM */
  Status = EE_VerifyPageFullWriteVariable(VirtAddress, Data);

  /* In case the EEPROM active page is full */
  if (Status == PAGE_FULL)
  {
    /* Perform Page transfer */
    Status = EE_PageTransfer(VirtAddress, Data);
  }

  /* Return last operation status */
  return Status;
}

/**
* @brief  Writes/upadtes a burst of variable data in EEPROM.
* @param  addr: target starting address
* @param  times: no of data bytes
* @param  buf: pointer to data buffer to be written
* @retval None
*/
void E2_WriteBurst(u16 addr, u16 times, u16* buf)
{
  u16 address,i;
  uint16_t eeprom_variable_data;
  address = addr + 4;
  i = 0;
  while (times--)
  {
    E2_ReadMem(eeprom_variable[address+i],&eeprom_variable_data);
    if ((eeprom_variable_data)!= buf[i])
      E2_WriteMem(eeprom_variable[address+i],buf[i]);
    i++;
  }
}



/**
* @brief  Returns the last stored variable data, if found, which correspond to
*   the passed virtual address
* @param  VirtAddress: Variable virtual address
* @param  Data: Global variable contains the read variable value
* @retval Success or error status:
*           - 0: if variable was found
*           - 1: if the variable was not found
*           - NO_VALID_PAGE: if no valid page was found.
*/
u16 E2_ReadMem(u16 VirtAddress,uint16_t* Data)
{
  uint16_t ValidPage = PAGE0;
  uint16_t AddressValue = 0x5555, ReadStatus = 1;
  uint32_t Address = 0x08010000, PageStartAddress = 0x08010000;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  PageStartAddress = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  Address = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from end */
  while (Address > (PageStartAddress + 2))
  {
    /* Get the current location content to be compared with virtual address */
    AddressValue = (*(__IO uint16_t*)Address);

    /* Compare the read address with the virtual address */
    if (AddressValue == VirtAddress)
    {
      /* Get content of Address-2 which is variable value */
      *Data = (*(__IO uint16_t*)(Address - 2));

      /* In case variable value is read, reset ReadStatus flag */
      ReadStatus = 0;

      break;
    }
    else
    {
      /* Next address location */
      Address = Address - 4;
    }
  }

  /* Return ReadStatus value: (0: variable exist, 1: variable doesn't exist) */
  return ReadStatus;
}


/**
* @brief  Erases PAGE0 and PAGE1 and writes VALID_PAGE header to PAGE0
* @param  None
* @retval Status of the last operation (Flash write or erase) done during
*         EEPROM formating
*/
static FLASH_Status EE_Format(void)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;

  /* Erase Page0 */
  FlashStatus = FLASH_ErasePage(PAGE0_BASE_ADDRESS);

  /* If erase operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */
  FlashStatus = FLASH_ProgramHalfWord(PAGE0_BASE_ADDRESS, VALID_PAGE);

  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Erase Page1 */
  FlashStatus = FLASH_ErasePage(PAGE1_BASE_ADDRESS);

  /* Return Page1 erase operation status */
  return FlashStatus;
}

/**
* @brief  Find valid Page for write or read operation
* @param  Operation: operation to achieve on the valid page.
*   This parameter can be one of the following values:
*     @arg READ_FROM_VALID_PAGE: read operation from valid page
*     @arg WRITE_IN_VALID_PAGE: write operation from valid page
* @retval Valid page number (PAGE0 or PAGE1) or NO_VALID_PAGE in case
*   of no valid page was found
*/
static uint16_t EE_FindValidPage(uint8_t Operation)
{
  uint16_t PageStatus0 = 6, PageStatus1 = 6;

  /* Get Page0 actual status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);

  /* Get Page1 actual status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);

  /* Write or read operation */
  switch (Operation)
  {
  case WRITE_IN_VALID_PAGE:   /* ---- Write operation ---- */
    if (PageStatus1 == VALID_PAGE)
    {
      /* Page0 receiving data */
      if (PageStatus0 == RECEIVE_DATA)
      {
        return PAGE0;         /* Page0 valid */
      }
      else
      {
        return PAGE1;         /* Page1 valid */
      }
    }
    else if (PageStatus0 == VALID_PAGE)
    {
      /* Page1 receiving data */
      if (PageStatus1 == RECEIVE_DATA)
      {
        return PAGE1;         /* Page1 valid */
      }
      else
      {
        return PAGE0;         /* Page0 valid */
      }
    }
    else
    {
      return NO_VALID_PAGE;   /* No valid Page */
    }

  case READ_FROM_VALID_PAGE:  /* ---- Read operation ---- */
    if (PageStatus0 == VALID_PAGE)
    {
      return PAGE0;           /* Page0 valid */
    }
    else if (PageStatus1 == VALID_PAGE)
    {
      return PAGE1;           /* Page1 valid */
    }
    else
    {
      return NO_VALID_PAGE ;  /* No valid Page */
    }

  default:
    return PAGE0;             /* Page0 valid */
  }
}

/**
* @brief  Verify if active page is full and Writes variable in EEPROM.
* @param  VirtAddress: 16 bit virtual address of the variable
* @param  Data: 16 bit data to be written as variable value
* @retval Success or error status:
*           - FLASH_COMPLETE: on success
*           - PAGE_FULL: if valid page is full
*           - NO_VALID_PAGE: if no valid page was found
*           - Flash error code: on write Flash error
*/
static uint16_t EE_VerifyPageFullWriteVariable(uint16_t VirtAddress, uint16_t Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint16_t ValidPage = PAGE0;
  uint32_t Address = 0x08010000, PageEndAddress = 0x080107FF;

  /* Get valid Page for write operation */
  ValidPage = EE_FindValidPage(WRITE_IN_VALID_PAGE);

  /* Check if there is no valid page */
  if (ValidPage == NO_VALID_PAGE)
  {
    return  NO_VALID_PAGE;
  }

  /* Get the valid Page start Address */
  Address = (uint32_t)(EEPROM_START_ADDRESS + (uint32_t)(ValidPage * PAGE_SIZE));

  /* Get the valid Page end Address */
  PageEndAddress = (uint32_t)((EEPROM_START_ADDRESS - 2) + (uint32_t)((1 + ValidPage) * PAGE_SIZE));

  /* Check each active page address starting from begining */
  while (Address < PageEndAddress)
  {
    /* Verify if Address and Address+2 contents are 0xFFFFFFFF */
    if ((*(__IO uint32_t*)Address) == 0xFFFFFFFF)
    {
      /* Set variable data */
      FlashStatus = FLASH_ProgramHalfWord(Address, Data);
      /* If program operation was failed, a Flash error code is returned */
      if (FlashStatus != FLASH_COMPLETE)
      {
        return FlashStatus;
      }
      /* Set variable virtual address */
      FlashStatus = FLASH_ProgramHalfWord(Address + 2, VirtAddress);
      /* Return program operation status */
      return FlashStatus;
    }
    else
    {
      /* Next address location */
      Address = Address + 4;
    }
  }

  /* Return PAGE_FULL in case the valid page is full */
  return PAGE_FULL;
}

/**
* @brief  Transfers last updated variables data from the full Page to
*   an empty one.
* @param  VirtAddress: 16 bit virtual address of the variable
* @param  Data: 16 bit data to be written as variable value
* @retval Success or error status:
*           - FLASH_COMPLETE: on success
*           - PAGE_FULL: if valid page is full
*           - NO_VALID_PAGE: if no valid page was found
*           - Flash error code: on write Flash error
*/
static uint16_t EE_PageTransfer(uint16_t VirtAddress, uint16_t Data)
{
  FLASH_Status FlashStatus = FLASH_COMPLETE;
  uint32_t NewPageAddress = 0x080103FF, OldPageAddress = 0x08010000;
  uint16_t ValidPage = PAGE0, VarIdx = 0;
  uint16_t EepromStatus = 0, ReadStatus = 0;

  /* Get active Page for read operation */
  ValidPage = EE_FindValidPage(READ_FROM_VALID_PAGE);

  if (ValidPage == PAGE1)       /* Page1 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE0_BASE_ADDRESS;

    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE1_BASE_ADDRESS;
  }
  else if (ValidPage == PAGE0)  /* Page0 valid */
  {
    /* New page address where variable will be moved to */
    NewPageAddress = PAGE1_BASE_ADDRESS;

    /* Old page address where variable will be taken from */
    OldPageAddress = PAGE0_BASE_ADDRESS;
  }
  else
  {
    return NO_VALID_PAGE;       /* No valid Page */
  }

  /* Set the new Page status to RECEIVE_DATA status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, RECEIVE_DATA);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Write the variable passed as parameter in the new active page */
  EepromStatus = EE_VerifyPageFullWriteVariable(VirtAddress, Data);
  /* If program operation was failed, a Flash error code is returned */
  if (EepromStatus != FLASH_COMPLETE)
  {
    return EepromStatus;
  }

  /* Transfer process: transfer variables from old to the new active page */
  for (VarIdx = 0; VarIdx < NumbOfVar; VarIdx++)
  {
    if (eeprom_variable[VarIdx] != VirtAddress)  /* Check each variable except the one passed as parameter */
    {
      /* Read the other last variable updates */
      ReadStatus = E2_ReadMem(eeprom_variable[VarIdx], &DataVar);
      /* In case variable corresponding to the virtual address was found */
      if (ReadStatus != 0x1)
      {
        /* Transfer the variable to the new active page */
        EepromStatus = EE_VerifyPageFullWriteVariable(eeprom_variable[VarIdx], DataVar);
        /* If program operation was failed, a Flash error code is returned */
        if (EepromStatus != FLASH_COMPLETE)
        {
          return EepromStatus;
        }
      }
    }
  }

  /* Erase the old Page: Set old Page status to ERASED status */
  FlashStatus = FLASH_ErasePage(OldPageAddress);
  /* If erase operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Set new Page status to VALID_PAGE status */
  FlashStatus = FLASH_ProgramHalfWord(NewPageAddress, VALID_PAGE);
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != FLASH_COMPLETE)
  {
    return FlashStatus;
  }

  /* Return last operation flash status */
  return FlashStatus;
}


/**
* @}
*/




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
