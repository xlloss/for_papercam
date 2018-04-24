//==============================================================================
//
//  File        : mmpf_sensor.c
//  Description : Sensor Control function
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
#if (SENSOR_EN)
#include "math.h"
#include "lib_retina.h"
#include "hdr_cfg.h"
#include "mmp_reg_vif.h"
#include "mmp_reg_rawproc.h"
#include "mmp_reg_scaler.h"
#include "mmp_reg_color.h"
#include "mmp_dsc_inc.h"

#include "mmpf_display.h"
#include "mmpf_mp4venc.h"
#include "mmpf_scaler.h"
#include "mmpf_sensor.h"
#include "mmpf_rawproc.h"
#include "mmpf_vif.h"
#include "mmpf_ibc.h"
#include "mmpf_system.h"
#include "mmpf_ptz.h"
#include "hdm_ctl.h"

#if (USER_LOG)
extern unsigned int small_log10(unsigned int v);
#define log10 small_log10
#endif
//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

/** @addtogroup MMPF_Sensor
@{
*/

static MMP_BOOL 	m_bRawPathPreview[2] = {MMP_FALSE, MMP_FALSE};
static MMP_BOOL 	m_bRawStoreOnly[2] = {MMP_FALSE, MMP_FALSE};
static MMP_UBYTE 	m_ubDo3ASensorId = PRM_SENSOR;
static MMP_UBYTE  m_ubNightVision ;

MMP_BOOL    		gbInFDTCMode = MMP_FALSE;

MMP_USHORT			gusSensorChangeModeIdx = 0;
MMP_UBYTE			gubChangeModeSensorId = 0;
MMP_UBYTE	        gbFrameEndFlag[VIF_SENSOR_MAX_NUM] = {MMPF_SENSOR_FRAME_END_NONE, MMPF_SENSOR_FRAME_END_NONE};

MMP_USHORT          gsCurPreviewMode[VIF_SENSOR_MAX_NUM] = {0, 0};
MMP_USHORT          gsCurCaptureMode[VIF_SENSOR_MAX_NUM] = {0, 0};

MMPF_SENSOR_FUNCTION *gsSensorFunction;
MMP_USHORT          gsSensorMode[VIF_SENSOR_MAX_NUM];
MMP_ULONG   		glISPBuffer[ISP_BUFFER_NUM];
MMP_ULONG   		m_ulISPFrameCount = 0;
MMP_ULONG   		m_ulVIFFrameCount[MMPF_VIF_MDL_NUM] = {0, 0};

// ISP setting
MMP_LONG    		glISPExpValForCapture = 0;
MMP_ULONG   		glISPExpLimitBufaddr = NULL; 
MMP_ULONG   		glISPExpLimitDatatypeByByte;
MMP_ULONG   		glISPExpLimitSize;
MMP_UBYTE   		gbISPAGC = 0;
MMP_ULONG			m_glISPBufferStartAddr;

/**	@brief 	when this flag is true means we want to do the ISP related movement.
			when this flag is false means we dont want to do the ISP related movement.*/
MMP_BOOL    		m_bISP3AStatus = MMP_FALSE;

/**	@brief	VIF frame start interrupt send this flag when it close vi output.
			ISP frame end interrupt will check that 
			does it have to release m_ISPOperationDoneSem, when it get this flag.*/
MMP_BOOL    		m_bWaitISPOperationDoneSig = MMP_FALSE;

#if (SUPPORT_ISP_TIMESHARING)
MMP_BOOL			m_bRetriggerRawF = MMP_FALSE;
MMP_UBYTE			m_ubRetriggerRawFSnrId;
MMP_UBYTE			m_ubRetriggerRawFRawId;
MMPF_OS_SEMID		m_IspFrmEndSem;
#endif

MMPF_OS_SEMID       m_IbcFrmRdySem[MMP_IBC_PIPE_MAX];

MMP_UBYTE			m_ubFrontCamRotatePipe = 0;
MMP_UBYTE			m_ubRearCamRotatePipe = 0;

/* While adding video frame into MMPF_StreamRing, it was using time of encode-done.
 * Now it time of encode-done shifts from frame to frame. The time of sensor input
 * time is needed. Kernel team, please revise this.
 * It's better attach the timestamp all the way to add frame.
 * As long as frame time is almost even, straming server should be happy about it.
 */
MMP_ULONG	 		gulVifGrabEndTime[MMPF_VIF_MDL_NUM] = {0, 0};


MMPF_SENSOR_INSTANCE    SensorInstance =
{
    MMPF_SENSOR_3A_RESET,   //b3AInitStat
    MMP_FALSE,              //b3AStatus
    MMP_TRUE,               //bIspAeFastConverge
    MMP_TRUE,               //bIspAwbFastConverge
    0,                      //ulIspAeConvCount
    0                       //ulIspAwbConvCount
};
MMPF_SENSOR_INSTANCE    *pSnr = &SensorInstance;


//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMPF_OS_FLAGID   SENSOR_Flag;

extern MMP_UBYTE        gbIBCLinkEncId[];
extern MMP_UBYTE        gbExposureDoneBufIdx[];
extern MMP_UBYTE        gbPreviewBufferCount[];
extern MMP_ULONG        glPreviewBufAddr[MMP_IBC_PIPE_MAX][4];
extern MMP_ULONG        glPreviewUBufAddr[MMP_IBC_PIPE_MAX][4];
extern MMP_ULONG        glPreviewVBufAddr[MMP_IBC_PIPE_MAX][4];

extern MMP_ULONG        glRotateBufAddr[MMP_IBC_PIPE_MAX];
extern MMP_ULONG        glRotateUBufAddr[MMP_IBC_PIPE_MAX];
extern MMP_ULONG        glRotateVBufAddr[MMP_IBC_PIPE_MAX];
extern MMP_ULONG        glRotateBufAddrRearCam[MMP_IBC_PIPE_MAX];
extern MMP_ULONG        glRotateUBufAddrRearCam[MMP_IBC_PIPE_MAX];
extern MMP_ULONG        glRotateVBufAddrRearCam[MMP_IBC_PIPE_MAX];
extern MMP_UBYTE        gbRotateBufferCount;
extern MMP_UBYTE        gbRotateCurBufIdx;
extern MMP_UBYTE        gbRotateDoneBufIdx;
extern MMP_UBYTE        gbRotateCurBufIdxRearCam;
extern MMP_UBYTE        gbRotateDoneBufIdxRearCam;

extern MMP_IBC_LINK_TYPE gIBCLinkType[];
extern MMP_UBYTE        gbPipeLinkedSnr[];

extern MMPF_OS_SEMID    m_StartPreviewFrameEndSem[];
extern MMPF_OS_SEMID    m_StopPreviewCtrlSem[];
extern MMPF_OS_SEMID    m_ISPOperationDoneSem;
extern MMP_BOOL         m_bReceiveStopPreviewSig[];
extern MMP_BOOL         m_bStopPreviewCloseVifInSig[];
extern MMP_BOOL		    m_bISP3AStatus;
extern MMP_UBYTE        m_ubStopPreviewSnrId;

extern MMPF_SENSOR_FUNCTION *SensorFuncTable;

extern MMP_BOOL	        m_bPipeLinkGraphic[];
extern MMP_USHORT  	    m_gsISPCoreID;

extern MMP_USHORT       m_usPcamSetValue;

extern MMP_UBYTE        m_ubHdrMainRawId;
extern MMP_UBYTE        m_ubHdrSubRawId;

extern MMP_UBYTE        m_ubYUVRawId;

//==============================================================================
//
//                              FUNCTION PROTOTYPE
//
//==============================================================================

MMP_BOOL MMPF_ISP_IsFrameEndStage(void);
MMP_UBYTE MMPF_Sensor_GetSnrIdFromVifId(MMP_UBYTE ubVifId);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================



MMP_ERR MMPF_Sensor_Set3AState(MMPF_SENSOR_3A_STATE state)
{
    pSnr->b3AInitState = state ;
    return MMP_ERR_NONE;
}

