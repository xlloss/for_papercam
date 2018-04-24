//==============================================================================
//
//  File        : mmpf_mp4venc.h
//  Description : Header function of video codec
//  Author      : Will Tseng
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_MP4VENC_H_
#define _MMPF_MP4VENC_H_

#include "includes_fw.h"
#include "mmpf_3gpmgr.h"
#include "mmpf_h264enc.h"
#include "mmpf_vidcmn.h"

/** @addtogroup MMPF_VIDEO
@{
*/

//==============================================================================
//
//                              COMPILER OPTION
//
//==============================================================================

#if (SHARE_REF_GEN_BUF)&&(H264ENC_ICOMP_EN)
    #error With share ref/gen buffer, h264 image compression is not supported
#endif

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

// Audio format (only used in recoder)
#define	MMPF_AUD_FORMAT_OTHERS		        0x00 
#define	MMPF_AUD_FORMAT_AMR			        0x01
#define	MMPF_AUD_FORMAT_MP4A		        0x02

#define BASELINE_PROFILE        66
#define MAIN_PROFILE            77

// FREXT Profile IDC definitions
#define FREXT_HP                100      ///< YUV 4:2:0/8 "High"
#define FREXT_Hi10P             110      ///< YUV 4:2:0/10 "High 10"
#define FREXT_Hi422             122      ///< YUV 4:2:2/10 "High 4:2:2"
#define FREXT_Hi444             244      ///< YUV 4:4:4/12 "High 4:4:4"

// H264 qp bound
#define H264E_MAX_MB_QP         (46)
#define H264E_MIN_MB_QP         (10)
#define H264E_MIN_MB_QP_8x8     (9)

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum _MMPF_H264ENC_HWRC_MODE {
    MMPF_H264ENC_HWRC_QPTUNE = 0,
    MMPF_H264ENC_HWRC_RDO,
    MMPF_H264ENC_HWRC_DIS
} MMPF_H264ENC_HWRC_MODE;

typedef enum _MMPF_VIDENC_DUMMY_FRAME_MODE {
    MMPF_VIDENC_DUMMY_FRAME_BY_REENC,
    MMPF_VIDENC_DUMMY_FRAME_BY_CONTAINER
} MMPF_VIDENC_DUMMY_FRAME_MODE;

typedef enum _MMPF_VIDENC_CB_TYPE {
    MMPF_VIDENC_CB_ME_DONE  = 0x00,
    MMPF_VIDENC_CB_MAX
} MMPF_VIDENC_CB_TYPE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef void MMPF_VIDENC_Callback(void *argc);

typedef struct _MMPF_VIDENC_MODULE {
    MMP_BOOL            bInitMod;
    MMP_UBYTE           ubRefCnt;
    MMP_USHORT          Format;
    MMPF_H264ENC_MODULE H264EMod;
} MMPF_VIDENC_MODULE;

typedef struct _MMPF_VIDNEC_INSTANCE {
    MMP_BOOL                bInitInst;
    MMPF_VIDENC_MODULE      *Module;
    MMPF_H264ENC_ENC_INFO   h264e;
    MMPF_VIDENC_Callback    *preEncExec ;
    void                    *preEncArgc ;
} MMPF_VIDENC_INSTANCE;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

#define get_vidinst_format(_p)      ((_p)->Module->Format)
#define get_vidinst_id(_p)          ((MMPF_VIDENC_INSTANCE*)(_p) - MMPF_VIDENC_GetInstance(0))

MMPF_VIDENC_MODULE   *MMPF_VIDENC_GetModule(void);
MMPF_VIDENC_INSTANCE *MMPF_VIDENC_GetInstance(MMP_UBYTE InstId);

MMP_ERR     MMPF_VIDMULENC_QueueProc(MMPF_DUALENC_QUEUE *queue,
                                    MMP_ULONG           qmaxsize,
                                    MMP_ULONG           *data,
                                    MMP_USHORT          type,
                                    void                *pres);

void        MMPF_VIDENC_SetVideoPath(MMP_USHORT usIBCPipe);
MMP_ULONG   MMPF_VIDENC_GetQueueDepth(MMPF_VIDENC_QUEUE *queue,
                                    MMP_BOOL            weighted);
MMP_ERR     MMPF_VIDENC_ShowQueue(  MMPF_VIDENC_QUEUE   *queue,
                                    MMP_ULONG           offset,
                                    MMP_ULONG           *data,
                                    MMP_BOOL            weighted);
