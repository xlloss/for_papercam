//==============================================================================
//
//                              INCLUDE FILE
//
//============================================================================== 
 
#include "mmp_lib.h"
#include "lib_retina.h"
#include "hdr_cfg.h"
#include "os_wrap.h"
#include "mmp_reg_mci.h"
#include "mmpf_mci.h"
#include "mmpf_monitor.h"

/** @addtogroup MMPF_MCI
 *  @{
 */

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_MCI_ISR(void)
{
    #if (MEM_MONITOR_EN)
    MMPF_Monitor_ISR();
    #endif
}

/* #pragma arm section code = "MCIdepth", rwdata = "MCIdepth",  zidata = "MCIdepth" */
//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_SetQueueDepth
//  Description :
//------------------------------------------------------------------------------
extern OS_CPU_SR OS_CPU_SR_Save_forThumb(void);
extern void OS_CPU_SR_Restore_forThumb(OS_CPU_SR cpu_sr);

MMP_ERR MMPF_MCI_SetQueueDepth(MMP_USHORT usQdepth)
{
    AITPS_MCI pMCI = AITC_BASE_MCI;

    OS_CRITICAL_INIT();

   OS_ENTER_CRITICAL_forThumb();
   /* OS_ENTER_CRITICAL(); */

	if (usQdepth == 4)
		pMCI->MCI_FB_DRAM_CTL |= (MCI_WR_Q_NUM_4 | MCI_RD_Q_NUM_4);
	else if (usQdepth == 8)
		pMCI->MCI_FB_DRAM_CTL &= ~(MCI_WR_Q_NUM_4 | MCI_RD_Q_NUM_4);

   OS_EXIT_CRITICAL_forThumb();
   /* OS_EXIT_CRITICAL(); */

	return MMP_ERR_NONE;
}
/* #pragma arm section code, rwdata,  zidata */

