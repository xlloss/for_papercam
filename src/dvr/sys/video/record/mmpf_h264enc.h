//==============================================================================
//
//  File        : mmpf_h264enc.h
//  Description : Header function of video codec
//  Author      : Will Tseng
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_H264ENC_H_
#define _MMPF_H264ENC_H_

#include "includes_fw.h"
#include "mmpf_3gpmgr.h"
#include "mmpf_vidcmn.h"

/** @addtogroup MMPF_VIDEO
@{
*/

//==============================================================================
//
//                              COMPILER OPTION
//
//==============================================================================

#define BD_LOW                  (0)
#define BD_HIGH                 (1)

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

typedef enum _MMPF_H264ENC_INTRA_REFRESH_MODE {
    MMPF_H264ENC_INTRA_REFRESH_DIS = 0,
    MMPF_H264ENC_INTRA_REFRESH_MB,
    MMPF_H264ENC_INTRA_REFRESH_ROW
} MMPF_H264ENC_INTRA_REFRESH_MODE;

typedef enum _MMPF_H264ENC_PADDING_TYPE {
    MMPF_H264ENC_PADDING_NONE,
    MMPF_H264ENC_PADDING_ZERO,
    MMPF_H264ENC_PADDING_REPEAT
} MMPF_H264ENC_PADDING_TYPE;

typedef enum _MMPF_H264ENC_HBR_MODE {
    MMPF_H264ENC_HBR_MODE_60P,
    MMPF_H264ENC_HBR_MODE_30P,
    MMPF_H264ENC_HBR_MODE_MAX
} MMPF_H264ENC_HBR_MODE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef struct _MMPF_H264ENC_PADDING_INFO {
    MMPF_H264ENC_PADDING_TYPE type;
    MMP_UBYTE ubPaddingCnt;
} MMPF_H264ENC_PADDING_INFO;

typedef struct _MMPF_H264ENC_MEMD_PARAM {
    MMP_USHORT  usMeStopThr[2];             ///< 0: low, 1: high
    MMP_USHORT  usMeSkipThr[2];             ///< 0: low, 1: high
} MMPF_H264ENC_MEMD_PARAM;

typedef struct _MMPF_H264ENC_FUNC_STATES {
    MMP_USHORT      ProfileIdc;
    VIDENC_ENTROPY  EntropyMode;
} MMPF_H264ENC_FUNC_STATES;

typedef struct _MMPF_H264ENC_MODULE {
    MMP_BOOL                      bWorking;
    struct _MMPF_H264ENC_ENC_INFO *pH264Inst;
    MMPF_H264ENC_FUNC_STATES      HwState;
} MMPF_H264ENC_MODULE;

#if (CHIP == MCR_V2)
//Image Compression
#define ICOMP_LSY_STATIC_REDUCTION_IDX      8
#define ICOMP_LSY_BLK_SIZE_IDX              0
#define ICOMP_LSY_YUVRNG_IDX                1
#define ICOMP_LSY_MAX_LVL_IDX               2
#define ICOMP_LSY_LUMA_BLK_SIZE_OFFSET      3       ///< The offset to change the block size setting value to fit hardware opr value(0:4x4, 1: 8x8) 
#define ICOMP_LSY_CHR_BLK_SIZE_OFFSET       2
#define ICOMP_LSY_YUVRNG_OFFSET             1
#define ICOMP_LSY_MAX_LVL_OFFSET            5
#define ICOMP_LSY_MEAN_OFFSET               0
#define ICOMP_LSY_CONTRAST_OFFSET           1
#define ICOMP_LSY_DIFF_THR_OFFSET           2

typedef struct _MMPF_H264ENC_ICOMP {
    MMP_BOOL bICompEnable;
    MMP_BOOL bICompCurMbLsyEn;
    MMP_BOOL bICompLsyLvlCtlEn;
    MMP_UBYTE ubICompRatio;
    MMP_UBYTE ubICompRatioIndex;
    MMP_UBYTE ubICompMinLsyLvlLum;
    MMP_UBYTE ubICompMinLsyLvlChr;
    MMP_UBYTE ubICompIniLsyLvlLum;
    MMP_UBYTE ubICompIniLsyLvlChr;
    MMP_UBYTE ubICompMaxLsyLvlLum;
    MMP_UBYTE ubICompMaxLsyLvlChr;
    MMP_ULONG ulICompFrmSize;
} MMPF_H264ENC_ICOMP;
#endif

