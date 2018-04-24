//------------------------------------------------------------------------------
//
//  File        : hdr_cfg.c
//  Description : Source file of HDR configuration
//  Author      : Eroy
//  Revision    : 0.1
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "Customer_config.h"
#include "hdr_cfg.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

/*
 * Configure of HDR
 */
HDR_CFG gsHdrCfg = {
    MMP_FALSE,          	// bVidEnable
    MMP_FALSE,          	// bDscEnable
    HDR_MODE_STAGGER,   	// ubMode
    MMP_TRUE,     			// bRawGrabEnable
    HDR_BITMODE_8BIT,   	// ubRawStoreBitMode
    #if 1
    HDR_VC_STORE_2PLANE,	// ubVcStoreMethod
    #else
    HDR_VC_STORE_2ENGINE,	// ubVcStoreMethod
	#endif
	2						// ubBktFrameNum
};

void MMP_EnableVidHDR(MMP_BOOL bEnable)
{
	gsHdrCfg.bVidEnable = bEnable;
}

void MMP_EnableDscHDR(MMP_BOOL bEnable)
{
	gsHdrCfg.bDscEnable = bEnable;
}

MMP_BOOL MMP_IsVidHDREnable(void)
{
    return gsHdrCfg.bVidEnable;
}

MMP_BOOL MMP_IsDscHDREnable(void)
{
    return gsHdrCfg.bDscEnable;
}
