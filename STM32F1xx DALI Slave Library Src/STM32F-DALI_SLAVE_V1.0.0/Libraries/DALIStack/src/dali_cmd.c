/**
  ******************************************************************************
  * @file    dali_cmd.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   DALI commands implementation (according DALI specification)
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
#include "dali_regs.h"
#include "dali_cmd.h"
#include "dali_pub.h"
#include "lite_timer_8bit.h"

/* Private macro -------------------------------------------------------------*/
#define DALI_REPETITION_WAIT 	120  /*Command repetition timeout (ms)*/
#define MEM_BANKS_CNT           2
#define MEM_BANK_SIZE           0x20

#define b_is_special            0
#define b_in_special_mode       1
#define b_is_selected           2
#define b_is_withdrawn          3
#define b_in_physical_selection 4
#define b_is_cmd_buffered       5
#define b_is_cmd_inbetween	    6

#define SetFlag(a) SetBit(b_status_reg,a)
#define ClrFlag(a) ClrBit(b_status_reg,a)
#define IsFlag(a)  ValBit(b_status_reg,a)


/* Private variables ---------------------------------------------------------*/
static u8 dtr;
static u8 dtr1;
static u8 dtr2;
static u8 write_enable_membanks;
static u8 iBufferedCmdHi,iBufferedCmdLo;
static u8 b_status_reg;

/* Extern variables ----------------------------------------------------------*/

#ifdef __ICCARM__
u8 membanks[MEM_BANKS_CNT][MEM_BANK_SIZE] @ "eeprom_zone"=
#else
u8 membanks[MEM_BANKS_CNT][MEM_BANK_SIZE]=
#endif
{
  // memory bank 0 - according specification
  {
    (MEM_BANK_SIZE-1)     , // 0x00 address of last accessible memory location factory burn in read-only
    (u8)(0 - (MEM_BANKS_CNT-1) - 0 - 1 - 2 - 3 - 4 - 5 - 1 - 0 - 0 - 0 - 0 - 1 - 'S' - 'T' - 'M' - '8' - 'D' - 'A' - 'L' - 'I' - ' ' - 'l' - 'i' - 'b' - 'r' - 'a' - 'r' - 'y' - '.')
      , // 0x01 checksum of memory bank 0 factory burn in read-only
      (MEM_BANKS_CNT-1)     , // 0x02 number of last accessible memory bank factory burn in read-only
      0                     , // 0x03 GTIN byte 0 (MSB) factory burn in read-only
      1                     , // 0x04 GTIN byte 1 factory burn in read-only
      2                     , // 0x05 GTIN byte 2 factory burn in read-only
      3                     , // 0x06 GTIN byte 3 factory burn in read-only
      4                     , // 0x07 GTIN byte 4 factory burn in read-only
      5                     , // 0x08 GTIN byte 5 factory burn in read-only
      1                     , // 0x09 control gear firmware version (major) factory burn in read-only
      0                     , // 0x0A control gear firmware version (minor) factory burn in read-only
      0                     , // 0x0B serial number byte 1 (MSB) factory burn in read-only
      0                     , // 0x0C serial number byte 2 factory burn in read-only
      0                     , // 0x0D serial number byte 3 factory burn in read-only
      1                     , // 0x0E serial number byte 4 factory burn in read-only
      'S','T','M','8','D','A','L','I',' ','l','i','b','r','a','r','y','.' // 0x0F- ... additional control gear information
  }
  ,
  // memory bank 1 - according specification
  {
    (MEM_BANK_SIZE-1)     , // 0x00 address of last accessible memory location factory burn in read-only
    30                    , // 0x01 checksum of memory bank 1 read-only
    0xFF                  , // 0x02 memory bank 1 lock byte (read-only if not 0x55) 0xFF read / write
    0xFF                  , // 0x03 OEM GTIN byte 0 (MSB) 0xFF read / write (lockable)
    0xFF                  , // 0x04 OEM GTIN byte 1 0xFF read / write (lockable)
    0xFF                  , // 0x05 OEM GTIN byte 2 0xFF read / write (lockable)
    0xFF                  , // 0x06 OEM GTIN byte 3 0xFF read / write (lockable)
    0xFF                  , // 0x07 OEM GTIN byte 4 0xFF read / write (lockable)
    0xFF                  , // 0x08 OEM GTIN byte 5 0xFF read / write (lockable)
    0xFF                  , // 0x09 OEM serial number byte 1 (MSB) 0xFF read / write (lockable)
    0xFF                  , // 0x0A OEM serial number byte 2 0xFF read / write (lockable)
    0xFF                  , // 0x0B OEM serial number byte 3 0xFF read / write (lockable)
    0xFF                  , // 0x0C OEM serial number byte 4 (LSB) 0xFF read / write (lockable)
    0xFF                  , // 0x0D Subsystem (bit 4 to bit 7) Device number (bit 0 to bit 3) 0xFF read / write (lockable)
    0xFF                  , // 0x0E Lamp type number (lockable) c 0xFF read / write (lockable)
    0xFF                  , // 0x0F Lamp type number 0xFF read / write
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF // 0x10- ... additional OEM information
  }

};

/* Private function prototypes -----------------------------------------------*/
void DALIC_ProcessSpecialCommand(void);
void DALIC_ProcessNormalCommand(void);

