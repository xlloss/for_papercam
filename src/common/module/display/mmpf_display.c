//==============================================================================
//
//  File        : mmpf_display.c
//  Description : Firmware Display Control Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "ait_utility.h"

#include "mmp_reg_gbl.h"
#include "mmp_reg_ibc.h"
#include "mmp_reg_vif.h"

#include "mmpf_mci.h"
#include "mmpf_hif.h"
#include "mmpf_display.h"
#include "mmpf_vif.h"
#if (VIDEO_R_EN)
#include "mmpf_mp4venc.h"
#endif
#include "mmpf_sensor.h"
#include "mmpf_ibc.h"
#include "mmpf_pio.h"
#include "mmpf_system.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define MAX_DISPLAY_PP_BUF_NUM	(4) // Max display ping-pong buffer num.

#define DISPLAY_DBG_LEVEL 		(0)

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

MMP_BOOL   		gbFrameExposureDone = MMP_FALSE;								///< For FLM panel use.

MMP_UBYTE   	gbPreviewBufferCount[MMP_IBC_PIPE_MAX]; 						///< Preview buffer count of ibc pipes
MMP_ULONG   	glPreviewBufAddr[MMP_IBC_PIPE_MAX][MAX_DISPLAY_PP_BUF_NUM];	 	///< Preview Y buffer of ibc pipes. 
MMP_ULONG   	glPreviewUBufAddr[MMP_IBC_PIPE_MAX][MAX_DISPLAY_PP_BUF_NUM];	///< Preview U buffer of ibc pipes. 
MMP_ULONG   	glPreviewVBufAddr[MMP_IBC_PIPE_MAX][MAX_DISPLAY_PP_BUF_NUM];	///< Preview V buffer of ibc pipes. 
MMP_UBYTE		gbExposureDoneBufIdx[MMP_IBC_PIPE_MAX];	 						///< Preview buffer index.
MMP_UBYTE   	gbIBCCurBufIdx[MMP_IBC_PIPE_MAX];								///< Preview buffer index.

MMP_UBYTE       gbIBCLinkEncId[MMP_IBC_PIPE_MAX];
MMP_BOOL        gbIBCReady[MMP_IBC_PIPE_MAX];

MMP_BOOL		gbPipeIsActive[MMP_IBC_PIPE_MAX] = {MMP_FALSE, MMP_FALSE};
MMP_UBYTE       gbPipeLinkedSnr[MMP_IBC_PIPE_MAX] = {PRM_SENSOR, PRM_SENSOR};
MMP_BOOL		m_bPipeLinkGraphic[MMP_IBC_PIPE_MAX] = {MMP_FALSE, MMP_FALSE};

MMP_IBC_LINK_TYPE		gIBCLinkType[MMP_IBC_PIPE_MAX];

/** @breif m_bReceiveStopPreviewSig for preview, created by MMPF_Sensor_Task()
MMPF_IBC_ISR() will release m_StopPreviewCtrlSem depending on m_bReceiveStopPreviewSig.
MMPF_Display_StartPreview() set m_bReceiveStopPreviewSig MMP_FALSE in order to open preview.
MMPF_Display_StopPreview()	set m_bReceiveStopPreviewSig MMP_TRUE in order to close preview.
*/
MMP_BOOL		m_bReceiveStopPreviewSig[MMP_IBC_PIPE_MAX] = {MMP_FALSE, MMP_FALSE};
MMP_UBYTE		m_ubStopPreviewSnrId = PRM_SENSOR;
MMP_BOOL 		m_bFirstSensorInitialize[VIF_SENSOR_MAX_NUM] = {MMP_TRUE, MMP_TRUE};
MMP_BOOL		m_bStartPreviewFrameEndSig[VIF_SENSOR_MAX_NUM] = {MMP_FALSE, MMP_FALSE};
MMP_BOOL		m_bStopPreviewCloseVifInSig[VIF_SENSOR_MAX_NUM] = {MMP_FALSE, MMP_FALSE};

MMPF_OS_SEMID 	m_StartPreviewFrameEndSem[VIF_SENSOR_MAX_NUM];
MMPF_OS_SEMID	m_StopPreviewCtrlSem[VIF_SENSOR_MAX_NUM];
MMPF_OS_SEMID	m_ISPOperationDoneSem;

