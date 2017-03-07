/**
  ******************************************************************************
  * @file    dali_pub.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    07-Dec-2012
  * @brief   Dali Public functions (need to understand DALI spec) - header
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

#ifndef DALI_PUB_H
#define DALI_PUB_H

//callback function type for light control
typedef void TDPLightControlCallback(u16 lighvalue);

/* Public Functions */
void DALIP_Init(TDPLightControlCallback LightControlFunction);



/*************************************************************
 * User Timer Functions------------------------------------- *
 * --------------------------------------------------------- *
 *************************************************************/
/***********************************************
 * DALIP_LaunchTimer				           *
 * -User ms-Timer.							   *
 * Param1: Number of times, the TimerCallback  *
 *         will be called. (e.g. pass 200 for  *
 *         a 200ms fade).					   *
 *         0xFF = infinite (runs until 		   *
 *                       DoneTimer is called)  *
 *         									   *
 ***********************************************/
void DALIP_LaunchTimer(u8);
void DALIP_DoneTimer(void);
void DALIP_TimerCallback(void);

/*************************************************************
 * DALI-Register Access Functions--------------------------- *
 * --------------------------------------------------------- *
 * use these Functions to write/read the specific DALI-Regs  *
 * Note: For Flag-Registers, pass 0 to clear the Bit and !=0 *
 *       to set it.											 *
 *************************************************************/
/*******************************************
 * DALI-Reg-Write-Functions                *
 *******************************************/
void DALIP_SetArc(u8);
void DALIP_SetBallastStatusFlag(u8);
void DALIP_SetLampFailureFlag(u8);
void DALIP_SetLampPowerOnFlag(u8);
void DALIP_SetFadeReadyFlag(u8);

/*******************************************
 * DALI-Reg-Read-Functions                 *
 *******************************************/
u8 DALIP_GetArc(void);
u8 DALIP_GetFadeTime(void);
u8 DALIP_GetFadeRate(void);
u8 DALIP_GetMaxLevel(void);
u8 DALIP_GetMinLevel(void);
u8 DALIP_GetPowerOnLevel(void);
u8 DALIP_GetSysFailureLevel(void);
u8 DALIP_GetStatus(void);
u8 DALIP_GetVersion(void);
u8 DALIP_GetPhysMinLevel(void);

/*************************************************************
 * E²PROM Access Functions---------------------------------- *
 * --------------------------------------------------------- *
 * using these functions, you can read and write to the      *
 * connected E²PROM. The addressing range is from 0 to the   *
 * return-value of DALIP_EEPROM_Size. Accesses outside that  *
 * range will be ignored.                                    *
 * Note: The returned E²PROM-Size is the actual size minus   *
 *       a few Bytes that are used for saving the			 *
 *       DALI-Registers. There is maximum size of 256 bytes  *
 *       for the connected E²PROM. (Bigger ones work too,	 *
 *       but only the lower 256 bytes can be accessed)       *
 * IMPORTANT: If a Page-Write exceeds the addressing range,  *
 *            the WHOLE Write Operation will be ignored!	 *
 *************************************************************/

/***********************************************
 * DALIP_EEPROM_Size						   *
 * Return: Highest address that can be passed  *
 *         to an E²PROM-Access-Command         *
 ***********************************************/
u8 DALIP_EEPROM_Size(void);

/***********************************************
 * DALIP_Read_E2                               *
 * -Reads one byte from the passed address     *
 * Param1: Address to be read                  *
 * Return: Databyte read from the E²PROM       *
 ***********************************************/
u8 DALIP_Read_E2(u8);

/***********************************************
 * DALIP_Write_E2                              *
 * -Writes one byte to the passed address      *
 * Param1: Address to write to                 *
 * Param2: Databyte to be written              *
 ***********************************************/
void DALIP_Write_E2(u8, u8);

/***********************************************
 * DALIP_Write_E2_Buffer					   *
 * -Writes a sequence of Bytes (uses the	   *
 *  page-write-operation of the E². -> faster) *
 * Param1: First Address to write to           *
 * Param2: Number of Bytes to be written       *
 * Param3: Pointer to the first byte of the    *
 *         array that containes the data	   *
 ***********************************************/
void DALIP_Write_E2_Buffer(u8, u8, u8*);

/*******************************************
 * Special-Functions                         *
 *******************************************/
void DALIP_Reserved_Function(u8);
void DALIP_Reserved_Special_Function(u8, u8);



void DALIP_Direct_Arc(u8);
void DALIP_Direct_Arc_NoFade(u8);
void DALIP_Off(void);
void DALIP_Up(void);
void DALIP_Down(void);
void DALIP_Step_Up(void);
void DALIP_Step_Down(void);
void DALIP_Recall_Max_Level(void);
void DALIP_Recall_Min_Level(void);
void DALIP_Step_Down_And_Off(void);
void DALIP_On_And_Step_Up(void);
void DALIP_Enable_DAPC_Sequence(void);
void DALIP_Stop_DAPC_Sequence(void);

u8 DALIP_What_Device_Type(void);
u8 DALIP_Is_Physically_Selected(void);
u8 DALIP_Is_Device_Type(u8);

/* Extended Commands */

/****************************************************************************
 * should return !=0 if this command (param 1) Requires an answer to be sent
 * to the master.
 ****************************************************************************/
u8 DALIP_Ext_Cmd_Is_Answer_Required(u8);

/****************************************************************************
 * should return !=0 if this command's answer is only yes/no.
 * in that case, the return value of DALIP_Extended_Command will be
 * interpreted as boolean, so, if DALIP_Extended_Command returns 0, there will
 * be the timeout-answer to the master
 ****************************************************************************/
u8 DALIP_Ext_Cmd_Is_Answer_YesNo(u8);

/****************************************************************************
 * an extended command (param 1) has been called.
 * return-value will be interpretes as read above.
 ****************************************************************************/
u8 DALIP_Extended_Command(u8);
u8 DALIP_Extended_Version_Number(void);


#endif



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