void DALIC_Direct_Arc(u8);
void DALIC_Direct_Arc_NoFade(u8 val);
void DALIC_Off(void);
void DALIC_Up(void);
void DALIC_Down(void);
void DALIC_Step_Up(void);
void DALIC_Step_Down(void);
void DALIC_Recall_Max_Level(void);
void DALIC_Step_Down_And_Off(void);
void DALIC_Enable_DAPC_Sequence(void);
void DALIC_Recall_Min_Level(void);
void DALIC_Go_To_Scene(u8);
void DALIC_Store_Act_Level_To_DTR(void);
void DALIC_Store_DTR_As_(u8);
void DALIC_Store_DTR_As_Max_Level(void);
void DALIC_Store_DTR_As_Min_Level(void);
void DALIC_Store_DTR_As_System_Failure_Level(void);
void DALIC_Store_DTR_As_Power_On_Level(void);
void DALIC_Store_DTR_As_Fade_Time(void);
void DALIC_Store_DTR_As_Fade_Rate(void);
void DALIC_Store_DTR_As_Scene(u8);
void DALIC_On_And_Step_Up(void);
void DALIC_Reset(void);
void DALIC_Remove_From_Scene(u8);
void DALIC_Add_To_Group(u8);
void DALIC_Remove_From_Group(u8);
void DALIC_Store_DTR_As_Short(void);
void DALIC_Enable_Write_Memory(void);
void DALIC_Query_Status(void);
void DALIC_Query_Ballast(void);
void DALIC_Query_Lamp_Failure(void);
void DALIC_Query_Lamp_Power_On(void);
void DALIC_Query_Limit_Error(void);
void DALIC_Query_Reset_State(void);
void DALIC_Query_Missing_Short_Address(void);
void DALIC_Query_Content_DTR(void);
void DALIC_Query_Content_DTR1(void);
void DALIC_Query_Content_DTR2(void);
void DALIC_Query_Reg_(u8);
void DALIC_Query_Device_Type(void);
void DALIC_Query_Power_Failure(void);
void DALIC_Query_Fade_Time_Rate(void);
void DALIC_Query_Application_Extended_Commands(u8);
void DALIC_Query_Application_Extended_Version_Number(void);
void DALIC_Query_Reg_Version_Number(void);
void DALIC_Query_Reg_Phys_Min_Level(void);
void DALIC_Query_Reg_Actual_Dim_Level(void);
void DALIC_Query_Reg_Max_Level(void);
void DALIC_Query_Reg_Min_Level(void);
void DALIC_Query_Reg_Power_On_Level(void);
void DALIC_Query_Reg_System_Failure_Level(void);
void DALIC_Query_Reg_Scene(u8);
void DALIC_Query_Reg_Group_0_7(void);
void DALIC_Query_Reg_Group_8_15(void);
void DALIC_Query_Reg_Random_Address0(void);
void DALIC_Query_Reg_Random_Address1(void);
void DALIC_Query_Reg_Random_Address2(void);
void DALIC_Read_Memory_Location(void);
void DALIC_Power_On(void);

void DALIC_Terminate(void);
void DALIC_SetDTR(u8);
void DALIC_SetDTR1(u8);
void DALIC_SetDTR2(u8);
void DALIC_Initialize(u8);
void DALIC_Randomize(void);
void DALIC_Compare(void);
void DALIC_Withdraw(void);
void DALIC_SetSearchAddress0(u8);
void DALIC_SetSearchAddress1(u8);
void DALIC_SetSearchAddress2(u8);
void DALIC_Program_Short_Address(u8);
void DALIC_Verify_Short_Address(u8);
void DALIC_Query_Short_Address(void);
void DALIC_Physical_Selection(void);
void DALIC_Initialize_Select(u8);
void DALIC_Enable_Device_Type_X(u8);
void DALIC_Write_Memory_Location(u8);
u8 DALIC_Mem_Checksum(u8);
void DALIC_Adjust_Actual_Level(void);

typedef void (*const TFuncPointer) (u8);