#if (H264ENC_TNR_EN)
typedef struct _MMPF_H264ENC_TNR_FILTER {
    MMP_UBYTE   luma_4x4;
    MMP_UBYTE   chroma_4x4;
    MMP_USHORT  thr_8x8;
} MMPF_H264ENC_TNR_FILTER;

typedef struct _MMPF_H264ENC_TNR {
    MMP_USHORT  low_mv_x_thr;
    MMP_USHORT  low_mv_y_thr;
    MMP_USHORT  zero_mv_x_thr;
    MMP_USHORT  zero_mv_y_thr;
    MMP_UBYTE   zero_luma_pxl_diff_thr;
    MMP_UBYTE   zero_chroma_pxl_diff_thr;
    MMP_UBYTE   zero_mv_4x4_cnt_thr; 
    MMPF_H264ENC_TNR_FILTER low_mv_filter;
    MMPF_H264ENC_TNR_FILTER zero_mv_filter;
    MMPF_H264ENC_TNR_FILTER high_mv_filter;
} MMPF_H264ENC_TNR;
#endif

#define I_FRAME                             (MMPF_3GPMGR_FRAME_TYPE_I)
#define P_FRAME                             (MMPF_3GPMGR_FRAME_TYPE_P)
#define B_FRAME                             (MMPF_3GPMGR_FRAME_TYPE_B)

#define WINDOW_SIZE                         (20)
#define MAX_SUPPORT_LAYER                   (MAX_NUM_TMP_LAYERS)

typedef struct {
    MMP_ULONG   LayerBitRate[MAX_SUPPORT_LAYER];
    MMP_ULONG   BitRate[MAX_SUPPORT_LAYER];
    MMP_ULONG   VBV_LayerSize[MAX_SUPPORT_LAYER];
    MMP_ULONG   VBV_size[MAX_SUPPORT_LAYER];
    MMP_LONG    VBV_fullness[MAX_SUPPORT_LAYER];
    MMP_ULONG   TargetVBVLevel[MAX_SUPPORT_LAYER];
    MMP_ULONG   TargetVBV[MAX_SUPPORT_LAYER];
    MMP_ULONG   VBVRatio[MAX_SUPPORT_LAYER];
} VBV_PARAM;