MMPF_SENSOR_3A_STATE MMPF_Sensor_Get3AState(void)
{
    return pSnr->b3AInitState ;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_CheckMIPISig
//  Description :
//------------------------------------------------------------------------------
void MMPF_VIF_CheckMIPISig(MMP_UBYTE ubVifId)
{
#if (SENSOR_MIPI_DBG_EN)
    MMP_UBYTE   ubSigErr;
    MMP_UBYTE   i = 0;
    AITPS_MIPI  pMIPI = AITC_BASE_MIPI;

    if (!(pMIPI->MIPI_CLK_CFG[ubVifId] & MIPI_CSI2_EN)) {
        return;
    }

    /* Check MIPI data lane error */
    for (i = 0; i < MAX_MIPI_DATA_LANE_NUM; i++) {
        ubSigErr = pMIPI->DATA_LANE[i].MIPI_DATA_ERR_SR[ubVifId];
        pMIPI->DATA_LANE[i].MIPI_DATA_ERR_SR[ubVifId] = ubSigErr;
        
        if (ubSigErr & MIPI_SOT_SEQ_ERR) {
            RTNA_DBG_Str(0, FG_RED(">>DAT SOT_SEQ_ERR")"\r\n");
        }
        if (ubSigErr & MIPI_SOT_SYNC_ERR) {
            RTNA_DBG_Str(0, FG_RED(">>DAT SOT_SYNC_ERR")"\r\n");
        }
        if (ubSigErr & MIPI_CTL_ERR) {
            RTNA_DBG_Str(0, FG_RED(">>DAT MIPI_CTL_ERR")"\r\n");
        }
    }
    
    /* Check MIPI clock lane error */
    ubSigErr = pMIPI->MIPI_CLK_ERR_SR[ubVifId];
    pMIPI->MIPI_CLK_ERR_SR[ubVifId] = ubSigErr;
    
    if (ubSigErr & MIPI_SOT_SEQ_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CLK SOT_SEQ_ERR")"\r\n");
    }
    if (ubSigErr & MIPI_SOT_SYNC_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CLK SOT_SYNC_ERR")"\r\n");
    }
    if (ubSigErr & MIPI_CTL_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CLK MIPI_CTL_ERR")"\r\n");
    }
    
    /* Check MIPI CSI error */
    ubSigErr = pMIPI->MIPI_CSI2_ERR_SR[ubVifId];
    pMIPI->MIPI_CSI2_ERR_SR[ubVifId] = ubSigErr; 

    if (ubSigErr & MIPI_CSI2_1ECC_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CSI2_1ECC_ERR")"\r\n");
    }
    if (ubSigErr & MIPI_CSI2_2ECC_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CSI2_2ECC_ERR")"\r\n");
    }
    if (ubSigErr & MIPI_CSI2_CRC_ERR) {
        RTNA_DBG_Str(0, FG_RED(">>CSI2_CRC_ERR")"\r\n");
    }
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_VIF_ISR(void) ITCMFUNC;
void MMPF_VIF_ISR(void)
{
    AITPS_VIF   pVIF        = AITC_BASE_VIF;
    MMP_UBYTE   intsrc      = 0;
    MMP_BOOL    bVIFOutEn   = MMP_FALSE;
    MMP_UBYTE   ubSnrSel    = 0;
    MMP_UBYTE   ubVifId     = MMPF_VIF_MDL_ID0;
    MMP_UBYTE   ubRawId     = ubVifId;
    VifCallBackFunc *cb;
    
    extern MMP_BOOL m_bDoCaptureFromRawPath;
    
    for (ubVifId = MMPF_VIF_MDL_ID0; ubVifId < MMPF_VIF_MDL_NUM; ubVifId++) 
    {
        ubRawId  = ubVifId; // Assume the VIF ID directly maps to RawProc ID (TBD)
        ubSnrSel = MMPF_Sensor_GetSnrIdFromVifId(ubVifId);

        intsrc = pVIF->VIF_INT_CPU_SR[ubVifId] & pVIF->VIF_INT_CPU_EN[ubVifId];
        pVIF->VIF_INT_CPU_SR[ubVifId] = intsrc;

        if (intsrc & VIF_INT_GRAB_END) 
        {
            m_ulVIFFrameCount[ubVifId]++;

            cb = CallBackFuncVif[ubVifId][MMPF_VIF_INT_EVENT_GRAB_ED];
            if (cb) {
	            cb(CallBackArguVif[ubVifId][MMPF_VIF_INT_EVENT_GRAB_ED]);
	        }

            if (MMPF_Display_GetActivePipeNum(ubSnrSel) != 0) 
            {
                MMPF_VIF_IsOutputEnable(ubVifId, &bVIFOutEn);

                if (bVIFOutEn) 
                {
                    if (m_bRawPathPreview[ubVifId]) 
                    {
                    	if (gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable)
                    	{
                        	if (gsHdrCfg.bRawGrabEnable == MMP_FALSE)
                        	{
    	                    	MMP_ULONG ulVifGrabW, ulVifGrabH;

    	                    	MMPF_VIF_GetGrabResolution(ubVifId, &ulVifGrabW, &ulVifGrabH);

    	                    	MMPF_HDR_Preview(ulVifGrabW, ulVifGrabH);
    						}
                            else {
                            	MMPF_PTZ_ExecutePTZ(ubSnrSel);
                            }
    					}
    					else if (m_bRawStoreOnly[ubVifId])
    					{
    					    /* Update store buffer at RawProc ISR */
                            //MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);
                            //MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE0);
    					}
    					else if (!m_bDoCaptureFromRawPath)
    					{
    						MMP_BOOL bSkipFrm = MMP_FALSE;

							bSkipFrm = bSkipFrm;

                            MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);
                            MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE0);
							
							#if (SUPPORT_ISP_TIMESHARING)
							m_bRetriggerRawF = !MMPF_ISP_IsFrameEndStage();

							if (m_bRetriggerRawF == MMP_FALSE) {

	                            MMPF_RAWPROC_UpdateFetchBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);

	                            if (!MMPF_RAWPROC_IsFetchBusy()) {

	                                MMPF_RAWPROC_FetchData (ubSnrSel,
	                                                        ubRawId,
	                	                                	MMP_RAW_FETCH_NO_ROTATE,
	                	                                	MMP_RAW_FETCH_NO_MIRROR);
	                            }
                            }
                            else {
                            	m_ubRetriggerRawFSnrId = ubSnrSel;
                            	m_ubRetriggerRawFRawId = ubRawId;
                            }
                            #else
                        	/* Check raw fetch busy status */
                        	if (MMPF_RAWPROC_IsFetchBusy()) {
    					        RTNA_DBG_Str(0, "Raw fetch busy !\r\n");
    					        bSkipFrm = MMP_TRUE;
                            }
                            else {
                                /* It may change raw fetch range */
                            	MMPF_PTZ_ExecutePTZ(ubSnrSel);
                            }

                            MMPF_RAWPROC_UpdateFetchBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);

                            if (bSkipFrm == MMP_FALSE && !MMPF_RAWPROC_IsFetchBusy()) {

                                MMPF_RAWPROC_FetchData (ubSnrSel,
                                                        ubRawId,
                	                                	MMP_RAW_FETCH_NO_ROTATE,
                	                                	MMP_RAW_FETCH_NO_MIRROR);
                            }
                            #endif
                        }
                    }
                    #if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
                    else {
                    	MMPF_PTZ_ExecutePTZ(ubSnrSel);
                    }
                    #endif
                }
            }
        }

        if (intsrc & VIF_INT_FRM_END) 
        {
            cb = CallBackFuncVif[ubVifId][MMPF_VIF_INT_EVENT_FRM_ED];
            if (cb) {
	            cb(CallBackArguVif[ubVifId][MMPF_VIF_INT_EVENT_FRM_ED]);
	        }

            if (m_bStopPreviewCloseVifInSig[ubSnrSel] == MMP_TRUE) {

            	gbFrameEndFlag[ubSnrSel] |= MMPF_SENSOR_VIF_FRAME_END;

                if (m_bLinkISPSel[ubSnrSel]) {
        			if (gbFrameEndFlag[ubSnrSel] == (MMPF_SENSOR_ISP_FRAME_END | MMPF_SENSOR_VIF_FRAME_END)) {
        				gbFrameEndFlag[ubSnrSel] = MMPF_SENSOR_FRAME_END_NONE;
        	            MMPF_VIF_EnableInputInterface(ubVifId, MMP_FALSE);
        	        }
                }
                else {
    				gbFrameEndFlag[ubSnrSel] = MMPF_SENSOR_FRAME_END_NONE;
    	            MMPF_VIF_EnableInputInterface(ubVifId, MMP_FALSE);
                }
                pVIF->VIF_INT_CPU_EN[ubVifId] &= ~(VIF_INT_FRM_END);

                m_bStopPreviewCloseVifInSig[ubSnrSel] = MMP_FALSE;
                MMPF_OS_ReleaseSem(m_StopPreviewCtrlSem[ubSnrSel]);
                
                MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_FALSE);
            }
        }

        if (intsrc & VIF_INT_FRM_ST) 
        {
            cb = CallBackFuncVif[ubVifId][MMPF_VIF_INT_EVENT_FRM_ST];
            if (cb) {
	            cb(CallBackArguVif[ubVifId][MMPF_VIF_INT_EVENT_FRM_ST]);
	        }

            #if (SENSOR_MIPI_DBG_EN)
            MMPF_VIF_CheckMIPISig(ubVifId);
            #endif
            
            /* To close VIF output if all pipes disabled */
            if (MMPF_Display_GetActivePipeNum(ubSnrSel) == 0)
            {
                MMPF_VIF_IsOutputEnable(ubVifId, &bVIFOutEn);
                
                if (bVIFOutEn) {

                	m_bWaitISPOperationDoneSig = MMP_TRUE;
    				
    				if (m_bRawPathPreview[ubVifId]) 
    				{
    					if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
    						gsHdrCfg.ubMode == HDR_MODE_STAGGER && 
    						gsHdrCfg.bRawGrabEnable)
    					{
    						MMPF_VIF_EnableOutput(ubVifId, MMP_FALSE);
    					}
    					else if (m_bRawStoreOnly[ubVifId])
    					{
                            MMPF_VIF_EnableOutput(ubVifId, MMP_FALSE);
    					}
    					else
    					{
    	    				if (MMPF_RAWPROC_IsFetchBusy() == MMP_FALSE) {

    	    				    // For create additional IBC frm_end interrupt
    	                        MMPF_RAWPROC_UpdateFetchBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);
    	    				    MMPF_RAWPROC_FetchData(	ubSnrSel,
    	    				                            ubRawId,
    	                    				        	MMP_RAW_FETCH_NO_ROTATE,
    	                    				        	MMP_RAW_FETCH_NO_MIRROR);

    	                        // Modify VIF disable flow for scale up and raw fetch very slow case.
    	                        // If we close VIF output first and raw busy happens. Preview can't stop any more.
    	       			 		MMPF_VIF_EnableOutput(ubVifId, MMP_FALSE); 
    	       			 	}
           			 	}
           			}
           			else {
    	                MMPF_VIF_EnableOutput(ubVifId, MMP_FALSE);
           			}
        		}
    		}
    		else
			{
	        	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
	        		gsHdrCfg.bDscEnable == MMP_FALSE) {
                	if (m_bLinkISPSel[ubSnrSel] && m_bISP3AStatus) {
                        #if (ISP_USE_TASK_DO3A == 1)
                        //move to acc done
                        //MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_VIF_FRM_ST, MMPF_OS_FLAG_SET);
                        #else
                        gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
                        #endif
                    }
                }
    		}
    	}
    	
    	//if ( (intsrc & VIF_INT_FRM_ST) && aitcam_ipc_is_debug_ts() )  {
        //        MMPF_DBG_Int(OSTime, -6);
        //        RTNA_DBG_Str0("_S\r\n");
    	//}
    	
	}
}