TFuncPointer normal_jt[]={
  /* Arc Power Control Commands */
  (TFuncPointer) DALIC_Off,                            /* Command 000 */
  (TFuncPointer) DALIC_Up,
  (TFuncPointer) DALIC_Down,
  (TFuncPointer) DALIC_Step_Up,
  (TFuncPointer) DALIC_Step_Down,
  (TFuncPointer) DALIC_Recall_Max_Level,		/* Command 005 */
  (TFuncPointer) DALIC_Recall_Min_Level,
  (TFuncPointer) DALIC_Step_Down_And_Off,
  (TFuncPointer) DALIC_On_And_Step_Up,
  (TFuncPointer) DALIC_Enable_DAPC_Sequence,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 010 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 015 */
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,			/* Command 020 */
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,			/* Command 025 */
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,
  (TFuncPointer) DALIC_Go_To_Scene,			/* Command 030 */
  (TFuncPointer) DALIC_Go_To_Scene,

  /* Configuration Commands */
  (TFuncPointer) DALIC_Reset,
  (TFuncPointer) DALIC_Store_Act_Level_To_DTR,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 035 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 040 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIC_Store_DTR_As_Max_Level,
  (TFuncPointer) DALIC_Store_DTR_As_Min_Level,
  (TFuncPointer) DALIC_Store_DTR_As_System_Failure_Level,
  (TFuncPointer) DALIC_Store_DTR_As_Power_On_Level,	/* Command 045 */
  (TFuncPointer) DALIC_Store_DTR_As_Fade_Time,
  (TFuncPointer) DALIC_Store_DTR_As_Fade_Rate,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,      	    /* Command 050 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,      	    /* Command 055 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,      	    /* Command 060 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,            	/* Command 065 */
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,            	/* Command 070 */
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,             /* Command 075 */
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Store_DTR_As_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,      		/* Command 080 */
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,      		/* Command 085 */
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,      		/* Command 090 */
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,
  (TFuncPointer) DALIC_Remove_From_Scene,      		/* Command 095 */
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,   		    	/* Command 100 */
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,   		  	/* Command 105 */
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Add_To_Group,   		  	/* Command 110 */
  (TFuncPointer) DALIC_Add_To_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,              /* Command 115 */
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,              /* Command 120 */
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,           	/* Command 125 */
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Remove_From_Group,
  (TFuncPointer) DALIC_Store_DTR_As_Short,
  (TFuncPointer) DALIC_Enable_Write_Memory,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 130 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 135 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 140 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,

  /* Query Commands */
  (TFuncPointer) DALIC_Query_Status,
  (TFuncPointer) DALIC_Query_Ballast,			/* Command 145 */
  (TFuncPointer) DALIC_Query_Lamp_Failure,
  (TFuncPointer) DALIC_Query_Lamp_Power_On,
  (TFuncPointer) DALIC_Query_Limit_Error,
  (TFuncPointer) DALIC_Query_Reset_State,
  (TFuncPointer) DALIC_Query_Missing_Short_Address,   	/* Command 150 */
  (TFuncPointer) DALIC_Query_Reg_Version_Number,
  (TFuncPointer) DALIC_Query_Content_DTR,
  (TFuncPointer) DALIC_Query_Device_Type,
  (TFuncPointer) DALIC_Query_Reg_Phys_Min_Level,
  (TFuncPointer) DALIC_Query_Power_Failure,		/* Command 155 */
  (TFuncPointer) DALIC_Query_Content_DTR1,
  (TFuncPointer) DALIC_Query_Content_DTR2,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIC_Query_Reg_Actual_Dim_Level,     /* Command 160 */
  (TFuncPointer) DALIC_Query_Reg_Max_Level,
  (TFuncPointer) DALIC_Query_Reg_Min_Level,
  (TFuncPointer) DALIC_Query_Reg_Power_On_Level,
  (TFuncPointer) DALIC_Query_Reg_System_Failure_Level,
  (TFuncPointer) DALIC_Query_Fade_Time_Rate,		/* Command 165 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 170 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,              /* Command 175 */
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,			/* Command 180 */
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,			/* Command 185 */
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Scene,			/* Command 190 */
  (TFuncPointer) DALIC_Query_Reg_Scene,
  (TFuncPointer) DALIC_Query_Reg_Group_0_7,
  (TFuncPointer) DALIC_Query_Reg_Group_8_15,
  (TFuncPointer) DALIC_Query_Reg_Random_Address0,
  (TFuncPointer) DALIC_Query_Reg_Random_Address1,	/* Command 195 */
  (TFuncPointer) DALIC_Query_Reg_Random_Address2,
  (TFuncPointer) DALIC_Read_Memory_Location,      /* Command 197 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,		/* Command 200 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,		/* Command 205 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,		/* Command 210 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,		/* Command 215 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,		/* Command 220 */
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIP_Reserved_Function,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 225 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 230 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 235 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 240 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 245 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,/* Command 250 */
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Commands,
  (TFuncPointer) DALIC_Query_Application_Extended_Version_Number,/* Command 255 */
};

TFuncPointer special_jt[]={
  (TFuncPointer) DALIC_Terminate,                      /* Command 256 */
  (TFuncPointer) DALIC_SetDTR,
  (TFuncPointer) DALIC_Initialize,
  (TFuncPointer) DALIC_Randomize,
  (TFuncPointer) DALIC_Compare,
  (TFuncPointer) DALIC_Withdraw,
  (TFuncPointer) DALIP_Reserved_Special_Function,
  (TFuncPointer) DALIP_Reserved_Special_Function,
  (TFuncPointer) DALIC_SetSearchAddress0,              /* Command 264 */
  (TFuncPointer) DALIC_SetSearchAddress1,
  (TFuncPointer) DALIC_SetSearchAddress2,
  (TFuncPointer) DALIC_Program_Short_Address,
  (TFuncPointer) DALIC_Verify_Short_Address,
  (TFuncPointer) DALIC_Query_Short_Address,
  (TFuncPointer) DALIC_Physical_Selection,
  (TFuncPointer) DALIP_Reserved_Special_Function,
  (TFuncPointer) DALIC_Enable_Device_Type_X,           /* Command 272 */
  (TFuncPointer) DALIC_SetDTR1,                        /* Command 273 */
  (TFuncPointer) DALIC_SetDTR2,                        /* Command 274 */
  (TFuncPointer) DALIC_Write_Memory_Location,          /* Command 275 */
};

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

void DALIC_Init(void)
{
  b_status_reg = 0;
  write_enable_membanks = 0;

  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,1);		//ALAL to set the "power failure" bit after a power on (to solve error 6.1.5)

  if (DALIR_ReadReg(DALIREG_SHORT_ADDRESS)==0xFF)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,1);
  }
}

void  DALIC_PowerOn(void)
{
  if (DALIR_ReadReg(DALIREG_POWER_ON_LEVEL) != 255)
    DALIC_Direct_Arc_NoFade(DALIR_ReadReg(DALIREG_POWER_ON_LEVEL));
  else
    DALIC_Direct_Arc_NoFade(DALIR_ReadReg(DALIREG_MIN_LEVEL));
}

