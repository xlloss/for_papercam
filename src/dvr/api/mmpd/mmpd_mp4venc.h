/// @ait_only
/**
 @file mmpd_mp4venc.h
 @brief INCLUDE File of Host VIDEO ENCODE Driver.
 @author Will Tseng
 @version 1.0
*/


#ifndef _MMPD_MP4VENC_H_
#define _MMPD_MP4VENC_H_

#include "mmp_lib.h"
#include "ait_config.h"
#include "mmpd_rawproc.h"
#include "mmpf_vidcmn.h"
#include "mmpf_mp4venc.h"

/** @addtogroup MMPD_3GP
 *  @{
 */

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef enum _MMPD_VIDENC_MODULE_ID {
	MMPD_VIDENC_MODULE_H264 = 0,
    MMPD_VIDENC_MODULE_MAX
} MMPD_VIDENC_MODULE_ID;

/// Firmware status. For video encode, the sequence is the same as firmware.
typedef enum _MMPD_MP4VENC_FW_OP {
    MMPD_MP4VENC_FW_OP_NONE = 0x00,
    MMPD_MP4VENC_FW_OP_START,
    MMPD_MP4VENC_FW_OP_PAUSE,
    MMPD_MP4VENC_FW_OP_RESUME,
    MMPD_MP4VENC_FW_OP_STOP,
    MMPD_MP4VENC_FW_OP_PREENCODE
} MMPD_MP4VENC_FW_OP;

/// Video format
typedef enum _MMPD_MP4VENC_FORMAT {
    MMPD_MP4VENC_FORMAT_OTHERS = 0x00,
    MMPD_MP4VENC_FORMAT_H264
} MMPD_MP4VENC_FORMAT;

/// Video Current Buffer Mode
typedef enum _MMPD_MP4VENC_CURBUF_MODE {
    MMPD_MP4VENC_CURBUF_FRAME = 0x00,
    MMPD_MP4VENC_CURBUF_RT,
    MMPD_MP4VENC_CURBUF_MAX
} MMPD_MP4VENC_CURBUF_MODE;

typedef enum _MMPD_VIDENC_PADDING_TYPE {
    MMPD_H264ENC_PADDING_NONE,
    MMPD_H264ENC_PADDING_ZERO,
    MMPD_H264ENC_PADDING_REPEAT
} MMPD_VIDENC_PADDING_TYPE;

