//==============================================================================
//
//  File        : mmpf_pio.c
//  Description : PIO Pins Control Interface
//  Author      : Ben Lu
//  Revision    : 1.0
//
//==============================================================================
/**
 *  @file mmpf_pio.c
 *  @brief The PIO pins control functions
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
#include "mmp_reg_gbl.h"
#include "mmp_reg_gpio.h"
#include "mmp_reg_pad.h"
#include "mmp_err.h"
#include "mmpf_pio.h"
#include "os_wrap.h"

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

/** @addtogroup MMPF_PIO
@{
*/
#if !defined(MINIBOOT_FW)
static MMPF_OS_SEMID    gPIO_OutputModeSemID;
static MMPF_OS_SEMID    gPIO_SetDataSemID;
#if (PIO_INTERRUPT_EN)
static MMPF_OS_SEMID    gPIO_EnableTrigModeSemID;
static MMPF_OS_SEMID    gPIO_EnableInterruptSemID;
#endif
#endif
#if (PIO_INTERRUPT_EN)
static MMP_ULONG        gPIO_BoundingTime[MMP_GPIO_NUM];
static GpioCallBackFunc *gPIO_CallBackFunc[MMP_GPIO_NUM];
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_Initialize
//  Description :
//------------------------------------------------------------------------------
/** @brief The function registers the interrupt and create related semaphore for PIO pins.

The function registers the interrupt and create related semaphore for PIO pins.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_Initialize(void)
{
    #if (PIO_INTERRUPT_EN)
    MMP_USHORT  i = 0;
    #if !defined(MINIBOOT_FW)
    AITPS_AIC   pAIC = AITC_BASE_AIC;
    #endif
    #endif
    static MMP_BOOL bPioInitFlag = MMP_FALSE;

    if (bPioInitFlag == MMP_FALSE) {
		#if !defined(MINIBOOT_FW)
        #if (PIO_INTERRUPT_EN)
        RTNA_AIC_Open(pAIC, AIC_SRC_GPIO, gpio_isr_a,
                        AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_GPIO);
        #endif

        gPIO_OutputModeSemID        = MMPF_OS_CreateSem(1);
        gPIO_SetDataSemID           = MMPF_OS_CreateSem(1);
        #if (PIO_INTERRUPT_EN)
        gPIO_EnableTrigModeSemID    = MMPF_OS_CreateSem(1);
        gPIO_EnableInterruptSemID   = MMPF_OS_CreateSem(1);
        #endif
        #endif

        #if (PIO_INTERRUPT_EN)
        MEMSET0(gPIO_BoundingTime);

        for (i = 0; i < MMP_GPIO_NUM ; i++) {
            gPIO_CallBackFunc[i] = NULL;
        }
        #endif

        bPioInitFlag = MMP_TRUE;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_Enable
//  Description :
//------------------------------------------------------------------------------
/** @brief The function enable or disable GPIO mode of multifunction IO.

The function configure the multifunction IO mode, set enable to GPIO mode.
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[in] bEnable is the choice of GPIO mode or not
@return It reports the status of the operation.
*/
void MMPF_PIO_Enable(MMP_GPIO_PIN piopin, MMP_BOOL bEnable)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);//bit pos within 4-bytes
    MMP_UBYTE   ubIndex = PIO_GET_INDEX(piopin); 				//index of 4-byte
    AITPS_GBL   pGBL = AITC_BASE_GBL;

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_Enable: Wrong GPIO number !!!")"\r\n");
        return;
	}

    if (bEnable)
        pGBL->GBL_GPIO_CFG[ubIndex] |= ulBitPos;
    else
        pGBL->GBL_GPIO_CFG[ubIndex] &= ~ulBitPos;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_WakeUpEnable