u8 DALIC_isTalkingToMe(void)
{
  u8 addr;
  u8 zw;

  ClrFlag(b_is_special);

  addr = dali_address;

  if (((addr & 0xE1) == 0xA1) || ((addr & 0xE1) == 0xC1))
  { /* Special command */
    SetFlag(b_is_special);
    return 1;
  }

  if ((addr & 0xFE) == 0xFE)
    return 1;        /* Broadcast */

  if ((addr & 0xE0) == 0x80)
  {                 /* it's a group address */
    addr &= 0x1E;
    addr = addr>>1;
    if (addr<8)
    {
      zw = 1<<addr;

      if ((DALIR_ReadReg(DALIREG_GROUP_0_7) & zw) != 0)
        return 1;  /* Ballast belongs to this group */
    }
    else
    {
      addr -= 8;
      zw = 1<<addr;

      if ((DALIR_ReadReg(DALIREG_GROUP_8_15) & zw) != 0)
        return 1;
    }
    return 0;
  }

  addr = addr|1; // to recognize the Short Address

  if (addr ==DALIR_ReadReg(DALIREG_SHORT_ADDRESS) )
    return 1;  /* Command is directed to this ballast */

  return 0; /* ignore command */
}


void DALIC_Direct_Arc_NoFade(u8 val)
{
  u8 zw;

  if (val==255)
  {
    DALIP_DoneTimer();
    return;
  }

  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);

  if (val==0)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 0);
    DALIP_Off();
    return;
  }
  zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  if (val < zw)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
    DALIP_Direct_Arc_NoFade(zw);
    return;
  }
  zw = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  if (val > zw)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
    DALIP_Direct_Arc_NoFade(zw);
    return;
  }
  DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
  DALIP_Direct_Arc_NoFade(val);
}


void DALIC_Process_System_Failure(void)
{
  u8 zw;
  u8 val;

  val = DALIR_ReadReg(DALIREG_SYSTEM_FAILURE_LEVEL);
  if (val!=255)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);

    if (val==0)
    {
      DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 0);
      DALIP_Off();
      return;
    }
    zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
    if (val < zw)
    {
      DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
      DALIP_Direct_Arc_NoFade(zw);
      return;
    }
    zw = DALIR_ReadReg(DALIREG_MAX_LEVEL);
    if (val > zw)
    {
      DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
      DALIP_Direct_Arc_NoFade(zw);
      return;
    }
    DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
    DALIP_Direct_Arc_NoFade(val);
  }
}

/************************************************************************************
* Buffers actual command, returns true if the command has been correctly           *
* repeated                                                                         *
************************************************************************************/
u8 DALIC_Is_Repeated(void)
{
  if (!IsFlag(b_is_cmd_buffered))
  {
    iBufferedCmdHi = dali_address;
    iBufferedCmdLo = dali_data;
    SetFlag(b_is_cmd_buffered);
    RTC_LaunchTimer(DALI_REPETITION_WAIT);
  }
  else
  {
    ClrFlag(b_is_cmd_buffered);
    if (RealTimeClock_TimerCountDown == 0)
      return 0; /* Timeout */
    if ((iBufferedCmdHi == dali_address) && (iBufferedCmdLo == dali_data))
    {
      return 1;
    }
  }
  return 0;
}

/************************************************************************************
* Returns false //ALAL true instead of false// only when an repetition-fault occurs.*
* According to the Specification, an other command inbetween                       *
* an expected repetition is ignored and leads to a cancelation                     *
* of the repetition-sequence                                                       *
************************************************************************************/
#define DCRF_OK		0
#define	DCRF_IGNORE	1

#define DCRF_NOTIMEOUT	0x00
#define DCRF_UNEQUAL	0x00
#define DCRF_TIMEOUT	0x01
#define DCRF_EQUAL	0x10
u8 DALIC_Is_Repetiton_Fault(void)
{
  u8 state;

  if (!IsFlag(b_is_cmd_buffered))
    return DCRF_OK;

  state = 0;
  if (RealTimeClock_TimerCountDown == 0)
    state += DCRF_TIMEOUT;
  if ((iBufferedCmdHi == dali_address) && (iBufferedCmdLo == dali_data))
    state += DCRF_EQUAL;
  switch (state)
  {
  case DCRF_NOTIMEOUT + DCRF_UNEQUAL:
    /*--------------------------------------*
    | Ignore this Command, keep on waiting |
    *--------------------------------------*/
    SetFlag(b_is_cmd_inbetween);
    return DCRF_IGNORE;

  case DCRF_TIMEOUT + DCRF_UNEQUAL:
  case DCRF_TIMEOUT + DCRF_EQUAL:
    /*--------------------------------------*
    | In Timeout case, clear the Buffer    |
    | execute this command                 |
    *--------------------------------------*/
    ClrFlag(b_is_cmd_buffered);
    ClrFlag(b_is_cmd_inbetween);
    return DCRF_OK;

  case DCRF_NOTIMEOUT + DCRF_EQUAL:
    /*--------------------------------------*
    | A correct repetion, if no command    |
    | was sent inbetween                   |
    *--------------------------------------*/
    if (IsFlag(b_is_cmd_inbetween))
    {
      ClrFlag(b_is_cmd_buffered);
      ClrFlag(b_is_cmd_inbetween);
      return DCRF_IGNORE;
    }
    else
    {
      return DCRF_OK;
    }

  default:
    ClrFlag(b_is_cmd_buffered);
    ClrFlag(b_is_cmd_inbetween);
    return DCRF_OK;
  }
}

void DALIC_ProcessCommand(void)
{
  if (DALIC_Is_Repetiton_Fault())
    return;
  if (IsFlag(b_is_special))
  {
    DALIC_ProcessSpecialCommand();
  }
  else
  {
    DALIC_ProcessNormalCommand();
  }
}