typedef enum _MMPD_VIDENC_MCI_MODE {
    #define MCI_MODE_MASK       	(0x000F)
    #define MCI_PIPE_MASK       	(0x00F0)
    #define MCI_RAWS_MASK       	(0x0F00)
    #define MCI_2NDPIPE_MASK    	(0xF000)

    #define MCI_SET_MODE(m)     	(m)
    #define MCI_SET_PIPE(p)     	((p) << 4)
    #define MCI_SET_RAWS(s)     	((s) << 8)
    #define MCI_SET_2NDPIPE(p)		((p) << 12)

    #define MCI_GET_MODE(_mode) 	(_mode & MCI_MODE_MASK)
    #define MCI_GET_PIPE(_mode) 	((_mode & MCI_PIPE_MASK) >> 4)
    #define MCI_GET_RAWS(_mode) 	((_mode & MCI_RAWS_MASK) >> 8)
    #define MCI_GET_2NDPIPE(_mode) 	((_mode & MCI_2NDPIPE_MASK) >> 12)

    MMPD_VIDENC_MCI_DEFAULT      			= MCI_SET_MODE(1),
    MMPD_VIDENC_MCI_DMAR_H264    			= MCI_SET_MODE(2),
    MMPD_VIDENC_MCI_DMAR_H264_P0 			= MCI_SET_MODE(2)|MCI_SET_PIPE(0),
    MMPD_VIDENC_MCI_DMAR_H264_P1 			= MCI_SET_MODE(2)|MCI_SET_PIPE(1),
    MMPD_VIDENC_MCI_DMAR_H264_P2 			= MCI_SET_MODE(2)|MCI_SET_PIPE(2),
    MMPD_VIDENC_MCI_DMAR_H264_P3 			= MCI_SET_MODE(2)|MCI_SET_PIPE(3),
    MMPD_VIDENC_MCI_RAW      				= MCI_SET_MODE(3),
    MMPD_VIDENC_MCI_RAW0_P0   				= MCI_SET_MODE(3)|MCI_SET_PIPE(0),
    MMPD_VIDENC_MCI_RAW0_P1   				= MCI_SET_MODE(3)|MCI_SET_PIPE(1),
    MMPD_VIDENC_MCI_RAW0_P2   				= MCI_SET_MODE(3)|MCI_SET_PIPE(2),
    MMPD_VIDENC_MCI_RAW0_P3   				= MCI_SET_MODE(3)|MCI_SET_PIPE(3),
    MMPD_VIDENC_MCI_RAW1_P0 				= MCI_SET_MODE(3)|MCI_SET_PIPE(0)|MCI_SET_RAWS(1),
    MMPD_VIDENC_MCI_RAW1_P1 				= MCI_SET_MODE(3)|MCI_SET_PIPE(1)|MCI_SET_RAWS(1),
    MMPD_VIDENC_MCI_GRA_LDC_H264 			= MCI_SET_MODE(4),
    MMPD_VIDENC_MCI_GRA_LDC_P0				= MCI_SET_MODE(4)|MCI_SET_PIPE(0),
    MMPD_VIDENC_MCI_GRA_LDC_P1				= MCI_SET_MODE(4)|MCI_SET_PIPE(1),
    MMPD_VIDENC_MCI_GRA_LDC_P2				= MCI_SET_MODE(4)|MCI_SET_PIPE(2),
    MMPD_VIDENC_MCI_GRA_LDC_P3				= MCI_SET_MODE(4)|MCI_SET_PIPE(3),
    MMPD_VIDENC_MCI_GRA_LDC_P0P2_H264_P1	= MCI_SET_MODE(4)|MCI_SET_PIPE(0)|MCI_SET_2NDPIPE(2),
    MMPD_VIDENC_MCI_GRA_LDC_P3P1_H264_P0	= MCI_SET_MODE(4)|MCI_SET_PIPE(3)|MCI_SET_2NDPIPE(1),
    MMPD_VIDENC_MCI_INVALID      			= MCI_SET_MODE(MCI_MODE_MASK)
} MMPD_VIDENC_MCI_MODE;

#define	VIDEO_INIT_QP_STEP_NUM      3
#define	VIDEO_INPUT_FB_MAX_CNT      4

typedef struct _MMPD_MP4VENC_INPUT_BUF {
	MMP_ULONG ulBufCnt;
    MMP_ULONG ulY[VIDEO_INPUT_FB_MAX_CNT];	///< Video encode input Y buffer start address (can be calculated)
    MMP_ULONG ulU[VIDEO_INPUT_FB_MAX_CNT]; 	///< Video encode input U buffer start address (can be calculated)
    MMP_ULONG ulV[VIDEO_INPUT_FB_MAX_CNT]; 	///< Video encode input V buffer start address (can be calculated)
} MMPD_MP4VENC_INPUT_BUF;

typedef struct _MMPD_MP4VENC_MISC_BUF {
    MMP_ULONG ulYDCBuf;			            ///< Video encode Y DC start buffer (can be calculated)
    MMP_ULONG ulYACBuf;						///< Video encode Y AC start buffer (can be calculated)
    MMP_ULONG ulUVDCBuf; 			        ///< Video encode UV DC start buffer (can be calculated)
    MMP_ULONG ulUVACBuf;					///< Video encode UV AC start buffer (can be calculated)
    MMP_ULONG ulMVBuf;						///< Video encode MV start buffer (can be calculated)
    MMP_ULONG ulSliceLenBuf;                ///< H264 slice length buf
} MMPD_MP4VENC_MISC_BUF;

typedef struct _MMPD_H264ENC_HEADER_BUF {
    MMP_ULONG ulSPSStart;
    MMP_ULONG ulSPSSize;
    MMP_ULONG ulTmpSPSStart;
    MMP_ULONG ulTmpSPSSize;
    MMP_ULONG ulPPSStart;
    MMP_ULONG ulPPSSize;
} MMPD_H264ENC_HEADER_BUF;