#if 0
void _____ISP_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_ISP_ISR(void) ITCMFUNC;
void MMPF_ISP_ISR(void)
{
    MMP_ULONG       intsrc;
    AITPS_ISP       pISP = AITC_BASE_ISP;
	MMP_BOOL    	bVIFOutEn;
	MMP_UBYTE		ubVifId = MMPF_VIF_MDL_ID0;

    intsrc = pISP->ISP_INT_CPU_SR & pISP->ISP_INT_CPU_EN;
    pISP->ISP_INT_CPU_SR = intsrc;
    
    
    if (intsrc & ISP_INT_AE_WINDOW_ACC_DONE) {
        if (m_bISP3AStatus) {
            gsSensorFunction->MMPF_Sensor_DoAEOperation_END(m_ubDo3ASensorId);
            //gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
            if (m_bISP3AStatus) {
                #if (ISP_USE_TASK_DO3A == 1)
                MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_VIF_FRM_ST, MMPF_OS_FLAG_SET);
                #else
                gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
                #endif
            }
        }
    }    
    
    if (intsrc & ISP_INT_COLOR_FRM_END)
    {
    	if (m_bISP3AStatus) {
            /* ISP_IF_AE_GetHWAcc() must be done at Frame End to Next Frame Start */
            //gsSensorFunction->MMPF_Sensor_DoAEOperation_END(m_ubDo3ASensorId);
    	    #if (ISP_USE_TASK_DO3A == 1)
            MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_ISP_FRM_END, MMPF_OS_FLAG_SET);
            m_ulISPFrameCount++;
    	    #else
    	    gsSensorFunction->MMPF_Sensor_DoAWBOperation(m_ubDo3ASensorId);
    	    gsSensorFunction->MMPF_Sensor_DoAFOperation(m_ubDo3ASensorId);
    	    gsSensorFunction->MMPF_Sensor_DoIQOperation(m_ubDo3ASensorId);
    	    m_ulISPFrameCount++;
    	    #endif
        }
    
        if (m_bWaitISPOperationDoneSig) {

			if (MMPF_Display_GetActivePipeNum(m_ubStopPreviewSnrId) == 0 &&
			    m_bLinkISPSel[m_ubStopPreviewSnrId] == MMP_TRUE) {
                
                ubVifId = MMPF_Sensor_GetVIFPad(m_ubStopPreviewSnrId);
                
				gbFrameEndFlag[m_ubStopPreviewSnrId] |= MMPF_SENSOR_ISP_FRAME_END;

				if (gbFrameEndFlag[m_ubStopPreviewSnrId] == (MMPF_SENSOR_ISP_FRAME_END | MMPF_SENSOR_VIF_FRAME_END)) {
					gbFrameEndFlag[m_ubStopPreviewSnrId] = MMPF_SENSOR_FRAME_END_NONE;
	            	MMPF_VIF_EnableInputInterface(ubVifId, MMP_FALSE);
	            }

	            MMPF_VIF_IsOutputEnable(ubVifId, &bVIFOutEn);

	            if (!bVIFOutEn) {
		            m_bWaitISPOperationDoneSig = MMP_FALSE;
	            	MMPF_OS_ReleaseSem(m_ISPOperationDoneSem);
	            }
	        }
        }
		
		#if (SUPPORT_ISP_TIMESHARING)
        MMPF_OS_ReleaseSem(m_IspFrmEndSem);
        
		if (m_bRetriggerRawF == MMP_TRUE) {
			
			m_bRetriggerRawF = MMP_FALSE;
			
            MMPF_RAWPROC_UpdateFetchBufIndex(m_ubRetriggerRawFRawId, MMP_RAW_STORE_PLANE0, 1);

            if (!MMPF_RAWPROC_IsFetchBusy()) {
                MMPF_RAWPROC_FetchData (m_ubRetriggerRawFSnrId,
                                        m_ubRetriggerRawFRawId,
	                                	MMP_RAW_FETCH_NO_ROTATE,
	                                	MMP_RAW_FETCH_NO_MIRROR);
            }
        }
        #endif
    }

    if (intsrc & ISP_INT_CI_FRM_ST)
    {
    	if (gsHdrCfg.bVidEnable == MMP_FALSE &&
    		gsHdrCfg.bDscEnable == MMP_FALSE)
    	{
	    	if (m_bISP3AStatus) {
		        #if (ISP_USE_TASK_DO3A == 1)
	        	MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_ISP_FRM_ST, MMPF_OS_FLAG_SET);
		        #else
		        gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
		        #endif
	    	}
    	}

		#if (DSC_MJPEG_FPS_CTRL_EN)
		if (m_bEncodeMJpegForWifi) {
			if (!MMPF_MJPEG_FpsControl()) {
				MMPF_MJPEG_EnableEncode();
			}
        }
        #endif
	}

	if (intsrc & ISP_INT_VIF_FRM_ST)
	{
		if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) &&
			(gsHdrCfg.ubMode == HDR_MODE_STAGGER))
		{
			static MMP_ULONG ulIspFrameCnt = 0;

			ulIspFrameCnt++;

			if (m_bISP3AStatus && ulIspFrameCnt % 2 == 0) {
		        #if (ISP_USE_TASK_DO3A == 1)
	        	MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_VIF_FRM_ST, MMPF_OS_FLAG_SET);
	        	MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_ISP_FRM_ST, MMPF_OS_FLAG_SET);
		        #else
		        gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
		        #endif
	    	}
    	}
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetEV
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetEV(MMP_LONG* plEv)
{
	#if (ISP_EN)
	*plEv = (MMP_LONG)ISP_IF_AE_GetEV();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetEVBias
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetEVBias(MMP_LONG* plEvBias, MMP_ULONG* pulEvBiasBase)
{
	#if (ISP_EN)
	ISP_IF_AE_GetEVBias((ISP_INT32*)plEvBias, (ISP_UINT32*)pulEvBiasBase);
	#endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetShutter
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetShutter(MMP_ULONG* pulShutter)
{
	#if (ISP_EN)
	*pulShutter = (MMP_ULONG)ISP_IF_AE_GetShutter();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetShutterBase
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetShutterBase(MMP_ULONG* pulShutterBase)
{
	#if (ISP_EN)
	*pulShutterBase = (MMP_ULONG)ISP_IF_AE_GetShutterBase();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetGain
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetGain(MMP_ULONG* pulGain)
{
	#if (ISP_EN)
	*pulGain = (MMP_ULONG)ISP_IF_AE_GetGain();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetGainBase
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetGainBase(MMP_ULONG* pulGainBase)
{
	#if (ISP_EN)
	*pulGainBase = (MMP_ULONG)ISP_IF_AE_GetGainBase();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetGainDB
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetGainDB(MMP_ULONG* pulGainDB)
{
	#if (ISP_EN)
	double dTemp = 0;

	dTemp = ((double)ISP_IF_AE_GetGain() / (double)ISP_IF_AE_GetGainBase());
	dTemp = 20 * log10(dTemp);
	
	*pulGainDB = (MMP_ULONG)dTemp;
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetExposureTime
//  Description : The exposure time unit is us.
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetExposureTime(MMP_ULONG* pulExpTime)
{
	#if (ISP_EN)
	*pulExpTime = (ISP_IF_AE_GetShutter()*1000000) / ISP_IF_AE_GetShutterBase();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetISOSpeed
//  Description : Suppose the 1x gain is ISO100.
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetISOSpeed(MMP_ULONG* pulIso)
{
	#if (ISP_EN)
	*pulIso= (ISP_IF_AE_GetGain() * 100) / ISP_IF_AE_GetGainBase();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetWBMode
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetWBMode(MMP_UBYTE* pubMode)
{
	#if (ISP_EN)
	*pubMode = (MMP_UBYTE)ISP_IF_AWB_GetMode();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetColorTemp
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetColorTemp(MMP_ULONG* pulTemp)
{
	#if (ISP_EN)
	*pulTemp = ISP_IF_AWB_GetColorTemp();
	#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_Set3AFunction
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC sFunc, int pParam)
{
#if (ISP_EN)
    switch(sFunc)
    {
        #if (SUPPORT_AUTO_FOCUS)
        case MMP_ISP_3A_FUNC_SET_AF_ENABLE:
            ISP_IF_AF_Control((ISP_AF_CTL)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AF_MODE:
            ISP_IF_AF_SetMode((ISP_AF_MODE)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AF_POS:
            ISP_IF_AF_SetPos((MMP_USHORT)pParam, 0);
        break;
        #endif
        case MMP_ISP_3A_FUNC_SET_AE_ENABLE:
            ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AE, (MMP_UBYTE)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_EV:
            ISP_IF_AE_SetEV((MMP_LONG)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_ISO:
            ISP_IF_AE_SetISO((MMP_ULONG)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AE_FAST_MODE:
            ISP_IF_AE_SetFastMode((MMP_UBYTE)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AE_FLICKER_MODE:
            switch(pParam) {
            case MMP_SNR_DEBAND_DSC_60HZ:
            case MMP_SNR_DEBAND_VIDEO_60HZ:
        		ISP_IF_AE_SetFlicker(ISP_AE_FLICKER_60HZ);
                break;
            case MMP_SNR_DEBAND_DSC_50HZ:
            case MMP_SNR_DEBAND_VIDEO_50HZ:
        		ISP_IF_AE_SetFlicker(ISP_AE_FLICKER_50HZ);
                break;
            default:
        		ISP_IF_AE_SetFlicker(ISP_AE_FLICKER_AUTO);
        		break;
            }
        break;
        case MMP_ISP_3A_FUNC_SET_AE_METER_MODE:
            ISP_IF_AE_SetMetering((ISP_AE_METERING)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AWB_ENABLE:
        	ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AWB, (MMP_UBYTE)pParam);
        	
            if (pParam)
                ISP_IF_AWB_SetMode(ISP_AWB_MODE_AUTO);
            else
                ISP_IF_AWB_SetMode(ISP_AWB_MODE_MANUAL);        
        break;
        case MMP_ISP_3A_FUNC_SET_AWB_MODE:
            ISP_IF_AWB_SetMode((ISP_AWB_MODE)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AWB_FAST_MODE:
            ISP_IF_AWB_SetFastMode((MMP_UBYTE)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_AWB_COLOR_TEMP:
            ISP_IF_AWB_SetColorTemp((MMP_ULONG)pParam);
        break;
        case MMP_ISP_3A_FUNC_SET_3A_ENABLE:
            m_bISP3AStatus = (MMP_BOOL)pParam;
            
            if (m_bISP3AStatus)
                ISP_IF_3A_Control(ISP_3A_ENABLE);
            else 
                ISP_IF_3A_Control(ISP_3A_DISABLE);
        break;
        default:
        //
        break;
    }
#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_SetIQFunction
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_SetIQFunction(MMP_ISP_IQ_FUNC sFunc, int pParam)
{
#if (ISP_EN)

    switch(sFunc)
    {
        case MMP_ISP_IQ_FUNC_SET_SHARPNESS:
            if (pParam > 128 || pParam < -128) {
                return MMP_SENSOR_ERR_PARAMETER;
            }
            ISP_IF_F_SetSharpness((MMP_SHORT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_EFFECT:
            if (pParam >= ISP_IMAGE_EFFECT_NUM) {
                return MMP_SENSOR_ERR_PARAMETER;
            }   
            ISP_IF_F_SetImageEffect((ISP_IMAGE_EFFECT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_GAMMA:
            if (pParam > 128 || pParam < -128) {
                return MMP_SENSOR_ERR_PARAMETER;
            }
            ISP_IF_F_SetGamma((MMP_SHORT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_CONTRAST:
            if (pParam > 128 || pParam < -128) {
                return MMP_SENSOR_ERR_PARAMETER;
            }
            ISP_IF_F_SetContrast((MMP_SHORT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_SATURATION:
            if (pParam > 128 || pParam < -128) {
                return MMP_SENSOR_ERR_PARAMETER;
            }
            ISP_IF_F_SetSaturation((MMP_SHORT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_WDR:
            ISP_IF_F_SetWDREn((MMP_USHORT)pParam);
        break;
        case MMP_ISP_IQ_FUNC_SET_HUE:
            ISP_IF_F_SetHue((MMP_SHORT)pParam);
        break;
        default:
        //
        break;
    }
#endif
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_UpdateInputSize
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_UpdateInputSize(MMP_USHORT usWidth, MMP_USHORT usHeight)
{
	#if (ISP_EN)
    ISP_IF_3A_Control(ISP_3A_DISABLE);
    ISP_IF_IQ_SetISPInputLength(usWidth, usHeight);
    ISP_IF_IQ_UpdateInputSize();
    ISP_IF_3A_Control(ISP_3A_ENABLE);
	#endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_AllocateBuffer
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_AllocateBuffer(MMP_ULONG ulStartAddr, MMP_ULONG *ulSize)
{
    if ((m_gsISPCoreID == 868) || (m_gsISPCoreID == 888)) {
        MMP_ULONG ulCurSize = 0;
        
        m_glISPBufferStartAddr = ulStartAddr;
        ulCurSize += ALIGN32(ISP_BUFFER_SIZE);
        *ulSize = ulCurSize;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_GetHWBufferSize
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_GetHWBufferSize(MMP_ULONG *ulSize)
{
    if ((m_gsISPCoreID == 868) || (m_gsISPCoreID == 888)) {
        *ulSize = ISP_DFT_BUF_SIZE + ISP_BUFFER_NUM * ISP_BUFFER_SIZE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_EnableInterrupt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_EnableInterrupt(MMP_ULONG ulFlag, MMP_BOOL bEnable)
{
    AITPS_ISP pISP = AITC_BASE_ISP;

    if (bEnable) {
        pISP->ISP_INT_CPU_SR = ulFlag;
        pISP->ISP_INT_CPU_EN |= ulFlag;
    } 
    else {
        pISP->ISP_INT_CPU_EN &= ~ulFlag;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_OpenInterrupt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_ISP_OpenInterrupt(MMP_BOOL bEnable)
{
    AITPS_AIC pAIC = AITC_BASE_AIC;

    if (bEnable) {
	    RTNA_AIC_Open(pAIC, AIC_SRC_ISP, isp_isr_a,
	                AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
	    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_ISP);
    } 
    else {
        RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_ISP);
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ISP_IsFrameEndStage
//  Description :
//------------------------------------------------------------------------------
MMP_BOOL MMPF_ISP_IsFrameEndStage(void)
{
#if (SUPPORT_ISP_TIMESHARING)
    MMP_USHORT usSemCount = 0;

    if (MMPF_OS_QuerySem(m_IspFrmEndSem, &usSemCount) == 0) {
        if (usSemCount) {
            return MMP_TRUE;
    	}
    }

    return MMP_FALSE;
#else
	return MMP_TRUE;
#endif
}

#if 0
void _____Sensor_Functions_____(){}
#endif

MMP_ULONG MMPF_Sensor_GetLightSensorLux(void)
{
extern unsigned int LightSensorLux(void);
  unsigned int lux = (unsigned int)LightSensorLux() ;
  return lux ;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_GetVIFPad
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_Sensor_GetVIFPad(MMP_UBYTE ubSnrSel)
{
	return gsSensorFunction->MMPF_Sensor_GetCurVifPad(ubSnrSel);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_GetSnrIdFromVifId
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_Sensor_GetSnrIdFromVifId(MMP_UBYTE ubVifId)
{
    MMP_UBYTE ubSnrSel = 0;

    for (ubSnrSel = 0; ubSnrSel < VIF_SENSOR_MAX_NUM; ubSnrSel++) {
	    
	    if (ubVifId == gsSensorFunction->MMPF_Sensor_GetCurVifPad(ubSnrSel))
	        return ubSnrSel;
    }
    
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_GetParam
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_GetParam(MMP_UBYTE ubSnrSel, MMPF_SENSOR_PARAM param_type, void* param)
{
    switch(param_type) {
    case MMPF_SENSOR_ISP_FRAME_COUNT:
        *(MMP_ULONG*)param = m_ulISPFrameCount;
        break;
    case MMPF_SENSOR_CURRENT_PREVIEW_MODE:
        *(MMP_USHORT*)param = gsCurPreviewMode[ubSnrSel];
        break;
    case MMPF_SENSOR_CURRENT_CAPTURE_MODE:
    	*(MMP_USHORT*)param = gsCurCaptureMode[ubSnrSel];
    	break;
    default:
    	RTNA_DBG_Str(0, "Not Support Parameter Type\r\n");
    	break;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_SetParam
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_SetParam(MMP_UBYTE ubSnrSel, MMPF_SENSOR_PARAM param_type, void* param)
{
    switch (param_type) {
    case MMPF_SENSOR_ISP_FRAME_COUNT:
        m_ulISPFrameCount = *(MMP_ULONG*)param;
        break;       
    case MMPF_SENSOR_CURRENT_PREVIEW_MODE:
        gsCurPreviewMode[ubSnrSel] = *(MMP_USHORT*)param;
        break;
    case MMPF_SENSOR_CURRENT_CAPTURE_MODE:
		gsCurCaptureMode[ubSnrSel] = *(MMP_USHORT*)param;
    	break;
    default:
    	RTNA_DBG_Str(0, "Not Support Parameter Type\r\n");
    	break;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_Set3AInterrupt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_Set3AInterrupt(MMP_UBYTE ubSnrSel, MMP_BOOL bEnable)
{	
	MMP_UBYTE ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);
	
	MMPF_VIF_EnableInterrupt(ubVifId, VIF_INT_FRM_ST, bEnable);
	MMPF_VIF_EnableInterrupt(ubVifId, VIF_INT_GRAB_END, bEnable);

	MMPF_ISP_EnableInterrupt(ISP_INT_VIF_FRM_ST, bEnable);
	MMPF_ISP_EnableInterrupt(ISP_INT_CI_FRM_ST, bEnable);
	MMPF_ISP_EnableInterrupt(ISP_INT_COLOR_FRM_END, bEnable);
	MMPF_ISP_EnableInterrupt(ISP_INT_AE_WINDOW_ACC_DONE, bEnable);	

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_LinkFunctionTable
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_LinkFunctionTable(void)
{
    gsSensorFunction = SensorFuncTable;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_Wait3AConverge
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_Wait3AConverge(MMP_UBYTE ubSnrSel)
{
    MMP_ULONG ulframeCnt = 0;
    do {
        MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulframeCnt);
#ifdef PROJECT_LD
        if ((ulframeCnt >= MIN_AE_CONVERGE_FRAME_NUM) && (1))
#else
        if ((ulframeCnt >= MIN_AE_CONVERGE_FRAME_NUM) && (ISP_IF_AE_AEConvergeCheck() == 1))
#endif            
        {
#if SUPPORT_UVC_JPEG==0
            printc("%d : AEConvergeCheck OK %d\r\n", ulframeCnt, OSTime);
#endif
            ISP_IF_AE_SetFastMode(0);
            ISP_IF_AWB_SetFastMode(0);
            
            break;
        }
        MMPF_OS_Sleep(5); // for 120fps, the delay time should less than 8ms
    } while (ulframeCnt <= MAX_AE_CONVERGE_FRAME_NUM);
    
    
    // SEAN : 20170510 , slow down AE 
    //gsSensorFunction->MMPF_Sensor_Switch3ASpeed( ubSnrId , MMP_TRUE );
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_Is3AConverge
//  Description :
//------------------------------------------------------------------------------
MMP_BOOL MMPF_Sensor_Is3AConverge(MMP_UBYTE ubSnrSel)
{
    MMP_ULONG ulframeCnt = 0;

    MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulframeCnt);

#ifdef PROJECT_LD
    if ((1) &&
        (ulframeCnt >= MIN_AE_CONVERGE_FRAME_NUM))
        return MMP_TRUE;
#else
    if ((ISP_IF_AE_AEConvergeCheck() == 1) &&
        (ulframeCnt >= MIN_AE_CONVERGE_FRAME_NUM))
        return MMP_TRUE;
#endif
        
    else if (ulframeCnt >= MAX_AE_CONVERGE_FRAME_NUM)
        return MMP_TRUE;

    return MMP_FALSE;
}

MMP_BOOL MMPF_Sensor_SetNightVisionMode(MMP_UBYTE ubSnrSel,MMP_UBYTE val )
{
    MMPF_SENSOR_CUSTOMER *cust ;
    cust = SNR_Cust(ubSnrSel) ;      
    switch( val ) {
      case 0:
        if(cust) cust->MMPF_Cust_NightVision(  0 ) ;        
        break;
      case 1:
        if(cust) cust->MMPF_Cust_NightVision(  1 ) ;
        break ;
    }
    return 0;
}

MMP_BOOL MMPF_Sensor_SetTest(MMP_UBYTE ubSnrSel, MMP_UBYTE val )
{
    MMPF_SENSOR_CUSTOMER *cust ;
    cust = SNR_Cust(ubSnrSel) ;      
    printc("%s val %d\r\n", __func__, val);
    switch( val ) {
        case 0:
            if(cust) cust->MMPF_Cust_Test(  0 ) ;        
            break;
        case 1:
            if(cust) cust->MMPF_Cust_Test(  1 ) ;
            break;
        case 2:
            if(cust) cust->MMPF_Cust_Test(  2 ) ;
            break;
        case 3:
            if(cust) cust->MMPF_Cust_Test(  3 ) ;
            break;
        default:
            break;
    }
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_ProcessCmd
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Sensor_ProcessCmd(void)
{
    MMP_ULONG   ulParameter[MAX_HIF_ARRAY_SIZE], i;
    MMP_USHORT  usData;
    MMP_USHORT  usCommand;
    MMP_USHORT  usIBCPipe;
    MMP_ERR     err;
    MMP_ULONG   ulSize;
    MMP_UBYTE   ubSnrId = 0;
    MMP_UBYTE   ubVifId = 0;

    usCommand = m_ulHifCmd[GRP_IDX_FLOWCTL];

	for (i = 0; i < MAX_HIF_ARRAY_SIZE; i ++) {
        ulParameter[i] = m_ulHifParam[GRP_IDX_FLOWCTL][i];
    }
    
	m_ulHifCmd[GRP_IDX_FLOWCTL] = 0;

    switch (usCommand & (FUNC_MASK|GRP_MASK)) {
    /* Sensor */
    case HIF_FCTL_CMD_INIT_SENSOR:
        m_ulISPFrameCount = 0;

        ubSnrId = (MMP_UBYTE)ulParameter[0];

        MMPF_Sensor_LinkFunctionTable();
        
       	err = gsSensorFunction->MMPF_Sensor_InitCustFunc(ubSnrId);

        if (gsSensorFunction->MMPF_Sensor_GetSnrType(ubSnrId) == MMPF_VIF_SNR_TYPE_BAYER)
            gsSensorFunction->MMPF_Sensor_LinkISPSelect(ubSnrId, MMP_TRUE);
        else
            gsSensorFunction->MMPF_Sensor_LinkISPSelect(ubSnrId, MMP_FALSE);
        
#if 0 // SEAN:TBD        
            if (err == MMP_ERR_NONE) {
              gsSensorFunction->MMPF_Sensor_InitializeVIF(ubSnrId);
              gsSensorFunction->MMPF_Sensor_InitializeISP(ubSnrId);
              MMPF_Sensor_Set3AInterrupt(ubSnrId, MMP_TRUE);
              err = gsSensorFunction->MMPF_Sensor_Initialize(ubSnrId);
              ubVifId = MMPF_Sensor_GetVIFPad(ubSnrId);
              m_ulISPFrameCount = 0;
              m_ulVIFFrameCount[ubVifId] = 0;
            }
		
#else
            gsSensorFunction->MMPF_Sensor_InitializeVIF(ubSnrId);
            if (m_bLinkISPSel[ubSnrId])
            {
                if (1/*MMPF_Sensor_Get3AState() == MMPF_SENSOR_3A_RESET*/) // do alyways
                {

                    gsSensorFunction->MMPF_Sensor_InitializeISP(ubSnrId);
                    MMPF_Sensor_Set3AInterrupt(ubSnrId, MMP_TRUE);
                    MMPF_Sensor_Set3AState(MMPF_SENSOR_3A_SET);
                    gsSensorFunction->MMPF_Sensor_FastDo3AOperation(ubSnrId, MMPF_SENSOR_FAST_3A_RESERVE_SETTING);
                }
                ubVifId = MMPF_Sensor_GetVIFPad(ubSnrId);
                m_ulVIFFrameCount[ubVifId] = 0;
                m_ulISPFrameCount = 0;
                pSnr->bIspAeFastConverge = MMP_TRUE;
                pSnr->ulIspAeConvCount = 0;
                pSnr->bIspAwbFastConverge = MMP_TRUE;
                pSnr->ulIspAwbConvCount = 0;
                
            }

            err = gsSensorFunction->MMPF_Sensor_Initialize(ubSnrId);
            /*
            TEST CODE : Get Light sensor here
            Light sensor need time to exposure
            Put here after sensor initialized to get more time
            */
#endif

        if (err == MMP_ERR_NONE) {
	        gsCurPreviewMode[ubSnrId] = (MMP_USHORT)ulParameter[1];
	        err = gsSensorFunction->MMPF_Sensor_SetPreviewMode(ubSnrId, gsCurPreviewMode[ubSnrId]);

	        //err = gsSensorFunction->MMPF_Sensor_SetSensorRotate(ubSnrId, gsCurPreviewMode[ubSnrId], MMPF_SENSOR_ROTATE_RIGHT_180);
	        //err = gsSensorFunction->MMPF_Sensor_SetSensorFlip(ubSnrId, gsCurPreviewMode[ubSnrId], MMPF_SENSOR_COLUMN_FLIP);

	        m_ulISPFrameCount = 0;
            
            if (err != MMP_ERR_NONE) {
                RTNA_DBG_Str(0, "Sensor Set Res Fail\r\n");
            }
            else {
              // SEAN : Fast mode
                //printc("[SEAN] : Set Fast Mode0\r\n");
                //ISP_IF_AE_SetFastMode(1);
                //ISP_IF_AWB_SetFastMode(1);
  
                if (m_bLinkISPSel[ubSnrId]) {
                    
                    gsSensorFunction->MMPF_Sensor_FastDo3AOperation(ubSnrId, MMPF_SENSOR_FAST_3A_CHANGE_MODE);
                }
              
                #if (TVDEC_SNR_USE_VIF_CNT_AS_FIELD_CNT)
                if (ubSnrId == SCD_SENSOR) 
                {
                    /* WorkAround for get the correct field number */
                    extern MMP_BOOL	 gbPipeIsActive[];
                    extern MMP_UBYTE gbPipeLinkedSnr[];
                    
                    // For not to close VIF out
                    gbPipeIsActive[MMP_IBC_PIPE_2] = MMP_TRUE;
	                gbPipeLinkedSnr[MMP_IBC_PIPE_2] = ubSnrId;
	                
                    MMPF_VIF_EnableInputInterface(ubVifId, MMP_TRUE);
                    MMPF_VIF_EnableOutput(ubVifId, MMP_TRUE);
                }
                #endif
            }
        } 
        else {
            RTNA_DBG_Str(0, "Sensor Initial Fail\r\n");
        }

        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, err);
        break;
    case HIF_FCTL_CMD_SENSOR_CONTROL:
        switch (usCommand & SUB_MASK) {
        case SET_REGISTER:
            gsSensorFunction->MMPF_Sensor_SetReg((MMP_UBYTE)ulParameter[0], INT_WORD0(ulParameter[1]), INT_WORD1(ulParameter[1]));
            break;
        case GET_REGISTER:
            gsSensorFunction->MMPF_Sensor_GetReg(INT_WORD0(ulParameter[0]), INT_WORD1(ulParameter[0]), &usData);
            MMPF_HIF_FeedbackParamW(GRP_IDX_FLOWCTL, 0, usData);
            break;
        }
        break;        
    case HIF_FCTL_CMD_SET_SENSOR_RES_MODE:
    	switch (usCommand & SUB_MASK) {
        case SENSOR_PREVIEW_MODE:
            ubSnrId = INT_WORD0(ulParameter[0]);
	        if (gsCurPreviewMode[ubSnrId] != INT_WORD1(ulParameter[0])) {
	            gsSensorFunction->MMPF_Sensor_SetPreviewMode(ubSnrId, INT_WORD1(ulParameter[0]));
    	        m_ulISPFrameCount = 0;
	        }
	        break;
        case SENSOR_CAPTURE_MODE:
        	MMPF_Sensor_SetParam(ulParameter[0], MMPF_SENSOR_CURRENT_CAPTURE_MODE, &ulParameter[1]);
        	break;
        }
        break;
    case HIF_FCTL_CMD_SET_SENSOR_MODE:
        gsSensorMode[(MMP_UBYTE)ulParameter[0]] = (MMP_USHORT)ulParameter[1];
        break;       
    case HIF_FCTL_CMD_SET_SENSOR_FLIP:
        gsSensorFunction->MMPF_Sensor_SetSensorFlip((MMP_UBYTE)ulParameter[0], gsCurPreviewMode[(MMP_UBYTE)ulParameter[0]], (MMP_UBYTE)ulParameter[1]);
        break;
    case HIF_FCTL_CMD_SET_SENSOR_ROTATE:
        gsSensorFunction->MMPF_Sensor_SetSensorRotate((MMP_UBYTE)ulParameter[0], gsCurPreviewMode[(MMP_UBYTE)ulParameter[0]], (MMP_UBYTE)ulParameter[1]);
        break;
    case HIF_FCTL_CMD_POWERDOWN_SENSOR:
        ubSnrId = (MMP_UBYTE)ulParameter[0];
        MMPF_Sensor_LinkFunctionTable();
        gsSensorFunction->MMPF_Sensor_PowerDown(ubSnrId);
        MMPF_Sensor_Set3AInterrupt(ubSnrId, MMP_FALSE);
        break;
    case HIF_FCTL_CMD_GET_SENSOR_DRV_PARAM:
        switch(ulParameter[1]) {
        case MMP_SNR_DRV_PARAM_SCAL_IN_RES:
	        {
    	    	MMP_ULONG ulW, ulH;

                gsSensorFunction->MMPF_Sensor_GetScalInputRes(INT_BYTE0(ulParameter[0]),
                                                              INT_WORD0(ulParameter[2]),
                                                              &ulW, &ulH); 

    	        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, ulW);
    	        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 4, ulH);
	        }
			break;
        case MMP_SNR_DRV_PARAM_PREVIEW_RES_NUM:
			MMPF_HIF_FeedbackParamB(GRP_IDX_FLOWCTL, 0, m_SensorRes.ubSensorModeNum);
			break;
        case MMP_SNR_DRV_PARAM_DEF_PREVIEW_RES:
			MMPF_HIF_FeedbackParamB(GRP_IDX_FLOWCTL, 0, m_SensorRes.ubDefPreviewMode);
			break;
        case MMP_SNR_DRV_PARAM_FPSx10:
            {
                MMP_ULONG ulFpsx10;

                gsSensorFunction->MMPF_Sensor_GetTargetFpsx10(
                                                    INT_BYTE0(ulParameter[0]),
                                                    INT_WORD0(ulParameter[2]),
                                                    &ulFpsx10);
                MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, ulFpsx10);
            }
            break;
        }
        break; 
    case HIF_FCTL_CMD_CHANGE_SENSOR_RES_MODE:
        ubSnrId = (MMP_UBYTE)ulParameter[0];
		if (gsCurPreviewMode[ubSnrId] != (MMP_USHORT)ulParameter[1]) {
        	m_ulISPFrameCount = 0;
            gsSensorFunction->MMPF_Sensor_FastDo3AOperation(ubSnrId, MMPF_SENSOR_FAST_3A_RESERVE_SETTING);
            gsSensorFunction->MMPF_Sensor_ChangeMode(ubSnrId, (MMP_USHORT)ulParameter[1], (MMP_USHORT)ulParameter[2]);
        	gsSensorFunction->MMPF_Sensor_FastDo3AOperation(ubSnrId, MMPF_SENSOR_FAST_3A_CHANGE_MODE);
        }
    	break;
    
    /* ISP/IQ */
    case HIF_FCTL_CMD_3A_FUNCTION:
        switch (usCommand & SUB_MASK) {
            case SET_HW_BUFFER:
                MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, MMPF_ISP_AllocateBuffer(ulParameter[0], &ulSize));
                MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 4, ulSize);
                break;
            case GET_HW_BUFFER_SIZE:
                MMPF_ISP_GetHWBufferSize(&ulSize);
                MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, ulSize);
                break;
         }
        break;
    case HIF_FCTL_CMD_AE_FUNC:
        switch (usCommand & SUB_MASK) {
        case SET_PREV_GAIN:
            gsSensorFunction->MMPF_Sensor_SetGain((MMP_UBYTE)ulParameter[0], ulParameter[1]);
            break;
        case SET_CAP_GAIN:
            gbISPAGC = ulParameter[0];
            break;
        case SET_PREV_EXP_LIMIT:
            gsSensorFunction->MMPF_Sensor_SetExposureLimit(ulParameter[0], ulParameter[1], ulParameter[2]);
            break;
        case SET_CAP_EXP_LIMIT:
            glISPExpLimitBufaddr 		= ulParameter[0]; 
            glISPExpLimitDatatypeByByte = ulParameter[1];
            glISPExpLimitSize 			= ulParameter[2];
            break;
        case SET_PREV_SHUTTER:
            break;
        case SET_CAP_SHUTTER:
            break;
        case SET_PREV_EXP_VAL:
            MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_EV, ulParameter[0]);
            break;
        case SET_CAP_EXP_VAL:
            glISPExpValForCapture = ulParameter[0];
            break;
        }
        break;
    
    /* Misc */
    case HIF_FCTL_CMD_SET_PREVIEW_ENABLE:
    	switch (usCommand & SUB_MASK) {
        case ENABLE_PREVIEW:
        	MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, (MMP_ULONG)MMPF_Display_StartPreview((MMP_UBYTE)ulParameter[0], (MMP_USHORT)ulParameter[1], (MMP_BOOL)ulParameter[2]));
        	break;
        case DISABLE_PREVIEW:
            MMPF_Display_StopPreview((MMP_UBYTE)ulParameter[0], (MMP_USHORT)ulParameter[1], (MMP_BOOL)ulParameter[2]);
        	break;
        case ENABLE_PIPE:
            MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, MMPF_Display_EnablePipe((MMP_UBYTE)ulParameter[0], (MMP_USHORT)ulParameter[1]));
            break;
        case DISABLE_PIPE:
            MMPF_Display_DisablePipe((MMP_UBYTE)ulParameter[0], (MMP_USHORT)ulParameter[1]);
            break;
        }
        break;
    case HIF_FCTL_CMD_SET_ROTATE_BUF:
        switch (usCommand & SUB_MASK) {
        case BUFFER_ADDRESS:
        case BUFFER_COUNT:
            break;
        }
        break;
    case HIF_FCTL_CMD_GET_PREVIEW_BUF:
        usIBCPipe = (MMP_USHORT)ulParameter[0];
        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 0, glPreviewBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]]);
        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 4, glPreviewUBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]]);
        MMPF_HIF_FeedbackParamL(GRP_IDX_FLOWCTL, 8, glPreviewVBufAddr[usIBCPipe][gbExposureDoneBufIdx[usIBCPipe]]);
        break;
    case HIF_FCTL_CMD_SET_PREVIEW_BUF:
        switch (usCommand & SUB_MASK) {
        case BUFFER_ADDRESS:
            glPreviewBufAddr[(MMP_USHORT)ulParameter[0]][(ulParameter[0] >> 16) & 0xFF] = ulParameter[1];
            glPreviewUBufAddr[(MMP_USHORT)ulParameter[0]][(ulParameter[0] >> 16) & 0xFF] = ulParameter[2];
            glPreviewVBufAddr[(MMP_USHORT)ulParameter[0]][(ulParameter[0] >> 16) & 0xFF] = ulParameter[3];
            break;  
        case BUFFER_COUNT:
            gbPreviewBufferCount[(MMP_USHORT)ulParameter[0]] = (MMP_UBYTE)(ulParameter[0] >> 16);
            break;
        case BUFFER_WIDTH:
        case BUFFER_HEIGHT:
            break;
        }
        break;
    case HIF_FCTL_CMD_SET_IBC_LINK_MODE:
        switch (usCommand & SUB_MASK) {
        case LINK_NONE:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] = MMP_IBC_LINK_NONE;
            break;
        case LINK_DISPLAY:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_DISPLAY;
            break;
        case LINK_VIDEO:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_VIDEO;
            gbIBCLinkEncId[(MMP_USHORT)ulParameter[0]] = (MMP_UBYTE)(ulParameter[0] >> 16);
            MMPF_VIDENC_SetVideoPath((MMP_USHORT)ulParameter[0]);
            break;
        case LINK_WIFI:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_WIFI;
            gbIBCLinkEncId[(MMP_USHORT)ulParameter[0]] = (MMP_UBYTE)(ulParameter[0] >> 16);
            MMPF_VIDENC_SetVideoPath((MMP_USHORT)ulParameter[0]);
            break;
        case LINK_DMA:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_ROTATE;
            break;
        case LINK_GRAY:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_GRAY;
            gbIBCLinkEncId[(MMP_USHORT)ulParameter[0]] = (MMP_UBYTE)(ulParameter[0] >> 16);
            break;
        case UNLINK_GRAY:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_GRAY);
            break;
        case LINK_IVA:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_IVA;
            gbIBCLinkEncId[(MMP_USHORT)ulParameter[0]] = (MMP_UBYTE)(ulParameter[0] >> 16);
            break;
        case UNLINK_IVA:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_IVA);
            break;
            
        case LINK_GRAPHIC:
        	m_bPipeLinkGraphic[(MMP_UBYTE)ulParameter[0]] = MMP_TRUE;
            break;
        case UNLINK_GRAPHIC:
        	m_bPipeLinkGraphic[(MMP_UBYTE)ulParameter[0]] = MMP_FALSE;
            break;
    	case LINK_USB:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_USB;
            break;
    	case UNLINK_USB:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_USB);
            break;
    	case LINK_LDC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_LDC;
            break;
    	case UNLINK_LDC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_LDC);
            break;
    	case LINK_MDTC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_MDTC;
            break;
    	case UNLINK_MDTC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_MDTC);
            break; 
        case LINK_GRA2MJEPG:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= (MMP_IBC_LINK_GRA2MJPEG);
            if ((MMP_BOOL)(ulParameter[1] >> 16)) {
                gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= (MMP_IBC_LINK_DISPLAY);
            }
            break;
        case UNLINK_GRA2MJPEG:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_GRA2MJPEG);
            break;
        case LINK_GRA2STILLJPEG:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= (MMP_IBC_LINK_GRA2STILLJPG);
            if ((MMP_BOOL)(ulParameter[1] >> 16)) {
                gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= (MMP_IBC_LINK_DISPLAY);
            }
            break;
        case UNLINK_GRA2STILLJPEG:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_GRA2STILLJPG);
            break;
    	case LINK_GRA2UVC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] |= MMP_IBC_LINK_GRA2UVC;
            break;
    	case UNLINK_GRA2UVC:
            gIBCLinkType[(MMP_USHORT)ulParameter[0]] &= ~(MMP_IBC_LINK_GRA2UVC);
            break;
        case GET_IBC_LINK_ATTR:
        	MMPF_HIF_FeedbackParamW(GRP_IDX_FLOWCTL, 0, gIBCLinkType[(MMP_USHORT)ulParameter[0]]);
            MMPF_HIF_FeedbackParamW(GRP_IDX_FLOWCTL, 2, 0);
            MMPF_HIF_FeedbackParamW(GRP_IDX_FLOWCTL, 4, 0);
        	MMPF_HIF_FeedbackParamW(GRP_IDX_FLOWCTL, 6, 0);
        	break;
        }
        break;
    case HIF_FCTL_CMD_SET_RAW_PREVIEW:
        switch (usCommand & SUB_MASK) {
        case ENABLE_RAW_PATH:
        {
            MMP_UBYTE ubRawId = (MMP_UBYTE)(ulParameter[0]>>24);
            MMP_UBYTE ubRawColorFmt = (MMP_UBYTE)ulParameter[1];
           
        	if (gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable)
        	{
        	    MMP_UBYTE ubRawSrc = MMP_RAW_S_SRC_ISP_BEFORE_BS;
        	    
        	    if ((MMP_BOOL)ulParameter[0]) {
        	    
    	        	MMPF_Scaler_EnableInterrupt(MMP_SCAL_PIPE_0, MMP_SCAL_EVENT_DBL_FRM_ST, MMP_TRUE);
    				MMPF_Scaler_EnableInterrupt(MMP_SCAL_PIPE_1, MMP_SCAL_EVENT_DBL_FRM_ST, MMP_TRUE);
    	        	
    				MMPF_HDR_InitRawStoreSetting(ubRawId);
    				
    				ubRawSrc = MMP_RAW_S_SRC_ISP_BEFORE_BS;
                }
                else {

    	        	MMPF_Scaler_EnableInterrupt(MMP_SCAL_PIPE_0, MMP_SCAL_EVENT_DBL_FRM_ST, MMP_FALSE);
    				MMPF_Scaler_EnableInterrupt(MMP_SCAL_PIPE_1, MMP_SCAL_EVENT_DBL_FRM_ST, MMP_FALSE);

                    ubRawSrc = MMP_RAW_S_SRC_VIF0;
                }
                
	            MMPF_RAWPROC_ResetBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0);
	            MMPF_RAWPROC_EnableFetchPath((MMP_BOOL)ulParameter[0]);
	            MMPF_RAWPROC_EnableStorePath(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, ubRawSrc, (MMP_BOOL)(ulParameter[0]>>16));
	            
	            if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
	            	MMPF_RAWPROC_ResetBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1);
	            	MMPF_RAWPROC_EnableStorePath(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1, ubRawSrc, (MMP_BOOL)(ulParameter[0]>>16));
	            }
	            else {
	            	MMPF_RAWPROC_ResetBufIndex(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0);
	            	MMPF_RAWPROC_EnableStorePath(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0, ubRawSrc, (MMP_BOOL)(ulParameter[0]>>16));
	            }
            }
            else
            {
                if (ubRawColorFmt == MMP_RAW_COLORFMT_YUV420) {
                    
                    MMPF_RAWPROC_SetInterruptForYUV420Fmt(ubRawId);

    	            MMPF_RAWPROC_ResetBufIndex(ubRawId, MMP_RAW_STORE_PLANE0);
    	            MMPF_RAWPROC_EnableFetchPath((MMP_BOOL)ulParameter[0]);
    	            MMPF_RAWPROC_EnableStorePath(ubRawId, MMP_RAW_STORE_PLANE0, 
    	            							(MMP_RAW_STORE_SRC)(ulParameter[0]>>8), (MMP_BOOL)(ulParameter[0]>>16));
                    
                    MMPF_RAWPROC_ResetBufIndex(ubRawId, MMP_RAW_STORE_PLANE1);
	                MMPF_RAWPROC_EnableStorePath(ubRawId, MMP_RAW_STORE_PLANE1, 
	            							(MMP_RAW_STORE_SRC)(ulParameter[0]>>8), (MMP_BOOL)(ulParameter[0]>>16));
            
                    MMPF_RAWPROC_ResetBufIndex(ubRawId, MMP_RAW_STORE_PLANE2);
	                MMPF_RAWPROC_EnableStorePath(ubRawId, MMP_RAW_STORE_PLANE2, 
	            							(MMP_RAW_STORE_SRC)(ulParameter[0]>>8), (MMP_BOOL)(ulParameter[0]>>16));
	            	
	            	MMPF_RAWPROC_SetStoreYUV420Enable(ubRawId, MMP_RAW_STORE_PLANE0, (MMP_BOOL)(ulParameter[0]>>16));
	            	MMPF_RAWPROC_SetStoreYUV420Enable(ubRawId, MMP_RAW_STORE_PLANE1, (MMP_BOOL)(ulParameter[0]>>16));
	            	MMPF_RAWPROC_SetStoreYUV420Enable(ubRawId, MMP_RAW_STORE_PLANE2, (MMP_BOOL)(ulParameter[0]>>16));
                }
                else if (ubRawColorFmt == MMP_RAW_COLORFMT_YUV422) {
                    
                    MMPF_RAWPROC_SetInterruptForYUV422Fmt(ubRawId);

    	            MMPF_RAWPROC_ResetBufIndex(ubRawId, MMP_RAW_STORE_PLANE0);
    	            MMPF_RAWPROC_EnableFetchPath((MMP_BOOL)ulParameter[0]);
    	            MMPF_RAWPROC_EnableStorePath(ubRawId, MMP_RAW_STORE_PLANE0, 
    	            							(MMP_RAW_STORE_SRC)(ulParameter[0]>>8), (MMP_BOOL)(ulParameter[0]>>16));
                }
                else {
                
     	            MMPF_RAWPROC_ResetBufIndex(ubRawId, MMP_RAW_STORE_PLANE0);
    	            MMPF_RAWPROC_EnableFetchPath((MMP_BOOL)ulParameter[0]);
    	            MMPF_RAWPROC_EnableStorePath(ubRawId, MMP_RAW_STORE_PLANE0, 
    	            							(MMP_RAW_STORE_SRC)(ulParameter[0]>>8), (MMP_BOOL)(ulParameter[0]>>16));                                   
                }
            }
        }
            break;
        case ENABLE_RAWPREVIEW:
            m_bRawPathPreview[(MMP_UBYTE)ulParameter[0]] = (MMP_BOOL)ulParameter[1];
            break;
        case SET_RAWSTORE_ONLY:
            m_bRawStoreOnly[(MMP_UBYTE)ulParameter[0]] = (MMP_BOOL)ulParameter[1];
            break;
        case SET_FETCH_RANGE:
            MMPF_RAWPROC_SetFetchRange( (MMP_USHORT)ulParameter[0],
                                        (MMP_USHORT)(ulParameter[0]>>16),
                                        (MMP_USHORT)ulParameter[1],
                                        (MMP_USHORT)(ulParameter[1]>>16),
                                        (MMP_USHORT)ulParameter[2]);
            break;
        case SET_RAWSTORE_ADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBuffer(ulParameter[0], MMP_RAW_STORE_PLANE0, i, ulParameter[i+2]);
            }
            break;
        case SET_RAWSTORE_UADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBuffer(ulParameter[0], MMP_RAW_STORE_PLANE1, i, ulParameter[i+2]);
            }
            break;
        case SET_RAWSTORE_VADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBuffer(ulParameter[0], MMP_RAW_STORE_PLANE2, i, ulParameter[i+2]);
            }
            break;
        case SET_RAWSTORE_ENDADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBufferEnd(ulParameter[0], MMP_RAW_STORE_PLANE0, i, ulParameter[i+2]);
            }
            break;
        case SET_RAWSTORE_ENDUADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBufferEnd(ulParameter[0], MMP_RAW_STORE_PLANE1, i, ulParameter[i+2]);
            }
            break;
        case SET_RAWSTORE_ENDVADDR:
            for (i = 0; i < (MMP_UBYTE)(ulParameter[1] & 0x000000FF); i++) {
                MMPF_RAWPROC_SetRawStoreBufferEnd(ulParameter[0], MMP_RAW_STORE_PLANE2, i, ulParameter[i+2]);
            }
            break;
        }
        break;
     case HIF_FCTL_CMD_SET_PIPE_LINKED_SNR:
     	gbPipeLinkedSnr[(MMP_UBYTE)ulParameter[0]] = (MMP_UBYTE)ulParameter[1];
    	break;
    }

    MMPF_OS_SetFlags(DSC_UI_Flag, SYS_FLAG_SENSOR_CMD_DONE, MMPF_OS_FLAG_SET);
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Sensor_Task
//  Description : Sensor main function
//------------------------------------------------------------------------------
void MMPF_Sensor_Task(void)
{
    MMP_UBYTE       i;
    MMPF_OS_FLAGS   flags;
    MMPF_OS_FLAGS   waitFlags;

    RTNA_DBG_Str3("Sensor_Task()\r\n");

    // Open VIF interrupt
    MMPF_VIF_OpenInterrupt(MMP_TRUE);
	// Open RAW interrupt
    MMPF_RAWPROC_OpenInterrupt(MMP_TRUE);
    // Open ISP interrupt
    MMPF_ISP_OpenInterrupt(MMP_TRUE);
    // Open SCAL interrupt
    MMPF_Scaler_OpenInterrupt(MMP_TRUE);
    // Open IBC interrupt
    MMPF_IBC_Init();

    // Initialize os semaphore
    for (i = 0; i < VIF_SENSOR_MAX_NUM; i++) {	

        m_StopPreviewCtrlSem[i] = MMPF_OS_CreateSem(0); 
        if (m_StopPreviewCtrlSem[i] == MMPF_OS_CREATE_SEM_INT_ERR) {
        	RTNA_DBG_Str(0, "m_StopPreviewCtrlSem Failed XXX\r\n");
        }

        m_StartPreviewFrameEndSem[i] = MMPF_OS_CreateSem(0); 
        if (m_StartPreviewFrameEndSem[i] == MMPF_OS_CREATE_SEM_INT_ERR) {
        	RTNA_DBG_Str(0, "m_StartPreviewFrameEndSem Failed XXX\r\n");
    	}
	}
	
	m_ISPOperationDoneSem = MMPF_OS_CreateSem(0); 
    if (m_ISPOperationDoneSem == MMPF_OS_CREATE_SEM_INT_ERR) {
    	RTNA_DBG_Str(0, "m_ISPOperationDoneSem Failed XXX\r\n");
	}
	
	#if (SUPPORT_ISP_TIMESHARING)
	m_IspFrmEndSem = MMPF_OS_CreateSem(1); 
    if (m_IspFrmEndSem == MMPF_OS_CREATE_SEM_INT_ERR) {
    	RTNA_DBG_Str(0, "m_IspFrmEndSem Failed XXX\r\n");
	}
	#endif

    for (i = 0; i < MMP_IBC_PIPE_MAX; i++) {

        m_IbcFrmRdySem[i] = MMPF_OS_CreateSem(0); 
        if (m_IbcFrmRdySem[i] == MMPF_OS_CREATE_SEM_INT_ERR) {
        	RTNA_DBG_Str(0, "m_IbcFrmRdySem Failed XXX\r\n");
        }
	}

    waitFlags = SENSOR_FLAG_SENSOR_CMD | SENSOR_FLAG_CHANGE_SENSOR_MODE;
    waitFlags |= SENSOR_FLAG_ROTDMA;
    waitFlags |= SENSOR_FLAG_ROTDMA_REAR;

    #if (ISP_USE_TASK_DO3A == 1)
    waitFlags |= (SENSOR_FLAG_ISP_FRM_END | SENSOR_FLAG_ISP_FRM_ST | SENSOR_FLAG_VIF_FRM_ST);
    #endif
    #if (SUPPORT_UVC_FUNC)
    waitFlags |= SENSOR_FLAG_PCAMOP;
    #endif
    #if (TVDEC_SNR_USE_DMA_DEINTERLACE)
    waitFlags |= SENSOR_FLAG_TRIGGER_DEINTERLACE;
    #endif
    waitFlags |= SENSOR_FLAG_TRIGGER_GRA_TO_PIPE;

    while (TRUE) {
    
        MMPF_OS_WaitFlags(SENSOR_Flag, waitFlags, MMPF_OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, 0, &flags);

        if (flags & SENSOR_FLAG_SENSOR_CMD) {
            MMPF_Sensor_ProcessCmd();
        }

        if (flags & SENSOR_FLAG_ROTDMA) {
        }

        if (flags & SENSOR_FLAG_ROTDMA_REAR) {
        }

        if (flags & SENSOR_FLAG_CHANGE_SENSOR_MODE) {
			gsSensorFunction->MMPF_Sensor_ChangeMode(gubChangeModeSensorId, gusSensorChangeModeIdx, 0);
        	m_ulISPFrameCount = 0;
        }

        #if (ISP_USE_TASK_DO3A == 1)
        // For raw path and real time path. Sensor shutter/Gain should change from ISP frame start to VIF frame start.
        if (flags & SENSOR_FLAG_VIF_FRM_ST) {
            gsSensorFunction->MMPF_Sensor_DoAEOperation_ST(m_ubDo3ASensorId);
        }
        if (flags & SENSOR_FLAG_ISP_FRM_END) {
            gsSensorFunction->MMPF_Sensor_DoAFOperation(m_ubDo3ASensorId);
			#if (ISP_EN)
			ISP_IF_AWB_GetHWAcc(1);
			#endif
        }
        if (flags & SENSOR_FLAG_ISP_FRM_ST) {
	        gsSensorFunction->MMPF_Sensor_DoAWBOperation(m_ubDo3ASensorId);
            gsSensorFunction->MMPF_Sensor_DoIQOperation(m_ubDo3ASensorId);
        }
        #endif

        if (flags & SENSOR_FLAG_PCAMOP) {
        }

        /* For TV signal DeInterlace */
        #if (TVDEC_SNR_USE_DMA_DEINTERLACE)
        if (flags & SENSOR_FLAG_TRIGGER_DEINTERLACE) {
            MMPF_RAWPROC_DmaDeInterlace(m_ubYUVRawId);
        }
        #endif
        if (flags & SENSOR_FLAG_TRIGGER_GRA_TO_PIPE) {
            MMPF_RAWPROC_TriggerGraToPipe(m_ubYUVRawId);
        }
    }
}

#endif
/// @}
