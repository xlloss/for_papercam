//==============================================================================
//
//  File        : mmpf_pwm.c
//  Description : PWM control driver
//  Author      : Ben Lu
//  Revision    : 1.0
//
//==============================================================================
/**
 *  @file mmpf_pwm.c
 *  @brief PWM control driver
 *  @author Ben Lu
 *  @version 1.0
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
 
#include "config_fw.h"
#include "mmpf_typedef.h"
#include "lib_retina.h"
#include "mmp_err.h"
#include "os_wrap.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_gpio.h"
#include "mmp_reg_vif.h"
#include "mmpf_pio.h"
#include "mmpf_pwm.h"
#include "mmpf_system.h"
#include "mmpf_pll.h"

#if SUPPORT_PWM
#define NO_ISR  (1)

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

static PwmCallBackFunc *m_PwmCallBack[PWM_MAX_NUM];
static MMPF_OS_SEMID  	m_PWMSemID[PWM_MAX_NUM];

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_ISR
//  Description : 
//------------------------------------------------------------------------------
/** @brief PWM interrupt handler function.

PWM interrupt handler function.
@return none
*/
#if NO_ISR==0
void MMPF_PWM_ISR(void)
{
    AITPS_PWMB  pPWM = (AITPS_PWMB)AITC_BASE_PWM;
    MMP_ULONG   i = 0;
    MMP_ULONG   status = 0x0;
    
    //ISR of PWM
   	for (i = 0; i < PWM_MAX_NUM_P0; i++) {
	   	if (pPWM->PWM0[i].PWM_INT_CPU_SR != 0 ) {  
	   	 	status = pPWM->PWM0[i].PWM_INT_CPU_SR;
		   	if (m_PwmCallBack[i] != NULL) {
		   		(*m_PwmCallBack[i])(status);
		   	}
		   	pPWM->PWM0[i].PWM_INT_CPU_SR |= status; //Clean status
		   	break;
	   	}
   	} 
   	
   	for (i = 0; i < PWM_MAX_NUM_P1; i++) {
	   	if (pPWM->PWM1[i].PWM_INT_CPU_SR != 0 ) {  
	   	 	status = pPWM->PWM1[i].PWM_INT_CPU_SR;
		   	if (m_PwmCallBack[i + PWM_MAX_NUM_P0] != NULL) {
		   		(*m_PwmCallBack[i + PWM_MAX_NUM_P0])(status);
		   	}
		   	pPWM->PWM1[i].PWM_INT_CPU_SR |= status; //Clean status
		   	break;
	   	}
   	}
}
#endif

