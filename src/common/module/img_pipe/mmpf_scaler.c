//------------------------------------------------------------------------------
//
//  File        : mmpf_scaler.c
//  Description : Firmware Scaler Control Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "lib_retina.h"
#include "mmp_lib.h"
#include "mmp_reg_scaler.h"
#include "mmp_reg_color.h"
#include "mmpf_scaler.h"
#include "mmpf_system.h"

#if (SENSOR_EN)
#include "mmpf_sensor.h"
#include "mmpf_rawproc.h"
#endif

/** @addtogroup MMPF_Scaler
@{
*/

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMP_UBYTE                m_ubScalerInput[MMP_SCAL_PIPE_NUM] = {0};
static ScalCallBackFunc         *CallBackFuncScaler[MMP_SCAL_PIPE_NUM][MMP_SCAL_EVENT_MAX];
static void            	        *CallBackArguScaler[MMP_SCAL_PIPE_NUM][MMP_SCAL_EVENT_MAX];
static MMP_SCAL_COLRMTX_MODE	m_ubScalerOutColorMode[MMP_SCAL_PIPE_NUM] = {MMP_SCAL_COLRMTX_YUV_FULLRANGE, MMP_SCAL_COLRMTX_YUV_FULLRANGE, MMP_SCAL_COLRMTX_YUV_FULLRANGE, MMP_SCAL_COLRMTX_YUV_FULLRANGE};

/* For BT601 Color Space */
static MMP_SHORT m_ScalerColorOffset_BT601[3] = {
    16, 128, 128
};
static MMP_SHORT m_ScalerColorMatrix_BT601[3][3] = {
    {219,   0,   0},
    {  0, 224,   0},
    {  0,   0, 224}
};
static MMP_SHORT m_ScalerColorClip_BT601[3][2] = {
    { 16, 235},
    { 16, 240},
    { 16, 240}
};

/* For BT601 to Full Range Color Space */
static MMP_SHORT m_ScalerColorOffset_BT601_FullRange[3] = {
    -19, 128, 128
};
static MMP_SHORT m_ScalerColorMatrix_BT601_FullRange[3][3] = {
    {297,   0,   0},
    {  0, 290,   0},
    {  0,   0, 290}
};
static MMP_SHORT m_ScalerColorClip_BT601_FullRange[3][2] = {
    { 0, 255},
    { 0, 255},
    { 0, 255}
};

/* For RGB Color Space */
#if 0 // For BT601 to RGB
static MMP_SHORT m_ScalerColorOffset_RGB[3] = {
    -19, -19, -19
};
static MMP_SHORT m_ScalerColorMatrix_RGB[3][3] = {
    {298,   0,   409},
    {298, -99,  -205},
    {298, 511,     0}
};
#else
static MMP_SHORT m_ScalerColorOffset_RGB[3] = {
    0, 0, 0
};
static MMP_SHORT m_ScalerColorMatrix_RGB[3][3] = {
    {256,   0,   359},
    {256, -87,  -180},
    {256, 453,     0}
};
#endif
static MMP_SHORT m_ScalerColorClip_RGB[3][2] = {
    { 0, 255},
    { 0, 255},
    { 0, 255}
};

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Internal_Function____(){ruturn;} //dummy
#endif

/* Standard C Function: Greatest Common Divisor */
MMP_USHORT Greatest_Common_Divisor(MMP_USHORT a, MMP_USHORT b)
{
  	MMP_USHORT 	c = 0;
  	MMP_ULONG 	ulLoop = 0;
  
  	while ( a != 0 ) {
     	c = a; 
     	a = b % a;  
     	b = c;
     	ulLoop++;
     
     	if (ulLoop >= 1000) 
     		break;
  	}
  	
	if (ulLoop >= 1000) { // Normally under 10
		RTNA_DBG_Str(0, "Greatest_Common_Divisor..Timeout\r\n ");
	}

  	return b;
}