void        MMPF_VIDENC_ResetQueue(MMPF_VIDENC_QUEUE *queue);
MMP_ERR     MMPF_VIDENC_PushQueue(  MMPF_VIDENC_QUEUE   *queue,
                                    MMP_ULONG           buffer,
                                    MMP_BOOL            weighted);
MMP_ERR     MMPF_VIDENC_PopQueue(   MMPF_VIDENC_QUEUE   *queue,
                                    MMP_ULONG           offset,
                                    MMP_ULONG           *data,
                                    MMP_BOOL            weighted);
MMP_ERR     MMPF_VIDENC_GetParameter(MMP_UBYTE ubEncId,
                                    MMPF_VIDENC_ATTRIBUTE   attrib,
                                    void                    *arg);
void        MMPF_VIDENC_SetFrameInfoList(MMP_UBYTE  ubEncID,
                                        MMP_ULONG       ulYBuf[],
                                        MMP_ULONG       ulUBuf[],
                                        MMP_ULONG       ulVBuf[],
                                        MMP_UBYTE       ubBufCnt);
MMP_ERR     MMPF_VIDENC_SetEncodeEnable (MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR     MMPF_VIDENC_SetEncodeDisable(MMPF_H264ENC_ENC_INFO *pEnc);
MMPF_3GPMGR_FRAME_TYPE MMPF_VIDENC_GetVidRecdFrameType(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ULONG   MMPF_VIDENC_GetTotalEncodeSize (MMP_ULONG ulEncId);

MMP_ERR     MMPF_VIDENC_TriggerEncode (void);
MMP_ERR     MMPF_VIDENC_SetRecdFrameReady(MMP_USHORT usEncID, MMP_ULONG *plCurBuf, MMP_ULONG *plIBCBuf);

MMP_BOOL    MMPF_VIDENC_IsModuleInit(void);
MMP_ERR     MMPF_VIDENC_InitModule(void);
MMP_ERR     MMPF_VIDENC_DeinitModule(void);
MMP_ERR     MMPF_VIDENC_InitInstance(MMP_ULONG *InstId);
MMP_ERR     MMPF_VIDENC_DeInitInstance(MMP_ULONG InstId);
MMP_ERR     MMPF_VIDENC_RegisterPreEncodeCallBack(MMP_UBYTE InstId,MMPF_VIDENC_Callback *cb,void *cb_argc) ;
MMP_ERR     MMPF_VIDENC_Start(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR     MMPF_VIDENC_Stop(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR     MMPF_VIDENC_Resume(MMP_ULONG ulEncId);
MMP_ERR     MMPF_VIDENC_Pause(MMP_ULONG ulEncId);
MMP_ERR     MMPF_VIDENC_PreEncode (MMPF_H264ENC_ENC_INFO *pEnc);
MMP_BOOL    MMPF_VIDENC_CheckCapability(MMP_ULONG total_mb, MMP_ULONG fps);

// functions of video encoder
MMP_ERR     MMPF_MP4VENC_SetVideoResolution(MMPF_H264ENC_ENC_INFO   *pEnc,
                                            MMP_USHORT              usWidth,
                                            MMP_USHORT              usHeight);
MMP_ERR     MMPF_MP4VENC_SetCropping(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_USHORT              usTop,
                                        MMP_USHORT              usBottom,
                                        MMP_USHORT              usLeft,
                                        MMP_USHORT              usRight);

MMP_ERR     MMPF_MP4VENC_SetVideoProfile(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                            MMP_ULONG               ulProfile);
MMP_ERR     MMPF_MP4VENC_SetVideoLevel( MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_ULONG               ulLevel);
MMP_ERR     MMPF_MP4VENC_SetVideoEntropy(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                            VIDENC_ENTROPY          entropy);
MMP_ERR     MMPF_MP4VENC_SetVideoGOP(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_USHORT  usPFrame,
                                        MMP_USHORT  usBFrame);
MMP_ERR     MMPF_MP4VENC_SetRcMode( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    VIDENC_RC_MODE          mode);
MMP_ERR     MMPF_MP4VENC_SetRcSkip(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG skip);
MMP_ERR     MMPF_MP4VENC_SetRcSkipType( MMPF_H264ENC_ENC_INFO   *pEnc,
                                        VIDENC_RC_SKIPTYPE      type);
MMP_ERR     MMPF_MP4VENC_SetRcLbSize(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_ULONG               lbs);
MMP_ERR     MMPF_MP4VENC_SetTNR(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG tnr);

MMP_ERR     MMPF_MP4VENC_SetInitQP( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_UBYTE   ubIQP,
                                    MMP_UBYTE   ubPQP);
MMP_ERR     MMPF_MP4VENC_SetQPBound(MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_UBYTE   ubLowBound,
                                    MMP_UBYTE   ubHighBound);

void        MMPF_MP4VENC_SetCurBufMode( MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_ULONG              curbufmode);
void        MMPF_MP4VENC_SetStatus(MMP_ULONG ulEncId, MMP_USHORT status);
MMP_USHORT  MMPF_MP4VENC_GetStatus(MMP_ULONG ulEncId);
MMP_ULONG   MMPF_VIDENC_GetSkipThreshold(MMP_ULONG ulEncId);

// functions of H264 encoder
void        MMPF_MP4VENC_SetBitRate(MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_ULONG               bitrate);

void        MMPF_MP4VENC_SetTargetBitRate(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG target_br,MMP_ULONG rampup_br);

void        MMPF_MP4VENC_SetEncFrameRate(MMPF_H264ENC_ENC_INFO  *pEnc,
                                        MMP_ULONG               timeresol,
                                        MMP_ULONG               timeincrement);
void        MMPF_MP4VENC_SetSnrFrameRate(MMPF_H264ENC_ENC_INFO  *pEnc,
                                        MMP_ULONG               timeresol,
                                        MMP_ULONG               timeincrement);
void        MMPF_MP4VENC_ForceI(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulCnt);

void        MMPF_VidRateCtl_GetRcVersion(MMP_USHORT *RcMajorVersion,
                                        MMP_USHORT  *RcMinorVersion);
MMP_LONG    MMPF_VidRateCtl_Get_VOP_QP( void        *RCHandle,
                                        MMP_LONG    vop_type,
                                        MMP_ULONG   *target_size,
                                        MMP_ULONG   *qp_delta,
                                        MMP_BOOL    *bSkipFrame,
                                        MMP_ULONG   ulMaxFrameSize);
MMP_ERR     MMPF_VidRateCtl_ForceQP(    void        *RCHandle,
                                        MMP_LONG    vop_type,
                                        MMP_ULONG   QP);
MMP_ULONG   MMPF_VidRateCtl_UpdateModel(void        *RCHandle,
                                        MMP_LONG    vop_type,
                                        MMP_ULONG   CurSize,
                                        MMP_ULONG   HeaderSize,
                                        MMP_ULONG   last_QP,
                                        MMP_BOOL    bForceSkip,
                                        MMP_BOOL    *bSkipFrame,
                                        MMP_ULONG   *pending_bytes);
MMP_ERR     MMPF_VidRateCtl_Init(       void        **handle,
                                        MMP_ULONG   idx,
                                        MMP_USHORT  gsVidRecdFormat,
                                        MMP_LONG    targetsize,
                                        MMP_ULONG   framerate,
                                        MMP_ULONG   nP,
                                        MMP_ULONG   nB,
                                        MMP_BOOL    PreventBufOverflow,
                                        RC_CONFIG_PARAM *RcConfig,
                                        MMP_ULONG   fps);
MMP_ERR     MMPF_VidRateCtl_DeInit(     void        *RCHandle,
                                        MMP_ULONG   handle_idx,
                                        MMP_LONG    target_framesize,
                                        MMP_ULONG   bit_rate,
                                        RC_CONFIG_PARAM *RcConfig,
                                        MMP_ULONG   fps);
void        MMPF_VidRateCtl_ResetBitrate(void       *RCHandle,
                                        MMP_LONG    bit_rate,
                                        MMP_ULONG   framerate,
                                        MMP_BOOL    ResetParams,
                                        MMP_ULONG   ulVBVSize,
                                        MMP_BOOL    bResetBufUsage);
void        MMPF_VidRateCtl_SetQPBoundary(void      *RCHandle,
                                        MMP_ULONG   frame_type,
                                        MMP_LONG    QP_LowerBound,
                                        MMP_LONG    QP_UpperBound);
void        MMPF_VidRateCtl_GetQPBoundary(void      *RCHandle,
                                        MMP_ULONG   frame_type,
                                        MMP_LONG    *QP_LowerBound,
                                        MMP_LONG    *QP_UpperBound);
void        MMPF_VidRateCtl_ResetBufSize(void       *RCHandle,
                                        MMP_LONG    BufSize);

/// @}

#endif	// _MMPF_MP4VENC_H_