//  Description :
//------------------------------------------------------------------------------
/** @brief The function enable or disable GPIO wakeup mode.

The function configure the GPIO wakeup mode, set enable to wakeup system by GPIO.
@param[in] bEnable is the choice of GPIO wakeup mode enable or disable
@return It reports the status of the operation.
*/
void MMPF_PIO_WakeUpEnable(MMP_BOOL bEnable)
{
    AITPS_GPIO_CNT  pGPIOCTL = AITC_BASE_GPIOCTL;

    if (bEnable)
        pGPIOCTL->GPIO_WAKEUP_INT_EN = GPIO_WAKEUP_MODE_EN;
    else
        pGPIOCTL->GPIO_WAKEUP_INT_EN &= ~(GPIO_WAKEUP_MODE_EN);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_EnableOutputMode
//  Description :
//------------------------------------------------------------------------------
/** @brief The function set the PIN as Output mode (bEnable = MMP_TRUE) or Input mode.

The function set the PIN as Output mode (bEnable = MMP_TRUE) or Input mode.
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[in] bEnable is the choice of output mode or input mode
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_EnableOutputMode(MMP_GPIO_PIN piopin, MMP_BOOL bEnable, MMP_BOOL bOsProtectEn)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO	 = AITC_BASE_GPIO;
    #if !defined(MINIBOOT_FW)
    MMP_UBYTE   SemErr 	 = OS_NO_ERR;
    #endif

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_EnableOutputMode: Wrong GPIO number !!! - "));
        RTNA_DBG_Short(0, piopin);
        RTNA_DBG_Str(0, "\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn) {
        SemErr = MMPF_OS_AcquireSem(gPIO_OutputModeSemID, PIO_SEM_TIMEOUT);
	}
	#endif
	
    // Set IO to GPIO mode
    MMPF_PIO_Enable(piopin, MMP_TRUE);
	
    if (bEnable) {
        /* GPIO81~96, GPI only */
        if ((piopin >= MMP_GPIO81) && (piopin <= MMP_GPIO96)) {
            RTNA_DBG_Str(0, "Not support output mode\r\n");
            goto L_PioOut;
        }

        pGPIO->GPIO_OUT_EN[ubIndex] |= ulBitPos;
    }
    else {
        pGPIO->GPIO_OUT_EN[ubIndex] &= ~(ulBitPos);
    }

L_PioOut:

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn && (SemErr == OS_NO_ERR)) {
        MMPF_OS_ReleaseSem(gPIO_OutputModeSemID);
	}
	#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_ClearHostStatus
//  Description :
//------------------------------------------------------------------------------
/** @brief The function clear host interrupt status of the specified PIO pin.

The function clear host interrupt status of the specified PIO pin.
@param[in] piopin is the PIO number
@param[in] int_sr is interrupt type
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_ClearHostStatus(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG int_sr)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO 	 = AITC_BASE_GPIO;

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_ClearHostStatus: Wrong GPIO number !!!")"\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

    // Clear GPIO Host interrupt status
    switch(int_sr) {
    case GPIO_H2L_EDGE_TRIG:
        pGPIO->GPIO_INT_HOST_H2L_SR[ubIndex] = ulBitPos;
        break;
    case GPIO_L2H_EDGE_TRIG:
        pGPIO->GPIO_INT_HOST_L2H_SR[ubIndex] = ulBitPos;
        break;
    case GPIO_H_LEVEL_TRIG:
        pGPIO->GPIO_INT_HOST_H_SR[ubIndex] = ulBitPos;
        break;
    case GPIO_L_LEVEL_TRIG:
        pGPIO->GPIO_INT_HOST_L_SR[ubIndex] = ulBitPos;
        break;
    default:
        //
        break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_GetHostStatus
//  Description :
//------------------------------------------------------------------------------
/** @brief The function check host interrupt status of the specified PIO pin.

The function check host interrupt status of the specified PIO pin.
@param[in] piopin is the PIO number
@param[in] int_sr is interrupt type
@return It reports the status of the interrupt.
*/
MMP_BOOL MMPF_PIO_GetHostStatus(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG int_type)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO 	 = AITC_BASE_GPIO;
    MMP_ULONG   ulIntSr  = 0;

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_GetHostStatus: Wrong GPIO number !!!")"\r\n");
        return MMP_FALSE;
	}

    // Clear GPIO Host interrupt status
    switch(int_type) {
    case GPIO_H2L_EDGE_TRIG:
        ulIntSr = pGPIO->GPIO_INT_HOST_H2L_SR[ubIndex];
        break;
    case GPIO_L2H_EDGE_TRIG:
        ulIntSr = pGPIO->GPIO_INT_HOST_L2H_SR[ubIndex];
        break;
    case GPIO_H_LEVEL_TRIG:
        ulIntSr = pGPIO->GPIO_INT_HOST_H_SR[ubIndex];
        break;
    case GPIO_L_LEVEL_TRIG:
        ulIntSr = pGPIO->GPIO_INT_HOST_L_SR[ubIndex];
        break;
    default:
        //
        break;
    }

    return (ulIntSr & ulBitPos) ? MMP_TRUE : MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_SetData
