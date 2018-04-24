/**
 @file mmpf_vstream.h
 @brief Header function of Video Streamer
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_VSTREAM_H_
#define _MMPF_VSTREAM_H_

#include "includes_fw.h"
#include "mmpf_vidcmn.h"
#include "mmp_mux_inc.h"
#include "mmp_ipc_inc.h"

/** @addtogroup MMPF_VSTREAM
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define VSTREAM_INFO_Q_DEPTH    (2048)  ///< Queue depth for encoded frame info.

/*
#define VSTREAM_RT_CONUS_I      (1)     ///< Number of contiguous I-frame
*/
// change to 1 to prevent too many latency
#define VSTREAM_RT_SKIP_THR     (1) // (30)    ///< Maximum 30 frames queued in buffer

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================
/*
 * Video Stremer Status
 */
typedef enum {
    VSTREAM_STATE_NONE = 0,
    VSTREAM_STATE_OPEN,
    VSTREAM_STATE_INVALID
} MMPF_VSTREAM_STATE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/*
 * Ring buffer
 */
typedef struct {
    MMP_ULONG   base;       ///< start address of ring buffer
    MMP_ULONG   size;       ///< size of ring buffer

    MMP_ULONG   rd_ptr;     ///< offset to read from ring buffer
    MMP_ULONG   wr_ptr;     ///< offset to write into ring buffer
    MMP_ULONG   rd_wrap;    ///< wrap count for ring buffer reading
    MMP_ULONG   wr_wrap;    ///< wrap count for ring buffer writting
} MMPF_VSTREAM_RINGBUF;

/*
 * Information of one frame
 */
typedef struct {
    MMP_ULONG   size;       ///< frame size
    MMP_ULONG   time;       ///< frame timestamp
    #if (WORK_AROUND_EP3)
    MMP_USHORT  type;       ///< frame type: I, P, or B
    MMP_USHORT  ep3;        ///< is EP3 inside the frame
    #else
    MMP_ULONG   type;
    #endif
} MMPF_VSTREAM_FRAMEINFO;

/*
 * Queue for keeping the information of frames
 */
typedef struct {
    // data buffer to keep the info. of the encoded frames
    MMPF_VSTREAM_FRAMEINFO  data[VSTREAM_INFO_Q_DEPTH];
    // ring queue maintenance
    MMPF_VSTREAM_RINGBUF    ring;
} MMPF_VSTREAM_INFO_Q;

/*
 * VStream Class
 */
typedef struct {
    MMPF_VSTREAM_STATE      state;      ///< streamer status
    MMPF_VSTREAM_INFO_Q     info_q;     ///< frame info queue
    MMPF_VSTREAM_RINGBUF    compbuf;    ///< compress buffer for video data
    IPC_STREAMER_OPTION     opt;        ///< streaming option
    IPC_STREAMER_MODE       mode;       ///< streaming mode
    MMP_ULONG               tx_size;    ///< transfer size
    MMP_ULONG               ts;         ///< timestamp
    MMP_ULONG               v4l2_id;    ///< mapping to V4L2 stream id
    MMP_ULONG               ts_rampup ;  ///< timestamp to log ramp-up
} MMPF_VSTREAM_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
void        MMPF_VStream_SetCompBuf(MMP_ULONG id,
                                    MMP_ULONG base,
                                    MMP_ULONG size);
MMP_ULONG   MMPF_VStream_CompBufAddr2WriteFrame(MMP_ULONG id);
void        MMPF_VStream_SaveFrameInfo( MMP_ULONG               id, 
                                        MMPF_VSTREAM_FRAMEINFO  *info);
MMP_BOOL    MMPF_VStream_ShouldFrameSkip(MMP_ULONG id, MMP_ULONG ulThreshold);
MMP_ERR     MMPF_VStream_TransferData(MMP_ULONG id);

MMP_ERR     MMPF_VStream_Start( MMP_UBYTE           encid,
                                MMP_ULONG           v4l2_id,
                                IPC_STREAMER_OPTION opt);
MMP_ERR     MMPF_VStream_Stop(MMP_UBYTE encid);
void        MMPF_VStream_SetStreamRTStartIFrames(MMP_USHORT cnt);

/// @}

#endif  // _MMPF_VSTREAM_H_
