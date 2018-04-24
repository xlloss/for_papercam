//------------------------------------------------------------------------------
//
//  File        : ipc_cfg.c
//  Description : Source file of IP Camera Configuration
//  Author      : Alterman
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "mmpf_dram.h"
#include "ipc_cfg.h"
#include "dualcpu_v4l2.h"

#ifndef __GNUC__
extern MMP_ULONG    Image$$RESET$$Base;
extern MMP_ULONG    Image$$ALL_DRAM$$ZI$$Limit;
extern MMP_ULONG    Image$$ALL_SRAM$$ZI$$Limit;
#else
extern char*    __RESET_START__;
extern char*    __RESET_END__; 
extern char*    __ALLSRAM_END__;  
#endif

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
//------------------------------------------------------------------------------
//                              System Buffer
//------------------------------------------------------------------------------
/*cpu-b dram base*/
MMP_ULONG       gulDramBase ;
/* Buffer in SRAM */
MMP_ULONG       gulSramBufBase;
MMP_ULONG       gulSramBufEnd;  // Chip depended

/* Buffer in DRAM */
MMP_ULONG       gulDramBufBase;
MMP_ULONG       gulDramBufEnd;  // Chip & project depended

/* Buffer for Linux (CPU-A) */
MMP_ULONG       gulDramBuf4Linux = DBUF_SIZE_4_LINUX;

//------------------------------------------------------------------------------
//                              H.264 Stream Related
//------------------------------------------------------------------------------
#if (MAX_H264_STREAM_NUM > 0)

/* Number of H.264 streams supported at the same time */
MMP_ULONG       gulMaxVStreamNum = MAX_H264_STREAM_NUM;

/* Capability for each H.264 stream */
ST_VIDEO_CAP    gstVStreamCap[MAX_H264_STREAM_NUM] = {
    // The 1st H.264 stream capability
    {
            1920,       ///< max. image width
            1080,       ///< max. image height
               2,       ///< max. 2 input frame buf (must <= MAX_INFRM_SBUF_NUM)
         4000000,       ///< bitrate, 4Mbps in max.
            8000,       ///< buffer can keep 8s video frames in max.
    },

#if (MAX_H264_STREAM_NUM > 1)
    // The 2nd H.264 stream capability
    {
#if (DRAM_SIZE <= 0x4000000)
            1280,       ///< max. image width
             720,       ///< max. image height
#elif (DRAM_SIZE >= 0x8000000)
            1920,       ///< max. image width
            1080,       ///< max. image height
#endif             
               2,       ///< max. 2 input frame buf (must <= MAX_INFRM_SBUF_NUM)
         4000000,       ///< bitrate, 6Mbps in max.
            8000,       ///< buffer can keep 8s video frames in maximum.
    },
#endif
};

#endif // (MAX_H264_STREAM_NUM > 0)

//------------------------------------------------------------------------------
//                          Audio Stream Related
//------------------------------------------------------------------------------
#if (MAX_AUD_STREAM_NUM > 0)

/* Audio input path */
MMP_AUD_INOUT_PATH  gADCInPath = MMP_AUD_AFE_IN_DIFF;
/* Audio output path */
MMP_AUD_INOUT_PATH  gDACOutPath = MMP_AUD_AFE_OUT_LINE;

/* Audio input channel */
MMP_AUD_LINEIN_CH   gADCInCh = MMP_AUD_LINEIN_L;

/* Number of audio streams supported at the same time */
MMP_ULONG       gulMaxAStreamNum = MAX_AUD_STREAM_NUM;

