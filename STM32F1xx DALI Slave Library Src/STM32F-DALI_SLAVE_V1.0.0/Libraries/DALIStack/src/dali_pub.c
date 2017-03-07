/**
  ******************************************************************************
  * @file    dali_pub.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali Public functions (need to understand DALI specification)
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
#include "dali_pub.h"
#include "dali_regs.h"
#include "lite_timer_8bit.h"
#include "eeprom.h"
#include "dali_config.h"
#include "dali.h"

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u32 DALIP_iChangeEvery;
u32 DALIP_iChangeCountdown;
u8 DALIP_bIncrease;
u8 DALIP_bOff_AfterFade;
u8 DALIP_iMaxLevel,DALIP_iMinLevel;
u8 DALIP_DTR;
u8 DALIP_FadeGoal;
volatile u8 Physically_Selected;
u8 DALIP_bEnable_DAPC;
/**************************************************************************
* These are the two Regs which are read only and therefore stored in ROM *
* The first is                               *
* - Version number   and the second          *
* - Physical minimum Level                   *
* You might want to adjust these.            *
**************************************************************************/
const u8 ROMRegs[2]={DALI_VERSION_NUMBER_ROM,PHYSICAL_MIN_LEVEL_ROM}; // ALAL old /* {Version Number, Phys. Min Level} */

/* Extern variables ----------------------------------------------------------*/
//definitions from dali_config.c
extern const u32 DALIP_FadeTimeTable[];
extern const u16 DALIP_FadeRateTable[];

#ifdef USE_ARC_TABLE
extern const u16 DALIP_ArcTable[];
#define DALIP_ConvertARC(a) DALIP_ArcTable[a]
#else
#define DALIP_ConvertARC(a) a
#endif

/* Private function prototypes -----------------------------------------------*/
void DALIP_HW_LIGHT_Set(u16 newval);
//callback function type for light control
TLightControlCallback *LightControlCallback = DALIP_HW_LIGHT_Set;

/**
* @brief  Initialize HW dependent light control callback
* @param  LightControlFunction
* @retval None
*/
void DALIP_Init(TDPLightControlCallback LightControlFunction)
{
  DALIP_bEnable_DAPC = 0;
  //callback function init
  if (LightControlFunction)
    LightControlCallback = LightControlFunction;
  else
    LightControlCallback = DALIP_HW_LIGHT_Set;
}

/**
* @brief  Set new light value - HW dependent
* @param  value to be written
* @retval None
*/
void DALIP_HW_LIGHT_Set(u16 newval)
{
  return;
}


void DALIP_LaunchTimer(u8 p)
{
  RTC_LaunchUserTimer(p);
}

void DALIP_DoneTimer(void){
  RTC_DoneUserTimer();
}

/***********************************************************
* This is a ms-Timer, meaning that the function           *
* DALIP_TimerCallback is called every 1ms precisely by    *
* Timer A.                        *
* You have to activate this Timer by              *
*   DALIP_LaunchTimer();                                  *
* and you should turn it off by using             *
*   DALIP_DoneTimer();                    *
*                                                         *
* NOTE: The Controller is programmed to go to         *
*       SLOW-WAIT-MODE (in order to save power) if it has *
*       nothing more to do. It can only go to this mode   *
*       when the user-timer is deactivated, so you should *
*       call DALIP_DoneTimer() as soon as you're finished *
***********************************************************/
void DALIP_TimerCallback(void)
{
  u8 zw;

  if (DALIP_iChangeCountdown)
  {
    DALIP_iChangeCountdown--;
  }
  else
  {
    DALIP_iChangeCountdown = DALIP_iChangeEvery;
    zw = DALIP_GetArc();
    if (DALIP_bIncrease)
    {
      if (zw < DALIP_iMaxLevel)
      {
        zw++;
        DALIP_SetArc(zw);
      }
      else
      {
        DALIP_DoneTimer();
        DALIP_SetFadeReadyFlag(0); /* fade is ready */
      }
    }
    else
    {
      if (zw > DALIP_iMinLevel)
      {
        zw--;
        DALIP_SetArc(zw);
      }
      else
      {
        DALIP_DoneTimer();
        DALIP_SetFadeReadyFlag(0); /* fade is ready */
        if (DALIP_bOff_AfterFade)
        {
          DALIP_bOff_AfterFade = 0;
          DALIR_WriteStatusBit(DALIREG_STATUS_POWER_FAILURE,0);
          DALIR_WriteStatusBit(DALIREG_STATUS_LAMP_ARC_POWER_ON,0);
          DALIP_Off();
          return;
        }
      }
    }
    LightControlCallback(DALIP_ConvertARC(zw));
    if (zw == DALIP_FadeGoal)
    {
      DALIP_DoneTimer();
      DALIP_SetFadeReadyFlag(0); /* fade is ready */
    }
  }
}

