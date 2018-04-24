/**
 @file mmpf_vidcmn.h
 @brief Header function of video driver related define
 @author
 @version 0.0
*/

#ifndef _MMPF_VIDCMN_H_
#define _MMPF_VIDCMN_H_

#include "config_fw.h"

//==============================================================================
//
//                              COMPILER OPTION
//
//==============================================================================

#define H264ENC_I2MANY_EN       (0)
#define H264ENC_HBR_EN          (0)
#define H264ENC_TNR_EN          (1)
#define H264ENC_RDO_EN          (1)
#define H264ENC_ICOMP_EN        (0) // Support in MCR_V2 MP only
#define H264ENC_LBR_EN          (1)
    #define H264ENC_LBR_FLOAT_RATIO (1)

#define H264ENC_GDR_EN          (0) // Intra-Refresh mode

#define VID_CTL_DBG_FPS         (0)
#define VID_CTL_DBG_GOP         (0)
#define VID_CTL_DBG_BR          (1)

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
/*
 * Rate control related
 */
#define MAX_NUM_TMP_LAYERS                  (2)
#define MAX_NUM_TMP_LAYERS_LBR              (2) //H264ENC_LBR_EN
#define TEMPORAL_ID_MASK                    (0x03)

#define RC_MIN_VBV_FRM_NUM                  ((RC_MAX_WEIGHT_I+2000)/1000)
#define RC_PSEUDO_GOP_SIZE                  (1000)

/*
 * Low bitrate ratio for I layers for static or motion scene
 */
#if (H264ENC_LBR_EN)&&(H264ENC_LBR_FLOAT_RATIO)
// for static scene
#define LBR_STATIC_I_RATIO_30FPS            (80)
#define LBR_STATIC_I_RATIO_60FPS            (70)
#define LBR_STATIC_I_RATIO_90FPS            (65)
#define LBR_STATIC_I_RATIO_120FPS           (60)

// for motion scene
#define LBR_MOTION_I_RATIO_30FPS            (20)
#define LBR_MOTION_I_RATIO_60FPS            (15)
#define LBR_MOTION_I_RATIO_90FPS            (10)
#define LBR_MOTION_I_RATIO_120FPS           (5)
#endif

#if (H264ENC_LBR_EN)
#define RC_MAX_WEIGHT_I                     (2000)
#define RC_INIT_WEIGHT_I                    (1000)
#else
#define RC_MAX_WEIGHT_I                     (3500)
#define RC_INIT_WEIGHT_I                    (2500)
#endif

/*
 * Encode related
 */
#define MMPF_VIDENC_MAX_QUEUE_SIZE          (4)

#define MMPF_VIDENC_MODULE_H264             (0)
#define MMPF_VIDENC_MODULE_MAX              (1)

// video format
#define MMPF_MP4VENC_FORMAT_OTHERS          0x00
#define MMPF_MP4VENC_FORMAT_H264            0x01
#define MMPF_MP4VENC_FORMAT_MJPEG           0x02

// video operation status
#define	MMPF_MP4VENC_FW_STATUS_RECORD       0x0000  ///< status of video encoder
#define	MMPF_MP4VENC_FW_STATUS_START        0x0001  ///< status of START
#define	MMPF_MP4VENC_FW_STATUS_PAUSE        0x0002  ///< status of PAUSE
#define	MMPF_MP4VENC_FW_STATUS_RESUME       0x0003  ///< status of RESUME
#define	MMPF_MP4VENC_FW_STATUS_STOP         0x0004  ///< status of STOP
#define	MMPF_MP4VENC_FW_STATUS_PREENCODE    0x0005  ///< status of PRE_ENCODE

#if CUSTOMER_H264_STREAMS > 0
#define MAX_VIDEO_STREAM_NUM				(CUSTOMER_H264_STREAMS) 
#else
#define MAX_VIDEO_STREAM_NUM				(2)
#endif

#define MAX_NUM_PARAM_CTL                   (16)

#define MAX_OSD_STR_LEN                     (32)

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================
typedef void VidEncEndCallBackFunc(void *);

