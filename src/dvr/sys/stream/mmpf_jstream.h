/**
 @file mmpf_jstream.h
 @brief Header function of JPEG Streamer
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_JSTREAM_H_
#define _MMPF_JSTREAM_H_

#include "includes_fw.h"
#include "aitu_ringbuf.h"
#include "mmp_dsc_inc.h"
#include "ipc_cfg.h"

/** @addtogroup MMPF_JSTREAM
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define JSTREAM_INFO_Q_DEPTH    (32)  ///< Queue depth for encoded frame info.

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================
/*
 * JPEG Stremer Status
 */
typedef enum {
    JSTREAM_STATE_NONE = 0,
    JSTREAM_STATE_OPEN,
    JSTREAM_STATE_START,
    JSTREAM_STATE_STOP,
    JSTREAM_STATE_INVALID
} MMPF_JSTREAM_STATE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/*
 * Information of one frame
 */
typedef struct {
    MMP_ULONG   addr;       ///< frame data address
    MMP_ULONG   size;       ///< frame size
    MMP_ULONG   time;       ///< frame timestamp
} MMPF_JSTREAM_FRAMEINFO;

/*
 * Queue for keeping the information of frames
 */
typedef struct {
    // data buffer to keep the info. of the encoded frames
    MMPF_JSTREAM_FRAMEINFO  data[JSTREAM_INFO_Q_DEPTH];
    // ring queue maintenance
    AUTL_RINGBUF            ring;
} MMPF_JSTREAM_INFO_Q;

/*
 * JStream Class
 */
typedef struct {
    MMPF_JSTREAM_STATE      state;      ///< streamer status
    MMPF_JSTREAM_INFO_Q     info_q;     ///< frame info queue
    IPC_STREAMER_OPTION     opt;        ///< streaming option
    MMP_ULONG               v4l2_id;    ///< mapping to V4L2 stream id
} MMPF_JSTREAM_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ULONG   MMPF_JStream_New(void);
void        MMPF_JStream_Delete(MMP_ULONG id);
MMP_ERR     MMPF_JStream_Start( MMP_ULONG           id,
                                MMP_ULONG           v4l2_id,
                                IPC_STREAMER_OPTION opt);
MMP_ERR     MMPF_JStream_Stop(MMP_ULONG id);
void        MMPF_JStream_TriggerEncode(MMP_ULONG id);
void        MMPF_JStream_TransferData(MMP_ULONG id);
MMP_ERR     MMPF_JStream_GetFrame(MMP_ULONG id,MMP_ULONG *jpg_addr , MMP_ULONG *jpg_size);

/// @}

#endif  // _MMPF_JSTREAM_H_