void DALIC_ProcessSpecialCommand(void)
{
  u8 cmd,data_val;

  cmd = (dali_address-161)>>1;
  data_val = dali_data;
  ClrFlag(b_is_selected);
  if (cmd != (275 - 256)) //if not write command
    write_enable_membanks = 0;
  if (cmd > (275 -256))
  {
    DALIP_Reserved_Special_Function(cmd,data_val);
    return;
  }
  special_jt[cmd](data_val);
}

void DALIC_ProcessNormalCommand(void)
{
  u8 cmd;

  cmd = dali_data;

  if (cmd < 0xE0)
    ClrFlag(b_is_selected);
  write_enable_membanks = 0;
  if(!(dali_address & 0x01))
  { /* Direct arc */
    DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
    DALIC_Direct_Arc(cmd);
    return;
  }
  normal_jt[cmd](cmd);
}

void DALIC_Direct_Arc(u8 val)
{
  u8 zw;

  if (val==255)
  {
    DALIP_DoneTimer();
    return;
  }

  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);

  if (val==0)
  {
    if (DALIP_GetFadeTime() == 0)
    {
      DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 0);
      DALIP_Off();
    }
    else
    {
      DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
      DALIP_Direct_Arc(val);
    }
    return;
  }

  zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  if (DALIP_GetArc() < zw)
    DALIP_Direct_Arc_NoFade(zw);
  if (val < zw)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
    DALIP_Direct_Arc(zw);
    return;
  }
  zw = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  if (DALIP_GetArc() > zw)
    DALIP_Direct_Arc_NoFade(zw);
  if (val > zw)
  {
    DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,1);
    DALIP_Direct_Arc(zw);
    return;
  }
  DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
  DALIP_Direct_Arc(val);
}

void DALIC_Recall_Max_Level(void)
{
  DALIP_Stop_DAPC_Sequence();
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);
  DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
  DALIP_Recall_Max_Level();
}

void DALIC_Recall_Min_Level(void)
{
  DALIP_Stop_DAPC_Sequence();
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);
  DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
  DALIP_Recall_Min_Level();
}

/* Executing a "Go To Scene" Command by reading
the corresponding scene-reg and calling DirectArc */
void DALIC_Go_To_Scene(u8 idx)
{
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
  DALIC_Direct_Arc(DALIR_ReadReg(DALIREG_SCENE + (idx & 0x0F)));
}

void DALIC_Store_Act_Level_To_DTR(void)
{
  if (!DALIC_Is_Repeated())
    return;
  dtr = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
}

void DALIC_Store_DTR_As_(u8 idx)
{
  if (!DALIC_Is_Repeated())
    return;
  DALIR_WriteReg(idx,dtr);
}

void DALIC_StoreMinMax_DTR_As_(u8 idx)
{
  u8 zw;

  if (!DALIC_Is_Repeated())
    return;

  zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  if (dtr < zw)
  {
    DALIR_WriteReg(idx,zw);
    return;
  }
  zw = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  if (dtr > zw)
  {
    DALIR_WriteReg(idx,zw);
    return;
  }
  DALIR_WriteReg(idx,dtr);
}

void DALIC_Remove_From_Scene(u8 idx)
{
  if (!DALIC_Is_Repeated())
    return;
  DALIR_WriteReg(DALIREG_SCENE+(idx & 0x0F),0xFF);
}

void DALIC_Add_To_Group(u8 grp)
{
  u8 izwgrp;

  if (!DALIC_Is_Repeated())
    return;
  grp &= 0x0F;
  if (grp < 8)
  {
    izwgrp = DALIR_ReadReg(DALIREG_GROUP_0_7);
    izwgrp |= 1<<grp;
    DALIR_WriteReg(DALIREG_GROUP_0_7,izwgrp);
  } else {
    grp -= 8;
    izwgrp = DALIR_ReadReg(DALIREG_GROUP_8_15);
    izwgrp |= 1<<grp;
    DALIR_WriteReg(DALIREG_GROUP_8_15,izwgrp);
  }
}

void DALIC_Remove_From_Group(u8 grp)
{
  u8 izwgrp;

  if (!DALIC_Is_Repeated())
    return;
  grp &= 0x0F;
  if (grp < 8)
  {
    izwgrp = DALIR_ReadReg(DALIREG_GROUP_0_7);
    izwgrp &= (1<<grp)^255;
    DALIR_WriteReg(DALIREG_GROUP_0_7,izwgrp);
  }
  else
  {
    grp -= 8;
    izwgrp = DALIR_ReadReg(DALIREG_GROUP_8_15);
    izwgrp &= (1<<grp)^255;
    DALIR_WriteReg(DALIREG_GROUP_8_15,izwgrp);
  }
}

void DALIC_Up(void)
{
  register u8 zw;

  DALIP_Stop_DAPC_Sequence();
  zw = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  if (zw==0)
    return;
  if (zw < DALIR_ReadReg(DALIREG_MAX_LEVEL))
  {
    DALIP_Up();
  }
}

void DALIC_Down(void)
{
  DALIP_Stop_DAPC_Sequence();
  if (DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL) == 0)
    return;
  DALIP_Down();
}

void DALIC_Step_Up(void)
{
  register u8 zw;

  DALIP_Stop_DAPC_Sequence();
  zw = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  if (zw == 0) return;
  if (zw < DALIR_ReadReg(DALIREG_MAX_LEVEL)) DALIP_Step_Up();
}