typedef struct {
    MMP_ULONG   bitrate;
    MMP_LONG    intra_period;
    MMP_LONG    VBV_fullness;
    MMP_LONG    VBV_size;
    MMP_LONG    target_framesize;

    MMP_LONG    bits_budget;
    MMP_LONG    total_frames;
    MMP_LONG    left_frames[3];
    MMP_LONG    Iframe_num;
    MMP_LONG    Pframe_num;
    MMP_LONG    total_framesize[3];
    //MMP_LONG    total_I_size;
    //MMP_LONG    total_P_size;
    //MMP_LONG    total_B_size;
    //MMP_LONG    last_qp;
    //MMP_LONG    lastXP;

    MMP_LONG    last_X[3];
    MMP_LONG    last_X2[3];
    unsigned long long    X[3];
    MMP_LONG    X_Count[3];
    unsigned long long    X2[3][WINDOW_SIZE];
    MMP_LONG    X_Idx[3];
    MMP_LONG    count[3];
    MMP_LONG    frame_count;
    MMP_LONG    target_P;
    MMP_LONG    last_bits;
    //MMP_LONG    last_IQP;
    //MMP_LONG    last_Bqp;
    MMP_ULONG   lastQP[3];
    //MMP_LONG    clip_qp;

    MMP_LONG    prev_window_size[3];
    MMP_LONG    window_size[3];

    //MMP_LONG    AlphaI;
    //MMP_LONG    AlphaB;
    MMP_LONG    Alpha[3];
    //MMP_LONG    avg_xp;
    MMP_LONG    avg_qp[3];

    //MMP_LONG    is_vbr_mode;
    MMP_LONG    rc_mode;
    MMP_LONG    enable_vbr_mode;
    MMP_LONG    GOP_frame_count;
    MMP_LONG    GOP_count;
    MMP_LONG    GOP_totalbits;
    MMP_LONG    QP_sum;
    MMP_LONG    GOP_QP[3];
    MMP_LONG    GOP_left_frames[3];

    MMP_LONG    GOP_num_per_I_period;
    MMP_LONG    GOP_count_inside_I_period;
    
    MMP_LONG    last_headerbits[3];
    MMP_ULONG   nP;
    MMP_ULONG   nB;
    
    MMP_ULONG   header_bits[3][WINDOW_SIZE];
    MMP_ULONG   header_index[3];
    MMP_ULONG   header_count[3];
    MMP_ULONG   avg_header[3];
    
    MMP_ULONG   avgXP[3];
    //test
    MMP_LONG    budget;
    MMP_ULONG   targetPSize;
    
    MMP_LONG    vbr_budget;
    MMP_LONG    vbr_total_frames;
	MMP_ULONG	framerate;
	MMP_BOOL	SkipFrame;
	
	MMP_LONG	GOPLeftFrames;
	MMP_BOOL	bResetRC;
	MMP_ULONG   light_condition;
	
	//MMP_LONG	TargetLowerBound;
	//MMP_LONG	TargetUpperBound;
	
	MMP_ULONG   MaxQPDelta[3];
	MMP_ULONG	MaxWeight[3];		//1.5 * 1000
	MMP_ULONG	MinWeight[3];		//1.0 * 1000
	MMP_ULONG	VBV_Delay;			//500 ms
	MMP_ULONG	TargetVBVLevel;		//250 ms
	MMP_ULONG   FrameCount;
	MMP_BOOL    SkipPrevFrame;
	MMP_ULONG   SkipFrameThreshold;
	
	MMP_ULONG   m_LowerQP[3];
    MMP_ULONG   m_UpperQP[3];
    MMP_ULONG   m_VideoFormat;
    MMP_ULONG   m_ResetCount;
    MMP_ULONG   m_GOP;
    MMP_ULONG   m_Budget;
    MMP_ULONG64 m_AvgHeaderSize;
    MMP_ULONG   m_lastQP;
    MMP_UBYTE   m_ubFormatIdx;
    MMP_ULONG   MBWidth;
	MMP_ULONG   MBHeight;
	MMP_ULONG   TargetVBV;
	MMP_BOOL    bPreSkipFrame;
	
	MMP_ULONG	VBVRatio;
	MMP_ULONG	bUseInitQP;
	
	MMP_ULONG   m_LastType;
	
	//++Will RC
	void*       pGlobalVBV;
	MMP_ULONG   LayerRelated;
	MMP_ULONG   Layer;
	//--Will RC
} RC;

typedef struct {
	MMP_ULONG	MaxIWeight;			//1.5 * 1000
	MMP_ULONG	MinIWeight;			//1.0 * 1000
	MMP_ULONG	MaxBWeight;			//1.0 * 1000
	MMP_ULONG	MinBWeight;			//0.5 * 1000
	MMP_ULONG	VBV_Delay;			//500 ms
	MMP_ULONG	TargetVBVLevel;		//250 ms
	MMP_ULONG	InitWeight[3];
	MMP_ULONG	MaxQPDelta[3];
	MMP_ULONG   SkipFrameThreshold;
	MMP_ULONG   MBWidth;
	MMP_ULONG   MBHeight;
	MMP_ULONG   InitQP[3];
	MMP_ULONG   rc_mode;
	MMP_ULONG   bPreSkipFrame;
	
	//++Will RC
	MMP_ULONG   LayerRelated;
	MMP_ULONG   Layer;
	MMP_ULONG   EncID;
	//--Will RC
} RC_CONFIG_PARAM;

typedef struct _MMPF_VIDENC_GLBSTATUS{
	MMP_USHORT                	VidOp;        		    ///< operation from host command.
    MMP_USHORT                	VidStatus;              ///< status of video engine.
    MMP_UBYTE                 	VideoCaptureStart;		///< flag of start record.	
} MMPF_VIDENC_GLBSTATUS;