/*******************************************
* DALI-Reg-Write-Functions                *
*******************************************/

void DALIP_SetArc(u8 val)
{
  DALIR_WriteReg(DALIREG_ACTUAL_DIM_LEVEL,val);
  PowerOnTimerReset();
}

void DALIP_SetBallastStatusFlag(u8 bit_nbr)
{
  DALIR_WriteStatusBit(0,bit_nbr);
}

void DALIP_SetLampFailureFlag(u8 bit_nbr)
{
  DALIR_WriteStatusBit(1,bit_nbr);
  if(bit_nbr)
  {
    DALIP_SetLampPowerOnFlag(0);
  }
  else
  {
    if(DALIP_GetArc())
      DALIP_SetLampPowerOnFlag(1);
  }
}

void DALIP_SetLampPowerOnFlag(u8 bit_nbr)
{
  DALIR_WriteStatusBit(2,bit_nbr);
}

void DALIP_SetFadeReadyFlag(u8 bit_nbr)
{
  DALIR_WriteStatusBit(4,bit_nbr);
}

void DALIP_SetPowerFailureFlag(u8 bit_nbr)
{
  DALIR_WriteStatusBit(7,bit_nbr);
}

/*******************************************
* DALI-Reg-Read-Functions                 *
*******************************************/

u8 DALIP_GetArc(void)
{
  return DALIR_ReadReg(DALIREG_ACTUAL_DIM_LEVEL);
}

u8 DALIP_GetFadeTime(void)
{
  return DALIR_ReadReg(DALIREG_FADE_TIME);
}

u8 DALIP_GetFadeRate(void)
{
  return DALIR_ReadReg(DALIREG_FADE_RATE);
}

u8 DALIP_GetMaxLevel(void)
{
  return DALIR_ReadReg(DALIREG_MAX_LEVEL);
}

u8 DALIP_GetMinLevel(void)
{
  return DALIR_ReadReg(DALIREG_MIN_LEVEL);
}

u8 DALIP_GetPowerOnLevel(void)
{
  return DALIR_ReadReg(DALIREG_POWER_ON_LEVEL);
}

u8 DALIP_GetSysFailureLevel(void)
{
  return DALIR_ReadReg(DALIREG_SYSTEM_FAILURE_LEVEL);
}

u8 DALIP_GetStatus(void)
{
  return DALIR_ReadReg(DALIREG_STATUS_INFORMATION);
}

u8 DALIP_GetVersion(void)
{
  return DALIR_ReadReg(DALIREG_VERSION_NUMBER);
}

u8 DALIP_GetPhysMinLevel(void)
{
  return DALIR_ReadReg(DALIREG_PHYS_MIN_LEVEL);
}

/*******************************************
* E²PROM Access Functions                 *
*******************************************/

u8 DALIP_EEPROM_Size(void)
{
  u16 zwms;
  zwms = E2_PHYSICAL_SIZE;
  zwms -= DALIREG_EEPROM_END-DALIREG_EEPROM_START+4;
  return (u8) zwms;
}

u8 DALIP_Read_E2(u8 addr)
{   u16 data;
if (addr >= DALIP_EEPROM_Size()) return 0;
E2_ReadMem((u16)eeprom_variable[(addr+DALIREG_EEPROM_END-DALIREG_EEPROM_START)],&data);
return (u8)data;
}

void DALIP_Write_E2(u8 addr, u8 data_val)
{
  if (addr >= DALIP_EEPROM_Size()) return;
  E2_WriteMem((u16)eeprom_variable[(addr+DALIREG_EEPROM_END-DALIREG_EEPROM_START)],(u16)data_val);
}

void DALIP_Write_E2_Buffer(u8 addr, u8 number, u8*buf)
{
  u8 zw;

  zw = DALIP_EEPROM_Size();
  if (addr >= zw) return;
  zw -= addr;
  if (number > zw) return;
  E2_WriteBurst((u16)addr,(u16)number,(u16*)buf);
}