#if 0
void ____Scaler_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_ResetModule
//  Description : This function reset scaler module.
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_ResetModule(MMP_SCAL_PIPEID pipeID)
{
    /* Mercury_V2 bug: 
     * It needs to set the output grab range again after reset scaler module, 
     * otherwise the scaler state machine will be wrong. 
     */
    MMP_USHORT usHStartPos, usHEndPos; 
    MMP_USHORT usVStartPos, usVEndPos;
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;
    MMP_BOOL   bScaleClkEn = MMP_TRUE;

    if (MMPF_SYS_CheckClockEnable(MMPF_SYS_CLK_SCALE) == MMP_FALSE) {
        bScaleClkEn = MMP_FALSE;
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_TRUE);
    }

    MMPF_Scaler_GetGrabPosition(pipeID, 
                                MMP_SCAL_GRAB_STAGE_OUT,
                                &usHStartPos, 
                                &usHEndPos, 
                                &usVStartPos, 
                                &usVEndPos);

	if (pipeID == MMP_SCAL_PIPE_0) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_SCAL0, MMP_FALSE);
		pSCAL->SCAL_P0_CPU_INT_SR = 0xFF;
	}
	else if (pipeID == MMP_SCAL_PIPE_1) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_SCAL1, MMP_FALSE);
		pSCAL->SCAL_P1_CPU_INT_SR = 0xFF;
	}
	else if (pipeID == MMP_SCAL_PIPE_2) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_SCAL2, MMP_FALSE);
		pSCAL->SCAL_P2_CPU_INT_SR = 0xFF;
	}
	else if (pipeID == MMP_SCAL_PIPE_3) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_SCAL3, MMP_FALSE);
		pSCAL->SCAL_P3_CPU_INT_SR = 0xFF;
	}
	else if (pipeID == MMP_SCAL_PIPE_4) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_SCAL4, MMP_FALSE);
		pSCAL->SCAL_P4_CPU_INT_SR = 0xFF;
	}

    MMPF_Scaler_SetGrabPosition(pipeID, 
                                MMP_SCAL_GRAB_STAGE_OUT,
                                usHStartPos, 
                                usHEndPos, 
                                usVStartPos, 
                                usVEndPos);

    if (bScaleClkEn == MMP_FALSE) {
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_FALSE);
    }
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetStopEnable
//  Description : This function set scaler stop mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function set scaler stop mode.
 * 
 *  This function set scaler stop mode.
 * @param[in] pipeID   : stands for scaler path index.  
 * @param[in] ubStopSrc : stands for stop source. 
 * @param[in] bEn       : stands for stop mode enable.  
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetStopEnable(MMP_SCAL_PIPEID pipeID, MMP_SCAL_STOP_SRC ubStopSrc, MMP_BOOL bEn)
{
	AITPS_SCAL pSCAL = AITC_BASE_SCAL;

    if (ubStopSrc == MMP_SCAL_STOP_SRC_H264)
    {
        switch(pipeID)
        {
            case MMP_SCAL_PIPE_0:
                if (bEn)
                    pSCAL->SCAL_P0_H264_STOP_EN |= SCAL_STOP_BY_H264_EN;
                else
                    pSCAL->SCAL_P0_H264_STOP_EN &= ~(SCAL_STOP_BY_H264_EN);
            break;
            case MMP_SCAL_PIPE_1:
                if (bEn)
                    pSCAL->SCAL_P1_H264_STOP_EN |= SCAL_STOP_BY_H264_EN;
                else
                    pSCAL->SCAL_P1_H264_STOP_EN &= ~(SCAL_STOP_BY_H264_EN);
            break;
            case MMP_SCAL_PIPE_2:
                if (bEn)
                    pSCAL->SCAL_P2_H264_STOP_EN |= SCAL_STOP_BY_H264_EN;
                else
                    pSCAL->SCAL_P2_H264_STOP_EN &= ~(SCAL_STOP_BY_H264_EN);
            break;
            case MMP_SCAL_PIPE_3:
                if (bEn)
                    pSCAL->SCAL_P3_H264_STOP_EN |= SCAL_STOP_BY_H264_EN;
                else
                    pSCAL->SCAL_P3_H264_STOP_EN &= ~(SCAL_STOP_BY_H264_EN);
            break;
            case MMP_SCAL_PIPE_4:
                if (bEn)
                    pSCAL->SCAL_P4_H264_STOP_EN |= SCAL_STOP_BY_H264_EN;
                else
                    pSCAL->SCAL_P4_H264_STOP_EN &= ~(SCAL_STOP_BY_H264_EN);
            break;
            default:
            //
            break;
        }
    }
    else if(ubStopSrc == MMP_SCAL_STOP_SRC_JPEG)
    {
        switch(pipeID)
        {
            case MMP_SCAL_PIPE_0:
                if (bEn)
                    pSCAL->SCAL_P0_BUSY_MODE_CTL |= SCAL_STOP_BY_JPEG_EN;
                else
                    pSCAL->SCAL_P0_BUSY_MODE_CTL &= ~(SCAL_STOP_BY_JPEG_EN);
            break;
            case MMP_SCAL_PIPE_1:
                if (bEn)
                    pSCAL->SCAL_P1_BUSY_MODE_CTL |= SCAL_STOP_BY_JPEG_EN;
                else
                    pSCAL->SCAL_P1_BUSY_MODE_CTL &= ~(SCAL_STOP_BY_JPEG_EN);
            break;
            case MMP_SCAL_PIPE_2:
                if (bEn)
                    pSCAL->SCAL_P2_BUSY_MODE_CTL |= SCAL_STOP_BY_JPEG_EN;
                else
                    pSCAL->SCAL_P2_BUSY_MODE_CTL &= ~(SCAL_STOP_BY_JPEG_EN);
            break;
            case MMP_SCAL_PIPE_3:
                if (bEn)
                    pSCAL->SCAL_P3_BUSY_MODE_CTL |= SCAL_STOP_BY_JPEG_EN;
                else
                    pSCAL->SCAL_P3_BUSY_MODE_CTL &= ~(SCAL_STOP_BY_JPEG_EN);
            break;
            case MMP_SCAL_PIPE_4:
                if (bEn)
                    pSCAL->SCAL_P4_BUSY_MODE_CTL |= SCAL_STOP_BY_JPEG_EN;
                else
                    pSCAL->SCAL_P4_BUSY_MODE_CTL &= ~(SCAL_STOP_BY_JPEG_EN);
            break;
            default:
            //
            break;
        }
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetBusyEnable
//  Description : This function set scaler busy mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function set scaler busy mode.
 * 
 *  This function set scaler busy mode.
 * @param[in] pipeID   : stands for scaler path index.
 * @param[in] bEn       : stands for busy mode enable.  
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetBusyMode(MMP_SCAL_PIPEID pipeID, MMP_BOOL bEn)
{
	AITPS_SCAL pSCAL = AITC_BASE_SCAL;
	
    switch(pipeID)
    {
        case MMP_SCAL_PIPE_0:
            if (bEn)
                pSCAL->SCAL_P0_BUSY_MODE_CTL |= SCAL_BUSY_MODE_EN;
            else
                pSCAL->SCAL_P0_BUSY_MODE_CTL &= ~(SCAL_BUSY_MODE_EN);
        break;
        case MMP_SCAL_PIPE_1:
            if (bEn)
                pSCAL->SCAL_P1_BUSY_MODE_CTL |= SCAL_BUSY_MODE_EN;
            else
                pSCAL->SCAL_P1_BUSY_MODE_CTL &= ~(SCAL_BUSY_MODE_EN);
        break;
        case MMP_SCAL_PIPE_2:
            if (bEn)
                pSCAL->SCAL_P2_BUSY_MODE_CTL |= SCAL_BUSY_MODE_EN;
            else
                pSCAL->SCAL_P2_BUSY_MODE_CTL &= ~(SCAL_BUSY_MODE_EN);
        break;
        case MMP_SCAL_PIPE_3:
            if (bEn)
                pSCAL->SCAL_P3_BUSY_MODE_CTL |= SCAL_BUSY_MODE_EN;
            else
                pSCAL->SCAL_P3_BUSY_MODE_CTL &= ~(SCAL_BUSY_MODE_EN);
        break;
        case MMP_SCAL_PIPE_4:
            if (bEn)
                pSCAL->SCAL_P4_BUSY_MODE_CTL |= SCAL_BUSY_MODE_EN;
            else
                pSCAL->SCAL_P4_BUSY_MODE_CTL &= ~(SCAL_BUSY_MODE_EN);
        break;
        default:
        break;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetMaxDelayLineWidth
//  Description :
//------------------------------------------------------------------------------
/** 
 * @brief This function get max delay line width.
 * 
 *  This function get max delay line width.
 * @param[in] pipeID   : stands for scaler path index. 
 * @return It return the function status.  
 */
MMP_ULONG MMPF_Scaler_GetMaxDelayLineWidth(MMP_SCAL_PIPEID pipeID)
{
	MMP_ULONG ulMaxWidth = 0;
	
    switch(pipeID)
    {
        case MMP_SCAL_PIPE_0:
            ulMaxWidth = SCALER_PATH0_MAX_WIDTH;
        break;
        case MMP_SCAL_PIPE_1:
            ulMaxWidth = SCALER_PATH1_MAX_WIDTH;
        break;
        case MMP_SCAL_PIPE_2:
            ulMaxWidth = SCALER_PATH2_MAX_WIDTH;
        break;
        case MMP_SCAL_PIPE_3:
            ulMaxWidth = SCALER_PATH3_MAX_WIDTH;
        break;
        case MMP_SCAL_PIPE_4:
            ulMaxWidth = SCALER_PATH4_MAX_WIDTH;
        break;
        default:
        //
        break;
    }

    return ulMaxWidth;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_ISR
//  Description : This function is the Scaler interrupt handler.
//------------------------------------------------------------------------------
/** 
 * @brief This function is the Scaler interrupt handler.
 * 
 *  This function is the Scaler interrupt handler.  
 */
void MMPF_Scaler_ISR(void)
{
#if (SCALER_ISR_EN)

	MMP_UBYTE 	i, intsrc[MMP_SCAL_PIPE_NUM];
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
	
	intsrc[0] = pSCAL->SCAL_P0_CPU_INT_EN & pSCAL->SCAL_P0_CPU_INT_SR;
	intsrc[1] = pSCAL->SCAL_P1_CPU_INT_EN & pSCAL->SCAL_P1_CPU_INT_SR;
	intsrc[2] = pSCAL->SCAL_P2_CPU_INT_EN & pSCAL->SCAL_P2_CPU_INT_SR;
	intsrc[3] = pSCAL->SCAL_P3_CPU_INT_EN & pSCAL->SCAL_P3_CPU_INT_SR;
	intsrc[4] = pSCAL->SCAL_P4_CPU_INT_EN & pSCAL->SCAL_P4_CPU_INT_SR;

	pSCAL->SCAL_P0_CPU_INT_SR = intsrc[0];
	pSCAL->SCAL_P1_CPU_INT_SR = intsrc[1];
	pSCAL->SCAL_P2_CPU_INT_SR = intsrc[2];
	pSCAL->SCAL_P3_CPU_INT_SR = intsrc[3];
	pSCAL->SCAL_P4_CPU_INT_SR = intsrc[4];
	
	for (i = MMP_SCAL_PIPE_0; i < MMP_SCAL_PIPE_NUM; i++)
	{
		if (intsrc[i] & SCAL_INT_FRM_ST) {

	        if (CallBackFuncScaler[i][MMP_SCAL_EVENT_FRM_ST]) {
	            ScalCallBackFunc *CallBack = CallBackFuncScaler[i][MMP_SCAL_EVENT_FRM_ST];
	            CallBack(CallBackArguScaler[i][MMP_SCAL_EVENT_FRM_ST]);
	        }
		}
		
		if (intsrc[i] & SCAL_INT_FRM_END) {

	        if (CallBackFuncScaler[i][MMP_SCAL_EVENT_FRM_END]) {
	            ScalCallBackFunc *CallBack = CallBackFuncScaler[i][MMP_SCAL_EVENT_FRM_END];
	            CallBack(CallBackArguScaler[i][SCAL_INT_FRM_END]);
	        }
		}

		if (intsrc[i] & SCAL_INT_INPUT_END) {

	        if (CallBackFuncScaler[i][MMP_SCAL_EVENT_INPUT_END]) {
	            ScalCallBackFunc *CallBack = CallBackFuncScaler[i][MMP_SCAL_EVENT_INPUT_END];
	            CallBack(CallBackArguScaler[i][SCAL_INT_INPUT_END]);
	        }
		}

		if (intsrc[i] & SCAL_INT_DBL_FRM_ST) {

			RTNA_DBG_Byte(3, i);
			RTNA_DBG_Str(3, " DFS\r\n");

	        if (CallBackFuncScaler[i][MMP_SCAL_EVENT_DBL_FRM_ST]) {
	            ScalCallBackFunc *CallBack = CallBackFuncScaler[i][MMP_SCAL_EVENT_DBL_FRM_ST];
	            CallBack(CallBackArguScaler[i][MMP_SCAL_EVENT_DBL_FRM_ST]);
	        }
		}
	}
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_RegisterIntrCallBack
//  Description : This function register interrupt callback function.
//------------------------------------------------------------------------------
/** 
 * @brief This function register interrupt callback function.
 * 
 *  This function register interrupt callback function.
 * @param[in] scalpath  : stands for scaler pipe ID.
 * @param[in] event     : stands for interrupt event.
 * @param[in] pCallBack : stands for interrupt callback function. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_RegisterIntrCallBack(MMP_SCAL_PIPEID scalpath, MMP_SCAL_EVENT event, ScalCallBackFunc *pCallBack, void *pArgument)
{
    if (pCallBack) {
        CallBackFuncScaler[scalpath][event] = pCallBack;
        CallBackArguScaler[scalpath][event] = (void *)pArgument;
    }
    else {
        CallBackFuncScaler[scalpath][event] = NULL;
        CallBackArguScaler[scalpath][event] = NULL;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_EnableInterrupt
//  Description : This function enable interrupt event.
//------------------------------------------------------------------------------
/** 
 * @brief This function enable interrupt event.
 * 
 *  This function enable interrupt event.
 * @param[in] pipeID  : stands for scaler pipe ID.
 * @param[in] event   : stands for interrupt event.
 * @param[in] bEnable : stands for interrupt enable. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_EnableInterrupt(MMP_SCAL_PIPEID pipeID, MMP_SCAL_EVENT event, MMP_BOOL bEnable)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;

    MMP_UBYTE ubFlags[MMP_SCAL_EVENT_MAX] = {SCAL_INT_FRM_ST, SCAL_INT_FRM_END, SCAL_INT_INPUT_END, SCAL_INT_DBL_FRM_ST};
    
    if (pipeID == MMP_SCAL_PIPE_0) {
        if (bEnable) {
            pSCAL->SCAL_P0_CPU_INT_SR = ubFlags[event];
            pSCAL->SCAL_P0_CPU_INT_EN |= ubFlags[event];
        }
        else {
            pSCAL->SCAL_P0_CPU_INT_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_SCAL_PIPE_1) {
        if (bEnable) {
            pSCAL->SCAL_P1_CPU_INT_SR = ubFlags[event];
            pSCAL->SCAL_P1_CPU_INT_EN |= ubFlags[event];
        }
        else {
            pSCAL->SCAL_P1_CPU_INT_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        if (bEnable) {
            pSCAL->SCAL_P2_CPU_INT_SR = ubFlags[event];
            pSCAL->SCAL_P2_CPU_INT_EN |= ubFlags[event];
        }
        else {
            pSCAL->SCAL_P2_CPU_INT_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_SCAL_PIPE_3) {
        if (bEnable) {
            pSCAL->SCAL_P3_CPU_INT_SR = ubFlags[event];
            pSCAL->SCAL_P3_CPU_INT_EN |= ubFlags[event];
        }
        else {
            pSCAL->SCAL_P3_CPU_INT_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_SCAL_PIPE_4) {
        if (bEnable) {
            pSCAL->SCAL_P4_CPU_INT_SR = ubFlags[event];
            pSCAL->SCAL_P4_CPU_INT_EN |= ubFlags[event];
        }
        else {
            pSCAL->SCAL_P4_CPU_INT_EN &= ~(ubFlags[event]);
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_OpenInterrupt
//  Description : This function open scaler interrupt.
//------------------------------------------------------------------------------
/** 
 * @brief This function open scaler interrupt.
 * 
 *  This function open scaler interrupt.
 * @param[in] bEnable : stands for interrupt enable. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_OpenInterrupt(MMP_BOOL bEnable)
{
    AITPS_AIC pAIC = AITC_BASE_AIC;
    
    if (bEnable) {
        RTNA_AIC_Open(pAIC, AIC_SRC_SCAL, scal_isr_a,
                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
		RTNA_AIC_IRQ_En(pAIC, AIC_SRC_SCAL);    
    }
    else {
        RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_SCAL);  
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_ClearFrameStart
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_ClearFrameStart(MMP_SCAL_PIPEID pipeID)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;

    if (pipeID == MMP_SCAL_PIPE_0) {
        pSCAL->SCAL_P0_HOST_INT_SR |= SCAL_INT_FRM_ST;
    }
    else if (pipeID == MMP_SCAL_PIPE_1) {
        pSCAL->SCAL_P1_HOST_INT_SR |= SCAL_INT_FRM_ST;
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        pSCAL->SCAL_P2_HOST_INT_SR |= SCAL_INT_FRM_ST;
    }
    else if (pipeID == MMP_SCAL_PIPE_3) {
        pSCAL->SCAL_P3_HOST_INT_SR |= SCAL_INT_FRM_ST;
    }
    else if (pipeID == MMP_SCAL_PIPE_4) {
        pSCAL->SCAL_P4_HOST_INT_SR |= SCAL_INT_FRM_ST;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_ClearFrameEnd
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_ClearFrameEnd(MMP_SCAL_PIPEID pipeID)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;

    if (pipeID == MMP_SCAL_PIPE_0) {
        pSCAL->SCAL_P0_HOST_INT_SR |= (SCAL_INT_FRM_END | SCAL_INT_INPUT_END);
    } 
    else if (pipeID == MMP_SCAL_PIPE_1) {
        pSCAL->SCAL_P1_HOST_INT_SR |= (SCAL_INT_FRM_END | SCAL_INT_INPUT_END);
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        pSCAL->SCAL_P2_HOST_INT_SR |= (SCAL_INT_FRM_END | SCAL_INT_INPUT_END);
    }
    else if (pipeID == MMP_SCAL_PIPE_3) {
        pSCAL->SCAL_P3_HOST_INT_SR |= (SCAL_INT_FRM_END | SCAL_INT_INPUT_END);
    }
    else if (pipeID == MMP_SCAL_PIPE_4) {
        pSCAL->SCAL_P4_HOST_INT_SR |= (SCAL_INT_FRM_END | SCAL_INT_INPUT_END);
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_BypassScaler
//  Description : This function set bypass scaler engine or not.
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_BypassScaler(MMP_SCAL_PIPEID pipeID, MMP_BOOL bBypass)
{
	AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
	AIT_REG_B 	*OprScaleCtl = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprScaleCtl = &(pSCAL->SCAL_SCAL_0_CTL);
	    break;    
		case MMP_SCAL_PIPE_1:
	        OprScaleCtl = &(pSCAL->SCAL_SCAL_1_CTL);
	    break;
		case MMP_SCAL_PIPE_2:
	        OprScaleCtl = &(pSCAL->SCAL_SCAL_2_CTL);
	    break;
		case MMP_SCAL_PIPE_3:
	        OprScaleCtl = &(pSCAL->SCAL_SCAL_3_CTL);
	    break;
		case MMP_SCAL_PIPE_4:
	        OprScaleCtl = &(pSCAL->SCAL_SCAL_4_CTL);
	    break;
        default:
        //
        break;
	}

    if (bBypass == MMP_TRUE)
    	(*OprScaleCtl) |= (SCAL_SCAL_BYPASS);
    else
    	(*OprScaleCtl) &= ~(SCAL_SCAL_BYPASS);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetEngine
//  Description : This function set scaler main engine.
//------------------------------------------------------------------------------
/** 
 * @brief This function set scaler main engine.
 * 
 *  This function set scaler main engine.
 * @param[in]     bUserDefine  : stands for use user define range.
 * @param[in]     pipeID      : stands for scaler pipe.
 * @param[in/out] fitrange     : stands for input/output range. 
 * @param[in/out] grabctl      : stands for output grab range. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE	bUserDefine,
							  MMP_SCAL_PIPEID 			pipeID,
                        	  MMP_SCAL_FIT_RANGE 		*fitrange, 
                        	  MMP_SCAL_GRAB_CTRL 		*grabctl)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    MMP_USHORT  unscale_width, unscale_height;
	
    AIT_REG_W   *OprGrabInHStart, *OprGrabInHEnd, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart, *OprGrabOutHEnd, *OprGrabOutVStart, *OprGrabOutVEnd;
    AIT_REG_W   *OprNh, *OprMh, *OprNv, *OprMv;
    AIT_REG_W   *OprHWeight, *OprVWeight;
    AIT_REG_B   *OprScaleCtl, *OprEdgeCtl;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprScaleCtl     = &(pSCAL->SCAL_SCAL_0_CTL);
	        OprGrabInHStart = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd   = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd   = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart= &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd  = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart= &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd  = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
	        OprNh           = &(pSCAL->SCAL_SCAL_0_H_N);
	        OprMh           = &(pSCAL->SCAL_SCAL_0_H_M);
	        OprNv           = &(pSCAL->SCAL_SCAL_0_V_N);
	        OprMv           = &(pSCAL->SCAL_SCAL_0_V_M);
	        OprHWeight      = &(pSCAL->SCAL_SCAL_0_H_WT);
	        OprVWeight      = &(pSCAL->SCAL_SCAL_0_V_WT);
	        OprEdgeCtl		= &(pSCAL->SCAL_EDGE_0_CTL);
		break;
		case MMP_SCAL_PIPE_1:
	        OprScaleCtl     = &(pSCAL->SCAL_SCAL_1_CTL);
	        OprGrabInHStart = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd   = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd   = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart= &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd  = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart= &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd  = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
	        OprNh           = &(pSCAL->SCAL_SCAL_1_H_N);
	        OprMh           = &(pSCAL->SCAL_SCAL_1_H_M);
	        OprNv           = &(pSCAL->SCAL_SCAL_1_V_N);
	        OprMv           = &(pSCAL->SCAL_SCAL_1_V_M);
	        OprHWeight      = &(pSCAL->SCAL_SCAL_1_H_WT);
	        OprVWeight      = &(pSCAL->SCAL_SCAL_1_V_WT);
	        OprEdgeCtl		= &(pSCAL->SCAL_EDGE_1_CTL);
		break;
		case MMP_SCAL_PIPE_2:
	        OprScaleCtl     = &(pSCAL->SCAL_SCAL_2_CTL);
	        OprGrabInHStart = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd   = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd   = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart= &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd  = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart= &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd  = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
	        OprNh           = &(pSCAL->SCAL_SCAL_2_H_N);
	        OprMh           = &(pSCAL->SCAL_SCAL_2_H_M);
	        OprNv           = &(pSCAL->SCAL_SCAL_2_V_N);
	        OprMv           = &(pSCAL->SCAL_SCAL_2_V_M);
	        OprHWeight      = &(pSCAL->SCAL_SCAL_2_H_WT);
	        OprVWeight      = &(pSCAL->SCAL_SCAL_2_V_WT);
	        OprEdgeCtl		= &(pSCAL->SCAL_EDGE_2_CTL);
		break;
		case MMP_SCAL_PIPE_3:
	        OprScaleCtl     = &(pSCAL->SCAL_SCAL_3_CTL);
	        OprGrabInHStart = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd   = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd   = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart= &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd  = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart= &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd  = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
	        OprNh           = &(pSCAL->SCAL_SCAL_3_H_N);
	        OprMh           = &(pSCAL->SCAL_SCAL_3_H_M);
	        OprNv           = &(pSCAL->SCAL_SCAL_3_V_N);
	        OprMv           = &(pSCAL->SCAL_SCAL_3_V_M);
	        OprHWeight      = &(pSCAL->SCAL_SCAL_3_H_WT);
	        OprVWeight      = &(pSCAL->SCAL_SCAL_3_V_WT);
	        OprEdgeCtl		= &(pSCAL->SCAL_EDGE_3_CTL);
		break;
		case MMP_SCAL_PIPE_4:
	        OprScaleCtl     = &(pSCAL->SCAL_SCAL_4_CTL);
	        OprGrabInHStart = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd   = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd   = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart= &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd  = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart= &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd  = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
	        OprNh           = &(pSCAL->SCAL_SCAL_4_H_N);
	        OprMh           = &(pSCAL->SCAL_SCAL_4_H_M);
	        OprNv           = &(pSCAL->SCAL_SCAL_4_V_N);
	        OprMv           = &(pSCAL->SCAL_SCAL_4_V_M);
	        OprHWeight      = &(pSCAL->SCAL_SCAL_4_H_WT);
	        OprVWeight      = &(pSCAL->SCAL_SCAL_4_V_WT);
	        OprEdgeCtl		= &(pSCAL->SCAL_EDGE_4_CTL);
		break;
		default:
			return MMP_ERR_NONE;
		break;
	}
    
    /* 1. Set 2nd & 3rd DownSampling */
	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
//			ulMaxScaleWidth = SCALER_PATH0_MAX_WIDTH;
			
	    	pSCAL->SCAL_DNSAMP_SCAL_0_H = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_SCAL_0_V = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_0_H 	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_0_V 	= SCAL_DNSAMP_NONE;	
		break;
		case MMP_SCAL_PIPE_1:
//	        ulMaxScaleWidth = SCALER_PATH1_MAX_WIDTH;

	    	pSCAL->SCAL_DNSAMP_SCAL_1_H = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_SCAL_1_V	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_1_H 	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_1_V 	= SCAL_DNSAMP_NONE;
		break;
		case MMP_SCAL_PIPE_2:
//	        ulMaxScaleWidth = SCALER_PATH2_MAX_WIDTH;

	    	pSCAL->SCAL_DNSAMP_SCAL_2_H = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_SCAL_2_V = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_2_H 	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_2_V 	= SCAL_DNSAMP_NONE;	
		break;
		case MMP_SCAL_PIPE_3:
//	        ulMaxScaleWidth = SCALER_PATH3_MAX_WIDTH;

	    	pSCAL->SCAL_DNSAMP_SCAL_3_H = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_SCAL_3_V = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_3_H 	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_3_V 	= SCAL_DNSAMP_NONE;	
		break;
		case MMP_SCAL_PIPE_4:
//	        ulMaxScaleWidth = SCALER_PATH4_MAX_WIDTH;

	    	pSCAL->SCAL_DNSAMP_SCAL_4_H = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_SCAL_4_V = SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_4_H 	= SCAL_DNSAMP_NONE;
	    	pSCAL->SCAL_DNSAMP_OUT_4_V 	= SCAL_DNSAMP_NONE;	
		break;
		default:
        //
		break;
	}

    /* 2. Set Edge Enhancement : Not used */
	(*OprEdgeCtl) |= SCAL_EDGE_BYPASS;

    /* 3. Set Scaler ratio and weighting */
    (*OprScaleCtl) &= ~(SCAL_SCAL_BYPASS);
    
    if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT || 
  		fitrange->fitmode == MMP_SCAL_FITMODE_IN) 
  	{
    	if (grabctl->ulScaleN > grabctl->ulScaleM) {
		    (*OprScaleCtl) |= (SCAL_SCAL_DBL_FIFO | SCAL_UP_EARLY_ST);	    
	    }
	    else {
		    (*OprScaleCtl) &= ~(SCAL_SCAL_DBL_FIFO | SCAL_UP_EARLY_ST);
	    }

	    *OprNh = grabctl->ulScaleN;
	    *OprMh = grabctl->ulScaleM;
	    *OprNv = grabctl->ulScaleN;
	    *OprMv = grabctl->ulScaleM;
    }
    else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL)
    {
    	if (grabctl->ulScaleXN > grabctl->ulScaleXM ||
    	    grabctl->ulScaleYN > grabctl->ulScaleYM) {
		    (*OprScaleCtl) |= (SCAL_SCAL_DBL_FIFO | SCAL_UP_EARLY_ST);	    
	    }
	    else {
		    (*OprScaleCtl) &= ~(SCAL_SCAL_DBL_FIFO | SCAL_UP_EARLY_ST);
	    }

	    *OprNh = grabctl->ulScaleXN;
	    *OprMh = grabctl->ulScaleXM;
	    *OprNv = grabctl->ulScaleYN;
	    *OprMv = grabctl->ulScaleYM; 
    }
    
    (*OprHWeight) &= ~(SCAL_SCAL_WT_AVG);
    (*OprVWeight) &= ~(SCAL_SCAL_WT_AVG);

    if ((*OprNh)*5 < (*OprMh)*3) {
        *OprHWeight = (*OprNh) >> 1;
        *OprVWeight = (*OprNv) >> 1;
    }
    else {
        *OprHWeight = 0;
        *OprVWeight = 0;
    }
    
    /* 5. Calculate scaler(2nd) grab range */
    if (bUserDefine == MMP_SCAL_USER_DEF_TYPE_IN_OUT)
    {
   		if (fitrange->bUseLpfGrab) {
    		*OprGrabInHStart = 1;
    		*OprGrabInHEnd   = fitrange->ulInGrabW + fitrange->ulDummyInPixelX;
    		*OprGrabInVStart = 1;
    		*OprGrabInVEnd   = fitrange->ulInGrabH + fitrange->ulDummyInPixelY;	
		}
		else {
    		*OprGrabInHStart = fitrange->ulInGrabX;
    		*OprGrabInHEnd   = fitrange->ulInGrabX + (fitrange->ulInGrabW - 1) + fitrange->ulDummyInPixelX;
    		*OprGrabInVStart = fitrange->ulInGrabY;
    		*OprGrabInVEnd   = fitrange->ulInGrabY + (fitrange->ulInGrabH - 1) + fitrange->ulDummyInPixelY;
		}

	    *OprGrabOutHStart = grabctl->ulOutStX;
	    *OprGrabOutHEnd   = grabctl->ulOutEdX;
	    *OprGrabOutVStart = grabctl->ulOutStY;
	    *OprGrabOutVEnd   = grabctl->ulOutEdY;
    }
    else if (bUserDefine == MMP_SCAL_USER_DEF_TYPE_OUT)
    {
    	MMP_ULONG ulTmpScaleXN = 0, ulTmpScaleXM = 0, ulTmpScaleYN = 0, ulTmpScaleYM = 0;

	    if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT || 
	  		fitrange->fitmode == MMP_SCAL_FITMODE_IN) 
	  	{
		    ulTmpScaleXN = grabctl->ulScaleN;
		    ulTmpScaleXM = grabctl->ulScaleM;
		    ulTmpScaleYN = grabctl->ulScaleN;
		    ulTmpScaleYM = grabctl->ulScaleM;
	    }
	    else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL)
	    {
		    ulTmpScaleXN = grabctl->ulScaleXN;
		    ulTmpScaleXM = grabctl->ulScaleXM;
		    ulTmpScaleYN = grabctl->ulScaleYN;
		    ulTmpScaleYM = grabctl->ulScaleYM; 
	    }

        if ((ulTmpScaleXN > ulTmpScaleXM) || (ulTmpScaleYN > ulTmpScaleYM)) 
        {
            MMP_ULONG x_st = (grabctl->ulOutStX * ulTmpScaleXM + ulTmpScaleXN -1) / ulTmpScaleXN;
            MMP_ULONG y_st = (grabctl->ulOutStY * ulTmpScaleYM + ulTmpScaleYN -1) / ulTmpScaleYN;

        	//Scale up function: UP((length_h-1)*Nh/Mh) * UP((length_v-1)*Nv/Mv)
            unscale_width  = 3 + ((grabctl->ulOutEdX - grabctl->ulOutStX) * ulTmpScaleXM) / ulTmpScaleXN;
		    unscale_height = 3 + ((grabctl->ulOutEdY - grabctl->ulOutStY) * ulTmpScaleYM) / ulTmpScaleYN;

		    if ((x_st + unscale_width - 1) > fitrange->ulInWidth) {
		    	unscale_width = fitrange->ulInWidth - x_st + 1;
		    }

		    if ((y_st + unscale_height - 1) > fitrange->ulInHeight) {
		    	unscale_height = fitrange->ulInHeight - y_st + 1;
		    }
	        
	        /* Scaler(2nd) grab needed pixels then Output(3rd) grab all pixels */
	        *OprGrabInHStart    = (grabctl->ulOutStX * ulTmpScaleXM + ulTmpScaleXN -1) / ulTmpScaleXN;
	        *OprGrabInHEnd      = *OprGrabInHStart + unscale_width - 1;
	        *OprGrabInVStart    = (grabctl->ulOutStY * ulTmpScaleYM + ulTmpScaleYN -1) / ulTmpScaleYN;
	        *OprGrabInVEnd      = *OprGrabInVStart + unscale_height - 1;

			*OprGrabOutHStart   = 1;
			*OprGrabOutHEnd     = grabctl->ulOutEdX - grabctl->ulOutStX + 1;
			*OprGrabOutVStart   = 1;
			*OprGrabOutVEnd     = grabctl->ulOutEdY - grabctl->ulOutStY + 1;
		}
		else {
		    /* Scaler(2nd) grab all pixels then Output(3rd) grab needed pixels */
	        *OprGrabInHStart    = 1;
	        *OprGrabInHEnd      = fitrange->ulInWidth;
	        *OprGrabInVStart    = 1;
	        *OprGrabInVEnd      = fitrange->ulInHeight;
			
			*OprGrabOutHStart   = grabctl->ulOutStX;
			*OprGrabOutHEnd     = grabctl->ulOutEdX;
			*OprGrabOutVStart   = grabctl->ulOutStY;
			*OprGrabOutVEnd     = grabctl->ulOutEdY;
	    }
    }
    
    if ((*OprNh != *OprMh) && ((*OprGrabOutHEnd - *OprGrabOutHStart) > MMPF_Scaler_GetMaxDelayLineWidth(pipeID))) {
        printc(FG_RED("Width > Pipe %d MaxDelayLineWidth(%d).\r\n"),  pipeID, MMPF_Scaler_GetMaxDelayLineWidth(pipeID));	
    }    

	#ifdef SCAL_FUNC_DBG
	MMPF_Scaler_DumpSetting((void *)__func__, fitrange, grabctl);
	#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetLPF
//  Description : The function set the LPF coefficient according to the grab control
//------------------------------------------------------------------------------
/** 
 * @brief The function set the LPF coefficient according to the grab control
 * 
 *  The function set the LPF coefficient according to the grab control
 * @param[in]     pipeID      : stands for scaler pipe.
 * @param[in/out] fitrange     : stands for input/output range. 
 * @param[in/out] grabctl      : stands for output grab range. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetLPF( MMP_SCAL_PIPEID 	pipeID,
							MMP_SCAL_FIT_RANGE 	*fitrange,
                            MMP_SCAL_GRAB_CTRL 	*grabctl)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    MMP_ULONG	ulMaxScaleWidth;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL;
    AIT_REG_W   *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabLpfHStart = NULL, *OprGrabLpfHEnd = NULL;
    AIT_REG_W   *OprGrabLpfVStart = NULL, *OprGrabLpfVEnd = NULL;
    AIT_REG_W   *OprNh = NULL, *OprMh = NULL, *OprNv = NULL;
    AIT_REG_W   *OprHWeight = NULL, *OprVWeight = NULL;
    AIT_REG_B   *OprLpfAutoCtl = NULL;
    MMP_ULONG	ulScaleXN, ulScaleXM, ulScaleYN, ulScaleYM;
    MMP_UBYTE	ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_NONE;

    ulMaxScaleWidth = LPF_MAX_WIDTH;
	
	/* 1. Initial LPF setting */
	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLpfHStart 	= &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLpfHEnd   	= &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLpfVStart 	= &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLpfVEnd   	= &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart 	= &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd   	= &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart 	= &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd   	= &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprNh           	= &(pSCAL->SCAL_SCAL_0_H_N);
	        OprMh           	= &(pSCAL->SCAL_SCAL_0_H_M);
	        OprNv           	= &(pSCAL->SCAL_SCAL_0_V_N);
//	        OprMv           	= &(pSCAL->SCAL_SCAL_0_V_M);
	        OprHWeight      	= &(pSCAL->SCAL_SCAL_0_H_WT);
	        OprVWeight      	= &(pSCAL->SCAL_SCAL_0_V_WT);
			OprLpfAutoCtl		= &(pSCAL->SCAL_LPF_AUTO_CTL);

	        pSCAL->SCAL_LPF_AUTO_CTL    |= SCAL_LPF0_EN;

	        pSCAL->SCAL_DNSAMP_LPF_0_H	= SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_DNSAMP_LPF_0_V	= SCAL_DNSAMP_NONE;

	        pSCAL->SCAL_LPF0_EQ_CTL     = SCAL_LPF_BYPASS;
	        pSCAL->SCAL_LPF0_DN_CTL     = SCAL_LPF_DNSAMP_NONE;	
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLpfHStart 	= &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLpfHEnd   	= &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLpfVStart 	= &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLpfVEnd   	= &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart 	= &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd   	= &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart 	= &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd   	= &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprNh           	= &(pSCAL->SCAL_SCAL_1_H_N);
	        OprMh           	= &(pSCAL->SCAL_SCAL_1_H_M);
	        OprNv           	= &(pSCAL->SCAL_SCAL_1_V_N);
//	        OprMv           	= &(pSCAL->SCAL_SCAL_1_V_M);
	        OprHWeight      	= &(pSCAL->SCAL_SCAL_1_H_WT);
	        OprVWeight      	= &(pSCAL->SCAL_SCAL_1_V_WT);
			OprLpfAutoCtl		= &(pSCAL->SCAL_LPF_AUTO_CTL);

	        pSCAL->SCAL_LPF_AUTO_CTL   	|= SCAL_LPF1_EN;

	        pSCAL->SCAL_DNSAMP_LPF_1_H  = SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_DNSAMP_LPF_1_V  = SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_LPF1_DN_CTL     = SCAL_LPF_DNSAMP_NONE;
		break;
		case MMP_SCAL_PIPE_2:
			OprGrabLpfHStart 	= &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLpfHEnd   	= &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLpfVStart 	= &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLpfVEnd   	= &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart 	= &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd   	= &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart	 	= &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd   	= &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprNh           	= &(pSCAL->SCAL_SCAL_2_H_N);
	        OprMh           	= &(pSCAL->SCAL_SCAL_2_H_M);
	        OprNv           	= &(pSCAL->SCAL_SCAL_2_V_N);
//	        OprMv           	= &(pSCAL->SCAL_SCAL_2_V_M);
	        OprHWeight      	= &(pSCAL->SCAL_SCAL_2_H_WT);
	        OprVWeight      	= &(pSCAL->SCAL_SCAL_2_V_WT);
			OprLpfAutoCtl		= &(pSCAL->SCAL_LPF2_AUTO_CTL);

	        pSCAL->SCAL_LPF2_AUTO_CTL   |= SCAL_LPF2_EN;
            
            pSCAL->SCAL_DNSAMP_LPF_2_H  = SCAL_DNSAMP_NONE;
            pSCAL->SCAL_DNSAMP_LPF_2_V  = SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_LPF2_DN_CTL     = SCAL_LPF_DNSAMP_NONE;
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLpfHStart 	= &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLpfHEnd   	= &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLpfVStart 	= &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLpfVEnd   	= &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart 	= &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd   	= &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart 	= &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd   	= &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprNh           	= &(pSCAL->SCAL_SCAL_3_H_N);
	        OprMh           	= &(pSCAL->SCAL_SCAL_3_H_M);
	        OprNv           	= &(pSCAL->SCAL_SCAL_3_V_N);
//	        OprMv           	= &(pSCAL->SCAL_SCAL_3_V_M);
	        OprHWeight      	= &(pSCAL->SCAL_SCAL_3_H_WT);
	        OprVWeight      	= &(pSCAL->SCAL_SCAL_3_V_WT);
			OprLpfAutoCtl		= &(pSCAL->SCAL_LPF2_AUTO_CTL);

	        pSCAL->SCAL_LPF2_AUTO_CTL   |= SCAL_LPF3_EN;
            
            pSCAL->SCAL_DNSAMP_LPF_3_H  = SCAL_DNSAMP_NONE;
            pSCAL->SCAL_DNSAMP_LPF_3_V  = SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_LPF3_DN_CTL  	= SCAL_LPF_DNSAMP_NONE;
		break;
		case MMP_SCAL_PIPE_4:
	        OprGrabLpfHStart 	= &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLpfHEnd   	= &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLpfVStart 	= &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLpfVEnd   	= &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart 	= &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd   	= &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart 	= &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd   	= &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprNh           	= &(pSCAL->SCAL_SCAL_4_H_N);
	        OprMh           	= &(pSCAL->SCAL_SCAL_4_H_M);
	        OprNv           	= &(pSCAL->SCAL_SCAL_4_V_N);
//	        OprMv           	= &(pSCAL->SCAL_SCAL_4_V_M);
	        OprHWeight      	= &(pSCAL->SCAL_SCAL_4_H_WT);
	        OprVWeight      	= &(pSCAL->SCAL_SCAL_4_V_WT);
			OprLpfAutoCtl		= &(pSCAL->SCAL_LPF4_AUTO_CTL);

	        pSCAL->SCAL_LPF4_AUTO_CTL  	|= SCAL_LPF4_EN;
	        
	    	pSCAL->SCAL_DNSAMP_LPF_4_H 	= SCAL_DNSAMP_NONE;
            pSCAL->SCAL_DNSAMP_LPF_4_V 	= SCAL_DNSAMP_NONE;
	        pSCAL->SCAL_LPF4_DN_CTL  	= SCAL_LPF_DNSAMP_NONE;
		break;
		default:
		break;
	}
	
	if (fitrange->bUseLpfGrab) {
		*OprGrabLpfHStart 	= fitrange->ulInGrabX;
		*OprGrabLpfHEnd		= fitrange->ulInGrabX + fitrange->ulInWidth - 1;
		*OprGrabLpfVStart	= fitrange->ulInGrabY;
		*OprGrabLpfVEnd		= fitrange->ulInGrabY + fitrange->ulInHeight - 1;	
	}
	else {
		*OprGrabLpfHStart 	= 1;
		*OprGrabLpfHEnd		= fitrange->ulInWidth;
		*OprGrabLpfVStart	= 1;
		*OprGrabLpfVEnd		= fitrange->ulInHeight;
	}
	
	/* 2. Check the max delay line width */
    if (fitrange->ulInWidth > ulMaxScaleWidth) 
    {
    	switch(pipeID)
    	{
    		case MMP_SCAL_PIPE_0:
    			*OprLpfAutoCtl &= ~(SCAL_LPF0_EN);
    		break;
    		case MMP_SCAL_PIPE_1:
    			*OprLpfAutoCtl &= ~(SCAL_LPF1_EN);
    		break;
    		case MMP_SCAL_PIPE_2:
    			*OprLpfAutoCtl &= ~(SCAL_LPF2_EN);
    		break;
    		case MMP_SCAL_PIPE_3:
    			*OprLpfAutoCtl &= ~(SCAL_LPF3_EN);
    		break;
    		case MMP_SCAL_PIPE_4:
    			*OprLpfAutoCtl &= ~(SCAL_LPF4_EN);
    		break;
            default:
            //
            break;
    	}
        return MMP_ERR_NONE;
    }
	
	/* 3. Calculate LPF Down-Sample setting */
	if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {
		ulScaleXN = grabctl->ulScaleXN;
		ulScaleXM = grabctl->ulScaleXM;
		ulScaleYN = grabctl->ulScaleYN;
		ulScaleYM = grabctl->ulScaleYM;
	}
	else {
		ulScaleXN = grabctl->ulScaleN;
		ulScaleXM = grabctl->ulScaleM;
		ulScaleYN = grabctl->ulScaleN;
		ulScaleYM = grabctl->ulScaleM;	
	}
	
	if (2 * ulScaleXN > ulScaleXM) 		// MAX->1/2
	{
		ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_NONE;
	}
	else if (4 * ulScaleXN > ulScaleXM) // 1/2->1/4
	{
		if (2 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_NONE;
		else 
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_2;
	}
	else if (8 * ulScaleXN > ulScaleXM) // 1/4->1/8
	{
		if (2 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_NONE;
		else if (4 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_2;
		else
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_4;
	}	
	else 								// 1/8->MIN
	{
		if (2 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_NONE;
		else if (4 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_2;
		else if (8 * ulScaleYN > ulScaleYM)
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_4;
		else
			ubLpfDownSamp = MMP_SCAL_LPF_DNSAMP_1_8;
	}	
	
   	if (ubLpfDownSamp == MMP_SCAL_LPF_DNSAMP_NONE) 
   	{
        if (pipeID == MMP_SCAL_PIPE_0)
            pSCAL->SCAL_LPF0_EQ_CTL = SCAL_LPF_BYPASS;
	}
	else if (ubLpfDownSamp == MMP_SCAL_LPF_DNSAMP_1_2) 
	{
		if (fitrange->bUseLpfGrab) {
			*OprGrabLpfHStart 	= fitrange->ulInGrabX;
			*OprGrabLpfHEnd		= fitrange->ulInGrabX + ALIGN2(fitrange->ulInWidth) - 1;
			*OprGrabLpfVStart	= fitrange->ulInGrabY;
			*OprGrabLpfVEnd		= fitrange->ulInGrabY + ALIGN2(fitrange->ulInHeight) - 1;

			*OprGrabInHStart	= 1;
	        *OprGrabInHEnd    	= ALIGN2(*OprGrabInHEnd)    >> 1;
	        *OprGrabInVStart  	= 1;
	        *OprGrabInVEnd    	= ALIGN2(*OprGrabInVEnd)    >> 1;
        }
        else {
	       	*OprGrabInHStart	= ALIGN2(*OprGrabInHStart)  >> 1;
	        *OprGrabInHEnd    	= ALIGN2(*OprGrabInHEnd)    >> 1;
	        *OprGrabInVStart  	= ALIGN2(*OprGrabInVStart)  >> 1;
	        *OprGrabInVEnd    	= ALIGN2(*OprGrabInVEnd)    >> 1;
		}

        (*OprNh) = (*OprNh) << 1;
        (*OprNv) = (*OprNv) << 1;
		
		switch(pipeID)
		{
			case MMP_SCAL_PIPE_0:
            pSCAL->SCAL_LPF0_DN_CTL   = SCAL_LPF_DNSAMP_1_2 	| 
                                        SCAL_LPF_Y_L1_EN 		| 
                                        SCAL_LPF_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_1:
            pSCAL->SCAL_LPF1_DN_CTL   = SCAL_LPF123_DNSAMP_1_2 	| 
                                        SCAL_LPF123_Y_L1_EN 	| 
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_2:
            pSCAL->SCAL_LPF2_DN_CTL   = SCAL_LPF123_DNSAMP_1_2 	| 
                                        SCAL_LPF123_Y_L1_EN 	| 
                                        SCAL_LPF123_UV_L1_EN;
			break;				
			case MMP_SCAL_PIPE_3:
            pSCAL->SCAL_LPF3_DN_CTL   = SCAL_LPF123_DNSAMP_1_2 	| 
                                        SCAL_LPF123_Y_L1_EN 	| 
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_4:
            pSCAL->SCAL_LPF4_DN_CTL   = SCAL_LPF123_DNSAMP_1_2 	| 
                                        SCAL_LPF123_Y_L1_EN;
			break;
            default:
            //
            break;
		}
	}
	else if (ubLpfDownSamp == MMP_SCAL_LPF_DNSAMP_1_4) 
	{
		if (fitrange->bUseLpfGrab) {
			*OprGrabLpfHStart 	= fitrange->ulInGrabX;
			*OprGrabLpfHEnd		= fitrange->ulInGrabX + ALIGN4(fitrange->ulInWidth) - 1;
			*OprGrabLpfVStart	= fitrange->ulInGrabY;
			*OprGrabLpfVEnd		= fitrange->ulInGrabY + ALIGN4(fitrange->ulInHeight) - 1;

			*OprGrabInHStart	= 1;
	        *OprGrabInHEnd    	= ALIGN4(*OprGrabInHEnd)    >> 2;
	        *OprGrabInVStart  	= 1;
	        *OprGrabInVEnd   	= ALIGN4(*OprGrabInVEnd)    >> 2;
        }
        else {
	        *OprGrabInHStart  	= ALIGN4(*OprGrabInHStart)  >> 2;
	        *OprGrabInHEnd    	= ALIGN4(*OprGrabInHEnd)    >> 2;
	        *OprGrabInVStart  	= ALIGN4(*OprGrabInVStart)  >> 2;
	        *OprGrabInVEnd    	= ALIGN4(*OprGrabInVEnd)    >> 2;
		}

        (*OprNh) = (*OprNh) << 2;
        (*OprNv) = (*OprNv) << 2;
		
		switch(pipeID)
		{
			case MMP_SCAL_PIPE_0:
            pSCAL->SCAL_LPF0_DN_CTL   = SCAL_LPF_DNSAMP_1_4 	| 
                                    	SCAL_LPF_Y_L1_EN 		| 
                                       	SCAL_LPF_Y_L2_EN 		|
                                       	SCAL_LPF_UV_L1_EN		|
                                       	SCAL_LPF_UV_L2_EN;
			break;
			case MMP_SCAL_PIPE_1:
            pSCAL->SCAL_LPF1_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_2:
            pSCAL->SCAL_LPF2_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_3:
            pSCAL->SCAL_LPF3_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_4:
            pSCAL->SCAL_LPF4_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN;
			break;
            default:
            //
            break;
		}
	}
	else if (ubLpfDownSamp == MMP_SCAL_LPF_DNSAMP_1_8)
	{
		if (pipeID == MMP_SCAL_PIPE_0)
		{
			if (fitrange->bUseLpfGrab) {
				*OprGrabLpfHStart 	= fitrange->ulInGrabX;
				*OprGrabLpfHEnd		= fitrange->ulInGrabX + ALIGN8(fitrange->ulInWidth) - 1;
				*OprGrabLpfVStart	= fitrange->ulInGrabY;
				*OprGrabLpfVEnd		= fitrange->ulInGrabY + ALIGN8(fitrange->ulInHeight) - 1;

				*OprGrabInHStart	= 1;
	        	*OprGrabInHEnd    	= ALIGN8(*OprGrabInHEnd)    >> 3;
	        	*OprGrabInVStart  	= 1;
	        	*OprGrabInVEnd    	= ALIGN8(*OprGrabInVEnd)    >> 3;
       	 	}
       	 	else {
		        *OprGrabInHStart  	= ALIGN8(*OprGrabInHStart)  >> 3;
		        *OprGrabInHEnd    	= ALIGN8(*OprGrabInHEnd)    >> 3;
		        *OprGrabInVStart  	= ALIGN8(*OprGrabInVStart)  >> 3;
		        *OprGrabInVEnd    	= ALIGN8(*OprGrabInVEnd)    >> 3;
			}

	        (*OprNh) = (*OprNh) << 3;
	        (*OprNv) = (*OprNv) << 3;
		}
		else
		{
			if (fitrange->bUseLpfGrab) {
				*OprGrabLpfHStart 	= fitrange->ulInGrabX;
				*OprGrabLpfHEnd		= fitrange->ulInGrabX + ALIGN4(fitrange->ulInWidth) - 1;
				*OprGrabLpfVStart	= fitrange->ulInGrabY;
				*OprGrabLpfVEnd		= fitrange->ulInGrabY + ALIGN4(fitrange->ulInHeight) - 1;

				*OprGrabInHStart	= 1;
	        	*OprGrabInHEnd   	= ALIGN4(*OprGrabInHEnd)    >> 2;
	        	*OprGrabInVStart  	= 1;
	        	*OprGrabInVEnd    	= ALIGN4(*OprGrabInVEnd)    >> 2;
       	 	}
       	 	else {
		        *OprGrabInHStart  	= ALIGN4(*OprGrabInHStart)  >> 2;
		        *OprGrabInHEnd    	= ALIGN4(*OprGrabInHEnd)    >> 2;
		        *OprGrabInVStart  	= ALIGN4(*OprGrabInVStart)  >> 2;
		        *OprGrabInVEnd    	= ALIGN4(*OprGrabInVEnd)    >> 2;
			}

	        (*OprNh) = (*OprNh) << 2;
	        (*OprNv) = (*OprNv) << 2;
		}
	
		switch(pipeID)
		{
			case MMP_SCAL_PIPE_0:
            pSCAL->SCAL_LPF0_DN_CTL	  = SCAL_LPF_DNSAMP_1_8 	| 
                                    	SCAL_LPF_Y_L1_EN 		| 
                                       	SCAL_LPF_Y_L2_EN 		|
                                       	SCAL_LPF_Y_L3_EN 		|
                                       	SCAL_LPF_UV_L1_EN		|
                                       	SCAL_LPF_UV_L2_EN;
			break;
			case MMP_SCAL_PIPE_1:
            pSCAL->SCAL_LPF1_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_2:
            pSCAL->SCAL_LPF2_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_3:
            pSCAL->SCAL_LPF3_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN 	|
                                        SCAL_LPF123_UV_L1_EN;
			break;
			case MMP_SCAL_PIPE_4:
            pSCAL->SCAL_LPF4_DN_CTL   = SCAL_LPF123_DNSAMP_1_4 	|
                                        SCAL_LPF123_Y_L1_EN 	|
                                        SCAL_LPF123_Y_L2_EN;
			break;
            default:
            //
            break;
		}
	}

    if ((*OprNh)*5 < (*OprMh)*3) {
        *OprHWeight = (*OprNh) >> 1;
        *OprVWeight = (*OprNv) >> 1;
    }
    else {
        *OprHWeight = 0;
        *OprVWeight = 0;
    }

	/* 4. Decide the LPF enable or not */
	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
			if ((pSCAL->SCAL_LPF0_EQ_CTL) || (pSCAL->SCAL_LPF0_DN_CTL))
				*OprLpfAutoCtl |= SCAL_LPF0_EN;
			else
				*OprLpfAutoCtl &= ~(SCAL_LPF0_EN);
		break;
		case MMP_SCAL_PIPE_1:
			if (pSCAL->SCAL_LPF1_DN_CTL)
				*OprLpfAutoCtl |= SCAL_LPF1_EN;
			else
				*OprLpfAutoCtl &= ~(SCAL_LPF1_EN);
		break;
		case MMP_SCAL_PIPE_2:
			if (pSCAL->SCAL_LPF2_DN_CTL)
				*OprLpfAutoCtl |= SCAL_LPF2_EN;
			else
				*OprLpfAutoCtl &= ~(SCAL_LPF2_EN);
		break;
		case MMP_SCAL_PIPE_3:
			if (pSCAL->SCAL_LPF3_DN_CTL)
				*OprLpfAutoCtl |= SCAL_LPF3_EN;
			else
				*OprLpfAutoCtl &= ~(SCAL_LPF3_EN);
		break;
		case MMP_SCAL_PIPE_4:
			if (pSCAL->SCAL_LPF4_DN_CTL)
				*OprLpfAutoCtl |= SCAL_LPF4_EN;
			else
				*OprLpfAutoCtl &= ~(SCAL_LPF4_EN);
		break;
        default:
        //
        break;
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetOutputFormat
//  Description : The function set scaler output color format.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler output color format.
 * 
 *  The function set scaler output color format.
 * @param[in] pipeID  : stands for scaler pipe.
 * @param[in] outcolor : stands for output color format.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetOutputFormat(MMP_SCAL_PIPEID pipeID, MMP_SCAL_COLORMODE outcolor)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;
    AIT_REG_B  *pOutCtlOpr = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
			pOutCtlOpr = &(pSCAL->SCAL_OUT_0_CTL);
		break;
		case MMP_SCAL_PIPE_1:
			pOutCtlOpr = &(pSCAL->SCAL_OUT_1_CTL);
		break;
		case MMP_SCAL_PIPE_2:
			pOutCtlOpr = &(pSCAL->SCAL_OUT_2_CTL);
		break;
		case MMP_SCAL_PIPE_3:
			pOutCtlOpr = &(pSCAL->SCAL_OUT_3_CTL);
		break;
		case MMP_SCAL_PIPE_4:
			return MMP_ERR_NONE;
		break;
        default:
        //
        break;
	}

    (*pOutCtlOpr) &= ~(SCAL_OUT_MASK);

    switch (outcolor)
    {
	case MMP_SCAL_COLOR_YUV422:
	    (*pOutCtlOpr) |= (SCAL_OUT_YUV_EN | SCAL_OUT_FMT_YUV422);
        break;
    case MMP_SCAL_COLOR_RGB565:
        (*pOutCtlOpr) |= (SCAL_OUT_RGB_EN | SCAL_OUT_FMT_RGB565 | SCAL_OUT_DITHER_RGB565 | SCAL_OUT_RGB_ORDER);
        break;
    case MMP_SCAL_COLOR_RGB888:
    	(*pOutCtlOpr) |= (SCAL_OUT_RGB_EN | SCAL_OUT_FMT_RGB888 | SCAL_OUT_RGB_ORDER);
        break;
    case MMP_SCAL_COLOR_YUV444:
    	(*pOutCtlOpr) |= (SCAL_OUT_YUV_EN | SCAL_OUT_FMT_YUV444);
        break;
    default:
        return MMP_SCALER_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetOutColorTransform
//  Description : The function set scaler output color transform matrix.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler output color transform matrix.
 * 
 *  The function set scaler output color transform matrix.
 * @param[in] pipeID    : stands for scaler pipe.
 * @param[in] bEnable    : stands for enable transform.
 * @param[in] MatrixMode : stands for color matrix mode.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetOutColorTransform(MMP_SCAL_PIPEID pipeID, MMP_BOOL bEnable, MMP_SCAL_COLRMTX_MODE MatrixMode)
{
    AITPS_SCAL 	pSCAL = AITC_BASE_SCAL;
    volatile AITS_SCAL_MTX          *pMtxOpr = NULL;
    volatile AITS_SCAL_COLR_CLIP    *pClipOpr = NULL;
    AIT_REG_W 	*pMtxYOffset = NULL, *pMtxUOffset = NULL, *pMtxVOffset = NULL;
    MMP_ULONG   i;
    MMP_LONG    s_coeff;
    MMP_SHORT   *pOffset;
    MMP_SHORT   (*pMatrix)[3];
    MMP_SHORT   (*pClip)[2];

    if (pipeID == MMP_SCAL_PIPE_0) {
        pMtxOpr  	= &(pSCAL->SCAL_P0_CT_MTX);
        pClipOpr 	= &(pSCAL->SCAL_P0_MTX_CLIP);
        pMtxYOffset = &(pSCAL->SCAL_P0_Y_OFST);
        pMtxUOffset = &(pSCAL->SCAL_P0_U_OFST);
        pMtxVOffset = &(pSCAL->SCAL_P0_V_OFST);
    }
    else if (pipeID == MMP_SCAL_PIPE_1) {
        pMtxOpr  	= &(pSCAL->SCAL_P1_CT_MTX);
        pClipOpr 	= &(pSCAL->SCAL_P1_MTX_CLIP);
        pMtxYOffset = &(pSCAL->SCAL_P1_Y_OFST);
        pMtxUOffset = &(pSCAL->SCAL_P1_U_OFST);
        pMtxVOffset = &(pSCAL->SCAL_P1_V_OFST);
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        pMtxOpr 	= &(pSCAL->SCAL_P2_CT_MTX);
        pClipOpr 	= &(pSCAL->SCAL_P2_MTX_CLIP);
        pMtxYOffset = &(pSCAL->SCAL_P2_Y_OFST);
        pMtxUOffset = &(pSCAL->SCAL_P2_U_OFST);
        pMtxVOffset = &(pSCAL->SCAL_P2_V_OFST);
    }
    else if (pipeID == MMP_SCAL_PIPE_3) {
        pMtxOpr 	= &(pSCAL->SCAL_P3_CT_MTX);
        pClipOpr 	= &(pSCAL->SCAL_P3_MTX_CLIP);
        pMtxYOffset = &(pSCAL->SCAL_P3_Y_OFST);
        pMtxUOffset = &(pSCAL->SCAL_P3_U_OFST);
        pMtxVOffset = &(pSCAL->SCAL_P3_V_OFST);
    }
    else if (pipeID == MMP_SCAL_PIPE_4) {
    	return MMP_ERR_NONE;
    }

	m_ubScalerOutColorMode[pipeID] = MatrixMode;

    if (bEnable) 
    {
        switch (MatrixMode) 
        {
        case MMP_SCAL_COLRMTX_YUV_FULLRANGE:
            pMtxOpr->COLRMTX_CTL &= ~(SCAL_COLRMTX_EN);
            return MMP_ERR_NONE;
            break;
        case MMP_SCAL_COLRMTX_FULLRANGE_TO_RGB:
        case MMP_SCAL_COLRMTX_FULLRANGE_TO_BT601:
        case MMP_SCAL_COLRMTX_BT601_TO_FULLRANGE:
            if (MatrixMode == MMP_SCAL_COLRMTX_FULLRANGE_TO_BT601) {
                pOffset = m_ScalerColorOffset_BT601;
                pMatrix = m_ScalerColorMatrix_BT601;
                pClip   = m_ScalerColorClip_BT601;
            }
            else if (MatrixMode == MMP_SCAL_COLRMTX_BT601_TO_FULLRANGE) {
                pOffset = m_ScalerColorOffset_BT601_FullRange;
                pMatrix = m_ScalerColorMatrix_BT601_FullRange;
                pClip   = m_ScalerColorClip_BT601_FullRange;
            }
            else {
                pOffset = m_ScalerColorOffset_RGB;
                pMatrix = m_ScalerColorMatrix_RGB;
                pClip   = m_ScalerColorClip_RGB;
            }
            pMtxOpr->MTX_COEFF_ROW1_MSB &= ~(SCAL_MTX_COL_COEFF_MSB_MASK);
            pMtxOpr->MTX_COEFF_ROW2_MSB &= ~(SCAL_MTX_COL_COEFF_MSB_MASK);
            pMtxOpr->MTX_COEFF_ROW3_MSB &= ~(SCAL_MTX_COL_COEFF_MSB_MASK);

            for (i = 0; i < 3; i++) {
                s_coeff = SCAL_MTX_2S_COMPLEMENT(pMatrix[0][i]);
                pMtxOpr->MTX_COEFF_ROW1[i] = (s_coeff & 0xFF);
                pMtxOpr->MTX_COEFF_ROW1_MSB |= (SCAL_MTX_COL_COEFF_MSB(s_coeff, i));

                s_coeff = SCAL_MTX_2S_COMPLEMENT(pMatrix[1][i]);
                pMtxOpr->MTX_COEFF_ROW2[i] = (s_coeff & 0xFF);
                pMtxOpr->MTX_COEFF_ROW2_MSB |= (SCAL_MTX_COL_COEFF_MSB(s_coeff, i));

                s_coeff = SCAL_MTX_2S_COMPLEMENT(pMatrix[2][i]);
                pMtxOpr->MTX_COEFF_ROW3[i] = (s_coeff & 0xFF);
                pMtxOpr->MTX_COEFF_ROW3_MSB |= (SCAL_MTX_COL_COEFF_MSB(s_coeff, i));
            }

            s_coeff = SCAL_YUV_OFST_2S_COMPLEMENT(pOffset[0]);
            *pMtxYOffset = s_coeff & SCAL_YUV_OFST_MASK;

            s_coeff = SCAL_YUV_OFST_2S_COMPLEMENT(pOffset[1]);
            *pMtxUOffset = s_coeff & SCAL_YUV_OFST_MASK;

            s_coeff = SCAL_YUV_OFST_2S_COMPLEMENT(pOffset[2]);
            *pMtxVOffset = s_coeff & SCAL_YUV_OFST_MASK;

            pClipOpr->OUT_Y_MIN = pClip[0][0] & 0xFF;
            pClipOpr->OUT_Y_MAX = pClip[0][1] & 0xFF;
            pClipOpr->OUT_U_MIN = pClip[1][0] & 0xFF;
            pClipOpr->OUT_U_MAX = pClip[1][1] & 0xFF;
            pClipOpr->OUT_V_MIN = pClip[2][0] & 0xFF;
            pClipOpr->OUT_V_MAX = pClip[2][1] & 0xFF;
            pClipOpr->OUT_YUV_MSB =
                                SCAL_CLIP_MSB(pClip[0][0], 0) |
                                SCAL_CLIP_MSB(pClip[0][1], 1) |
                                SCAL_CLIP_MSB(pClip[1][0], 2) |
                                SCAL_CLIP_MSB(pClip[1][1], 3) |
                                SCAL_CLIP_MSB(pClip[2][0], 4) |
                                SCAL_CLIP_MSB(pClip[2][1], 5);
            break;
        default:
            return MMP_SCALER_ERR_PARAMETER;
        }
        pMtxOpr->COLRMTX_CTL |= SCAL_COLRMTX_EN;
    }
    else {
        pMtxOpr->COLRMTX_CTL &= ~(SCAL_COLRMTX_EN);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetEnable
//  Description : The function enable scaler pipe.
//------------------------------------------------------------------------------
/** 
 * @brief The function enable scaler pipe.
 * 
 *  The function enable scaler pipe.
 * @param[in] pipeID  : stands for scaler pipe.
 * @param[in] bEnable : stands for enable scaler path.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetEnable(MMP_SCAL_PIPEID pipeID, MMP_BOOL bEnable)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    AIT_REG_B   *OprScalCtl = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
        	OprScalCtl = &(pSCAL->SCAL_SCAL_0_CTL);
		break;
		case MMP_SCAL_PIPE_1:
       	 	OprScalCtl = &(pSCAL->SCAL_SCAL_1_CTL);
		break;
		case MMP_SCAL_PIPE_2:
        	OprScalCtl = &(pSCAL->SCAL_SCAL_2_CTL);
		break;
		case MMP_SCAL_PIPE_3:
        	OprScalCtl = &(pSCAL->SCAL_SCAL_3_CTL);
		break;
		case MMP_SCAL_PIPE_4:
        	OprScalCtl = &(pSCAL->SCAL_SCAL_4_CTL);
		break;
        default:
        //
        break;
	}

    if (bEnable == MMP_TRUE) {
        (*OprScalCtl) |= (SCAL_SCAL_PATH_EN);
    }
    else {
        (*OprScalCtl) &= ~(SCAL_SCAL_PATH_EN);
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetEnable
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetEnable(MMP_SCAL_PIPEID pipeID, MMP_BOOL* bEnable)
{
	AITPS_SCAL pSCAL = AITC_BASE_SCAL;

	switch(pipeID)
	{
	    case MMP_SCAL_PIPE_0:
            if (pSCAL->SCAL_SCAL_0_CTL & SCAL_SCAL_PATH_EN)
        	    *bEnable = MMP_TRUE;
            else
        	    *bEnable = MMP_FALSE;
	    break;
	    case MMP_SCAL_PIPE_1:
            if (pSCAL->SCAL_SCAL_1_CTL & SCAL_SCAL_PATH_EN)
        	    *bEnable = MMP_TRUE;
            else
        	    *bEnable = MMP_FALSE;
	    break;	
	    case MMP_SCAL_PIPE_2:
            if (pSCAL->SCAL_SCAL_2_CTL & SCAL_SCAL_PATH_EN)
        	    *bEnable = MMP_TRUE;
            else
        	    *bEnable = MMP_FALSE;
	    break;
	    case MMP_SCAL_PIPE_3:
            if (pSCAL->SCAL_SCAL_3_CTL & SCAL_SCAL_PATH_EN)
        	    *bEnable = MMP_TRUE;
            else
        	    *bEnable = MMP_FALSE;
	    break;	
	    case MMP_SCAL_PIPE_4:
            if (pSCAL->SCAL_SCAL_4_CTL & SCAL_SCAL_PATH_EN)
        	    *bEnable = MMP_TRUE;
            else
        	    *bEnable = MMP_FALSE;
	    break;
        default:
        //
        break;
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetPixelDelay
//  Description : The function set scaler pixel and line delay.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler pixel and line delay.
 * 
 *  The function set scaler pixel delay.Pixel and line delay only useful when scaling up
 *  For MCR_V2  : It means M clock cycle output N pixel. 
 * @param[in] pipeID       : stands for scaler pipe.
 * @param[in] ubPixelDelayN : stands for pixel delay ratio N.
 * @param[in] ubPixelDelayM : stands for pixel delay divider M. (pixel delay = N/M)
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetPixelDelay(MMP_SCAL_PIPEID pipeID, MMP_UBYTE ubPixelDelayN, MMP_UBYTE ubPixelDelayM)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;

    if (ubPixelDelayN > ubPixelDelayM) {
        RTNA_DBG_Str(3, "N must less/equal to M\r\n");
        return MMP_ERR_NONE;
    }

    if (pipeID == MMP_SCAL_PIPE_0) {
        pSCAL->SCAL_P0_PIXL_DELAY_N  = ubPixelDelayN;
        pSCAL->SCAL_P0_PIXL_DELAY_M  = ubPixelDelayM;
    }
    else if (pipeID == MMP_SCAL_PIPE_1) {
        pSCAL->SCAL_P1_PIXL_DELAY_N  = ubPixelDelayN;
        pSCAL->SCAL_P1_PIXL_DELAY_M  = ubPixelDelayM;
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        pSCAL->SCAL_P2_PIXL_DELAY_N  = ubPixelDelayN;
        pSCAL->SCAL_P2_PIXL_DELAY_M  = ubPixelDelayM;
    } 
    else if (pipeID == MMP_SCAL_PIPE_3) {
        pSCAL->SCAL_P3_PIXL_DELAY_N  = ubPixelDelayN;
        pSCAL->SCAL_P3_PIXL_DELAY_M  = ubPixelDelayM;
    } 
    else if (pipeID == MMP_SCAL_PIPE_4) {
        pSCAL->SCAL_P4_PIXL_DELAY_N  = ubPixelDelayN;
        pSCAL->SCAL_P4_PIXL_DELAY_M  = ubPixelDelayM;
    } 

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetLineDelay
//  Description : The function set scaler pixel and line delay.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler line delay.
 * 
 *  The function set scaler line delay. Pixel and line delay only useful when scaling up
 * @param[in] pipeID       : stands for scaler pipe.
 * @param[in] ubLineDelay   : stands for line delay. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetLineDelay(MMP_SCAL_PIPEID pipeID, MMP_UBYTE ubLineDelay)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;

    if (pipeID == MMP_SCAL_PIPE_0) {
        pSCAL->SCAL_P0_LINE_DELAY = ubLineDelay;
    }
    else if (pipeID == MMP_SCAL_PIPE_1) {
        pSCAL->SCAL_P1_LINE_DELAY = ubLineDelay;
    }
    else if (pipeID == MMP_SCAL_PIPE_2) {
        pSCAL->SCAL_P2_LINE_DELAY = ubLineDelay;
    } 
    else if (pipeID == MMP_SCAL_PIPE_3) {
        pSCAL->SCAL_P3_LINE_DELAY = ubLineDelay;
    } 
    else if (pipeID == MMP_SCAL_PIPE_4) {
        pSCAL->SCAL_P4_LINE_DELAY = ubLineDelay;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetEqDownModeDelay
//  Description : The function set scaler output delay enable.
//------------------------------------------------------------------------------
/** 
 * @brief The function set output delay enable.
 * 
 *  The function set scaler output delay enable.
 * @param[in] pipeID   : stands for scaler pipe.
 * @param[in] ubEnable : stands for enable the delay.
 * @param[in] usThd    : stands for the blanking threshold.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetEqDownModeDelay(MMP_SCAL_PIPEID pipeID, MMP_BOOL ubEnable, MMP_USHORT usThd)
{
    AITPS_SCAL 	pSCAL = AITC_BASE_SCAL;
    AIT_REG_B   *OprOutBlnkCtl = NULL;
	AIT_REG_W   *OprOutBlnkThd = NULL;
	
	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
        	OprOutBlnkCtl = &(pSCAL->SCAL_OUT_BLKNG_0_EN);
        	OprOutBlnkThd = &(pSCAL->SCAL_OUT_BLKNG_0_TH);
		break;
		case MMP_SCAL_PIPE_1:
       	 	OprOutBlnkCtl = &(pSCAL->SCAL_OUT_BLKNG_1_EN);
       	 	OprOutBlnkThd = &(pSCAL->SCAL_OUT_BLKNG_1_TH);
		break;
		case MMP_SCAL_PIPE_2:
        	OprOutBlnkCtl = &(pSCAL->SCAL_OUT_BLKNG_2_EN);
        	OprOutBlnkThd = &(pSCAL->SCAL_OUT_BLKNG_2_TH);
		break;		
		case MMP_SCAL_PIPE_3:
        	OprOutBlnkCtl = &(pSCAL->SCAL_OUT_BLKNG_3_EN);
        	OprOutBlnkThd = &(pSCAL->SCAL_OUT_BLKNG_3_TH);
		break;
		case MMP_SCAL_PIPE_4:
        	OprOutBlnkCtl = &(pSCAL->SCAL_OUT_BLKNG_4_EN);
        	OprOutBlnkThd = &(pSCAL->SCAL_OUT_BLKNG_4_TH);
		break;
        default:
        //
        break;
	}

	if (ubEnable) {
		*OprOutBlnkCtl |= SCAL_OUT_BLKNG_FOR_EQ_DN;
	}
	else {
		*OprOutBlnkCtl &= ~(SCAL_OUT_BLKNG_FOR_EQ_DN);
	}
		
	if (usThd != 0xFFFF) {
		*OprOutBlnkThd = usThd;
	} 
        
	return MMP_ERR_NONE;
}


#if (SENSOR_EN)
//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SelectSyncSrc
//  Description : The function set scaler input mux frame sync source.
//------------------------------------------------------------------------------
static MMP_UBYTE MMPF_Scaler_SelectSyncSrc(MMP_SCAL_SOURCE source)
{
	MMP_UBYTE ubSyncSrc = 0;

	switch (source)
	{
		case MMP_SCAL_SOURCE_ISP:
			ubSyncSrc |= SCAL_IN_MUX_SYNC_BY_ISP_FRM_ST;
		break;	
		case MMP_SCAL_SOURCE_JPG:
			ubSyncSrc |= SCAL_IN_MUX_SYNC_BY_JPG_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_GRA:
			ubSyncSrc |= SCAL_IN_MUX_SYNC_BY_GRA_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_LDC:
			ubSyncSrc |= SCAL_IN_MUX_SYNC_BY_LDC_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_YUV:
			ubSyncSrc |= SCAL_IN_MUX_SYNC_BY_YUV_FRM_ST;
		break;
        default:
        //
        break;
	}
	
	return ubSyncSrc;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SelectP4SyncSrc
//  Description : The function set scaler input mux frame sync source.
//------------------------------------------------------------------------------
static MMP_UBYTE MMPF_Scaler_SelectP4SyncSrc(MMP_SCAL_SOURCE source)
{
	MMP_UBYTE ubSyncSrc = 0;

	switch (source)
	{
		case MMP_SCAL_SOURCE_ISP:
			ubSyncSrc |= SCAL_P4_IN_MUX_SYNC_BY_ISP_FRM_ST;
		break;	
		case MMP_SCAL_SOURCE_JPG:
			ubSyncSrc |= SCAL_P4_IN_MUX_SYNC_BY_JPG_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_GRA:
			ubSyncSrc |= SCAL_P4_IN_MUX_SYNC_BY_GRA_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_LDC:
			ubSyncSrc |= SCAL_P4_IN_MUX_SYNC_BY_LDC_FRM_ST;
		break;
		case MMP_SCAL_SOURCE_YUV:
			ubSyncSrc |= SCAL_P4_IN_MUX_SYNC_BY_YUV_FRM_ST;
		break;
        default:
        //
        break;
	}	

	return ubSyncSrc;
}

#endif
//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetInputMuxAttr
//  Description : The function set scaler input mux frame sync attribute.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler input mux frame sync attribute.
 * 
 *  The function set scaler input mux frame sync attribute.
 * @param[in] pipeID  : stands for scaler pipe.
 * @param[in] pAttr   : stands for input mux attribute.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_SetInputMuxAttr(MMP_SCAL_PIPEID pipeID, MMP_SCAL_IN_MUX_ATTR* pAttr)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    /* __packed AIT_REG_B *OprInMuxCtl; */
    /* __packed AIT_REG_B *OprInFrmSyncCtl; */
    AIT_REG_B *OprInMuxCtl = NULL;
    AIT_REG_B *OprInFrmSyncCtl = NULL;
    
    switch(pipeID)
    {
        case MMP_SCAL_PIPE_0:
            OprInMuxCtl 	= &(pSCAL->SCAL_P0_IN_MUX_CTL);
            OprInFrmSyncCtl	= &(pSCAL->SCAL_P0_IN_FRM_SYNC_CTL);
        break;
        case MMP_SCAL_PIPE_1:
            OprInMuxCtl 	= &(pSCAL->SCAL_P1_IN_MUX_CTL);
            OprInFrmSyncCtl	= &(pSCAL->SCAL_P1_IN_FRM_SYNC_CTL);
        break;
        case MMP_SCAL_PIPE_2:
            OprInMuxCtl 	= &(pSCAL->SCAL_P2_IN_MUX_CTL);
            OprInFrmSyncCtl	= &(pSCAL->SCAL_P2_IN_FRM_SYNC_CTL);
        break;
        case MMP_SCAL_PIPE_3:
            OprInMuxCtl 	= &(pSCAL->SCAL_P3_IN_MUX_CTL);
            OprInFrmSyncCtl	= &(pSCAL->SCAL_P3_IN_FRM_SYNC_CTL);
        break;
        case MMP_SCAL_PIPE_4:
            OprInMuxCtl 	= &(pSCAL->SCAL_P4_IN_FRM_SYNC_CTL);
            OprInFrmSyncCtl	= &(pSCAL->SCAL_P4_IN_FRM_SYNC_CTL);
        break;
        default:
        //
        break;
    }
    
    if(pAttr != NULL)
    {
    	if (pipeID <= MMP_SCAL_PIPE_3) {
    	
	        if (pAttr->bFrmSyncEn == MMP_TRUE)
	            *OprInFrmSyncCtl |= SCAL_IN_MUX_FRM_SYNC_EN;
	        else
	            *OprInFrmSyncCtl &= ~(SCAL_IN_MUX_FRM_SYNC_EN);
	            
	        if (pAttr->ubFrmSyncSel != 0) {
	        	*OprInMuxCtl &= ~(SCAL_IN_MUX_SYNC_CHK_MASK);
	            *OprInMuxCtl |= (pAttr->ubFrmSyncSel & SCAL_IN_MUX_SYNC_CHK_MASK);
	    	}
    	}
    	else {

	        if (pAttr->bFrmSyncEn == MMP_TRUE)
	            *OprInFrmSyncCtl |= SCAL_P4_IN_MUX_FRM_SYNC_EN;
	        else
	            *OprInFrmSyncCtl &= ~(SCAL_P4_IN_MUX_FRM_SYNC_EN);
	            
	        if (pAttr->ubFrmSyncSel != 0) {
	        	*OprInMuxCtl &= ~(SCAL_P4_IN_MUX_SYNC_CHK_MASK);
	            *OprInMuxCtl |= (pAttr->ubFrmSyncSel & SCAL_P4_IN_MUX_SYNC_CHK_MASK);
	    	}
    	} 
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetInputSrc
//  Description : The function set scaler input source moudle.
//------------------------------------------------------------------------------
/** 
 * @brief The function set scaler input source moudle.
 * 
 *  The function set scaler input source moudle.
 * @param[in] pipeID : stands for scaler pipe.
 * @param[in] source  : stands for scaler source.
 * @return It return the function status.  
 */
MMP_UBYTE MMPF_Scaler_SetInputSrc(MMP_SCAL_PIPEID pipeID, MMP_SCAL_SOURCE source, MMP_BOOL bUpdate)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    MMP_UBYTE   ctl_reg = 0;

	if (pipeID >= MMP_SCAL_PIPE_NUM) {
		return 0;
	}
     
    switch (source) 
    {
    case MMP_SCAL_SOURCE_ISP:
        ctl_reg = SCAL_ISP_2_SCAL;
        break;
     case MMP_SCAL_SOURCE_JPG:
        ctl_reg = SCAL_JPG_2_SCAL;
        break;
    case MMP_SCAL_SOURCE_GRA:
        ctl_reg = SCAL_GRA_2_SCAL;
        break;
    case MMP_SCAL_SOURCE_LDC:
        ctl_reg = SCAL_LDC_2_SCAL;
        break;
    case MMP_SCAL_SOURCE_P0:
        if (pipeID == MMP_SCAL_PIPE_0) {
            return 0;
        }
        else if(pipeID == MMP_SCAL_PIPE_1) {
            ctl_reg = SCAL_P0_2_P1;
        }
        else if(pipeID == MMP_SCAL_PIPE_2) {
            ctl_reg = SCAL_P0_2_P2;
        }
		else if(pipeID == MMP_SCAL_PIPE_3) {
            ctl_reg = SCAL_P0_2_P3;
        }
        break;
    case MMP_SCAL_SOURCE_P1:
        if (pipeID == MMP_SCAL_PIPE_0) {
            ctl_reg = SCAL_P1_2_P0;
        }
        else if(pipeID == MMP_SCAL_PIPE_1) {
            return 0;
        }
        else if(pipeID == MMP_SCAL_PIPE_2) {
            ctl_reg = SCAL_P1_2_P2;
        }
		else if(pipeID == MMP_SCAL_PIPE_3) {
            ctl_reg = SCAL_P1_2_P3;
        }
        break;
    case MMP_SCAL_SOURCE_P2:
        if (pipeID == MMP_SCAL_PIPE_0) {
            ctl_reg = SCAL_P2_2_P0;
        }
        else if(pipeID == MMP_SCAL_PIPE_1) {
            ctl_reg = SCAL_P2_2_P1;
        }
        else if(pipeID == MMP_SCAL_PIPE_2) {
            return 0;
        }
		else if(pipeID == MMP_SCAL_PIPE_3) {
            ctl_reg = SCAL_P2_2_P3;
        }
        break;   
    case MMP_SCAL_SOURCE_P3:
        if (pipeID == MMP_SCAL_PIPE_0) {
            ctl_reg = SCAL_P3_2_P0;
        }
        else if(pipeID == MMP_SCAL_PIPE_1) {
            ctl_reg = SCAL_P3_2_P1;
        }
        else if(pipeID == MMP_SCAL_PIPE_2) {
            ctl_reg = SCAL_P3_2_P2;
        }
		else if(pipeID == MMP_SCAL_PIPE_3) {
            return 0;
        }
        break;
    default:
    //
    break;
    }

    if (bUpdate)
    {
        switch (pipeID)
        {
            case MMP_SCAL_PIPE_0:
                pSCAL->SCAL_PATH_0_CTL = ctl_reg;
            break;
            case MMP_SCAL_PIPE_1:
                pSCAL->SCAL_PATH_1_CTL = ctl_reg;
            break;
            case MMP_SCAL_PIPE_2:
                pSCAL->SCAL_PATH_2_CTL = ctl_reg;
            break;
            case MMP_SCAL_PIPE_3:
                pSCAL->SCAL_PATH_3_CTL = ctl_reg;
            break;
            case MMP_SCAL_PIPE_4:
                pSCAL->SCAL_PATH_4_CTL = ctl_reg;
            break;
            default:
            //
            break;
        }
    }

    return ctl_reg;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetInputSrc
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetInputSrc(MMP_SCAL_PIPEID pipeID, MMP_SCAL_SOURCE* source)
{
    AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    MMP_UBYTE   ubInputReg = 0;

    switch(pipeID)
    {
        case MMP_SCAL_PIPE_0:
            ubInputReg = pSCAL->SCAL_PATH_0_CTL;
        break;
        case MMP_SCAL_PIPE_1:
            ubInputReg = pSCAL->SCAL_PATH_1_CTL;
        break;
        case MMP_SCAL_PIPE_2:
            ubInputReg = pSCAL->SCAL_PATH_2_CTL;
        break;
        case MMP_SCAL_PIPE_3:
            ubInputReg = pSCAL->SCAL_PATH_3_CTL;
        break;        
        case MMP_SCAL_PIPE_4:
            ubInputReg = pSCAL->SCAL_PATH_4_CTL;
        break; 
        default:
        //
        break;
    }

    if (ubInputReg & SCAL_JPG_2_SCAL)
        *source = MMP_SCAL_SOURCE_JPG;
    else if (ubInputReg & SCAL_GRA_2_SCAL)
        *source = MMP_SCAL_SOURCE_GRA;
    else if (ubInputReg & SCAL_LDC_2_SCAL)
        *source = MMP_SCAL_SOURCE_LDC;
    else 
        *source = MMP_SCAL_SOURCE_ISP;
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetPath
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_SetPath(MMP_SCAL_PIPEID pipeID, MMP_SCAL_SOURCE NewSource)
{
    MMP_SCAL_SOURCE		PreSource;
    #if (SENSOR_EN)
    AITPS_SCAL          pSCAL	= AITC_BASE_SCAL;
    //AITPS_ISP	        pISP    = AITC_BASE_ISP;
    #endif
	
	MMPF_Scaler_GetInputSrc(pipeID, &PreSource);
	
	if (NewSource == PreSource) {
		RTNA_DBG_Str(3, "Scaler has the same input\r\n");
		return MMP_ERR_NONE;
	}
    
    m_ubScalerInput[pipeID] = MMPF_Scaler_SetInputSrc(pipeID, NewSource, MMP_FALSE);
    
	#if (SENSOR_EN)
	{
		MMP_UBYTE				ubSyncSrc = 0;
		MMP_SCAL_IN_MUX_ATTR 	muxAttr;
		
		if (pipeID <= MMP_SCAL_PIPE_3) {
			ubSyncSrc  = MMPF_Scaler_SelectSyncSrc(PreSource);
			ubSyncSrc |= MMPF_Scaler_SelectSyncSrc(NewSource);
		}
		else {
			ubSyncSrc  = MMPF_Scaler_SelectP4SyncSrc(PreSource);
			ubSyncSrc |= MMPF_Scaler_SelectP4SyncSrc(NewSource);
		}

		muxAttr.bFrmSyncEn   = MMP_TRUE;
		muxAttr.ubFrmSyncSel = ubSyncSrc;
		
		MMPF_Scaler_SetInputMuxAttr(pipeID, &muxAttr);
	}

	if (PreSource == MMP_SCAL_SOURCE_ISP)
	{
		if (0/*pISP->ISP_MISC_CTL & ISP_RAW_MODE_EN*/) {
		
			MMPF_RAWPROC_WaitFetchDone();
			MMPF_Scaler_ChangePath(pipeID);
		}
		else {
		    /* Note: YUV420 Sensor has no ISP frame end interrupt */
			MMPF_Scaler_ChangePath(pipeID);
		}
	}
	else
	{	
		MMPF_Scaler_ChangePath(pipeID);
	}
	
	/* Wait the input path change done */
	if (pipeID == MMP_SCAL_PIPE_0)
		while(pSCAL->SCAL_PATH_0_CTL != m_ubScalerInput[pipeID]);
	else if (pipeID == MMP_SCAL_PIPE_1)
		while(pSCAL->SCAL_PATH_1_CTL != m_ubScalerInput[pipeID]);
	else if (pipeID == MMP_SCAL_PIPE_2)
		while(pSCAL->SCAL_PATH_2_CTL != m_ubScalerInput[pipeID]);
	else if (pipeID == MMP_SCAL_PIPE_3)
		while(pSCAL->SCAL_PATH_3_CTL != m_ubScalerInput[pipeID]);
	else if (pipeID == MMP_SCAL_PIPE_4)
		while(pSCAL->SCAL_PATH_4_CTL != m_ubScalerInput[pipeID]);
	
	#else//(SENSOR_EN)
    
    MMPF_Scaler_ChangePath(pipeID);
    
    #endif
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_ChangePath
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_ChangePath(MMP_SCAL_PIPEID pipeID)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;
    
    switch (pipeID)
    {
        case MMP_SCAL_PIPE_0:
            pSCAL->SCAL_PATH_0_CTL = m_ubScalerInput[pipeID];
        break;
        case MMP_SCAL_PIPE_1:
            pSCAL->SCAL_PATH_1_CTL = m_ubScalerInput[pipeID];
        break;   
        case MMP_SCAL_PIPE_2:
            pSCAL->SCAL_PATH_2_CTL = m_ubScalerInput[pipeID];
        break;
        case MMP_SCAL_PIPE_3:
            pSCAL->SCAL_PATH_3_CTL = m_ubScalerInput[pipeID];
        break;
        case MMP_SCAL_PIPE_4:
            pSCAL->SCAL_PATH_4_CTL = m_ubScalerInput[pipeID];
        break;
        default:
        //
        break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetSerialLinkPipe
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_SetSerialLinkPipe(MMP_SCAL_PIPEID srcPath, MMP_SCAL_PIPEID dstPath)
{
	if (srcPath == MMP_SCAL_PIPE_0)
		MMPF_Scaler_SetPath(dstPath, MMP_SCAL_SOURCE_P0);
	else if (srcPath == MMP_SCAL_PIPE_1)
		MMPF_Scaler_SetPath(dstPath, MMP_SCAL_SOURCE_P1);
	else if (srcPath == MMP_SCAL_PIPE_2)
		MMPF_Scaler_SetPath(dstPath, MMP_SCAL_SOURCE_P2);
	else if (srcPath == MMP_SCAL_PIPE_3)
		MMPF_Scaler_SetPath(dstPath, MMP_SCAL_SOURCE_P3);
	else
		return MMP_SCALER_ERR_PARAMETER;
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetGrabPosition
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_SetGrabPosition(MMP_SCAL_PIPEID 		pipeID,
									MMP_SCAL_GRAB_STAGE 	grabstage,
									MMP_USHORT 				usHStartPos, 
									MMP_USHORT 				usHEndPos, 
									MMP_USHORT 				usVStartPos, 
									MMP_USHORT 				usVEndPos)
{
	AITPS_SCAL	pSCAL   = AITC_BASE_SCAL;

    AIT_REG_W   *OprGrabLpfHStart = NULL, *OprGrabLpfHEnd = NULL, *OprGrabLpfVStart = NULL, *OprGrabLpfVEnd = NULL;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart = NULL, *OprGrabOutHEnd = NULL, *OprGrabOutVStart = NULL, *OprGrabOutVEnd = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
		break;
		case MMP_SCAL_PIPE_2:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
		break;			
		case MMP_SCAL_PIPE_4:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
		break;
        default:
        //
        break;
	}
    
	if(grabstage == MMP_SCAL_GRAB_STAGE_LPF)
	{
		*OprGrabLpfHStart	= usHStartPos;
        *OprGrabLpfHEnd     = usHEndPos;
        *OprGrabLpfVStart   = usVStartPos;
        *OprGrabLpfVEnd     = usVEndPos;  	
    }
    else if(grabstage == MMP_SCAL_GRAB_STAGE_SCA)
    {
		*OprGrabInHStart	= usHStartPos;
        *OprGrabInHEnd     	= usHEndPos;
        *OprGrabInVStart   	= usVStartPos;
        *OprGrabInVEnd     	= usVEndPos; 
    }
    else if(grabstage == MMP_SCAL_GRAB_STAGE_OUT)
    {
		*OprGrabOutHStart   = usHStartPos;
        *OprGrabOutHEnd     = usHEndPos;
        *OprGrabOutVStart   = usVStartPos;
        *OprGrabOutVEnd     = usVEndPos; 
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetGrabPosition
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetGrabPosition(MMP_SCAL_PIPEID pipeID, MMP_SCAL_GRAB_STAGE grabstage,
									MMP_USHORT *usHStartPos, MMP_USHORT *usHEndPos, 
									MMP_USHORT *usVStartPos, MMP_USHORT *usVEndPos)
{
	AITPS_SCAL	pSCAL   = AITC_BASE_SCAL;
	
    AIT_REG_W   *OprGrabLpfHStart = NULL, *OprGrabLpfHEnd = NULL, *OprGrabLpfVStart = NULL, *OprGrabLpfVEnd = NULL;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart = NULL, *OprGrabOutHEnd = NULL, *OprGrabOutVStart = NULL, *OprGrabOutVEnd = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
		break;
		case MMP_SCAL_PIPE_2:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
		break;
		case MMP_SCAL_PIPE_4:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
		break;	
        default:
        //
        break;
	}
    
	if(grabstage == MMP_SCAL_GRAB_STAGE_LPF)
	{
		*usHStartPos    = *OprGrabLpfHStart;
        *usHEndPos      = *OprGrabLpfHEnd;
        *usVStartPos    = *OprGrabLpfVStart;
        *usVEndPos      = *OprGrabLpfVEnd;
    }
    else if(grabstage == MMP_SCAL_GRAB_STAGE_SCA)
    {
		*usHStartPos    = *OprGrabInHStart;
        *usHEndPos      = *OprGrabInHEnd;
        *usVStartPos    = *OprGrabInVStart;
        *usVEndPos      = *OprGrabInVEnd;
    }
    else if(grabstage == MMP_SCAL_GRAB_STAGE_OUT)
    {
		*usHStartPos    = *OprGrabOutHStart;
        *usHEndPos      = *OprGrabOutHEnd;
        *usVStartPos    = *OprGrabOutVStart;
        *usVEndPos      = *OprGrabOutVEnd;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetGrabRange
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetGrabRange(MMP_SCAL_PIPEID		pipeID,
								 MMP_SCAL_GRAB_STAGE 	grabstage,
								 MMP_USHORT 			*usWidth, 
								 MMP_USHORT 			*usHeight)
{
	AITPS_SCAL	pSCAL   = AITC_BASE_SCAL;

    AIT_REG_W   *OprGrabLpfHStart = NULL, *OprGrabLpfHEnd = NULL, *OprGrabLpfVStart = NULL, *OprGrabLpfVEnd = NULL;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart = NULL, *OprGrabOutHEnd = NULL, *OprGrabOutVStart = NULL, *OprGrabOutVEnd = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
		break;
		case MMP_SCAL_PIPE_2:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
		break;
		case MMP_SCAL_PIPE_4:
	        OprGrabLpfHStart = &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLpfHEnd   = &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLpfVStart = &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLpfVEnd   = &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart  = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart  = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd    = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart = &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd   = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart = &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd   = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
		break;	
        default:
        //
        break;
	}

	if (grabstage == MMP_SCAL_GRAB_STAGE_LPF)
	{
        if (usWidth) 	*usWidth 	= *OprGrabLpfHEnd - *OprGrabLpfHStart + 1;
        if (usHeight) 	*usHeight 	= *OprGrabLpfVEnd - *OprGrabLpfVStart + 1;
    }
    else if (grabstage == MMP_SCAL_GRAB_STAGE_SCA)
    {
        if (usWidth) 	*usWidth	= *OprGrabInHEnd - *OprGrabInHStart + 1;
        if (usHeight) 	*usHeight  	= *OprGrabInVEnd - *OprGrabInVStart + 1;
    }
    else if (grabstage == MMP_SCAL_GRAB_STAGE_OUT) 
    {
        if (usWidth) 	*usWidth	= *OprGrabOutHEnd - *OprGrabOutHStart + 1;
        if (usHeight) 	*usHeight  	= *OprGrabOutVEnd - *OprGrabOutVStart + 1;
    }
   
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_CheckLPFAbility
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_CheckLPFAbility(MMP_SCAL_PIPEID pipeID, MMP_SCAL_LPF_ABILITY *ability)
{
	AITPS_SCAL pSCAL = AITC_BASE_SCAL;
	
	*ability = MMP_SCAL_LPF_ABILITY_NONE;
	
	switch(pipeID)
	{
	    case MMP_SCAL_PIPE_0:
            if( pSCAL->SCAL_LPF0_EQ_CTL & SCAL_LPF_EQ_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_EQ_EN;
			
		    if( pSCAL->SCAL_LPF0_DN_CTL & SCAL_LPF_DNSAMP_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_DN_EN;
	    break;
	    case MMP_SCAL_PIPE_1:
		    if( pSCAL->SCAL_LPF1_DN_CTL & SCAL_LPF_DNSAMP_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_DN_EN;
	    break;
	    case MMP_SCAL_PIPE_2:
		    if( pSCAL->SCAL_LPF2_DN_CTL & SCAL_LPF_DNSAMP_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_DN_EN;
	    break;
	    case MMP_SCAL_PIPE_3:
		    if( pSCAL->SCAL_LPF3_DN_CTL & SCAL_LPF_DNSAMP_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_DN_EN;
	    break;	
	    case MMP_SCAL_PIPE_4:
		    if( pSCAL->SCAL_LPF4_DN_CTL & SCAL_LPF_DNSAMP_MASK)
			    *ability |= MMP_SCAL_LPF_ABILITY_DN_EN;
	    break;
        default:
        //
        break;
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetLPFEnable
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetLPFEnable(MMP_SCAL_PIPEID pipeID, MMP_BOOL *bEnable)
{
    AITPS_SCAL pSCAL = AITC_BASE_SCAL;

	switch(pipeID)
	{
	    case MMP_SCAL_PIPE_0:
    		if(pSCAL->SCAL_LPF_AUTO_CTL & SCAL_LPF0_EN)
    			*bEnable = MMP_TRUE;
    		else
    			*bEnable = MMP_FALSE;
	    break;
	    case MMP_SCAL_PIPE_1:
    		if(pSCAL->SCAL_LPF_AUTO_CTL & SCAL_LPF1_EN)
    			*bEnable = MMP_TRUE;
    		else
    			*bEnable = MMP_FALSE;
	    break;
	    case MMP_SCAL_PIPE_2:
    		if(pSCAL->SCAL_LPF2_AUTO_CTL & SCAL_LPF2_EN)
    			*bEnable = MMP_TRUE;
    		else
    			*bEnable = MMP_FALSE;
	    break;
	    case MMP_SCAL_PIPE_3:
    		if(pSCAL->SCAL_LPF2_AUTO_CTL & SCAL_LPF3_EN)
    			*bEnable = MMP_TRUE;
    		else
    			*bEnable = MMP_FALSE;
	    break;	
	    case MMP_SCAL_PIPE_4:
    		if(pSCAL->SCAL_LPF4_AUTO_CTL & SCAL_LPF4_EN)
    			*bEnable = MMP_TRUE;
    		else
    			*bEnable = MMP_FALSE;
	    break;
        default:
        //
        break;
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetLPFDownSample
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetLPFDownSample(MMP_SCAL_PIPEID pipeID, MMP_SCAL_LPF_DNSAMP *downsample)
{	
    AITPS_SCAL	pSCAL   = AITC_BASE_SCAL;
    MMP_UBYTE 	ubReg = 0;

    *downsample = MMP_SCAL_LPF_DNSAMP_NONE;
    
    switch(pipeID)
    {
        case MMP_SCAL_PIPE_0:
            ubReg = pSCAL->SCAL_LPF0_DN_CTL & SCAL_LPF_DNSAMP_MASK;  
        break;
        case MMP_SCAL_PIPE_1:
            ubReg = pSCAL->SCAL_LPF1_DN_CTL & SCAL_LPF_DNSAMP_MASK;       
        break; 
        case MMP_SCAL_PIPE_2:
            ubReg = pSCAL->SCAL_LPF2_DN_CTL & SCAL_LPF_DNSAMP_MASK;        
        break;
        case MMP_SCAL_PIPE_3:
            ubReg = pSCAL->SCAL_LPF3_DN_CTL & SCAL_LPF_DNSAMP_MASK;       
        break;
        case MMP_SCAL_PIPE_4:
            ubReg = pSCAL->SCAL_LPF4_DN_CTL & SCAL_LPF_DNSAMP_MASK;      
        break;
        default:
        //
        break;
    }

    if (ubReg ==  SCAL_LPF_DNSAMP_1_2)
        *downsample = MMP_SCAL_LPF_DNSAMP_1_2;
    else if (ubReg == SCAL_LPF_DNSAMP_1_4)
        *downsample = MMP_SCAL_LPF_DNSAMP_1_4;
    else if (ubReg == SCAL_LPF_DNSAMP_1_8)
        *downsample = MMP_SCAL_LPF_DNSAMP_1_8; 

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetScalingRatio
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetScalingRatio(MMP_SCAL_PIPEID pipeID, 
									MMP_USHORT *usHN, MMP_USHORT *usHM,
									MMP_USHORT *usVN, MMP_USHORT *usVM)
{
	AITPS_SCAL	pSCAL = AITC_BASE_SCAL;
    AIT_REG_W   *OprNh, *OprMh, *OprNv, *OprMv;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprNh	= &(pSCAL->SCAL_SCAL_0_H_N);
	        OprMh	= &(pSCAL->SCAL_SCAL_0_H_M);
	        OprNv	= &(pSCAL->SCAL_SCAL_0_V_N);
	        OprMv	= &(pSCAL->SCAL_SCAL_0_V_M);
		break;
		case MMP_SCAL_PIPE_1:
	        OprNh	= &(pSCAL->SCAL_SCAL_1_H_N);
	        OprMh	= &(pSCAL->SCAL_SCAL_1_H_M);
	        OprNv	= &(pSCAL->SCAL_SCAL_1_V_N);
	        OprMv	= &(pSCAL->SCAL_SCAL_1_V_M);
		break;	
		case MMP_SCAL_PIPE_2:
	        OprNh 	= &(pSCAL->SCAL_SCAL_2_H_N);
	        OprMh 	= &(pSCAL->SCAL_SCAL_2_H_M);
	        OprNv 	= &(pSCAL->SCAL_SCAL_2_V_N);
	        OprMv 	= &(pSCAL->SCAL_SCAL_2_V_M);
		break;
		case MMP_SCAL_PIPE_3:
	        OprNh 	= &(pSCAL->SCAL_SCAL_3_H_N);
	        OprMh 	= &(pSCAL->SCAL_SCAL_3_H_M);
	        OprNv 	= &(pSCAL->SCAL_SCAL_3_V_N);
	        OprMv  	= &(pSCAL->SCAL_SCAL_3_V_M);
		break;
		case MMP_SCAL_PIPE_4:
	        OprNh  	= &(pSCAL->SCAL_SCAL_4_H_N);
	        OprMh 	= &(pSCAL->SCAL_SCAL_4_H_M);
	        OprNv  	= &(pSCAL->SCAL_SCAL_4_V_N);
	        OprMv  	= &(pSCAL->SCAL_SCAL_4_V_M);
		break;
		default:
			return MMP_ERR_NONE;
		break;
	}
	
	*usHN = *OprNh;
	*usHM = *OprMh;
	*usVN = *OprNv;
	*usVM = *OprMv;
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetAttributes
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetAttributes(MMP_SCAL_PIPEID pipeID, MMP_SCAL_SETTING* pSetting)
{
    AITPS_SCAL	pSCAL = AITC_BASE_SCAL;
    
    AIT_REG_W   *OprGrabLPFHStart = NULL, *OprGrabLPFHEnd = NULL, *OprGrabLPFVStart = NULL, *OprGrabLPFVEnd = NULL;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart = NULL, *OprGrabOutHEnd = NULL, *OprGrabOutVStart = NULL, *OprGrabOutVEnd = NULL;
    AIT_REG_B   *OprHDNSampLPF = NULL, *OprVDNSampLPF = NULL;
    AIT_REG_B   *OprHDNSampIn = NULL, *OprVDNSampIn = NULL;
    AIT_REG_B   *OprHDNSampOut = NULL, *OprVDNSampOut = NULL;
    AIT_REG_B   *OprLPF_DNCtl = NULL, *OprLPF_EQCtl = NULL, *OprLPF_AutoCtl = NULL;
    AIT_REG_B   *OprScaleCtl = NULL;
    AIT_REG_W   *OprNh = NULL, *OprMh = NULL, *OprNv = NULL, *OprMv = NULL;
    AIT_REG_W   *OprHWeight = NULL, *OprVWeight = NULL;
    AIT_REG_B   *OprEdgeCtl = NULL, *OprEdgeGain = NULL, *OprEdgeCore = NULL;
    AIT_REG_B   *OprOutCtl = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_0_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_0_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_0_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_0_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_0_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_0_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF0_DN_CTL);
	        OprLPF_EQCtl        = &(pSCAL->SCAL_LPF0_EQ_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF_AUTO_CTL);
	        
	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_0_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_0_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_0_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_0_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_0_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_0_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_0_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_0_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_0_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_0_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_0_CTL);
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_1_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_1_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_1_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_1_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_1_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_1_V);

	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF1_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF_AUTO_CTL);
	        
	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_1_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_1_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_1_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_1_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_1_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_1_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_1_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_1_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_1_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_1_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_1_CTL);
		break;	
		case MMP_SCAL_PIPE_2:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_2_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_2_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_2_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_2_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_2_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_2_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF2_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF2_AUTO_CTL);
	        
	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_2_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_2_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_2_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_2_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_2_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_2_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_2_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_2_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_2_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_2_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_2_CTL);
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_3_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_3_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_3_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_3_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_3_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_3_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF3_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF2_AUTO_CTL);
	        
	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_3_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_3_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_3_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_3_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_3_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_3_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_3_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_3_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_3_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_3_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_3_CTL);	
		break;
		case MMP_SCAL_PIPE_4:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_4_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_4_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_4_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_4_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_4_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_4_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF4_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF4_AUTO_CTL);
	        
	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_4_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_4_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_4_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_4_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_4_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_4_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_4_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_4_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_4_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_4_CORE);
		    
		    OprOutCtl           = &(pSCAL->_x4570[0]);
		break;
		default:
			return MMP_ERR_NONE;
		break;
	}

	pSetting->ulLpfHRange 			= (*OprGrabLPFHStart << 16) | *OprGrabLPFHEnd;
	pSetting->ulLpfVRange			= (*OprGrabLPFVStart << 16) | *OprGrabLPFVEnd;
	pSetting->usLpfDownSampleVal	= (*OprHDNSampLPF << 8)     | *OprVDNSampLPF;
	
	if (pipeID == MMP_SCAL_PIPE_0) {
	    pSetting->usLPFSetting	    = (*OprLPF_DNCtl << 8)      | *OprLPF_EQCtl;
	    pSetting->ubAutoLPF			= (*OprLPF_AutoCtl) & SCAL_LPF0_EN;
	}
	else if (pipeID == MMP_SCAL_PIPE_1) {
	    pSetting->usLPFSetting		= (*OprLPF_DNCtl << 8);
	    pSetting->ubAutoLPF			= (*OprLPF_AutoCtl) & SCAL_LPF1_EN;
	}
	else if (pipeID == MMP_SCAL_PIPE_2) {
	    pSetting->usLPFSetting		= (*OprLPF_DNCtl << 8);        
	    pSetting->ubAutoLPF			= (*OprLPF_AutoCtl) & SCAL_LPF2_EN;
	}
	else if (pipeID == MMP_SCAL_PIPE_3) {
	    pSetting->usLPFSetting		= (*OprLPF_DNCtl << 8);        
	    pSetting->ubAutoLPF			= (*OprLPF_AutoCtl) & SCAL_LPF3_EN;
	}
	else if (pipeID == MMP_SCAL_PIPE_4) {
	    pSetting->usLPFSetting		= (*OprLPF_DNCtl << 8);        
	    pSetting->ubAutoLPF			= (*OprLPF_AutoCtl) & SCAL_LPF4_EN;
	}
	
	pSetting->ulScaInHRange 		= (*OprGrabInHStart << 16)  | *OprGrabInHEnd;
	pSetting->ulScaInVRange			= (*OprGrabInVStart << 16)  | *OprGrabInVEnd;
	pSetting->usScaInDownSampleVal	= (*OprHDNSampIn << 8)      | *OprVDNSampIn;
	
	pSetting->ubScalerCtl			= *OprScaleCtl;
	pSetting->ulHorizontalRatio		= (*OprMh << 16)            | *OprNh;
	pSetting->ulVerticalRatio		= (*OprMv << 16)            | *OprNv;
	pSetting->ulWeight				= (*OprHWeight << 16)       | *OprVWeight;
	
	pSetting->ulEdgeEnhance			= (*OprEdgeCtl << 16)       | (*OprEdgeGain << 8)   |   *OprEdgeCore;
							  
	pSetting->ulOutHRange 			= (*OprGrabOutHStart << 16) | *OprGrabOutHEnd;
	pSetting->ulOutVRange			= (*OprGrabOutVStart << 16) | *OprGrabOutVEnd;
	pSetting->usOutDownSampleVal	= (*OprHDNSampOut << 8)     | *OprVDNSampOut;
	pSetting->ubOutputCtl			= *OprOutCtl;

	pSetting->ubOutColorMode		= m_ubScalerOutColorMode[pipeID];

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_SetAttributes
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_SetAttributes(MMP_SCAL_PIPEID pipeID, MMP_SCAL_SETTING* pSetting)
{
    AITPS_SCAL	pSCAL   = AITC_BASE_SCAL;
    
//    AIT_REG_W   *OprGrabLPFHStart, *OprGrabLPFHEnd, *OprGrabLPFVStart, *OprGrabLPFVEnd;
    AIT_REG_W   *OprGrabLPFHStart = NULL, *OprGrabLPFHEnd = NULL, *OprGrabLPFVStart = NULL, *OprGrabLPFVEnd = NULL;
    AIT_REG_W   *OprGrabInHStart = NULL, *OprGrabInHEnd = NULL, *OprGrabInVStart = NULL, *OprGrabInVEnd = NULL;
    AIT_REG_W   *OprGrabOutHStart = NULL, *OprGrabOutHEnd = NULL, *OprGrabOutVStart = NULL, *OprGrabOutVEnd = NULL;
    AIT_REG_B   *OprHDNSampLPF = NULL, *OprVDNSampLPF = NULL;
    AIT_REG_B   *OprHDNSampIn = NULL, *OprVDNSampIn = NULL;
    AIT_REG_B   *OprHDNSampOut = NULL, *OprVDNSampOut = NULL;
    AIT_REG_B   *OprLPF_DNCtl = NULL, *OprLPF_EQCtl = NULL, *OprLPF_AutoCtl = NULL;
    AIT_REG_W   *OprNh = NULL, *OprMh = NULL, *OprNv = NULL, *OprMv = NULL;
    AIT_REG_W   *OprHWeight = NULL, *OprVWeight = NULL;
    AIT_REG_B   *OprEdgeCtl = NULL, *OprEdgeGain = NULL, *OprEdgeCore = NULL;
    AIT_REG_B   *OprOutCtl = NULL;

	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_0_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_0_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_0_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_0_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_0_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_0_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_0_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_0_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_0_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_0_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_0_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_0_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_0_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_0_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_0_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_0_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_0_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_0_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF0_DN_CTL);
	        OprLPF_EQCtl        = &(pSCAL->SCAL_LPF0_EQ_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF_AUTO_CTL);
	        
//	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_0_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_0_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_0_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_0_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_0_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_0_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_0_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_0_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_0_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_0_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_0_CTL);
		break;
		case MMP_SCAL_PIPE_1:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_1_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_1_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_1_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_1_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_1_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_1_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_1_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_1_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_1_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_1_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_1_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_1_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_1_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_1_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_1_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_1_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_1_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_1_V);

	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF1_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF_AUTO_CTL);
	        
//	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_1_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_1_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_1_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_1_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_1_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_1_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_1_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_1_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_1_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_1_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_1_CTL);	
		break;	
		case MMP_SCAL_PIPE_2:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_2_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_2_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_2_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_2_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_2_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_2_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_2_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_2_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_2_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_2_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_2_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_2_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_2_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_2_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_2_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_2_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_2_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_2_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF2_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF2_AUTO_CTL);
	        
//	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_2_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_2_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_2_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_2_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_2_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_2_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_2_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_2_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_2_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_2_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_2_CTL);	
		break;
		case MMP_SCAL_PIPE_3:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_3_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_3_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_3_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_3_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_3_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_3_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_3_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_3_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_3_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_3_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_3_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_3_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_3_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_3_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_3_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_3_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_3_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_3_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF3_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF2_AUTO_CTL);
	        
//	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_3_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_3_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_3_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_3_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_3_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_3_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_3_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_3_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_3_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_3_CORE);
		    
		    OprOutCtl           = &(pSCAL->SCAL_OUT_3_CTL);	
		break;
		case MMP_SCAL_PIPE_4:
	        OprGrabLPFHStart    = &(pSCAL->SCAL_GRAB_LPF_4_H_ST);
	        OprGrabLPFHEnd      = &(pSCAL->SCAL_GRAB_LPF_4_H_ED);
	        OprGrabLPFVStart    = &(pSCAL->SCAL_GRAB_LPF_4_V_ST);
	        OprGrabLPFVEnd      = &(pSCAL->SCAL_GRAB_LPF_4_V_ED);
	        OprGrabInHStart     = &(pSCAL->SCAL_GRAB_SCAL_4_H_ST);
	        OprGrabInHEnd       = &(pSCAL->SCAL_GRAB_SCAL_4_H_ED);
	        OprGrabInVStart     = &(pSCAL->SCAL_GRAB_SCAL_4_V_ST);
	        OprGrabInVEnd       = &(pSCAL->SCAL_GRAB_SCAL_4_V_ED);
	        OprGrabOutHStart    = &(pSCAL->SCAL_GRAB_OUT_4_H_ST);
	        OprGrabOutHEnd      = &(pSCAL->SCAL_GRAB_OUT_4_H_ED);
	        OprGrabOutVStart    = &(pSCAL->SCAL_GRAB_OUT_4_V_ST);
	        OprGrabOutVEnd      = &(pSCAL->SCAL_GRAB_OUT_4_V_ED);
	        
	        OprHDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_4_H);
	        OprVDNSampLPF       = &(pSCAL->SCAL_DNSAMP_LPF_4_V);
	        OprHDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_4_H);
	        OprVDNSampIn        = &(pSCAL->SCAL_DNSAMP_SCAL_4_V);
	        OprHDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_4_H);
            OprVDNSampOut       = &(pSCAL->SCAL_DNSAMP_OUT_4_V);
	        
	        OprLPF_DNCtl        = &(pSCAL->SCAL_LPF4_DN_CTL);
	        OprLPF_AutoCtl      = &(pSCAL->SCAL_LPF4_AUTO_CTL);
	        
//	        OprScaleCtl         = &(pSCAL->SCAL_SCAL_4_CTL);
	        OprNh               = &(pSCAL->SCAL_SCAL_4_H_N);
	        OprMh               = &(pSCAL->SCAL_SCAL_4_H_M);
	        OprNv               = &(pSCAL->SCAL_SCAL_4_V_N);
	        OprMv               = &(pSCAL->SCAL_SCAL_4_V_M);
	        OprHWeight          = &(pSCAL->SCAL_SCAL_4_H_WT);
	        OprVWeight          = &(pSCAL->SCAL_SCAL_4_V_WT);
		
		    OprEdgeCtl          = &(pSCAL->SCAL_EDGE_4_CTL);
		    OprEdgeGain         = &(pSCAL->SCAL_EDGE_4_GAIN);
		    OprEdgeCore         = &(pSCAL->SCAL_EDGE_4_CORE);
		    
		    OprOutCtl           = &(pSCAL->_x4570[0]);	
		break;
		default:
			return MMP_ERR_NONE;
		break;
	}

	*OprGrabLPFHStart   = SHIFT16(pSetting->ulLpfHRange);
	*OprGrabLPFHEnd     = MASK16(pSetting->ulLpfHRange);
	*OprGrabLPFVStart   = SHIFT16(pSetting->ulLpfVRange);
	*OprGrabLPFVEnd     = MASK16(pSetting->ulLpfVRange);
	*OprHDNSampLPF      = SHIFT8(pSetting->usLpfDownSampleVal);
	*OprVDNSampLPF      = MASK8(pSetting->usLpfDownSampleVal);
	
	*OprLPF_DNCtl       = SHIFT8(pSetting->usLPFSetting);

    if (pipeID == MMP_SCAL_PIPE_0) {
	    *OprLPF_EQCtl   = MASK8(pSetting->usLPFSetting);
        if (pSetting->ubAutoLPF != 0)
            *OprLPF_AutoCtl |= SCAL_LPF0_EN;
    }
    else if(pipeID == MMP_SCAL_PIPE_1) {
        if (pSetting->ubAutoLPF != 0)
            *OprLPF_AutoCtl |= SCAL_LPF1_EN;
    }
    else if(pipeID == MMP_SCAL_PIPE_2) {
        if (pSetting->ubAutoLPF != 0)
            *OprLPF_AutoCtl |= SCAL_LPF2_EN;
    }
    else if(pipeID == MMP_SCAL_PIPE_3) {
        if (pSetting->ubAutoLPF != 0)
            *OprLPF_AutoCtl |= SCAL_LPF3_EN;
    }
    else if(pipeID == MMP_SCAL_PIPE_4) {
        if (pSetting->ubAutoLPF != 0)
            *OprLPF_AutoCtl |= SCAL_LPF4_EN;
    }

	*OprGrabInHStart    = SHIFT16(pSetting->ulScaInHRange);
	*OprGrabInHEnd      = MASK16(pSetting->ulScaInHRange);
	*OprGrabInVStart    = SHIFT16(pSetting->ulScaInVRange);
	*OprGrabInVEnd      = MASK16(pSetting->ulScaInVRange);
	*OprHDNSampIn       = SHIFT8(pSetting->usScaInDownSampleVal);
	*OprVDNSampIn       = MASK8(pSetting->usScaInDownSampleVal);
	
//	*OprScaleCtl        = pSetting->ubScalerCtl;
	*OprMh              = SHIFT16(pSetting->ulHorizontalRatio);
	*OprNh              = MASK16(pSetting->ulHorizontalRatio);
	*OprMv              = SHIFT16(pSetting->ulVerticalRatio);
	*OprNv              = MASK16(pSetting->ulVerticalRatio);
	*OprHWeight         = SHIFT16(pSetting->ulWeight);
	*OprVWeight         = MASK16(pSetting->ulWeight);
	
	*OprEdgeCtl         = SHIFT16(pSetting->ulEdgeEnhance);
	*OprEdgeGain        = SHIFT8(pSetting->ulEdgeEnhance);
	*OprEdgeCore        = MASK8(pSetting->ulEdgeEnhance);
	
	*OprGrabOutHStart   = SHIFT16(pSetting->ulOutHRange);
	*OprGrabOutHEnd     = MASK16(pSetting->ulOutHRange);
	*OprGrabOutVStart   = SHIFT16(pSetting->ulOutVRange);
	*OprGrabOutVEnd     = MASK16(pSetting->ulOutVRange);
	*OprHDNSampOut      = SHIFT8(pSetting->usOutDownSampleVal);
	*OprVDNSampOut      = MASK8(pSetting->usOutDownSampleVal);

    *OprOutCtl          = pSetting->ubOutputCtl;

	// Restore Color Transform setting
	MMPF_Scaler_SetOutColorTransform(pipeID, MMP_TRUE, pSetting->ubOutColorMode);

	return MMP_ERR_NONE;
}

#if 0
void ____Flow_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetScaleUpHBlanking
//  Description : The function get the exact H-blanking value when scaling up.
//------------------------------------------------------------------------------
/** 
 * @brief The function get the exact H-blanking value when scaling up.
 * 
 *  The function get the exact H-blanking value when scaling up.
 * @param[in] pipeID     : stands for scaler pipe.
 * @param[out] pHBlanking : stands for out horizontal blanking.
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_GetScaleUpHBlanking(MMP_SCAL_PIPEID pipeID, MMP_ULONG64 *pHBlanking)
{
	AITPS_SCAL  pSCAL = AITC_BASE_SCAL;
    MMP_ULONG   ScalerInWidth = 0;
    MMP_USHORT  ScaleHM = 0, ScaleHN = 0, ScaleVM = 0, ScaleVN = 0;
	
	switch(pipeID)
	{
		case MMP_SCAL_PIPE_0:
//	        OprScaleCtl 	= &(pSCAL->SCAL_SCAL_0_CTL);
	        ScalerInWidth 	= pSCAL->SCAL_GRAB_SCAL_0_H_ED - pSCAL->SCAL_GRAB_SCAL_0_H_ST + 1;

	        ScaleHN 		= pSCAL->SCAL_SCAL_0_H_N;
	        ScaleHM 		= pSCAL->SCAL_SCAL_0_H_M;
	        ScaleVN 		= pSCAL->SCAL_SCAL_0_V_N;
	        ScaleVM 		= pSCAL->SCAL_SCAL_0_V_M;
		break;
		case MMP_SCAL_PIPE_1:
 //       	OprScaleCtl 	= &(pSCAL->SCAL_SCAL_1_CTL);
        	ScalerInWidth 	= pSCAL->SCAL_GRAB_SCAL_1_H_ED - pSCAL->SCAL_GRAB_SCAL_1_H_ST + 1;

        	ScaleHN 		= pSCAL->SCAL_SCAL_1_H_N;
        	ScaleHM 		= pSCAL->SCAL_SCAL_1_H_M;
        	ScaleVN 		= pSCAL->SCAL_SCAL_1_V_N;
        	ScaleVM 		= pSCAL->SCAL_SCAL_1_V_M;
		break;
		case MMP_SCAL_PIPE_2:
//	        OprScaleCtl 	= &(pSCAL->SCAL_SCAL_2_CTL);
	        ScalerInWidth 	= pSCAL->SCAL_GRAB_SCAL_2_H_ED - pSCAL->SCAL_GRAB_SCAL_2_H_ST + 1;

	        ScaleHN 		= pSCAL->SCAL_SCAL_2_H_N;
	        ScaleHM 		= pSCAL->SCAL_SCAL_2_H_M;
	        ScaleVN 		= pSCAL->SCAL_SCAL_2_V_N;
	        ScaleVM 		= pSCAL->SCAL_SCAL_2_V_M;
		break;
		case MMP_SCAL_PIPE_3:
//	        OprScaleCtl 	= &(pSCAL->SCAL_SCAL_3_CTL);
	        ScalerInWidth 	= pSCAL->SCAL_GRAB_SCAL_3_H_ED - pSCAL->SCAL_GRAB_SCAL_3_H_ST + 1;

	        ScaleHN 		= pSCAL->SCAL_SCAL_3_H_N;
	        ScaleHM 		= pSCAL->SCAL_SCAL_3_H_M;
	        ScaleVN 		= pSCAL->SCAL_SCAL_3_V_N;
	        ScaleVM 		= pSCAL->SCAL_SCAL_3_V_M;
		break;
		case MMP_SCAL_PIPE_4:
//	        OprScaleCtl 	= &(pSCAL->SCAL_SCAL_4_CTL);
	        ScalerInWidth 	= pSCAL->SCAL_GRAB_SCAL_4_H_ED - pSCAL->SCAL_GRAB_SCAL_4_H_ST + 1;

	        ScaleHN 		= pSCAL->SCAL_SCAL_4_H_N;
	        ScaleHM 		= pSCAL->SCAL_SCAL_4_H_M;
	        ScaleVN 		= pSCAL->SCAL_SCAL_4_V_N;
	        ScaleVM 		= pSCAL->SCAL_SCAL_4_V_M;
		break;
        default:
        //
        break;
	}

    if ((ScaleHN > ScaleHM) || (ScaleVN > ScaleVM)){

        *pHBlanking = ScalerInWidth * ((MMP_ULONG)(ScaleHN + ScaleHM -1)/ScaleHM) * ((MMP_ULONG)(ScaleVN + ScaleVM -1)/ScaleVM) + 64 + 16; //16 is tolerance 
    }
    else {
        *pHBlanking = 16; // ISP line delay must have 3 pixel at least. 16 is tolerance 
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetGCDBestFitScale
//  Description : Use GCD algorithm to get best fit grab range.
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetGCDBestFitScale(MMP_SCAL_FIT_RANGE *fitrange, MMP_SCAL_GRAB_CTRL *grabctl)
{
    MMP_ULONG  	gcd_w = 0, gcd_h = 0;
    MMP_USHORT 	src_w = 0, dst_w = 0, src_h = 0, dst_h = 0, alignBase = 1, tmp_dummy = 0;
    MMP_USHORT	usMaxMNVal = MAX_SCAL_NM_VALUE;
    MMP_UBYTE  	ubScalMode = SCALER_MODE_BILINEAR;
    
    #define MAX_SCAL_DUMMY_PIXELS (8)
    
    /* Parameter Check */
    if ((fitrange == NULL) || (grabctl == NULL))
    {
        RTNA_DBG_Str(0, "GetGCDBestFitScale parameter Error0!\r\n");
        return MMP_ERR_NONE;
    }

    if ((fitrange->ulInWidth == 0) || (fitrange->ulInHeight == 0) || 
        (fitrange->ulOutWidth == 0) || (fitrange->ulOutHeight == 0) ||
        (fitrange->ulInGrabW == 0) || (fitrange->ulInGrabH == 0))
    {
        RTNA_DBG_Str(0, "GetGCDBestFitScale parameter Error1!\r\n");
        return MMP_ERR_NONE;
    }
    
    if (((fitrange->ulInGrabX + fitrange->ulInGrabW - 1) > fitrange->ulInWidth) ||
        ((fitrange->ulInGrabY + fitrange->ulInGrabH - 1) > fitrange->ulInHeight))
    {
        RTNA_DBG_Str(0, "GetGCDBestFitScale parameter Error2!\r\n");
        return MMP_ERR_NONE;
    }
	
	/* Decide the scaler type and max value limitation */
	if (fitrange->scalerType == MMP_SCAL_TYPE_BAYERSCALER) {
		usMaxMNVal = MAX_BAYERSCAL_NM_VALUE;
		ubScalMode = SCALER_MODE_BILINEAR;
	}
	else if (fitrange->scalerType == MMP_SCAL_TYPE_WINSCALER) {
		usMaxMNVal = MAX_WINSCAL_NM_VALUE;
		ubScalMode = SCALER_MODE_BICUBIC;
	}
	else {
		usMaxMNVal = MAX_SCAL_NM_VALUE;
		ubScalMode = SCALER_MODE_BILINEAR;
	}

    MEMSET((void*)grabctl, 0x0, sizeof(MMP_SCAL_GRAB_CTRL));

    alignBase = 1;

    /* Calculate Horizontal Ratio */
    do {
    	if (fitrange->ulInGrabW < fitrange->ulOutWidth && ubScalMode == SCALER_MODE_BILINEAR) {
 	        src_w = fitrange->ulInGrabW - 1;
	        dst_w = fitrange->ulOutWidth;
    	}
    	else {
	        src_w = fitrange->ulInGrabW;
	        dst_w = fitrange->ulOutWidth;
        }
        
        dst_w = ALIGN_X(dst_w, alignBase);
        gcd_w = Greatest_Common_Divisor(src_w, dst_w);
        
        grabctl->ulScaleXN = dst_w / gcd_w;
        grabctl->ulScaleXM = src_w / gcd_w;

        if ((grabctl->ulScaleXN > usMaxMNVal) || (grabctl->ulScaleXM > usMaxMNVal)) {
            RTNA_DBG_Str(3, "Horizontal N/M overflow\r\n");
        }
        
        alignBase = alignBase << 1;

    } while((grabctl->ulScaleXN > usMaxMNVal) || (grabctl->ulScaleXM > usMaxMNVal));
    
    alignBase = 1;
    
    /* Calculate Vertical Ratio */
    do {
    	if (fitrange->ulInGrabH < fitrange->ulOutHeight && ubScalMode == SCALER_MODE_BILINEAR) {
	        src_h = fitrange->ulInGrabH - 1;
	        dst_h = fitrange->ulOutHeight;
    	}
    	else {
	        src_h = fitrange->ulInGrabH;
	        dst_h = fitrange->ulOutHeight;
		}

        dst_h = ALIGN_X(dst_h, alignBase);
        gcd_h = Greatest_Common_Divisor(src_h, dst_h);
        
        grabctl->ulScaleYN = dst_h / gcd_h;
        grabctl->ulScaleYM = src_h / gcd_h;

        if ((grabctl->ulScaleYN > usMaxMNVal) || (grabctl->ulScaleYM > usMaxMNVal)) {
            RTNA_DBG_Str(3, "Vertical N/M overflow\r\n");
        }

        alignBase = alignBase << 1;

    } while((grabctl->ulScaleYN > usMaxMNVal) || (grabctl->ulScaleYM > usMaxMNVal));
    
	/* Calculate Grab Range and Dummy Pixels */
    if (((fitrange->fitmode == MMP_SCAL_FITMODE_OUT) || (fitrange->fitmode == MMP_SCAL_FITMODE_IN)) &&
    	(((fitrange->ulInGrabW >= fitrange->ulOutWidth) && (fitrange->ulInGrabH >= fitrange->ulOutHeight)) ||
         ((fitrange->ulInGrabW < fitrange->ulOutWidth) && (fitrange->ulInGrabH < fitrange->ulOutHeight)))) 
    {
        /* Scaling Up/Down on both side */
        if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT) 
        {     
	        if ((grabctl->ulScaleXN * grabctl->ulScaleYM) >= 
	            (grabctl->ulScaleYN * grabctl->ulScaleXM)) {
	            grabctl->ulScaleN = grabctl->ulScaleXN;
	            grabctl->ulScaleM = grabctl->ulScaleXM;
	        }
	        else {
	            grabctl->ulScaleN = grabctl->ulScaleYN;
	            grabctl->ulScaleM = grabctl->ulScaleYM;
        	}

			/* Grab Center Region */
			if (grabctl->ulScaleN > grabctl->ulScaleM && ubScalMode == SCALER_MODE_BILINEAR)
			{
		        grabctl->ulOutStX = ((((fitrange->ulInGrabW - 1) * grabctl->ulScaleN / grabctl->ulScaleM) - fitrange->ulOutWidth) >> 1) + 1;
		        grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;

		        grabctl->ulOutStY = ((((fitrange->ulInGrabH - 1) * grabctl->ulScaleN / grabctl->ulScaleM) - fitrange->ulOutHeight) >> 1) + 1;
		        grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
        	}
        	else
        	{
		        grabctl->ulOutStX = (((fitrange->ulInGrabW * grabctl->ulScaleN / grabctl->ulScaleM) - fitrange->ulOutWidth) >> 1) + 1;
		        grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;

		        grabctl->ulOutStY = (((fitrange->ulInGrabH * grabctl->ulScaleN / grabctl->ulScaleM) - fitrange->ulOutHeight) >> 1) + 1;
		        grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
        	}
        }
        else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN)
        {
	        if ((grabctl->ulScaleXN * grabctl->ulScaleYM) >= 
	            (grabctl->ulScaleYN * grabctl->ulScaleXM)) {
	            grabctl->ulScaleN = grabctl->ulScaleYN;
	            grabctl->ulScaleM = grabctl->ulScaleYM;
	        }
	        else {
	            grabctl->ulScaleN = grabctl->ulScaleXN;
	            grabctl->ulScaleM = grabctl->ulScaleXM;
        	}

			/* Grab All Region */
			if (grabctl->ulScaleN > grabctl->ulScaleM && ubScalMode == SCALER_MODE_BILINEAR)
			{
		        grabctl->ulOutStX = 1;
	       		grabctl->ulOutStY = 1;
			    grabctl->ulOutEdX = (fitrange->ulInGrabW - 1) * grabctl->ulScaleN / grabctl->ulScaleM;
		    	grabctl->ulOutEdY = (fitrange->ulInGrabH - 1) * grabctl->ulScaleN / grabctl->ulScaleM;
			}
			else
			{
		        grabctl->ulOutStX = 1;
	       		grabctl->ulOutStY = 1;
			    grabctl->ulOutEdX = fitrange->ulInGrabW * grabctl->ulScaleN / grabctl->ulScaleM;
		    	grabctl->ulOutEdY = fitrange->ulInGrabH * grabctl->ulScaleN / grabctl->ulScaleM;
        	}
        }
		
		/* Calculate Dummy Input Pixels for Scaling Up */
        if (grabctl->ulScaleN > grabctl->ulScaleM) {

            tmp_dummy = fitrange->ulInWidth - (fitrange->ulInGrabX + fitrange->ulInGrabW - 1);
            fitrange->ulDummyInPixelX = (tmp_dummy >= MAX_SCAL_DUMMY_PIXELS) ? MAX_SCAL_DUMMY_PIXELS : tmp_dummy;
            
            tmp_dummy = fitrange->ulInHeight - (fitrange->ulInGrabY + fitrange->ulInGrabH - 1);
            fitrange->ulDummyInPixelY = (tmp_dummy >= MAX_SCAL_DUMMY_PIXELS) ? MAX_SCAL_DUMMY_PIXELS : tmp_dummy;
        }        
        else {
            fitrange->ulDummyInPixelX = fitrange->ulDummyInPixelY = 0;
        }
    }
    else {
		
		/* One side Scaling Up and the other side Scaling Down or Use MMP_SCAL_FITMODE_OPTIMAL mode */  
        grabctl->ulScaleN = 0;
        grabctl->ulScaleM = 0;                
		
		/* Grab Center Region */
		if (grabctl->ulScaleXN > grabctl->ulScaleXM && ubScalMode == SCALER_MODE_BILINEAR)
		{
	        grabctl->ulOutStX = ((((fitrange->ulInGrabW - 1) * grabctl->ulScaleXN / grabctl->ulScaleXM) - fitrange->ulOutWidth) >> 1) + 1;
	        grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;
		}
		else
		{
	        grabctl->ulOutStX = (((fitrange->ulInGrabW * grabctl->ulScaleXN / grabctl->ulScaleXM) - fitrange->ulOutWidth) >> 1) + 1;
	        grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;
		}
		
		if (grabctl->ulScaleYN > grabctl->ulScaleYM && ubScalMode == SCALER_MODE_BILINEAR)
		{
	        grabctl->ulOutStY = ((((fitrange->ulInGrabH - 1) * grabctl->ulScaleYN / grabctl->ulScaleYM) - fitrange->ulOutHeight) >> 1) + 1;
	        grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
		}
		else
		{
	        grabctl->ulOutStY = (((fitrange->ulInGrabH * grabctl->ulScaleYN / grabctl->ulScaleYM) - fitrange->ulOutHeight) >> 1) + 1;
	        grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
		}

		/* Calculate Dummy Pixels for Scaling Up */
        if (grabctl->ulScaleXN > grabctl->ulScaleXM) {
            tmp_dummy = fitrange->ulInWidth - (fitrange->ulInGrabX + fitrange->ulInGrabW - 1);
            fitrange->ulDummyInPixelX = (tmp_dummy >= MAX_SCAL_DUMMY_PIXELS) ? MAX_SCAL_DUMMY_PIXELS : tmp_dummy;
        }                     
        else {
            fitrange->ulDummyInPixelX = 0;
        }          
        
        if (grabctl->ulScaleYN > grabctl->ulScaleYM) {
            tmp_dummy = fitrange->ulInHeight - (fitrange->ulInGrabY + fitrange->ulInGrabH - 1);
            fitrange->ulDummyInPixelY = (tmp_dummy >= MAX_SCAL_DUMMY_PIXELS) ? MAX_SCAL_DUMMY_PIXELS : tmp_dummy;
        }                     
        else {
            fitrange->ulDummyInPixelY = 0;
        }          
    }

	#ifdef SCAL_FUNC_DBG
    {     
        static MMP_ULONG oldxn = 0, oldxm = 0, oldyn = 0, oldym = 0, oldinw = 0, oldinh = 0;
        
        if ((oldxn != grabctl->ulScaleXN) || (oldxm != grabctl->ulScaleXM) || 
            (oldyn != grabctl->ulScaleYN) || (oldym != grabctl->ulScaleYM) || 
            (oldinw != fitrange->ulInGrabW) || (oldinh != fitrange->ulInGrabH)) {
	
			MMPF_Scaler_DumpSetting((void *)__func__, fitrange, grabctl);

            oldxn 	= grabctl->ulScaleXN;
            oldxm 	= grabctl->ulScaleXM;
            oldyn 	= grabctl->ulScaleYN;
            oldym 	= grabctl->ulScaleYM;
            oldinw 	= fitrange->ulInGrabW;
            oldinh 	= fitrange->ulInGrabH;
        }
    }
	#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetBestInGrabRange
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetBestInGrabRange(MMP_SCAL_FIT_RANGE *fitrange, MMP_SCAL_GRAB_CTRL *grabctl)
{
	MMP_ULONG 	ulTmpScaleXN = 0, ulTmpScaleXM = 0, ulTmpScaleYN = 0, ulTmpScaleYM = 0;
	MMP_USHORT  usUnScaleWidth = 0, usUnScaleHeight = 0;

    if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT || 
  		fitrange->fitmode == MMP_SCAL_FITMODE_IN) 
  	{
	    ulTmpScaleXN = grabctl->ulScaleN;
	    ulTmpScaleXM = grabctl->ulScaleM;
	    ulTmpScaleYN = grabctl->ulScaleN;
	    ulTmpScaleYM = grabctl->ulScaleM;
    }
    else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL)
    {
	    ulTmpScaleXN = grabctl->ulScaleXN;
	    ulTmpScaleXM = grabctl->ulScaleXM;
	    ulTmpScaleYN = grabctl->ulScaleYN;
	    ulTmpScaleYM = grabctl->ulScaleYM; 
    }

	if ((ulTmpScaleXN > ulTmpScaleXM) || (ulTmpScaleYN > ulTmpScaleYM)) 
	{
        MMP_ULONG x_st = (grabctl->ulOutStX * ulTmpScaleXM + ulTmpScaleXN -1) / ulTmpScaleXN;
        MMP_ULONG y_st = (grabctl->ulOutStY * ulTmpScaleYM + ulTmpScaleYN -1) / ulTmpScaleYN;

    	//Scale up function: UP((length_h-1)*Nh/Mh) * UP((length_v-1)*Nv/Mv)            
        usUnScaleWidth  = 3 + ((grabctl->ulOutEdX - grabctl->ulOutStX) * ulTmpScaleXM) / ulTmpScaleXN;
	    usUnScaleHeight = 3 + ((grabctl->ulOutEdY - grabctl->ulOutStY) * ulTmpScaleYM) / ulTmpScaleYN;

	    if ((x_st + usUnScaleWidth - 1) > fitrange->ulInWidth) {
	    	usUnScaleWidth = fitrange->ulInWidth - x_st + 1;
	    }
	    
	    if ((y_st + usUnScaleHeight - 1) > fitrange->ulInHeight) {
	    	usUnScaleHeight = fitrange->ulInHeight - y_st + 1;
	    }
        
        /* Scaler(2nd) grab needed pixels then Output(3rd) grab all pixels */
        fitrange->ulInGrabX			= (grabctl->ulOutStX * ulTmpScaleXM + ulTmpScaleXN -1) / ulTmpScaleXN;
        fitrange->ulInGrabW 		= usUnScaleWidth;
        fitrange->ulInGrabY 		= (grabctl->ulOutStY * ulTmpScaleYM + ulTmpScaleYN -1) / ulTmpScaleYN;
        fitrange->ulInGrabH 		= usUnScaleHeight;
        fitrange->ulDummyInPixelX 	= 0;
        fitrange->ulDummyInPixelY 	= 0;
 		
 		grabctl->ulOutStX 			= 1;
    	grabctl->ulOutStY 			= 1;
	    grabctl->ulOutEdX 			= grabctl->ulOutStX + fitrange->ulOutWidth - 1;
	    grabctl->ulOutEdY 			= grabctl->ulOutStY + fitrange->ulOutHeight - 1;  
	}
	else {
	    /* Scaler(2nd) grab all pixels then Output(3rd) grab needed pixels */
        fitrange->ulInGrabX 		= 1;
        fitrange->ulInGrabW 		= fitrange->ulInWidth;
        fitrange->ulInGrabY 		= 1;
        fitrange->ulInGrabH 		= fitrange->ulInHeight;
        fitrange->ulDummyInPixelX 	= 0;
        fitrange->ulDummyInPixelY 	= 0;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_GetBestOutGrabRange
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_GetBestOutGrabRange(MMP_SCAL_FIT_RANGE *fitrange, MMP_SCAL_GRAB_CTRL *grabctl)
{
    MMP_USHORT  x_scale = 0, y_scale = 0;
    MMP_ULONG 	ulTmpScaleXN = 0, ulTmpScaleXM = 0, ulTmpScaleYN = 0, ulTmpScaleYM = 0;
    
    grabctl->ulScaleM = grabctl->ulScaleXM = grabctl->ulScaleYM = fitrange->ulFitResol;
	
	if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT ||
		fitrange->fitmode == MMP_SCAL_FITMODE_IN) 
	{	
	    x_scale = (fitrange->ulOutWidth * grabctl->ulScaleM + fitrange->ulInWidth - 1) / fitrange->ulInWidth;
	    y_scale = (fitrange->ulOutHeight * grabctl->ulScaleM + fitrange->ulInHeight - 1) / fitrange->ulInHeight;
	}
	else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) 
	{	
	    x_scale = (fitrange->ulOutWidth * grabctl->ulScaleXM + fitrange->ulInWidth - 1) / fitrange->ulInWidth;
	    y_scale = (fitrange->ulOutHeight * grabctl->ulScaleYM + fitrange->ulInHeight - 1) / fitrange->ulInHeight;
	}	
	
	/* Calculate the best fit scaling ratio */
	if ((fitrange->ulOutHeight > fitrange->ulInHeight) || 
		(fitrange->ulOutWidth  > fitrange->ulInWidth)) 
	{
		if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT) {

	    	while ((((fitrange->ulInHeight - 1) * y_scale + grabctl->ulScaleM - 1) / grabctl->ulScaleM)
	    			< fitrange->ulOutHeight)
    			y_scale++;
	    	while ((((fitrange->ulInWidth - 1) * x_scale + grabctl->ulScaleM - 1) / grabctl->ulScaleM)
	    			< fitrange->ulOutWidth)
    			x_scale++;
		}
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN) {

	    	while ((((fitrange->ulInHeight - 1) * y_scale + grabctl->ulScaleM - 1) / grabctl->ulScaleM)
	    			> fitrange->ulOutHeight)
    			y_scale--;
	    	while ((((fitrange->ulInWidth - 1) * x_scale + grabctl->ulScaleM - 1) / grabctl->ulScaleM)
	    			> fitrange->ulOutWidth)
    			x_scale--;
		}	    		
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {

	    	while ((((fitrange->ulInHeight - 1) * y_scale + grabctl->ulScaleYM - 1) / grabctl->ulScaleYM)
	    			< fitrange->ulOutHeight)
    			y_scale++;
	    	while ((((fitrange->ulInWidth - 1) * x_scale + grabctl->ulScaleXM - 1) / grabctl->ulScaleXM)
	    			< fitrange->ulOutWidth)
    			x_scale++;
		}
	}
	else {

		if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT) {

		    while (((fitrange->ulInHeight) * y_scale) < (fitrange->ulOutHeight * grabctl->ulScaleM))
    			y_scale++;
		    while (((fitrange->ulInWidth) * x_scale) < (fitrange->ulOutWidth * grabctl->ulScaleM))
	    		x_scale++;
		}
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN) {

	    	while (((fitrange->ulInHeight) * y_scale) > (fitrange->ulOutHeight * grabctl->ulScaleM))
    			y_scale--;
	    	while (((fitrange->ulInWidth) * x_scale) > (fitrange->ulOutWidth * grabctl->ulScaleM))
    			x_scale--;
		}	    		
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {

		    while (((fitrange->ulInHeight) * y_scale) < (fitrange->ulOutHeight * grabctl->ulScaleYM))
    			y_scale++;
		    while (((fitrange->ulInWidth) * x_scale) < (fitrange->ulOutWidth * grabctl->ulScaleXM))
	    		x_scale++;
		}
	}
	
    if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT) {
        grabctl->ulScaleN = (x_scale > y_scale) ? (x_scale):(y_scale);

        grabctl->ulScaleXN = x_scale;
        grabctl->ulScaleYN = y_scale;

	    ulTmpScaleXN = grabctl->ulScaleN;
	    ulTmpScaleXM = grabctl->ulScaleM;
	    ulTmpScaleYN = grabctl->ulScaleN;
	    ulTmpScaleYM = grabctl->ulScaleM;
   	}
	else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN) {
	    grabctl->ulScaleN = (x_scale > y_scale) ? (y_scale):(x_scale);

        grabctl->ulScaleXN = x_scale;
        grabctl->ulScaleYN = y_scale;

	    ulTmpScaleXN = grabctl->ulScaleN;
	    ulTmpScaleXM = grabctl->ulScaleM;
	    ulTmpScaleYN = grabctl->ulScaleN;
	    ulTmpScaleYM = grabctl->ulScaleM;
   	}
	else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {

	   	grabctl->ulScaleN = 0;
	    grabctl->ulScaleM = 0;

        grabctl->ulScaleXN = x_scale;
        grabctl->ulScaleYN = y_scale;

	    ulTmpScaleXN = grabctl->ulScaleXN;
	    ulTmpScaleXM = grabctl->ulScaleXM;
	    ulTmpScaleYN = grabctl->ulScaleYN;
	    ulTmpScaleYM = grabctl->ulScaleYM;
   	}
	
	/* Calculate the best fit grab range */
    if (ulTmpScaleXN > ulTmpScaleXM || ulTmpScaleYN > ulTmpScaleYM)
    {
		if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT ||
			fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {

	    	grabctl->ulOutStX = ((fitrange->ulInWidth - 1) * ulTmpScaleXN / ulTmpScaleXM - fitrange->ulOutWidth) / 2 + 1;
	    	grabctl->ulOutStY = ((fitrange->ulInHeight - 1) * ulTmpScaleYN / ulTmpScaleYM - fitrange->ulOutHeight) / 2 + 1;
		    grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;
		    grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
		}
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN) {
    	    grabctl->ulOutStX = 1;
       		grabctl->ulOutStY = 1;
		    grabctl->ulOutEdX = ((fitrange->ulInWidth - 1) * ulTmpScaleXN + ulTmpScaleXM - 1) / ulTmpScaleXM;
	    	grabctl->ulOutEdY = ((fitrange->ulInHeight - 1) * ulTmpScaleYN + ulTmpScaleYM - 1) / ulTmpScaleYM;
		}
	}
    else 
    {
		if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT ||
			fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) {

        	grabctl->ulOutStX = (fitrange->ulInWidth * ulTmpScaleXN / ulTmpScaleXM - fitrange->ulOutWidth) / 2 + 1;
        	grabctl->ulOutStY = (fitrange->ulInHeight * ulTmpScaleYN / ulTmpScaleYM - fitrange->ulOutHeight) / 2 + 1;
		    grabctl->ulOutEdX = grabctl->ulOutStX + fitrange->ulOutWidth - 1;
    		grabctl->ulOutEdY = grabctl->ulOutStY + fitrange->ulOutHeight - 1;
		}
		else if (fitrange->fitmode == MMP_SCAL_FITMODE_IN) {
	        grabctl->ulOutStX = 1;
       		grabctl->ulOutStY = 1;
		    grabctl->ulOutEdX = fitrange->ulInWidth * ulTmpScaleXN / ulTmpScaleXM;
	    	grabctl->ulOutEdY = fitrange->ulInHeight * ulTmpScaleYN / ulTmpScaleYM;
		}	    	
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_TransferCoordinate
//  Description : Transfer from scale ouput coordinate to input coordinate
//------------------------------------------------------------------------------
/** 
 * @brief This function transfer scaler coordinate.
 * 
 *  This function transfer scaler coordinate.
 * @param[in] usOutGrabXpos : the X position at the buffer of IBC
 * @param[in] usOutGrabYpos : the Y position at the buffer of IBC
 * @param[in] us1stGrabXpos : the X position at the buffer of the 1st grab of scaler
 * @param[in] us1stGrabYpos : the Y position at the buffer of the 1st grab of scaler 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Scaler_TransferCoordinate(MMP_SCAL_PIPEID 	pipeID,
                                       MMP_USHORT       usOutGrabXpos, 
                                       MMP_USHORT       usOutGrabYpos, 
                                       MMP_USHORT       *us1stGrabXpos, 
                                       MMP_USHORT       *us1stGrabYpos)
{
    MMP_USHORT  usHStart, usVStart, usHEnd, usVEnd;
    MMP_USHORT	usHratioN, usHratioM, usVratioN, usVratioM;
    MMP_BOOL    bEnable;
    MMP_SCAL_LPF_ABILITY lpfAbility;
    MMP_SCAL_LPF_DNSAMP  downsample;
    MMP_ULONG   ulTmpHOffset, ulTmpVOffset;
	
    MMPF_Scaler_GetGrabPosition(pipeID,
                                MMP_SCAL_GRAB_STAGE_OUT,
                                &usHStart,
                                &usHEnd,
                                &usVStart,
                                &usVEnd);

    ulTmpHOffset  = usHStart + usOutGrabXpos - 1;
    ulTmpVOffset  = usVStart + usOutGrabYpos - 1;

    MMPF_Scaler_GetScalingRatio(pipeID, 
                                &usHratioN,
                                &usHratioM,
                                &usVratioN,
                                &usVratioM);

    if (ulTmpHOffset > 1) {
        ulTmpHOffset = (ulTmpHOffset * usHratioM + usHratioN - 1) / usHratioN;

        if (usHratioN > usHratioM)
            ulTmpHOffset++;
    }
    else {
        ulTmpHOffset = 0;
    }

    if (ulTmpVOffset > 1) {
        ulTmpVOffset = (ulTmpVOffset  * usVratioM + usVratioN - 1)/ usVratioN;

        if (usVratioN > usVratioM)
            ulTmpVOffset++;
    }
    else {
        ulTmpVOffset = 0;
    }

    MMPF_Scaler_GetGrabPosition(pipeID,
                                MMP_SCAL_GRAB_STAGE_SCA,
                                &usHStart,
                                &usHEnd,
                                &usVStart,
                                &usVEnd);

    if (ulTmpHOffset > (usHEnd - usHStart + 1))
        ulTmpHOffset = usHEnd - usHStart + 1;
        
    if (ulTmpVOffset > (usVEnd - usVStart + 1))
        ulTmpVOffset = usVEnd - usVStart + 1;

    ulTmpHOffset = usHStart + ulTmpHOffset - 1;
    ulTmpVOffset = usVStart + ulTmpVOffset - 1;

    MMPF_Scaler_GetLPFEnable(pipeID, &bEnable);

    MMPF_Scaler_GetGrabPosition(pipeID,
                                MMP_SCAL_GRAB_STAGE_LPF,
                                &usHStart,
                                &usHEnd,
                                &usVStart,
                                &usVEnd);

    if(bEnable) 
    {
        MMPF_Scaler_CheckLPFAbility(pipeID, &lpfAbility);

        if(lpfAbility & MMP_SCAL_LPF_ABILITY_DN_EN) {
            MMPF_Scaler_GetLPFDownSample(pipeID, &downsample);

            if(downsample == MMP_SCAL_LPF_DNSAMP_1_2) {
                ulTmpHOffset  <<= 1;
                ulTmpVOffset  <<= 1;
            }
            else if(downsample == MMP_SCAL_LPF_DNSAMP_1_4) {
                ulTmpHOffset  <<= 2;
                ulTmpVOffset  <<= 2;
            }
        }
    }

    if (ulTmpHOffset == 0)
        ulTmpHOffset = usHStart;

    if (ulTmpVOffset == 0)
        ulTmpVOffset = usVStart;

    if (ulTmpHOffset > (usHEnd - usHStart + 1))
        ulTmpHOffset = usHEnd - usHStart + 1;

    if (ulTmpVOffset > (usVEnd - usVStart + 1))
        ulTmpVOffset = usVEnd - usVStart + 1;

    *us1stGrabXpos = (MMP_USHORT)ulTmpHOffset;
    *us1stGrabYpos = (MMP_USHORT)ulTmpVOffset;

    return MMP_ERR_NONE;
}

#ifdef SCAL_FUNC_DBG
//------------------------------------------------------------------------------
//  Function    : MMPF_Scaler_DumpSetting
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Scaler_DumpSetting(MMP_UBYTE			*pFuncName,
								MMP_SCAL_FIT_RANGE 	*fitrange,
                                MMP_SCAL_GRAB_CTRL 	*grabctl)
{
#if defined(ALL_FW)
	printc(">>> %s\r\n", pFuncName);

    printc("[Mode:%d,Type:%d,IW:%d,IH:%d,GX:%d,GY:%d,GW:%d,GH:%d,OW:%d,OH:%d]\r\n",
        	fitrange->fitmode,	 fitrange->scalerType,
        	fitrange->ulInWidth, fitrange->ulInHeight, 
        	fitrange->ulInGrabX, fitrange->ulInGrabY, 
       	 	fitrange->ulInGrabW, fitrange->ulInGrabH, 
        	fitrange->ulOutWidth, fitrange->ulOutHeight);

    printc("[DummyInX,Y:%d,%d,DummyOutX,Y:%d,%d]\r\n",
        	fitrange->ulDummyInPixelX, fitrange->ulDummyInPixelY,
        	fitrange->ulDummyOutPixelX,fitrange->ulDummyOutPixelY);  

    printc("[N:%d,M:%d,XN:%d,XM:%d,YN:%d,YM:%d]\r\n", 
        	grabctl->ulScaleN,grabctl->ulScaleM,
        	grabctl->ulScaleXN,grabctl->ulScaleXM,
        	grabctl->ulScaleYN,grabctl->ulScaleYM);

    printc("[OUTXS:%d,OUTXE:%d,OUTYS:%d,OUTYE:%d]\r\n", 
        	grabctl->ulOutStX, grabctl->ulOutEdX, 
        	grabctl->ulOutStY, grabctl->ulOutEdY);
#endif    
	return MMP_ERR_NONE;
}
#endif

/// @}