////----------------------------------------------------------------------------
//  Function    : GetpPWM
//  Description : Return the AITPS_PWM of the given id
//  Note        : Because we have 18 PWM engines.
//  Return      :
//------------------------------------------------------------------------------
static AITPS_PWM GetpPWM(MMP_UBYTE id)
{
    AITPS_PWMB  pPWMB = (AITPS_PWMB)AITC_BASE_PWM;

    if (id >= PWM_MAX_NUM) {
	    RTNA_DBG_Str(0, "ERR: Invalid PWM ID\r\n");
	    return NULL;
	}

    if (id < PWM_MAX_NUM_P0)
        return &pPWMB->PWM0[id];
    else
        return &pPWMB->PWM1[id - PWM_MAX_NUM_P0];
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_Initialize
//  Description : 
//------------------------------------------------------------------------------
/** @brief Driver init

Driver init
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_Initialize(void)
{
	MMP_UBYTE i = 0;
	#if NO_ISR==0
	AITPS_AIC pAIC = AITC_BASE_AIC;
	#endif
	static MMP_BOOL ubInitFlag = MMP_FALSE;

	if (ubInitFlag == MMP_FALSE) 
	{
	    #if NO_ISR==0
      RTNA_AIC_Open(pAIC, AIC_SRC_PWM, pwm_isr_a,
                  AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 7);
      RTNA_AIC_IRQ_En(pAIC, AIC_SRC_PWM);
      #endif
      
	    for(i = 0; i < PWM_MAX_NUM; i++) {
	    	m_PwmCallBack[i] = NULL;
	    	m_PWMSemID[i] = MMPF_OS_CreateSem(1);
	    }
	    ubInitFlag = MMP_TRUE;
    }
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_SetAttribe
//  Description : 
//------------------------------------------------------------------------------
/** @brief Driver init
Parameters:
@param[in] attr : PWM attribute, please refer the structure MMP_PWM_ATTR
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_SetAttribe(MMP_PWM_ATTR *attr)
{
    AITPS_PWM   pPWM = GetpPWM(attr->ubID);
    MMP_UBYTE   ret = 0xFF;

    if (pPWM == NULL)
        return MMP_PWM_ERR_PARAMETER;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_PWM, MMP_TRUE);

    ret = MMPF_OS_AcquireSem(m_PWMSemID[attr->ubID], PWM_SEM_TIMEOUT);

    if (attr->uPulseID == MMP_PWM_PULSE_ID_A) {
        pPWM->PWM_PULSE_A_T0 = attr->ulClkDuty_T0;
        pPWM->PWM_PULSE_A_T1 = attr->ulClkDuty_T1;
        pPWM->PWM_PULSE_A_T2 = attr->ulClkDuty_T2;
        pPWM->PWM_PULSE_A_T3 = attr->ulClkDuty_T3;
        pPWM->PWM_PULSE_A_PEROID = attr->ulClkDuty_Peroid;
        pPWM->PWM_PULSE_A_NUM = attr->ubNumOfPulses;
    }
    else if (attr->uPulseID == MMP_PWM_PULSE_ID_B) {
        pPWM->PWM_PULSE_B_T0 = attr->ulClkDuty_T0;
        pPWM->PWM_PULSE_B_T1 = attr->ulClkDuty_T1;
        pPWM->PWM_PULSE_B_T2 = attr->ulClkDuty_T2;
        pPWM->PWM_PULSE_B_T3 = attr->ulClkDuty_T3;
        pPWM->PWM_PULSE_B_PEROID = attr->ulClkDuty_Peroid;
        pPWM->PWM_PULSE_B_NUM = attr->ubNumOfPulses;
    }

    if (ret == 0) {
        MMPF_OS_ReleaseSem(m_PWMSemID[attr->ubID]);
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_EnableInterrupt
//  Description : 
//------------------------------------------------------------------------------
/** @brief Driver init
Parameters:
@param[in] uID : PWM ID
@param[in] bEnable : enable/disable interrupt
@param[in] cb : call back function when interrupt happens.
@param[in] intr : PWM interupt type.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_EnableInterrupt(MMP_UBYTE ubID, MMP_BOOL bEnable, PwmCallBackFunc *cb, MMP_PWM_INT intr)
{
	AITPS_PWM   pPWM = GetpPWM(ubID);
    MMP_UBYTE 	ret = 0xFF;

	if (pPWM == NULL)
        return MMP_PWM_ERR_PARAMETER;

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_PWM, MMP_TRUE);

	ret = MMPF_OS_AcquireSem(m_PWMSemID[ubID], PWM_SEM_TIMEOUT);

	if (bEnable == MMP_TRUE) {
		m_PwmCallBack[ubID] = cb;
		pPWM->PWM_INT_CPU_EN |= (0x1 << intr);
	}
	else {
		pPWM->PWM_INT_CPU_EN &= ~(0x1 << intr);
		m_PwmCallBack[ubID] = NULL;
	}

	if (ret == 0) {
		MMPF_OS_ReleaseSem(m_PWMSemID[ubID]);
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_ControlSet
//  Description : Set PWM control attribute
//------------------------------------------------------------------------------
/** @brief Set PWM control attribute
Parameters:
@param[in] ubID : PWM ID
@param[in] control : control attribute               
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_ControlSet(MMP_UBYTE ubID, MMP_UBYTE control)
{
    AITPS_PWM   pPWM = GetpPWM(ubID);
    MMP_UBYTE 	ret = 0xFF;

	if (pPWM == NULL)
        return MMP_PWM_ERR_PARAMETER;

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_PWM, MMP_TRUE);

	ret = MMPF_OS_AcquireSem(m_PWMSemID[ubID], PWM_SEM_TIMEOUT);

    pPWM->PWM_CTL = control;

	if (ret == 0) {
		MMPF_OS_ReleaseSem(m_PWMSemID[ubID]);
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_EnableOutputPin
//  Description : 
//------------------------------------------------------------------------------
/** @brief enable/disable the PWM single output pin

enable/disable the PWM single output pin
@param[in] pwm_pin : PWM I/O pin selection, please refer MMP_PWM_PIN
@param[in] bEnable : enable/disable the specific PWM pin
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_EnableOutputPin(MMP_PWM_PIN pwm_pin, MMP_BOOL bEnable)
{
	AITPS_GBL pGBL = AITC_BASE_GBL;
	
	if (bEnable) 
	{
        if (pwm_pin < MMP_PWM_PIN_MAX) {
            pGBL->GBL_PWM_IO_CFG |= (0x1 << PWN_GET_PIN_OFST(pwm_pin));
        }
        else {
            RTNA_DBG_Str(0, "Unsupport PWM IO\r\n");
        }
	}
	else {
	
        if (pwm_pin < MMP_PWM_PIN_MAX) {
            pGBL->GBL_PWM_IO_CFG &= ~(0x1 << PWN_GET_PIN_OFST(pwm_pin));
        }
        else {
            RTNA_DBG_Str(0, "Unsupport PWM IO\r\n");
        }
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PWM_OutputPulse
//  Description : 
//------------------------------------------------------------------------------
/** @brief Simplely output some pulses (According to the parameters)

Simplely output some pulses (According to the parameters)
@param[in] pwm_pin      : PWM I/O pin selection, please refer MMP_PWM_PIN
@param[in] bEnableIoPin : Enable/disable the specific PWM pin
@param[in] ulFreq       : The pulse frequency in unit of Hz.
@param[in] bHigh2Low    : MMP_TRUE: High to Low pulse, MMP_FALSE: Low to High pulse
@param[in] bEnableIntr  : enable interrupt or not
@param[in] cb           : Call back function when interrupt occurs
@param[in] ulNumOfPulse : Number of pulse, 0 stand for using PWM auto mode to generate infinite pulse.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PWM_OutputPulse(MMP_PWM_PIN pwm_pin, MMP_BOOL bEnableIoPin, 
                             MMP_ULONG ulFreq, MMP_ULONG duty,
                             MMP_BOOL bHigh2Low, MMP_BOOL bEnableIntr,
                             PwmCallBackFunc *cb, MMP_ULONG ulNumOfPulse)
{
    MMP_PWM_ATTR attr;
    MMP_ULONG ulHighTicks, ulLowTicks;
    MMP_ULONG ulTickPerPluse = 0;
    MMP_ULONG ulPwmClkFreq = 0;

    if ((ulFreq == 0x0) && bEnableIoPin) {
        RTNA_DBG_Str(0, "Error: PWM invalid speed\r\n");
        return MMP_PWM_ERR_PARAMETER;
    }
	if (ulNumOfPulse > 0x1FE) {
		RTNA_DBG_Str(0, "Error: PWM pulse number overflow\r\n");
		return MMP_PWM_ERR_PARAMETER;
	}
	if (pwm_pin >= MMP_PWM_PIN_MAX) {
		RTNA_DBG_Str(0, "Error: Un-support PWM pin\r\n");
		return MMP_PWM_ERR_PARAMETER;
	}

    attr.ubID = PWM_GET_ID(pwm_pin);

	if (bEnableIoPin == MMP_TRUE)
	{
        /* PWM clock source is G0_Slow */
        MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulPwmClkFreq);
        ulPwmClkFreq = ulPwmClkFreq * 500; //in unit of HZ

        ulTickPerPluse = ulPwmClkFreq / ulFreq;
        if (ulTickPerPluse <= 1) {  // Max can output half of PWM clock freq.
    		RTNA_DBG_Str(0, "Error: PWM exceed max supported speed\r\n");
    		return MMP_PWM_ERR_PARAMETER;
    	}

		MMPF_PWM_EnableOutputPin(pwm_pin, MMP_TRUE);

        if (duty > 100) {
            duty = 100;
        }

        ulHighTicks = (ulTickPerPluse * duty) / 100;
        ulLowTicks  = ulTickPerPluse - ulHighTicks;

        attr.uPulseID = MMP_PWM_PULSE_ID_A;
        if (bHigh2Low) {
            attr.ulClkDuty_T0 = ulHighTicks;
    		attr.ulClkDuty_T1 = ulTickPerPluse;
    		attr.ulClkDuty_T2 = ulTickPerPluse;
    		attr.ulClkDuty_T3 = ulTickPerPluse;
        }
        else {
            attr.ulClkDuty_T0 = 0;
    		attr.ulClkDuty_T1 = 0;
    		attr.ulClkDuty_T2 = 0;
    		attr.ulClkDuty_T3 = ulLowTicks;
        }
        attr.ulClkDuty_Peroid = ulTickPerPluse;

        if (ulNumOfPulse == 0) {
            attr.ubNumOfPulses = 1;
        }
        else {
            if (ulNumOfPulse > 0xFF)
                attr.ubNumOfPulses = 0xFF;
            else
                attr.ubNumOfPulses = ulNumOfPulse;
        }

        MMPF_PWM_SetAttribe(&attr);

        /* Use pulse B for extra period */
        attr.uPulseID = MMP_PWM_PULSE_ID_B;
        if (ulNumOfPulse > 0xFF)
            attr.ubNumOfPulses = ulNumOfPulse - 0xFF;
        else
            attr.ubNumOfPulses = 0;
        MMPF_PWM_SetAttribe(&attr);

        if (bEnableIntr == MMP_TRUE)
            MMPF_PWM_EnableInterrupt(attr.ubID, MMP_TRUE, cb, MMP_PWM_INT_ONE_ROUND);

        if (ulNumOfPulse != 0) {
            MMPF_PWM_ControlSet(attr.ubID, (PWM_PULSE_A_FIRST |
                                        PWM_ONE_ROUND | PWM_PULSE_B_NEG |
                                        PWM_PULSE_A_NEG | PWM_EN));
        }
        else { //For PWM auto mode
            MMPF_PWM_ControlSet(attr.ubID, (PWM_PULSE_A_FIRST |
                                        PWM_AUTO_CYC | PWM_PULSE_B_NEG |
                                        PWM_PULSE_A_NEG | PWM_EN));
        }
	}
	else {
        MMPF_PWM_ControlSet(attr.ubID, 0);
		MMPF_PWM_EnableOutputPin(pwm_pin, MMP_FALSE);
	}
	return MMP_ERR_NONE;
}