/* Capability for each audio stream */
ST_AUDIO_CAP    gstAStreamCap[MAX_AUD_STREAM_NUM] = {
    // The 1st audio stream capability
    {
      MMP_AUD_AFE_IN_DIFF,  ///< Input from internal ADC
          MMP_ASTREAM_AAC,  ///< AAC format
                    16000,  ///< sample rate 32KHz
                    32000,  ///< bitrate 64Kbps
                     8000,  ///< buffer can keep 8s audio frames in maximum.
    },

#if (MAX_AUD_STREAM_NUM > 1)
    // The 2nd audio stream capability
    {
      MMP_AUD_AFE_IN_DIFF,  ///< Input from internal ADC
          MMP_ASTREAM_AAC,  ///< AAC format
                    16000,  ///< sample rate 32KHz
                    32000,  ///< bitrate 128Kbps
                     8000,  ///< buffer can keep 8s audio frames in maximum.
    },
#endif
};

#endif // (MAX_AUD_STREAM_NUM > 0)

//------------------------------------------------------------------------------
//                          JPEG Stream Related
//------------------------------------------------------------------------------
/* Capability for each JPEG streams */
#if (MAX_JPG_STREAM_NUM > 0)
MMP_ULONG       gulMaxJStreamNum = MAX_JPG_STREAM_NUM;
ST_JPEG_CAP     gstJStreamCap[MAX_JPG_STREAM_NUM] = {
    // The 1st JPEG stream capability
    {
             1920,       ///< max. image width
             1080,       ///< max. image height
             200,       ///< target size 500K
        0x100000,       ///< compress buffer size 1MB
    },
};
#endif

//------------------------------------------------------------------------------
//                          Gray Stream Related
//------------------------------------------------------------------------------
/* Capability for each gray streams */
#if (MAX_GRAY_STREAM_NUM > 0)
MMP_ULONG       gulMaxYStreamNum = MAX_GRAY_STREAM_NUM;
ST_GRAY_CAP     gstYStreamCap[MAX_GRAY_STREAM_NUM] = {
    // The 1st gray stream capability
    {
            640,        ///< max. gray image width
            360,        ///< max. gray image height
        V4L2_PIX_FMT_GREY,
    }
    #if SUPPORT_IVA
    ,
    {
        320,
        180,
        V4L2_PIX_FMT_I420, 
            
    }
    #endif
};
#endif

//------------------------------------------------------------------------------
//                        Motion Detection Related
//------------------------------------------------------------------------------
/* Capability for motion detection */
#if (SUPPORT_MDTC)
ST_MDTC_CAP     gstMdtcCap = {
#if (DRAM_SIZE <= 0x4000000)
            320,        ///< max. luma frame width
            180,        ///< max. luma frame height
#else
/*change to 90p in case AEC is running */
            320,        ///< max. luma frame width
            180,        ///< max. luma frame height
#endif  

#if MD_USE_ROI_TYPE==0
    MDTC_MAX_DIV_X,     ///< max. number of window divisions in horizontal
    MDTC_MAX_DIV_Y,     ///< max. number of window divisions in vertical
#else
    1,1
#endif    
};
#endif


//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#ifndef __GNUC__
void IPC_Cfg_Init(void)
{
    gulSramBufBase = ALIGN256((MMP_ULONG)&Image$$ALL_SRAM$$ZI$$Limit);
    gulSramBufEnd  = 0x0016E000;

    gulDramBase = (MMP_ULONG)&Image$$RESET$$Base;
        
    gulDramBufBase = ALIGN4K((MMP_ULONG)&Image$$ALL_DRAM$$ZI$$Limit);
    gulDramBufEnd  = gulDramBase + DBUF_SIZE_4_VIDEO - gulDramBuf4Linux;
}
#else
void IPC_Cfg_Init(void)
{

    gulSramBufBase = ALIGN256((int)&__ALLSRAM_END__);
    gulSramBufEnd  = 0x0016E000;

    gulDramBase = (int)&__RESET_START__; 
        
    gulDramBufBase = ALIGN4K((int)&__RESET_END__);
    gulDramBufEnd  = gulDramBase + DBUF_SIZE_4_VIDEO - gulDramBuf4Linux;

}
#endif
