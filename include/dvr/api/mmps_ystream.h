/**
 @file mmps_ystream.h
 @brief Header File for Gray Stream Control
 @author Alterman
 @version 1.0
*/

#ifndef _MMPS_YSTREAM_H_
#define _MMPS_YSTREAM_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_fctl.h"

#include "ipc_cfg.h"

/** @addtogroup MMPS_YSTREAM
@{
*/

//==============================================================================
//
//                               OPTIONS
//
//==============================================================================

#define DBG_YSTREAM_MEM_MAP     (0)

//==============================================================================
//
//                               CONSTANT
//
//==============================================================================

#define GRAY_FRM_SLOTS          (2)

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
} MMPS_YSTREAM_MEMBLK;

/* Stream Properties */
typedef struct {
    // Stream properties
    MMP_UBYTE       snrID;      ///< Input sensor ID
    MMP_ULONG       w;          ///< Video width
    MMP_ULONG       h;          ///< Video height
    MMP_ULONG       fmt ;       ///< Video format
} MMPS_YSTREAM_PROPT;

/*
 * Stream Class
 */
typedef struct {
    /*
     * Public members
     */
    MMPS_YSTREAM_PROPT      propt;      ///< stream properties

    /*
     * Private members
     */
    // Stream state
    IPC_STREAMER_STATE      state;      ///< stream state
    // Stream capability
    ST_GRAY_CAP             *cap;       ///< capability by ipc_cfg.c
    // Stream pipe
    MMP_BOOL                pipe_en;    ///< is pipe activated
    MMPD_FCTL_LINK          pipe;       ///< pipe path used
    // Stream buffers
    MMPS_YSTREAM_MEMBLK     heap;       ///< heap of stream to allocate buffers
    MMPS_YSTREAM_MEMBLK     frmbuf[GRAY_FRM_SLOTS]; ///< Input buffers info.
    // Streamer ID
    MMP_ULONG               id;         ///< associated ID
} MMPS_YSTREAM_CLASS;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================
int    MMPS_YStream_ModInit(void);
MMPS_YSTREAM_CLASS *MMPS_YStream_Open(MMPS_YSTREAM_PROPT *feat);
MMP_ERR MMPS_YStream_Start( MMPS_YSTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id);
MMP_ERR MMPS_YStream_Stop(MMPS_YSTREAM_CLASS *obj);
MMP_ERR MMPS_YStream_Close(MMPS_YSTREAM_CLASS *obj);

/// @}

#endif //_MMPS_YSTREAM_H_

