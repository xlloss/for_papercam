//==============================================================================
//
//  File        : mmpf_ldc.c
//  Description : MMPF_LDC functions
//  Author      : Eroy Yang
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
#include "mmp_reg_ldc.h"
#include "mmp_reg_graphics.h"
#include "mmpf_system.h"
#include "mmpf_dma.h"
#include "mmpf_ldc.h"
#include "mmpf_graphics.h"
#include "mmpf_ibc.h"
#include "mmpf_ldc_ctl.h"

#if (CHIP == MCR_V2)

/** @addtogroup MMPF_LDC
@{
*/

#if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
//==============================================================================
//
//                              GLOBAL VARIABLE (Table)
//
//==============================================================================

/* For 1080p Use */
//#include "ldc_LUT_1080p_AR1820_5dist_Mode00.c"	//OK
//#include "ldc_LUT_1080p_AR1820_4dist_Mode11.c"	//OK
#include "ldc_LUT_1080p_AR1820_4dist_Mode10.c"		//OK
//#include "ldc_LUT_1080p_AR1820_3dist_Mode10.c"    //OK
//#include "ldc_LUT_1080p_AR1820_2dist_Mode10.c"    //OK
//#include "ldc_LUT_1080p_AR1820_1dist_Mode10.c"    //OK

//#include "ldc_LUT_1080p_AR1820_TV-4_X1.05.c"		//OK
//#include "ldc_LUT_1080p_AR1820_TV-4_X1.10.c"		//Fail
//#include "ldc_LUT_1080p_AR1820_TV-4_X1.15.c"		//OK
//#include "ldc_LUT_1080p_AR1820_TV-4_X1.20.c"		//OK
//#include "ldc_LUT_1080p_AR1820_3dist_Mode11.c"	//OK

/* For 720p Use */
//#include "ldc_LUT_720p_AR1820_7d5dist_Mode11.c"	//OK
//#include "ldc_LUT_720p_AR1820_5dist_Mode11.c"		//OK
#include "ldc_LUT_720p_AR1820_4dist_Mode10.c"		//OK
//#include "ldc_LUT_720p_AR1820_3dist_Mode10.c"		//OK
//#include "ldc_LUT_720p_AR1820_2dist_Mode10.c"		//OK
//#include "ldc_LUT_720p_AR1820_1dist_Mode10.c"		//OK

/* For WVGA Use */
#include "ldc_LUT_WVGA_AR1820_4dist_Mode10.c"		//OK
//#include "ldc_LUT_WVGA_AR1820_3dist_Mode10.c"		//OK
//#include "ldc_LUT_WVGA_AR1820_2dist_Mode10.c"		//OK
//#include "ldc_LUT_WVGA_AR1820_1dist_Mode10.c"		//OK

/* For Multi-Slice */
#include "ldc_LUT_1536p_AR0330_MultiSlice.c"
#include "ldc_LUT_1080p_AR0330_MultiSlice.c"
#if (LDC_SAVE_BANDWIDTH_VER == 2)
#include "ldc_LUT_736To736p_AR0330_MultiSlice_1123.c"
#elif (LDC_SAVE_BANDWIDTH_VER == 1)
#include "ldc_LUT_1088To736p_AR0330_MultiSlice_1113.c"
#else
#include "ldc_LUT_1536To736p_AR0330_MultiSlice_1111.c"
#endif
#endif //(SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

MMP_LDC_LINK 		m_LdcPathLink 			= {MMP_IBC_PIPE_3, MMP_IBC_PIPE_1, MMP_IBC_PIPE_0, MMP_IBC_PIPE_1, MMP_IBC_PIPE_2};
MMP_UBYTE 			m_ubSwiPipeLbTargetCnt 	= 1; /* This count is not included source pipe */
MMP_UBYTE 			m_ubSwiPipeLbCurCnt 	= 0;
MMP_UBYTE  			m_ubLdcLbSrcPipe 		= MMP_IBC_PIPE_3;
MMP_BOOL			m_bRetriggerGRA			= MMP_FALSE;

MMP_LDC_MULTI_SLICE_INFO 	m_LdcMultiSliceInfo;
MMP_USHORT               	m_usLdcSliceIdx 	= 0xFF;
MMP_UBYTE					m_ubLdcIBCSnrId		= 0;
MMP_UBYTE					m_ubLdcCurProcSnrId	= 0;
MMP_ULONG 					m_ulLdcFrmDoneCnt 	= 0;

static MMP_LDC_RES_MODE	m_LdcResMode 		= MMP_LDC_RES_MODE_FHD;
static MMP_LDC_FPS_MODE	m_LdcFpsMode 		= MMP_LDC_FPS_MODE_60P;
static MMP_LDC_RUN_MODE	m_LdcRunMode 		= MMP_LDC_RUN_MODE_DISABLE;

static MMP_USHORT	*m_pusLdcPositionTbl[2];
static MMP_ULONG	*m_pulLdcDeltaTbl[12];

static MMP_BOOL 	m_bLdcInitFlag = MMP_FALSE;

static MMP_ULONG	m_ulLdcSrcFrameWidth 	= 0;
static MMP_ULONG	m_ulLdcSrcFrameHeight 	= 0;
static MMP_ULONG	m_ulLdcOutFrameWidth 	= 0;
static MMP_ULONG	m_ulLdcOutFrameHeight 	= 0;

#if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
static MMP_UBYTE	gbLoopBackDoneBufIdx	= 1;
static MMP_UBYTE 	gbLoopBackCurBufIdx 	= 0;
#endif

#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
MMP_ULONG			gulLdcSliceStartTime[MAX_LDC_SLICE_NUM][LDC_DEBUG_TIME_TBL_MAX_NUM];
#endif

//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================

#if (SUPPORT_LDC_RECD) || (SUPPORT_LDC_CAPTURE)
extern MMP_IBC_LINK_TYPE 	gIBCLinkType[];
extern MMP_UBYTE            gbIBCCurBufIdx[];
extern MMP_UBYTE   			gbExposureDoneBufIdx[];
extern MMP_ULONG   			glPreviewBufAddr[MMP_IBC_PIPE_MAX][4];
extern MMP_ULONG   			glPreviewUBufAddr[MMP_IBC_PIPE_MAX][4];
extern MMP_ULONG   			glPreviewVBufAddr[MMP_IBC_PIPE_MAX][4];
extern MMP_USHORT 	 		gsPreviewBufWidth[];
extern MMP_USHORT  			gsPreviewBufHeight[];
extern MMP_UBYTE			m_ubRawFetchSnrId;
#endif

