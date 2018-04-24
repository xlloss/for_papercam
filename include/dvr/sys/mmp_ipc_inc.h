//------------------------------------------------------------------------------
//
//  File        : ipc_cfg.h
//  Description : Header file of IP Camera Common Data Types
//  Author      : Alterman
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

#ifndef _MMP_IPC_INC_H_
#define _MMP_IPC_INC_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

/*
 * Streamer handle option
 */
typedef enum {
    IPC_STREAMER_OPT_NONE   = 0x00,
    IPC_STREAMER_OPT_FLUSH  = 0x01
} IPC_STREAMER_OPTION;

/*
 * Streaming mode option
 */
typedef enum {
    IPC_STREAMER_MODE_NONE  = 0x00,
    IPC_STREAMER_MODE_RT    = 0x01,  ///< real-time streaming
    IPC_STREAMER_MODE_RAMPUP_BR = 0x02, /// < ramp up bitrate to reduce impact of startup
    
} IPC_STREAMER_MODE;

/*
 * Streamer State
 *              open            start
 *   [ IDLE ] ------> [ OPEN ] ------> [ START ]
 *       ^              | ^                |
 *       |_____________ | |________________|
 *            close              stop
 */
typedef enum {
    IPC_STREAMER_IDLE = 0,
    IPC_STREAMER_OPEN,
    IPC_STREAMER_START,
    IPC_STREAMER_STATE_UNKNOWN
} IPC_STREAMER_STATE;

/*
 * Video stream control
 */
typedef enum {
    IPC_VIDCTL_BITRATE = 0,
    IPC_VIDCTL_GOP,
    IPC_VIDCTL_ENC_FPS,
    IPC_VIDCTL_FORCE_I,
    IPC_VIDCTL_RC_MODE,
    IPC_VIDCTL_RC_SKIP,
    IPC_VIDCTL_RC_SKIP_TYPE,
    IPC_VIDCTL_QP_INIT,
    IPC_VIDCTL_QP_BOUND,
    IPC_VIDCTL_LB_SIZE,
    IPC_VIDCTL_PROFILE,
    IPC_VIDCTL_LEVEL,
    IPC_VIDCTL_ENTROPY,
    IPC_VIDCTL_TNR,
    IPC_VIDCTL_NUM
} IPC_VIDEO_CTL;

/*
 * ALSA control
 */
typedef enum {
    IPC_ALSA_CTL_MUTE = 0,  ///< mute control
    IPC_ALSA_CTL_A_GAIN,    ///< analog gain control
    IPC_ALSA_CTL_D_GAIN,    ///< digital gain control
    IPC_ASLA_CTL_NUM
} IPC_ALSA_CTL;

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================


//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================


#endif //_MMP_IPC_INC_H_
