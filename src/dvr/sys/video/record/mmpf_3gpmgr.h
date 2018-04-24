/**
 @file mmpf_3gpmgr.h
 @brief Header function of 3gp processor
 @author Will Tseng, Truman Yang
 @version 1.1 Add constant and struction section to sync with the host
 @version 1.0 Original Version
*/

#ifndef _MMPF_3GPMGR_H_
#define _MMPF_3GPMGR_H_

#include "includes_fw.h"
#include "mmp_mux_inc.h"
#include "mmpf_vidcmn.h"

//==============================================================================
//                                IPC SECTION
//==============================================================================
/** @addtogroup MMPF_3GPP
@{
*/

/** @name IPC
Inter Process Communication section.
All definition and declaration here are used for host MMP inter process communication.
This section should be sync with the host.
@{
*/
    /// @}
/** @} */ // end of 3GPP module
//==============================================================================
//
//                              COMPILER OPTION
//
//==============================================================================


//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================


//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

/// Video frame type
typedef enum _MMPF_3GPMGR_FRAME_TYPE {
    MMPF_3GPMGR_FRAME_TYPE_I = 0,
    MMPF_3GPMGR_FRAME_TYPE_P,
    MMPF_3GPMGR_FRAME_TYPE_B,
    MMPF_3GPMGR_FRAME_TYPE_MAX
} MMPF_3GPMGR_FRAME_TYPE;

typedef enum _MMPF_VIDMGR_TX_STAT {
    MMPF_VIDMGR_TX_STAT_NONE = 0,
    MMPF_VIDMGR_TX_STAT_OPEN,
    MMPF_VIDMGR_TX_STAT_CLOSE
} MMPF_VIDMGR_TX_STAT;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================


//==============================================================================
//
//                              DATA TYPES
//
//==============================================================================


//==============================================================================
//
//                              MODULATION
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

#endif
