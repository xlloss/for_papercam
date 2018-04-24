//==============================================================================
//
//  File        : mmpf_dsc.c
//  Description : JPEG DSC function
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

#if (DSC_R_EN)

#include "math.h"
#include "ait_utility.h"
#include "isp_if.h"
#include "ptz_cfg.h"
#include "hdr_cfg.h"
#include "mmp_reg_ibc.h"
#include "mmp_reg_jpeg.h"
#include "mmp_reg_rawproc.h"
#include "mmp_reg_graphics.h"
#include "mmp_reg_scaler.h"
#include "mmp_dsc_inc.h"
#include "mmp_ldc_inc.h"

#include "mmpf_bayerscaler.h"
#include "mmpf_dma.h"
#include "mmpf_dsc.h"
#include "mmpf_hif.h"
#include "mmpf_vif.h"
#include "mmpf_rawproc.h"
#include "mmpf_scaler.h"
#include "mmpf_icon.h"
#include "mmpf_ibc.h"
#include "mmpf_ptz.h"
#include "mmpf_system.h"
#include "mmpf_sensor.h"

/** @addtogroup MMPF_DSC
@{
*/

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define DSC_DBG_LEVEL (0)

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

#if (DSC_R_EN)
static MMP_USHORT  				m_usJpegEncW 			= 1920;
static MMP_USHORT  				m_usJpegEncH 			= 1080;
  
