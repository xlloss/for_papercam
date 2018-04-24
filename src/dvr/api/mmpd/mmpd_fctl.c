//==============================================================================
//
//  File        : mmpd_fctl.c
//  Description : Ritian Flow Control driver function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

/**
*  @file mmpd_fctl.c
*  @brief The FLOW control functions
*  @author Penguin Torng
*  @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "lib_retina.h"
#include "mmp_lib.h"
#include "mmpd_system.h"
#include "mmpd_fctl.h"
#include "mmpd_scaler.h"
#include "mmph_hif.h"
#include "mmpf_ldc.h"
#include "mmpf_ibc.h"
#include "mmpf_mci.h"
#include "mmpf_graphics.h"

/** @addtogroup MMPD_FCTL
 *  @{
 */

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMPD_FCTL_ATTR   m_FctlAttr[MAX_PIPELINE_NUM];
static MMPD_FCTL_PIPE   m_stPipe[MMP_SCAL_PIPE_NUM];

//==============================================================================
//
//                              MACRO FUNCTION
//
//==============================================================================

#define FCTL_PIPE_ID(pipe)  (pipe - &m_stPipe[0])

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_SetPipeAttrForIbcFB
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_SetPipeAttrForIbcFB(MMPD_FCTL_ATTR *pAttr)
{	
   	MMP_ULONG			i;
   	MMP_ERR				mmpstatus = -1; 
   	MMP_SCAL_COLORMODE 	scal_outcolor = 0;
   	MMP_ICO_PIPE_ATTR	icoAttr;
   	MMP_IBC_PIPE_ATTR	ibcAttr;
   	MMP_USHORT			usScalInW, usScalOutW;

	/* Set Pipe Linked Sensor */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ubPipeLinkedSnr);
	MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PIPE_LINKED_SNR);
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	
	/* Config IBC Module */
    switch (pAttr->colormode) {

    case MMP_DISPLAY_COLOR_RGB565:
		ibcAttr.colorformat = MMP_IBC_COLOR_RGB565;
        scal_outcolor 		= MMP_SCAL_COLOR_RGB565;
        break;
    case MMP_DISPLAY_COLOR_RGB888:
		ibcAttr.colorformat = MMP_IBC_COLOR_RGB888;
        scal_outcolor 		= MMP_SCAL_COLOR_RGB888;
        break;
    case MMP_DISPLAY_COLOR_YUV422:
		ibcAttr.colorformat = MMP_IBC_COLOR_YUV444_2_YUV422_UYVY;
        scal_outcolor 		= MMP_SCAL_COLOR_YUV444;
		break;
    case MMP_DISPLAY_COLOR_YUV420:
		ibcAttr.colorformat = MMP_IBC_COLOR_I420;
        scal_outcolor 		= MMP_SCAL_COLOR_YUV444;
        break;
    case MMP_DISPLAY_COLOR_YUV420_INTERLEAVE:
		ibcAttr.colorformat = MMP_IBC_COLOR_NV12;
        scal_outcolor 		= MMP_SCAL_COLOR_YUV444;
        break;
    case MMP_DISPLAY_COLOR_Y:
        if (pAttr->fctllink.ibcpipeID == MMP_IBC_PIPE_4)
            return MMPD_Fctl_SetSubPipeAttr(pAttr);
        ibcAttr.colorformat = MMP_IBC_COLOR_YUV420_LUMA_ONLY;
        scal_outcolor 		= MMP_SCAL_COLOR_YUV444;
        break;
    }

	ibcAttr.ulBaseAddr      = pAttr->ulBaseAddr[0];
	ibcAttr.ulBaseUAddr     = pAttr->ulBaseUAddr[0];
	ibcAttr.ulBaseVAddr     = pAttr->ulBaseVAddr[0];
    ibcAttr.ulLineOffset    = 0;
    ibcAttr.InputSource     = pAttr->fctllink.icopipeID;
    if ( (int)ibcAttr.colorformat != (int)MMP_DISPLAY_COLOR_Y)
	    ibcAttr.function    = MMP_IBC_FX_TOFB;
    else
        ibcAttr.function    = MMP_IBC_FX_FB_GRAY;
	ibcAttr.bMirrorEnable   = MMP_FALSE;
    #if (MCR_V2_UNDER_DBG)
    /* Have to reset MCI byte count to 256. Otherwise, ICON overflowed. */
    if ( (int)ibcAttr.colorformat != (int)MMP_DISPLAY_COLOR_Y)
        MMPD_IBC_SetMCI_ByteCount(  pAttr->fctllink.ibcpipeID,
                                    MMPF_MCI_BYTECNT_SEL_256BYTE);
    else
        MMPD_IBC_SetMCI_ByteCount(  pAttr->fctllink.ibcpipeID,
                                    MMPF_MCI_BYTECNT_SEL_128BYTE);
    #endif
	MMPD_IBC_SetAttributes(pAttr->fctllink.ibcpipeID, &ibcAttr);

	/* Config Icon Module */
    icoAttr.inputsel 	= pAttr->fctllink.scalerpath;
	icoAttr.bDlineEn 	= MMP_TRUE;
	icoAttr.usFrmWidth 	= pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1;
    MMPD_Icon_ResetModule(pAttr->fctllink.icopipeID);
	MMPD_Icon_SetDLAttributes(pAttr->fctllink.icopipeID, &icoAttr);
	MMPD_Icon_SetDLEnable(pAttr->fctllink.icopipeID, MMP_TRUE);
	
	/* Config Scaler Module */
    MMPD_Scaler_SetOutColor(pAttr->fctllink.scalerpath, scal_outcolor);

	if (scal_outcolor == MMP_SCAL_COLOR_RGB565 || 
		scal_outcolor == MMP_SCAL_COLOR_RGB888) {
		MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE, MMP_SCAL_COLRMTX_FULLRANGE_TO_RGB);
	}
	else {
	    #if (CCIR656_FORCE_SEL_BT601)
		MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE, MMP_SCAL_COLRMTX_FULLRANGE_TO_BT601);
		#else
		MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE, MMP_SCAL_COLRMTX_YUV_FULLRANGE);
		#endif
	}

	MMPD_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
	MMPD_Scaler_SetLPF(pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
	
	/* Check Bypass Scaler or not */
	MMPD_Scaler_GetGrabRange(pAttr->fctllink.scalerpath, MMP_SCAL_GRAB_STAGE_SCA, &usScalInW, NULL);
	MMPD_Scaler_GetGrabRange(pAttr->fctllink.scalerpath, MMP_SCAL_GRAB_STAGE_OUT, &usScalOutW, NULL);

    if ((usScalOutW > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
        (usScalOutW <= usScalInW))
    {
        // Scaling down, the output width is limited
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
    }
    else if ((usScalInW > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
             (usScalOutW > usScalInW))
    {
        // Scaling up, the input width is limited
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
    }
    else {
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_FALSE);
    }

    MMPD_Scaler_SetPixelDelay(pAttr->fctllink.scalerpath, 1, 1);
    
    if (pAttr->bSetScalerSrc) {
        MMPD_Scaler_SetPath(pAttr->fctllink.scalerpath, pAttr->scalsrc, MMP_TRUE);
    }
    MMPD_Scaler_SetEnable(pAttr->fctllink.scalerpath, MMP_TRUE);

    /* Set preview buffer count and address */
	{
	    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
	    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
		MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, pAttr->usBufCnt);
    	MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_COUNT);
    	
		for (i = 0; i < pAttr->usBufCnt; i++) 
		{   
	        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
	        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, i);
	        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ulBaseAddr[i]);
	        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8, pAttr->ulBaseUAddr[i]);
	        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 12, pAttr->ulBaseVAddr[i]);

	        mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_ADDRESS);
		}

        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1);
        MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_WIDTH);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID); 
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, pAttr->grabctl.ulOutEdY - pAttr->grabctl.ulOutStY + 1);
        MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_HEIGHT);

		MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	}
	
	/* Set rotate buffer count and address */
    if (pAttr->bUseRotateDMA) {
    
        MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->usRotateBufCnt);  
        MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_ROTATE_BUF | BUFFER_COUNT);
        
    	for (i = 0; i < pAttr->usRotateBufCnt; i++) {

			MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
           	MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, i);
            MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ulRotateAddr[i]);
            MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8, pAttr->ulRotateUAddr[i]);
            MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 12, pAttr->ulRotateVAddr[i]);

            MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_ROTATE_BUF | BUFFER_ADDRESS);
		}
		
		MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	}
    
	/* Reset Link pipes */
	MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_NONE);
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	
	m_FctlAttr[pAttr->fctllink.ibcpipeID] = *pAttr;

	return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_SetPipeAttrForH264FB
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_SetPipeAttrForH264FB(MMPD_FCTL_ATTR *pAttr)
{	
   	MMP_ULONG			i;
   	MMP_ERR				mmpstatus = -1;
   	MMP_ICO_PIPE_ATTR 	icoAttr;
   	MMP_IBC_PIPE_ATTR	ibcAttr;
   	MMP_USHORT			usScalInW, usScalOutW;
   	
	/* Set Pipe Linked Sensor */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ubPipeLinkedSnr);
	MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PIPE_LINKED_SNR);
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);

	/* Config IBC Module */
    switch (pAttr->colormode) {

    case MMP_DISPLAY_COLOR_YUV420_INTERLEAVE:
		ibcAttr.colorformat = MMP_IBC_COLOR_NV12;
        break;
    default:
		RTNA_DBG_Str(0, "InCorrect H264 supported format\r\n");
		return MMP_FCTL_ERR_PARAMETER;
    	break;
    }

	ibcAttr.ulBaseAddr 	    = pAttr->ulBaseAddr[0];
	ibcAttr.ulBaseUAddr     = pAttr->ulBaseUAddr[0];
	ibcAttr.ulBaseVAddr 	= pAttr->ulBaseVAddr[0];
    ibcAttr.ulLineOffset 	= 0;
    ibcAttr.InputSource     = pAttr->fctllink.icopipeID;
	ibcAttr.function 		= MMP_IBC_FX_TOFB;
	ibcAttr.bMirrorEnable   = MMP_FALSE;
    #if (MCR_V2_UNDER_DBG)
    /* Have to reset MCI byte count to 256. Otherwise, ICON overflowed. */
    MMPD_IBC_SetMCI_ByteCount(pAttr->fctllink.ibcpipeID, MMPF_MCI_BYTECNT_SEL_256BYTE);
    #endif
	MMPD_IBC_SetAttributes(pAttr->fctllink.ibcpipeID, &ibcAttr);

	/* Config Icon Module */
    icoAttr.inputsel 	= pAttr->fctllink.scalerpath;
	icoAttr.bDlineEn 	= MMP_TRUE;
	icoAttr.usFrmWidth 	= pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1;
    MMPD_Icon_ResetModule(pAttr->fctllink.icopipeID);
	MMPD_Icon_SetDLAttributes(pAttr->fctllink.icopipeID, &icoAttr);
	MMPD_Icon_SetDLEnable(pAttr->fctllink.icopipeID, MMP_TRUE);
	
	/* Config Scaler Module */
    MMPD_Scaler_SetOutColor(pAttr->fctllink.scalerpath, MMP_SCAL_COLOR_YUV444);
	MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE, MMP_SCAL_COLRMTX_FULLRANGE_TO_BT601);
	MMPD_Scaler_SetStopEnable(pAttr->fctllink.scalerpath, MMP_SCAL_STOP_SRC_H264, MMP_TRUE);

    MMPD_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
    MMPD_Scaler_SetLPF(pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));  

	/* Check Bypass Scaler or not */
	MMPD_Scaler_GetGrabRange(pAttr->fctllink.scalerpath, MMP_SCAL_GRAB_STAGE_SCA, &usScalInW, NULL);
	MMPD_Scaler_GetGrabRange(pAttr->fctllink.scalerpath, MMP_SCAL_GRAB_STAGE_OUT, &usScalOutW, NULL);

    if ((usScalOutW > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
        (usScalOutW <= usScalInW))
    {
        // Scaling down, the output width is limited
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
    }
    else if ((usScalInW > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
             (usScalOutW > usScalInW))
    {
        // Scaling up, the input width is limited
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
    }
    else {
        MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_FALSE);
    }

    MMPD_Scaler_SetPixelDelay(pAttr->fctllink.scalerpath, 1, 1);
    
    if (pAttr->bSetScalerSrc) {
		MMPD_Scaler_SetPath(pAttr->fctllink.scalerpath, pAttr->scalsrc, MMP_TRUE);    
	}
	MMPD_Scaler_SetEnable(pAttr->fctllink.scalerpath, MMP_TRUE);
    
    /* Set encode buffer (current buffer) address */
	for (i = 0; i < pAttr->usBufCnt; i++) 
	{
	    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, i);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ulBaseAddr[i]);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8, pAttr->ulBaseUAddr[i]);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 12, pAttr->ulBaseVAddr[i]);

        mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_ADDRESS);
	    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	}

    /* Set encode buffer count */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, pAttr->usBufCnt);  
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_COUNT);
	
	/* Reset Link pipes */
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_NONE);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);

	m_FctlAttr[pAttr->fctllink.ibcpipeID] = *pAttr;

	return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_SetPipeAttrForH264Rt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_SetPipeAttrForH264Rt(MMPD_FCTL_ATTR *pAttr, MMP_ULONG ulEncWidth)
{
   	MMP_ERR				mmpstatus = MMP_ERR_NONE;
   	MMP_ICO_PIPE_ATTR	icoAttr;
   	MMP_IBC_PIPE_ATTR	ibcAttr;

	/* Set Pipe Linked Sensor */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pAttr->ubPipeLinkedSnr);
	MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PIPE_LINKED_SNR);
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);

	/* Config IBC Module */
    switch (pAttr->colormode) {
     
    case MMP_DISPLAY_COLOR_YUV420_INTERLEAVE:
		ibcAttr.colorformat = MMP_IBC_COLOR_NV12;
        break;
    default:
		RTNA_DBG_Str(0, "InCorrect H264 supported format\r\n");
		return MMP_FCTL_ERR_PARAMETER;    
    	break;
    }

    ibcAttr.ulLineOffset 	= 0;
    ibcAttr.InputSource     = pAttr->fctllink.icopipeID;
	ibcAttr.function 		= MMP_IBC_FX_H264_RT;
	ibcAttr.bMirrorEnable   = MMP_FALSE;

	MMPD_IBC_SetAttributes(pAttr->fctllink.ibcpipeID, &ibcAttr);
    MMPD_IBC_SetRtPingPongAddr(pAttr->fctllink.ibcpipeID,
                               pAttr->ulBaseAddr[0],
                               pAttr->ulBaseUAddr[0],
                               pAttr->ulBaseVAddr[0],
                               pAttr->ulBaseAddr[1],
                               pAttr->ulBaseUAddr[1],
                               pAttr->ulBaseVAddr[1],
                               ulEncWidth);

    #if (MCR_V2_UNDER_DBG)
    /* Have to reset MCI byte count to 128 after DSC capture,
     * otherwise, no H.264 encode done, and ICON overflowed. */
    MMPD_IBC_SetMCI_ByteCount(pAttr->fctllink.ibcpipeID, MMPF_MCI_BYTECNT_SEL_128BYTE);
    #endif

	/* Config Icon Module */
    icoAttr.inputsel 	= pAttr->fctllink.scalerpath;
	icoAttr.bDlineEn 	= MMP_TRUE;
	icoAttr.usFrmWidth 	= pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1;
    MMPD_Icon_ResetModule(pAttr->fctllink.icopipeID);
	MMPD_Icon_SetDLAttributes(pAttr->fctllink.icopipeID, &icoAttr);
	MMPD_Icon_SetDLEnable(pAttr->fctllink.icopipeID, MMP_TRUE);
	
	/* Config Scaler Module */
    MMPD_Scaler_SetOutColor(pAttr->fctllink.scalerpath, MMP_SCAL_COLOR_YUV444);
	MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE, MMP_SCAL_COLRMTX_FULLRANGE_TO_BT601);
    MMPD_Scaler_SetStopEnable(pAttr->fctllink.scalerpath, MMP_SCAL_STOP_SRC_H264, MMP_TRUE);

    MMPD_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
    MMPD_Scaler_SetLPF(pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));

	if ((pAttr->fitrange).ulInGrabW < (pAttr->fitrange).ulOutWidth || 
		(pAttr->fitrange).ulInGrabH < (pAttr->fitrange).ulOutHeight) {
		// RealTime mode need to set pixel delay to prevent H264 buffer overlap.
    	MMPD_Scaler_SetPixelDelay(pAttr->fctllink.scalerpath, 16, 20); // For ISP:480MHz, G0:264MHz case
    	//MMPD_Scaler_SetPixelDelay(pAttr->fctllink.scalerpath, 11, 20); // For ISP:528MHz, G0:264MHz case
	}
	else {
	    MMPD_Scaler_SetPixelDelay(pAttr->fctllink.scalerpath, 1, 1);
	}

    if (pAttr->bSetScalerSrc) {
		MMPD_Scaler_SetPath(pAttr->fctllink.scalerpath, pAttr->scalsrc, MMP_TRUE);    
	}
	MMPD_Scaler_SetEnable(pAttr->fctllink.scalerpath, MMP_TRUE);

	/* Reset Link pipes */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_NONE);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    	
	m_FctlAttr[pAttr->fctllink.ibcpipeID] = *pAttr;

	return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_SetPipeAttrForJpeg
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_SetPipeAttrForJpeg(MMPD_FCTL_ATTR *pAttr,
                                     MMP_BOOL       bSetScalerGrab,
                                     MMP_BOOL       bSrcIsBT601)
{
   	MMP_ERR				mmpstatus = MMP_ERR_NONE;
   	MMP_ICO_PIPE_ATTR 	icoAttr;
   	MMP_IBC_PIPE_ATTR	ibcAttr;

	/* Config IBC Module */
    ibcAttr.InputSource     = pAttr->fctllink.icopipeID;
	ibcAttr.function 		= MMP_IBC_FX_JPG;
	ibcAttr.bMirrorEnable   = MMP_FALSE;
	MMPD_IBC_SetAttributes(pAttr->fctllink.ibcpipeID, &ibcAttr);
    	
	/* Config Icon Module */
    icoAttr.inputsel 	= pAttr->fctllink.scalerpath;
	icoAttr.bDlineEn 	= MMP_TRUE;
	icoAttr.usFrmWidth 	= pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1;
    MMPD_Icon_ResetModule(pAttr->fctllink.icopipeID);
	MMPD_Icon_SetDLAttributes(pAttr->fctllink.icopipeID, &icoAttr);
	MMPD_Icon_SetDLEnable(pAttr->fctllink.icopipeID, MMP_TRUE);
	
	/* Config Scaler Module */
    MMPD_Scaler_SetOutColor(pAttr->fctllink.scalerpath, MMP_SCAL_COLOR_YUV444);
    if (bSrcIsBT601) {
        MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE,
                                         MMP_SCAL_COLRMTX_BT601_TO_FULLRANGE);
    }
    else {
	    MMPD_Scaler_SetOutColorTransform(pAttr->fctllink.scalerpath, MMP_TRUE,
                                         MMP_SCAL_COLRMTX_YUV_FULLRANGE);
    }
	
	if (bSetScalerGrab) {
    	MMPD_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
    	MMPD_Scaler_SetLPF(pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));

		if ((pAttr->fitrange.ulOutWidth > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
            (pAttr->fitrange.ulOutWidth <= pAttr->fitrange.ulInGrabW))
        {
            // Scaling down, the output width is limited
            MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
        }
        else if ((pAttr->fitrange.ulInGrabW > MMPD_Scaler_GetMaxDelayLineWidth(pAttr->fctllink.scalerpath)) &&
                 (pAttr->fitrange.ulOutWidth > pAttr->fitrange.ulInGrabW))
        {
            // Scaling up, the input width is limited
            MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_TRUE);
        }
        else {
            MMPD_Scaler_BypassScaler(pAttr->fctllink.scalerpath, MMP_FALSE);
        }
	}
	
    if (pAttr->bSetScalerSrc) {
		MMPD_Scaler_SetPath(pAttr->fctllink.scalerpath, pAttr->scalsrc, MMP_TRUE);
	}
    MMPD_Scaler_SetEnable(pAttr->fctllink.scalerpath, MMP_TRUE);

	return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_SetSubPipeAttr
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_SetSubPipeAttr(MMPD_FCTL_ATTR *pAttr)
{	
   	MMP_ULONG					i;
   	MMP_ERR						mmpstatus = -1;
   	MMP_ICO_PIPE_ATTR 			icoSubAttr;
   	MMP_IBC_PIPE_ATTR			ibcSubAttr;
    
    if (pAttr->fctllink.ibcpipeID != MMP_IBC_PIPE_4) {
    	return MMP_ERR_NONE;
    }

	/* Config IBC Module */
	ibcSubAttr.ulBaseAddr		= pAttr->ulBaseAddr[0];
	ibcSubAttr.bMirrorEnable	= MMP_FALSE;
	ibcSubAttr.usMirrorWidth 	= 0;
	MMPD_IBC_SetAttributes(pAttr->fctllink.ibcpipeID, &ibcSubAttr);
    MMPD_IBC_SetStoreEnable(pAttr->fctllink.ibcpipeID, MMP_TRUE);
    
	/* Config Icon Module */
    icoSubAttr.inputsel 	= pAttr->fctllink.scalerpath;
	icoSubAttr.bDlineEn 	= MMP_TRUE;
	icoSubAttr.usFrmWidth 	= pAttr->grabctl.ulOutEdX - pAttr->grabctl.ulOutStX + 1;
    MMPD_Icon_ResetModule(pAttr->fctllink.icopipeID);
	MMPD_Icon_SetDLAttributes(pAttr->fctllink.icopipeID, &icoSubAttr);
	MMPD_Icon_SetDLEnable(pAttr->fctllink.icopipeID, MMP_TRUE);
	
	/* Config Scaler Module */
    MMPD_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl)); 
    MMPD_Scaler_SetLPF(pAttr->fctllink.scalerpath, &(pAttr->fitrange), &(pAttr->grabctl));
    
    if (pAttr->bSetScalerSrc) {
		MMPD_Scaler_SetPath(pAttr->fctllink.scalerpath, pAttr->scalsrc, MMP_TRUE);    
	}
	MMPD_Scaler_SetEnable(pAttr->fctllink.scalerpath, MMP_TRUE);
    
    /* Set preview buffer count and address */
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
	MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, pAttr->usBufCnt);
	MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_COUNT);
	
	for (i = 0; i < pAttr->usBufCnt; i++) 
	{   
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0,  pAttr->fctllink.ibcpipeID);
        MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2,  i);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4,  pAttr->ulBaseAddr[i]);
		MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8,  0);
		MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 12, 0);
		
        mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_BUF | BUFFER_ADDRESS);
	}
	
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
	/* Set Link Type */
	MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pAttr->fctllink.ibcpipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_NONE);
	MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	
	m_FctlAttr[pAttr->fctllink.ibcpipeID] = *pAttr;

	return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_GetAttributes
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_GetAttributes(MMP_IBC_PIPEID pipeID, MMPD_FCTL_ATTR *pAttr)
{
	*pAttr = m_FctlAttr[pipeID];

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_EnablePreview
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_EnablePreview(MMP_UBYTE ubSnrSel, MMP_IBC_PIPEID pipeID, MMP_BOOL bEnable, MMP_BOOL bCheckFrameEnd)
{
	MMP_ERR	mmpstatus;

	if (bEnable) {

    	MMPD_System_EnableClock(MMPD_SYS_CLK_GRA, 	MMP_TRUE);
        MMPD_System_EnableClock(MMPD_SYS_CLK_DMA, 	MMP_TRUE);
		MMPD_System_EnableClock(MMPD_SYS_CLK_SCALE, MMP_TRUE);
		MMPD_System_EnableClock(MMPD_SYS_CLK_ICON, 	MMP_TRUE);
        MMPD_System_EnableClock(MMPD_SYS_CLK_IBC, 	MMP_TRUE);
	
	    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
	    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, ubSnrSel);
	    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pipeID);
		MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8, bCheckFrameEnd);

		mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_ENABLE | ENABLE_PREVIEW);
        
		if (MMPH_HIF_GetParameterL(GRP_IDX_FLOWCTL, 0) == 0) {
		    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	        return MMP_ERR_NONE;
	    }    
	    else {
	        MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	        return MMP_DISPLAY_ERR_FRAME_END;
	    }    
	}
	else {

        MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, ubSnrSel);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pipeID);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 8, bCheckFrameEnd);

		mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_ENABLE | DISABLE_PREVIEW);
		MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
        
        MMPD_Scaler_SetEnable(pipeID, MMP_FALSE);
		
		// It should ensure all pipe are disabled then close clock.
		//MMPD_System_EnableClock(MMPD_SYS_CLK_GRA, 	MMP_FALSE);
        //MMPD_System_EnableClock(MMPD_SYS_CLK_DMA, 	MMP_FALSE);
		//MMPD_System_EnableClock(MMPD_SYS_CLK_IBC, 	MMP_FALSE);
		//MMPD_System_EnableClock(MMPD_SYS_CLK_SCALE, 	MMP_FALSE);
		//MMPD_System_EnableClock(MMPD_SYS_CLK_ICON, 	MMP_FALSE);

        return mmpstatus;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_EnablePipe
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_EnablePipe(MMP_UBYTE ubSnrSel, MMP_IBC_PIPEID pipeID, MMP_BOOL bEnable)
{
	MMP_ERR	mmpstatus;

	if (bEnable) {
		MMPD_System_EnableClock(MMPD_SYS_CLK_SCALE, MMP_TRUE);
		MMPD_System_EnableClock(MMPD_SYS_CLK_ICON, 	MMP_TRUE);
        MMPD_System_EnableClock(MMPD_SYS_CLK_IBC, 	MMP_TRUE);
	
	    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
	    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, ubSnrSel);
	    MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pipeID);

		mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_ENABLE | ENABLE_PIPE);
		if (MMPH_HIF_GetParameterL(GRP_IDX_FLOWCTL, 0) == 0) {
		    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	        return MMP_ERR_NONE;
	    }    
	    else {
	        MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
	        return MMP_DISPLAY_ERR_FRAME_END;
	    }    
	}
	else {
        MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 0, ubSnrSel);
        MMPH_HIF_SetParameterL(GRP_IDX_FLOWCTL, 4, pipeID);

		mmpstatus = MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_PREVIEW_ENABLE | DISABLE_PIPE);
		MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);

        /* Reset scaler source to JPG, and disable it */
        MMPD_Scaler_SetPath(pipeID, MMP_SCAL_SOURCE_JPG, MMP_FALSE);
        MMPD_Scaler_SetEnable(pipeID, MMP_FALSE);

		// It should ensure all pipe are disabled then close clock.
		MMPD_System_EnableClock(MMPD_SYS_CLK_IBC,   MMP_FALSE);
		MMPD_System_EnableClock(MMPD_SYS_CLK_SCALE, MMP_FALSE);
		MMPD_System_EnableClock(MMPD_SYS_CLK_ICON,  MMP_FALSE);

        return mmpstatus;
	}
}