void DALIC_Step_Down(void)
{
  register u8  zw;

  DALIP_Stop_DAPC_Sequence();
  zw = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  if (zw == 0)
    return;
  if (zw > DALIR_ReadReg(DALIREG_MIN_LEVEL))
    DALIP_Step_Down();
}

void DALIC_Store_DTR_As_Short(void)
{

  if (!DALIC_Is_Repeated())
    return;
  if (dtr == 255)
  {
    DALIR_DeleteShort();
    DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,1);
    return;
  }
  if (dtr & 0x80)
    return;
  if (!(dtr & 0x01))
    return;
  DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,0);
  DALIR_WriteReg(DALIREG_SHORT_ADDRESS,dtr);
}

void DALIC_Query_Status(void)
{
  Send_DALI_Frame(DALIR_ReadReg(DALIREG_STATUS_INFORMATION));
}

void DALIC_Query_Ballast(void)
{
  if (!DALIR_ReadStatusBit(DALIREG_STATUS_BALLAST))
    Send_DALI_Frame(0xFF);           /* Answer with Yes */
}

void DALIC_Query_Lamp_Failure(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_LAMP_FAILURE))
    Send_DALI_Frame(0xFF);          /* Answer with Yes */
}

void DALIC_Query_Lamp_Power_On(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON))
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Limit_Error(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_LIMIT_ERROR))
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Reset_State(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_RESET_STATE))
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Missing_Short_Address(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_MISSING_SHORT))
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Reg_(u8 reg)
{
  Send_DALI_Frame(DALIR_ReadReg(reg));
}

void DALIC_Query_Content_DTR(void)
{
  Send_DALI_Frame(dtr);
}

void DALIC_Query_Content_DTR1(void)
{
  Send_DALI_Frame(dtr1);
}

void DALIC_Query_Content_DTR2(void)
{
  Send_DALI_Frame(dtr2);
}

void DALIC_Query_Device_Type(void)
{
  Send_DALI_Frame(DALIP_What_Device_Type());
}

void DALIC_Query_Power_Failure(void)
{
  if (DALIR_ReadStatusBit(DALIREG_STATUS_POWER_FAILURE))
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Fade_Time_Rate(void)
{
  u8 zw;

  zw = DALIR_ReadReg(DALIREG_FADE_TIME)<<4;
  zw += DALIR_ReadReg(DALIREG_FADE_RATE);
  Send_DALI_Frame(zw);
}

void DALIC_Query_Application_Extended_Commands(u8 cmd)
{
  if (!IsFlag(b_is_selected))
    return;
  if (DALIP_Ext_Cmd_Is_Answer_Required(cmd))
  {
    if (DALIP_Ext_Cmd_Is_Answer_YesNo(cmd))
    {
      if (DALIP_Extended_Command(cmd))
        Send_DALI_Frame(0xFF);
    }
    else
    {
      Send_DALI_Frame(DALIP_Extended_Command(cmd));
    }
  }
  else
  {
    DALIP_Extended_Command(cmd);
  }
}

void DALIC_Query_Application_Extended_Version_Number(void)
{
  if (!IsFlag(b_is_selected))
    return;
  Send_DALI_Frame(DALIP_Extended_Version_Number());
}

u8 DALIC_Is_Selected(void)
{
  u8 zw;

  if (!IsFlag(b_in_physical_selection))
  {
    zw = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 0);
    if (DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 0) != zw)
      return 0;
    zw = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 1);
    if (DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 1) != zw)
      return 0;
    zw = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 2);
    if (DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 2) != zw)
      return 0;
    return 1;
  }
  return DALIP_Is_Physically_Selected();
}

void DALIC_LaunchInit(void)
{
  RTC_LaunchBigTimer(15); /* Start a 15-Minute-Timer */

  SetFlag(b_in_special_mode);
  ClrFlag(b_is_withdrawn);
  ClrFlag(b_in_physical_selection);
}

void DALIC_Initialize(u8 addr)
{

  if (!DALIC_Is_Repeated())
    return;
  if ((IsFlag(b_is_withdrawn)) && (IsFlag(b_in_special_mode)))
    return;
  switch (addr)
  {
  case 0x00:   /* normal initialize */
    DALIC_LaunchInit();
    break;

  case 0xFF:	 /* initialize on missing short address */
    if (DALIR_ReadReg(DALIREG_SHORT_ADDRESS)==0xFF)
    {
      DALIC_LaunchInit();
    }
    break;

  default:	 /* initialize if own short address recognized */
    dali_address = addr;
    if (DALIC_isTalkingToMe())
    {
      DALIC_LaunchInit();
    }
    break;
  }
}

void DALIC_Terminate(void)
{
  ClrFlag(b_in_special_mode);
  RealTimeClock_BigTimer = 0;
}

void DALIC_Randomize(void)
{
  if (!DALIC_Is_Repeated())
    return;

  if (!RealTimeClock_BigTimer)
    return;

  DALIR_WriteReg(DALIREG_RANDOM_ADDRESS + 0,Get_DALI_Random()); // take value from AR_Timer
  DALIR_WriteReg(DALIREG_RANDOM_ADDRESS + 1,Get_DALI_Random()); // take value from AR_Timer
  DALIR_WriteReg(DALIREG_RANDOM_ADDRESS + 2,Get_DALI_Random()); // take value from AR_Timer
}