MMP_ERR MMPF_PWM_SetFreqDuty(MMP_PWM_PIN pwm_pin,MMP_ULONG pdFreqHz, MMP_UBYTE pdDuty)
{

	MMP_PWM_ATTR pwm_attribute = {0,                   // ubID
                                        MMP_PWM_PULSE_ID_A, // uPulseID
                                        0x1,                 // ulClkDuty_T0
                                        0x2,                 // ulClkDuty_T1
                                        0x2,                 // ulClkDuty_T2
                                        0x2,                 // ulClkDuty_T3
                                        0x1,                 // ulClkDuty_Peroid
                                        0x1                  // ubNumOfPulses
                                        };
  MMP_ULONG ulMaxFreq;
  
  MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulMaxFreq);
  ulMaxFreq = ulMaxFreq * 250; 
  MMPF_PWM_EnableOutputPin(pwm_pin,MMP_TRUE);
  pwm_attribute.ubID = PWM_GET_ID(pwm_pin);
  pwm_attribute.ulClkDuty_T0 = ((MMP_ULONG)(ulMaxFreq/100)*pdDuty)/(pdFreqHz);
  pwm_attribute.ulClkDuty_T1 = ((MMP_ULONG)(ulMaxFreq)/(pdFreqHz));
  pwm_attribute.ulClkDuty_T2 = pwm_attribute.ulClkDuty_T1;
  pwm_attribute.ulClkDuty_T3 = pwm_attribute.ulClkDuty_T1;
  pwm_attribute.ulClkDuty_Peroid = (ulMaxFreq/pdFreqHz);
  MMPF_PWM_SetAttribe(&pwm_attribute);	
  MMPF_PWM_ControlSet(pwm_attribute.ubID, (PWM_PULSE_A_FIRST|
                                      PWM_AUTO_CYC|PWM_PULSE_B_NEG|
                                      PWM_PULSE_A_POS|PWM_EN));
  return MMP_ERR_NONE;
}
#endif