/* JPEG size control information */
static MMP_USHORT   			m_usMaxScaleQFactor[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_USHORT   		    m_usMinScaleQFactor[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_USHORT				m_usHQScaleFactor[MMP_DSC_JPEG_RC_ID_NUM]; 
static MMP_USHORT               m_usLQScaleFactor[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_USHORT				m_usHQJpegSizeInKB[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_USHORT				m_usLQJpegSizeInKB[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_USHORT				m_usCurScaleFactor[MMP_DSC_JPEG_RC_ID_NUM] = {64, 64, 64};  ///< when Q-factor is 64, Q-factor will not change the Q-table of JPEG.

/* Below size control variable unit is KBytes */
static MMP_USHORT				m_usTargetLow[MMP_DSC_JPEG_RC_ID_NUM]; 
static MMP_USHORT				m_usTargetHigh[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_ULONG				m_ulLimitJpegSize[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_ULONG				m_ulTargetJpegSize[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_UBYTE				m_bLimitJpegSizeEn[MMP_DSC_JPEG_RC_ID_NUM] = {MMP_FALSE, MMP_FALSE, MMP_FALSE};

/**	@brief	When encoding JPEG is at size control mode, 
			m_bScaleOK is true means the size of compressed data is under control.
			@ref MMPF_DSC_DoJpegScale().
*/
static MMP_BOOL					m_bScaleOK[MMP_DSC_JPEG_RC_ID_NUM] = {MMP_FALSE, MMP_FALSE, MMP_FALSE};

/**	@brief	When encoding JPEG is at size control mode, 
			m_bChangeQ is true means changing Qtable_Low already.
			@ref MMPF_DSC_DoJpegScale().
*/
static MMP_BOOL					m_bChangeQ[MMP_DSC_JPEG_RC_ID_NUM] = {MMP_FALSE, MMP_FALSE, MMP_FALSE};

/** @brief	m_usJpegScaleCurCount is the cur. number of retry encoding JPEG 
			m_usJpegScaleMaxCount is the max. number of retry encoding JPEG
*/
static MMP_USHORT				m_usJpegScaleCurCount[MMP_DSC_JPEG_RC_ID_NUM] = {0, 0, 0};
static MMP_USHORT				m_usJpegScaleMaxCount[MMP_DSC_JPEG_RC_ID_NUM] = {3, 1, 1};

static MMPF_DSC_SIZE_CTRL_MODE	m_JpegSizeCtlMode[MMP_DSC_JPEG_RC_ID_NUM];

/* The size of JPEG after encoding JPEG is done.(KB) */
volatile static MMP_USHORT	    m_usCurJpegSizeInKB[MMP_DSC_JPEG_RC_ID_NUM] = {0, 0, 0};
/* The size of JPEG after encoding JPEG is done.(Byte) */
volatile static MMP_ULONG	    m_ulEncodedJpegSize = 0;
volatile static MMP_ULONG       m_ulEncodedJpegTime = 0;

/* JPEG image path settings */
static MMP_SCAL_PIPEID   		m_CapturePathScaler	= MMP_SCAL_PIPE_0;
static MMP_ICO_PIPEID    		m_CapturePathIcon  	= MMP_ICO_PIPE_0;
static MMP_IBC_PIPEID    		m_CapturePathIBC    = MMP_IBC_PIPE_0;

static MMP_UBYTE 				QtableY[MMP_DSC_JPEG_RC_ID_NUM][64];
static MMP_UBYTE 				QtableU[MMP_DSC_JPEG_RC_ID_NUM][64];
static MMP_UBYTE 				QtableV[MMP_DSC_JPEG_RC_ID_NUM][64];
static MMP_DSC_JPEG_QTABLE 		gQTSet[MMP_DSC_JPEG_RC_ID_NUM];
static MMP_DSC_JPEG_QTABLE 		gQTSetCur = MMP_DSC_JPEG_QT_NUM;

/** @brief	After MMPF_DSC_DoJpegScale() release m_JpgEncDoneSem
			MMPF_DSC_EncodeJpeg acquire m_JpgEncDoneSem
*/
MMPF_OS_SEMID 			        m_JpgEncDoneSem;

const static MMP_UBYTE 			Qtable_Low[128] = 
{
    //low quality
    0x0b,0x0c,0x0c,0x0c,0x0c,0x0c,0x10,0x10,
    0x10,0x10,0x14,0x10,0x0c,0x0c,0x14,0x1a,
    0x14,0x10,0x10,0x14,0x1a,0x1c,0x14,0x14,
    0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
    0x24,0x24,0x24,0x24,0x24,0x24,0x24,0x24,
    0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
    0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
    0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,

    0x0c,0x12,0x12,0x1a,0x1a,0x1c,0x24,0x24,
    0x24,0x24,0x2c,0x24,0x24,0x24,0x2c,0x2c,
    0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,
    0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
    0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
};


static MMPF_JPEG_ENCDONE_MODE 	m_JpegEncDoneMode 	= MMPF_DSC_ENCDONE_NONE;

MMP_BOOL                        m_bDoCaptureFromRawPath = MMP_FALSE;

static MMPF_DSC_IMAGE_SOURCE    m_JpegSourcePath    = MMPF_DSC_SRC_SENSOR;
static MMPF_DSC_CAPTUREMODE     m_JpegShotMode      = MMPF_DSC_SHOTMODE_SINGLE;

MMP_DSC_CAPTURE_BUF             m_sDscCaptureBuf;

/* For Encode Callback Function */
JpegEncCallBackFunc 	        *CallBackFuncJpegEnc[MMP_DSC_ENC_EVENT_MAX];
void            	            *CallBackArguJpegEnc[MMP_DSC_ENC_EVENT_MAX];

#endif

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMPF_OS_SEMID            m_ISPOperationDoneSem;

extern volatile MMP_UBYTE		gbExposureDoneBufIdx[MMP_IBC_PIPE_MAX];

extern MMP_BOOL   		        gbFrameExposureDone;
extern MMP_BOOL	    			gbPipeIsActive[];

extern RC_CONFIG_PARAM          gRcConfig;

//==============================================================================
//
//                              FUNCTIONS PROTOTYPE
//
//==============================================================================

static MMP_ERR MMPF_DSC_DoJpegScale(MMP_UBYTE ubRcIdx);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void _____Internal_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_JPG_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_JPG_ISR(void) ITCMFUNC;
void MMPF_JPG_ISR(void)
{
    AITPS_JPG   pJPG = AITC_BASE_JPG;
    MMP_USHORT  intsrc;

    intsrc = pJPG->JPG_INT_CPU_SR & pJPG->JPG_INT_CPU_EN;
    pJPG->JPG_INT_CPU_SR = intsrc;
 
    /* Encode Interrupt Event */
    if (intsrc & JPG_INT_ENC_DONE) 
    {
        if (CallBackFuncJpegEnc[MMP_DSC_ENC_EVENT_ENC_DONE]) {
            CallBackFuncJpegEnc[MMP_DSC_ENC_EVENT_ENC_DONE](CallBackArguJpegEnc[MMP_DSC_ENC_EVENT_ENC_DONE]);
        }

        if (m_JpegEncDoneMode == MMPF_DSC_ENCDONE_SIZE_CTL) {
            m_usCurJpegSizeInKB[MMP_DSC_JPEG_RC_ID_CAPTURE] = (pJPG->JPG_ENC_FRAME_SIZE >> 10) + 1;

            if (m_JpegSizeCtlMode[MMP_DSC_JPEG_RC_ID_CAPTURE] != MMPF_DSC_SIZE_CTL_DISABLE) {
                MMPF_DSC_DoJpegScale(MMP_DSC_JPEG_RC_ID_CAPTURE);
            }
            MMPF_OS_ReleaseSem(m_JpgEncDoneSem);
    	}
        else {
            MMPF_OS_ReleaseSem(m_JpgEncDoneSem);
        }
    }

    if (intsrc & JPG_INT_LINE_BUF_OVERLAP) {
        MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "Jpeg Encode Line Buf OverLap\r\n");
        if (CallBackFuncJpegEnc[MMP_DSC_ENC_EVENT_LINEBUF_OVERLAP]) {
            CallBackFuncJpegEnc[MMP_DSC_ENC_EVENT_LINEBUF_OVERLAP](CallBackArguJpegEnc[MMP_DSC_ENC_EVENT_LINEBUF_OVERLAP]);
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JPG_SetEncDoneMode
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_JPG_SetEncDoneMode(MMPF_JPEG_ENCDONE_MODE ubMode)
{
	m_JpegEncDoneMode = ubMode;
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JPG_TriggerEncode
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_JPG_TriggerEncode(void)
{
    AITPS_JPG pJPG = AITC_BASE_JPG;

    m_ulEncodedJpegTime = OSTime;
	pJPG->JPG_CTL = JPG_ENC_EN | JPG_ENC_MARKER_EN | JPG_ENC_FMT_YUV422;
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_PollingReg
//  Description : Polling the status of register by func not while loop.
//	              ulTimeOutMs : the limit is 4,294,967,296 / 12000
//------------------------------------------------------------------------------
static MMP_BOOL MMPF_DSC_PollingReg(PFuncRegCheck pFunc, MMP_ULONG ulTimeOutMs)
{
    // Start time in ms    
    MMP_ULONG	ulTimeStart;
    MMP_ULONG	ulCurTime;
    MMP_BOOL	bResult = MMP_TRUE;

	MMPF_OS_GetTime(&ulTimeStart);
    
    while (pFunc() == MMP_FALSE)
    {
        MMPF_OS_Sleep(2);
        
        MMPF_OS_GetTime(&ulCurTime);
         
        if (ulCurTime - ulTimeStart > ulTimeOutMs)
        {
            bResult = MMP_FALSE;
            break;
        }
    }
	
    return bResult;
} 

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_CheckEncodeDone
//  Description : Check the encode done of jpeg.
//------------------------------------------------------------------------------
static MMP_BOOL MMPF_DSC_CheckEncodeDone(void)
{
	AITPS_JPG pJPG = AITC_BASE_JPG;
	return (pJPG->JPG_CTL & JPG_ENC_EN) ? (MMP_FALSE) : (MMP_TRUE);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_CheckEncodeStart
//  Description : Check the encode start of jpeg.
//------------------------------------------------------------------------------
static MMP_BOOL MMPF_DSC_CheckEncodeStart(void)
{
	AITPS_JPG pJPG = AITC_BASE_JPG;
	return (pJPG->JPG_ENC_CNT > 0 ) ? (MMP_TRUE) : (MMP_FALSE);
}

#if 0
void _____RateControl_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetQFactor
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetQFactor(MMP_USHORT qCtl1, MMP_USHORT qCtl2)
{
    AITPS_JPG pJPG = AITC_BASE_JPG;

    pJPG->JPG_QLTY_CTL_FACTOR1 = qCtl1;
    pJPG->JPG_QLTY_CTL_FACTOR2 = qCtl2;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_InitialJpegScale
//  Description : This function initialize quality array parameter.
//                They are use in calculate quality factor
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_InitialJpegScale(MMP_UBYTE ubRcIdx)
{
	m_bScaleOK[ubRcIdx] = MMP_FALSE;
	m_bChangeQ[ubRcIdx] = MMP_FALSE;

	m_usJpegScaleCurCount[ubRcIdx] = 0;

	m_usHQScaleFactor[ubRcIdx]    = JPEG_SCALE_FACTOR_HQ;
	m_usLQScaleFactor[ubRcIdx]    = JPEG_SCALE_FACTOR_LQ;
	m_usCurScaleFactor[ubRcIdx]   = JPEG_SCALE_FACTOR_CUR;
    m_usMaxScaleQFactor[ubRcIdx]  = JPEG_SCALE_FACTOR_MAX;
    m_usMinScaleQFactor[ubRcIdx]  = JPEG_SCALE_FACTOR_MIN;
	
	MMPF_DSC_SetQFactor(m_usCurScaleFactor[ubRcIdx], m_usCurScaleFactor[ubRcIdx]);
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetJpegQualityCtl
//  Description : This function calculates the rate control target size and 
//                limite size
//------------------------------------------------------------------------------
/** @brief
@param[in] usTargetCtlEnable 	the flag of target control
@param[in] usLimitCtlEnable 	the flag of limit control
@param[in] usTargetsize 		the target size for the Jpeg encode
@param[in] usLimitSize 			the limit size for the Jpeg encode
@param[in] usMaxCount 			the maximum count for the Jpeg encode trial
@return It reports the status of the operation.
*/
MMP_ERR	MMPF_DSC_SetJpegQualityCtl( MMP_UBYTE   ubRcIdx,
                                    MMP_BOOL	bTargetCtlEnable,
                                    MMP_BOOL 	bLimitCtlEnable,
			                        MMP_ULONG 	ulTargetSize, 
			                        MMP_ULONG 	ulLimitSize,
			                        MMP_USHORT 	usMaxCount)
{
	if (bTargetCtlEnable || bLimitCtlEnable) {
        m_JpegSizeCtlMode[ubRcIdx] = MMPF_DSC_SIZE_CTL_NORMAL;
	}
	else {
		m_JpegSizeCtlMode[ubRcIdx] = MMPF_DSC_SIZE_CTL_DISABLE;
    }

	m_bLimitJpegSizeEn[ubRcIdx] = MMP_FALSE;
	
	// Limit size mode: target size depends on ulLimitSize, and limit control enable
	if (bLimitCtlEnable) {  
		m_ulLimitJpegSize[ubRcIdx]     = ulLimitSize * 95 / 100; // For the same quality factor compresses different size, it protect the jpeg size larger then limit size
		m_bLimitJpegSizeEn[ubRcIdx]    = MMP_TRUE;
		m_usTargetLow[ubRcIdx]         = 0;
		m_usTargetHigh[ubRcIdx]        = m_ulLimitJpegSize[ubRcIdx];
		m_ulTargetJpegSize[ubRcIdx]    = m_ulLimitJpegSize[ubRcIdx] * 8 / 10;
	}
	
	// Target size mode: target size depends on usTargetsize, limit control depends on usLimitCtlEnable
	// When the target control and limit control enable at the same time, the target size depends on usTargetsize
	if (bTargetCtlEnable) {  
		m_usTargetLow[ubRcIdx]         = ulTargetSize * 9 / 10;
		m_usTargetHigh[ubRcIdx]        = ulTargetSize * 11 / 10;
		m_ulTargetJpegSize[ubRcIdx]    = ulTargetSize;
	}
	
	m_usJpegScaleCurCount[ubRcIdx] = 0;
	m_usJpegScaleMaxCount[ubRcIdx] = usMaxCount;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetQTableLow
//  Description : Just in case, we set the lower Q-table when we can not encode the limited size we want. 
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetQTableLow(void)
{
    MMP_ULONG   i;

    // Q table can only be accessed in BYTE
	for (i = 0; i < 64; i++) {
        *(AITC_BASE_TBL_Q + i)          = Qtable_Low[i];
        *(AITC_BASE_TBL_Q + i + 64)     = Qtable_Low[i + 64];
        *(AITC_BASE_TBL_Q + i + 128)    = Qtable_Low[i + 64];
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetQTableIntoOpr
//  Description : With some case, we need to set the Q-table with F/W flow. 
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetQTableIntoOpr(MMP_UBYTE ubIdx)
{
    MMP_ULONG   i;
    
	if ((gQTSetCur != gQTSet[ubIdx]) || (gQTSet[ubIdx] == MMP_DSC_JPEG_QT_CUSTOMER)) {
	    // Q table can only be accessed in BYTE
		for (i = 0; i < 64; i++) {
			*(AITC_BASE_TBL_Q + i) 			= QtableY[ubIdx][i];
			*(AITC_BASE_TBL_Q + i + 64) 	= QtableU[ubIdx][i];
			*(AITC_BASE_TBL_Q + i + 128)	= QtableV[ubIdx][i];
		}
		gQTSetCur = gQTSet[ubIdx];
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetQTableInfo
//  Description : With some case, we need to set the Q-table with F/W flow. 
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetQTableInfo(MMP_UBYTE ubIdx, MMP_UBYTE *ubQtable, MMP_UBYTE *ubUQtable, MMP_UBYTE *ubVQtable, MMP_DSC_JPEG_QTABLE QTSet)
{
    MMP_ULONG   i;

	gQTSet[ubIdx] = QTSet;

    // Q table can only be accessed in BYTE
	for (i = 0; i < 64; i++) {
		QtableY[ubIdx][i] 	= *(ubQtable++);
		QtableU[ubIdx][i] 	= *(ubUQtable++);
		QtableV[ubIdx][i] 	= *(ubVQtable++);
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_JpegRateControl
//  Description : This function calculates a new quality factor that is used in 
//                jpeg encode. When the value of m_usCurScaleFactor become smaller,
//                the jpeg size become bigger and the quality better
//------------------------------------------------------------------------------
static MMP_ERR MMPF_DSC_JpegRateControl(MMP_UBYTE ubRcIdx)
{
	MMP_USHORT usNewScaleFactor = 0;

	if (m_usCurJpegSizeInKB[ubRcIdx] >= m_ulTargetJpegSize[ubRcIdx]) // Increase the Q-factor to reduce the size
	{
		if (m_usCurScaleFactor[ubRcIdx] < m_usMaxScaleQFactor[ubRcIdx]) {
			
			m_usHQScaleFactor[ubRcIdx]  = m_usCurScaleFactor[ubRcIdx];
			m_usHQJpegSizeInKB[ubRcIdx] = m_usCurJpegSizeInKB[ubRcIdx];
		
			// Patch : make sure that the setting is right. 
			if (m_usLQScaleFactor[ubRcIdx] <= m_usHQScaleFactor[ubRcIdx]) {
				m_usLQScaleFactor[ubRcIdx] = ((m_usHQScaleFactor[ubRcIdx] + 2) >= m_usMaxScaleQFactor[ubRcIdx]) ? (m_usMaxScaleQFactor[ubRcIdx]) : (m_usHQScaleFactor[ubRcIdx] + 2);
			}
            if (m_usLQJpegSizeInKB[ubRcIdx] >= m_usHQJpegSizeInKB[ubRcIdx]) {
                m_usLQJpegSizeInKB[ubRcIdx] = ((m_usHQJpegSizeInKB[ubRcIdx] - 5) >= 1) ? (m_usHQJpegSizeInKB[ubRcIdx] - 5) : (1);
			}

			usNewScaleFactor = 	m_usHQScaleFactor[ubRcIdx]
								+ (m_usHQJpegSizeInKB[ubRcIdx] - m_ulTargetJpegSize[ubRcIdx])
								* (m_usLQScaleFactor[ubRcIdx] - m_usHQScaleFactor[ubRcIdx]) / (m_usHQJpegSizeInKB[ubRcIdx] - m_usLQJpegSizeInKB[ubRcIdx]);

			if (m_bScaleOK[ubRcIdx])
				usNewScaleFactor = m_usHQScaleFactor[ubRcIdx] + 1; //EROY CHECK
			if (usNewScaleFactor == m_usCurScaleFactor[ubRcIdx])
				usNewScaleFactor += 1;
		}
		else {
			usNewScaleFactor = m_usMaxScaleQFactor[ubRcIdx];
		}
	}
	else // Decrease the Q-factor to increase the size
	{
		if (m_usCurScaleFactor[ubRcIdx] > m_usMinScaleQFactor[ubRcIdx]) 
		{
			m_usLQScaleFactor[ubRcIdx]   = m_usCurScaleFactor[ubRcIdx];
			m_usLQJpegSizeInKB[ubRcIdx]  = m_usCurJpegSizeInKB[ubRcIdx];

			// Patch : make sure that the setting is right. 
			if (m_usLQScaleFactor[ubRcIdx] <= m_usHQScaleFactor[ubRcIdx]) {
				m_usHQScaleFactor[ubRcIdx] = ((m_usLQScaleFactor[ubRcIdx] - 2) >= m_usMinScaleQFactor[ubRcIdx]) ? (m_usLQScaleFactor[ubRcIdx] - 2) : (m_usMinScaleQFactor[ubRcIdx]);		
			}
			if (m_usLQJpegSizeInKB[ubRcIdx] >= m_usHQJpegSizeInKB[ubRcIdx]) {
				m_usHQJpegSizeInKB[ubRcIdx] = m_usLQJpegSizeInKB[ubRcIdx] + 5;
			}
			
			if (m_usHQJpegSizeInKB[ubRcIdx] >= m_ulTargetJpegSize[ubRcIdx])
			{
				usNewScaleFactor = m_usHQScaleFactor[ubRcIdx]
								+ (m_usHQJpegSizeInKB[ubRcIdx] - m_ulTargetJpegSize[ubRcIdx])
								* (m_usLQScaleFactor[ubRcIdx] - m_usHQScaleFactor[ubRcIdx]) / (2*(m_usHQJpegSizeInKB[ubRcIdx] - m_usLQJpegSizeInKB[ubRcIdx]));
			}
			else
			{
				usNewScaleFactor = m_usHQScaleFactor[ubRcIdx]
                                + (m_ulTargetJpegSize[ubRcIdx] - m_usHQJpegSizeInKB[ubRcIdx])
                                * (m_usLQScaleFactor[ubRcIdx] - m_usHQScaleFactor[ubRcIdx]) / (2*(m_usHQJpegSizeInKB[ubRcIdx] - m_usLQJpegSizeInKB[ubRcIdx]));
			}

			if (m_bScaleOK[ubRcIdx])
				usNewScaleFactor = m_usLQScaleFactor[ubRcIdx] - 1;
			if (usNewScaleFactor == m_usCurScaleFactor[ubRcIdx])
				usNewScaleFactor -= 1;
		}
		else {
			usNewScaleFactor = m_usMinScaleQFactor[ubRcIdx];
		}
	}

	if (usNewScaleFactor > m_usMaxScaleQFactor[ubRcIdx])
		m_usCurScaleFactor[ubRcIdx] = m_usMaxScaleQFactor[ubRcIdx];
	else if (usNewScaleFactor < m_usMinScaleQFactor[ubRcIdx])
		m_usCurScaleFactor[ubRcIdx] = m_usMinScaleQFactor[ubRcIdx];
	else
		m_usCurScaleFactor[ubRcIdx] = usNewScaleFactor;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_DoJpegScale
//  Description : This function set a new quality factor to encode jpeg and check
//                jpeg size. If the jpeg size is in the target range, rate control
//                finish.
//------------------------------------------------------------------------------
static MMP_ERR MMPF_DSC_DoJpegScale(MMP_UBYTE ubRcIdx)
{
    AITPS_JPG   pJPG = AITC_BASE_JPG;
    MMP_BOOL 	bTmp = MMP_FALSE;

	if (m_usCurJpegSizeInKB[ubRcIdx] != 1 && m_JpegSizeCtlMode[ubRcIdx] != MMPF_DSC_SIZE_CTL_DISABLE) 
	{
		if ((m_JpegEncDoneMode == MMPF_DSC_ENCDONE_SIZE_CTL) || 
            (m_JpegEncDoneMode == MMPF_DSC_ENCDONE_CONTINUOUS_SHOT)) {
			m_usJpegScaleCurCount[ubRcIdx]++;
		}
		else {
		    /* For MJPEG relative application, don't increase the m_usJpegScaleCurCount */
			m_usJpegScaleCurCount[ubRcIdx] = 0;
		}
		
		if (m_usJpegScaleCurCount[ubRcIdx] >= m_usJpegScaleMaxCount[ubRcIdx] && 
		    m_usJpegScaleMaxCount[ubRcIdx] != 0xFFFF) 
		{
			if ((m_usCurJpegSizeInKB[ubRcIdx] < m_ulLimitJpegSize[ubRcIdx])||
			    (m_bChangeQ[ubRcIdx])||(!m_bLimitJpegSizeEn[ubRcIdx])) {
	  			m_bScaleOK[ubRcIdx] = MMP_TRUE;
     		}
    		else {
				if (m_usCurScaleFactor[ubRcIdx] == m_usMaxScaleQFactor[ubRcIdx]) {
					MMPF_DSC_SetQTableLow();
					m_bChangeQ[ubRcIdx] = MMP_TRUE;
	   				m_bScaleOK[ubRcIdx] = MMP_FALSE;
				}
				else {
	   				m_bScaleOK[ubRcIdx] = MMP_FALSE;
	  				if (m_usCurScaleFactor[ubRcIdx] <= (m_usMaxScaleQFactor[ubRcIdx] - 16)) {
			  			m_usCurScaleFactor[ubRcIdx] += 16;
		  			}
		  			else {
		  				m_usCurScaleFactor[ubRcIdx] = m_usMaxScaleQFactor[ubRcIdx];
		  			}
		  		}
	  		}
		}
		else 
		{
			/* To ensure the Q-factor register is the same as the previous QF variable, 
			 * Or it means the new QF is not updated into register. 
			 */
            bTmp = (pJPG->JPG_QLTY_CTL_FACTOR1 == m_usCurScaleFactor[ubRcIdx]); 
    
            if (bTmp)
			{
				if ((m_usCurJpegSizeInKB[ubRcIdx] >= m_usTargetLow[ubRcIdx]) && 
				    (m_usCurJpegSizeInKB[ubRcIdx] <= m_usTargetHigh[ubRcIdx]))
					m_bScaleOK[ubRcIdx] = MMP_TRUE;
				else
					m_bScaleOK[ubRcIdx] = MMP_FALSE;

				if (!m_bScaleOK[ubRcIdx]) {
					MMPF_DSC_JpegRateControl(ubRcIdx);

					if (m_usCurScaleFactor[ubRcIdx] >= m_usMaxScaleQFactor[ubRcIdx]) {
						if (m_JpegEncDoneMode == MMPF_DSC_ENCDONE_SIZE_CTL) {
							RTNA_DBG_Str(3, "Last Compression cycle\r\n");
							m_usJpegScaleCurCount[ubRcIdx] = m_usJpegScaleMaxCount[ubRcIdx] - 1;
						}
						m_usCurScaleFactor[ubRcIdx] = m_usMaxScaleQFactor[ubRcIdx];
					}
				}
			}
		}

		if (!m_bScaleOK[ubRcIdx]) {
		    if (ubRcIdx == MMP_DSC_JPEG_RC_ID_CAPTURE) {
        	    pJPG->JPG_QLTY_CTL_FACTOR1 = m_usCurScaleFactor[ubRcIdx];
        	    pJPG->JPG_QLTY_CTL_FACTOR2 = m_usCurScaleFactor[ubRcIdx];
		    }
		    #if (HANDLE_JPEG_EVENT_BY_QUEUE)
		    else if (ubRcIdx == MMP_DSC_JPEG_RC_ID_MJPEG_1ST_STREAM) {
		        gusFrontCamJpgScaleFactor = m_usCurScaleFactor[ubRcIdx];
		    }
		    else if (ubRcIdx == MMP_DSC_JPEG_RC_ID_MJPEG_2ND_STREAM) {
		        gusRearCamJpgScaleFactor = m_usCurScaleFactor[ubRcIdx];
		    }
		    #endif
		}
	}

	return MMP_ERR_NONE;
}

#if 0
void _____Capture_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_RegisterEncIntrCallBack
//  Description : This function register interrupt callback function.
//------------------------------------------------------------------------------
/** 
 * @brief This function register interrupt callback function.
 * 
 *  This function register interrupt callback function.
 * @param[in] event     : stands for interrupt event.
 * @param[in] pCallBack : stands for interrupt callback function. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_DSC_RegisterEncIntrCallBack(MMP_DSC_ENC_EVENT event, JpegEncCallBackFunc *pCallBack, void *pArgument)
{
    CallBackFuncJpegEnc[event] = pCallBack;
    CallBackArguJpegEnc[event] = pArgument;
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetCapturePath
//  Description : Set the pipe for DSC capture
//------------------------------------------------------------------------------
/** @brief Set the Image Path for DSC capture
@param[in] usScaPipe    capture scaler pipe
@param[in] usIcoPipe    catpure icon pipe
@param[in] usIBCPipe    capture use IBC Pipe
@return It reports the status of the operation.
*/
MMP_ERR MMPF_DSC_SetCapturePath(MMP_USHORT	usScaPipe,
                                MMP_USHORT	usIcoPipe,
                                MMP_USHORT	usIbcPipe)
{
    m_CapturePathScaler = usScaPipe;
    m_CapturePathIcon   = usIcoPipe;
    m_CapturePathIBC    = usIbcPipe;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetCaptureBuffers
//  Description :
//------------------------------------------------------------------------------
/** @brief Configure the buffer for capture operation

The function sets the buffer address for compress buffer start, compress buffer end and line buffer
@param[in] pBuf The buffer information for firmware to store compressed JPEG bitstream 
                and to set "Line Buffer" for incoming image.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_DSC_SetCaptureBuffers(MMP_DSC_CAPTURE_BUF *pBuf)
{
    MEMCPY(&m_sDscCaptureBuf, pBuf, sizeof(MMP_DSC_CAPTURE_BUF));

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetLineBufType
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetLineBufType(void)
{
    AITPS_JPG   pJPG = AITC_BASE_JPG;
    AITPS_IBC   pIBC = AITC_BASE_IBC;

    if (0/*pJPG->JPG_ENC_W > DSC_SINGLE_LINE_BUF_TH*/) {
    	pJPG->JPG_ENC_LINEBUF_CTL = JPG_SINGLE_LINE_BUF;
    		
    	if (m_CapturePathIBC == MMP_IBC_PIPE_0) {
        	pIBC->IBC_MCI_CFG 		 |= IBC_P0_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_0.IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    	}
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_1) {
        	pIBC->IBC_MCI_CFG 		 |= IBC_P1_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_1.IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    	}
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_2) {
        	pIBC->IBC_MCI_CFG 		 |= IBC_P2_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_2.IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    	}    	
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_3) {
        	pIBC->IBC_MCI_CFG 		 |= IBC_P3_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_3.IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    	}
    }
    else {
    	pJPG->JPG_ENC_LINEBUF_CTL = JPG_DOUBLE_LINE_BUF_1ADDR;
    	
    	if (m_CapturePathIBC == MMP_IBC_PIPE_0) {
    		pIBC->IBC_MCI_CFG 		 |= IBC_P0_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_0.IBC_BUF_CFG |= (IBC_STORE_PIX_CONT);
    	}
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_1) {
    		pIBC->IBC_MCI_CFG 		 |= IBC_P1_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_1.IBC_BUF_CFG |= (IBC_STORE_PIX_CONT);
    	}
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_2) {
    		pIBC->IBC_MCI_CFG 		 |= IBC_P2_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_2.IBC_BUF_CFG |= (IBC_STORE_PIX_CONT);
    	}
    	else if (m_CapturePathIBC == MMP_IBC_PIPE_3) {
    		pIBC->IBC_MCI_CFG 		 |= IBC_P3_MCI_256BYTE_CNT_EN;
        	pIBC->IBCP_3.IBC_BUF_CFG |= (IBC_STORE_PIX_CONT);
    	}
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_SetJpegResol
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_SetJpegResol(MMP_USHORT usJpegWidth, MMP_USHORT usJpegHeight, MMP_UBYTE ubRcIdx)
{
    AITPS_JPG pJPG = AITC_BASE_JPG;
    
    m_usJpegEncW = usJpegWidth;
    m_usJpegEncH = usJpegHeight;

    pJPG->JPG_ENC_W = usJpegWidth;
    pJPG->JPG_ENC_H = usJpegHeight;

	// Calculates jpeg high, low quality size for rate control
	m_usHQJpegSizeInKB[ubRcIdx] = (usJpegWidth * usJpegHeight * 3)>>12;  // w*h*0.75/1024
	m_usLQJpegSizeInKB[ubRcIdx] = (usJpegWidth * usJpegHeight)>>14;      // w*h*0.0625/1024

	MMPF_DSC_InitialJpegScale(ubRcIdx);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_EncodeJpeg
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR	MMPF_DSC_EncodeJpeg(MMP_UBYTE ubSnrSel, MMPF_DSC_IMAGE_SOURCE SourcePath, MMPF_DSC_CAPTUREMODE ubShotMode)
{
    AITPS_JPG   pJPG = AITC_BASE_JPG;
    MMP_UBYTE   ubRcIdx = MMP_DSC_JPEG_RC_ID_CAPTURE;

    m_JpegSourcePath = SourcePath;
    m_JpegShotMode   = ubShotMode;

    /* Set Compress/Line Buffer */
    pJPG->JPG_ENC_CMP_BUF_ST	= m_sDscCaptureBuf.ulCompressStart;
    pJPG->JPG_ENC_CMP_BUF_ED 	= m_sDscCaptureBuf.ulCompressEnd;
    pJPG->JPG_LINE_BUF_ST       = m_sDscCaptureBuf.ulLineStart;
    pJPG->JPG_DEC_BUF_CMP_ST    = 0;
    pJPG->JPG_DEC_BUF_CMP_ED    = 0;

    /* Enable Jpeg encode */
	pJPG->JPG_INT_CPU_SR = JPG_INT_ENC_DONE | JPG_INT_CMP_BUF_OVERFLOW;
	pJPG->JPG_INT_CPU_EN = JPG_INT_ENC_DONE | JPG_INT_CMP_BUF_OVERFLOW;

	switch(ubShotMode)
	{
		case MMPF_DSC_SHOTMODE_CONTINUOUS:
			MMPF_JPG_SetEncDoneMode(MMPF_DSC_ENCDONE_CONTINUOUS_SHOT);
			ubRcIdx = MMP_DSC_JPEG_RC_ID_CAPTURE;
		break;	
		case MMPF_DSC_SHOTMODE_MJPEG:
			MMPF_JPG_SetEncDoneMode(MMPF_DSC_ENCDONE_WIFI_MJPG);
			ubRcIdx = MMP_DSC_JPEG_RC_ID_MJPEG_1ST_STREAM;
		break;
		case MMPF_DSC_SHOTMODE_VID2MJPEG:
			MMPF_JPG_SetEncDoneMode(MMPF_DSC_ENCDONE_VID_TO_MJPG);
			ubRcIdx = MMP_DSC_JPEG_RC_ID_MJPEG_1ST_STREAM;
		break;
		default:
			MMPF_JPG_SetEncDoneMode(MMPF_DSC_ENCDONE_SIZE_CTL);
			ubRcIdx = MMP_DSC_JPEG_RC_ID_CAPTURE;
		break;
    }

	/* Set LineBuffer Type */
	MMPF_DSC_SetLineBufType();

    /* Reset JPEG to clear the abnormal status from decoding */
    MMPF_SYS_ResetHModule(MMPF_SYS_MDL_JPG, MMP_FALSE);

	/* If FPS control enable, MJPEG will be triggered at ISP ISR */
    MMPF_JPG_TriggerEncode();

	/* The frame source is triggered by decoder at Video to MJPEG mode */
	if (ubShotMode == MMPF_DSC_SHOTMODE_VID2MJPEG) {
		return MMP_ERR_NONE;
	}

    if (ubShotMode == MMPF_DSC_SHOTMODE_CONTINUOUS ||
    	ubShotMode == MMPF_DSC_SHOTMODE_MJPEG) {
        return MMP_ERR_NONE;
    }

    if (m_JpegSizeCtlMode[ubRcIdx] == MMPF_DSC_SIZE_CTL_DISABLE) 
    {
#if SUPPORT_UVC_JPEG==0
        RTNA_DBG_Str(0, "Size Control Off\r\n");
#endif
        if (MMP_FALSE == MMPF_DSC_PollingReg(MMPF_DSC_CheckEncodeStart, DSC_TIMEOUT_MS))
		{
			MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "MMPF_DSC_CheckEncodeStart Fail\r\n");
			return MMP_DSC_ERR_CAPTURE_FAIL;
		}
    }
    else 
    {
		while (1) 
		{
       		RTNA_DBG_Str(0, "Re-Encode Loop\r\n");

       		/* Wait the JPEG encode done */
            if (MMPF_OS_AcquireSem(m_JpgEncDoneSem, DSC_CAPTURE_DONE_SEM_TIMEOUT) != MMP_ERR_NONE) {
                MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "Wait m_JpgEncDoneSem Failed\r\n");
			}
			
			if (!m_bScaleOK[ubRcIdx]) 
			{
				RTNA_DBG_Str(3, "Size is not fit the target\r\n");
			
				if (pJPG->JPG_CTL & JPG_ENC_EN) {
				    // Need to reset JPEG or IBC?
                    MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "JPEG encode engine Failed\r\n");
				}
				
				/* Trigger Re-Encode */
                MMPF_JPG_TriggerEncode();
			}
			else 
			{
				RTNA_DBG_Str(3, "Size is fit the target\r\n");
				break;
			}
		}
    }

    if (MMP_FALSE == MMPF_DSC_PollingReg(MMPF_DSC_CheckEncodeDone, DSC_TIMEOUT_MS))
	{
		MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "MMPF_DSC_CheckEncodeDone Fail\r\n");
		return MMP_DSC_ERR_CAPTURE_FAIL;
	}

    m_ulEncodedJpegSize = pJPG->JPG_ENC_FRAME_SIZE;
#if SUPPORT_UVC_JPEG==0
    /* RTNA_DBG_PrintLong(0, m_ulEncodedJpegSize); */
    printc("0x%08X\r\n", m_ulEncodedJpegSize);

    /* RTNA_DBG_Str(0, "MMPF_DSC_EncodeJpeg SUCCESS\r\n"); */
    printc("MMPF_DSC_EncodeJpeg SUCCESS:%d\r\n",OSTime);
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_GetJpeg
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_DSC_GetJpeg(MMP_ULONG *ulJpegBuf, MMP_ULONG *ulSize, MMP_ULONG *ulTime)
{
	*ulJpegBuf  = m_sDscCaptureBuf.ulCompressStart;
    *ulSize     = m_ulEncodedJpegSize;
    *ulTime     = m_ulEncodedJpegTime;

    return MMP_ERR_NONE;
}

#if 0
void _____OS_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_DSC_Task
//  Description : DSC main function
//------------------------------------------------------------------------------
void MMPF_DSC_Task(void)
{
    AITPS_AIC   	pAIC = AITC_BASE_AIC;
    
    RTNA_AIC_Open(  pAIC, AIC_SRC_JPG, jpg_isr_a,
                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_JPG);

    m_JpgEncDoneSem = MMPF_OS_CreateSem(0);
    if (m_JpgEncDoneSem == MMPF_OS_CREATE_SEM_INT_ERR) {
        MMP_PRINT_RET_ERROR(DSC_DBG_LEVEL, 0, "m_JpgEncDoneSem Create Failed\r\n");
    }
}

#endif // (DSC_R_EN)

/// @}