typedef enum _MMPF_VIDENC_ATTRIBUTE {
    MMPF_VIDENC_ATTRIBUTE_PROFILE = 0,
    MMPF_VIDENC_ATTRIBUTE_LEVEL,
    MMPF_VIDENC_ATTRIBUTE_ENTROPY_MODE,
    MMPF_VIDENC_ATTRIBUTE_LAYERS,
    MMPF_VIDENC_ATTRIBUTE_PADDING_INFO,
    MMPF_VIDENC_ATTRIBUTE_RC_MODE,
    MMPF_VIDENC_ATTRIBUTE_RC_SKIPPABLE,
    MMPF_VIDENC_ATTRIBUTE_FRM_QP,
    MMPF_VIDENC_ATTRIBUTE_FRM_QP_BOUND,
    MMPF_VIDENC_ATTRIBUTE_BR,
    MMPF_VIDENC_ATTRIBUTE_LB_SIZE,
    MMPF_VIDENC_ATTRIBUTE_PIC_MAX_WEIGHT,
    MMPF_VIDENC_ATTRIBUTE_CROPPING,
    MMPF_VIDENC_ATTRIBUTE_GOP_CTL,
    MMPF_VIDENC_ATTRIBUTE_FORCE_I,
    MMPF_VIDENC_ATTRIBUTE_VIDEO_FULL_RANGE,
    MMPF_VIDENC_ATTRIBUTE_MAX_FPS,
    MMPF_VIDENC_ATTRIBUTE_SLICE_CTL,
    MMPF_VIDENC_ATTRIBUTE_CURBUF_MODE,
    MMPF_VIDENC_ATTRIBUTE_CURBUF_ADDR,
    MMPF_VIDENC_ATTRIBUTE_SWITCH_CURBUF_MODE,
    MMPF_VIDENC_ATTRIBUTE_RTFCTL_MODE,
    MMPF_VIDENC_ATTRIBUTE_RINGBUF_EN,
    MMPF_VIDENC_ATTRIBUTE_POC_TYPE,
    MMPF_VIDENC_ATTRIBUTE_REG_CALLBACK_ENC_START,
    MMPF_VIDENC_ATTRIBUTE_REG_CALLBACK_ENC_RESTART,
    MMPF_VIDENC_ATTRIBUTE_REG_CALLBACK_ENC_END,
    MMPF_VIDENC_ATTRIBUTE_RESOLUTION,
    MMPF_VIDENC_ATTRIBUTE_PARSET_EVERY_FRM,
    MMPF_VIDENC_ATTRIBUTE_PRIORITY_ID,
	MMPF_VIDENC_ATTRIBUTE_SEI_CTL,
    MMPF_VIDENC_ATTRIBUTE_PNALU_MODE,
    MMPF_VIDENC_ATTRIBUTE_ME_ITR_MAX_STEPS,
    
    MMPF_VIDENC_ATTRIBUTE_TNR_EN,
    MMPF_VIDENC_ATTRIBUTE_TNR_LOW_MV_THR,
    MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_THR,
    MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_LUMA_PXL_DIFF_THR,
    MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_CHROMA_PXL_DIFF_THR,
    MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_4x4_CNT_THR,
    MMPF_VIDENC_ATTRIBUTE_TNR_LOW_MV_FILTER,
    MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_FILTER,
    MMPF_VIDENC_ATTRIBUTE_TNR_HIGH_MV_FILTER,

    MMPF_VIDENC_ATTRIBUTE_PTZ_EN,
    // RDO control
    MMPF_VIDENC_ATTRIBUTE_RDO_EN,
    MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P1,
    MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P2,
    // RC skip type
    MMPF_VIDENC_ATTRIBUTE_RC_SKIPTYPE,
    MMPF_VIDENC_ATTRIBUTE_MEMD_PARAM,

    MMPF_VIDENC_ATTRIBUTE_ENC_FPS,
    MMPF_VIDENC_ATTRIBUTE_SNR_FPS
} MMPF_VIDENC_ATTRIBUTE;

typedef enum _MMPF_VIDENC_SYNCFRAME_TYPE {
    MMPF_VIDENC_SYNCFRAME_IDR = 0,
    MMPF_VIDENC_SYNCFRAME_I,
    MMPF_VIDENC_SYNCFRAME_GDR,
    MMPF_VIDENC_SYNCFRAME_LT_IDR,
    MMPF_VIDENC_SYNCFRAME_LT_I,
    MMPF_VIDENC_SYNCFRAME_LT_P,
    MMPF_VIDENC_SYNCFRAME_MAX
} MMPF_VIDENC_SYNCFRAME_TYPE;

// video current buffer mode
typedef enum _MMPF_VIDENC_CURBUF_MODE {
	MMPF_MP4VENC_CURBUF_FRAME,
	MMPF_MP4VENC_CURBUF_RT,
	MMPF_MP4VENC_CURBUF_MAX
} MMPF_VIDENC_CURBUF_MODE;

// H.264 entropy coding
typedef enum _VIDENC_ENTROPY {
    H264ENC_ENTROPY_CAVLC = 0,
    H264ENC_ENTROPY_CABAC,
    H264ENC_ENTROPY_NONE
} VIDENC_ENTROPY;