/*******************************************
* Reserved-Functions                       *
*******************************************/

void DALIP_Reserved_Function(u8 cmd)
{

}

void DALIP_Reserved_Special_Function(u8 cmd, u8 data_val)
{

}

/*********************************************************************
* Note:-The Flags
*       "Status of Ballast",
*   "Lamp Failure",
*       "Lamp arc power on",
*       "Fade Ready" and the
*       "Arc Power Level" Reg
*       have to be set by the Ballast-code!
*       see dali_pub.h, "Write-Functions".
*
*      -All routines have to return AS QUICKLY AS POSSIBLE!
*       The Slave is not able to process DALI-Commands while in a
*       User-Function. So, for instance, if there's a fade-command,
*       use the Timer-Interrupt rather that a waiting-loop.
*       (Don't forget to set the "Fade Ready Flag" to false while
*       fading).
*********************************************************************/

void DALIP_Direct_Arc(u8 val)
{
  u8 iActVal;
  u8 iActFT;
  u32 FadeTime;

  iActVal = DALIP_GetArc();
  if (iActVal == val) return;
  iActFT = DALIP_GetFadeTime();

  DALIP_DoneTimer();
  if ((iActFT == 0)&&(!DALIP_bEnable_DAPC))
  {
    DALIP_SetArc(val);
    LightControlCallback(DALIP_ConvertARC(val));
  }
  else
  {
    if(DALIP_bEnable_DAPC)
      FadeTime = 200;
    else
      FadeTime = DALIP_FadeTimeTable[iActFT];
    DALIP_bOff_AfterFade = 0;
    if (iActVal > val)
    {
      DALIP_iMinLevel = DALIP_GetMinLevel();
      DALIP_bIncrease = 0;
      if(val)
      {
        DALIP_iChangeEvery = (FadeTime/((long)(iActVal - val)))-1;
      }
      else
      {
        DALIP_iChangeEvery = (FadeTime/((long)(iActVal - DALIP_iMinLevel)))-1;
        DALIP_bOff_AfterFade = 1;
      }
    }
    else
    {
      DALIP_iMaxLevel = DALIP_GetMaxLevel();
      DALIP_bIncrease = 1;
      DALIP_iChangeEvery = (FadeTime/((long)(val - iActVal)))-1;
    }
    DALIP_iChangeCountdown = DALIP_iChangeEvery;
    DALIP_SetFadeReadyFlag(1);                       /* Fade running from now */
    DALIP_FadeGoal = val;
    DALIP_LaunchTimer(0xFF);
  }
}

void DALIP_Direct_Arc_NoFade(u8 val)
{
  if (DALIP_GetArc() == val) return;
  DALIP_DoneTimer();
  DALIP_SetArc(val);
  LightControlCallback(DALIP_ConvertARC(val));
}

void DALIP_Off(void)
{
  DALIP_DoneTimer();
  DALIP_SetArc(0);
  LightControlCallback(0);
}

void DALIP_Up(void)
{
  u8 zw;

  DALIP_DoneTimer();
  if (DALIR_ReadStatusBit(DALIREG_STATUS_FADE_READY)==0)
  {
    zw = DALIP_GetFadeRate();
    if (zw == 0)
    {
      zw = DALIP_GetArc();
      zw++;
      DALIP_SetArc(zw);
      LightControlCallback(DALIP_ConvertARC(zw));
    }
    else
    {
      DALIP_iMaxLevel = DALIP_GetMaxLevel();
      DALIP_bIncrease = 1;
      DALIP_iChangeEvery = DALIP_FadeRateTable[zw]-1;
      DALIP_iChangeCountdown = DALIP_iChangeEvery;
      DALIP_SetFadeReadyFlag(1);      /* Fade running from now */
      DALIP_FadeGoal = 255;
      DALIP_LaunchTimer(200);
    }
  }
  else
  {
    DALIP_LaunchTimer(200);
  }
}

