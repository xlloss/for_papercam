/**
 @file mmpf_astream.h
 @brief Header function of Audio Streamer
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_ASTREAM_H_
#define _MMPF_ASTREAM_H_

#include "includes_fw.h"
#include "mmpf_audio_ctl.h"
#include "mmp_aud_inc.h"
#include "aitu_ringbuf.h"
#include "ipc_cfg.h"

/** @addtogroup MMPF_ASTREAM
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define AUD_FRM_INFO_Q_DEPTH    (1024)

#define ASTREAM_RT_SKIP_THR     (30)    ///< Maximum 30 frames queued in buffer

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

/*
 * Audio Stremer Status
 */
typedef enum {
    ASTREAM_STATE_NONE = 0,
    ASTREAM_STATE_OPEN,
    ASTREAM_STATE_START,
    ASTREAM_STATE_INVALID
} MMPF_ASTREAM_STATE;

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
} MMPF_ASTREAM_RINGBUF;

/*
 * Audio frame information
 */
typedef struct {
    MMP_ULONG   size;       ///< frame size
    MMP_ULONG   dts;        ///< frame timestamp
} AUDIO_FRM_INFO;

/*
 * Audio frame information
 */
typedef struct {
    AUDIO_FRM_INFO  buf[AUD_FRM_INFO_Q_DEPTH];
    AUTL_RINGBUF    queue;
} MMPF_ASTREAM_FRMINFO_Q;

/*
 * AStream Class
 */
typedef struct {
    MMPF_ASTREAM_STATE      state;      ///< streamer status
    /* Properties */
    MMP_ASTREAM_FMT         fmt;        ///< Audio format
    MMP_ULONG               fs;         ///< Sample rate
    MMP_ULONG               bitrate;    ///< Bitrate
    /* Associated ADC module */
    MMPF_ADC_CLASS          *adc;       ///< Associated ADC module
    /*Associated audio encoder */
    MMP_AUD_ENCODER         enc;        ///< Associated encoder
    /* Buffers */
    MMP_AUD_ENCBUF          buf;        ///< Buffers
    AUTL_RINGBUF            inbuf;      ///< Input ring buffer handler
    AUTL_RINGBUF            outbuf;     ///< Output ring buffer handler
    /* Frame infor. */
    MMPF_ASTREAM_FRMINFO_Q  infobuf;    ///< Frame information queue    
    /* Streaming option */
    IPC_STREAMER_OPTION     opt;        ///< Streaming option
    IPC_STREAMER_MODE       mode;       ///< Streaming mode
    /* Transfer infor. */
    MMP_ULONG               tx_cnt;     ///< transfer frames count
    MMP_ULONG               ts;         ///< timestamp
    /* V4L2 association */
    MMP_ULONG               v4l2_id;    ///< mapping to V4L2 stream id
} MMPF_ASTREAM_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ULONG MMPF_AStream_New(ST_AUDIO_CAP *feat);
void MMPF_AStream_Delete(MMP_ULONG id);

MMP_ERR MMPF_AStream_OpenADC(MMP_ULONG id, MMP_AUD_INOUT_PATH path);
MMP_ERR MMPF_AStream_CloseADC(MMP_ULONG id);

MMP_ERR MMPF_AStream_OpenEncoder(MMP_ULONG id, MMP_AUD_ENCBUF *buf);
MMP_ERR MMPF_AStream_CloseEncoder(MMP_ULONG id);

MMP_ERR MMPF_AStream_Start( MMP_ULONG           id,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt);
MMP_ERR MMPF_AStream_Stop(MMP_ULONG id);

void MMPF_AStream_TransferData(MMP_ULONG id);
MMP_USHORT MMPF_AStream_GetADCFs(MMP_ULONG id);
MMP_USHORT MMPF_AStream_GetEncoderFs(MMP_ULONG id);

/// @}

#endif  // _MMPF_ASTREAM_H_