void DALIC_Compare(void)
{
  u8 search[3],rand[3],i;

  if (!RealTimeClock_BigTimer)
    return;
  if (IsFlag(b_is_withdrawn))
    return;


  search[0] = DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 0);
  search[1] = DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 1);
  search[2] = DALIR_ReadReg(DALIREG_SEARCH_ADDRESS + 2);

  rand[0] = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 0);
  rand[1] = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 1);
  rand[2] = DALIR_ReadReg(DALIREG_RANDOM_ADDRESS + 2);

  for (i=0;i<3;i++)
  {
    if (rand[i]>search[i])
      return;
    if (rand[i]<search[i])
    {
      Send_DALI_Frame(0xFF);
      return;
    }
  }
  Send_DALI_Frame(0xFF); //Here, the random and search-addresses are equal
}


void DALIC_Withdraw(void)
{
  if (!RealTimeClock_BigTimer)
    return;

  if (DALIC_Is_Selected())
    SetFlag(b_is_withdrawn);
}

void DALIC_Program_Short_Address(u8 addr)
{

  if (!RealTimeClock_BigTimer)
    return;

  if (addr == 0xFF)
  { /* Delete short */
    DALIR_DeleteShort();
    DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,1);
    return;
  }
  if (addr & 0x80)
    return; /* only valid up to 7E */

  if (DALIC_Is_Selected())
  {
    DALIR_WriteReg(DALIREG_SHORT_ADDRESS,addr);
    DALIR_WriteStatusBit(DALIREG_STATUS_MISSING_SHORT,0);
  }
}

void DALIC_Verify_Short_Address(u8 addr)
{
  if (addr & 0x80) return; /* only valid up to 7E */
  addr = addr>>1;
  if (((DALIR_ReadReg(DALIREG_SHORT_ADDRESS)&0xFE)>>1) == addr)
    Send_DALI_Frame(0xFF);
}

void DALIC_Query_Short_Address(void)
{
  u8 zw;
  if (!RealTimeClock_BigTimer)
    return;
  if (DALIC_Is_Selected())
  {
    zw = DALIR_ReadReg(DALIREG_SHORT_ADDRESS);
    Send_DALI_Frame(zw);
  }
}

void DALIC_Physical_Selection(void)
{
  if (!RealTimeClock_BigTimer)
    return;
  SetFlag(b_in_physical_selection);
}

void DALIC_Enable_Device_Type_X(u8 devtype)
{
  if (DALIP_What_Device_Type()==devtype)
    SetFlag(b_is_selected);
}

void DALIC_SetDTR(u8 newdtr)
{
  dtr = newdtr;
}

void DALIC_SetDTR1(u8 newdtr)
{
  dtr1 = newdtr;
}

void DALIC_SetDTR2(u8 newdtr)
{
  dtr2 = newdtr;
}

void DALIC_SetSearchAddress0(u8 newsearch)
{
  if (!RealTimeClock_BigTimer)
    return;
  DALIR_WriteReg(DALIREG_SEARCH_ADDRESS + 0, newsearch);
}

void DALIC_SetSearchAddress1(u8 newsearch)
{
  if (!RealTimeClock_BigTimer)
    return;
  DALIR_WriteReg(DALIREG_SEARCH_ADDRESS + 1, newsearch);
}

void DALIC_SetSearchAddress2(u8 newsearch)
{
  if (!RealTimeClock_BigTimer)
    return;
  DALIR_WriteReg(DALIREG_SEARCH_ADDRESS + 2, newsearch);
}

void DALIC_Off(void)
{
  DALIP_Stop_DAPC_Sequence();
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON,0);
  DALIR_WriteStatusBit(DALIREG_STATUS_LIMIT_ERROR,0);
  DALIP_Off();
}

void DALIC_Step_Down_And_Off(void)
{
  register u8 zw;

  DALIP_Stop_DAPC_Sequence();
  zw = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);

  if (zw <= DALIR_ReadReg(DALIREG_MIN_LEVEL))
  {
    DALIC_Off();
  }
  else
  {
    DALIP_Step_Down_And_Off();
  }
}

void DALIC_On_And_Step_Up(void)
{
  register u8 zw;

  DALIP_Stop_DAPC_Sequence();
  zw = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
  DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON, 1);
  if (zw < DALIR_ReadReg(DALIREG_MAX_LEVEL))
    DALIP_On_And_Step_Up();
}

void DALIC_Enable_DAPC_Sequence(void)
{
  DALIP_Enable_DAPC_Sequence();
}

void DALIC_Reset(void)
{
  u8 zw;

  if (!DALIC_Is_Repeated())
    return;
  DALIP_DoneTimer();
  zw = DALIP_GetArc();
  DALIR_ResetRegs();
  DALIC_Direct_Arc_NoFade(zw);
  DALIC_PowerOn();
}

u8 DALIC_BoundPhys(u8 val)
{
  u8 min;
  if (val == 255)
    return 254;

  min = DALIR_ReadReg(DALIREG_PHYS_MIN_LEVEL);
  return (val<min ? min : val);
}

void DALIC_Adjust_Actual_Level(void)
{
  u8 min,max,act;

  min = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  max = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  act = DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
  if(!act)
    return;
  if (act<min)
  {
    DALIC_Direct_Arc(min);
  }
  if (max<act)
  {
    DALIC_Direct_Arc(max);
  }
}

void DALIC_Store_DTR_As_Max_Level(void)
{
  register u8 zw,zw2;

  if (!DALIC_Is_Repeated())
    return;

  zw = DALIC_BoundPhys(dtr);
  zw2 = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  if (zw<zw2)
    zw = zw2;
  DALIR_WriteReg(DALIREG_MAX_LEVEL,zw);
  DALIC_Adjust_Actual_Level();
}