typedef struct _MMPD_MP4VENC_BITSTREAM_BUF {
    MMP_ULONG ulStart;      		  		///< Video encode compressed buffer start address (can be calculated)
    MMP_ULONG ulEnd;		          		///< Video encode compressed buffer end address (can be calculated)
} MMPD_MP4VENC_BITSTREAM_BUF;

typedef struct _MMPD_MP4VENC_LINE_BUF {
    MMP_ULONG ulY[12];          ///< ME Y line start address, 12 lines for h264 high profile
    MMP_ULONG ulU[12];		    ///< ME U line start address, 12 lines for h264 high profile UV interleave mode
    MMP_ULONG ulV[12];		    ///< ME Y line start address
    MMP_ULONG ulUP[4];          ///< ME Y up sample, for h264
} MMPD_MP4VENC_LINE_BUF;

typedef struct _MMPD_MP4VENC_REFGEN_BUF {
    MMP_ULONG ulGenY;			///< Video encode Y generate buffer
    MMP_ULONG ulGenU;			///< Video encode U generate buffer
    MMP_ULONG ulGenV;			///< Video encode V generate buffer
    MMP_ULONG ulRefY;     		///< Video encode Y reference buffer
    MMP_ULONG ulRefU;    		///< Video encode U reference buffer
    MMP_ULONG ulRefV;	   		///< Video encode V reference buffer
} MMPD_MP4VENC_REFGEN_BUF;

typedef struct _MMPD_MP4VENC_REFGEN_BD {
    MMP_ULONG ulYStart;			///< Video encode Y reference buffer start address (can be calculated)
    MMP_ULONG ulYEnd;			///< Video encode Y reference buffer end address (can be calculated)
    MMP_ULONG ulUStart;			///< Video encode U reference buffer start address (can be calculated)
    MMP_ULONG ulUEnd;     		///< Video encode U reference buffer end address (can be calculated)
    MMP_ULONG ulVStart;    		///< Video encode V reference buffer start address (can be calculated)
    MMP_ULONG ulVEnd;      		///< Video encode V reference buffer end address (can be calculated)
    MMP_ULONG ulGenStart;       ///< deblock out YUV start addr, H264
    MMP_ULONG ulGenEnd;         ///< deblock out YUV end addr, H264
    MMP_ULONG ulGenYStart;
    MMP_ULONG ulGenYEnd;
    MMP_ULONG ulGenUVStart;
    MMP_ULONG ulGenUVEnd;
} MMPD_MP4VENC_REFGEN_BD;

typedef struct _MMPD_MP4VENC_VIDEOBUF {
	MMPD_MP4VENC_MISC_BUF	miscbuf;
	MMPD_MP4VENC_BITSTREAM_BUF	bsbuf;
	MMPD_MP4VENC_LINE_BUF	linebuf;
	MMPD_MP4VENC_REFGEN_BD	refgenbd;
} MMPD_MP4VENC_VIDEOBUF;

// DUALENC_SUPPORT
typedef enum{
	MMPD_SET_REFLOW = 0,
	MMPD_SET_REFHIGH,
	MMPD_SET_RECBUF
}MMPD_VIDENC_MULTI_SETBUFTYPE;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
MMP_ERR MMPD_VIDENC_InitInstance(MMP_ULONG *InstId);
MMP_ERR MMPD_VIDENC_DeInitInstance(MMP_ULONG InstId);
MMP_ERR MMPD_VIDENC_SetResolution(  MMP_ULONG   ulEncId,
                                    MMP_USHORT  usWidth,
                                    MMP_USHORT  usHeight);
MMP_ERR MMPD_VIDENC_SetCropping(MMP_ULONG   ulEncId,
                                MMP_USHORT  usTop,
                                MMP_USHORT  usBottom,
                                MMP_USHORT  usLeft,
                                MMP_USHORT  usRight);
MMP_ERR MMPD_VIDENC_SetPadding( MMP_ULONG   ulEncId,
                                MMP_USHORT  usType,
                                MMP_USHORT  usCnt);
MMP_ERR MMPD_VIDENC_SetCurBufMode(MMP_ULONG                 ulEncId,
                                MMPD_MP4VENC_CURBUF_MODE    VideoCurBufMode);
