//==============================================================================
//
//  File        : mmpf_rawproc.c
//  Description : Raw Processor Function
//  Author      : Ted Huang
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "Customer_config.h"
#include "lib_retina.h"
#include "hdr_cfg.h"
#include "mmp_reg_vif.h"
#include "mmp_reg_rawproc.h"
#include "mmp_reg_color.h"
#include "mmpf_rawproc.h"
#include "mmpf_bayerscaler.h"
#include "mmpf_scaler.h"
#include "mmpf_mci.h"
#include "mmpf_dma.h"
#include "mmpf_ibc.h"
#include "mmpf_system.h"
#include "mmpf_sensor.h"
#include "mmpf_graphics.h"
#if (VIDEO_R_EN)
#include "mmpf_mp4venc.h"
#endif

/** @addtogroup MMPF_RAWPROC
@{
*/

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMP_ULONG    m_ulRawStoreP0Addr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP1Addr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP2Addr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP3Addr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];

static MMP_ULONG    m_ulRawStoreP0EndAddr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP1EndAddr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP2EndAddr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];
static MMP_ULONG    m_ulRawStoreP3EndAddr[MMP_RAW_MDL_NUM][MAX_RAW_STORE_BUF_NUM];

static MMP_BOOL     m_ulRawStoreRingEnable[MMP_RAW_MDL_NUM][MMP_RAW_STORE_PLANE_NUM]; 
static MMP_ULONG    m_ulRawStoreBufNum[MMP_RAW_MDL_NUM][MMP_RAW_STORE_PLANE_NUM];
static MMP_ULONG    m_ulRawStoreIdx[MMP_RAW_MDL_NUM][MMP_RAW_STORE_PLANE_NUM];
static MMP_ULONG    m_ulRawFetchIdx[MMP_RAW_MDL_NUM][MMP_RAW_STORE_PLANE_NUM];

static MMP_USHORT   m_usRawStoreWidth[MMP_RAW_MDL_NUM]			= {0};
static MMP_USHORT   m_usRawStoreHeight[MMP_RAW_MDL_NUM]         = {0};

static MMP_BOOL		m_bRestoreBurstFetch							= MMP_FALSE;

static MMP_BOOL		m_bRawStoreGrabEnable[MMP_RAW_MDL_NUM]		= {MMP_FALSE};
static MMP_USHORT   m_usRawStoreGrabW[MMP_RAW_MDL_NUM]          = {0};
static MMP_USHORT   m_usRawStoreGrabH[MMP_RAW_MDL_NUM]         	= {0};

static MMP_RAW_FETCH_ATTR   	m_RawFetchAttr;

static RawCallBackFunc	*CallBackFuncRaw0[MMP_RAW_STORE_PLANE_NUM][MMP_RAW_EVENT_MAX];
static RawCallBackFunc	*CallBackFuncRaw1[MMP_RAW_STORE_PLANE_NUM][MMP_RAW_EVENT_MAX];
static void             *CallBackArguRaw0[MMP_RAW_STORE_PLANE_NUM][MMP_RAW_EVENT_MAX];
static void             *CallBackArguRaw1[MMP_RAW_STORE_PLANE_NUM][MMP_RAW_EVENT_MAX];

/* HDR */
static MMP_ULONG	m_ulMergeFrameAddr0 = 0;
static MMP_ULONG	m_ulMergeFrameAddr1 = 0;
static MMP_ULONG	m_ulFetchFrameAddr0 = 0;
static MMP_ULONG	m_ulFetchFrameAddr1 = 0;
static MMP_ULONG	m_ulMergeFrameSubW  = 0;
static MMP_ULONG	m_ulMergeFrameSubH  = 0;
static MMP_ULONG	m_ulHdrBufferEnd	= 0;
static MMP_UBYTE    m_ubHdrSensorId     = PRM_SENSOR;
MMP_UBYTE           m_ubHdrMainRawId    = MMP_RAW_MDL_ID0;
MMP_UBYTE           m_ubHdrSubRawId     = MMP_RAW_MDL_ID1;

/* YUV Store */
MMP_UBYTE           m_ubYUVRawId        = MMP_RAW_MDL_ID0;
#if (TVDEC_SNR_USE_DMA_DEINTERLACE)
static MMP_ULONG    m_ulDeInterlaceYBuf = 0;
static MMP_ULONG    m_ulDeInterlaceUBuf = 0;
static MMP_ULONG    m_ulDeInterlaceVBuf = 0;
static MMP_ULONG    m_ulDmaDoneCnt      = 0;
#endif

/* Dual Sensor */
MMP_UBYTE			m_ubRawFetchSnrId	= PRM_SENSOR;

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMP_BOOL		m_bDoCaptureFromRawPath;