void DALIC_Store_DTR_As_Min_Level(void)
{
  register u8 zw,zw2;

  if (!DALIC_Is_Repeated())
    return;
  zw = DALIC_BoundPhys(dtr);
  zw2 = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  if (zw>zw2)
    zw = zw2;
  DALIR_WriteReg(DALIREG_MIN_LEVEL,zw);
  DALIC_Adjust_Actual_Level();
}

void DALIC_Store_DTR_As_System_Failure_Level(void)
{
  DALIC_Store_DTR_As_(DALIREG_SYSTEM_FAILURE_LEVEL);
}

void DALIC_Store_DTR_As_Power_On_Level(void)
{
  DALIC_Store_DTR_As_(DALIREG_POWER_ON_LEVEL);
}

void DALIC_Store_DTR_As_Fade_Time(void)
{
  if (!DALIC_Is_Repeated())
    return;
  if (dtr < 15)
    DALIR_WriteReg(DALIREG_FADE_TIME,dtr);
  else
    DALIR_WriteReg(DALIREG_FADE_TIME,15);
}

void DALIC_Store_DTR_As_Fade_Rate(void)
{
  register u8 zw;

  if (!DALIC_Is_Repeated())
    return;
  zw = dtr;
  if (zw > 15)
    zw = 15;
  if (zw == 0)
    zw = 1;
  DALIR_WriteReg(DALIREG_FADE_RATE,zw);
}

void DALIC_Store_DTR_As_Scene(u8 idx)
{
  DALIC_Store_DTR_As_(DALIREG_SCENE+(idx & 0x0F));
}

void DALIC_Query_Reg_Version_Number(void)
{
  DALIC_Query_Reg_(DALIREG_VERSION_NUMBER);
}

void DALIC_Query_Reg_Phys_Min_Level(void)
{
  DALIC_Query_Reg_(DALIREG_PHYS_MIN_LEVEL);
}

void DALIC_Query_Reg_Actual_Dim_Level(void)
{
  DALIC_Query_Reg_(DALIREG_ACTUAL_DIM_LEVEL);
}

void DALIC_Query_Reg_Max_Level(void)
{
  DALIC_Query_Reg_(DALIREG_MAX_LEVEL);
}

void DALIC_Query_Reg_Min_Level(void)
{
  DALIC_Query_Reg_(DALIREG_MIN_LEVEL);
}

void DALIC_Query_Reg_Power_On_Level(void)
{
  DALIC_Query_Reg_(DALIREG_POWER_ON_LEVEL);
}

void DALIC_Query_Reg_System_Failure_Level(void)
{
  DALIC_Query_Reg_(DALIREG_SYSTEM_FAILURE_LEVEL);
}

void DALIC_Query_Reg_Scene(u8 idx)
{
  DALIC_Query_Reg_(DALIREG_SCENE + (idx & 0x0F));
}

void DALIC_Query_Reg_Group_0_7(void)
{
  DALIC_Query_Reg_(DALIREG_GROUP_0_7);
}

void DALIC_Query_Reg_Group_8_15(void)
{
  DALIC_Query_Reg_(DALIREG_GROUP_8_15);
}

void DALIC_Query_Reg_Random_Address0(void)
{
  DALIC_Query_Reg_(DALIREG_RANDOM_ADDRESS + 0);
}

void DALIC_Query_Reg_Random_Address1(void)
{
  DALIC_Query_Reg_(DALIREG_RANDOM_ADDRESS + 1);
}

void DALIC_Query_Reg_Random_Address2(void)
{
  DALIC_Query_Reg_(DALIREG_RANDOM_ADDRESS + 2);
}

void DALIC_Enable_Write_Memory(void)
{
  write_enable_membanks = 1;
}

void DALIC_Read_Memory_Location(void)
{
  u8 mem_data;

  if((dtr1 < MEM_BANKS_CNT) && (dtr <= membanks[dtr1][0]))
  {
    mem_data = membanks[dtr1][dtr];
    dtr++;
    if (dtr <= membanks[dtr1][0])
      dtr2 = membanks[dtr1][dtr];
    Send_DALI_Frame(mem_data);
  }
}

void DALIC_Write_Memory_Location(u8 mem_data)
{
  if(
     (write_enable_membanks)     && // check global write protection
       (dtr1 <= membanks[0][2])    && // check dtr1 to membanks count
         (dtr1 > 0)                  && // check dtr1 to membanks min (bank 0 is read only)
           (dtr  <= membanks[dtr1][0]) && // check dtr to max address
             (dtr  >= 2)                 && // check dtr to min address
               ((dtr==2) || (membanks[dtr1][2] == 0x55) || ((dtr==0x0F)&&(dtr1==1)))
                 // check if memory bank is write protected (except protection byte - index "2" and index 0x0F in bank 1)
                 )
  {
    membanks[dtr1][dtr] = mem_data;    //write to memory bank "dtr1" at address "dtr"
    membanks[dtr1][1]   = DALIC_Mem_Checksum(dtr1);  //update memory bank checksum
    if (dtr == membanks[dtr1][0])
      write_enable_membanks = 0;
    dtr++;
    Send_DALI_Frame(mem_data);
  }
}

u8 DALIC_Mem_Checksum(u8 mem_number)
{
  u8 i;
  u8 checksum;

  checksum = 0;
  for (i = 2; i <= membanks[mem_number][0]; i++)
    checksum -= membanks[mem_number][i];
  return checksum;
}

/**
* @}
*/



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/