//  Description :
//------------------------------------------------------------------------------
/** @brief The function set the PIO pin as High or Low (When the pin is at output mode).

The function set the PIO pin as High or Low (When the pin is at output mode).
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[in] outputValue is 0 stands for Low, otherwise it stands for High
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_SetData(MMP_GPIO_PIN piopin, MMP_UBYTE outputValue, MMP_BOOL bOsProtectEn)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO 	 = AITC_BASE_GPIO;
    #if !defined(MINIBOOT_FW)
    MMP_UBYTE   SemErr 	 = OS_NO_ERR;
    #endif

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_SetData: Wrong GPIO number !!!")"\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn) {
        SemErr = MMPF_OS_AcquireSem(gPIO_SetDataSemID, PIO_SEM_TIMEOUT);
	}
	#endif
	
    /* GPIO81~96, GPI only */
    if ((piopin >= MMP_GPIO81) && (piopin <= MMP_GPIO96)) {
        RTNA_DBG_Str(0, "Not support output mode\r\n");
        goto L_PioOut;
    }

    // Set IO to GPIO mode
    MMPF_PIO_Enable(piopin, MMP_TRUE);

    if (outputValue)
        pGPIO->GPIO_DATA[ubIndex] |= ulBitPos;
    else
        pGPIO->GPIO_DATA[ubIndex] &= ~ulBitPos;

L_PioOut:
	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn && (SemErr == OS_NO_ERR)) {
        MMPF_OS_ReleaseSem(gPIO_SetDataSemID);
	}
	#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_GetData