/// H.264 video profile
typedef enum _VIDENC_PROFILE {
    H264ENC_PROFILE_NONE = 0,
    H264ENC_BASELINE_PROFILE,   ///< H.264 baseline profile
    H264ENC_MAIN_PROFILE,       ///< H.264 main profile
    H264ENC_HIGH_PROFILE,       ///< H.264 high profile
    H264ENC_PROFILE_MAX
} VIDENC_PROFILE;

// video rate control mode
typedef enum _VIDENC_RC_MODE {
    VIDENC_RC_MODE_CBR = 0,
    VIDENC_RC_MODE_VBR,
    VIDENC_RC_MODE_CQP,
    VIDENC_RC_MODE_LOWBR,
    VIDENC_RC_MODE_MAX
} VIDENC_RC_MODE;

// RC Skip type
typedef enum _VIDENC_RC_SKIPTYPE {
    VIDENC_RC_SKIP_DIRECT = 0,
    VIDENC_RC_SKIP_SMOOTH
} VIDENC_RC_SKIPTYPE;

// TNR features
typedef enum _VIDENC_TNR_FEAT {
    TNR_ZERO_MV_EN  = 1 << 0, 
    TNR_LOW_MV_EN   = 1 << 1,
    TNR_HIGH_MV_EN  = 1 << 2
} VIDENC_TNR_FEAT;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef struct _MMPF_VIDENC_RESOLUTION {
    MMP_USHORT  usWidth;
    MMP_USHORT  usHeight;
} MMPF_VIDENC_RESOLUTION;

typedef struct _MMPF_VIDENC_RC_MODE_CTL {
    VIDENC_RC_MODE  RcMode;
    MMP_BOOL        bLayerGlobalRc;
} MMPF_VIDENC_RC_MODE_CTL;

typedef struct _MMPF_VIDENC_GOP_CTL {
    MMP_USHORT  usGopSize;
    MMP_USHORT  usMaxContBFrameNum;
    MMPF_VIDENC_SYNCFRAME_TYPE SyncFrameType;
    MMP_BOOL    bReset;
} MMPF_VIDENC_GOP_CTL;

typedef struct _MMPF_VIDENC_CROPPING {
    MMP_USHORT  usTop;
    MMP_USHORT  usBottom;
    MMP_USHORT  usLeft;
    MMP_USHORT  usRight;
} MMPF_VIDENC_CROPPING;

typedef struct _MMPF_VIDENC_BITRATE_CTL {   ///< bitrate param control
	MMP_UBYTE ubLayerBitMap;                ///< 0'b111 means all temporal layers
    MMP_ULONG ulBitrate[MAX_NUM_TMP_LAYERS];///< bitrate, bits
} MMPF_VIDENC_BITRATE_CTL;

typedef struct _MMPF_VIDENC_LEAKYBUCKET_CTL { ///< leacky bucket param control
    MMP_UBYTE ubLayerBitMap;                ///< 0'b111 means all temporal layers
    MMP_ULONG ulLeakyBucket[MAX_NUM_TMP_LAYERS];///< in ms
} MMPF_VIDENC_LEAKYBUCKET_CTL;

typedef struct _MMPF_VIDENC_QP_CTL {        ///< QP control, for initail QP and CQP
    MMP_UBYTE ubTID;                        ///< 0'b111 means all temporal layers
    MMP_UBYTE ubTypeBitMap;                 ///< 0: I, 1: P, 2: B
    MMP_UBYTE ubQP[3];
    MMP_LONG  CbrQpIdxOffset[3];            ///< Chroma QP index offset
    MMP_LONG  CrQpIdxOffset[3];             ///< 2nd chroma QP index offset
} MMPF_VIDENC_QP_CTL;

typedef struct _MMPF_VIDENC_QP_BOUND_CTL {  ///< QP Boundary
    MMP_UBYTE ubLayerID;                    ///< 0'b111 means all temporal layers
    MMP_UBYTE ubTypeBitMap;                 ///< 0: I, 1: P, 2: B
    MMP_UBYTE ubQPBound[3][2];
} MMPF_VIDENC_QP_BOUND_CTL;

typedef struct _MMPF_VIDENC_QUEUE {
    MMP_ULONG   buffers[MMPF_VIDENC_MAX_QUEUE_SIZE];  ///< queue for buffer ready to encode, in display order
    MMP_ULONG   weight[MMPF_VIDENC_MAX_QUEUE_SIZE];   ///< the times to encode the same frame
    MMP_ULONG   head;
    MMP_ULONG   size;
    MMP_ULONG   weighted_size;
} MMPF_VIDENC_QUEUE;

#define DUALENC_QMAXSIZE 	(20)

