/**
 @file mmpf_ystream.h
 @brief Header function of Gray Streamer
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_YSTREAM_H_
#define _MMPF_YSTREAM_H_

#include "mmp_ipc_inc.h"

/** @addtogroup MMPF_YSTREAM
@{
*/

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
/*
 * Gray Stremer Status
 */
typedef enum {
    YSTREAM_STATE_NONE = 0,
    YSTREAM_STATE_IDLE,
    YSTREAM_STATE_OPEN,
    YSTREAM_STATE_INVALID
} MMPF_YSTREAM_STATE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

/*
 * VStream Class
 */
typedef struct {
    MMPF_YSTREAM_STATE      state;      ///< streamer status
    MMP_ULONG               frm;        ///< gray frame address
    MMP_ULONG               frm_size;   ///< gray frame size
    MMP_ULONG               ts;         ///< gray frame timestamp
    MMP_ULONG               v4l2_id;    ///< mapping to V4L2 stream id
} MMPF_YSTREAM_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
void        MMPF_YStream_FrameRdy(MMP_ULONG id, MMP_ULONG frm, MMP_ULONG ts);
MMP_ERR     MMPF_YStream_TransferData(MMP_ULONG id);

MMP_ULONG   MMPF_YStream_New(MMP_ULONG w, MMP_ULONG h,MMP_BOOL y_only);
void        MMPF_YStream_Delete(MMP_ULONG id);
MMP_ERR     MMPF_YStream_Start(MMP_ULONG id, MMP_ULONG v4l2_id);
MMP_ERR     MMPF_YStream_Stop(MMP_ULONG id);

/// @}

#endif  // _MMPF_YSTREAM_H_