typedef struct _MMPF_H264ENC_ENC_INFO {
    // Sequence Level
    MMP_ULONG                   enc_id;

    MMP_USHORT                  profile;
    MMP_ULONG                   mb_num;
    MMP_USHORT                  mb_w;
    MMP_USHORT                  mb_h;
    MMPF_H264ENC_PADDING_INFO   paddinginfo;
	#if (H264ENC_ICOMP_EN)
    MMPF_H264ENC_ICOMP          ICompConfig;
	#endif
    MMP_USHORT                  level;
    MMP_USHORT                  b_frame_num;
    MMP_ULONG                   conus_i_frame_num;      ///< contiguous I-frame count
    MMP_ULONG                   gop_frame_num;          ///< zero if sync frame is output
    MMP_USHORT                  gop_size;
    MMP_ULONG                   mv_addr;
    MMP_BOOL                    co_mv_valid;
    MMP_BOOL                    insert_sps_pps;
    MMP_ULONG                   stream_bitrate;         ///< total bitrate
    MMP_ULONG                   target_bitrate;         ///< target bitrate when ramup , if set to zero, no rampup
    MMP_ULONG                   rampup_bitrate;
    /* skip frame for rate control */
    MMP_BOOL                    rc_skippable;           ///< rc can skip frames or not
    MMP_BOOL                    rc_skip_smoothdown;     ///< 0: direct skip, 1: smooth skip
    MMP_BOOL                    rc_skip_bypass;         ///< bypass current frame RC skip

    /* config for rc temporal layers */
    MMP_ULONG                   layer_bitrate[MAX_NUM_TMP_LAYERS];      ///< per-layer bitrate
    MMP_ULONG                   layer_lb_size[MAX_NUM_TMP_LAYERS];      ///< leakybucket size per-layer in ms
    MMP_ULONG                   layer_tgt_frm_size[MAX_NUM_TMP_LAYERS]; ///< target frame size for each layer
    MMP_ULONG                   layer_frm_thr[MAX_NUM_TMP_LAYERS];      ///< threshold to skip encoding
    MMP_ULONG                   layer_fps_ratio[MAX_NUM_TMP_LAYERS];    ///< fps ratio for each layer
    #if (H264ENC_LBR_EN)
    MMP_ULONG                   layer_fps_ratio_low_br[MAX_NUM_TMP_LAYERS_LBR];
    #endif

    RC_CONFIG_PARAM             rc_config[MAX_NUM_TMP_LAYERS];
    void                        *layer_rc_hdl[MAX_NUM_TMP_LAYERS];
    MMP_UBYTE                   total_layers;

    MMPF_VIDENC_DUMMY_DROP_INFO dummydropinfo;
    VIDENC_ENTROPY              entropy_mode;
    MMPF_VIDENC_CROPPING        crop;

    MMP_ULONG                   TotalEncodeSize;
    MMP_ULONG                   total_frames;			///< total encoded frames
    MMP_ULONG                   prev_ref_num;			///< increase when cur frame can be ref
    MMPF_VIDENC_CURBUF_MODE     CurBufMode;
    MMPF_H264ENC_MEMD_PARAM     Memd;
    MMP_UBYTE                   qp_tune_mode;			///< mb, row, slice, frame
    MMP_UBYTE                   qp_tune_size;			///< unit size
    MMP_USHORT                	inter_cost_th;			///< reset 1024
    MMP_USHORT                	intra_cost_adj;			///< reset 18
    MMP_USHORT                	inter_cost_bias;		///< reset 0
    MMP_USHORT                	me_itr_steps;
    MMP_ULONG                 	temporal_id;			///< I:0, P:1(ref),2(non-ref)
    MMP_ULONG64               	timestamp;
    MMP_BOOL                  	video_full_range;		///< indicate video input is 16-255
    MMP_BOOL                    bRcEnable;
    #if (H264ENC_RDO_EN)
    MMP_BOOL                  	bRdoEnable;
    MMP_BOOL                  	bRdoMadValid;
    MMP_ULONG                 	ulRdoMadFrmPre;
    MMP_ULONG                 	ulRdoInitFac;
    MMP_ULONG                 	ulRdoMadLastRowPre;
    MMP_UBYTE                 	ubRdoMissThrLoAdj;
    MMP_UBYTE                 	ubRdoMissThrHiAdj;
    MMP_UBYTE                 	ubRdoMissTuneQp;
    #endif
    MMP_LONG                    MbQpBound[MAX_NUM_TMP_LAYERS][MMPF_3GPMGR_FRAME_TYPE_MAX][2];
    MMP_LONG                    CurRcQP[MAX_NUM_TMP_LAYERS][MMPF_3GPMGR_FRAME_TYPE_MAX];

    MMP_ULONG                   ulParamQueueRdWrapIdx;	///< MS3Byte wrap, LSByte idx
    MMP_ULONG                   ulParamQueueWrWrapIdx;	///< MS3Byte wrap, LSByte idx
    MMPF_VIDENC_PARAM_CTL       ParamQueue[MAX_NUM_PARAM_CTL];
    MMPF_3GPMGR_FRAME_TYPE      OpFrameType;
    MMP_BOOL                    OpIdrPic;
    MMP_BOOL                    OpInsParset;
    MMP_UBYTE                   CurRTSrcPipeId;

    // runtime, slice level
    MMP_USHORT                  usMaxNumRefFrame;
    MMP_UBYTE                   usMaxFrameNumAndPoc;

    MMP_ULONG                   slice_addr;

    MMP_ULONG                   RefGenBufLowBound;
    MMP_ULONG                   RefGenBufHighBound;
    MMPF_VIDENC_FRAMEBUF_BD     RefBufBound;
    MMPF_VIDENC_FRAMEBUF_BD     GenBufBound;

    MMPF_3GPMGR_FRAME_TYPE      cur_frm_type;
    MMPF_VIDENC_FRAME           cur_frm[4];        		///< frame buffer addr and timestamp info
    MMPF_VIDENC_FRAME           ref_frm;           	//m_RefGenFrame
    MMPF_VIDENC_FRAME           rec_frm;

    MMP_UBYTE                   enc_frm_buf;       		///< buffer idx for encode

    MMP_ULONG                 	ulEncFpsRes;       		///< host controled max fps
    MMP_ULONG                 	ulEncFpsInc;       		///< host controled max fps
    MMP_ULONG                 	ulEncFps1000xInc;  		///< host controled max fps
    MMP_ULONG                 	ulSnrFpsRes;       		///< host controled max fps
    MMP_ULONG                 	ulSnrFpsInc;       		///< host controled max fps
    MMP_ULONG                 	ulSnrFps1000xInc;  		///< host controled max fps
    MMP_ULONG                 	ulFpsInputRes;     		///< input framerate resolution
    MMP_ULONG                 	ulFpsInputInc;     		///< input framerate increament
    MMP_ULONG                 	ulFpsInputAcc;     		///< input framerate accumulate
    MMP_ULONG                 	ulFpsOutputRes;    		///< enc framerate resolution
    MMP_ULONG                 	ulFpsOutputInc;    		///< enc framerate increament
    MMP_ULONG                 	ulFpsOutputAcc;    		///< enc framerate accumulate

    MMP_ULONG                   ulOutputAvailSize;
    MMP_ULONG                   cur_frm_bs_addr;   	///< bs addr for current enc frame
    MMP_ULONG                   cur_frm_bs_high_bd;	///< bs addr for current enc frame
    MMP_ULONG                   cur_frm_bs_low_bd; 	///< bs addr for current enc frame
    MMP_ULONG                   cur_frm_wr_size;   	///< accumulating size write to bs buf
    MMP_ULONG                   cur_frm_rd_size;   	///< accumulating size read from bs buf
    MMP_ULONG                   cur_frm_parset_num;	///< hw sps/pps slice num
    MMP_ULONG                   cur_frm_slice_num; 	///< hw total slice num(incluing parset)
    MMP_ULONG                   cur_frm_slice_idx; 	///<

    // buffer addr and buffer for modify header
    MMP_ULONG                   cur_slice_wr_idx;  	///<
    MMP_ULONG                   cur_slice_rd_idx;		///<
    MMP_ULONG                   cur_slice_xhdr_sum;	///< generated slice header size sum
    MMP_ULONG                   cur_slice_parsed_sum;	///< parsed slice header size sum

    MMP_ULONG                   sps_len;
    MMP_ULONG                   pps_len;

    VidEncEndCallBackFunc       *EncReStartCallback;
    VidEncEndCallBackFunc       *EncEndCallback;

	MMPF_VIDENC_GLBSTATUS       GlbStatus;
    MMPF_H264ENC_MODULE         *module;
    void                        *priv_data;
    void                        *Container;

    #if (VID_CTL_DBG_BR)
    MMP_ULONG                   br_total_size;
    MMP_ULONG                   br_base_time;
    MMP_ULONG                   br_last_time;
    #endif

    #if (H264ENC_LBR_EN)
    MMP_UBYTE                   ubAccResetLowBrRatioTimes;
    MMP_UBYTE                   ubAccResetHighBrRatioTimes;
    MMP_BOOL                    bResetLowBrRatio;
    MMP_BOOL                    bResetHighBrRatio;
    MMP_ULONG                   intra_mb;
    MMP_UBYTE                   decision_mode;
    #endif

    #if (H264ENC_TNR_EN)
    MMP_UBYTE                   tnr_en;
    MMPF_H264ENC_TNR            tnr_ctl; 
    #endif

    #if (H264ENC_RDO_EN)
    MMP_UBYTE                   qstep3[10];
    #endif
    
    #if H264ENC_GDR_EN
    MMP_BOOL                    intra_refresh_en     ;
    MMP_BOOL                    intra_refresh_trigger;    
    MMP_USHORT                  intra_refresh_period ;
    MMP_USHORT                  intra_refresh_offset ;
    #endif
} MMPF_H264ENC_ENC_INFO;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMPF_H264ENC_ENC_INFO *MMPF_H264ENC_GetHandle(MMP_UBYTE ubEncId);