//  Description :
//------------------------------------------------------------------------------
/** @brief The function get the PIO pin's singal. (When the pin is at input mode).

The function get the PIO pin's singal. (When the pin is at input mode).
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[out] retValue is standing for the High or Low signal.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_GetData(MMP_GPIO_PIN piopin, MMP_UBYTE* retValue)
{
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO = AITC_BASE_GPIO;

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_GetData: Wrong GPIO number !!!")"\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

    if (pGPIO->GPIO_OUT_EN[ubIndex] & ulBitPos) {
        //Error !!! PIO Output Mode to call MMPF_PIO_GetData
        return MMP_PIO_ERR_OUTPUTMODEGETDATA;
    }

    // Set IO to GPIO mode
    MMPF_PIO_Enable(piopin, MMP_TRUE);
    
    *retValue = (pGPIO->GPIO_DATA[ubIndex] & ulBitPos) ? 1 : 0;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_EnableTrigMode
//  Description :
//------------------------------------------------------------------------------
/** @brief The function get the PIO pin's singal. (When the pin is at input mode).

The function get the PIO pin's singal. (When the pin is at input mode).
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[in] trigmode set the pio pin as edge trigger (H2L or L2H) or level trigger (H or L)
@param[out] bEnable make the tirgger settings work or not.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PIO_EnableTrigMode(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG trigmode, MMP_BOOL bEnable, MMP_BOOL bOsProtectEn)
{
    #if (PIO_INTERRUPT_EN)
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO 	 = AITC_BASE_GPIO;
    #if !defined(MINIBOOT_FW)
    MMP_UBYTE   SemErr 	 = OS_NO_ERR;
    #endif

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_EnableTrigMode: Wrong GPIO number !!!")"\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn) {
        SemErr = MMPF_OS_AcquireSem(gPIO_EnableTrigModeSemID, PIO_SEM_TIMEOUT);
	}
	#endif
	
    if (bEnable) {
        switch(trigmode) {
        case GPIO_H2L_EDGE_TRIG:
            pGPIO->GPIO_INT_H2L_EN[ubIndex] |= ulBitPos;
            break;
        case GPIO_L2H_EDGE_TRIG:
            pGPIO->GPIO_INT_L2H_EN[ubIndex] |= ulBitPos;
            break;
        case GPIO_H_LEVEL_TRIG:
            pGPIO->GPIO_INT_H_EN[ubIndex] |= ulBitPos;
            break;
        case GPIO_L_LEVEL_TRIG:
            pGPIO->GPIO_INT_L_EN[ubIndex] |= ulBitPos;
            break;
        }
    }
    else
    {
        switch(trigmode) {
        case GPIO_H2L_EDGE_TRIG:
            pGPIO->GPIO_INT_H2L_EN[ubIndex] &= ~ulBitPos;
            break;
        case GPIO_L2H_EDGE_TRIG:
            pGPIO->GPIO_INT_L2H_EN[ubIndex] &= ~ulBitPos;
            break;
        case GPIO_H_LEVEL_TRIG:
            pGPIO->GPIO_INT_H_EN[ubIndex] &= ~ulBitPos;
            break;
        case GPIO_L_LEVEL_TRIG:
            pGPIO->GPIO_INT_L_EN[ubIndex] &= ~ulBitPos;
            break;
        }
    }
    #if !defined(MINIBOOT_FW)
    if (bOsProtectEn && (SemErr == OS_NO_ERR)) {
        MMPF_OS_ReleaseSem(gPIO_EnableTrigModeSemID);
	}
	#endif
    #endif
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_EnableInterrupt
//  Description :
//------------------------------------------------------------------------------
/** @brief The function set the PIO pin's interrupt actions.

The function set the PIO pin's interrupt actions.
@param[in] piopin is the PIO number, please reference the data structure of MMP_GPIO_PIN
@param[in] bEnable stands for enable interrupt or not.
@param[in] boundingTime is used for advanced interrupt settings.
@param[in] CallBackFunc is used by interrupt handler.
@return It reports the status of the operation.
*/
#if !defined(MINIBOOT_FW)
MMP_ERR MMPF_PIO_EnableInterrupt(MMP_GPIO_PIN piopin, MMP_BOOL bEnable, MMP_ULONG boundingTime, GpioCallBackFunc *cb, MMP_BOOL bOsProtectEn)
{
    #if (PIO_INTERRUPT_EN)
    MMP_ULONG   ulBitPos = 1 << (piopin & PIO_BITPOSITION_INFO);
    MMP_UBYTE   ubIndex  = PIO_GET_INDEX(piopin);
    AITPS_GPIO  pGPIO 	 = AITC_BASE_GPIO;
    #if !defined(MINIBOOT_FW)
    MMP_UBYTE   SemErr 	 = OS_NO_ERR;
    #endif
	
    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, FG_RED("MMPF_PIO_EnableInterrupt: Wrong GPIO number !!!")"\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}
	
	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn) {
        SemErr = MMPF_OS_AcquireSem(gPIO_EnableInterruptSemID, PIO_SEM_TIMEOUT);
	}
	#endif

    if (bEnable) {
        gPIO_BoundingTime[piopin] = boundingTime;
        gPIO_CallBackFunc[piopin] = cb;
        pGPIO->GPIO_INT_CPU_EN[ubIndex] |= ulBitPos;
    }
    else {
        pGPIO->GPIO_INT_CPU_EN[ubIndex] &= ~ulBitPos;
        gPIO_BoundingTime[piopin] = 0;
        gPIO_CallBackFunc[piopin] = NULL;
    }
	
	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn && (SemErr == OS_NO_ERR)) {
        MMPF_OS_ReleaseSem(gPIO_EnableInterruptSemID);
	}
	#endif
    #else
    RTNA_DBG_Str(0, "PIO interrupt disabled\r\n");
    #endif
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_ISR
//  Description :
//------------------------------------------------------------------------------
/** @brief PIO pin's interrupt handler function.

PIO pin's interrupt handler function.
@return void
*/
void MMPF_PIO_ISR(void)
{
    #if (PIO_INTERRUPT_EN)
    MMP_ULONG       i = 0, j = 0;
    MMP_ULONG       intsrc_H = 0, intsrc_L = 0, intsrc_H2L = 0, intsrc_L2H = 0;
    MMP_GPIO_PIN    piopin = MMP_GPIO_NUM;
    AITPS_GPIO      pGPIO = AITC_BASE_GPIO;

    for (i = 0; i < 4; i++) {
        intsrc_H    = pGPIO->GPIO_INT_CPU_EN[i] & pGPIO->GPIO_INT_H_SR[i];
        intsrc_L    = pGPIO->GPIO_INT_CPU_EN[i] & pGPIO->GPIO_INT_L_SR[i];
        intsrc_H2L  = pGPIO->GPIO_INT_CPU_EN[i] & pGPIO->GPIO_INT_H2L_SR[i];
        intsrc_L2H  = pGPIO->GPIO_INT_CPU_EN[i] & pGPIO->GPIO_INT_L2H_SR[i];

        if(intsrc_H != 0x0){
            pGPIO->GPIO_INT_H_SR[i] = intsrc_H;
            for (j = 1; j <= 0x20; j++){
                if((intsrc_H >> j) == 0) break;
            }
            piopin = i*0x20 + (j-1);
            break;
        }

        if(intsrc_L != 0x0){
            pGPIO->GPIO_INT_L_SR[i] = intsrc_L;
            for (j = 1; j <= 0x20; j++){
                if((intsrc_L >> j) == 0) break;
            }
            piopin = i*0x20 + (j-1);
            break;
        }

        if(intsrc_H2L != 0x0){
            pGPIO->GPIO_INT_H2L_SR[i] = intsrc_H2L;
            for (j = 1; j <= 0x20; j++){
                if((intsrc_H2L >> j) == 0) break;
            }
            piopin = i*0x20 + (j-1);
            break;
        }

        if(intsrc_L2H != 0x0){
            pGPIO->GPIO_INT_L2H_SR[i] = intsrc_L2H;
            for (j = 1; j <= 0x20; j++){
                if((intsrc_L2H >> j) == 0) break;
            }
            piopin = i*0x20 + (j-1);
            break;
        }
    }

    if (piopin < MMP_GPIO_NUM) {
        if (gPIO_CallBackFunc[piopin] != NULL)
            (*gPIO_CallBackFunc[piopin])(piopin);
    }
    #endif
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_PIO_PadConfig
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PIO_PadConfig(MMP_GPIO_PIN piopin, MMP_UBYTE bConfig, MMP_BOOL bOsProtectEn)
{
    AITPS_PAD   pPAD  = AITC_BASE_PAD;
    #if !defined(MINIBOOT_FW)
    MMP_UBYTE   SemErr 	 = OS_NO_ERR;
    #endif

    if (piopin >= MMP_GPIO_NUM) {
        RTNA_DBG_Str(0, BG_RED("MMPF_PIO_PadConfig: Wrong GPIO number !!!\r\n"));
        RTNA_DBG_Short(0, piopin);
        RTNA_DBG_Str(0, "\r\n");
        return MMP_PIO_ERR_PARAMETER;
	}

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn) {
        SemErr = MMPF_OS_AcquireSem(gPIO_OutputModeSemID, PIO_SEM_TIMEOUT);
    }

    if (OS_NO_ERR != SemErr) {
        RTNA_DBG_Str(0, BG_RED("MMPF_PIO_PadConfig: Wait semaphore fail !!!\r\n"));
        RTNA_DBG_Short(0, piopin);
        RTNA_DBG_Str(0, "\r\n");
    }
    #endif

    {
    	#if !defined(MINIBOOT_FW)
    	OS_CRITICAL_INIT();
    	OS_ENTER_CRITICAL();
        #endif
        
        bConfig &= 0x6F;
        
        if (piopin < MMP_GPIO2) {
            pPAD->PAD_IO_CFG_PHI2C[piopin - MMP_GPIO0] = bConfig;
        }
        else if (piopin < MMP_GPIO10) { 
            pPAD->PAD_IO_CFG_PAGPIO[piopin - MMP_GPIO2] = bConfig;
        }
        else if (piopin < MMP_GPIO32) { 
            pPAD->PAD_IO_CFG_PBGPIO[piopin - MMP_GPIO10] = bConfig;
        }
        else if (piopin < MMP_GPIO64) { 
            pPAD->PAD_IO_CFG_PCGPIO[piopin - MMP_GPIO32] = bConfig;
        }
        else if (piopin < MMP_GPIO70) { 
            pPAD->PAD_IO_CFG_PDGPIO[piopin - MMP_GPIO64] = bConfig;
        }
        else if (piopin < MMP_GPIO81) { 
            pPAD->PAD_IO_CFG_PSNR[piopin - MMP_GPIO70] = bConfig;
        }
        else if (piopin < MMP_GPIO97) { 
            // NOP
        }
        else if (piopin < MMP_GPIO102) { 
            pPAD->PAD_IO_CFG_PI2S[piopin - MMP_GPIO97] = bConfig;
        }
        else if (piopin < MMP_GPIO118) { 
            // NOP
            //pPAD->PAD_IO_CFG_PLCD[piopin - MMP_GPIO102] = bConfig;
        }
        else if (piopin < MMP_GPIO125) { 
            // NOP
            //pPAD->PAD_IO_CFG_PLCD[piopin - MMP_GPIO118] = bConfig;
        }
        else if (piopin < MMP_GPIO_NUM) { 
            pPAD->PAD_IO_CFG_HDMI[piopin - MMP_GPIO125] = bConfig;
        }

    	#if !defined(MINIBOOT_FW)
		OS_EXIT_CRITICAL();
		#endif
	}

	#if !defined(MINIBOOT_FW)
    if (bOsProtectEn && (SemErr == OS_NO_ERR)) {
        MMPF_OS_ReleaseSem(gPIO_OutputModeSemID);
	}
	#endif

    return MMP_ERR_NONE;
}

/** @} */ // MMPF_PIO
