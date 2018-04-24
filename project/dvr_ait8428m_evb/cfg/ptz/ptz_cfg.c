//------------------------------------------------------------------------------
//
//  File        : ptz_cfg.c
//  Description : Source file of Pan/Tilt/Zoom configuration
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
#include "ptz_cfg.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

/*
 * Configure of Ptz
 */
PTZ_CFG gsVidPtzCfg = {
    MMP_FALSE, 	// bEnable
    16,   		// usMaxZoomRatio
    400,     	// usMaxZoomSteps
    200,    	// sMaxPanSteps
    -200,   	// sMinPanSteps
    200,		// sMaxTiltSteps
	-200		// sMinTiltSteps
};

PTZ_CFG gsDscPtzCfg = {
    MMP_FALSE, 	// bEnable
    16,   		// usMaxZoomRatio
    400,     	// usMaxZoomSteps
    200,    	// sMaxPanSteps
    -200,   	// sMinPanSteps
    200,		// sMaxTiltSteps
	-200		// sMinTiltSteps
};

MMP_BOOL MMP_IsVidPtzEnable(void)
{
    return gsVidPtzCfg.bEnable;
}

MMP_BOOL MMP_IsDscPtzEnable(void)
{
    return gsDscPtzCfg.bEnable;
}