MMP_ERR MMPF_H264ENC_SetPadding(MMPF_H264ENC_ENC_INFO       *pEnc,
                                MMPF_H264ENC_PADDING_TYPE   ubPaddingType,
                                MMP_UBYTE                   ubPaddingCnt);
MMP_ERR MMPF_H264ENC_InitImageComp( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_H264ENC_ICOMP      *pConfig);
MMP_ERR MMPF_H264ENC_SetH264ByteCnt(MMP_USHORT usByteCnt);

MMP_ERR MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_MODE ubMode,
                                    MMP_USHORT          usPeriod,
                                    MMP_USHORT          usOffset);
MMP_ERR MMPF_H264ENC_EnableTnr(MMPF_H264ENC_ENC_INFO *pEnc);
void    MMPF_H264ENC_EnableRdo(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR MMPF_H264ENC_UpdateModeDecision(MMPF_H264ENC_ENC_INFO *pEnc);

MMP_ERR MMPF_H264ENC_SetBSBuf(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulLowBound,
                                MMP_ULONG               ulHighBound);
MMP_ERR MMPF_H264ENC_SetMiscBuf(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulMVBuf,
                                MMP_ULONG               ulSliceLenBuf);
MMP_ERR MMPF_H264ENC_SetRefListBound(MMPF_H264ENC_ENC_INFO  *pEnc,
                                    MMP_ULONG               ulLowBound,
                                    MMP_ULONG               ulHighBound);
MMP_ERR MMPF_H264ENC_SetParameter(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_VIDENC_ATTRIBUTE   attrib,
                                    void                    *arg);
MMP_ERR MMPF_H264ENC_GetParameter(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_VIDENC_ATTRIBUTE   attrib,
                                    void                    *ulValue);
MMP_ERR MMPF_H264ENC_InitRCConfig(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR MMPF_H264ENC_SetQPBound(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               layer,
                                MMPF_3GPMGR_FRAME_TYPE  type,
                                MMP_LONG                lMinQp,
                                MMP_LONG                lMaxQp);
MMP_ERR MMPF_H264ENC_InitModule(MMPF_H264ENC_MODULE *pModule);
MMP_ERR MMPF_H264ENC_DeinitModule(MMPF_H264ENC_MODULE *pModule);
MMP_ERR MMPF_H264ENC_InitInstance(  MMPF_H264ENC_ENC_INFO   *enc,
                                    MMPF_H264ENC_MODULE     *attached_mod,
                                    void                    *priv);
MMP_ERR MMPF_H264ENC_DeInitInstance(MMPF_H264ENC_ENC_INFO   *enc,
                                    MMP_ULONG               InstId);
void instance_parameter_config(MMPF_H264ENC_ENC_INFO *enc);
MMP_ERR MMPF_H264ENC_InitRefListConfig (MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR MMPF_H264ENC_Open(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR MMPF_H264ENC_Resume(MMPF_H264ENC_ENC_INFO *pEnc);
MMP_ERR MMPF_H264ENC_TriggerEncode( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_3GPMGR_FRAME_TYPE  FrameType,
                                    MMP_ULONG               ulFrameTime,
                                    MMPF_VIDENC_FRAME       *pFrameInfo);

#endif	// _MMPF_H264ENC_H_
