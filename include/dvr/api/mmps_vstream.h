/**
 @file mmps_vstream.h
 @brief Header File for Video Stream Control
 @author Alterman
 @version 1.0
*/

#ifndef _MMPS_VSTREAM_H_
#define _MMPS_VSTREAM_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_fctl.h"
#include "mmpd_mp4venc.h"
#include "mmpf_vidcmn.h"

#include "ipc_cfg.h"

/** @addtogroup MMPS_VSTREAM
@{
*/

//==============================================================================
//
//                               OPTIONS
//
//==============================================================================

#define DBG_VSTREAM_MEM_MAP     (0)

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
} MMPS_VSTREAM_MEMBLK;

/* Stream Properties */
typedef struct {
    // Stream properties
    MMP_UBYTE       snrID;      ///< Input sensor ID
    MMP_BOOL        seamless;   ///< Seamless streaming mode
    MMP_ULONG       w;          ///< Video width
    MMP_ULONG       h;          ///< Video height
    MMP_VID_FPS     snr_fps;    ///< Sensor input frame rate
    MMP_ULONG       infrmslots; ///< Number of input frame buffer slots
    MMP_ULONG       outbuf_ms;  ///< Duration of outbuf queuing data(ms)
    // Control properties
    VIDENC_CTL_SET  ctl;        ///< Video control set
    // OSD properties
    MMP_BOOL OsdStrEnable;
    MMP_BOOL OsdRtcEnable;
    MMP_ULONG rtc_pos_startX;
    MMP_ULONG rtc_pos_startY;
    MMP_ULONG str_pos_startX;
    MMP_ULONG str_pos_startY;
    void *CallBackFunc;
    MMP_ULONG CallBackArgc;

} MMPS_VSTREAM_PROPT;

/*
 * Stream Class
 */
typedef struct {
    /*
     * Public members
     */
    MMPS_VSTREAM_PROPT      propt;      ///< stream properties

    /*
     * Private members
     */
    // Stream state
    IPC_STREAMER_STATE      state;      ///< stream state
    // Stream capability
    ST_VIDEO_CAP            *cap;       ///< capability by ipc_cfg.c
    // Stream pipe
    MMP_BOOL                pipe_en;    ///< is pipe activated
    MMPD_FCTL_LINK          pipe;       ///< pipe path used
    // Stream buffers
    MMPS_VSTREAM_MEMBLK     heap;       ///< heap of stream to allocate buffers
    MMPD_MP4VENC_INPUT_BUF  inbuf;      ///< Input buffers info.
    MMPS_VSTREAM_MEMBLK     refbuf;     ///< Reference frame buffer
    MMPS_VSTREAM_MEMBLK     genbuf;     ///< Reconstruct frame buffer
    MMPS_VSTREAM_MEMBLK     outbuf;     ///< Compress buffer
    MMPS_VSTREAM_MEMBLK     lenbuf;     ///< Slice length buffer
    MMPS_VSTREAM_MEMBLK     mvbuf;      ///< MV buffer
    // Stream encoder
    MMP_ULONG               enc_id;     ///< associated encoder ID
} MMPS_VSTREAM_CLASS;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================

int    MMPS_VStream_ModInit(void);
MMPS_VSTREAM_CLASS *MMPS_VStream_Open(MMPS_VSTREAM_PROPT *feat);
MMP_ERR MMPS_VStream_Start( MMPS_VSTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt);
MMP_ERR MMPS_VStream_Stop(MMPS_VSTREAM_CLASS *obj);
MMP_ERR MMPS_VStream_Close(MMPS_VSTREAM_CLASS *obj);
MMP_ERR MMPS_VStream_Control(   MMPS_VSTREAM_CLASS  *obj,
                                IPC_VIDEO_CTL       ctrl,
                                VIDENC_CTL_SET      *param);
MMP_ERR MMPS_VStream_RegisterDrawer(   MMPS_VSTREAM_CLASS  *obj, MMPF_VIDENC_Callback *cb,void *cb_argc );

/// @}

#endif //_MMPS_VSTREAM_H_