#if (SENSOR_EN)
/**
@brief The status of preview display 
@details The status of any pipe set to preview display.
       It could be MMP_IBC_LINK_DISPLAY or MMP_IBC_LINK_ROTATE.
*/
static MMP_UBYTE    m_ubPrevwDispSts = MMP_FALSE;
#endif

//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================

extern MMP_UBYTE	    gbFrameEndFlag[];
extern MMP_USHORT       gsVidRecdCurBufMode;

#if defined(ALL_FW)
extern MMPF_OS_FLAGID   SENSOR_Flag;
#endif

#if (SENSOR_EN)
extern MMPF_SENSOR_FUNCTION *gsSensorFunction;
#endif

#if (VIDEO_R_EN)
extern MMPF_VIDENC_QUEUE gVidRecdFreeQueue[MAX_VIDEO_STREAM_NUM];
#endif

#if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
extern MMP_LDC_LINK 	m_LdcPathLink;
extern MMP_UBYTE 		m_ubSwiPipeLbTargetCnt;
extern MMP_UBYTE 		m_ubSwiPipeLbCurCnt;
extern MMP_UBYTE  		m_ubLdcLbSrcPipe;
extern MMP_BOOL			m_bRetriggerGRA;
extern MMP_USHORT       m_usLdcSliceIdx;
extern MMP_UBYTE		m_ubRawFetchSnrId;
extern MMP_ULONG 		m_ulLdcFrmDoneCnt;
#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
extern MMP_ULONG		gulLdcSliceStartTime[MAX_LDC_SLICE_NUM][LDC_DEBUG_TIME_TBL_MAX_NUM];
#endif
#endif

#if (HANDLE_LDC_EVENT_BY_TASK)
extern MMPF_OS_SEMID	m_LdcCtlSem;
#endif

extern MMP_UBYTE        m_ubYUVRawId;
extern MMPF_OS_SEMID    m_IbcFrmRdySem[];

extern MMP_UBYTE		m_ubFrontCamRotatePipe;
extern MMP_UBYTE		m_ubRearCamRotatePipe;

//==============================================================================
//
//                              EXTERN FUNCTION
//
//==============================================================================

