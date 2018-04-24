/**
 @file mmps_astream.h
 @brief Header File for Audio Stream Control
 @author Alterman
 @version 1.0
*/

#ifndef _MMPS_ASTREAM_H_
#define _MMPS_ASTREAM_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"

#include "ipc_cfg.h"

/** @addtogroup MMPS_ASTREAM
@{
*/

//==============================================================================
//
//                               OPTIONS
//
//==============================================================================

#define DBG_ASTREAM_MEM_MAP     (0)

//==============================================================================
//
//                               ENUMERATION
//
//==============================================================================


//==============================================================================
//
//                               STRUCTURES
//
//==============================================================================
/*
 * Stream Memory Block
 */
typedef struct {
    MMP_ULONG   base;       ///< Base address of memory block
    MMP_ULONG   end;        ///< End address of memory block
    MMP_ULONG   size;       ///< Size of memory block
} MMPS_ASTREAM_MEMBLK;

/*
 * Stream Properties
 */
typedef struct {
    ST_AUDIO_CAP    feat;       ///< Buf size to keep audio for 'buftime' ms
    MMP_BOOL        seamless;   ///< Seamless streaming mode
} MMPS_ASTREAM_PROPT;

/*
 * Stream Class
 */
typedef struct {
    /*
     * Public members
     */
    // Stream state
    IPC_STREAMER_STATE      state;      ///< stream state

    /*
     * Private members
     */
    // Stream capability
    ST_AUDIO_CAP            *cap;       ///< capability by ipc_cfg.c
    // Stream Properties
    MMPS_ASTREAM_PROPT      propt;      ///< capability by ipc_cfg.c
    // Stream buffers
    MMPS_ASTREAM_MEMBLK     heap;       ///< heap of stream to allocate buffers
    // Streamer
    MMP_ULONG               id;         ///< associated streamer ID
} MMPS_ASTREAM_CLASS;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================

int    MMPS_AStream_ModInit(void);
MMPS_ASTREAM_CLASS *MMPS_AStream_Open(MMPS_ASTREAM_PROPT *feat);
MMP_ERR MMPS_AStream_Start( MMPS_ASTREAM_CLASS *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt);
MMP_ERR MMPS_AStream_Stop(MMPS_ASTREAM_CLASS *obj);
MMP_ERR MMPS_AStream_Close(MMPS_ASTREAM_CLASS *obj);

/// @}

#endif //_MMPS_ASTREAM_H_