MMP_ERR MMPD_VIDENC_SetProfile(MMP_ULONG ulEncId, VIDENC_PROFILE profile);
MMP_ERR MMPD_VIDENC_SetLevel(MMP_ULONG ulEncId, MMP_ULONG level);
MMP_ERR MMPD_VIDENC_SetEntropy(MMP_ULONG ulEncId, VIDENC_ENTROPY entropy);

MMP_ERR MMPD_VIDENC_SetGOP( MMP_ULONG   ulEncId,
                            MMP_USHORT  usPFrame,
                            MMP_USHORT  usBFrame);
MMP_ERR MMPD_VIDENC_SetBitrate(MMP_ULONG ulEncId, MMP_ULONG ulBitrate);
MMP_ERR MMPD_VIDENC_SetTargetBitrate(MMP_ULONG ulEncId, MMP_ULONG ulTargetBr,MMP_ULONG ulRampupBr);
MMP_ERR MMPD_VIDENC_SetInitalQP(MMP_ULONG   ulEncId,
                                MMP_UBYTE   *ubInitQP);
MMP_ERR MMPD_VIDENC_SetQPBoundary(  MMP_ULONG   ulEncId,
                                    MMP_ULONG   lowBound,
                                    MMP_ULONG   highBound);
MMP_ERR MMPD_VIDENC_SetRcMode(MMP_ULONG ulEncId, VIDENC_RC_MODE mode);
MMP_ERR MMPD_VIDENC_SetRcSkip(MMP_ULONG ulEncId, MMP_BOOL skip);
MMP_ERR MMPD_VIDENC_SetRcSkipType(MMP_ULONG ulEncId, VIDENC_RC_SKIPTYPE type);
MMP_ERR MMPD_VIDENC_SetRcLbSize(MMP_ULONG ulEncId, MMP_ULONG lbs);
MMP_ERR MMPD_VIDENC_SetTNR(MMP_ULONG ulEncId, MMP_ULONG tnr);

MMP_ERR MMPD_VIDENC_SetEncFrameRate(MMP_ULONG   ulEncId,
                                    MMP_ULONG   ulIncr,
                                    MMP_ULONG   ulResol);
MMP_ERR MMPD_VIDENC_UpdateEncFrameRate( MMP_ULONG   ulEncId,
                                        MMP_ULONG   ulIncr,
                                        MMP_ULONG   ulResol);
MMP_ERR MMPD_VIDENC_SetSnrFrameRate(MMP_ULONG   ulEncId,
                                    MMP_ULONG   ulIncr,
                                    MMP_ULONG   ulResol);
MMP_ERR MMPD_VIDENC_ForceI(MMP_ULONG ulEncId, MMP_ULONG ulCnt);
MMP_ERR MMPD_VIDENC_EnableClock(MMP_BOOL bEnable);

MMP_ERR MMPD_VIDENC_SetRefGenBound( MMP_ULONG               ubEncId,
                                    MMPD_MP4VENC_REFGEN_BD  *refgenbd);
MMP_ERR MMPD_VIDENC_SetBitstreamBuf(MMP_ULONG                   ubEncId,
                                    MMPD_MP4VENC_BITSTREAM_BUF  *bsbuf);
MMP_ERR MMPD_VIDENC_SetMiscBuf( MMP_ULONG               ubEncId,
                                MMPD_MP4VENC_MISC_BUF   *miscbuf);
MMP_ERR MMPD_VIDENC_CalculateRefBuf(MMP_USHORT              usWidth,
                                    MMP_USHORT              usHeight, 
                                    MMPD_MP4VENC_REFGEN_BD  *refgenbd,
                                    MMP_ULONG               *ulCurAddr);

MMP_ERR MMPD_VIDENC_GetStatus(MMP_ULONG ubEncId, MMPD_MP4VENC_FW_OP *status);
MMP_ERR MMPD_VIDENC_CheckCapability(MMP_ULONG w, MMP_ULONG h, MMP_ULONG fps);

MMP_ERR MMPD_VIDENC_TuneMCIPriority(MMPD_VIDENC_MCI_MODE mciMode);
void    MMPD_VIDENC_TuneEncodePipeMCIPriority(MMP_UBYTE ubPipe);

/// @}

#endif // _MMPD_MP4VENC_H_

/// @end_ait_only