#if (SUPPORT_MDTC)
extern void MMPF_MD_FrameReady(MMP_UBYTE *buf_idx);
#endif
#if (V4L2_GRAY)
extern void MMPF_YStream_FrameRdy(MMP_ULONG id, MMP_ULONG frm, MMP_ULONG ts);
#endif
#if (SUPPORT_IVA)
extern void MMPF_IVA_FrameReady(MMP_UBYTE *buf_idx);
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_GetActivePipeNum
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_Display_GetActivePipeNum(MMP_UBYTE ubSnrSel)
{
	MMP_UBYTE ubIdx = 0;
	MMP_UBYTE ubSum = 0;

    for (ubIdx = MMP_IBC_PIPE_0; ubIdx < MMP_IBC_PIPE_MAX; ubIdx++) {

	    if (gbPipeIsActive[ubIdx] == MMP_TRUE && gbPipeLinkedSnr[ubIdx] == ubSnrSel) {
	        ubSum++;
	    }
    }
    return ubSum;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_GetActivePipeId
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_GetActivePipeId(MMP_UBYTE ubSnrSel, MMP_UBYTE *pPipe, MMP_UBYTE ubSearchNum)
{
	MMP_UBYTE ubTempIdx = 0;
	MMP_UBYTE ubIdx = 0;
	
    for (ubIdx = MMP_IBC_PIPE_0; ubIdx < MMP_IBC_PIPE_MAX; ubIdx++) {

	    if (gbPipeIsActive[ubIdx] == MMP_TRUE && gbPipeLinkedSnr[ubIdx] == ubSnrSel) {
	        pPipe[ubTempIdx] = ubIdx;
	        ubTempIdx++;
	     
	        if (ubTempIdx == ubSearchNum)
	        	break;
	        else
	        	continue;
	    }
    }
	return MMP_ERR_NONE;
}

#if (SENSOR_EN)
//------------------------------------------------------------------------------
//  Function    : MMPF_Display_StartSoftwareRefresh
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_StartSoftwareRefresh(MMP_USHORT usIBCPipe)
{
	gbExposureDoneBufIdx[usIBCPipe]	= gbPreviewBufferCount[usIBCPipe] - 1;
	gbIBCCurBufIdx[usIBCPipe] 		= 0;

    #if (VIDEO_R_EN)
    if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)
    {
        if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        	// Real-time enocde mode doesn't need to handle current (IBC) frame buffers
        }
        else {
	        MMP_ULONG   i;
	        
	        MMPF_VIDENC_ResetQueue(&gVidRecdFreeQueue[gbIBCLinkEncId[usIBCPipe]]);
	        
	        #if (VR_SINGLE_CUR_FRAME == 1)
	        // Set all free queue points to the same frame buffer index
	        for (i = 1; i < MMPF_VIDENC_MAX_QUEUE_SIZE; i++) {
	        	MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[gbIBCLinkEncId[usIBCPipe]], 0, MMP_FALSE);
	        }
	        #else
	        for (i = 1; i < gbPreviewBufferCount[usIBCPipe]; i++) {
	        	MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[gbIBCLinkEncId[usIBCPipe]], i, MMP_FALSE);
	        }
	        #endif
	        
	        MMPF_VIDENC_SetFrameInfoList(gbIBCLinkEncId[usIBCPipe],
										 glPreviewBufAddr[usIBCPipe],
								         glPreviewUBufAddr[usIBCPipe],
								         glPreviewVBufAddr[usIBCPipe],
								         gbPreviewBufferCount[usIBCPipe]);
        }
    }
    #endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_EnablePipe
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_EnablePipe(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe)
{
    MMP_BOOL    bVIFEn  = MMP_FALSE;
	MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;

	gbPipeIsActive[usIBCPipe] = MMP_TRUE;
	gbPipeLinkedSnr[usIBCPipe] = ubSnrSel; // TBD, Remove later.

	/* Initial IBC store buffer and enable interrupt */
	if (usIBCPipe == MMP_IBC_PIPE_0) {
		m_bReceiveStopPreviewSig[0] = MMP_FALSE;
	}
	else if (usIBCPipe == MMP_IBC_PIPE_1) {
		m_bReceiveStopPreviewSig[1] = MMP_FALSE;
	}
	else if (usIBCPipe == MMP_IBC_PIPE_2) {
		m_bReceiveStopPreviewSig[2] = MMP_FALSE;
	}
	else if (usIBCPipe == MMP_IBC_PIPE_3) {
		m_bReceiveStopPreviewSig[3] = MMP_FALSE;
	}
	else if (usIBCPipe == MMP_IBC_PIPE_4) {
		m_bReceiveStopPreviewSig[4] = MMP_FALSE;
	}

	/* Enable VIF input/output and 3A */
	if (ubSnrSel == PRM_SENSOR || ubSnrSel == SCD_SENSOR) {

		ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);

	    MMPF_VIF_IsInterfaceEnable(ubVifId, &bVIFEn);

	    if (!bVIFEn) {
	        MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);

    		MMPF_VIF_EnableInputInterface(ubVifId, MMP_TRUE);
    		MMPF_VIF_EnableOutput(ubVifId, MMP_TRUE);
	    }
    }

    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_Display_DisablePipe
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_DisablePipe(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe)
{
    if (usIBCPipe >= MMP_IBC_PIPE_MAX)
        return MMP_DISPLAY_ERR_PARAMETER;

    gbIBCReady[usIBCPipe]     = MMP_FALSE;
	gbPipeIsActive[usIBCPipe] = MMP_FALSE;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_StartPreview
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_StartPreview(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe, MMP_BOOL bCheckFrameEnd)
{
    AITPS_IBC   pIBC    = AITC_BASE_IBC;
    MMP_BOOL    bVIFEn  = MMP_FALSE;
	MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;

	gbPipeIsActive[usIBCPipe] = MMP_TRUE;
	gbPipeLinkedSnr[usIBCPipe] = ubSnrSel; // TBD, Remove later.

	/* Reset video encode queue */
    MMPF_Display_StartSoftwareRefresh(usIBCPipe);
	
	/* Initial IBC store buffer and enable interrupt */
	if (usIBCPipe == MMP_IBC_PIPE_0) {
		m_bReceiveStopPreviewSig[0] = MMP_FALSE;

        pIBC->IBC_P0_INT_CPU_SR = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;
        pIBC->IBC_P0_INT_CPU_EN = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;

        if ((gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) &&
            (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)) {

            pIBC->IBC_P0_INT_CPU_SR |= IBC_INT_H264_RT_BUF_OVF;
            pIBC->IBC_P0_INT_CPU_EN |= IBC_INT_H264_RT_BUF_OVF;
        }
        else {
            pIBC->IBCP_0.IBC_ADDR_Y_ST = glPreviewBufAddr[usIBCPipe][0];
            pIBC->IBCP_0.IBC_ADDR_U_ST = glPreviewUBufAddr[usIBCPipe][0];
            pIBC->IBCP_0.IBC_ADDR_V_ST = glPreviewVBufAddr[usIBCPipe][0];
        }

        if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)
                pIBC->IBCP_0.IBC_BUF_CFG &= ~(IBC_STORE_EN);
            else
                pIBC->IBCP_0.IBC_BUF_CFG |= IBC_STORE_EN;
        }
        else {
            pIBC->IBCP_0.IBC_BUF_CFG |= IBC_STORE_EN;
        }
	}
	else if (usIBCPipe == MMP_IBC_PIPE_1) {
		m_bReceiveStopPreviewSig[1] = MMP_FALSE;
		
        pIBC->IBC_P1_INT_CPU_SR = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;
        pIBC->IBC_P1_INT_CPU_EN = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;

        if ((gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) &&
            (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)) {

            pIBC->IBC_P1_INT_CPU_SR |= IBC_INT_H264_RT_BUF_OVF;
            pIBC->IBC_P1_INT_CPU_EN |= IBC_INT_H264_RT_BUF_OVF;
        }
        else {
            pIBC->IBCP_1.IBC_ADDR_Y_ST = glPreviewBufAddr[usIBCPipe][0];
            pIBC->IBCP_1.IBC_ADDR_U_ST = glPreviewUBufAddr[usIBCPipe][0];
            pIBC->IBCP_1.IBC_ADDR_V_ST = glPreviewVBufAddr[usIBCPipe][0];
        }

        if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)
                pIBC->IBCP_1.IBC_BUF_CFG &= ~(IBC_STORE_EN);
            else
                pIBC->IBCP_1.IBC_BUF_CFG |= IBC_STORE_EN;
        }
        else {
            pIBC->IBCP_1.IBC_BUF_CFG |= IBC_STORE_EN;
        }
	}
	else if (usIBCPipe == MMP_IBC_PIPE_2) {
		m_bReceiveStopPreviewSig[2] = MMP_FALSE;
		
        pIBC->IBC_P2_INT_CPU_SR = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;
        pIBC->IBC_P2_INT_CPU_EN = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;

        if ((gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) &&
            (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)) {

            pIBC->IBC_P2_INT_CPU_SR |= IBC_INT_H264_RT_BUF_OVF;
            pIBC->IBC_P2_INT_CPU_EN |= IBC_INT_H264_RT_BUF_OVF;
        }
        else {
            pIBC->IBCP_2.IBC_ADDR_Y_ST = glPreviewBufAddr[usIBCPipe][0];
            pIBC->IBCP_2.IBC_ADDR_U_ST = glPreviewUBufAddr[usIBCPipe][0];
            pIBC->IBCP_2.IBC_ADDR_V_ST = glPreviewVBufAddr[usIBCPipe][0];
        }

        if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)
                pIBC->IBCP_2.IBC_BUF_CFG &= ~(IBC_STORE_EN);
            else
                pIBC->IBCP_2.IBC_BUF_CFG |= IBC_STORE_EN;
        }
        else {
            pIBC->IBCP_2.IBC_BUF_CFG |= IBC_STORE_EN;
        }
	}
	else if (usIBCPipe == MMP_IBC_PIPE_3) {
		m_bReceiveStopPreviewSig[3] = MMP_FALSE;
		
        pIBC->IBC_P3_INT_CPU_SR = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;
        pIBC->IBC_P3_INT_CPU_EN = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;

        if ((gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) &&
            (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)) {

            pIBC->IBC_P3_INT_CPU_SR |= IBC_INT_H264_RT_BUF_OVF;
            pIBC->IBC_P3_INT_CPU_EN |= IBC_INT_H264_RT_BUF_OVF;
        }
        else {
            pIBC->IBCP_3.IBC_ADDR_Y_ST = glPreviewBufAddr[usIBCPipe][0];
            pIBC->IBCP_3.IBC_ADDR_U_ST = glPreviewUBufAddr[usIBCPipe][0];
            pIBC->IBCP_3.IBC_ADDR_V_ST = glPreviewVBufAddr[usIBCPipe][0];
        }

        if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO)
                pIBC->IBCP_3.IBC_BUF_CFG &= ~(IBC_STORE_EN);
            else
                pIBC->IBCP_3.IBC_BUF_CFG |= IBC_STORE_EN;
        }
        else {
            pIBC->IBCP_3.IBC_BUF_CFG |= IBC_STORE_EN;
        }
	}
	else if (usIBCPipe == MMP_IBC_PIPE_4) {
		m_bReceiveStopPreviewSig[4] = MMP_FALSE;
		
        pIBC->IBC_P4_INT_CPU_SR = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;
        pIBC->IBC_P4_INT_CPU_EN = IBC_INT_FRM_RDY | IBC_INT_FRM_END | IBC_INT_FRM_ST;

        pIBC->IBC_P4_ADDR_Y_ST = glPreviewBufAddr[usIBCPipe][0];
        pIBC->IBC_P4_BUF_CFG |= IBC_P4_STORE_EN;
	}

	/* Enable VIF input/output and 3A */
	if (ubSnrSel == PRM_SENSOR || ubSnrSel == SCD_SENSOR) {

		ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);

	    MMPF_VIF_IsInterfaceEnable(ubVifId, &bVIFEn);

	    if (!bVIFEn) {
	        MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
	    	if (m_bFirstSensorInitialize[ubSnrSel] == MMP_TRUE) {
	    		m_bFirstSensorInitialize[ubSnrSel] = MMP_FALSE;
	    		MMPF_VIF_EnableInputInterface(ubVifId, MMP_TRUE);
	    		MMPF_VIF_EnableOutput(ubVifId, MMP_TRUE);
	    	}
	    	else {
				MMPF_VIF_EnableInputInterface(ubVifId, MMP_TRUE);
				MMPF_VIF_CheckFrameSig(ubVifId, MMPF_VIF_INT_EVENT_FRM_ST, 1);
				MMPF_VIF_CheckFrameSig(ubVifId, MMPF_VIF_INT_EVENT_GRAB_ED, 1);
				MMPF_VIF_EnableOutput(ubVifId, MMP_TRUE);
	    	}
	    }
    }

	if (ubSnrSel == PRM_SENSOR || ubSnrSel == SCD_SENSOR) {
		if (bCheckFrameEnd) {
			m_bStartPreviewFrameEndSig[ubSnrSel] = MMP_TRUE;

			if (MMPF_OS_AcquireSem(m_StartPreviewFrameEndSem[ubSnrSel], DISP_START_PREVIEW_SEM_TIMEOUT)) {
				RTNA_DBG_Str(0, "Start Preview : Wait IBC frame end fail\r\n");
				return MMP_DISPLAY_ERR_FRAME_END;
			}
		}
	}
	
	/* MMPF_SYS_DumpTimerMark(); */
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_StopPreview
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_StopPreview(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe, MMP_BOOL bCheckFrameEnd)
{
	AITPS_IBC   pIBC    = AITC_BASE_IBC;
    MMP_UBYTE   ubOsErr = 0;

	OS_CRITICAL_INIT();
    
	if (MMPF_Display_GetActivePipeNum(ubSnrSel) == 0 || usIBCPipe >= MMP_IBC_PIPE_MAX) {
		return MMP_ERR_NONE;
	}

	OS_ENTER_CRITICAL();
	
	/* Disable IBC interrupt */	
    if (usIBCPipe == MMP_IBC_PIPE_0) {
        pIBC->IBC_P0_INT_CPU_EN &= ~(IBC_INT_FRM_RDY);
    }
    else if (usIBCPipe == MMP_IBC_PIPE_1) {
        pIBC->IBC_P1_INT_CPU_EN &= ~(IBC_INT_FRM_RDY);
    }
    else if (usIBCPipe == MMP_IBC_PIPE_2) {
        pIBC->IBC_P2_INT_CPU_EN &= ~(IBC_INT_FRM_RDY);
    }
    else if (usIBCPipe == MMP_IBC_PIPE_3) {
        pIBC->IBC_P3_INT_CPU_EN &= ~(IBC_INT_FRM_RDY);
    }
    else if (usIBCPipe == MMP_IBC_PIPE_4) {
        pIBC->IBC_P4_INT_CPU_EN &= ~(IBC_INT_FRM_RDY);
    }

    gbIBCReady[usIBCPipe]     = MMP_FALSE;
	gbPipeIsActive[usIBCPipe] = MMP_FALSE;

    if (m_bReceiveStopPreviewSig[usIBCPipe] == MMP_TRUE) {
		OS_EXIT_CRITICAL();
		return MMP_ERR_NONE;
	}
    else {
		/* VIF frame start interrupt close vi-output and set the flag (m_bWaitISPOperationDoneSig)
		 * ISP frame end interrupt set the flag of closing vi-input (MMPF_SENSOR_ISP_FRAME_END)
    	 * IBC frame end interrupt set the flag of closing vi-input (m_bStopPreviewCloseVifInSig) and close itself.
    	 * VIF frame end interrupt close vi-input finally.
         */
		
		if (ubSnrSel == PRM_SENSOR || ubSnrSel == SCD_SENSOR) {

	        m_bReceiveStopPreviewSig[usIBCPipe] = MMP_TRUE;

			m_ubStopPreviewSnrId = ubSnrSel;
			
			OS_EXIT_CRITICAL();

	        if (MMPF_Display_GetActivePipeNum(ubSnrSel) == 0) {
	        	/* Make sure that we stop preview after the frame end interrupt of ISP
	        	 * and ISP module finish its operation.
	        	 * We only use this semaphore when we want to close vi-out */
	            if (m_bLinkISPSel[ubSnrSel]) {
	        	    /* Assume Sub Sensor doesn't pass ISP module */
	        	    ubOsErr = MMPF_OS_AcquireSem(m_ISPOperationDoneSem, DISP_STOP_PREVIEW_SEM_TIMEOUT);

	                if (ubOsErr == 1) {
	                    MMP_PRINT_RET_ERROR(DISPLAY_DBG_LEVEL, 0, "Stop Preview AcquireSem TimeOut1\r\n");
	                    return MMP_DISPLAY_ERR_STOP_PREVIEW_TIMEOUT;
	                }
	            }
	        }

    	    ubOsErr = MMPF_OS_AcquireSem(m_StopPreviewCtrlSem[ubSnrSel], DISP_STOP_PREVIEW_SEM_TIMEOUT);
            if (ubOsErr == 1) {
                MMP_PRINT_RET_ERROR(DISPLAY_DBG_LEVEL, 0, "Stop Preview AcquireSem TimeOut2\r\n");
                return MMP_DISPLAY_ERR_STOP_PREVIEW_TIMEOUT;
            }

		    if (MMPF_Display_GetActivePipeNum(ubSnrSel) == 0) {
		    	gbFrameEndFlag[ubSnrSel] = MMPF_SENSOR_FRAME_END_NONE;
		    }
		}
		else {
			OS_EXIT_CRITICAL();
		}
		
		if (m_bPipeLinkGraphic[usIBCPipe]) {
        	gIBCLinkType[usIBCPipe] &= ~(MMP_IBC_LINK_GRAPHIC);
        }
	}

    if ((gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_DISPLAY) ||
        (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_ROTATE))
    {
        m_ubPrevwDispSts = MMP_FALSE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_GetPreviewSts
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_GetPreviewSts(MMP_UBYTE *pubStatus)
{
    *pubStatus = m_ubPrevwDispSts;
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_FrameDoneTrigger
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_FrameDoneTrigger(MMP_USHORT usIBCPipe)
{
    if (gIBCLinkType[usIBCPipe] == MMP_IBC_LINK_NONE) {
        return MMP_ERR_NONE;
    }

    MMPF_OS_ReleaseSem(m_IbcFrmRdySem[usIBCPipe]);

    #if	(VIDEO_R_EN)
    if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_VIDEO) {
        MMP_ULONG   ulCurIndex, ulIbcIndex;

        ulCurIndex = gbExposureDoneBufIdx[usIBCPipe];
        ulIbcIndex = gbIBCCurBufIdx[usIBCPipe];

        MMPF_VIDENC_SetRecdFrameReady(usIBCPipe, &ulCurIndex, &ulIbcIndex);

        gbExposureDoneBufIdx[usIBCPipe] = ulCurIndex;
        gbIBCCurBufIdx[usIBCPipe]       = ulIbcIndex;
        gbIBCReady[usIBCPipe]           = MMP_TRUE;
    }
    else
    #endif
    #if (V4L2_GRAY)
    if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_GRAY) {
        MMP_ULONG   ulRdyIndex;

        ulRdyIndex = gbIBCCurBufIdx[usIBCPipe];
        MMPF_YStream_FrameRdy(  gbIBCLinkEncId[usIBCPipe],
                                glPreviewBufAddr[usIBCPipe][ulRdyIndex],
                                OSTime);
        gbExposureDoneBufIdx[usIBCPipe] = ulRdyIndex;

        gbIBCCurBufIdx[usIBCPipe]++;
        if (gbIBCCurBufIdx[usIBCPipe] >= gbPreviewBufferCount[usIBCPipe])
            gbIBCCurBufIdx[usIBCPipe] = 0;
    }
    else if ( gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_IVA ) {
    #if (SUPPORT_IVA)
        MMP_ULONG   ulRdyIndex;
        ulRdyIndex = gbIBCCurBufIdx[usIBCPipe];
        MMPF_IVA_FrameReady(&gbIBCCurBufIdx[usIBCPipe]);
        gbExposureDoneBufIdx[usIBCPipe] = ulRdyIndex;
        
    #endif
    }
    else
    #endif
    #if (SUPPORT_MDTC)
    if (gIBCLinkType[usIBCPipe] & MMP_IBC_LINK_MDTC) {
        MMP_ULONG   ulRdyIndex;

        ulRdyIndex = gbIBCCurBufIdx[usIBCPipe];
        MMPF_MD_FrameReady(&gbIBCCurBufIdx[usIBCPipe]);
        gbExposureDoneBufIdx[usIBCPipe] = ulRdyIndex;
    }
    else
    #endif
    {
        gbExposureDoneBufIdx[usIBCPipe] += 1;

        if (gbExposureDoneBufIdx[usIBCPipe] >= gbPreviewBufferCount[usIBCPipe])
            gbExposureDoneBufIdx[usIBCPipe] = 0;

        gbIBCCurBufIdx[usIBCPipe] += 1;

        if (gbIBCCurBufIdx[usIBCPipe] >= gbPreviewBufferCount[usIBCPipe])
            gbIBCCurBufIdx[usIBCPipe] = 0;
    }

    MMPF_IBC_UpdateStoreAddress(usIBCPipe,
                    glPreviewBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]],
                    glPreviewUBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]],
                    glPreviewVBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]]);

    return MMP_ERR_NONE;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_GetExposedFrameBuffer
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_GetExposedFrameBuffer(MMP_USHORT usIBCPipe, MMP_ULONG *pulYAddr, MMP_ULONG *pulUAddr, MMP_ULONG *pulVAddr)
{
	if (pulYAddr != NULL) {
		*pulYAddr = glPreviewBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]];
	}	
	if (pulUAddr != NULL) {
		*pulUAddr = glPreviewUBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]];
	}
	if (pulVAddr != NULL) {
		*pulVAddr = glPreviewVBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]];
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Display_GetCurIbcFrameBuffer
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Display_GetCurIbcFrameBuffer(MMP_USHORT usIBCPipe, MMP_ULONG *pulYAddr, MMP_ULONG *pulUAddr, MMP_ULONG *pulVAddr)
{
	if (pulYAddr != NULL) {
		*pulYAddr = glPreviewBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]];
	}	
	if (pulUAddr != NULL) {
		*pulUAddr = glPreviewUBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]];
	}
	if (pulVAddr != NULL) {
		*pulVAddr = glPreviewVBufAddr[usIBCPipe][gbIBCCurBufIdx[usIBCPipe]];
	}
	return MMP_ERR_NONE;
}