#if (SUPPORT_DUAL_SNR_PCAM_OUT)
extern MMP_BOOL		gbDualSnrPcamOutEnable;
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_ISR
//  Description : This function is the raw processor ISR.
//------------------------------------------------------------------------------
/** 
 * @brief This function is the raw processor ISR.
 * 
 *  This function is the raw processor ISR.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_ISR(void)
{
   	AITPS_RAWPROC 	pRAW = AITC_BASE_RAWPROC;
    MMP_ULONG	  	intsrc[MMP_RAW_MDL_NUM][MMP_RAW_STORE_PLANE_NUM];
    MMP_UBYTE		i = MMP_RAW_MDL_ID0;
    
    /* For 1st Plane */
    intsrc[MMP_RAW_MDL_ID0][0] = pRAW->RAWPROC_CPU_INT_EN_PLANE0 & pRAW->RAWPROC_CPU_INT_SR_PLANE0;
    pRAW->RAWPROC_CPU_INT_SR_PLANE0 = intsrc[MMP_RAW_MDL_ID0][0];

    intsrc[MMP_RAW_MDL_ID1][0] = pRAW->RAWPROC1_CPU_INT_EN_PLANE0 & pRAW->RAWPROC1_CPU_INT_SR_PLANE0;
    pRAW->RAWPROC1_CPU_INT_SR_PLANE0 = intsrc[MMP_RAW_MDL_ID1][0];
    
    for (i = MMP_RAW_MDL_ID0; i < MMP_RAW_MDL_NUM; i++) {

		if (intsrc[i][0] & BAYER_STORE_DONE_PLANE0) {

	        if (i == 0 && CallBackFuncRaw0[0][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[0][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw0[0][MMP_RAW_EVENT_STORE_DONE]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[0][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[0][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw1[0][MMP_RAW_EVENT_STORE_DONE]);
	        }
		}

		if (intsrc[i][0] & BAYER_STORE_FIFO_FULL_PLANE0) {

	        if (i == 0 && CallBackFuncRaw0[0][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[0][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw0[0][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[0][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[0][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw1[0][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
		}
		
		if (intsrc[i][0] & BAYER_FETCH_DONE) {

	        if (i == 0 && CallBackFuncRaw0[0][MMP_RAW_EVENT_FETCH_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[0][MMP_RAW_EVENT_FETCH_DONE];
	            CallBack(CallBackArguRaw0[0][MMP_RAW_EVENT_FETCH_DONE]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[0][MMP_RAW_EVENT_FETCH_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[0][MMP_RAW_EVENT_FETCH_DONE];
	            CallBack(CallBackArguRaw1[0][MMP_RAW_EVENT_FETCH_DONE]);
	        }
		}
    }
	
	/* For 2nd Plane */
    intsrc[MMP_RAW_MDL_ID0][1] = pRAW->RAWPROC_CPU_INT_EN_PLANE123 & pRAW->RAWPROC_CPU_INT_SR_PLANE123;
    pRAW->RAWPROC_CPU_INT_SR_PLANE123 = intsrc[MMP_RAW_MDL_ID0][1];

    intsrc[MMP_RAW_MDL_ID1][1] = pRAW->RAWPROC1_CPU_INT_EN_PLANE123 & pRAW->RAWPROC1_CPU_INT_SR_PLANE123;
    pRAW->RAWPROC1_CPU_INT_SR_PLANE123 = intsrc[MMP_RAW_MDL_ID1][1];
    
    for (i = MMP_RAW_MDL_ID0; i < MMP_RAW_MDL_NUM; i++) {

		if (intsrc[i][1] & BAYER_STORE_DONE_PLANE1) {

	        if (i == 0 && CallBackFuncRaw0[1][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[1][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw0[1][MMP_RAW_EVENT_STORE_DONE]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[1][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[1][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw1[1][MMP_RAW_EVENT_STORE_DONE]);
	        }
		}

		if (intsrc[i][1] & BAYER_STORE_FIFO_FULL_PLANE1) {

	        if (i == 0 && CallBackFuncRaw0[1][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[1][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw0[1][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[1][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[1][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw1[1][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
		}

		if (intsrc[i][1] & BAYER_STORE_DONE_PLANE2) {

	        if (i == 0 && CallBackFuncRaw0[2][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[2][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw0[2][MMP_RAW_EVENT_STORE_DONE]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[2][MMP_RAW_EVENT_STORE_DONE]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[2][MMP_RAW_EVENT_STORE_DONE];
	            CallBack(CallBackArguRaw1[2][MMP_RAW_EVENT_STORE_DONE]);
	        }
		}

		if (intsrc[i][1] & BAYER_STORE_FIFO_FULL_PLANE2) {

	        if (i == 0 && CallBackFuncRaw0[2][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw0[2][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw0[2][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
	        else if (i == 1 && CallBackFuncRaw1[2][MMP_RAW_EVENT_STORE_FIFO_FULL]){
	            RawCallBackFunc *CallBack = CallBackFuncRaw1[2][MMP_RAW_EVENT_STORE_FIFO_FULL];
	            CallBack(CallBackArguRaw1[2][MMP_RAW_EVENT_STORE_FIFO_FULL]);
	        }
		}
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_RegisterIntrCallBack
//  Description : This function register interrupt callback function.
//------------------------------------------------------------------------------
/** 
 * @brief This function register interrupt callback function.
 * 
 *  This function register interrupt callback function.
 * @param[in] ubRawIdx  : stands for raw ID.
 * @param[in] ubPlane   : stands for plane ID.
 * @param[in] event     : stands for interrupt event.
 * @param[in] pCallBack : stands for interrupt callback function.
 * @param[in] pArgument : stands for argument for callback function.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_RegisterIntrCallBack(MMP_UBYTE 		ubRawId, 
										  MMP_UBYTE 		ubPlane, 
										  MMP_RAW_EVENT 	event, 
										  RawCallBackFunc 	*pCallBack,
										  void              *pArgument)
{
    if (pCallBack) 
    {
    	if (ubRawId == MMP_RAW_MDL_ID0) {
        	CallBackFuncRaw0[ubPlane][event] = pCallBack;
        	CallBackArguRaw0[ubPlane][event] = pArgument;
        }	
    	else if (ubRawId == MMP_RAW_MDL_ID1) {
        	CallBackFuncRaw1[ubPlane][event] = pCallBack;
            CallBackArguRaw1[ubPlane][event] = pArgument;
        }
    }
    else
    {
    	if (ubRawId == MMP_RAW_MDL_ID0) {
        	CallBackFuncRaw0[ubPlane][event] = NULL;
        	CallBackArguRaw0[ubPlane][event] = NULL;
        }	
    	else if (ubRawId == MMP_RAW_MDL_ID1) {
        	CallBackFuncRaw1[ubPlane][event] = NULL;
        	CallBackArguRaw1[ubPlane][event] = NULL;
        }
    }
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableInterrupt
//  Description : The function set interrupt.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_EnableInterrupt(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane,
									 MMP_UBYTE ubFlag, MMP_BOOL bEnable)
{
	AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

	if (bEnable) {
		
		if (ubRawId == MMP_RAW_MDL_ID0)
		{
			if (ubPlane == MMP_RAW_STORE_PLANE0) {
				pRAW->RAWPROC_CPU_INT_SR_PLANE0 |= ubFlag;
				pRAW->RAWPROC_CPU_INT_EN_PLANE0 |= ubFlag;
			}
			else {
				pRAW->RAWPROC_CPU_INT_SR_PLANE123 |= ubFlag;
				pRAW->RAWPROC_CPU_INT_EN_PLANE123 |= ubFlag;
			}
		}
		else 
		{
			if (ubPlane == MMP_RAW_STORE_PLANE0) {
				pRAW->RAWPROC1_CPU_INT_SR_PLANE0 |= ubFlag;
				pRAW->RAWPROC1_CPU_INT_EN_PLANE0 |= ubFlag;
			}
			else {
				pRAW->RAWPROC1_CPU_INT_SR_PLANE123 |= ubFlag;
				pRAW->RAWPROC1_CPU_INT_EN_PLANE123 |= ubFlag;
			}
		}
	}
	else {

		if (ubRawId == MMP_RAW_MDL_ID0)
		{
			if (ubPlane == MMP_RAW_STORE_PLANE0) {
				pRAW->RAWPROC_CPU_INT_EN_PLANE0 &= ~ubFlag;
			}
			else {
				pRAW->RAWPROC_CPU_INT_EN_PLANE123 &= ~ubFlag;
			}
		}
		else 
		{
			if (ubPlane == MMP_RAW_STORE_PLANE0) {
				pRAW->RAWPROC1_CPU_INT_EN_PLANE0 &= ~ubFlag;
			}
			else {
				pRAW->RAWPROC1_CPU_INT_EN_PLANE123 &= ~ubFlag;
			}
		}
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_OpenInterrupt
//  Description : The function open interrupt.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_OpenInterrupt(MMP_BOOL bEnable)
{
	AITPS_AIC pAIC = AITC_BASE_AIC;

	if (bEnable) {
		RTNA_AIC_Open(pAIC, AIC_SRC_RAW, raw_isr_a,
                    		AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);

		RTNA_AIC_IRQ_En(pAIC, AIC_SRC_RAW);
	}
	else {
		RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_RAW);
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_ResetModule
//  Description : This function reset Raw module.
//------------------------------------------------------------------------------
/** 
 * @brief This function reset Raw module.
 * 
 *  This function reset Raw module.  
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_ResetModule(MMP_UBYTE ubRawId)
{
	if (ubRawId == MMP_RAW_MDL_ID0)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_RAW_S0, MMP_FALSE);
	else if (ubRawId == MMP_RAW_MDL_ID1)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_RAW_S1, MMP_FALSE);
	
	MMPF_SYS_ResetHModule(MMPF_SYS_MDL_RAW_F, MMP_FALSE);
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_GetFetchAttr
//  Description : The function returns current fetch attribute
//------------------------------------------------------------------------------
MMP_RAW_FETCH_ATTR * MMPF_RAWPROC_GetFetchAttr(void)
{
    return &m_RawFetchAttr;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchPixelDelay
//  Description : This function set Raw fetch pixel delay
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw fetch pixel delay
 * 
 *  This function set Raw fetch pixel delay.
 * @param[in] ubPixelDelay : stands for Raw fetch pixel delay.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetFetchPixelDelay(MMP_UBYTE ubPixelDelay)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    if (ubPixelDelay > RAWPROC_F_MAX_PIXL_DELAY) {
        ubPixelDelay = RAWPROC_F_MAX_PIXL_DELAY;
    }

    pRAW->RAWPROC_F_PIXL_DELAY = ubPixelDelay;
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_CalcFetchPixelDelay
//  Description : This function calculate Raw fetch pixel delay
//------------------------------------------------------------------------------
/** 
 * @brief This function calculate Raw fetch pixel delay
 * 
 *  This function calculate Raw fetch pixel delay.
 * @return It return the function status.  
 */
MMP_UBYTE MMPF_RAWPROC_CalcFetchPixelDelay(void)
{
    MMP_ULONG ulBayerFreq, ulColorFreq;
    MMP_UBYTE ubPixelDelay = 0; /* 0 means no delay */
    
    MMPF_PLL_GetGroupFreq(CLK_GRP_BAYER, &ulBayerFreq);
    MMPF_PLL_GetGroupFreq(CLK_GRP_COLOR, &ulColorFreq);
    
    if (ulBayerFreq > ulColorFreq) {
        ubPixelDelay = ulBayerFreq / ulColorFreq;
    }
    else {
        ubPixelDelay = 0;
    }
    
    #if (BIND_SENSOR_OV2710)
    ubPixelDelay = 1;
    #endif
    
    return ubPixelDelay;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchLineDelay
//  Description : This function set Raw fetch line delay.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw fetch line delay.
 * 
 *  This function set Raw fetch line delay.        
 * @param[in] ubLineDelay  : stands for Raw fetch line delay.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetFetchLineDelay(MMP_USHORT usLineDelay)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (usLineDelay > RAWPROC_F_MAX_LINE_DELAY) {
        usLineDelay = RAWPROC_F_MAX_LINE_DELAY;
    }

    pRAW->RAWPROC_F_LINE_DELAY = usLineDelay;
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchRange
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetFetchRange(MMP_USHORT  usLeft,
                                   MMP_USHORT  usTop,
                                   MMP_USHORT  usWidth,
                                   MMP_USHORT  usHeight,
                                   MMP_USHORT  usLineOffset)
{
	MMP_BOOL bUpdateISP = MMP_FALSE;
    
    if (usWidth  != m_RawFetchAttr.usWidth || 
    	usHeight != m_RawFetchAttr.usHeight) {
    	bUpdateISP = MMP_TRUE;
    }
    
    m_RawFetchAttr.usLeft         = usLeft;
    m_RawFetchAttr.usTop          = usTop;
    m_RawFetchAttr.usWidth        = usWidth;
    m_RawFetchAttr.usHeight       = usHeight;
    m_RawFetchAttr.usLineOffset   = usLineOffset;

    if (bUpdateISP) // Update ISP 3A acc param 
    {
        if ((gsHdrCfg.bVidEnable == MMP_TRUE && gsHdrCfg.ubMode == HDR_MODE_STAGGER) ||
            (gsHdrCfg.bDscEnable == MMP_TRUE && gsHdrCfg.ubMode == HDR_MODE_STAGGER)) {
            usWidth = usWidth / 2;
        }

    	MMPF_ISP_UpdateInputSize(usWidth, usHeight);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_IsFetchBusy
//  Description : Check raw fetch finish
//------------------------------------------------------------------------------
MMP_BOOL MMPF_RAWPROC_IsFetchBusy(void)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    return ((pRAW->RAWPROC_MODE_SEL & RAWPROC_FETCH_EN) ? (MMP_TRUE) : (MMP_FALSE));
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableStorePath
//  Description : Enable/Disable RAWPROC store path
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_EnableStorePath(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_RAW_STORE_SRC ubRawSrc, MMP_BOOL bEnable)
{
    AITPS_RAWPROC   pRAW = AITC_BASE_RAWPROC;
    AITPS_VIF       pVIF = AITC_BASE_VIF;
    AITPS_ISP       pISP = AITC_BASE_ISP;
	
	/* Set RAW store source */
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
  			#if (HDR_DBG_RAW_ENABLE)
  			pRAW->RAWPROC_S_SRC_SEL = RAWPROC_SRC_VIF0;
  			#else
  			pRAW->RAWPROC_S_SRC_SEL = (ubRawSrc & RAWPROC_SRC_MASK);
  			#endif
  			
  			switch(ubPlane) {
  		    	case MMP_RAW_STORE_PLANE0:
		  		    if (bEnable)   
				        pRAW->RAWPROC_MODE_SEL |= RAWPROC_S_PLANE0_EN;
				    else
				        pRAW->RAWPROC_MODE_SEL &= ~(RAWPROC_S_PLANE0_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE1:
		  		    if (bEnable)   
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE1_EN;
				    else
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE1_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE2:
		  		    if (bEnable)   
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE2_EN;
				    else
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE2_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE3:
		  		    if (bEnable)   
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE3_EN;
				    else
				        pRAW->RAWPROC_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE3_EN);
		    	break;
		    }
  		break;
  		case MMP_RAW_MDL_ID1:
  			#if (HDR_DBG_RAW_ENABLE)
  			pRAW->RAWPROC_S_SRC_SEL = RAWPROC_SRC_VIF1;
  			#else
  			pRAW->RAWPROC1_S_SRC_SEL = (ubRawSrc & RAWPROC_SRC_MASK);
            #endif
     
  			switch(ubPlane) {
  		    	case MMP_RAW_STORE_PLANE0:
		  		    if (bEnable)   
				        pRAW->RAWPROC1_MODE_SEL |= RAWPROC_S_PLANE0_EN;
				    else
				        pRAW->RAWPROC1_MODE_SEL &= ~(RAWPROC_S_PLANE0_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE1:
		  		    if (bEnable)   
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE1_EN;
				    else
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE1_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE2:
		  		    if (bEnable)   
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE2_EN;
				    else
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE2_EN);
		    	break;
  		    	case MMP_RAW_STORE_PLANE3:
		  		    if (bEnable)   
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 |= RAWPROC_S_PLANE3_EN;
				    else
				        pRAW->RAWPROC1_S_EN_CTL_PLANE123 &= ~(RAWPROC_S_PLANE3_EN);
		    	break;
		    }		    
  		break;
  	}
	
	/* Set VIF output destination */
   	switch(ubRawSrc)
   	{
		case MMP_RAW_S_SRC_VIF0:
			pVIF->VIF_RAW_OUT_EN[0] &= ~(VIF_OUT_MASK);

		    if (bEnable)
			    pVIF->VIF_RAW_OUT_EN[0] |= VIF_2_RAW_EN;
			else
			    pVIF->VIF_RAW_OUT_EN[0] &= ~(VIF_2_RAW_EN);

	        m_usRawStoreWidth[ubRawId]  = pVIF->VIF_GRAB[0].PIXL_ED - pVIF->VIF_GRAB[0].PIXL_ST + 1;
            m_usRawStoreHeight[ubRawId] = pVIF->VIF_GRAB[0].LINE_ED - pVIF->VIF_GRAB[0].LINE_ST + 1;
		break;
		case MMP_RAW_S_SRC_VIF1:
			pVIF->VIF_RAW_OUT_EN[1] &= ~(VIF_OUT_MASK);

		    if (bEnable)
			    pVIF->VIF_RAW_OUT_EN[1] |= VIF_2_RAW_EN; 
		    else
		        pVIF->VIF_RAW_OUT_EN[1] &= ~(VIF_2_RAW_EN); 

	        m_usRawStoreWidth[ubRawId]  = pVIF->VIF_GRAB[1].PIXL_ED - pVIF->VIF_GRAB[1].PIXL_ST + 1;
            m_usRawStoreHeight[ubRawId] = pVIF->VIF_GRAB[1].LINE_ED - pVIF->VIF_GRAB[1].LINE_ST + 1;
		break;
		case MMP_RAW_S_SRC_ISP_BEFORE_BS:
		case MMP_RAW_S_SRC_ISP_AFTER_BS:
		case MMP_RAW_S_SRC_ISP_Y_DATA:
		case MMP_RAW_S_SRC_ISP_AFTER_HDR:
			pVIF->VIF_RAW_OUT_EN[0] &= ~(VIF_OUT_MASK);

		    if (bEnable) {
		        pVIF->VIF_RAW_OUT_EN[0] |= VIF_2_ISP_EN;
		    	pISP->ISP_MISC_CTL		|= ISP_RAW_MODE_EN;
		    }
		    else {
		        pVIF->VIF_RAW_OUT_EN[0] &= ~(VIF_2_ISP_EN);
			}
			
		    // Raw Store From ISP
		    if (ubRawSrc == MMP_RAW_S_SRC_ISP_BEFORE_BS) {
		    	MMPF_BayerScaler_GetResolution(0, &m_usRawStoreWidth[ubRawId], &m_usRawStoreHeight[ubRawId]);
            }
            else if (ubRawSrc == MMP_RAW_S_SRC_ISP_AFTER_BS) {
 		    	MMPF_BayerScaler_GetResolution(1, &m_usRawStoreWidth[ubRawId], &m_usRawStoreHeight[ubRawId]);
            }
            else {
            	//TBD
            }
   		break;
        default:
        //
        break;
   	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableFetchPath
//  Description : Enable/Disable RAWPROC fetch path
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_EnableFetchPath(MMP_BOOL bEnable)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    AITPS_ISP     pISP = AITC_BASE_ISP;

    if (bEnable) {
        pRAW->RAWPROC_MODE_SEL  |= RAWPROC_F_BURST_EN;
        
        pISP->ISP_MISC_CTL		&= ~(ISP_RAWF_DST_MASK);
        pISP->ISP_MISC_CTL		|= ISP_RAW_MODE_EN;
        
        if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
        	(gsHdrCfg.bRawGrabEnable)) {
            pISP->ISP_MISC_CTL 	|= ISP_RAWF_DST_BEFORE_HDR;
        }
    }
    else {
    	pRAW->RAWPROC_MODE_SEL 	&= ~(RAWPROC_F_BURST_EN);
    	pISP->ISP_MISC_CTL		&= ~(ISP_RAW_MODE_EN);

        if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
        	(gsHdrCfg.bRawGrabEnable)) {
            pISP->ISP_MISC_CTL 	&= ~(ISP_RAWF_DST_BEFORE_HDR);
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_UpdateStoreAddr
//  Description : 
//------------------------------------------------------------------------------
/** @brief Store the sensor input frame data

The function store the sensor input frame data
@param[in]  ubRawId    stand for raw module index.
@param[in]  ubPlane    stand for raw plane index.
*/
MMP_ERR MMPF_RAWPROC_UpdateStoreAddr(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    switch(ubRawId)
    {
        case MMP_RAW_MDL_ID0:

        	switch(ubPlane)
        	{
        		case MMP_RAW_STORE_PLANE0:
		            pRAW->RAWPROC_S_ADDR_PLANE0 = m_ulRawStoreP0Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC_S_END_ADDR_PLANE0 = m_ulRawStoreP0EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC_S_END_ADDR_PLANE0 = 0;
		    	break;        
        		case MMP_RAW_STORE_PLANE1:
		            pRAW->RAWPROC_S_ADDR_PLANE1 = m_ulRawStoreP1Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC_S_END_ADDR_PLANE1 = m_ulRawStoreP1EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC_S_END_ADDR_PLANE1 = 0;
		    	break;
        		case MMP_RAW_STORE_PLANE2:
		            pRAW->RAWPROC_S_ADDR_PLANE2 = m_ulRawStoreP2Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC_S_END_ADDR_PLANE2 = m_ulRawStoreP2EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC_S_END_ADDR_PLANE2 = 0;
		    	break;
        		case MMP_RAW_STORE_PLANE3:
		            pRAW->RAWPROC_S_ADDR_PLANE3 = m_ulRawStoreP3Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC_S_END_ADDR_PLANE3 = m_ulRawStoreP3EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC_S_END_ADDR_PLANE3 = 0;
		    	break;
            }
        break;
        case MMP_RAW_MDL_ID1:
        
        	switch(ubPlane)
        	{
        		case MMP_RAW_STORE_PLANE0:
		            pRAW->RAWPROC1_S_ADDR_PLANE0 = m_ulRawStoreP0Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE0 = m_ulRawStoreP0EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE0 = 0;
		    	break;        
        		case MMP_RAW_STORE_PLANE1:
		            pRAW->RAWPROC1_S_ADDR_PLANE1 = m_ulRawStoreP1Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE1 = m_ulRawStoreP1EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE1 = 0;
		    	break;
        		case MMP_RAW_STORE_PLANE2:
		            pRAW->RAWPROC1_S_ADDR_PLANE2 = m_ulRawStoreP2Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE2 = m_ulRawStoreP2EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE2 = 0;
		    	break;
        		case MMP_RAW_STORE_PLANE3:
		            pRAW->RAWPROC1_S_ADDR_PLANE3 = m_ulRawStoreP3Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            
		            if (m_ulRawStoreRingEnable[ubRawId][ubPlane]) 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE3 = m_ulRawStoreP3EndAddr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
		            else 
		                pRAW->RAWPROC1_S_END_ADDR_PLANE3 = 0;
		    	break;
            }
        break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_FetchData
//  Description : 
//------------------------------------------------------------------------------
/** @brief Fetch the sensor input frame data

The function fetch the sensor input frame data

@param[in]  ubRawId     	stand for raw module index.
@param[in]  RotateType  	stand for rotate type.
@param[in]  mirrorType   	stand for mirror type.
*/
MMP_ERR MMPF_RAWPROC_FetchData (MMP_UBYTE                   ubSnrSel,
                                MMP_UBYTE                   ubRawId,
                                MMP_RAW_FETCH_ROTATE_TYPE  	RotateType,
                                MMP_RAW_FETCH_MIRROR_TYPE  	mirrorType)
{
    AITPS_VIF       pVIF = AITC_BASE_VIF;
    AITPS_RAWPROC   pRAW = AITC_BASE_RAWPROC;
    AITPS_ISP       pISP = AITC_BASE_ISP;
    MMP_UBYTE       ubXpos = 0;
    MMP_UBYTE       ubYpos = 0;
    MMP_UBYTE       ubTempColorID;
    MMP_UBYTE       ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);
    MMP_SHORT       sLineOffset;
	MMP_SCAL_FIT_RANGE	fitrangeBayer;
	MMP_SCAL_GRAB_CTRL	grabctlBayer;

    if (MMPF_RAWPROC_IsFetchBusy())
    {
        RTNA_DBG_Str(0, "Raw Fetch Busy!\r\n");
        return MMP_ERR_NONE;
    }

	#if (SUPPORT_ISP_TIMESHARING)
	{
		MMP_UBYTE 	ubRet 	= 0;
		MMP_USHORT 	usCount = 0;
		extern MMPF_OS_SEMID m_IspFrmEndSem;

	    ubRet = MMPF_OS_AcquireSem(m_IspFrmEndSem, 100);

	    if (ubRet == OS_ERR_PEND_ISR) {
	        MMPF_OS_AcceptSem(m_IspFrmEndSem, &usCount);
	        if (usCount == 0) {
	            return MMP_ERR_NONE;
	        }
	    }
	    else if (ubRet != OS_NO_ERR) {
	        return MMP_ERR_NONE;
	    }
	    
	    m_ubRawFetchSnrId = ubRawId;
    }
	#endif

    /* for avoid compiling warning */
    sLineOffset = (MMP_SHORT)m_RawFetchAttr.usLineOffset;
    
    /* Set RawFetch Parameter */
    if (mirrorType == MMP_RAW_FETCH_NO_MIRROR)
    {
        switch(RotateType)
        {
            case MMP_RAW_FETCH_NO_ROTATE:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = (m_RawFetchAttr.usLeft + m_RawFetchAttr.usTop * m_RawFetchAttr.usLineOffset);
                pRAW->RAWPROC_F_PIX_OFST    = 1;
                pRAW->RAWPROC_F_LINE_OFST   = m_RawFetchAttr.usLineOffset;            
            break;
            case MMP_RAW_FETCH_ROTATE_RIGHT_90:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = m_RawFetchAttr.usLeft + 
                                              (m_RawFetchAttr.usTop + m_RawFetchAttr.usHeight - 1) * m_RawFetchAttr.usLineOffset;
                pRAW->RAWPROC_F_PIX_OFST    = RAW_OFST_2S_COMPLEMENT(-sLineOffset);
                pRAW->RAWPROC_F_LINE_OFST   = RAW_OFST_2S_COMPLEMENT(1);
            break;        
            case MMP_RAW_FETCH_ROTATE_RIGHT_180:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = (m_RawFetchAttr.usLeft + m_RawFetchAttr.usWidth - 1) +
                                              (m_RawFetchAttr.usTop + m_RawFetchAttr.usHeight - 1) * m_RawFetchAttr.usLineOffset;
                pRAW->RAWPROC_F_PIX_OFST    = RAW_OFST_2S_COMPLEMENT(-1);
                pRAW->RAWPROC_F_LINE_OFST   = RAW_OFST_2S_COMPLEMENT(-sLineOffset);
            break;
            case MMP_RAW_FETCH_ROTATE_RIGHT_270:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = (m_RawFetchAttr.usLeft + m_RawFetchAttr.usWidth - 1) +
                                              (m_RawFetchAttr.usTop * m_RawFetchAttr.usLineOffset);
                pRAW->RAWPROC_F_PIX_OFST    = RAW_OFST_2S_COMPLEMENT(sLineOffset);
                pRAW->RAWPROC_F_LINE_OFST   = RAW_OFST_2S_COMPLEMENT(-1);
            break;
        }
        
        if (RotateType != MMP_RAW_FETCH_NO_ROTATE)
        {
        	pRAW->RAWPROC_MODE_SEL &= ~(RAWPROC_F_BURST_EN);
        	m_bRestoreBurstFetch = MMP_TRUE;
    	}
    }
    else
    {
        switch(mirrorType)
        {   
            case MMP_RAW_FETCH_V_MIRROR:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = m_RawFetchAttr.usLeft + 
                                              (m_RawFetchAttr.usTop + m_RawFetchAttr.usHeight - 1) * m_RawFetchAttr.usLineOffset;
                pRAW->RAWPROC_F_PIX_OFST    = RAW_OFST_2S_COMPLEMENT(1);
                pRAW->RAWPROC_F_LINE_OFST   = RAW_OFST_2S_COMPLEMENT(-sLineOffset);
            break;
            case MMP_RAW_FETCH_H_MIRROR:
                pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usWidth;
                pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usHeight;
                pRAW->RAWPROC_F_ADDR        = m_ulRawStoreP0Addr[ubRawId][m_ulRawFetchIdx[ubRawId][0]];
                pRAW->RAWPROC_F_ST_OFST     = (m_RawFetchAttr.usLeft + m_RawFetchAttr.usWidth - 1) +
                                              (m_RawFetchAttr.usTop * m_RawFetchAttr.usLineOffset);
                pRAW->RAWPROC_F_PIX_OFST    = RAW_OFST_2S_COMPLEMENT(-1);
                pRAW->RAWPROC_F_LINE_OFST   = RAW_OFST_2S_COMPLEMENT(sLineOffset);             
            break;
            default:
            //
            break;
        }
    }

    if (m_bLinkISPSel[ubSnrSel])
    {
    	// To Get the original ulInWidth / ulInHeight from BYPASS mode.
        MMPF_BayerScaler_GetZoomInfo(MMP_BAYER_SCAL_BYPASS, &fitrangeBayer, &grabctlBayer);
        
    	if (gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) {
    		fitrangeBayer.ulInGrabW		= pRAW->RAWPROC_F_WIDTH / 2;
    	}
    	else {
    		fitrangeBayer.ulInGrabW		= pRAW->RAWPROC_F_WIDTH;
    	}

        fitrangeBayer.ulInGrabH 		= pRAW->RAWPROC_F_HEIGHT;
        fitrangeBayer.ulOutWidth 		= fitrangeBayer.ulInGrabW;
        fitrangeBayer.ulOutHeight 		= fitrangeBayer.ulInGrabH;
        fitrangeBayer.ulDummyOutPixelX 	= 0;
        fitrangeBayer.ulDummyOutPixelY 	= 0;

        MMPF_BayerScaler_SetZoomInfo(MMP_BAYER_SCAL_RAW_FETCH,
        							 MMP_SCAL_FITMODE_OUT, 
    					             fitrangeBayer.ulInWidth,  fitrangeBayer.ulInHeight,
    					             1,						   1,
    					             fitrangeBayer.ulInGrabW,  fitrangeBayer.ulInGrabH,
    					             fitrangeBayer.ulOutWidth, fitrangeBayer.ulOutHeight,
    					             fitrangeBayer.ulDummyOutPixelX, fitrangeBayer.ulDummyOutPixelY);

        MMPF_BayerScaler_GetZoomInfo(MMP_BAYER_SCAL_RAW_FETCH, &fitrangeBayer, &grabctlBayer);

        MMPF_BayerScaler_SetEngine(&fitrangeBayer, &grabctlBayer);

    	if (grabctlBayer.ulScaleN != grabctlBayer.ulScaleM)
    		MMPF_BayerScaler_SetEnable(MMP_TRUE);
    	else
    		MMPF_BayerScaler_SetEnable(MMP_FALSE);
    }

    /* Set ISP Source and color ID (Frame_Sync) */
    pISP->ISP_MISC_CTL |= ISP_RAW_MODE_EN;
    pISP->ISP_MISC_CTL &= ~(ISP_RAWF_DST_MASK);
    pISP->ISP_MISC_CTL &= ~(ISP_RAWF_COLORID_FMT_MASK);
    
    /* Set HDR relative setting */
    if (gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable)
    {
	    pISP->ISP_MISC_CTL 					|= ISP_RAWF_DST_BEFORE_HDR;	
		pISP->ISP_COLOR_SYS_CTL 			|= ISP_RAW_FETCH_FRM_SYNC_EN;

		MMPF_RAWPROC_SetFetchByteCount(MMPF_MCI_BYTECNT_SEL_256BYTE);

	    if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT) {
	    	MMPF_RAWPROC_SetFetchBitMode(MMP_RAW_COLORFMT_BAYER10);
	    }

	    /* Merge Frame Setting */
	    pRAW->RAWPROC_F_LINE_OFST 			= m_ulMergeFrameSubW;
	    pRAW->RAWPROC_F_ADDR 				= m_ulFetchFrameAddr0;

	    pRAW->RAWPROC_FRAME_MERGE_EN 		= RAWPROC_MERGE_LEFT_RIGHT;
	    pRAW->RAWPROC_SUBFETCH_H_LEN 		= m_ulMergeFrameSubW;
	    pRAW->RAWPROC_SUBFETCH_V_LEN 		= m_ulMergeFrameSubH;
	    pRAW->RAWPROC_2ND_FETCH_ADDR_L 		= m_ulFetchFrameAddr1 & 0xFFFF;
	    pRAW->RAWPROC_2ND_FETCH_ADDR_H 		= (m_ulFetchFrameAddr1 >> 16) & 0xFFFF;
	    pRAW->RAWPROC_2ND_FETCH_ST_OFST_L 	= 0;
	    pRAW->RAWPROC_2ND_FETCH_ST_OFST_H 	= 0;
	    pRAW->RAWPROC_2ND_FETCH_LINE_OFST 	= m_ulMergeFrameSubW;
    }

    switch(RotateType) 
    {
    	case MMP_RAW_FETCH_NO_ROTATE:
    		if ((gsHdrCfg.bVidEnable && gsHdrCfg.bRawGrabEnable) ||
    			(gsHdrCfg.bDscEnable && gsHdrCfg.bRawGrabEnable)) {
	    		ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + (pRAW->RAWS0_GRAB[0].PIXL_ST - 1) + m_RawFetchAttr.usLeft + 1) & 1);
	    		ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + (pRAW->RAWS0_GRAB[0].LINE_ST - 1) + m_RawFetchAttr.usTop + 1) & 1);
    		}
    		else {
	    		ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + m_RawFetchAttr.usLeft + 1) & 1);
	    		ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + m_RawFetchAttr.usTop + 1) & 1);
    		}
    	break;
    	case MMP_RAW_FETCH_ROTATE_RIGHT_90:
    		ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + 0 + 1) & 1);
    		ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + m_RawFetchAttr.usHeight - 1 + 1) & 1);
    	break;
    	case MMP_RAW_FETCH_ROTATE_RIGHT_180:
    		ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + m_RawFetchAttr.usWidth - 1 + 1) & 1);
    		ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + m_RawFetchAttr.usHeight - 1 + 1) & 1);
    	break;
    	case MMP_RAW_FETCH_ROTATE_RIGHT_270:
    		ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + m_RawFetchAttr.usWidth - 1 + 1) & 1);
    		ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + 0 + 1) & 1);
    	break;
    }
    
    ubTempColorID = ((pVIF->VIF_SENSR_CTL[ubVifId] & VIF_COLORID_FORMAT_MASK) >> 2)^((ubYpos << 1) | ubXpos);

    pISP->ISP_MISC_CTL |= ISP_RAWF_COLORID(ubTempColorID);
    
    /* Trigger Fetch */
	pRAW->RAWPROC_MODE_SEL |= RAWPROC_F_BURST_EN;
    pRAW->RAWPROC_MODE_SEL |= RAWPROC_FETCH_EN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_FetchRotatedData
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_FetchRotatedData(  MMP_UBYTE   ubSnrSel,
                                		MMP_USHORT	usOffsetX,
                               	 		MMP_USHORT	usOffsetY,
                               	 		MMP_ULONG	ulFetchAddr)
{
    AITPS_VIF       pVIF = AITC_BASE_VIF;
    AITPS_RAWPROC   pRAW = AITC_BASE_RAWPROC;
    AITPS_ISP       pISP = AITC_BASE_ISP;
    MMP_UBYTE       ubXpos;
    MMP_UBYTE       ubYpos;
    MMP_UBYTE       ubTempColorID;
    MMP_UBYTE       ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);

    if (MMPF_RAWPROC_IsFetchBusy()) 
    {
        RTNA_DBG_Str(0, "Raw Fetch Busy!\r\n");
        return MMP_ERR_NONE;
    }
    
    /* Set RawFetch Parameter */
    pRAW->RAWPROC_F_WIDTH       = m_RawFetchAttr.usWidth;
    pRAW->RAWPROC_F_HEIGHT      = m_RawFetchAttr.usHeight;
    pRAW->RAWPROC_F_ADDR        = ulFetchAddr;
    pRAW->RAWPROC_F_ST_OFST     = 0;
    pRAW->RAWPROC_F_PIX_OFST    = 1;
    pRAW->RAWPROC_F_LINE_OFST   = m_RawFetchAttr.usLineOffset;

	// Update Bayer scaler input and output resolution
	if (m_bLinkISPSel[ubSnrSel])
	{
    	MMP_SCAL_FIT_RANGE	fitrange;
    	MMP_SCAL_GRAB_CTRL	grabOut;
    	
    	fitrange.fitmode 		= MMP_SCAL_FITMODE_OUT;
    	fitrange.scalerType 	= MMP_SCAL_TYPE_BAYERSCALER;
    	fitrange.ulInWidth		= pRAW->RAWPROC_F_WIDTH;
    	fitrange.ulInHeight		= pRAW->RAWPROC_F_HEIGHT;
    	fitrange.ulOutWidth		= fitrange.ulInWidth;
    	fitrange.ulOutHeight	= fitrange.ulInHeight;
		
		fitrange.ulInGrabX		= 1;
		fitrange.ulInGrabY		= 1;
		fitrange.ulInGrabW		= fitrange.ulInWidth;
		fitrange.ulInGrabH		= fitrange.ulInHeight;

		MMPF_Scaler_GetGCDBestFitScale(&fitrange, &grabOut);

		MMPF_BayerScaler_SetEngine(&fitrange, &grabOut);

		if (grabOut.ulScaleN != grabOut.ulScaleM)
			MMPF_BayerScaler_SetEnable(MMP_TRUE);
		else
			MMPF_BayerScaler_SetEnable(MMP_FALSE);
	}

    /* Set ISP Source and color ID (Frame_Sync) */
    pISP->ISP_MISC_CTL |= ISP_RAW_MODE_EN;
    pISP->ISP_MISC_CTL &= ~(ISP_RAWF_DST_MASK);
    pISP->ISP_MISC_CTL &= ~(ISP_RAWF_COLORID_FMT_MASK);
    
    ubXpos = ((pVIF->VIF_GRAB[ubVifId].PIXL_ST + usOffsetX + 1) & 1);
    ubYpos = ((pVIF->VIF_GRAB[ubVifId].LINE_ST + usOffsetY + 1) & 1);
    
    ubTempColorID = ((pVIF->VIF_SENSR_CTL[ubVifId] & VIF_COLORID_FORMAT_MASK) >> 2)^( (ubYpos << 1) | ubXpos);
    pISP->ISP_MISC_CTL |= ISP_RAWF_COLORID(ubTempColorID);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_WaitFetchDone
//  Description : The function wait raw fetch finish.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_WaitFetchDone(void)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    while(pRAW->RAWPROC_MODE_SEL & RAWPROC_FETCH_EN);
    
    if (m_bRestoreBurstFetch == MMP_TRUE) {
		pRAW->RAWPROC_MODE_SEL |= RAWPROC_F_BURST_EN;
		m_bRestoreBurstFetch = MMP_FALSE;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_ClearStoreDone
//  Description : The function clear raw store done status.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_ClearStoreDone(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    if (ubRawId == MMP_RAW_MDL_ID0) {
    
	    switch(ubPlane) {
	    	case MMP_RAW_STORE_PLANE0:
	    		pRAW->RAWPROC_HOST_INT_SR_PLANE0 |= BAYER_STORE_DONE_PLANE0;
	    	break;
	    	case MMP_RAW_STORE_PLANE1:
	    		pRAW->RAWPROC_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE1;
	    	break;
	    	case MMP_RAW_STORE_PLANE2:
	    		pRAW->RAWPROC_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE2;
	    	break;
	    	case MMP_RAW_STORE_PLANE3:
	    		pRAW->RAWPROC_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE3;
	    	break;
	    }
    }
    else if (ubRawId == MMP_RAW_MDL_ID1) {
    
	    switch(ubPlane) {
	    	case MMP_RAW_STORE_PLANE0:
	    		pRAW->RAWPROC1_HOST_INT_SR_PLANE0 |= BAYER_STORE_DONE_PLANE0;
	    	break;
	    	case MMP_RAW_STORE_PLANE1:
	    		pRAW->RAWPROC1_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE1;
	    	break;
	    	case MMP_RAW_STORE_PLANE2:
	    		pRAW->RAWPROC1_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE2;
	    	break;
	    	case MMP_RAW_STORE_PLANE3:
	    		pRAW->RAWPROC1_HOST_INT_SR_PLANE123 |= BAYER_STORE_DONE_PLANE3;
	    	break;
	    }
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_WaitStoreDone
//  Description : The function wait raw store finish.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_WaitStoreDone(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    if (ubRawId == MMP_RAW_MDL_ID0) {
    
	    switch(ubPlane) {
	    	case MMP_RAW_STORE_PLANE0:
	    		while((pRAW->RAWPROC_HOST_INT_SR_PLANE0 & BAYER_STORE_DONE_PLANE0) != BAYER_STORE_DONE_PLANE0);
	    	break;
	    	case MMP_RAW_STORE_PLANE1:
	    		while((pRAW->RAWPROC_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE1) != BAYER_STORE_DONE_PLANE1);
	    	break;
	    	case MMP_RAW_STORE_PLANE2:
	    		while((pRAW->RAWPROC_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE2) != BAYER_STORE_DONE_PLANE2);
	    	break;
	    	case MMP_RAW_STORE_PLANE3:
	    		while((pRAW->RAWPROC_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE3) != BAYER_STORE_DONE_PLANE3);
	    	break;
	    }
    }
	else if (ubRawId == MMP_RAW_MDL_ID1) {
	
	    switch(ubPlane) {
	    	case MMP_RAW_STORE_PLANE0:
	    		while((pRAW->RAWPROC1_HOST_INT_SR_PLANE0 & BAYER_STORE_DONE_PLANE0) != BAYER_STORE_DONE_PLANE0);
	    	break;
	    	case MMP_RAW_STORE_PLANE1:
	    		while((pRAW->RAWPROC1_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE1) != BAYER_STORE_DONE_PLANE1);
	    	break;
	    	case MMP_RAW_STORE_PLANE2:
	    		while((pRAW->RAWPROC1_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE2) != BAYER_STORE_DONE_PLANE2);
	    	break;
	    	case MMP_RAW_STORE_PLANE3:
	    		while((pRAW->RAWPROC1_HOST_INT_SR_PLANE123 & BAYER_STORE_DONE_PLANE3) != BAYER_STORE_DONE_PLANE3);
	    	break;
	    }
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_GetStoreRange
//  Description : The function get raw store range according to the source.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_GetStoreRange(MMP_UBYTE ubRawId, MMP_USHORT* pusW, MMP_USHORT* pusH)
{
	*pusW = m_usRawStoreWidth[ubRawId];
	*pusH = m_usRawStoreHeight[ubRawId];
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_GetGrabRange
//  Description : The function get raw store grab range.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_GetGrabRange(MMP_UBYTE ubRawId, MMP_USHORT* pusW, MMP_USHORT* pusH)
{
	*pusW = m_usRawStoreGrabW[ubRawId];
	*pusH = m_usRawStoreGrabH[ubRawId];
	
	return MMP_ERR_NONE;
}

#if 0
void ____RAW_YUV_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_DmaDeInterlace
//  Description :
//------------------------------------------------------------------------------
void CallbackFunc_DmaDeInterlace(void *argu)
{
#if (TVDEC_SNR_USE_DMA_DEINTERLACE)
    m_ulDmaDoneCnt++;
    
    if (m_ulDmaDoneCnt == 2) {
        /* Reset Dma Counter */
        m_ulDmaDoneCnt = 0;
        MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_TRIGGER_GRA_TO_PIPE, MMPF_OS_FLAG_SET);
    }
#endif
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStoreFifoFullForYUV420Fmt
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStoreFifoFullForYUV420Fmt(void* pArg)
{
    RTNA_DBG_Str(0, "RAWS PLANE FULL\r\n");
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStoreDoneForYUV420Fmt
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStoreDoneForYUV420Fmt(void* pArg)
{
    MMP_UBYTE               *pubRawId   = (MMP_UBYTE*)pArg;
    MMP_UBYTE               ubRawId     = *pubRawId; // Sensor1->VIF0->Raw0

#if (TVDEC_SNR_USE_DMA_DEINTERLACE)

    // TBD

#else
    AITPS_RAWPROC 	        pRAW        = AITC_BASE_RAWPROC;
    MMP_GRAPHICS_BUF_ATTR	srcBuf      = {0, }, dstBuf;
	MMP_GRAPHICS_RECT		srcRect     = {0, };

    srcBuf.usWidth 		    = m_usRawStoreWidth[ubRawId];
    #if (BIND_SENSOR_BIT1603) // Only 790x242 pixels are effective
    srcBuf.usWidth 		    = m_usRawStoreWidth[ubRawId]-2;
    #endif
	srcBuf.usHeight		    = m_usRawStoreHeight[ubRawId];
	srcBuf.usLineOffset	    = srcBuf.usWidth;
	srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV420;

	if (ubRawId == MMP_RAW_MDL_ID0) {
        srcBuf.ulBaseAddr   = pRAW->RAWPROC_S_ADDR_PLANE0;
        srcBuf.ulBaseUAddr  = pRAW->RAWPROC_S_ADDR_PLANE1;
        srcBuf.ulBaseVAddr  = pRAW->RAWPROC_S_ADDR_PLANE2;
    }
	else if (ubRawId == MMP_RAW_MDL_ID1) {
        srcBuf.ulBaseAddr   = pRAW->RAWPROC1_S_ADDR_PLANE0;
        srcBuf.ulBaseUAddr  = pRAW->RAWPROC1_S_ADDR_PLANE1;
        srcBuf.ulBaseVAddr  = pRAW->RAWPROC1_S_ADDR_PLANE2;
    }
    
    dstBuf.ulBaseAddr 	    = (dstBuf.ulBaseUAddr = (dstBuf.ulBaseVAddr = 0));
    
	srcRect.usLeft			= 0;
	srcRect.usTop			= 0;
	srcRect.usWidth 		= srcBuf.usWidth;
	srcRect.usHeight		= srcBuf.usHeight;
    
    MMPF_Graphics_SetScaleAttr(&srcBuf, &srcRect, 1);
    MMPF_Graphics_SetDelayType(MMP_GRAPHICS_DELAY_CHK_SCA_BUSY);
    MMPF_Graphics_SetPixDelay(10, 20);
	MMPF_Graphics_SetLineDelay(0);

    MMPF_Graphics_ScaleStart(&srcBuf, &dstBuf, NULL, NULL);
#endif

    /* Update store address after graphics starting */
    MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);
    MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE0);

    MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE1, 1);
    MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE1);

    MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE2, 1);
    MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE2);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetInterruptForYUV420Fmt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetInterruptForYUV420Fmt(MMP_UBYTE ubRawId)
{
    m_ubYUVRawId = ubRawId;

	/* Enable Interrupt */
    MMPF_RAWPROC_EnableInterrupt(ubRawId,
    							 MMP_RAW_STORE_PLANE0,
    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
    							 MMP_TRUE);
    							 
    MMPF_RAWPROC_EnableInterrupt(ubRawId,
    							 MMP_RAW_STORE_PLANE1,
    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
    							 MMP_TRUE);

    MMPF_RAWPROC_EnableInterrupt(ubRawId,
    							 MMP_RAW_STORE_PLANE2,
    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
    							 MMP_TRUE);

    /* Register Callback Function */
    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE0,
    								  MMP_RAW_EVENT_STORE_DONE,
    								  CallbackFunc_RawStoreDoneForYUV420Fmt,
    								  (void*)&m_ubYUVRawId);

    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE0,
    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    								  CallbackFunc_RawStoreFifoFullForYUV420Fmt,
    								  (void*)&m_ubYUVRawId);

    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE1,
    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    								  CallbackFunc_RawStoreFifoFullForYUV420Fmt,
    								  (void*)&m_ubYUVRawId);
    								  
    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE2,
    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    								  CallbackFunc_RawStoreFifoFullForYUV420Fmt,
    								  (void*)&m_ubYUVRawId);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStoreFifoFullForYUV422Fmt
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStoreFifoFullForYUV422Fmt(void* pArg)
{
    RTNA_DBG_Str(0, "RAWS PLANE FULL\r\n");
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStoreDoneForYUV422Fmt
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStoreDoneForYUV422Fmt(void* pArg)
{
    MMP_UBYTE   *pubRawId   = (MMP_UBYTE*)pArg;
    MMP_UBYTE   ubRawId     = *pubRawId; // Sensor1->VIF0->Raw0

    #if (TVDEC_SNR_USE_DMA_DEINTERLACE)
    MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_TRIGGER_DEINTERLACE, MMPF_OS_FLAG_SET);
    #else
    MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_TRIGGER_GRA_TO_PIPE, MMPF_OS_FLAG_SET);
    #endif

    /* Update store address after graphics starting */
    MMPF_RAWPROC_UpdateStoreBufIndex(ubRawId, MMP_RAW_STORE_PLANE0, 1);
    MMPF_RAWPROC_UpdateStoreAddr(ubRawId, MMP_RAW_STORE_PLANE0);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetInterruptForYUV422Fmt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetInterruptForYUV422Fmt(MMP_UBYTE ubRawId)
{
    m_ubYUVRawId = ubRawId;

	/* Enable Interrupt */
    MMPF_RAWPROC_EnableInterrupt(ubRawId,
    							 MMP_RAW_STORE_PLANE0,
    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
    							 MMP_TRUE);
    							 
    /* Register Callback Function */
    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE0,
    								  MMP_RAW_EVENT_STORE_DONE,
    								  CallbackFunc_RawStoreDoneForYUV422Fmt,
    								  (void*)&m_ubYUVRawId);

    MMPF_RAWPROC_RegisterIntrCallBack(ubRawId,
    								  MMP_RAW_STORE_PLANE0,
    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    								  CallbackFunc_RawStoreFifoFullForYUV422Fmt,
    								  (void*)&m_ubYUVRawId);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetDeInterlaceBuf
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetDeInterlaceBuf(MMP_ULONG ulYAddr, MMP_ULONG ulUAddr, MMP_ULONG ulVAddr)
{
#if (TVDEC_SNR_USE_DMA_DEINTERLACE)
    m_ulDeInterlaceYBuf = ulYAddr;
    m_ulDeInterlaceUBuf = ulUAddr;
    m_ulDeInterlaceVBuf = ulVAddr;
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_TriggerGraToPipe
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_TriggerGraToPipe(MMP_UBYTE ubRawId)
{
    MMP_GRAPHICS_BUF_ATTR	srcBuf  = {0, }, dstBuf;
	MMP_GRAPHICS_RECT		srcRect = {0, };

#if (TVDEC_SNR_USE_DMA_DEINTERLACE)
    
    srcBuf.usWidth 		    = m_usRawStoreWidth[ubRawId];
    srcBuf.usHeight		    = m_usRawStoreHeight[ubRawId] * 2;
	srcBuf.usLineOffset	    = srcBuf.usWidth * 2;
	srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY;
    #if (BIND_SENSOR_BIT1603)
    srcBuf.usLineOffset     = srcBuf.usWidth;
	srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU;
    #endif
    srcBuf.ulBaseAddr       = m_ulDeInterlaceYBuf;
    srcBuf.ulBaseUAddr      = 0;
    srcBuf.ulBaseVAddr      = 0;

#else

    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    srcBuf.usWidth 		    = m_usRawStoreWidth[ubRawId];
    srcBuf.usHeight		    = m_usRawStoreHeight[ubRawId];
	srcBuf.usLineOffset	    = srcBuf.usWidth * 2;
	srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY;
    #if (BIND_SENSOR_BIT1603)
    srcBuf.usLineOffset     = srcBuf.usWidth;
	srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY;
    #endif
    
    if (ubRawId == MMP_RAW_MDL_ID0) {
        srcBuf.ulBaseAddr   = pRAW->RAWPROC_S_ADDR_PLANE0;
    }
    else if (ubRawId == MMP_RAW_MDL_ID1) {
        srcBuf.ulBaseAddr   = pRAW->RAWPROC1_S_ADDR_PLANE0;
    }
    srcBuf.ulBaseUAddr      = 0;
    srcBuf.ulBaseVAddr      = 0;
#endif
    
    dstBuf.ulBaseAddr 	    = (dstBuf.ulBaseUAddr = (dstBuf.ulBaseVAddr = 0));
    
	srcRect.usLeft			= 0;
	srcRect.usTop			= 0;
	srcRect.usWidth 		= srcBuf.usWidth;
	srcRect.usHeight		= srcBuf.usHeight;
    
    MMPF_Graphics_SetScaleAttr(&srcBuf, &srcRect, 1);
    MMPF_Graphics_SetDelayType(MMP_GRAPHICS_DELAY_CHK_SCA_BUSY);
    MMPF_Graphics_SetPixDelay(10, 20);
	MMPF_Graphics_SetLineDelay(0);

    MMPF_Graphics_ScaleStart(&srcBuf, &dstBuf, NULL, NULL);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_DmaDeInterlace
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_DmaDeInterlace(MMP_UBYTE ubRawId)
{
#if (TVDEC_SNR_USE_DMA_DEINTERLACE)

    #if (TVDEC_SNR_USE_VIF_CNT_AS_FIELD_CNT)
    extern MMP_ULONG    m_ulVIFFrameCount[];
    MMP_ULONG           ulFieldCnt  = 1;
    #else
    static MMP_ULONG    ulFieldCnt  = 1;
    #endif
    AITPS_RAWPROC       pRAW        = AITC_BASE_RAWPROC;
    MMP_ULONG 		    ulSrcAddr;
    MMP_ULONG 		    ulDstAddr;
    MMP_ULONG 		    ulCount;
    MMP_DMA_LINE_OFST   LineOfstArg;
    
    #if (TVDEC_SNR_USE_VIF_CNT_AS_FIELD_CNT)
    ulFieldCnt = m_ulVIFFrameCount[ubRawId];
    #endif

    if (ubRawId == MMP_RAW_MDL_ID0) {
        ulSrcAddr = pRAW->RAWPROC_S_ADDR_PLANE0;
    }
    else if (ubRawId == MMP_RAW_MDL_ID1) {
        ulSrcAddr = pRAW->RAWPROC1_S_ADDR_PLANE0;
    }
    
    if (ulFieldCnt % 2 == 1) {
        ulDstAddr = m_ulDeInterlaceYBuf;
    }
    else if (ulFieldCnt % 2 == 0) {
        ulDstAddr = m_ulDeInterlaceYBuf + m_usRawStoreWidth[ubRawId];
    }
    
    ulCount = m_usRawStoreWidth[ubRawId] * m_usRawStoreHeight[ubRawId];
    
    LineOfstArg.ulSrcWidth  = m_usRawStoreWidth[ubRawId];
    LineOfstArg.ulSrcOffset = m_usRawStoreWidth[ubRawId];
    LineOfstArg.ulDstWidth  = m_usRawStoreWidth[ubRawId];
    LineOfstArg.ulDstOffset = m_usRawStoreWidth[ubRawId] * 2;
    
    MMPF_DMA_MoveData(ulSrcAddr, ulDstAddr, ulCount, CallbackFunc_DmaDeInterlace, 0, MMP_TRUE, &LineOfstArg);
    
    #if (TVDEC_SNR_USE_VIF_CNT_AS_FIELD_CNT == 0)
    ulFieldCnt++;
    #endif
#endif
    return MMP_ERR_NONE;
}

#if 0
void ____RAW_HDR_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStorePlane0FifoFull
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStorePlane0FifoFull(void* pArg)
{
    RTNA_DBG_Str(0, "RAWS0 P0 FULL\r\n");
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawStorePlane1FifoFull
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawStorePlane1FifoFull(void* pArg)
{
	if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
    	RTNA_DBG_Str(0, "RAWS0 P1 FULL\r\n");
    }
    else {
    	RTNA_DBG_Str(0, "RAWS1 P0 FULL\r\n");
    }
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawGrabDoneVC0ForHDR
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawGrabDoneVC0ForHDR(void* pArg)
{
	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return;
	}

	if (gsHdrCfg.ubMode == HDR_MODE_STAGGER && gsHdrCfg.bRawGrabEnable)
	{
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {

			if (m_bDoCaptureFromRawPath) {
				return;
			}

			MMPF_RAWPROC_UpdateStoreBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);

			MMPF_HDR_UpdateStoreFrameAddr(MMP_RAW_VC_SRC_VC0, 0, m_usRawStoreGrabW[m_ubHdrMainRawId], m_usRawStoreGrabH[m_ubHdrMainRawId]);

			MMPF_HDR_UpdateStoreFrameAddr(MMP_RAW_VC_SRC_VC1, 0, m_usRawStoreGrabW[m_ubHdrMainRawId], m_usRawStoreGrabH[m_ubHdrMainRawId]);

			MMPF_HDR_UpdateFetchFrameAddr();

			MMPF_RAWPROC_UpdateFetchBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);

		    if (!MMPF_RAWPROC_IsFetchBusy()) {

		        MMPF_RAWPROC_FetchData (m_ubHdrSensorId,
		                                m_ubHdrMainRawId,
		                            	MMP_RAW_FETCH_NO_ROTATE,
		                            	MMP_RAW_FETCH_NO_MIRROR);
		    }
		}
		else {
		
			MMPF_RAWPROC_UpdateStoreBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);

			MMPF_HDR_UpdateStoreFrameAddr(MMP_RAW_VC_SRC_VC0, 0, m_usRawStoreGrabW[m_ubHdrMainRawId], m_usRawStoreGrabH[m_ubHdrMainRawId]);
		}
	}
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_RawGrabDoneVC1ForHDR
//  Description : 
//------------------------------------------------------------------------------
void CallbackFunc_RawGrabDoneVC1ForHDR(void* pArg)
{
	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return;
	}

	if (gsHdrCfg.ubMode == HDR_MODE_STAGGER && gsHdrCfg.bRawGrabEnable)
	{
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
			// NOP
		}
		else {

			MMPF_HDR_UpdateStoreFrameAddr(MMP_RAW_VC_SRC_VC1, 0, m_usRawStoreGrabW[m_ubHdrMainRawId], m_usRawStoreGrabH[m_ubHdrMainRawId]);

			MMPF_HDR_UpdateFetchFrameAddr();

			MMPF_RAWPROC_UpdateFetchBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);

		    if (!MMPF_RAWPROC_IsFetchBusy()) {

		        MMPF_RAWPROC_FetchData (m_ubHdrSensorId,
		                                m_ubHdrMainRawId,
		                            	MMP_RAW_FETCH_NO_ROTATE,
		                            	MMP_RAW_FETCH_NO_MIRROR);
		    }
		}
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_InitModule
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_InitModule(MMP_USHORT usSubW, MMP_USHORT usSubH)
{
    AITPS_HDR pHDR = AITC_BASE_HDR;
    const MMP_USHORT usSeqModeMaxFrmW       = 2624;
    const MMP_USHORT usStaggerModeMaxFrmW   = 2432;

	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}

	pHDR->HDR_CTL = HDR_FRAME_IDX_EN | HDR_ENABLE | HDR_FS_CHANGE_FRAME_IDX;
	
	#if 0 // Init in ISP IQ Excel
	pHDR->HDR_THD_PARAM         = 0x06;
	pHDR->HDR_GAP_PARAM         = 0x06;
	pHDR->HDR_RATIO_PARAM       = 1*4096;

	pHDR->HDR_MAX_X_THD_PARAM   = 0x3FF;
	pHDR->HDR_MAX_Y_THD_PARAM   = 0x3FF;
	#endif
    
    /* Check the HDR max frame width */
    if (gsHdrCfg.ubMode == HDR_MODE_STAGGER) {
        if (usSubW > usStaggerModeMaxFrmW)
            RTNA_DBG_Str(0, "Warning:Exceed Max HDR Width1\r\n");
    }
    else if (gsHdrCfg.ubMode == HDR_MODE_SEQUENTIAL) {
        if (usSubW > usSeqModeMaxFrmW)
            RTNA_DBG_Str(0, "Warning:Exceed Max HDR Width2\r\n");
    }

	pHDR->HDR_MERGE_WIDTH 		= usSubW;
	
	/* Update Fetch Frame Sub Width/Height */
	m_ulMergeFrameSubW 			= usSubW;
	m_ulMergeFrameSubH 			= usSubH;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_UnInitModule
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_UnInitModule(MMP_UBYTE ubSnrSel)
{
    AITPS_ISP 	pISP 	= AITC_BASE_ISP;
	AITPS_HDR 	pHDR 	= AITC_BASE_HDR;
    AITPS_VIF 	pVIF 	= AITC_BASE_VIF;
    AITPS_MIPI  pMIPI	= AITC_BASE_MIPI;
    MMP_UBYTE	i 		= 0;
    MMP_UBYTE	ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);
    	
	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}
	
	/* Disable Interrupt */
	if (gsHdrCfg.bRawGrabEnable)
	{
	    MMPF_RAWPROC_EnableInterrupt(m_ubHdrMainRawId,
	    							 MMP_RAW_STORE_PLANE0,
	    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
	    							 MMP_FALSE);
	    							 
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
		    MMPF_RAWPROC_EnableInterrupt(m_ubHdrMainRawId,
		    							 MMP_RAW_STORE_PLANE1,
		    							 BAYER_STORE_DONE_PLANE1 | BAYER_STORE_FIFO_FULL_PLANE1,
		    							 MMP_FALSE);
	    }
		else {
		    MMPF_RAWPROC_EnableInterrupt(m_ubHdrSubRawId,
		    							 MMP_RAW_STORE_PLANE0,
		    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
		    							 MMP_FALSE);
		}
    }
    
    /* Reset ISP/HDR setting */
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_BAYER, MMP_TRUE);
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_ISP, 	 MMP_TRUE);

    pISP->ISP_COLOR_SYS_CTL &= ~(ISP_RAW_FETCH_FRM_SYNC_EN);
	pHDR->HDR_CTL = 0;

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_BAYER, MMP_FALSE);
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_ISP,   MMP_FALSE);
    
    /* Reset VIF/MIPI setting */
  	pVIF->VIF_IMG_BUF_EN[ubVifId] 		= VIF_TO_ISP_IMG_BUF_EN;

    pMIPI->MIPI_VC_CTL[ubVifId] 		= 0;
    pMIPI->MIPI_VC2ISP_CTL[ubVifId] 	= MIPI_VC2ISP_CH_EN_MASK;

    for (i = 0; i < MAX_MIPI_DATA_LANE_NUM; i++) {
	    pMIPI->DATA_LANE[i].MIPI_DATA_DELAY[ubVifId] &= ~(MIPI_DATA_RECOVERY);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_InitRawStoreSetting
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_InitRawStoreSetting(MMP_UBYTE ubSnrSel)
{
	AITPS_RAWPROC       pRAW = AITC_BASE_RAWPROC;
	MMP_UBYTE           ubMciByteCnt = MMPF_MCI_BYTECNT_SEL_128BYTE;
    MMP_RAW_COLORFMT    sBitMode = MMP_RAW_COLORFMT_BAYER8;

	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}
	
	m_ubHdrSensorId = ubSnrSel; 

	if (gsHdrCfg.bRawGrabEnable)
	{
		/* Enable Interrupt */
	    MMPF_RAWPROC_EnableInterrupt(m_ubHdrMainRawId,
	    							 MMP_RAW_STORE_PLANE0,
	    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
	    							 MMP_TRUE);
	    							 
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
		    MMPF_RAWPROC_EnableInterrupt(m_ubHdrMainRawId,
		    							 MMP_RAW_STORE_PLANE1,
		    							 BAYER_STORE_DONE_PLANE1 | BAYER_STORE_FIFO_FULL_PLANE1,
		    							 MMP_TRUE);
	    }
		else {
		    MMPF_RAWPROC_EnableInterrupt(m_ubHdrSubRawId,
		    							 MMP_RAW_STORE_PLANE0,
		    							 BAYER_STORE_DONE_PLANE0 | BAYER_STORE_FIFO_FULL_PLANE0,
		    							 MMP_TRUE);
		}

	    /* Register Callback Function */
	    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrMainRawId,
	    								  MMP_RAW_STORE_PLANE0,
	    								  MMP_RAW_EVENT_STORE_DONE,
	    								  CallbackFunc_RawGrabDoneVC0ForHDR,
	    								  NULL);

	    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrMainRawId,
	    								  MMP_RAW_STORE_PLANE0,
	    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
	    								  CallbackFunc_RawStorePlane0FifoFull,
	    								  NULL);

	    if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
		    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrMainRawId,
		    								  MMP_RAW_STORE_PLANE1,
		    								  MMP_RAW_EVENT_STORE_DONE,
		    								  CallbackFunc_RawGrabDoneVC1ForHDR,
		    								  NULL);

    	    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrMainRawId,
    	    								  MMP_RAW_STORE_PLANE1,
    	    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    	    								  CallbackFunc_RawStorePlane1FifoFull,
    	    								  NULL);
	   	}
	   	else {
		    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrSubRawId,
		    								  MMP_RAW_STORE_PLANE0,
		    								  MMP_RAW_EVENT_STORE_DONE,
		    								  CallbackFunc_RawGrabDoneVC1ForHDR,
		    								  NULL);

    	    MMPF_RAWPROC_RegisterIntrCallBack(m_ubHdrSubRawId,
    	    								  MMP_RAW_STORE_PLANE0,
    	    								  MMP_RAW_EVENT_STORE_FIFO_FULL,
    	    								  CallbackFunc_RawStorePlane1FifoFull,
    	    								  NULL);
	    }

	    /* Set Grab Range */
	    {
	    	MMP_ULONG   ulVifGrabW, ulVifGrabH;
            MMP_UBYTE   ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel); 
            
			MMPF_VIF_GetGrabResolution(ubVifId, &ulVifGrabW, &ulVifGrabH);

			/* Reduce some lines to prevent store FIFO full case */
		    MMPF_RAWPROC_SetRawStoreGrabRange(m_ubHdrMainRawId,
		    								  MMP_RAW_STORE_PLANE0,
		    								  MMP_TRUE, 
		    								  1, 1, 
											  ulVifGrabW, 
											  ulVifGrabH);

		    if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
			    MMPF_RAWPROC_SetRawStoreGrabRange(m_ubHdrMainRawId,
			    								  MMP_RAW_STORE_PLANE1,
			    								  MMP_TRUE, 
			    								  1, 1, 
												  ulVifGrabW, 
												  ulVifGrabH);
		    }
		    else {
			    MMPF_RAWPROC_SetRawStoreGrabRange(m_ubHdrSubRawId,
			    								  MMP_RAW_STORE_PLANE0,
			    								  MMP_TRUE, 
			    								  1, 1, 
												  ulVifGrabW, 
												  ulVifGrabH);
		    }
	    }

	    /* Modify the Fetch Range */
	    MMPF_RAWPROC_SetFetchRange( 0,
	                                0,
	                                m_usRawStoreGrabW[m_ubHdrMainRawId]*2,
	                                m_usRawStoreGrabH[m_ubHdrMainRawId],
	                                m_usRawStoreGrabW[m_ubHdrMainRawId]*2);
	    
	    /* Modify the HDR Output Range */
	    MMPF_HDR_InitModule(m_usRawStoreGrabW[m_ubHdrMainRawId], 
	    					m_usRawStoreGrabH[m_ubHdrMainRawId]);

		/* Set Fetch Address First */
		if (gsHdrCfg.ubMode == HDR_MODE_SEQUENTIAL)
		{
			m_ulMergeFrameAddr0 = pRAW->RAWPROC_S_ADDR_PLANE0 = ALIGN256(m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]);
		}
		else if (gsHdrCfg.ubMode == HDR_MODE_STAGGER)
		{
			if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
				if (m_ubHdrMainRawId == MMP_RAW_MDL_ID0)
					pRAW->RAWPROC_S_ADDR_PLANE1 = pRAW->RAWPROC_S_ADDR_PLANE0;
				else
					pRAW->RAWPROC1_S_ADDR_PLANE1 = pRAW->RAWPROC1_S_ADDR_PLANE0;
			}
			else {
				pRAW->RAWPROC1_S_ADDR_PLANE0 = pRAW->RAWPROC_S_ADDR_PLANE0;
			}
		}
	}
	
	/* Set Bit Mode */
	if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT) {
	    sBitMode = MMP_RAW_COLORFMT_BAYER10;
	}
	else {
	    sBitMode = MMP_RAW_COLORFMT_BAYER8;
	}
	
	if (gsHdrCfg.ubMode == HDR_MODE_SEQUENTIAL)
	{
		MMPF_RAWPROC_SetStoreBitMode(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, sBitMode);
	}
	else if (gsHdrCfg.ubMode == HDR_MODE_STAGGER)
	{
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
			MMPF_RAWPROC_SetStoreBitMode(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, sBitMode);
			MMPF_RAWPROC_SetStoreBitMode(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1, sBitMode);
		}
		else {
			MMPF_RAWPROC_SetStoreBitMode(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, sBitMode);
			MMPF_RAWPROC_SetStoreBitMode(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0, sBitMode);
		}
	}
	
	/* Set Store MCI Byte Count */
	ubMciByteCnt = MMPF_MCI_BYTECNT_SEL_128BYTE; // Use 128ByteCount to prevent store Fifo full.
	
	MMPF_RAWPROC_SetStoreByteCount(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, ubMciByteCnt);

	if (gsHdrCfg.ubMode == HDR_MODE_STAGGER)
	{
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
			MMPF_RAWPROC_SetStoreByteCount(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1, ubMciByteCnt);
		}
		else {
			MMPF_RAWPROC_SetStoreByteCount(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0, ubMciByteCnt);
		}
	}

	/* Set Virtual Channel */
	if (gsHdrCfg.ubMode == HDR_MODE_STAGGER)
	{
		if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
			MMPF_RAWPROC_SetStoreVCSelect(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE, MMP_RAW_VC_SRC_VC0);
			MMPF_RAWPROC_SetStoreVCSelect(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1, MMP_TRUE, MMP_RAW_VC_SRC_VC1);
		}
		else {
			MMPF_RAWPROC_SetStoreVCSelect(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE, MMP_RAW_VC_SRC_VC0);
			MMPF_RAWPROC_SetStoreVCSelect(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE, MMP_RAW_VC_SRC_VC1);
		}
	}
	
	/* Set Store Frame Sync */
	MMPF_RAWPROC_SetStoreFrameSync(m_ubHdrMainRawId, MMP_TRUE);
	MMPF_RAWPROC_SetStoreFrameSync(m_ubHdrSubRawId, MMP_TRUE);
	
	/* Set Store Ring Enable (Buffer Protection)*/
	if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
		if (m_ubHdrMainRawId == MMP_RAW_MDL_ID0) {
    		pRAW->RAWPROC_S_END_ADDR_PLANE0 = m_ulHdrBufferEnd - (m_usRawStoreGrabW[m_ubHdrMainRawId] * m_usRawStoreGrabH[m_ubHdrMainRawId]);
    		pRAW->RAWPROC_S_END_ADDR_PLANE1 = m_ulHdrBufferEnd;
    	}
    	else {
    		pRAW->RAWPROC1_S_END_ADDR_PLANE0 = m_ulHdrBufferEnd - (m_usRawStoreGrabW[m_ubHdrMainRawId] * m_usRawStoreGrabH[m_ubHdrMainRawId]);
    		pRAW->RAWPROC1_S_END_ADDR_PLANE1 = m_ulHdrBufferEnd;
    	}
    	MMPF_RAWPROC_EnableRingStore(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE);
    	MMPF_RAWPROC_EnableRingStore(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE1, MMP_TRUE);
	}
	else {
    	pRAW->RAWPROC_S_END_ADDR_PLANE0 = m_ulHdrBufferEnd - (m_usRawStoreGrabW[m_ubHdrMainRawId] * m_usRawStoreGrabH[m_ubHdrMainRawId]);
    	pRAW->RAWPROC1_S_END_ADDR_PLANE0 = m_ulHdrBufferEnd;
    	MMPF_RAWPROC_EnableRingStore(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE);
    	MMPF_RAWPROC_EnableRingStore(m_ubHdrSubRawId, MMP_RAW_STORE_PLANE0, MMP_TRUE);
	}
	
	#if 0
	/* Set Store Urgent Enable */
	MMPF_MCI_SetUrgentEnable(MMPF_MCI_SRC_RAWS0, MMP_TRUE);
	
	pRAW->RAWPROC_S_URGENT_CTL[0] = RAWPROC_S_URGENT_MODE_EN | RAWPROC_S_URGENT_THD(0x40);
	pRAW->RAWPROC_S_URGENT_CTL[1] = RAWPROC_S_URGENT_MODE_EN | RAWPROC_S_URGENT_THD(0x40);
	#endif
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_UpdateStoreFrameAddr
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_UpdateStoreFrameAddr(MMP_UBYTE ubVcChannel, MMP_UBYTE ubFrameIdx, MMP_USHORT usFrameW, MMP_USHORT usFrameH)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}

	/* If enable raw store grab function, 
	 * the store address should be 256 byte aligment.
	 * To be Check! 
	 */
	if (gsHdrCfg.ubMode == HDR_MODE_SEQUENTIAL)
	{
		if (ubFrameIdx == 0) {
			m_ulMergeFrameAddr0 = pRAW->RAWPROC_S_ADDR_PLANE0 = ALIGN256(m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]);
		}
		else if (ubFrameIdx == 1) { 
			if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT)
				m_ulMergeFrameAddr1 = pRAW->RAWPROC_S_ADDR_PLANE0 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH * 4 / 3);
			else
				m_ulMergeFrameAddr1 = pRAW->RAWPROC_S_ADDR_PLANE0 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH);
		}
	}
	else if (gsHdrCfg.ubMode == HDR_MODE_STAGGER)
	{
		if (ubVcChannel == MMP_RAW_VC_SRC_VC0) {
			if (m_ubHdrMainRawId == MMP_RAW_MDL_ID0) {
				m_ulMergeFrameAddr0 = pRAW->RAWPROC_S_ADDR_PLANE0 = ALIGN256(m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]);
			}
			else {
				m_ulMergeFrameAddr0 = pRAW->RAWPROC1_S_ADDR_PLANE0 = ALIGN256(m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]);
			}
		}
		else if (ubVcChannel == MMP_RAW_VC_SRC_VC1) {

			if (gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE) {
				if (m_ubHdrMainRawId == MMP_RAW_MDL_ID0) {
					if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT)
						m_ulMergeFrameAddr1 = pRAW->RAWPROC_S_ADDR_PLANE1 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH * 4 / 3);
					else
						m_ulMergeFrameAddr1 = pRAW->RAWPROC_S_ADDR_PLANE1 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH);
				}
				else {
					if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT)
						m_ulMergeFrameAddr1 = pRAW->RAWPROC1_S_ADDR_PLANE1 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH * 4 / 3);
					else
						m_ulMergeFrameAddr1 = pRAW->RAWPROC1_S_ADDR_PLANE1 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH);				
				}
			}
			else {
				if (gsHdrCfg.ubRawStoreBitMode == HDR_BITMODE_10BIT)
					m_ulMergeFrameAddr1 = pRAW->RAWPROC1_S_ADDR_PLANE0 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH * 4 / 3);
				else
					m_ulMergeFrameAddr1 = pRAW->RAWPROC1_S_ADDR_PLANE0 = ALIGN256((m_ulRawStoreP0Addr[m_ubHdrMainRawId][m_ulRawStoreIdx[m_ubHdrMainRawId][0]]) + usFrameW * usFrameH);
			}
		}
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_UpdateFetchFrameAddr
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_UpdateFetchFrameAddr(void)
{
	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}

	m_ulFetchFrameAddr0 = m_ulMergeFrameAddr0;
	m_ulFetchFrameAddr1	= m_ulMergeFrameAddr1;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_Preview
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_Preview(MMP_USHORT usFrameW, MMP_USHORT usFrameH)
{
	if (gsHdrCfg.bVidEnable == MMP_FALSE && 
		gsHdrCfg.bDscEnable == MMP_FALSE) {
		return MMP_ERR_NONE;
	}

	if (gsHdrCfg.ubMode == HDR_MODE_SEQUENTIAL)
	{
		static MMP_UBYTE ubFrameIdx = 0;

		if (ubFrameIdx < gsHdrCfg.ubBktFrameNum)
		{
			if (ubFrameIdx == (gsHdrCfg.ubBktFrameNum - 1)) {
				MMPF_HDR_UpdateFetchFrameAddr();
	    		MMPF_RAWPROC_UpdateStoreBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);
	    	}

			/* Update next frame store address */
	    	MMPF_HDR_UpdateStoreFrameAddr(MMP_RAW_STORE_PLANE0, (ubFrameIdx + 1) % gsHdrCfg.ubBktFrameNum, usFrameW, usFrameH);
		
			ubFrameIdx++;
		}

		if (ubFrameIdx == gsHdrCfg.ubBktFrameNum)
		{
	 		MMPF_RAWPROC_UpdateFetchBufIndex(m_ubHdrMainRawId, MMP_RAW_STORE_PLANE0, 1);

	        if (!MMPF_RAWPROC_IsFetchBusy()) {

	            MMPF_RAWPROC_FetchData (m_ubHdrSensorId,
	                                    m_ubHdrMainRawId,
	                                	MMP_RAW_FETCH_NO_ROTATE,
	                                	MMP_RAW_FETCH_NO_MIRROR);
	        	ubFrameIdx = 0;
	        }
	    } 
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HDR_SetBufEnd
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_HDR_SetBufEnd(MMP_ULONG ulEndAddr)
{
	m_ulHdrBufferEnd = ulEndAddr;
	return MMP_ERR_NONE;
}

#if 0
void ____RAW_Buffer_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_ResetBufIndex
//  Description : The function initial raw store/fetch bayer buffer index
//------------------------------------------------------------------------------
void MMPF_RAWPROC_ResetBufIndex(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    m_ulRawStoreIdx[ubRawId][ubPlane] = 0;

    m_ulRawFetchIdx[ubRawId][ubPlane] = (m_ulRawStoreBufNum[ubRawId][ubPlane]) ? 
    									(m_ulRawStoreBufNum[ubRawId][ubPlane] - 1) : (0);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchBufIndex
//  Description : 
//------------------------------------------------------------------------------		
void MMPF_RAWPROC_SetFetchBufIndex(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG ulIndex)
{
	m_ulRawFetchIdx[ubRawId][ubPlane] = ulIndex;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_UpdateStoreBufIndex
//  Description : The function update rawstore buffer bayer index
//------------------------------------------------------------------------------
/** @brief Update rawstore buffer bayer index
@param[in] ulOffset offset of buffer index to advance
*/
void MMPF_RAWPROC_UpdateStoreBufIndex(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG ulOffset)
{
    if (m_ulRawStoreBufNum[ubRawId][ubPlane] == 0) {
        return;
    }

    if (ulOffset >= m_ulRawStoreBufNum[ubRawId][ubPlane]) {
        ulOffset %= m_ulRawStoreBufNum[ubRawId][ubPlane];
    }

    m_ulRawStoreIdx[ubRawId][ubPlane] += ulOffset;
    if (m_ulRawStoreIdx[ubRawId][ubPlane] >= m_ulRawStoreBufNum[ubRawId][ubPlane]) {
        m_ulRawStoreIdx[ubRawId][ubPlane] -= m_ulRawStoreBufNum[ubRawId][ubPlane];
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_UpdateFetchBufIndex
//  Description : The function update rawfetch buffer bayer index
//------------------------------------------------------------------------------
/** @brief Update rawfetch buffer write index
@param[in] ulOffset offset of buffer index to advance
*/
void MMPF_RAWPROC_UpdateFetchBufIndex(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG ulOffset)
{
    if (m_ulRawStoreBufNum[ubRawId][ubPlane] == 0) {
        return;
    }

    if (ulOffset >= m_ulRawStoreBufNum[ubRawId][ubPlane]) {
        ulOffset %= m_ulRawStoreBufNum[ubRawId][ubPlane];
    }

    m_ulRawFetchIdx[ubRawId][ubPlane] += ulOffset;
    if (m_ulRawFetchIdx[ubRawId][ubPlane] >= m_ulRawStoreBufNum[ubRawId][ubPlane]) {
        m_ulRawFetchIdx[ubRawId][ubPlane] -= m_ulRawStoreBufNum[ubRawId][ubPlane];
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetCaptureStoreAddr
//  Description : The function set store/fetch address for JPEG capture
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetCaptureStoreAddr(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

	if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }

    switch(ubRawId)
    {
        case MMP_RAW_MDL_ID0:
        	switch(ubPlane)
        	{
            	case MMP_RAW_STORE_PLANE0:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC_S_ADDR_PLANE0 = m_ulRawStoreP0Addr[ubRawId][0];
            	break;	
            	case MMP_RAW_STORE_PLANE1:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC_S_ADDR_PLANE1 = m_ulRawStoreP1Addr[ubRawId][0];
            	break;
            	case MMP_RAW_STORE_PLANE2:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC_S_ADDR_PLANE2 = m_ulRawStoreP2Addr[ubRawId][0];
            	break;
            	case MMP_RAW_STORE_PLANE3:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC_S_ADDR_PLANE3 = m_ulRawStoreP3Addr[ubRawId][0];
            	break;
        	}
        break;
        case MMP_RAW_MDL_ID1:
        	switch(ubPlane)
        	{
            	case MMP_RAW_STORE_PLANE0:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC1_S_ADDR_PLANE0 = m_ulRawStoreP0Addr[ubRawId][0];
            	break;	
            	case MMP_RAW_STORE_PLANE1:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC1_S_ADDR_PLANE1 = m_ulRawStoreP1Addr[ubRawId][0];
            	break;
            	case MMP_RAW_STORE_PLANE2:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC1_S_ADDR_PLANE2 = m_ulRawStoreP2Addr[ubRawId][0];
            	break;
            	case MMP_RAW_STORE_PLANE3:
            		pRAW->RAWPROC_F_ADDR = pRAW->RAWPROC1_S_ADDR_PLANE3 = m_ulRawStoreP3Addr[ubRawId][0];
            	break;
        	}
        break;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_GetCaptureStoreAddr
//  Description : The function get store address for JPEG capture
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_GetCaptureStoreAddr(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG *ulAddr)
{
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

	if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }

	switch(ubPlane) 
	{
		case MMP_RAW_STORE_PLANE0:
			*ulAddr = m_ulRawStoreP0Addr[ubRawId][0];
			break;
		case MMP_RAW_STORE_PLANE1:
			*ulAddr = m_ulRawStoreP1Addr[ubRawId][0];
			break;
		case MMP_RAW_STORE_PLANE2:
			*ulAddr = m_ulRawStoreP2Addr[ubRawId][0];
			break;
		case MMP_RAW_STORE_PLANE3:
			*ulAddr = m_ulRawStoreP3Addr[ubRawId][0];
			break;
	}
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetRawStoreBuffer
//  Description : 
//------------------------------------------------------------------------------
/** @brief Set one raw store bayer buffer by index
@param[in]  ubRawId  	The Raw module index.
@param[in]  ubPlane  	The Raw plane index.
@param[in]  ubBufIdx  	The i-th bayer buffer
@param[in]  ulAddr  	The buffer address
*/
MMP_ERR MMPF_RAWPROC_SetRawStoreBuffer(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane,
									   MMP_UBYTE ubBufIdx, MMP_ULONG ulAddr)
{
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

	if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
	
	switch(ubPlane) 
	{
		case MMP_RAW_STORE_PLANE0:
			m_ulRawStoreP0Addr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE1:
			m_ulRawStoreP1Addr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE2:
			m_ulRawStoreP2Addr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE3:
			m_ulRawStoreP3Addr[ubRawId][ubBufIdx] = ulAddr;
			break;
	}

    if ((ubBufIdx+1) > m_ulRawStoreBufNum[ubRawId][ubPlane]){
        m_ulRawStoreBufNum[ubRawId][ubPlane] = ubBufIdx + 1;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetRawStoreBufferEnd
//  Description : 
//------------------------------------------------------------------------------
/** @brief Set one raw store bayer buffer end by index
@param[in]  ubRawId  	The Raw module index.
@param[in]  ubPlane  	The Raw plane index.
@param[in]  ubBufIdx  	The i-th bayer buffer
@param[in]  ulAddr  	The buffer address
*/
MMP_ERR MMPF_RAWPROC_SetRawStoreBufferEnd(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane,
										  MMP_UBYTE ubBufIdx, MMP_ULONG ulAddr)
{
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

	if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
	
	switch(ubPlane) 
	{
		case MMP_RAW_STORE_PLANE0:
			m_ulRawStoreP0EndAddr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE1:
			m_ulRawStoreP1EndAddr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE2:
			m_ulRawStoreP2EndAddr[ubRawId][ubBufIdx] = ulAddr;
			break;
		case MMP_RAW_STORE_PLANE3:
			m_ulRawStoreP3EndAddr[ubRawId][ubBufIdx] = ulAddr;
			break;
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_GetCurRawStoreBuf
//  Description : 
//------------------------------------------------------------------------------
MMP_ULONG MMPF_RAWPROC_GetCurRawStoreBuf(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane)
{
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

	if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
	
	switch(ubPlane) 
	{
		case MMP_RAW_STORE_PLANE0:
			return m_ulRawStoreP0Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
			break;
		case MMP_RAW_STORE_PLANE1:
			return m_ulRawStoreP1Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
			break;
		case MMP_RAW_STORE_PLANE2:
			return m_ulRawStoreP2Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
			break;
		case MMP_RAW_STORE_PLANE3:
			return m_ulRawStoreP3Addr[ubRawId][m_ulRawStoreIdx[ubRawId][ubPlane]];
			break;
	}
	
	return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_InitStoreBuffer
//  Description : This function initial buffer address.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_InitStoreBuffer(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG ulAddr)
{
	AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
	
	if (ubRawId == MMP_RAW_MDL_ID0) {
		switch(ubPlane)
		{
			case MMP_RAW_STORE_PLANE0:
				pRAW->RAWPROC_S_ADDR_PLANE0 = ulAddr;
			break;	
			case MMP_RAW_STORE_PLANE1:
				pRAW->RAWPROC_S_ADDR_PLANE1 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE2:
				pRAW->RAWPROC_S_ADDR_PLANE2 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE3:
				pRAW->RAWPROC_S_ADDR_PLANE3 = ulAddr;
			break;
		}
	}		
	else if (ubRawId == MMP_RAW_MDL_ID1) {
		switch(ubPlane)
		{
			case MMP_RAW_STORE_PLANE0:
				pRAW->RAWPROC1_S_ADDR_PLANE0 = ulAddr;
			break;	
			case MMP_RAW_STORE_PLANE1:
				pRAW->RAWPROC1_S_ADDR_PLANE1 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE2:
				pRAW->RAWPROC1_S_ADDR_PLANE2 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE3:
				pRAW->RAWPROC1_S_ADDR_PLANE3 = ulAddr;
			break;
		}
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_InitStoreBufferEnd
//  Description : This function initial buffer end address.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_InitStoreBufferEnd(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_ULONG ulAddr)
{
	AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
	
	if (ubRawId == MMP_RAW_MDL_ID0) {
		switch(ubPlane)
		{
			case MMP_RAW_STORE_PLANE0:
				pRAW->RAWPROC_S_END_ADDR_PLANE0 = ulAddr;
			break;	
			case MMP_RAW_STORE_PLANE1:
				pRAW->RAWPROC_S_END_ADDR_PLANE1 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE2:
				pRAW->RAWPROC_S_END_ADDR_PLANE2 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE3:
				pRAW->RAWPROC_S_END_ADDR_PLANE3 = ulAddr;
			break;
		}
	}		
	else if (ubRawId == MMP_RAW_MDL_ID1) {
		switch(ubPlane)
		{
			case MMP_RAW_STORE_PLANE0:
				pRAW->RAWPROC1_S_END_ADDR_PLANE0 = ulAddr;
			break;	
			case MMP_RAW_STORE_PLANE1:
				pRAW->RAWPROC1_S_END_ADDR_PLANE1 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE2:
				pRAW->RAWPROC1_S_END_ADDR_PLANE2 = ulAddr;
			break;
			case MMP_RAW_STORE_PLANE3:
				pRAW->RAWPROC1_S_END_ADDR_PLANE3 = ulAddr;
			break;
		}
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_CalcBufSize
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_CalcBufSize(MMP_RAW_COLORFMT colorFormat, MMP_USHORT usWidth, MMP_USHORT usHeight, MMP_ULONG* ulSize)
{
    switch(colorFormat)
    {
        case MMP_RAW_COLORFMT_BAYER8:
            *ulSize	= usWidth*usHeight;
        break;
        case MMP_RAW_COLORFMT_BAYER10:
            *ulSize	= ((usWidth*usHeight*4) % 3 == 0)?(usWidth*usHeight*4/3):((usWidth*usHeight*4/3) + 1);
        break;
        case MMP_RAW_COLORFMT_BAYER12:
            *ulSize	= ((usWidth*usHeight*8) % 5 == 0)?(usWidth*usHeight*8/5):((usWidth*usHeight*8/5) + 1);
        break;
        case MMP_RAW_COLORFMT_BAYER14:
            //TBD
        break;
        case MMP_RAW_COLORFMT_YUV420:
            *ulSize	= usWidth*usHeight*3/2;
        break;
        case MMP_RAW_COLORFMT_YUV422:
            *ulSize	= usWidth*usHeight*2;
        break;
        default:
        	return MMP_ERR_NONE;
        break;
    }
    
    /* For the MCI ByteCount Alignment */
    *ulSize	= ALIGN256(*ulSize);

	return MMP_ERR_NONE;
}

#if 0
void ____RAW_Property_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_CalcRawFetchTiming
//  Description : The function calculate raw fetch timing
//------------------------------------------------------------------------------
/** 
 * @brief Calculate raw fetch timing
 * @param[in]  ubScalerPath: the timing calculation is depended on this path
 */
MMP_ERR MMPF_RAWPROC_CalcRawFetchTiming(MMP_SCAL_PIPEID ubScalerPath)
{
    #if 0 // TBD
    MMP_USHORT	usTempPixDelay = 0;
    MMP_ULONG64	ulTempLineDelay = 0;

    while (1) {
  
        // Zoom 240X need to enlarge the line delay to longlong.
        MMPF_Scaler_GetScaleUpHBlanking(ubScalerPath, (MMP_ULONG64 *)&ulTempLineDelay);

		/**	@brief	for the consideration of the relationship between the line delay and pixel delay. */
		ulTempLineDelay = (ulTempLineDelay + usTempPixDelay) / (usTempPixDelay + 1);

		/**	@brief	rawproc line delay = (value * 16)*pixel delay. Scaler clock = 2* ISP clock. */
        ulTempLineDelay = (ulTempLineDelay + 31) >> 5;

        if (ulTempLineDelay <= RAWPROC_F_MAX_LINE_DELAY)
            break;
      
        usTempPixDelay++;
        
        if (usTempPixDelay > RAWPROC_F_MAX_PIXL_DELAY) {
            usTempPixDelay  = RAWPROC_F_MAX_PIXL_DELAY;
            ulTempLineDelay = RAWPROC_F_MAX_LINE_DELAY;
            break;
        }
    }

    if (ulTempLineDelay < 8) {
        ulTempLineDelay = 8;
	}
	
	MMPF_RAWPROC_SetFetchPixelDelay((MMP_UBYTE)usTempPixDelay);
	MMPF_RAWPROC_SetFetchLineDelay((MMP_USHORT)ulTempLineDelay);
    #endif

	// For zoom from raw path.
	if (gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) {
	    MMPF_RAWPROC_SetFetchPixelDelay(0);
	}
	else {
	    MMPF_RAWPROC_SetFetchPixelDelay(MMPF_RAWPROC_CalcFetchPixelDelay());
	}
	MMPF_RAWPROC_SetFetchLineDelay(0x2);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableRingStore
//  Description : 
//------------------------------------------------------------------------------
/** @brief Enable/Disable raw store ring buffer mode.

The function enable/disable raw store ring buffer mode.
@param[in]  ubRawId    stand for raw module index.
@param[in]  ubPlane    stand for raw plane index.
@param[in]  bRingBufEn stand for use ring buffer or not.
*/
MMP_ERR MMPF_RAWPROC_EnableRingStore(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_BOOL bRingBufEn)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }

    switch(ubRawId)
    {
        case MMP_RAW_MDL_ID0:

            if (bRingBufEn) {
                pRAW->RAWPROC_S_FUNC_CTL1 |= RAWPROC_S_RINGBUF_EN(ubPlane);
            }
            else {
                pRAW->RAWPROC_S_FUNC_CTL1 &= ~(RAWPROC_S_RINGBUF_EN(ubPlane));            
            }

			m_ulRawStoreRingEnable[ubRawId][ubPlane] = bRingBufEn;
        break;
        case MMP_RAW_MDL_ID1:
        
            if (bRingBufEn) {
                pRAW->RAWPROC1_S_FUNC_CTL1 |= RAWPROC_S_RINGBUF_EN(ubPlane);
            }
            else {
                pRAW->RAWPROC1_S_FUNC_CTL1 &= ~(RAWPROC_S_RINGBUF_EN(ubPlane));            
            }
			
			m_ulRawStoreRingEnable[ubRawId][ubPlane] = bRingBufEn;
        break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableDownsample
//  Description : 
//------------------------------------------------------------------------------
/** @brief Enable/Disable raw store downsample

The function enable or disable raw store downsample

@param[in] bEnable  Enable/Disable
@param[in] ulRatio  downsample ratio
*/
MMP_ERR MMPF_RAWPROC_EnableDownsample(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, 
								      MMP_BOOL bEnable,  MMP_UBYTE ubRatio)
{
    AITPS_RAWPROC 	pRAW = AITC_BASE_RAWPROC;
    MMP_UBYTE		ubDnsampRatio;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
    if (bEnable)
    	ubDnsampRatio = RAWPROC_S_DNSAMP_RATIO(ubRatio);
    else
    	ubDnsampRatio = 0;	
    
    switch(ubRawId)
    {
    	case MMP_RAW_MDL_ID0:  	
    		switch(ubPlane)
    		{
    			case MMP_RAW_STORE_PLANE0:
    				pRAW->RAWPROC_S_DNSAMP_H_PLANE0 = pRAW->RAWPROC_S_DNSAMP_V_PLANE0 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE1:
    				pRAW->RAWPROC_S_DNSAMP_H_PLANE1 = pRAW->RAWPROC_S_DNSAMP_V_PLANE1 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE2:
    				pRAW->RAWPROC_S_DNSAMP_H_PLANE2 = pRAW->RAWPROC_S_DNSAMP_V_PLANE2 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE3:
    				pRAW->RAWPROC_S_DNSAMP_H_PLANE3 = pRAW->RAWPROC_S_DNSAMP_V_PLANE3 = ubDnsampRatio;
    			break;
    		}
    	break;
    	case MMP_RAW_MDL_ID1:
    		switch(ubPlane)
    		{
    			case MMP_RAW_STORE_PLANE0:
    				pRAW->RAWPROC1_S_DNSAMP_H_PLANE0 = pRAW->RAWPROC1_S_DNSAMP_V_PLANE0 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE1:
    				pRAW->RAWPROC1_S_DNSAMP_H_PLANE1 = pRAW->RAWPROC1_S_DNSAMP_V_PLANE1 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE2:
    				pRAW->RAWPROC1_S_DNSAMP_H_PLANE2 = pRAW->RAWPROC1_S_DNSAMP_V_PLANE2 = ubDnsampRatio;
    			break;
    			case MMP_RAW_STORE_PLANE3:
    				pRAW->RAWPROC1_S_DNSAMP_H_PLANE3 = pRAW->RAWPROC1_S_DNSAMP_V_PLANE3 = ubDnsampRatio;
    			break;
    		}
    	break;  
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetRawStoreGrabRange
//  Description : This function set Raw store grab range.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw store grab range.
 * 
 *  This function set Raw store grab range.
 * @param[in] ubRawId : stands for Raw module index.
 * @param[in] ubPlane : stands for Raw plane index.
 * @param[in] bEnable : stands for Raw store grab enable.
 * @param[in] usXst   : stands for desired grab x start position.  
 * @param[in] usYst   : stands for desired grab y start position.
 * @param[in] usW     : stands for desired grab width.  
 * @param[in] usH     : stands for desired grab height. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetRawStoreGrabRange(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, 
										  MMP_BOOL  bEnable,
										  MMP_USHORT usXst, MMP_USHORT usYst, 
										  MMP_USHORT usW, MMP_USHORT usH)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (usXst < 1 || usYst < 1 || usW == 0 || usH == 0) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
            pRAW->RAWS0_GRAB[ubPlane].PIXL_ST = usXst;
            pRAW->RAWS0_GRAB[ubPlane].PIXL_ED = usXst + usW - 1;
            pRAW->RAWS0_GRAB[ubPlane].LINE_ST = usYst;    
            pRAW->RAWS0_GRAB[ubPlane].LINE_ED = usYst + usH - 1;

            m_usRawStoreGrabW[ubRawId] = usW;
            m_usRawStoreGrabH[ubRawId] = usH;
			
			if (bEnable) {
				pRAW->RAWPROC_S_FUNC_CTL0 |= RAWPROC_S_GRAB_EN(ubPlane);
            	m_bRawStoreGrabEnable[ubRawId] = MMP_TRUE;
            }
            else {
 				pRAW->RAWPROC_S_FUNC_CTL0 &= ~RAWPROC_S_GRAB_EN(ubPlane);
            	m_bRawStoreGrabEnable[ubRawId] = MMP_FALSE;
            }
  		break;
  		case MMP_RAW_MDL_ID1:
            pRAW->RAWS1_GRAB[ubPlane].PIXL_ST = usXst;
            pRAW->RAWS1_GRAB[ubPlane].PIXL_ED = usXst + usW - 1;
            pRAW->RAWS1_GRAB[ubPlane].LINE_ST = usYst;    
            pRAW->RAWS1_GRAB[ubPlane].LINE_ED = usYst + usH - 1;
            
            m_usRawStoreGrabW[ubRawId] = usW;
            m_usRawStoreGrabH[ubRawId] = usH;
  		
  			if (bEnable) {
				//pRAW->RAWPROC1_S_FUNC_CTL0 |= RAWPROC_S_GRAB_EN(ubPlane);
            	m_bRawStoreGrabEnable[ubRawId] = MMP_TRUE;
            }
            else {
 				pRAW->RAWPROC1_S_FUNC_CTL0 &= ~RAWPROC_S_GRAB_EN(ubPlane);
            	m_bRawStoreGrabEnable[ubRawId] = MMP_FALSE;
            }
  		break;
  	}
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreLineOffset
//  Description : This function set Raw store line offset.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw store line offset.
 * 
 *  This function set Raw store line offset.
 * @param[in] ubRawId  : stands for Raw module index.
 * @param[in] ubPlane  : stands for Raw plane index
 * @param[in] bEnable  : stands for store line offset mode enable.
 * @param[in] ulOffset : stands for store line offset.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreLineOffset(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, 
									    MMP_BOOL bEnable, MMP_ULONG ulOffset)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }

  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
  			if (bEnable) {
  				pRAW->RAWPROC_S_FUNC_CTL2 |= RAWPROC_S_LINEOFST_EN(ubPlane);
  				pRAW->RAWPROC_S_LINEOFST[ubPlane] = ulOffset;
  			}
  			else {
  				pRAW->RAWPROC_S_FUNC_CTL2 &= ~RAWPROC_S_LINEOFST_EN(ubPlane);
  				pRAW->RAWPROC_S_LINEOFST[ubPlane] = 0;
  			}  		
  		break;
  		case MMP_RAW_MDL_ID1:
  			if (bEnable) {
  				pRAW->RAWPROC1_S_FUNC_CTL2 |= RAWPROC_S_LINEOFST_EN(ubPlane);
  				pRAW->RAWPROC1_S_LINEOFST[ubPlane] = ulOffset;
  			}
  			else {
  				pRAW->RAWPROC1_S_FUNC_CTL2 &= ~RAWPROC_S_LINEOFST_EN(ubPlane);
  				pRAW->RAWPROC1_S_LINEOFST[ubPlane] = 0;
  			}
  		break;
  	}
	
    return MMP_ERR_NONE;  
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreVCSelect
//  Description : This function set Raw store virtual channel attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw store line offset.
 * 
 *  This function set Raw store line offset.
 * @param[in] ubRawId : stands for Raw module index.
 * @param[in] ubPlane : stands for Raw plane index. 
 * @param[in] bEnable : stands for store VC enable.
 * @param[in] ubSrc   : stands for store VC selection. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreVCSelect(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, 
									  MMP_BOOL bEnable, MMP_UBYTE ubSrc)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
  			if (bEnable) {
  				pRAW->RAWPROC_S_FUNC_CTL2 |= RAWPROC_S_VC_EN(ubPlane);
  				
  				switch (ubPlane)
  				{
  					case MMP_RAW_STORE_PLANE0:
  						pRAW->RAWPROC_S_VC_SELECT &= ~(RAWPROC_S_PLANE0_SRC_VC_MASK);
  						pRAW->RAWPROC_S_VC_SELECT |= RAWPROC_S_PLANE0_SRC_VC(ubSrc);
  					break;
  					case MMP_RAW_STORE_PLANE1:
  						pRAW->RAWPROC_S_VC_SELECT &= ~(RAWPROC_S_PLANE1_SRC_VC_MASK);
  						pRAW->RAWPROC_S_VC_SELECT |= RAWPROC_S_PLANE1_SRC_VC(ubSrc);
  					break;
  					case MMP_RAW_STORE_PLANE2:
  						pRAW->RAWPROC_S_VC_SELECT &= ~(RAWPROC_S_PLANE2_SRC_VC_MASK);
  						pRAW->RAWPROC_S_VC_SELECT |= RAWPROC_S_PLANE2_SRC_VC(ubSrc);
  					break;
  					case MMP_RAW_STORE_PLANE3:
  						pRAW->RAWPROC_S_VC_SELECT &= ~(RAWPROC_S_PLANE3_SRC_VC_MASK);
  						pRAW->RAWPROC_S_VC_SELECT |= RAWPROC_S_PLANE3_SRC_VC(ubSrc);
  					break;
  				}
  			}
  			else {
  				pRAW->RAWPROC_S_FUNC_CTL2 &= ~RAWPROC_S_VC_EN(ubPlane);
  			}
  		break;
  		case MMP_RAW_MDL_ID1:
  			if (bEnable) {
  				pRAW->RAWPROC1_S_FUNC_CTL2 |= RAWPROC_S_VC_EN(ubPlane);
  				
  				switch (ubPlane)
  				{
  					case MMP_RAW_STORE_PLANE0:
  						pRAW->RAWPROC1_S_VC_SELECT &= ~(RAWPROC_S_PLANE0_SRC_VC_MASK);
  						pRAW->RAWPROC1_S_VC_SELECT |= RAWPROC_S_PLANE0_SRC_VC(ubSrc);
  					break;	
  					case MMP_RAW_STORE_PLANE1:
  						pRAW->RAWPROC1_S_VC_SELECT &= ~(RAWPROC_S_PLANE1_SRC_VC_MASK);
  						pRAW->RAWPROC1_S_VC_SELECT |= RAWPROC_S_PLANE1_SRC_VC(ubSrc);
  					break;	
  					case MMP_RAW_STORE_PLANE2:
  						pRAW->RAWPROC1_S_VC_SELECT &= ~(RAWPROC_S_PLANE2_SRC_VC_MASK);
  						pRAW->RAWPROC1_S_VC_SELECT |= RAWPROC_S_PLANE2_SRC_VC(ubSrc);
  					break;	
  					case MMP_RAW_STORE_PLANE3:
  						pRAW->RAWPROC1_S_VC_SELECT &= ~(RAWPROC_S_PLANE3_SRC_VC_MASK);
  						pRAW->RAWPROC1_S_VC_SELECT |= RAWPROC_S_PLANE3_SRC_VC(ubSrc);
  					break;
  				}
  			}
  			else {
  				pRAW->RAWPROC1_S_FUNC_CTL2 &= ~RAWPROC_S_VC_EN(ubPlane);
  			}
  		break;
  	}
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreByteCount
//  Description : This function set Raw store MCI byte count.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw MCI byte count.
 * 
 *  This function set Raw MCI byte count.
 * @param[in] ubRawId      : stands for Raw module index.
 * @param[in] ubPlane      : stands for Raw plane index. 
 * @param[in] ubByteCntSel : stands for MCI byte count selection.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreByteCount(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_UBYTE ubByteCntSel)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
  		    if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_128BYTE)
  		        pRAW->RAWPROC_S_FUNC_CTL0 &= ~RAWPROC_S_256BYTECNT(ubPlane);
  		    else if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_256BYTE)
                pRAW->RAWPROC_S_FUNC_CTL0 |= RAWPROC_S_256BYTECNT(ubPlane);
  		break;
  		case MMP_RAW_MDL_ID1:
  		    if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_128BYTE)
  		        pRAW->RAWPROC1_S_FUNC_CTL0 &= ~RAWPROC_S_256BYTECNT(ubPlane);
  		    else if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_256BYTE)
                pRAW->RAWPROC1_S_FUNC_CTL0 |= RAWPROC_S_256BYTECNT(ubPlane);
  		break;
  	}
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchByteCount
//  Description : This function set Raw fetch MCI byte count.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw MCI byte count.
 * 
 *  This function set Raw MCI byte count.
 * @param[in] ubByteCntSel : stands for MCI byte count selection.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetFetchByteCount(MMP_UBYTE ubByteCntSel)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_128BYTE)
        pRAW->RAWPROC_F_MODE_CTL &= ~(RAWPROC_F_256BYTECNT_EN);
    else if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_256BYTE)
        pRAW->RAWPROC_F_MODE_CTL |= RAWPROC_F_256BYTECNT_EN;
	
    return MMP_ERR_NONE;  
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreUrgentMode
//  Description : This function set Raw store urgent function.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw MCI urgent function.
 * 
 *  This function set Raw MCI urgent function.
 * @param[in] ubRawId   : stands for Raw module index.
 * @param[in] ubPlane   : stands for Raw plane index.
 * @param[in] bEnable   : stands for MCI urgent mode enable.
 * @param[in] usThd     : stands for MCI urgent mode threshold.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreUrgentMode(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_BOOL bEnable, MMP_USHORT usThd)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
        	if (bEnable) {
        		pRAW->RAWPROC_S_URGENT_CTL[ubPlane] |= RAWPROC_S_URGENT_MODE_EN;
        		pRAW->RAWPROC_S_URGENT_CTL[ubPlane] |= RAWPROC_S_URGENT_THD(usThd);
        	}
            else {
        		pRAW->RAWPROC_S_URGENT_CTL[ubPlane] = 0;
        	}
  		break;
  		case MMP_RAW_MDL_ID1:
        	if (bEnable) {
        		pRAW->RAWPROC1_S_URGENT_CTL[ubPlane] |= RAWPROC_S_URGENT_MODE_EN;
        		pRAW->RAWPROC1_S_URGENT_CTL[ubPlane] |= RAWPROC_S_URGENT_THD(usThd);
        	}
            else {
        		pRAW->RAWPROC1_S_URGENT_CTL[ubPlane] = 0;
        	}
  		break;
  	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchUrgentMode
//  Description : This function set Raw fetch urgent function.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw MCI urgent function.
 * 
 *  This function set Raw MCI urgent function.
 * @param[in] bEnable   : stands for MCI urgent mode enable.
 * @param[in] usThd     : stands for MCI urgent mode threshold.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetFetchUrgentMode(MMP_BOOL bEnable, MMP_USHORT usThd)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;

	if (bEnable) {
		pRAW->RAWPROC_F_URGENT_CTL |= RAWPROC_F_URGENT_MODE_EN;
		pRAW->RAWPROC_F_URGENT_CTL |= RAWPROC_F_URGENT_THD(usThd);
	}
    else {
		pRAW->RAWPROC_F_URGENT_CTL = 0;
	}    

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableFetchBusyMode
//  Description : This function enable Raw fetch busy mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function enable Raw fetch busy mode.
 * 
 *  This function enable Raw fetch busy mode. 
 *  Note that the line delay will be ineffective if enable busy mode.
 * @param[in] bEnable : stands for Raw fetch busy mode enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_EnableFetchBusyMode(MMP_BOOL bEnable)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    if (bEnable)
        pRAW->RAWPROC_F_BUSY_MODE_CTL |= RAWPROC_SCA_BUSY_MODE_EN;
    else
        pRAW->RAWPROC_F_BUSY_MODE_CTL &= ~(RAWPROC_SCA_BUSY_MODE_EN);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFrontFetchTiming
//  Description : This function set raw fetch front line timing.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetFrontFetchTiming(MMP_USHORT usFrontLn, MMP_USHORT usFrontTiming, MMP_USHORT usExtraTiming)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    pRAW->RAWPROC_F_FRONT_LINE_NUM  = usFrontLn;
    pRAW->RAWPROC_F_FRONT_LINE_TIME = usFrontTiming;
    pRAW->RAWPROC_F_EXTRA_LINE_TIME = usExtraTiming;
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableDPCM
//  Description : This function enable raw fetch DPCM mode.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_EnableDPCM(MMP_BOOL bEnable)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
     
    if (bEnable) {
        pRAW->RAWPROC_F_MODE_CTL |= RAWPROC_F_DPCM_MODE_EN;
    }
    else{
        pRAW->RAWPROC_F_MODE_CTL &= ~(RAWPROC_F_DPCM_MODE_EN);
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetRollingOffset
//  Description : This function set raw rolling offset for compensation.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_SetRollingOffset(MMP_UBYTE ubBankIdx, MMP_UBYTE* ubTable)
{
    #define RAW_OFSET_NUM_PER_BANK	(256)
    
    AITPS_RAWPROC 	pRAW 	= AITC_BASE_RAWPROC;
    AIT_REG_B* 		pTable 	= AITC_BASE_RAW_OFST_TBL;
    MMP_ULONG       i = 0;
         
    pRAW->RAWPROC_F_ROLLING_OFST_TBL_BANK_SEL = (ubBankIdx & RAWPROC_F_ROLLING_OFST_TBL_BANK_MASK);
    
    for (i = 0; i < RAW_OFSET_NUM_PER_BANK; i++) {
		pTable[i] = ubTable[i];
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_EnableRollingOfstComp
//  Description : This function enable raw rolling offset compensation.
//------------------------------------------------------------------------------
MMP_ERR MMPF_RAWPROC_EnableRollingOfstComp(MMP_BOOL bEnable, MMP_BOOL bInitZero, MMP_USHORT usStartLine)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
	
	if (bEnable) {
		pRAW->RAWPROC_F_ROLLING_OFST_CTL = RAWPROC_F_ROLLING_OFST_EN;
	}
	else {
		pRAW->RAWPROC_F_ROLLING_OFST_CTL &= ~(RAWPROC_F_ROLLING_OFST_EN);
	}
	
	if (bInitZero) {
		pRAW->RAWPROC_F_ROLLING_OFST_CTL |= RAWPROC_F_ROLLING_OFST_TBL_AUTO_INIT0;
	}
	else {
		pRAW->RAWPROC_F_ROLLING_OFST_CTL &= ~(RAWPROC_F_ROLLING_OFST_TBL_AUTO_INIT0);
	}
	
	pRAW->RAWPROC_F_ROLLING_OFST_START_LINE	= usStartLine;
	  
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreBitMode
//  Description : This function set Raw store bit mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw store bit mode.
 * 
 *  This function set Raw store bit mode.
 * @param[in] ubRawId   : stands for Raw module index.
 * @param[in] ubPlane   : stands for Raw plane index.
 * @param[in] ubBitMode : stands for Raw bit mode selection.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreBitMode(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, 
									 MMP_RAW_COLORFMT colorFormat)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
	AIT_REG_B     *oprBitModePlane0;
 	AIT_REG_B     *oprBitModePlane123;
    
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
     
    if (colorFormat >= MMP_RAW_COLORFMT_YUV420) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
  		case MMP_RAW_MDL_ID0:
  			oprBitModePlane0 	= &pRAW->RAWPROC_MODE_SEL;
			oprBitModePlane123	= &pRAW->RAWPROC_S_BIT_MODE_PLANE123;
  		break;
  		case MMP_RAW_MDL_ID1:
  		    oprBitModePlane0 	= &pRAW->RAWPROC1_MODE_SEL;
			oprBitModePlane123 	= &pRAW->RAWPROC1_S_BIT_MODE_PLANE123;
  		break;
  	}

	if (ubPlane == MMP_RAW_STORE_PLANE0) {
    	*oprBitModePlane0 &= ~(RAWPROC_S_PLANE0_BIT_MODE_MASK);
    	*oprBitModePlane0 |= RAWPROC_S_PLANE0_BIT_MODE(colorFormat);
	}
	else if (ubPlane == MMP_RAW_STORE_PLANE1) {
    	*oprBitModePlane123 &= ~(RAWPROC_S_PLANE1_BIT_MODE_MASK);
    	*oprBitModePlane123 |= RAWPROC_S_PLANE1_BIT_MODE(colorFormat);
	}
	else if (ubPlane == MMP_RAW_STORE_PLANE2) {
    	*oprBitModePlane123 &= ~(RAWPROC_S_PLANE2_BIT_MODE_MASK);
    	*oprBitModePlane123 |= RAWPROC_S_PLANE2_BIT_MODE(colorFormat);
	}
	else if (ubPlane == MMP_RAW_STORE_PLANE3) {
    	*oprBitModePlane123 &= ~(RAWPROC_S_PLANE3_BIT_MODE_MASK);
    	*oprBitModePlane123 |= RAWPROC_S_PLANE3_BIT_MODE(colorFormat);
	}
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetFetchBitMode
//  Description : This function set Raw fetch bit mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw fetch bit mode.
 * 
 *  This function set Raw fetch bit mode.
 * @param[in] ubBitMode : stands for Raw bit mode selection.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetFetchBitMode(MMP_RAW_COLORFMT colorFormat)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
     
    if (colorFormat >= MMP_RAW_COLORFMT_YUV420) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    pRAW->RAWPROC_F_MODE_CTL &= ~(RAWPROC_F_BIT_MODE_MASK);
    pRAW->RAWPROC_F_MODE_CTL |= RAWPROC_F_BIT_MODE(colorFormat);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreFrameSync
//  Description : This function set Raw store OPR frame sync.
//------------------------------------------------------------------------------
/** 
 * @brief This function set Raw store OPR frame sync.
 * 
 *  This function set Raw store OPR frame sync.
 * @param[in] ubRawId   : stands for Raw module index. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreFrameSync(MMP_UBYTE ubRawId, MMP_BOOL bEnable)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
	
	if (ubRawId == MMP_RAW_MDL_ID0) {
		if (bEnable)
    		pRAW->RAWPROC_S_OPR_SYNC_EN |= RAWPROC_S_OPR_FRMSYNC_EN;
    	else
    		pRAW->RAWPROC_S_OPR_SYNC_EN &= ~(RAWPROC_S_OPR_FRMSYNC_EN);	
    }
	else if (ubRawId == MMP_RAW_MDL_ID1) {
		if (bEnable)
    		pRAW->RAWPROC1_S_OPR_SYNC_EN |= RAWPROC_S_OPR_FRMSYNC_EN;
    	else
    		pRAW->RAWPROC1_S_OPR_SYNC_EN &= ~(RAWPROC_S_OPR_FRMSYNC_EN);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RAWPROC_SetStoreYUV420Enable
//  Description : This function set Raw store bit mode.
//------------------------------------------------------------------------------
/** 
 * @brief This function set YUV420 mode.
 * 
 *  This function set YUV420 mode.
 * @param[in] ubRawId   : stands for Raw module index.
 * @param[in] ubPlane   : stands for Raw plane index.
 * @param[in] bEnable   : stand for store YUV420 or not.
 * @return It return the function status.  
 */
MMP_ERR MMPF_RAWPROC_SetStoreYUV420Enable(MMP_UBYTE ubRawId, MMP_UBYTE ubPlane, MMP_BOOL bEnable)
{
    AITPS_RAWPROC pRAW = AITC_BASE_RAWPROC;
    
    if (ubRawId >= MMP_RAW_MDL_NUM) {
        return MMP_RAWPROC_ERR_PARAMETER;
    }

    if (ubPlane >= MMP_RAW_STORE_PLANE_NUM) {
    	return MMP_RAWPROC_ERR_PARAMETER;
    }
    
  	switch(ubRawId)
  	{
        case MMP_RAW_MDL_ID0:
            if (bEnable) {
                pRAW->RAWPROC_S_FUNC_CTL1 |= RAWPROC_S_YUV420_EN(ubPlane);
            }
            else {
                pRAW->RAWPROC_S_FUNC_CTL1 &= ~(RAWPROC_S_YUV420_EN(ubPlane));            
            }
        break;
        case MMP_RAW_MDL_ID1:
            if (bEnable) {
                pRAW->RAWPROC1_S_FUNC_CTL1 |= RAWPROC_S_YUV420_EN(ubPlane);
            }
            else {
                pRAW->RAWPROC1_S_FUNC_CTL1 &= ~(RAWPROC_S_YUV420_EN(ubPlane));            
            }
        break;
  	}
	
    return MMP_ERR_NONE;
}

/** @}*/ //end of MMPF_RAWPROC
