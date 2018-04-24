//------------------------------------------------------------------------------
//
//  File        : ipc_cfg.h
//  Description : Header file of IP Camera Configuration
//  Author      : Alterman
//  Revision    : 1.0
//
//------------------------------------------------------------------------------
#ifndef _IPC_CFG_H_
#define _IPC_CFG_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "config_fw.h"
#include "mmpf_vidcmn.h"
#include "mmp_aud_inc.h"
#include "mmp_ipc_inc.h"
#if defined(ALL_FW)
#include "mmpf_mdtc.h"
#endif
//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
/*
 * System Buffer
 */
#define DRAM_START_ADDR         (0x01000000) // Chip depended

#if (DRAM_SIZE <= 0x4000000)
#define DBUF_SIZE_4_VIDEO       (0x2A00000) // DRAM buffer size for video driver
#elif (DRAM_SIZE >= 0x8000000)
#define DBUF_SIZE_4_VIDEO       (0x4000000) // DRAM buffer size for video driver
#else 
     #error Unknown DRAM size
#endif
#if V4L2_JPG==0
#define DBUF_SIZE_4_LINUX       (0x400000)  // DRAM buffer size for Linux: 4MB
#else
#define DBUF_SIZE_4_LINUX       (0x500000)  // DRAM buffer size for Linux: 4MB
#endif
/*
 *  H.264 Stream
 */
#define MAX_H264_STREAM_NUM     (MAX_VIDEO_STREAM_NUM) // max. H.264 streams

/*
 *  Audio Stream
 */
#define MAX_AUD_STREAM_NUM      (2)             // max. 2 AAC

/*
 *  JPEG Stream
 */
#define MAX_JPG_STREAM_NUM      (1)             // max. 1 JPEG

/*
 *  Gray Stream
 */
#if SUPPORT_IVA==0
#define MAX_GRAY_STREAM_NUM     (1)             // max. 1 gray stream
#else
#define MAX_GRAY_STREAM_NUM     (2)             // max. 2 gray stream
#endif
//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

// Video Stream Capability Configuration
typedef struct {
    MMP_ULONG   img_w;      ///< Maximum image width
    MMP_ULONG   img_h;      ///< Maximum image height
    MMP_ULONG   infrmslots; ///< Maximum slot buffer number for input frames
    MMP_ULONG   bitrate;    ///< Maximum bitrate of video stream
    MMP_ULONG   buftime;    ///< Buffer depth to keep frames within 'buftime' ms
} ST_VIDEO_CAP;

// Audio Stream Capability Configuration
typedef struct {
    MMP_AUD_INOUT_PATH  src;        ///< Source input path
    MMP_ASTREAM_FMT     fmt;        ///< Audio format
    MMP_ULONG           fs;         ///< Sample rate
    MMP_ULONG           bitrate;    ///< Bitrate
    MMP_ULONG           buftime;    ///< Buf size to keep audio for 'buftime' ms
} ST_AUDIO_CAP;

// JPEG Stream Capability Configuration
typedef struct {
    MMP_ULONG   img_w;      ///< Maximum image width
    MMP_ULONG   img_h;      ///< Maximum image height
    MMP_ULONG   targetsize; ///< Target size of JPEG
    MMP_ULONG   bufsize;    ///< Compress buffer size
} ST_JPEG_CAP;

// Gray Stream Capability Configuration
typedef struct {
    MMP_ULONG   img_w;      ///< Maximum gray frame width
    MMP_ULONG   img_h;      ///< Maximum gray frame height
    MMP_ULONG   img_fmt;
} ST_GRAY_CAP;

#if (SUPPORT_MDTC)
// Motion Detection Capability Configuration
typedef struct {
    MMP_ULONG   luma_w;     ///< Maximum luma frame width
    MMP_ULONG   luma_h;     ///< Maximum luma frame height
    MMP_ULONG   div_x;      ///< Maximum window divisions in horizontal
    MMP_ULONG   div_y;      ///< Maximum window divisions in vertical
} ST_MDTC_CAP;
#endif

//==============================================================================
//
//                              PUBLIC VARIABLES
//
//==============================================================================
/* cpu-b start address in DRAM*/
extern MMP_ULONG        gulDramBase ;        
/* Buffer Part */
// Buffer in SRAM
extern MMP_ULONG        gulSramBufBase;
extern MMP_ULONG        gulSramBufEnd;
// Buffer in DRAM
extern MMP_ULONG        gulDramBufBase;
extern MMP_ULONG        gulDramBufEnd;

/* H.264 Capability of all streams */
#if (MAX_H264_STREAM_NUM > 0)
extern MMP_ULONG        gulMaxVStreamNum;
extern ST_VIDEO_CAP     gstVStreamCap[MAX_H264_STREAM_NUM];
#endif

/* Audio Capability of all streams */
#if (MAX_AUD_STREAM_NUM > 0)
extern MMP_AUD_INOUT_PATH   gADCInPath;
extern MMP_AUD_INOUT_PATH   gDACOutPath;
extern MMP_AUD_LINEIN_CH    gADCInCh;

extern MMP_ULONG        gulMaxAStreamNum;
extern ST_AUDIO_CAP     gstAStreamCap[MAX_AUD_STREAM_NUM];
#endif

/* JPEG Capability of all streams */
#if (MAX_JPG_STREAM_NUM > 0)
extern MMP_ULONG        gulMaxJStreamNum;
extern ST_JPEG_CAP      gstJStreamCap[MAX_JPG_STREAM_NUM];
#endif

/* Gray Capability of all streams */
#if (MAX_GRAY_STREAM_NUM > 0)
extern MMP_ULONG        gulMaxYStreamNum;
extern ST_GRAY_CAP      gstYStreamCap[MAX_GRAY_STREAM_NUM];
#endif

/* Motion Detection Capability */
#if (SUPPORT_MDTC)
extern ST_MDTC_CAP      gstMdtcCap;
#endif


//==============================================================================
//
//                              PUBLIC FUNCTIONS
//
//==============================================================================

void IPC_Cfg_Init(void);

#endif // _IPC_CFG_H_