#if 0
void ____Link_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_ResetIBCLinkType
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_ResetIBCLinkType(MMP_IBC_PIPEID pipeID)
{
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_NONE);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_LinkPipeToVideo
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_LinkPipeToVideo(MMP_IBC_PIPEID pipeID, MMP_USHORT ubEncId)
{
	if (m_FctlAttr[pipeID].colormode != MMP_DISPLAY_COLOR_YUV420_INTERLEAVE) {
		return MMP_FCTL_ERR_PARAMETER;
    }
    
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pipeID);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, ubEncId);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_VIDEO);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_LinkPipeToGraphic
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_LinkPipeToGraphic(MMP_IBC_PIPEID pipeID)
{
   	MMP_GRAPHICS_BUF_ATTR  	bufAttr = {0, };
   	MMP_GRAPHICS_RECT		srcrect = {0, };

    bufAttr.usWidth	 = m_FctlAttr[pipeID].grabctl.ulOutEdX - m_FctlAttr[pipeID].grabctl.ulOutStX + 1;
    bufAttr.usHeight = m_FctlAttr[pipeID].grabctl.ulOutEdY - m_FctlAttr[pipeID].grabctl.ulOutStY + 1;

    switch (m_FctlAttr[pipeID].colormode) {

	    case MMP_DISPLAY_COLOR_RGB565:
	        bufAttr.usLineOffset = bufAttr.usWidth*2;
	        bufAttr.colordepth 	 = MMP_GRAPHICS_COLORDEPTH_16;
	        break;
	    case MMP_DISPLAY_COLOR_RGB888:
	        bufAttr.usLineOffset = bufAttr.usWidth*3;
	        bufAttr.colordepth 	 = MMP_GRAPHICS_COLORDEPTH_24;
	        break;
	    case MMP_DISPLAY_COLOR_YUV422:
	        bufAttr.usLineOffset = bufAttr.usWidth*2;
	        bufAttr.colordepth 	 = MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY;
	        break;
	    case MMP_DISPLAY_COLOR_YUV420:
	        bufAttr.usLineOffset = bufAttr.usWidth;
	        bufAttr.colordepth 	 = MMP_GRAPHICS_COLORDEPTH_YUV420;
	        break;
	    case MMP_DISPLAY_COLOR_YUV420_INTERLEAVE:
	        bufAttr.usLineOffset = bufAttr.usWidth;
	        bufAttr.colordepth 	 = MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
	        break;
        default:
            //
            break;
    }

    srcrect.usLeft 		= 0;
    srcrect.usTop 		= 0;
    srcrect.usWidth 	= bufAttr.usWidth;
	srcrect.usHeight 	= bufAttr.usHeight;
	
	// Source buffer address will be assigned in the IBC ISR
	MMPD_Graphics_SetScaleAttr(&bufAttr, &srcrect, 1);
    
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | LINK_GRAPHIC);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_LinkPipeToGray
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_LinkPipeToWithId(MMP_IBC_PIPEID pipeID, MMP_ULONG id,MMP_ULONG module)
{
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pipeID);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 2, id);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | module);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_LinkPipeTo
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPD_Fctl_LinkPipeTo(MMP_IBC_PIPEID pipeID, MMP_ULONG module)
{
    MMPH_HIF_WaitSem(GRP_IDX_FLOWCTL, 0);
    MMPH_HIF_SetParameterW(GRP_IDX_FLOWCTL, 0, pipeID);
    MMPH_HIF_SendCmd(GRP_IDX_FLOWCTL, HIF_FCTL_CMD_SET_IBC_LINK_MODE | module);
    MMPH_HIF_ReleaseSem(GRP_IDX_FLOWCTL);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_AllocatePipe
//  Description : Allocate the pipe resource by the specified request.
//------------------------------------------------------------------------------
/** @brief Assign the pipe resource according to the request capability.

@return It return the id of available pipe for the request
*/
MMP_IBC_PIPEID MMPD_Fctl_AllocatePipe(MMP_ULONG width, MMPD_FCTL_LINK_TO dst)
{
    MMP_ULONG       p, max_w_sel = 0;
    MMPD_FCTL_PIPE  *pipe_sel = NULL;

    /* Search a available pipe which can satisfy the request */
    if ((dst == PIPE_LINK_FB_Y) &&
        (m_stPipe[MMP_IBC_PIPE_4].used == MMP_FALSE) &&
        (m_stPipe[MMP_IBC_PIPE_4].max_w >= width)) {

        // Give the highest priority to pipe4 as the pipe for Y output
        pipe_sel = &m_stPipe[MMP_SCAL_PIPE_4];
    }
    else {
        // Search a pipe whose capability is best-fit to the request
        for(p = MMP_IBC_PIPE_0; p < MMP_IBC_PIPE_MAX; p++) {
            if ((m_stPipe[p].used == MMP_FALSE) && (m_stPipe[p].link2 & dst)) {

                if (m_stPipe[p].max_w == width) {
                    pipe_sel = &m_stPipe[p];
                    break;
                }
                else if (m_stPipe[p].max_w > width) {
                    if (!pipe_sel || (m_stPipe[p].max_w < max_w_sel)) {
                        pipe_sel  = &m_stPipe[p];
                        max_w_sel = pipe_sel->max_w;
                    }
                }
            }
        }
    }

    if (!pipe_sel) // No pipe resource available
        return MMP_IBC_PIPE_MAX;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_IBC, MMP_TRUE);

    pipe_sel->used = MMP_TRUE;

    return pipe_sel->id;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_Fctl_ReleasePipe
//  Description : Release the pipe resource of the specified id
//------------------------------------------------------------------------------
/** @brief Resource the pipe resource of the specified id

@return It reports the status of the operation.
*/
MMP_ERR MMPD_Fctl_ReleasePipe(MMP_IBC_PIPEID id)
{
    if (id >= MMP_IBC_PIPE_MAX)
        return MMP_FCTL_ERR_PARAMETER;
    
    m_stPipe[id].used = MMP_FALSE;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_FALSE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_FALSE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_IBC, MMP_FALSE);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : Pipe_Module_Init
//  Description : Initialize the pipe resource
//------------------------------------------------------------------------------
/** @brief Initialize all pipes to link with same ID, and set the capability of
each pipe. The source of pipes are set to JPEG to avoid any data input in
unexpected time.

@return It reports the status of the operation.
*/
int Pipe_Module_Init(void)
{
    MMP_ICO_PIPE_ATTR icon;
    MMP_ULONG p;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_IBC, MMP_TRUE);

    /* Setup the the width capability for each pipe */
    m_stPipe[0].max_w = SCALER_PATH0_MAX_WIDTH;
    m_stPipe[1].max_w = SCALER_PATH1_MAX_WIDTH;
    m_stPipe[2].max_w = SCALER_PATH2_MAX_WIDTH;
    m_stPipe[3].max_w = SCALER_PATH3_MAX_WIDTH;
    m_stPipe[4].max_w = SCALER_PATH4_MAX_WIDTH;

    for(p = MMP_IBC_PIPE_0; p < MMP_IBC_PIPE_MAX; p++) {
        m_stPipe[p].id = p;
        m_stPipe[p].used = MMP_FALSE;
        if (p != MMP_IBC_PIPE_4)
            m_stPipe[p].link2 = PIPE_LINK_ALL;
        else
            m_stPipe[p].link2 = PIPE_LINK_FB_Y;

        /* Initialize scaler source to JPG, and default disable */
        MMPF_Scaler_SetInputSrc(p, MMP_SCAL_SOURCE_JPG, MMP_TRUE);
        MMPF_Scaler_SetEnable(p, MMP_FALSE);

        /* Connect icon & ibc into the same pipe link */
        icon.inputsel   = p;
        icon.bDlineEn 	= MMP_TRUE;
        icon.usFrmWidth	= m_stPipe[p].max_w;
        MMPF_Icon_SetDLAttributes(p, &icon);
        MMPF_IBC_SetInputSrc(p, p);
    }

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_SCALE, MMP_FALSE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_FALSE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_IBC, MMP_FALSE);

    return 0;
}

/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall"  */
/* #pragma O0 */
/* __attribute__((optimize("O0"))) */
ait_module_init(Pipe_Module_Init); 
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

/// @}

