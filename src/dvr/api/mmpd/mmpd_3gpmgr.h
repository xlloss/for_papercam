/// @ait_only
/**
 @file mmpd_3gpmgr.h
 @brief Header File for the Host 3GP MERGER Driver.
 @author Will Tseng
 @version 1.0
*/

#ifndef _MMPD_3GPMGR_H_
#define _MMPD_3GPMGR_H_

#include "includes_fw.h"
#include "mmp_mux_inc.h"

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
/** @addtogroup MMPD_3GPMGR
 *  @{
 */
MMP_ERR MMPD_3GPMGR_StartCapture(MMP_ULONG ubEncId);
MMP_ERR MMPD_3GPMGR_StopCapture(MMP_ULONG ubEncId);
MMP_ERR MMPD_3GPMGR_SetEncodeCompBuf(   MMP_ULONG   ubEncId,
                                        MMP_ULONG   addr,
                                        MMP_ULONG   size);

#endif // _INCLUDES_H_
/// @end_ait_only
