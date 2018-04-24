/**
 @file mmps_jstream.h
 @brief Header File for JPEG Stream Control
 @author Alterman
 @version 1.0
*/

#ifndef _MMPS_JSTREAM_H_
#define _MMPS_JSTREAM_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_fctl.h"

#include "ipc_cfg.h"

/** @addtogroup MMPS_JSTREAM
@{
*/

//==============================================================================
//
//                               OPTIONS
//
//==============================================================================

#define DBG_JSTREAM_MEM_MAP     (0)

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
} MMPS_JSTREAM_MEMBLK;

/* Stream Properties */
typedef struct {
    // Stream properties
    MMP_UBYTE               snrID;      ///< input sensor ID
    MMP_ULONG               w;          ///< JPEG width
    MMP_ULONG               h;          ///< JPEG height
    MMP_ULONG               size;       ///< JPEG target size
    MMP_ULONG               bufsize;    ///< Compress buffer size
} MMPS_JSTREAM_PROPT;

/*
 * Stream Class
 */
typedef struct {
    /*
     * Public members
     */
    MMPS_JSTREAM_PROPT      propt;      ///< stream properties

    /*
     * Private members
     */
    // Stream state
    IPC_STREAMER_STATE      state;      ///< stream state
    // Stream capability
    ST_JPEG_CAP             *cap;       ///< capability by ipc_cfg.c
    // Stream pipe
    MMP_BOOL                pipe_en;    ///< is pipe activated
    MMPD_FCTL_LINK          pipe;       ///< pipe path used
    // Stream buffers
    MMPS_JSTREAM_MEMBLK     heap;       ///< heap of stream to allocate buffers
    MMPS_JSTREAM_MEMBLK     linebuf;    ///< Line buffers
    MMPS_JSTREAM_MEMBLK     compbuf;    ///< Compress buffer
    // Stream encoder
    MMP_ULONG               id;         ///< associated ID
} MMPS_JSTREAM_CLASS;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================

int    MMPS_JStream_ModInit(void);
MMPS_JSTREAM_CLASS *MMPS_JStream_Open(MMPS_JSTREAM_PROPT *feat);
MMP_ERR MMPS_JStream_Start( MMPS_JSTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt);
MMP_ERR MMPS_JStream_Stop(MMPS_JSTREAM_CLASS *obj);
MMP_ERR MMPS_JStream_Close(MMPS_JSTREAM_CLASS *obj);

/// @}

#endif //_MMPS_JSTREAM_H_