#if (HANDLE_LDC_EVENT_BY_TASK)
extern MMPF_OS_SEMID		m_LdcCtlSem;
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_ISR
//  Description : This function is the LDC interrupt service routine.
//------------------------------------------------------------------------------
void MMPF_LDC_ISR(void)
{
   	AITPS_LDC   pLDC = AITC_BASE_LDC;
    MMP_ULONG	intsrc;
    
    intsrc = pLDC->LDC_INT_CPU_EN & pLDC->LDC_INT_CPU_SR;
    pLDC->LDC_INT_CPU_SR = intsrc;
    
    //TBD
	if (intsrc & LDC_INT_INPUT_FRM_END) {
	}
	if (intsrc & LDC_INT_INPUT_FRM_ST) {
	}
	if (intsrc & LDC_INT_OUTPUT_FRM_END) {
	}
	if (intsrc & LDC_INT_TBL_UPDATE_RDY) {
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_ClearAllInterrupt
//  Description :
//------------------------------------------------------------------------------
void MMPF_LDC_ClearAllInterrupt(void)
{
   	AITPS_LDC pLDC = AITC_BASE_LDC;
    
    pLDC->LDC_INT_HOST_SR |= LDC_INT_MASK;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_Initialize
//  Description : Open interrupt and module clock.
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_Initialize(void)
{
   	AITPS_AIC pAIC = AITC_BASE_AIC;
	
	if (m_bLdcInitFlag == MMP_FALSE) {
	    
        RTNA_AIC_Open(pAIC, AIC_SRC_LDC, ldc_isr_a,
                    	AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);

		RTNA_AIC_IRQ_En(pAIC, AIC_SRC_LDC);
        
		m_bLdcInitFlag = MMP_TRUE;
	}
	
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_TRUE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_UnInitialize
//  Description : Close module clock.
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_UnInitialize(void)
{
    AITPS_LDC pLDC = AITC_BASE_LDC;  
    AITPS_AIC pAIC = AITC_BASE_AIC;

    if (m_bLdcInitFlag == MMP_TRUE) {

        pLDC->LDC_CTL = LDC_BYPASS;
        RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_LDC);

        m_bLdcInitFlag = MMP_FALSE;
    }

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_FALSE);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetResMode
//  Description : Set LDC resolution mode
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetResMode(MMP_UBYTE ubResMode)
{
#if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
	if (ubResMode >= MMP_LDC_RES_MODE_NUM) {
		return MMP_LDC_ERR_PARAMETER;
	}
	
	m_LdcResMode = ubResMode;
	
	/* Initial Position/Delta Table for Single-Run or Multi-Run */
	switch(ubResMode)
	{
		case MMP_LDC_RES_MODE_FHD:
			m_pusLdcPositionTbl[0] 	= &m_usPositionX_FHD[0];
			m_pusLdcPositionTbl[1] 	= &m_usPositionY_FHD[0];
			
			m_pulLdcDeltaTbl[0]		= &m_ulDeltaMemA_000_127_FHD[0];
			m_pulLdcDeltaTbl[1]		= &m_ulDeltaMemA_128_255_FHD[0];
			m_pulLdcDeltaTbl[2]		= &m_ulDeltaMemA_256_335_FHD[0];
			m_pulLdcDeltaTbl[3]		= &m_ulDeltaMemB_000_127_FHD[0];
			m_pulLdcDeltaTbl[4]		= &m_ulDeltaMemB_128_255_FHD[0];
			m_pulLdcDeltaTbl[5]		= &m_ulDeltaMemB_256_335_FHD[0];
			m_pulLdcDeltaTbl[6]		= &m_ulDeltaMemC_000_127_FHD[0];
			m_pulLdcDeltaTbl[7]		= &m_ulDeltaMemC_128_255_FHD[0];
			m_pulLdcDeltaTbl[8]		= &m_ulDeltaMemC_256_335_FHD[0];
			m_pulLdcDeltaTbl[9]		= &m_ulDeltaMemD_000_127_FHD[0];
			m_pulLdcDeltaTbl[10]	= &m_ulDeltaMemD_128_255_FHD[0];
			m_pulLdcDeltaTbl[11]	= &m_ulDeltaMemD_256_335_FHD[0];
		break;
		case MMP_LDC_RES_MODE_HD:
			m_pusLdcPositionTbl[0] 	= &m_usPositionX_HD[0];
			m_pusLdcPositionTbl[1] 	= &m_usPositionY_HD[0];
			
			m_pulLdcDeltaTbl[0]		= &m_ulDeltaMemA_000_127_HD[0];
			m_pulLdcDeltaTbl[1]		= &m_ulDeltaMemA_128_255_HD[0];
			m_pulLdcDeltaTbl[2]		= &m_ulDeltaMemA_256_335_HD[0];
			m_pulLdcDeltaTbl[3]		= &m_ulDeltaMemB_000_127_HD[0];
			m_pulLdcDeltaTbl[4]		= &m_ulDeltaMemB_128_255_HD[0];
			m_pulLdcDeltaTbl[5]		= &m_ulDeltaMemB_256_335_HD[0];
			m_pulLdcDeltaTbl[6]		= &m_ulDeltaMemC_000_127_HD[0];
			m_pulLdcDeltaTbl[7]		= &m_ulDeltaMemC_128_255_HD[0];
			m_pulLdcDeltaTbl[8]		= &m_ulDeltaMemC_256_335_HD[0];
			m_pulLdcDeltaTbl[9]		= &m_ulDeltaMemD_000_127_HD[0];
			m_pulLdcDeltaTbl[10]	= &m_ulDeltaMemD_128_255_HD[0];
			m_pulLdcDeltaTbl[11]	= &m_ulDeltaMemD_256_335_HD[0];
		break;
		case MMP_LDC_RES_MODE_WVGA:
			m_pusLdcPositionTbl[0] 	= &m_usPositionX_WVGA[0];
			m_pusLdcPositionTbl[1] 	= &m_usPositionY_WVGA[0];
			
			m_pulLdcDeltaTbl[0]		= &m_ulDeltaMemA_000_127_WVGA[0];
			m_pulLdcDeltaTbl[1]		= &m_ulDeltaMemA_128_255_WVGA[0];
			m_pulLdcDeltaTbl[2]		= &m_ulDeltaMemA_256_335_WVGA[0];
			m_pulLdcDeltaTbl[3]		= &m_ulDeltaMemB_000_127_WVGA[0];
			m_pulLdcDeltaTbl[4]		= &m_ulDeltaMemB_128_255_WVGA[0];
			m_pulLdcDeltaTbl[5]		= &m_ulDeltaMemB_256_335_WVGA[0];
			m_pulLdcDeltaTbl[6]		= &m_ulDeltaMemC_000_127_WVGA[0];
			m_pulLdcDeltaTbl[7]		= &m_ulDeltaMemC_128_255_WVGA[0];
			m_pulLdcDeltaTbl[8]		= &m_ulDeltaMemC_256_335_WVGA[0];
			m_pulLdcDeltaTbl[9]		= &m_ulDeltaMemD_000_127_WVGA[0];
			m_pulLdcDeltaTbl[10]	= &m_ulDeltaMemD_128_255_WVGA[0];
			m_pulLdcDeltaTbl[11]	= &m_ulDeltaMemD_256_335_WVGA[0];
		break;
	}
#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_GetResMode
//  Description : Get LDC resolution mode
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_LDC_GetResMode(void)
{
	return m_LdcResMode;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetResMode
//  Description : Set LDC frame rate mode
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetFpsMode(MMP_UBYTE ubFpsMode)
{
	if (ubFpsMode >= MMP_LDC_FPS_MODE_NUM) {
		return MMP_LDC_ERR_PARAMETER;
	}

	m_LdcFpsMode = ubFpsMode;
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_GetFpsMode
//  Description : Get LDC frame rate mode
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_LDC_GetFpsMode(void)
{
	return m_LdcFpsMode;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetRunMode
//  Description : Set LDC run mode
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetRunMode(MMP_UBYTE ubRunMode)
{
	if (ubRunMode >= MMP_LDC_RUN_MODE_NUM) {
		return MMP_LDC_ERR_PARAMETER;
	}

	m_LdcRunMode = ubRunMode;
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_GetRunMode
//  Description : Get LDC run mode
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_LDC_GetRunMode(void)
{
	return m_LdcRunMode;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetFrameRes
//  Description : Set LDC frame resoltion
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetFrameRes(MMP_ULONG ulSrcW, MMP_ULONG ulSrcH,
							 MMP_ULONG ulOutW, MMP_ULONG ulOutH)
{

	m_ulLdcSrcFrameWidth 	= ulSrcW;
	m_ulLdcSrcFrameHeight 	= ulSrcH;
	m_ulLdcOutFrameWidth	= ulOutW;
	m_ulLdcOutFrameHeight	= ulOutH;
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetLinkPipe
//  Description : Set LDC link pipe
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetLinkPipe(MMP_UBYTE ubSrcPipe, MMP_UBYTE ubPrvPipe, MMP_UBYTE ubEncPipe, MMP_UBYTE ubSwiPipe, MMP_UBYTE ubJpegPipe)
{
	m_LdcPathLink.srcPipeID = ubSrcPipe;
	m_LdcPathLink.prvPipeID	= ubPrvPipe;
	m_LdcPathLink.encPipeID	= ubEncPipe;
	m_LdcPathLink.swiPipeID	= ubSwiPipe;
	m_LdcPathLink.jpgPipeID	= ubJpegPipe;
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_GetLinkPipe
//  Description : Get LDC link pipe
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_GetLinkPipe(MMP_LDC_LINK *pLink)
{	
	*pLink = m_LdcPathLink;
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_SetAttribute
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_SetAttribute(MMP_LDC_ATTR* pAttr)
{
    AITPS_LDC pLDC = AITC_BASE_LDC;

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_TRUE);

	if (pAttr->bLdcEnable) {
		
		pLDC->LDC_CTL 			    = LDC_ENALBE | LDC_SRAM_MODE_EN | LDC_SHARE_FB_EN;
		pLDC->LDC_BUSY_CHK 			= LDC_BUSY_CHK_EN;

    	pLDC->LDC_X_DELTA_BASE      = 4;
    	pLDC->LDC_Y_DELTA_BASE      = 4;
    	pLDC->LDC_BICUBIC_INT_EN    = LDC_BICUBIC_INT_ENABLE;

    	/* Below setting is for the line which over table boundary */
		pLDC->LDC_BOUND_EXT			= LDC_BOUND_EXT_EN;

		/* Set input source */
		if (pAttr->ubInputPath == MMPF_LDC_INPUT_FROM_ISP)
			pLDC->LDC_INPUT_PATH = LDC_ISP_INPUT;
		else if (pAttr->ubInputPath == MMPF_LDC_INPUT_FROM_GRA)	
			pLDC->LDC_INPUT_PATH = LDC_GRA_INPUT;
		
		/* Set input range */
		pLDC->LDC_IN_INIT_X_OFST 	= 0;
		pLDC->LDC_IN_INIT_Y_OFST 	= 0;
		pLDC->LDC_IN_X_ST			= pAttr->usInputStX;
		pLDC->LDC_IN_Y_ST			= pAttr->usInputStY;
		pLDC->LDC_IN_FRM_W			= pAttr->usInputW;
		pLDC->LDC_IN_FRM_H			= pAttr->usInputH;
		
		pLDC->LDC_IN_X_RATIO		= pAttr->ubInputXratio;
		pLDC->LDC_IN_Y_RATIO		= pAttr->ubInputYratio;

        if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE) {
			pLDC->LDC_SHARE_FB_MODE |= LDC_SHARE_FB_720P_MODE;
			pLDC->LDC_SHARE_FB_DEEP = 102;
        }
        else {
        
    		if (m_LdcResMode == MMP_LDC_RES_MODE_HD ||
    			m_LdcResMode == MMP_LDC_RES_MODE_WVGA) {
    			pLDC->LDC_SHARE_FB_MODE |= LDC_SHARE_FB_720P_MODE;
    			pLDC->LDC_SHARE_FB_DEEP = 102;
    		}
    		else {
    			pLDC->LDC_SHARE_FB_MODE &= ~(LDC_SHARE_FB_720P_MODE);
    		}
        }

		pLDC->LDC_SHARE_FB_MODE 	|= LDC_EOL_RST_LOAD_FSM;

		/* Set output range */
		pLDC->LDC_OUT_CROP_EN		= pAttr->bOutCrop;
		pLDC->LDC_OUT_CROP_X_ST		= pAttr->usOutputStX;
		pLDC->LDC_OUT_CROP_Y_ST		= pAttr->usOutputStY;
		pLDC->LDC_OUT_CROP_X_LEN	= pAttr->usOutputW;
		pLDC->LDC_OUT_CROP_Y_LEN	= pAttr->usOutputH;
		
		pLDC->LDC_OUT_LINE_BLANKING	= pAttr->ubOutLineDelay;
		
		/* Set LUT DMA attribute */
		if (pAttr->ulDmaAddr != 0x0) {
			pLDC->LDC_LUT_DMA_EN 	= LDC_LUT_DMA_FRM_END;
			pLDC->LDC_LUT_DMA_ADDR 	= pAttr->ulDmaAddr;
		}
		
		/* Misc attribute : TBD */
		#if 1
		pLDC->LDC_PROG_FRM_X_ST     = 0x00;
    	pLDC->LDC_PROG_FRM_Y_ST     = 0x00;
		#else
		pLDC->LDC_PROG_FRM_X_ST     = 0x03C0;
    	pLDC->LDC_PROG_FRM_Y_ST     = 0x004E;
		#endif
	}
	else {
		pLDC->LDC_CTL &= ~(LDC_ENALBE | LDC_SRAM_MODE_EN | LDC_SHARE_FB_EN);
		pLDC->LDC_CTL |= LDC_BYPASS;
	}

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_FALSE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_UpdateLUT
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_UpdateLUT(MMP_USHORT* pPosTbl[], MMP_ULONG* pDeltaTbl[])
{
    AITPS_LDC       pLDC    = AITC_BASE_LDC;
    AITPS_LDC_LUT   pLDCLUT = AITC_BASE_LDC_LUT;
    MMP_ULONG       i = 0;

	if (pPosTbl == NULL) {
		pPosTbl = m_pusLdcPositionTbl;
	}
	if (pDeltaTbl == NULL) {
		pDeltaTbl = m_pulLdcDeltaTbl;
	}

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_TRUE);

    // Set LDC X position (42 pixel)
    for (i = 0; i < LDC_X_POS_ARRAY_SIZE; i++){
        pLDC->LDC_LUT_X[i] = pPosTbl[0][i];
    }
    
    // Set LDC Y position (32 pixel)
    for (i = 0; i < LDC_Y_POS_ARRAY_SIZE; i++){
        pLDC->LDC_LUT_Y[i] = pPosTbl[1][i];
    }
    
    // LDC Delta Table (MEMA)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[0][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[1][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[2][i]; 
    }
  
    // LDC Delta Table (MEMB)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[3][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[4][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[5][i]; 
    }  

    // LDC Delta Table (MEMC)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[6][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[7][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[8][i]; 
    }

    // LDC Delta Table (MEMD)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[9][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[10][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++){
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[11][i]; 
    }

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_FALSE);

    return MMP_ERR_NONE; 
}

#if 0
void ____Multi_Run_Function____(){ruturn;} //dummy
#endif

#if (SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)
//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunSwitchPipeMode
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiRunSwitchPipeMode(MMP_UBYTE ubMode)
{
	MMP_UBYTE ubPipe = m_LdcPathLink.swiPipeID;

	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_RUN)
	{
		MMPF_IBC_ResetModule((MMP_IBC_PIPEID)ubPipe);

		if (ubMode == MMPF_LDC_PIPE_MODE_LOOPBACK)
		{
			gIBCLinkType[ubPipe] = MMP_IBC_LINK_LDC;
		}
		else if (ubMode == MMPF_LDC_PIPE_MODE_DISP)
		{
			gIBCLinkType[ubPipe] = MMP_IBC_LINK_DISPLAY;
		}
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunTriggerLoopBack
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiRunTriggerLoopBack(MMP_UBYTE ubIBCPipe)
{
	MMP_GRAPHICS_BUF_ATTR src, dst;

    dst.ulBaseAddr 	= (dst.ulBaseUAddr = (dst.ulBaseVAddr = 0));

	src.colordepth  = MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
    src.ulBaseAddr 	= glPreviewBufAddr[ubIBCPipe][gbExposureDoneBufIdx[ubIBCPipe]];
    src.ulBaseUAddr = glPreviewUBufAddr[ubIBCPipe][gbExposureDoneBufIdx[ubIBCPipe]];
    src.ulBaseVAddr = glPreviewVBufAddr[ubIBCPipe][gbExposureDoneBufIdx[ubIBCPipe]];
	
	m_ubLdcLbSrcPipe = ubIBCPipe;
	
	if (m_ubSwiPipeLbCurCnt == 1)
	{
		/* Before last time trigger Graphics, 
		   Enable encode pipe for H264 and switch pipe for display */
		MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.encPipeID, MMP_TRUE);
		MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.jpgPipeID, MMP_TRUE);
		
		MMPF_LDC_MultiRunSwitchPipeMode(MMPF_LDC_PIPE_MODE_DISP);
	}
	
	MMPF_Graphics_ScaleStart(&src, &dst, 
							 MMPF_LDC_MultiRunNormalGraCallback, 
							 MMPF_LDC_MultiRunRetriggerGraCallback);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunGetMaxLoopBackCount
//  Description : 
//------------------------------------------------------------------------------
MMP_ULONG MMPF_LDC_MultiRunGetMaxLoopBackCount(void)
{
	if (m_LdcResMode == MMP_LDC_RES_MODE_FHD)
		return (LDC_MAX_LB_CNT_FOR_FHD - 1);
	else if (m_LdcResMode == MMP_LDC_RES_MODE_HD)
		return (LDC_MAX_LB_CNT_FOR_HD - 1);
	else if (m_LdcResMode == MMP_LDC_RES_MODE_WVGA)
		return (LDC_MAX_LB_CNT_FOR_WVGA - 1);
	else
		return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunSetLoopBackCount
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiRunSetLoopBackCount(MMP_UBYTE ubCnt)
{
	m_ubSwiPipeLbTargetCnt = ubCnt;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunNormalGraCallback
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiRunNormalGraCallback(void)
{
    if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_RUN)
   	{
   		if (m_ubLdcLbSrcPipe == m_LdcPathLink.swiPipeID && 
   			m_bRetriggerGRA == MMP_FALSE)
   		{
   			if (m_ubSwiPipeLbCurCnt > 0) {
   				m_ubSwiPipeLbCurCnt--;
   			}
   		}
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiRunRetriggerGraCallback
//  Description : Patch For 1080p
//------------------------------------------------------------------------------
void MMPF_LDC_MultiRunRetriggerGraCallback(void)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_RUN)
	{
   		if (m_bRetriggerGRA == MMP_TRUE)
   		{
   			m_bRetriggerGRA = MMP_FALSE;
   			MMPF_LDC_MultiRunTriggerLoopBack(m_LdcPathLink.swiPipeID);
   			m_ubSwiPipeLbCurCnt--;
   		}
	}
}

#if 0
void ____Multi_Slice_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_LdcSrcPipeFrameSt
//  Description :
//------------------------------------------------------------------------------
void CallbackFunc_LdcSrcPipeFrameSt(void* argu)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
		/* Get the sensor ID at frame start than frame ready to avoid getting the wrong ID when pipe busy */
		m_ubLdcIBCSnrId = m_ubRawFetchSnrId;
	}
}

//------------------------------------------------------------------------------
//  Function    : CallbackFunc_LdcDmaMoveFrame
//  Description :
//------------------------------------------------------------------------------
void CallbackFunc_LdcDmaMoveFrame(void* argu)
{
#if (PARALLEL_FRAME_STORE_TYPE == PARALLEL_FRM_STORE_EQUIRETANGLE)

	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
	    MMP_GRAPHICS_BUF_ATTR src, dst;
		
		src.ulBaseAddr 	= (src.ulBaseUAddr = (src.ulBaseVAddr = 0));
		dst.ulBaseAddr 	= (dst.ulBaseUAddr = (dst.ulBaseVAddr = 0));

		src.colordepth  = MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;

		/* Enable encode pipe */
		MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.encPipeID, MMP_TRUE);
		MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.jpgPipeID, MMP_TRUE);

		MMPF_LDC_MultiSliceSwitchPipeMode(MMPF_LDC_PIPE_MODE_DISP);

	    MMPF_LDC_MultiSliceRestoreDispPipe();

		MMPF_LDC_MultiSliceUpdateOutBufIdx();

	    MMPF_Graphics_ScaleStart(&src, &dst, 
	                             MMPF_LDC_MultiSliceNormalGraCallback, 
	                             MMPF_LDC_MultiSliceRetriggerGraCallback);
	}
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceDmaMoveFrame
//  Description :
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceDmaMoveFrame(void)
{
#if (PARALLEL_FRAME_STORE_TYPE == PARALLEL_FRM_STORE_EQUIRETANGLE)

	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
	#if 1 // DMA Rotate

	    MMP_GRAPHICS_BUF_ATTR 	srcBufAttr, dstBufAttr;
	    MMP_GRAPHICS_RECT 		srcRect;
        MMP_USHORT	 			usDstStartX;
    	MMP_USHORT 				usDstStartY;
    	MMP_UBYTE				ubBufIdx = gbLoopBackDoneBufIdx;

	    srcBufAttr.usWidth 		= m_ulLdcOutFrameWidth * 5 / 2;
	    srcBufAttr.usHeight 	= m_ulLdcOutFrameHeight;
		srcBufAttr.usLineOffset = m_ulLdcOutFrameWidth * 5 / 2;
		srcBufAttr.colordepth 	= MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
	    srcBufAttr.ulBaseAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[ubBufIdx];
	    srcBufAttr.ulBaseUAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[ubBufIdx];
	    srcBufAttr.ulBaseVAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[ubBufIdx];

	    srcRect.usLeft 			= m_ulLdcOutFrameWidth * 2;
	    srcRect.usTop 			= 0;
	    srcRect.usWidth 		= m_ulLdcOutFrameWidth / 2;
	    srcRect.usHeight 		= m_ulLdcOutFrameHeight;
	    
		dstBufAttr.colordepth 	= MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
		dstBufAttr.usLineOffset = m_ulLdcOutFrameWidth * 5 / 2;
		dstBufAttr.ulBaseAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[ubBufIdx];
    	dstBufAttr.ulBaseUAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[ubBufIdx];
    	dstBufAttr.ulBaseVAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[ubBufIdx];				
		
		usDstStartX 			= 0;
		usDstStartY				= 0;
		
	    MMPF_DMA_RotateImageBuftoBuf(&srcBufAttr, &srcRect, &dstBufAttr, 
                                     usDstStartX, usDstStartY, 
                                     MMP_GRAPHICS_ROTATE_NO_ROTATE, 
                                     CallbackFunc_LdcDmaMoveFrame, NULL, 
                                     MMP_FALSE, DMA_NO_MIRROR);
	    	
	#else // DMA Move (NV12 Move 2 Times)
	
	    MMP_ULONG 		    ulSrcAddr;
	    MMP_ULONG 		    ulDstAddr;
	    MMP_ULONG 		    ulCount;
	    MMP_DMA_LINE_OFST   LineOfstArg;
		MMP_UBYTE			ubBufIdx = gbLoopBackDoneBufIdx;
			
	    ulSrcAddr = m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[ubBufIdx] + m_ulLdcOutFrameWidth * 2;

	    ulDstAddr = m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[ubBufIdx];
	    
	    ulCount = (m_ulLdcOutFrameWidth / 2) * m_ulLdcOutFrameHeight;
	    
	    LineOfstArg.ulSrcWidth  = m_ulLdcOutFrameWidth / 2;
	    LineOfstArg.ulSrcOffset = m_ulLdcOutFrameWidth * 5 / 2;
	    LineOfstArg.ulDstWidth  = m_ulLdcOutFrameWidth / 2;
	    LineOfstArg.ulDstOffset = m_ulLdcOutFrameWidth * 5 / 2;
	    
	    MMPF_DMA_MoveData(ulSrcAddr, ulDstAddr, ulCount, NULL, 0, MMP_TRUE, &LineOfstArg);

	    ulSrcAddr = m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[ubBufIdx] + m_ulLdcOutFrameWidth * 2;

	    ulDstAddr = m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[ubBufIdx];
	    
	    ulCount = (m_ulLdcOutFrameWidth / 2) * (m_ulLdcOutFrameHeight / 2);
	    
	    LineOfstArg.ulSrcWidth  = m_ulLdcOutFrameWidth / 2;
	    LineOfstArg.ulSrcOffset = m_ulLdcOutFrameWidth * 5 / 2;
	    LineOfstArg.ulDstWidth  = m_ulLdcOutFrameWidth / 2;
	    LineOfstArg.ulDstOffset = m_ulLdcOutFrameWidth * 5 / 2;
	    
	    MMPF_DMA_MoveData(ulSrcAddr, ulDstAddr, ulCount, CallbackFunc_LdcDmaMoveFrame, NULL, MMP_TRUE, &LineOfstArg);
	#endif
	}
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceUpdateSnrId
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceUpdateSnrId(MMP_UBYTE ubSnrId)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
		m_ubLdcCurProcSnrId = ubSnrId;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceUpdateOutBufIdx
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceUpdateOutBufIdx(void)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
		gbLoopBackDoneBufIdx += 1;
		
		if (gbLoopBackDoneBufIdx >= MAX_LDC_OUT_BUF_NUM) {
			gbLoopBackDoneBufIdx = 0;
	    }
	    
		gbLoopBackCurBufIdx += 1;
		
		if (gbLoopBackCurBufIdx >= MAX_LDC_OUT_BUF_NUM) {
			gbLoopBackCurBufIdx = 0;
	    }
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceSwitchPipeMode
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceSwitchPipeMode(MMP_UBYTE ubMode)
{
	MMP_UBYTE ubPipe = m_LdcPathLink.swiPipeID;

	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE) 
	{
		//MMPF_IBC_ResetModule((MMP_IBC_PIPEID)ubPipe);

		if (ubMode == MMPF_LDC_PIPE_MODE_LOOPBACK)
		{
			MMPF_Scaler_SetPath((MMP_SCAL_PIPEID)ubPipe, MMP_SCAL_SOURCE_LDC);

			gIBCLinkType[ubPipe] = MMP_IBC_LINK_LDC;
		}
		else if (ubMode == MMPF_LDC_PIPE_MODE_DISP)
		{
			MMPF_Scaler_SetPath((MMP_SCAL_PIPEID)ubPipe, MMP_SCAL_SOURCE_GRA);

			gIBCLinkType[ubPipe] = MMP_IBC_LINK_DISPLAY;
		}
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceNormalGraCallback
//  Description : 
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceNormalGraCallback(void)
{
    if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
   	{
   		if (m_bRetriggerGRA == MMP_FALSE)
   		{
   		    m_usLdcSliceIdx++;
   		}
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceRetriggerGraCallback
//  Description :
//------------------------------------------------------------------------------
void MMPF_LDC_MultiSliceRetriggerGraCallback(void)
{
    MMP_GRAPHICS_BUF_ATTR src, dst;

	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
   		if (m_bRetriggerGRA == MMP_TRUE)
   		{
			src.ulBaseAddr 	= (src.ulBaseUAddr = (src.ulBaseVAddr = 0));
			dst.ulBaseAddr 	= (dst.ulBaseUAddr = (dst.ulBaseVAddr = 0));

			src.colordepth  = MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;

   			m_bRetriggerGRA = MMP_FALSE;

   			m_usLdcSliceIdx++;

			/* LoopBack the other slices */
   			if (m_usLdcSliceIdx < MMPF_LDC_GetMultiSliceNum()) 
   			{
  				#if (LDC_DEBUG_MSG_EN)
  				if (m_ulLdcFrmDoneCnt < LDC_DEBUG_FRAME_MAX_NUM) {
  					#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
					gulLdcSliceStartTime[m_usLdcSliceIdx][m_ulLdcFrmDoneCnt] = OSTimeGet();
					#else
					printc(FG_YELLOW(">ReTrig S%d %d")"\r\n", m_usLdcSliceIdx, OSTimeGet());
					#endif
				}
				#endif

   			    MMPF_LDC_MultiSliceUpdatePipeAttr(m_usLdcSliceIdx);

		        MMPF_Graphics_ScaleStart(&src, &dst, 
		                                 MMPF_LDC_MultiSliceNormalGraCallback, 
		                                 MMPF_LDC_MultiSliceRetriggerGraCallback);
   		    }
   		    /* Switch to Display */
   		    else if (m_usLdcSliceIdx == MMPF_LDC_GetMultiSliceNum())
   		    {
  				#if (LDC_DEBUG_MSG_EN)
  				if (m_ulLdcFrmDoneCnt < LDC_DEBUG_FRAME_MAX_NUM) {
  					#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
					gulLdcSliceStartTime[m_usLdcSliceIdx][m_ulLdcFrmDoneCnt] = OSTimeGet();
					#else
					printc(FG_YELLOW(">ReTrig Done %d")"\r\n", OSTimeGet());
					#endif
				}
				#endif

   		    	m_ulLdcFrmDoneCnt++;

   		    	#if (HANDLE_LDC_EVENT_BY_TASK)

				if (m_ulLdcFrmDoneCnt % 2 == 0)
   		    	{
   		    		#if (LDC_DEBUG_MSG_EN)
   		    		if (m_ulLdcFrmDoneCnt < LDC_DEBUG_FRAME_MAX_NUM) {
   		    			#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
   		    			
   		    			#else
						printc(FG_YELLOW(">ReTrig Done2 %d")"\r\n", OSTimeGet());
						#endif
					}
					#endif
					
					#if (PARALLEL_FRAME_STORE_TYPE == PARALLEL_FRM_STORE_EQUIRETANGLE)

    				MMPF_OS_SetFlags(LDC_Ctl_Flag, LDC_FLAG_DMA_MOVE_FRAME, MMPF_OS_FLAG_SET);
					
					#else // (PARALLEL_FRAME_STORE_TYPE == PARALLEL_FRM_STORE_EQUIRETANGLE)

    		    	/* Enable encode pipe */
					MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.encPipeID, MMP_TRUE);
 					MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.jpgPipeID, MMP_TRUE);

	 				MMPF_LDC_MultiSliceSwitchPipeMode(MMPF_LDC_PIPE_MODE_DISP);

	                MMPF_LDC_MultiSliceRestoreDispPipe();

   		    		MMPF_LDC_MultiSliceUpdateOutBufIdx();

				    MMPF_Graphics_ScaleStart(&src, &dst, 
				                             MMPF_LDC_MultiSliceNormalGraCallback, 
				                             MMPF_LDC_MultiSliceRetriggerGraCallback);
   		    		#endif
   		    	}
   		    	else {
   		    		MMPF_OS_ReleaseSem(m_LdcCtlSem);
				}
   		    	#else
   				
   		    	/* Enable encode pipe */
				MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.encPipeID, MMP_TRUE);
				MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)m_LdcPathLink.jpgPipeID, MMP_TRUE);

				MMPF_LDC_MultiSliceSwitchPipeMode(MMPF_LDC_PIPE_MODE_DISP);

                MMPF_LDC_MultiSliceRestoreDispPipe();

   				MMPF_LDC_MultiSliceUpdateOutBufIdx();

			    MMPF_Graphics_ScaleStart(&src, &dst, 
			                             MMPF_LDC_MultiSliceNormalGraCallback, 
			                             MMPF_LDC_MultiSliceRetriggerGraCallback);
   		    	#endif
   		    }
   		}
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceUpdateLUT
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceUpdateLUT(MMP_UBYTE ubSlice)
{
    AITPS_LDC       pLDC    = AITC_BASE_LDC;
    AITPS_LDC_LUT   pLDCLUT = AITC_BASE_LDC_LUT;
    MMP_ULONG       i = 0;
    MMP_USHORT*     pPosTbl[2];
    MMP_ULONG*      pDeltaTbl[12];
    
	pPosTbl[0]      = m_LdcMultiSliceInfo.pusLdcPosTbl[0][ubSlice];
    pPosTbl[1]      = m_LdcMultiSliceInfo.pusLdcPosTbl[1][ubSlice];

    pDeltaTbl[0]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[0][ubSlice];
    pDeltaTbl[1]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[1][ubSlice];
    pDeltaTbl[2]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[2][ubSlice];
    pDeltaTbl[3]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[3][ubSlice];
    pDeltaTbl[4]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[4][ubSlice];
    pDeltaTbl[5]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[5][ubSlice];
    pDeltaTbl[6]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[6][ubSlice];
    pDeltaTbl[7]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[7][ubSlice];
    pDeltaTbl[8]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[8][ubSlice];
    pDeltaTbl[9]    = m_LdcMultiSliceInfo.pulLdcDeltaTbl[9][ubSlice];
    pDeltaTbl[10]   = m_LdcMultiSliceInfo.pulLdcDeltaTbl[10][ubSlice];
    pDeltaTbl[11]   = m_LdcMultiSliceInfo.pulLdcDeltaTbl[11][ubSlice];
                    
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_LDC, MMP_TRUE);

    // Set LDC X position (42 pixel)
    for (i = 0; i < LDC_X_POS_ARRAY_SIZE; i++) {
        pLDC->LDC_LUT_X[i] = pPosTbl[0][i];
    }
    
    // Set LDC Y position (32 pixel)
    for (i = 0; i < LDC_Y_POS_ARRAY_SIZE; i++) {
        pLDC->LDC_LUT_Y[i] = pPosTbl[1][i];
    }
    
    // LDC Delta Table (MEMA)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[0][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[1][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMA;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[2][i]; 
    }
  
    // LDC Delta Table (MEMB)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[3][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[4][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMB;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[5][i]; 
    }  

    // LDC Delta Table (MEMC)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[6][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[7][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMC;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[8][i]; 
    }

    // LDC Delta Table (MEMD)
    pLDC->LDC_TBL_WR_SEL = LDC_RW_000_127_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[9][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_128_255_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[10][i]; 
    }

    pLDC->LDC_TBL_WR_SEL = LDC_RW_256_335_DATA_MEMD;

    for (i = 0; i< LDC_DELTA_ARRAY_SIZE; i++) {
        pLDCLUT->DELTA_TBL[i] = pDeltaTbl[11][i]; 
    }

    return MMP_ERR_NONE; 
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceInitAttr
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceInitAttr(MMP_UBYTE ubResIdx)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
	    MMP_UBYTE   ubSrcPipe = m_LdcPathLink.srcPipeID;
	    MMP_ULONG   ulBackupYBufAddr[MAX_LDC_OUT_BUF_NUM] = {0, 0};
	    MMP_ULONG   ulBackupUBufAddr[MAX_LDC_OUT_BUF_NUM] = {0, 0};
	    MMP_ULONG   ulBackupVBufAddr[MAX_LDC_OUT_BUF_NUM] = {0, 0};
		MMP_UBYTE	i = 0;
		MMP_USHORT	usSliceNum = 3;
		MMP_LDC_MULTI_SLICE_BLOCKINFO* psMultiSliceInfo[MAX_LDC_SLICE_NUM];
		
		/* Reset the Store Buffer Index */
		gbLoopBackDoneBufIdx	= MAX_LDC_OUT_BUF_NUM - 1;
		gbLoopBackCurBufIdx 	= 0;

		/* Reset the Frame Count */
		m_ulLdcFrmDoneCnt		= 0;
		
		/* Initial Slice Table Pointer */
		if (ubResIdx == MMP_LDC_RES_MODE_MS_736P) {
			usSliceNum = gusSlice_736P_Num;

			for (i = 0; i < usSliceNum; i++) {
				psMultiSliceInfo[i] = gpsMultiSlice_736P_Info[i];
			}
		}
		else if (ubResIdx == MMP_LDC_RES_MODE_MS_1080P) {
			usSliceNum = gusSlice_1080P_Num;

			for (i = 0; i < usSliceNum; i++) {
				psMultiSliceInfo[i] = gpsMultiSlice_1080P_Info[i];
			}
		}
		else if (ubResIdx == MMP_LDC_RES_MODE_MS_1536P) {
			usSliceNum = gusSlice_1536P_Num;

			for (i = 0; i < usSliceNum; i++) {
				psMultiSliceInfo[i] = gpsMultiSlice_1536P_Info[i];
			}
		}
		else {
			return MMP_LDC_ERR_PARAMETER;
		}

		/* BackUp the Store Address */
		for (i = 0; i < MAX_LDC_OUT_BUF_NUM; i++) {
	    	ulBackupYBufAddr[i] = m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[i];
	    	ulBackupUBufAddr[i] = m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[i];
	    	ulBackupVBufAddr[i] = m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[i];		
		}
		
		MEMSET(&m_LdcMultiSliceInfo, 0, sizeof(MMP_LDC_MULTI_SLICE_INFO));

		m_LdcMultiSliceInfo.usSliceNum					= usSliceNum;
		
		for (i = 0; i < MAX_LDC_OUT_BUF_NUM; i++) {
	        m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[i]	= ulBackupYBufAddr[i];
	        m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[i]	= ulBackupUBufAddr[i];
		    m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[i]	= ulBackupVBufAddr[i];
		}
		
		/* Source Frame Buffer Information */
		m_LdcMultiSliceInfo.sGraBufAttr.usWidth 		= m_ulLdcSrcFrameWidth;
		m_LdcMultiSliceInfo.sGraBufAttr.usHeight		= m_ulLdcSrcFrameHeight;
		m_LdcMultiSliceInfo.sGraBufAttr.usLineOffset	= m_LdcMultiSliceInfo.sGraBufAttr.usWidth;
		m_LdcMultiSliceInfo.sGraBufAttr.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
		m_LdcMultiSliceInfo.sGraBufAttr.ulBaseAddr		= glPreviewBufAddr[ubSrcPipe][gbExposureDoneBufIdx[ubSrcPipe]];
		m_LdcMultiSliceInfo.sGraBufAttr.ulBaseUAddr		= glPreviewUBufAddr[ubSrcPipe][gbExposureDoneBufIdx[ubSrcPipe]];
		m_LdcMultiSliceInfo.sGraBufAttr.ulBaseVAddr		= glPreviewVBufAddr[ubSrcPipe][gbExposureDoneBufIdx[ubSrcPipe]];
		
		for (i = 0; i < m_LdcMultiSliceInfo.usSliceNum; i++)
		{
			/* Grab Frame Rectangle Information */
			m_LdcMultiSliceInfo.sGraRect[i].usLeft 			= psMultiSliceInfo[i]->sGraInRect.usLeft;
			m_LdcMultiSliceInfo.sGraRect[i].usTop			= psMultiSliceInfo[i]->sGraInRect.usTop;
			m_LdcMultiSliceInfo.sGraRect[i].usWidth			= psMultiSliceInfo[i]->sGraInRect.usWidth;
			m_LdcMultiSliceInfo.sGraRect[i].usHeight		= psMultiSliceInfo[i]->sGraInRect.usHeight;
			
			/* LDC Position Table */
			m_LdcMultiSliceInfo.pusLdcPosTbl[0][i]			= psMultiSliceInfo[i]->pusLdcPosTbl[0];
			m_LdcMultiSliceInfo.pusLdcPosTbl[1][i]			= psMultiSliceInfo[i]->pusLdcPosTbl[1];

			/* LDC Delta Table */
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[0][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[0];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[1][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[1];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[2][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[2];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[3][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[3];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[4][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[4];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[5][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[5];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[6][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[6];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[7][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[7];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[8][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[8];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[9][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[9];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[10][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[10];
			m_LdcMultiSliceInfo.pulLdcDeltaTbl[11][i]		= psMultiSliceInfo[i]->pulLdcDeltaTbl[11];

			/* LDC Module Information */
			m_LdcMultiSliceInfo.sLdcAttr[i].bLdcEnable		= MMP_TRUE;
			m_LdcMultiSliceInfo.sLdcAttr[i].ubInputPath		= MMPF_LDC_INPUT_FROM_GRA;
			m_LdcMultiSliceInfo.sLdcAttr[i].usInputStX		= 0;
			m_LdcMultiSliceInfo.sLdcAttr[i].usInputStY		= 0;
			m_LdcMultiSliceInfo.sLdcAttr[i].usInputW		= m_LdcMultiSliceInfo.sGraRect[i].usWidth;
			m_LdcMultiSliceInfo.sLdcAttr[i].usInputH		= m_LdcMultiSliceInfo.sGraRect[i].usHeight;
			m_LdcMultiSliceInfo.sLdcAttr[i].ubInputXratio	= 0;
			m_LdcMultiSliceInfo.sLdcAttr[i].ubInputYratio	= 0;
			m_LdcMultiSliceInfo.sLdcAttr[i].bOutCrop		= MMP_TRUE;
			m_LdcMultiSliceInfo.sLdcAttr[i].usOutputStX		= psMultiSliceInfo[i]->sLdcOutRect.usLeft;
			m_LdcMultiSliceInfo.sLdcAttr[i].usOutputStY		= psMultiSliceInfo[i]->sLdcOutRect.usTop;
			m_LdcMultiSliceInfo.sLdcAttr[i].usOutputW		= psMultiSliceInfo[i]->sLdcOutRect.usWidth;
			m_LdcMultiSliceInfo.sLdcAttr[i].usOutputH		= psMultiSliceInfo[i]->sLdcOutRect.usHeight;
			m_LdcMultiSliceInfo.sLdcAttr[i].ubOutLineDelay	= 0;
			m_LdcMultiSliceInfo.sLdcAttr[i].ulDmaAddr		= 0;
	
			/* LDC Internal Scaling (Up) Ratio */
			m_LdcMultiSliceInfo.ubScaleRatioH[i]			= psMultiSliceInfo[i]->ubScaleRatioH;
			m_LdcMultiSliceInfo.ubScaleRatioV[i]			= psMultiSliceInfo[i]->ubScaleRatioV;

			/* Scaler Fit Range Information */
			m_LdcMultiSliceInfo.sFitRange[i].fitmode		= MMP_SCAL_FITMODE_OPTIMAL;
			m_LdcMultiSliceInfo.sFitRange[i].scalerType		= MMP_SCAL_TYPE_SCALER;
			m_LdcMultiSliceInfo.sFitRange[i].ulInWidth		= m_LdcMultiSliceInfo.sLdcAttr[i].usOutputW;
			m_LdcMultiSliceInfo.sFitRange[i].ulInHeight		= m_LdcMultiSliceInfo.sLdcAttr[i].usOutputH;
			m_LdcMultiSliceInfo.sFitRange[i].ulOutWidth		= m_LdcMultiSliceInfo.sFitRange[i].ulInWidth / m_LdcMultiSliceInfo.ubScaleRatioH[i];
			m_LdcMultiSliceInfo.sFitRange[i].ulOutHeight	= m_LdcMultiSliceInfo.sFitRange[i].ulInHeight / m_LdcMultiSliceInfo.ubScaleRatioV[i];
			m_LdcMultiSliceInfo.sFitRange[i].ulInGrabX		= 1;
			m_LdcMultiSliceInfo.sFitRange[i].ulInGrabY		= 1;
			m_LdcMultiSliceInfo.sFitRange[i].ulInGrabW		= m_LdcMultiSliceInfo.sFitRange[i].ulInWidth;
			m_LdcMultiSliceInfo.sFitRange[i].ulInGrabH		= m_LdcMultiSliceInfo.sFitRange[i].ulInHeight;

			/* Scaler Grab Control Information */
			MMPF_Scaler_GetGCDBestFitScale(&m_LdcMultiSliceInfo.sFitRange[i],
										   &m_LdcMultiSliceInfo.sGrabCtl[i]);

			/* Icon Frame Width Information */
			m_LdcMultiSliceInfo.usIconFrmW[i]				= m_LdcMultiSliceInfo.sGrabCtl[i].ulOutEdX -
															  m_LdcMultiSliceInfo.sGrabCtl[i].ulOutStX + 1;
			m_LdcMultiSliceInfo.usIconFrmH[i]				= m_LdcMultiSliceInfo.sGrabCtl[i].ulOutEdY -
															  m_LdcMultiSliceInfo.sGrabCtl[i].ulOutStY + 1;

			/* IBC Destination Position Information */	
			m_LdcMultiSliceInfo.ulDstPosX[i]				= psMultiSliceInfo[i]->ulDstPosX;
			m_LdcMultiSliceInfo.ulDstPosY[i]				= psMultiSliceInfo[i]->ulDstPosY;

			/* IBC Line Offset Information */
			m_LdcMultiSliceInfo.ulLineOffset[i]				= m_ulLdcOutFrameWidth;
			m_LdcMultiSliceInfo.ulCbrLineOffset[i]			= m_ulLdcOutFrameWidth;
		}
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceInitOutStoreBuf
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceInitOutStoreBuf(MMP_UBYTE ubIdx, MMP_ULONG ulYAddr, MMP_ULONG ulUAddr, MMP_ULONG ulVAddr)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
	    m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[ubIdx] = ulYAddr;
	    m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[ubIdx] = ulUAddr;
	    m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[ubIdx] = ulVAddr;

		printc(">>LDC Out Buf[%d] Y %x U %x V %x\r\n",ubIdx, ulYAddr, ulUAddr, ulVAddr);
	}

	return MMP_ERR_NONE;
}	

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceUpdateSrcBufAddr
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceUpdateSrcBufAddr(MMP_ULONG ulYAddr, MMP_ULONG ulUAddr, MMP_ULONG ulVAddr)
{
    if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
    {
    	m_LdcMultiSliceInfo.sGraBufAttr.ulBaseAddr	= ulYAddr;
    	m_LdcMultiSliceInfo.sGraBufAttr.ulBaseUAddr	= ulUAddr;
    	m_LdcMultiSliceInfo.sGraBufAttr.ulBaseVAddr	= ulVAddr;
    }
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceUpdatePipeAttr
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceUpdatePipeAttr(MMP_UBYTE ubSlice)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
		MMP_ICO_PIPE_ATTR 	icoAttr;
		MMP_IBC_PIPE_ATTR  	ibcAttr;
		MMP_IBC_RECT       	ibcRect;
		MMP_UBYTE			ubSwiPipe 	= m_LdcPathLink.swiPipeID;
		MMP_LDC_RES_MODE	sResMode 	= m_LdcResMode;

		if (ubSlice >= m_LdcMultiSliceInfo.usSliceNum) {
			return MMP_LDC_ERR_PARAMETER;
		}
		
		/* Update Graphics Engine */
		MMPF_Graphics_SetScaleAttr(&m_LdcMultiSliceInfo.sGraBufAttr,
								   &m_LdcMultiSliceInfo.sGraRect[ubSlice],
								   1);
		
		if (sResMode == MMP_LDC_RES_MODE_MS_736P)
			MMPF_Graphics_SetPixDelay(16, 20);
		else
			MMPF_Graphics_SetPixDelay(13, 20);
		
		/* Update LDC Attribute */
		MMPF_LDC_ClearAllInterrupt();
		MMPF_LDC_SetAttribute(&m_LdcMultiSliceInfo.sLdcAttr[ubSlice]);
		
		/* Update LDC Table */
		MMPF_LDC_MultiSliceUpdateLUT(ubSlice);

		/* Reset Image Pipe */
		//MMPF_IBC_ResetModule(ubSwiPipe);
		
		/* Update Scaler Engine */
		MMPF_Scaler_ClearFrameStart((MMP_SCAL_PIPEID)ubSwiPipe);
		MMPF_Scaler_ClearFrameEnd((MMP_SCAL_PIPEID)ubSwiPipe);

        MMPF_Scaler_SetOutputFormat((MMP_SCAL_PIPEID)ubSwiPipe, MMP_SCAL_COLOR_YUV444);

		MMPF_Scaler_SetOutColorTransform((MMP_SCAL_PIPEID)ubSwiPipe, MMP_TRUE, MMP_SCAL_COLRMTX_YUV_FULLRANGE);
		
		MMPF_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, 
						      (MMP_SCAL_PIPEID)ubSwiPipe, 
							  &m_LdcMultiSliceInfo.sFitRange[ubSlice], 
							  &m_LdcMultiSliceInfo.sGrabCtl[ubSlice]);
		
		MMPF_Scaler_SetLPF((MMP_SCAL_PIPEID)ubSwiPipe, 
						   &m_LdcMultiSliceInfo.sFitRange[ubSlice], 
						   &m_LdcMultiSliceInfo.sGrabCtl[ubSlice]);
						   
		/* Update Icon Engine */
		icoAttr.inputsel 	= (MMP_SCAL_PIPEID)ubSwiPipe;
		icoAttr.bDlineEn	= MMP_TRUE;
		icoAttr.usFrmWidth	= m_LdcMultiSliceInfo.usIconFrmW[ubSlice];
		MMPF_Icon_SetDLAttributes((MMP_ICO_PIPEID)ubSwiPipe, &icoAttr);
		MMPF_Icon_SetDLEnable((MMP_ICO_PIPEID)ubSwiPipe, MMP_TRUE);
		
		/* Update IBC Engine */
    	ibcAttr.ulBaseAddr		= m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[gbLoopBackCurBufIdx];
    	ibcAttr.ulBaseUAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[gbLoopBackCurBufIdx];
    	ibcAttr.ulBaseVAddr 	= m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[gbLoopBackCurBufIdx];
		ibcAttr.usBufWidth		= m_ulLdcOutFrameWidth;
		ibcAttr.colorformat 	= MMP_IBC_COLOR_NV12;
		ibcAttr.ulLineOffset	= m_LdcMultiSliceInfo.ulLineOffset[ubSlice];
		ibcAttr.ulCbrLineOffset	= m_LdcMultiSliceInfo.ulCbrLineOffset[ubSlice];
		
		ibcRect.usLeft			= m_LdcMultiSliceInfo.ulDstPosX[ubSlice];
		ibcRect.usTop			= m_LdcMultiSliceInfo.ulDstPosY[ubSlice];
		ibcRect.usWidth			= m_LdcMultiSliceInfo.usIconFrmW[ubSlice];
		ibcRect.usHeight		= m_LdcMultiSliceInfo.usIconFrmH[ubSlice];
		
		MMPF_IBC_SetPartialStoreAttr(ubSwiPipe, &ibcAttr, &ibcRect);
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_MultiSliceRestoreDispPipe
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_MultiSliceRestoreDispPipe(void)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
	{
		MMP_ICO_PIPE_ATTR 	    icoAttr;
		MMP_IBC_PIPE_ATTR  	    ibcAttr;
		MMP_UBYTE			    ubSwiPipe 	= m_LdcPathLink.swiPipeID;
	    MMP_SCAL_FIT_RANGE      fitrange;
	    MMP_SCAL_GRAB_CTRL      grabctl;
        MMP_GRAPHICS_BUF_ATTR	srcBuf      = {0, };
	    MMP_GRAPHICS_RECT		srcRect     = {0, };
	    MMP_LDC_ATTR            ldcAttr;
		MMP_LDC_RES_MODE		sResMode 	= m_LdcResMode;

	    /* Bypass LDC Engine */
	    ldcAttr.bLdcEnable = MMP_FALSE;
	    MMPF_LDC_SetAttribute(&ldcAttr);
	
		/* Update Scaler Engine */
    	fitrange.fitmode 		= MMP_SCAL_FITMODE_OPTIMAL;
    	fitrange.scalerType 	= MMP_SCAL_TYPE_SCALER;
    	fitrange.ulInWidth 		= m_ulLdcOutFrameWidth;
        fitrange.ulInHeight 	= m_ulLdcOutFrameHeight;
    	fitrange.ulOutWidth 	= m_ulLdcOutFrameWidth;
        fitrange.ulOutHeight 	= m_ulLdcOutFrameHeight;

    	fitrange.ulInGrabX 		= 1;
        fitrange.ulInGrabY 		= 1;
        fitrange.ulInGrabW 		= fitrange.ulInWidth;
        fitrange.ulInGrabH 		= fitrange.ulInHeight;

		MMPF_Scaler_GetGCDBestFitScale(&fitrange, &grabctl);

		MMPF_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, 
						      (MMP_SCAL_PIPEID)ubSwiPipe, 
							  &fitrange, 
							  &grabctl);
		
		MMPF_Scaler_SetLPF((MMP_SCAL_PIPEID)ubSwiPipe, &fitrange, &grabctl);

		/* Update Icon Engine */
		icoAttr.inputsel 	= (MMP_SCAL_PIPEID)ubSwiPipe;
		icoAttr.bDlineEn	= MMP_TRUE;
		icoAttr.usFrmWidth	= fitrange.ulOutWidth;
		MMPF_Icon_SetDLAttributes((MMP_ICO_PIPEID)ubSwiPipe, &icoAttr);
		MMPF_Icon_SetDLEnable((MMP_ICO_PIPEID)ubSwiPipe, MMP_TRUE);
		
		/* Update IBC Engine */
    	ibcAttr.ulBaseAddr		= glPreviewBufAddr[ubSwiPipe][gbIBCCurBufIdx[ubSwiPipe]];
    	ibcAttr.ulBaseUAddr 	= glPreviewUBufAddr[ubSwiPipe][gbIBCCurBufIdx[ubSwiPipe]];
    	ibcAttr.ulBaseVAddr 	= glPreviewVBufAddr[ubSwiPipe][gbIBCCurBufIdx[ubSwiPipe]];
		ibcAttr.colorformat 	= MMP_IBC_COLOR_NV12;
		ibcAttr.usBufWidth		= fitrange.ulOutWidth;
		ibcAttr.ulLineOffset	= 0;
		ibcAttr.ulCbrLineOffset	= 0;
        ibcAttr.InputSource     = (MMP_ICO_PIPEID)ubSwiPipe;
    	ibcAttr.function 		= MMP_IBC_FX_TOFB;
    	ibcAttr.bMirrorEnable   = MMP_FALSE;
		MMPF_IBC_SetAttributes(ubSwiPipe, &ibcAttr);
        MMPF_IBC_SetStoreEnable(ubSwiPipe, MMP_TRUE);
		
		/* Update Graphics Engine */
		srcBuf.usWidth 		    = fitrange.ulInWidth;
		srcBuf.usHeight		    = fitrange.ulInHeight;
		srcBuf.usLineOffset	    = srcBuf.usWidth;
		srcBuf.colordepth		= MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
		srcBuf.ulBaseAddr		= m_LdcMultiSliceInfo.ulLdcOutStoreYAddr[gbLoopBackDoneBufIdx];
		srcBuf.ulBaseUAddr		= m_LdcMultiSliceInfo.ulLdcOutStoreUAddr[gbLoopBackDoneBufIdx];
		srcBuf.ulBaseVAddr		= m_LdcMultiSliceInfo.ulLdcOutStoreVAddr[gbLoopBackDoneBufIdx];

    	srcRect.usLeft			= 0;
    	srcRect.usTop			= 0;
    	srcRect.usWidth 		= srcBuf.usWidth;
    	srcRect.usHeight		= srcBuf.usHeight;

		if (sResMode == MMP_LDC_RES_MODE_MS_736P)
			MMPF_Graphics_SetPixDelay(15, 20);
		else
			MMPF_Graphics_SetPixDelay(13, 20);
	
		MMPF_Graphics_SetScaleAttr(&srcBuf, &srcRect, 1);
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_GetMultiSliceNum
//  Description :
//------------------------------------------------------------------------------
MMP_USHORT MMPF_LDC_GetMultiSliceNum(void)
{
	if (m_LdcRunMode == MMP_LDC_RUN_MODE_MULTI_SLICE)
		return m_LdcMultiSliceInfo.usSliceNum;
	else
		return 0;
}
#endif //(SUPPORT_LDC_RECD)||(SUPPORT_LDC_CAPTURE)

#endif

/** @}*/ //end of MMPF_LDC