void DALIP_Down(void)
{
  u8 zw;

  DALIP_DoneTimer();
  if (DALIR_ReadStatusBit(DALIREG_STATUS_FADE_READY)==0)
  {
    zw = DALIP_GetFadeRate();
    if (zw == 0)
    {
      zw = DALIP_GetArc();
      zw--;
      DALIP_SetArc(zw);
      LightControlCallback(DALIP_ConvertARC(zw));
    }
    else
    {
      DALIP_bIncrease = 0;
      DALIP_iMinLevel = DALIP_GetMinLevel();
      DALIP_iChangeEvery = DALIP_FadeRateTable[zw]-1;
      DALIP_iChangeCountdown = DALIP_iChangeEvery;
      DALIP_SetFadeReadyFlag(1);      /* Fade running from now */
      DALIP_FadeGoal = 255;
      DALIP_LaunchTimer(200);
    }
  }
  else
  {
    DALIP_LaunchTimer(200);
  }
}

void DALIP_Enable_DAPC_Sequence(void)
{
  DALIP_bEnable_DAPC = 1;
  RTC_LaunchDAPCTimer();
}

void DALIP_Stop_DAPC_Sequence(void)
{
  RTC_DoneDAPCTimer();
  DALIP_bEnable_DAPC = 0;
}

void DALIP_Step_Up(void)
{
  u8 zw;

  DALIP_DoneTimer();
  zw = DALIP_GetArc();
  if(zw)
    zw++;
  else
    zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  DALIP_SetArc(zw);
  LightControlCallback(DALIP_ConvertARC(zw));
}

void DALIP_Step_Down(void)
{
  u8 zw;

  DALIP_DoneTimer();
  zw = DALIP_GetArc();
  zw--;
  DALIP_SetArc(zw);
  LightControlCallback(DALIP_ConvertARC(zw));
}

void DALIP_Recall_Max_Level(void)
{
  u8 zw;

  DALIP_DoneTimer();
  zw = DALIR_ReadReg(DALIREG_MAX_LEVEL);
  DALIP_SetArc(zw);
  LightControlCallback(DALIP_ConvertARC(zw));
}

void DALIP_Recall_Min_Level(void)
{
  u8 zw;

  DALIP_DoneTimer();
  zw = DALIR_ReadReg(DALIREG_MIN_LEVEL);
  DALIP_SetArc(zw);
  LightControlCallback(DALIP_ConvertARC(zw));
}

void DALIP_Step_Down_And_Off(void)
{
  DALIP_Step_Down();
}

void DALIP_On_And_Step_Up(void)
{
  DALIP_Step_Up();
}

/* is THIS device of the specified device_type? (0 = NO) */
//u8 DALIP_Is_Device_Type(u8 device_type){      //ALAL function useless (DALIP_What_Device_Type can be used instead)
//  return (device_type==0);            //ALAL 0 => fluorescent lamp
//}

/* 1 for yes */
u8 DALIP_Is_Physically_Selected(void)
{
  return Physically_Selected; //global variable set by user
}

/* return the device type (see spec.) */
u8 DALIP_What_Device_Type(void)
{
  return DEVICE_TYPE;              // globally defined
}

/********************************************************************************
* Extended Commands -----------------------------------------------------------*
* -----------------------------------------------------------------------------*
*                                                                              *
* Note:-These Commands are specific for certain device types. If you don't need*
* extended commands, just leave those functions returning 0, they will not be  *
* called anyway, if you don't support them. (Supposed, the Master-Software     *
* works correctly.) See special appendixes for the distinct device types.      *
*-the passed parameter "command" is the second byte of the received DALI-frame *
* (the first byte contains only the short-address, according to DALI-Spec.)    *
*******************************************************************************/

/****************************************************************************
* should return !=0 if this extended command (param 1) Requires an answer
* to be sent to the master.
* If this function returns 0, the return-value of DALIP_Extended_Command
* will be ignored.
****************************************************************************/
u8 DALIP_Ext_Cmd_Is_Answer_Required(u8 command)
{
  return 0;
}

/****************************************************************************
* should return !=0 if this command's answer is only yes/no.
* in that case, the return value of DALIP_Extended_Command will be
* interpreted as boolean, so, if DALIP_Extended_Command returns 0, there will
* be the timeout-answer to the master
****************************************************************************/
u8 DALIP_Ext_Cmd_Is_Answer_YesNo(u8 command)
{
  return 0;
}

/****************************************************************************
* an extended command (param 1) has been called.
* return-value will be interpreted as explained above.
****************************************************************************/
u8 DALIP_Extended_Command(u8 command)
{
  return 0;
}

/****************************************************************************
* return-value will be Extended version number
****************************************************************************/
u8 DALIP_Extended_Version_Number(void)
{
  return 0;
}

/**
* @}
*/





/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/