//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_SetUrgentEnable
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_MCI_SetUrgentEnable(MMP_UBYTE ubSrc, MMP_BOOL ubEn)
{
	AITPS_MCI pMCI = AITC_BASE_MCI;
	
	if (ubEn == MMP_TRUE)
		pMCI->MCI_URGENT_EN |= (1 << ubSrc);
	else
		pMCI->MCI_URGENT_EN &= ~(1 << ubSrc);
		
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_SetIBCMaxPriority
//  Description :
//------------------------------------------------------------------------------
void MMPF_MCI_SetIBCMaxPriority(MMP_UBYTE ubPipe)
{
	AITPS_MCI pMCI = AITC_BASE_MCI;
	MMP_UBYTE src;
	
    switch (ubPipe)
    {
    	case 0:
    		src = MCI_SRC_IBC0;
    	break;
    	case 1:
    		src = MCI_SRC_IBC1;
    	break;
    	case 2:
    		src = MCI_SRC_IBC2;
    	break;
    	case 3:
    		src = MCI_SRC_LCD1_IBC3;
    	break;
    	case 4:
    	default:
    		src = MCI_SRC_LCD2_CCIR1_IBC4;
    	break;
    }

    pMCI->MCI_INIT_DEC_VAL[src] = MCI_INIT_WT_MAX;
    pMCI->MCI_NA_INIT_DEC_VAL[src] = MCI_NA_INIT_WT_MAX;
    pMCI->MCI_ROW_INIT_DEC_VAL[src] = MCI_ROW_INIT_WT_MAX;
    pMCI->MCI_RW_INIT_DEC_VAL[src] = MCI_RW_INIT_WT_MAX;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_SetModuleMaxPriority
//  Description :
//------------------------------------------------------------------------------
void MMPF_MCI_SetModuleMaxPriority(MMP_UBYTE ubSrc)
{
	AITPS_MCI       pMCI = AITC_BASE_MCI;
	MMPF_MCI_SRC    src = ubSrc;

    pMCI->MCI_INIT_DEC_VAL[src] = MCI_INIT_WT_MAX;
    pMCI->MCI_NA_INIT_DEC_VAL[src] = MCI_NA_INIT_WT_MAX;
    pMCI->MCI_ROW_INIT_DEC_VAL[src] = MCI_ROW_INIT_WT_MAX;
    pMCI->MCI_RW_INIT_DEC_VAL[src] = MCI_RW_INIT_WT_MAX;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MCI_TunePriority
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_MCI_TunePriority(MMPF_MCI_TUNE_MODE mciMode)
{
    AITPS_MCI pMCI = AITC_BASE_MCI;
	MMP_UBYTE src = 0;

    if (MCI_GET_MODE(mciMode) == MCI_MODE_DMAR_H264)
    {
        pMCI->MCI_FB_DRAM_CTL |= MCI_FB_REQ_WITH_LOCK;

		// Icon sticker
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_INIT_WT_MAX;

		// IBC
		MMPF_MCI_SetIBCMaxPriority(MCI_GET_PIPE(mciMode));

		// H264 cur/ref : For H264 OV issue. Tune 264 priority 
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_INIT_WT_MAX | MCI_DEC_VAL(1);
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_RW_INIT_WT_MAX;

		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_INIT_WT_MAX | MCI_DEC_VAL(1);
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_RW_INIT_WT_MAX;

        // LCD display
        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_INIT_WT_MAX -1;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_RW_INIT_WT_MAX;

        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_INIT_WT_MAX - 1;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_RW_INIT_WT_MAX;

        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_INIT_WT_MAX - 1;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_RW_INIT_WT_MAX;	    	    	    
    }
    else if (MCI_GET_MODE(mciMode) == MCI_MODE_RAW)
    {
		pMCI->MCI_FB_DRAM_CTL |= MCI_FB_REQ_WITH_LOCK;

		// RAW store
		if (MCI_GET_RAWS(mciMode) == 0)
		    src = MCI_SRC_RAWS0;
		else if (MCI_GET_RAWS(mciMode) == 1)
		    src = MCI_SRC_RAWS1_JPGLB;

	    pMCI->MCI_INIT_DEC_VAL[src] = MCI_INIT_WT_MAX;
	    pMCI->MCI_NA_INIT_DEC_VAL[src] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[src] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[src] = MCI_RW_INIT_WT_MAX;

        #if defined(ALL_FW)
		if (gsHdrCfg.bVidEnable && 
			gsHdrCfg.ubMode == HDR_MODE_STAGGER)
		{
			extern MMP_UBYTE m_ubHdrMainRawId;
			
			if ((gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2ENGINE) ||
				(gsHdrCfg.ubVcStoreMethod == HDR_VC_STORE_2PLANE && m_ubHdrMainRawId == 1/*MMP_RAW_MDL_ID1*/))
			{
				pMCI->MCI_INIT_DEC_VAL[MCI_SRC_RAWS1_JPGLB] = MCI_INIT_WT_MAX;
            	pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_RAWS1_JPGLB] = MCI_NA_INIT_WT_MAX;
            	pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_RAWS1_JPGLB] = MCI_ROW_INIT_WT_MAX;
            	pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_RAWS1_JPGLB] = MCI_RW_INIT_WT_MAX;
			}
		}
		#endif

		// Raw fetch
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_RAW_F] = (MCI_INIT_WT_MAX-2) | MCI_DEC_VAL(1);

	    // Icon sticker
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_INIT_WT_MAX;
	    pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_RW_INIT_WT_MAX;

        // IBC
        MMPF_MCI_SetIBCMaxPriority(MCI_GET_PIPE(mciMode));

		// H264 cur/ref
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_INIT_WT_MAX | MCI_DEC_VAL(1);
    }
    else if (MCI_GET_MODE(mciMode) == MCI_MODE_GRA_LDC_H264)
    {
		pMCI->MCI_FB_DRAM_CTL |= MCI_FB_REQ_WITH_LOCK;

		// RAW store
		#if defined(ALL_FW)
		if (gsHdrCfg.bVidEnable && 
			gsHdrCfg.ubMode == HDR_MODE_STAGGER)
		{
		    pMCI->MCI_INIT_DEC_VAL[MCI_SRC_RAWS0] = MCI_INIT_WT_MAX;
		    pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_RAWS0] = MCI_NA_INIT_WT_MAX;
		    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_RAWS0] = MCI_ROW_INIT_WT_MAX;
		    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_RAWS0] = MCI_RW_INIT_WT_MAX;
	    }
	    #endif

	    // Icon sticker
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_DMAR1_ICON_SIF] = MCI_INIT_WT_MAX;

        // IBC
        MMPF_MCI_SetIBCMaxPriority(MCI_GET_PIPE(mciMode));

		if (MCI_GET_2NDPIPE(mciMode) == 1)
		{
			MMPF_MCI_SetIBCMaxPriority(1);
	    }
		else if (MCI_GET_2NDPIPE(mciMode) == 2)
		{
	        MMPF_MCI_SetIBCMaxPriority(2);
	    }

		#if (VRP_DEMO_EN == 0)
		// H264 cur/ref
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_H264_1] = MCI_RW_INIT_WT_MAX;  

		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_H264_2] = MCI_RW_INIT_WT_MAX;

		// Graphics
		pMCI->MCI_INIT_DEC_VAL[MCI_SRC_GRA] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_GRA] = MCI_NA_INIT_WT_MAX;
	    pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_GRA] = MCI_ROW_INIT_WT_MAX;
	    pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_GRA] = MCI_RW_INIT_WT_MAX;
    	#endif
    }
    else if (MCI_GET_MODE(mciMode) == MCI_MODE_GRA_DECODE_H264)
    {
    	// IBC
        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_IBC0] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_IBC0] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_IBC0] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_IBC0] = MCI_RW_INIT_WT_MAX;  

        // LCD display : For HDMI frame shifted in video play issue
        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD1_IBC3] = MCI_RW_INIT_WT_MAX;

        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD2_CCIR1_IBC4] = MCI_RW_INIT_WT_MAX;

        pMCI->MCI_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_INIT_WT_MAX;
        pMCI->MCI_NA_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_NA_INIT_WT_MAX;
        pMCI->MCI_ROW_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_ROW_INIT_WT_MAX;
        pMCI->MCI_RW_INIT_DEC_VAL[MCI_SRC_LCD3_CCIR2] = MCI_RW_INIT_WT_MAX;
    }
    else if (MCI_GET_MODE(mciMode) == MCI_MODE_DEFAULT)
    {
    	MMP_ULONG src = 0;

    	for (src = 0; src < MCI_SRC_NUM; src++) {
    		pMCI->MCI_INIT_DEC_VAL[src]  		= MCI_INIT_DEC_DEFT_VAL;
    		pMCI->MCI_DEC_THD[src]  	 		= MCI_WT_DEC_DEFT_VAL;
    		pMCI->MCI_NA_INIT_DEC_VAL[src]  	= MCI_NA_INIT_DEFT_VAl;
    		pMCI->MCI_ROW_INIT_DEC_VAL[src]  	= MCI_ROW_INIT_DEFT_VAl;
    		pMCI->MCI_RW_INIT_DEC_VAL[src]  	= MCI_RW_INIT_DEFT_VAl;
    	}
    	pMCI->MCI_FB_DRAM_CTL &= ~(MCI_FB_REQ_WITH_LOCK);
    }

    return MMP_ERR_NONE;
}

/// @}
/// @end_ait_only