typedef struct _MMPF_DUALENC_QUEUE {
    MMP_ULONG   buffers[DUALENC_QMAXSIZE];  ///< queue for buffer ready to encode, in display order
    MMP_ULONG   weight[DUALENC_QMAXSIZE];   ///< the times to encode the same frame
    MMP_ULONG   head;
    MMP_ULONG   size;
    MMP_ULONG   weighted_size;
} MMPF_DUALENC_QUEUE;

typedef struct _MMPF_VIDENC_FRAME{
    MMP_ULONG   ulYAddr;
    MMP_ULONG   ulUAddr;
    MMP_ULONG   ulVAddr;
    MMP_ULONG   ulTimestamp;
} MMPF_VIDENC_FRAME;

typedef struct _MMPF_VIDENC_FRAMEBUF_BD {
    MMPF_VIDENC_FRAME   LowBound;
    MMPF_VIDENC_FRAME   HighBound;
} MMPF_VIDENC_FRAMEBUF_BD;

typedef struct _MMPF_VIDENC_DUMMY_DROP_INFO {
	MMP_ULONG	ulDummyFrmCnt;           ///< specified how many video frames to be duplicated
	MMP_ULONG   ulDropFrmCnt;            ///< specified how many video frames to be dropped
	MMP_USHORT  usAccumSkipFrames;              ///< number of skip frames within 1 sec
	MMP_ULONG   ulBeginSkipTimeInMS;            ///< the absolute timer counter of the beginning skip frame within 1 sec.
} MMPF_VIDENC_DUMMY_DROP_INFO;

// Frame rate representation
typedef struct MMP_VID_FPS {
    MMP_ULONG   ulResol;
    MMP_ULONG   ulIncr;
    MMP_ULONG   ulIncrx1000;
} MMP_VID_FPS;

typedef union {
    MMPF_VIDENC_BITRATE_CTL     Bitrate;
    MMPF_VIDENC_RC_MODE_CTL     RcMode;
    MMPF_VIDENC_LEAKYBUCKET_CTL CpbSize;
    MMPF_VIDENC_QP_CTL          Qp;
    MMPF_VIDENC_GOP_CTL         Gop;
    MMP_VID_FPS                 Fps;
    MMP_ULONG                   ConusI; // Contiguous I-frame count
} VIDENC_CTL;

typedef struct _MMPF_VIDENC_PARAM_CTL {
    MMPF_VIDENC_ATTRIBUTE   Attrib;
    void                    (*CallBack)(MMP_ERR);
    VIDENC_CTL              Ctl;
} MMPF_VIDENC_PARAM_CTL;

enum ait_osd_axis {
    AXIS_X = 0,
    AXIS_Y = 1,
    AXIS_MAX
};

typedef struct _ait_rtc_config {
    unsigned long  usYear;
    unsigned long  usMonth;
    unsigned long  usDay;
    unsigned long  usHour;
    unsigned long  usMinute;
    unsigned long  usSecond;
    char            bOn;
} ait_rtc_config;

typedef struct _ait_osd_config {
    unsigned long           index;
    unsigned long           type;
    unsigned long          pos[AXIS_MAX];
    unsigned long          TextColorY;
    unsigned long          TextColorU;
    unsigned long          TextColorV;

    char                    str[MAX_OSD_STR_LEN];
}ait_osd_config;

typedef struct _ait_osd_position {
    unsigned long           osd_start_x;
    unsigned long           osd_start_y;
}ait_osd_position;

// Video controlling
typedef struct {
    MMP_ULONG       bitrate;
    MMP_ULONG       target_bitrate,rampup_bitrate ; // for bitrate ramp0up
    MMP_ULONG       gop;
    MMP_ULONG       lb_size;        // leaky bucket size
    MMP_VID_FPS     enc_fps;
    VIDENC_PROFILE  profile;
    MMP_ULONG       level;
    VIDENC_PROFILE  entropy;
    VIDENC_RC_MODE  rc_mode;
    VIDENC_RC_MODE  rc_skiptype;
    MMP_BOOL        rc_skip;
    MMP_BOOL        force_idr;      // force I
    MMP_UBYTE       qp_init[2];     // init QP for I/P
    MMP_UBYTE       qp_bound[2];    // QP high/low boundary
    MMP_UBYTE       tnr;            // TNR features
    MMP_USHORT      start_i_frames;
	ait_rtc_config  rtc_config;
	ait_osd_config  osd_config_rtc;
	ait_osd_config  osd_config_str;
    ait_osd_position osd_postion_rtc;
    ait_osd_position osd_postion_str;

} VIDENC_CTL_SET;

#endif //_MMPF_VIDCMN_H_
