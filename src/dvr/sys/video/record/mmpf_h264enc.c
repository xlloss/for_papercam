/**
@file mmpf_h264enc.c
@brief h264 encode related function
@version 1.0
*/

#include "includes_fw.h"
#if (VIDEO_R_EN)
#include "lib_retina.h"
#include "mmp_reg_h264enc.h"
#include "mmp_reg_h264dec.h"
#include "mmp_reg_ibc.h"
#include "mmp_reg_gbl.h"
#include "mmpf_vstream.h"
#include "mmpf_mp4venc.h"
#include "mmpf_h264enc.h"
#include "mmpf_display.h"
#include "mmpf_scaler.h"
#include "mmpf_timer.h"
#include "mmpf_ibc.h"
#include "mmpf_system.h"
#include "mmpf_audio_ctl.h"
#if (HANDLE_EVENT_BY_TASK == 1)
#include "mmpf_event.h"
#endif
#if (FPS_CTL_EN)
#include "mmpf_fpsctl.h"
#endif
#include "isp_if.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

#if (H264ENC_HBR_EN)
static MMP_UBYTE        m_ubHbrSel = MMPF_H264ENC_HBR_MODE_30P;
#endif
#if (WORK_AROUND_EP3)
static MMP_BOOL         m_bEP3BHappens = MMP_FALSE;
#endif
static MMP_ULONG        m_ulFrameStartAddr; ///< start address of each output frame.

MMPF_VIDENC_FRAME       *m_RefFrame, *m_GenFrame;

extern MMP_USHORT       m_usVidRecPath[MAX_VIDEO_STREAM_NUM];

extern MMP_BOOL         gbRtEncDropFrameEn;

extern volatile MMP_BOOL    m_bRtCurBufOvlp;
extern volatile MMP_BOOL    m_bRtScaleDFT;

extern MMPF_OS_FLAGID       VID_REC_Flag;

extern MMPF_VIDENC_QUEUE    gVidRecdRdyQueue[MAX_VIDEO_STREAM_NUM];
extern MMPF_VIDENC_QUEUE    gVidRecdFreeQueue[MAX_VIDEO_STREAM_NUM];
extern MMPF_VIDENC_QUEUE    gVidRecdDtsQueue[MAX_VIDEO_STREAM_NUM];

RC_CONFIG_PARAM gRcConfig = {
    #if (H264ENC_LBR_EN)
    2000,   //MaxIWeight
    1000,   //MinIWeight
    #else
    7168, //dbg ori: 3000,   //MaxIWeight
    1024, //dbg ori: 1000,   //MinIWeight
    #endif
    1000,   //MaxBWeight
    500,    //MinBWeight
    500000, //VBV_Delay, LB(ms)*BR/1000
    500,    //TargetVBVLevel
    #if (H264ENC_LBR_EN)
    {1000,1000,800},
    #else
    {6000,1000,800},
    //dbg ori: {5000,1000,800},    //Init Weight[3]
    #endif
    {0,0,0},//MaxQPDelta[3]
    900,    //SkipFrameThreshold
    320,    //MBWidth
    240,    //MBHeight
    {32,32,32},//InitQP[3]
    VIDENC_RC_MODE_CBR, //rc_mode
    0,      //bPreSkipFrame
    0,      //LayerRelated
    0,      //Layer
    0       //EncID
};

void            *m_RCHandle;

#if (H264ENC_ICOMP_EN)
// block-based lossy table:
// 0.0) [7:0]={1'b use_mean, 4'b contrast, 4'b diff_thr};
//      [8]={1'b blk_size, 4'b yuvrange, 3'b max_lvl};
const int blk_lsy_tbl[16][9][3] = {
//[0]        [1]       [2]       [3]       [4]       [5]       [6]       [7]       setting
//------------------------------------------------------------------------------
//==(ideal: compression ratio,simulation: compression ratio)==//    
//==(55%,55%)==//       
  {{0,16, 0},{1,15, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{0,16, 3}},  // lum //0
  {{0,16, 0},{0,15, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0,16, 3}},  // chr
//==(50%,50%)==//       
  {{0,16, 0},{1,15, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{0,16, 7}},  // lum //2
  {{0,16, 0},{0,15, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0,16, 7}},  // chr
//==(45%,48%)==//       
//{{0,16, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{1, 8, 0},{0,16, 7}},  // lum 
//{{0,16, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0, 8, 0},{0,16, 7}},  // chr
//==(43%,46%)==//       
  {{0,16, 0},{1,15, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{0,13, 7}},  // lum //4
  {{0,16, 0},{0,15, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0,14, 7}},  // chr
//------------------------------------------------------------------------------
//==(42%,45%)==//
//{{0,16, 0},{1,14, 0},{1,12, 0},{1,10, 0},{1, 9, 0},{1, 8, 0},{1, 7, 0},{1, 6, 0},{0,16, 7}},  // lum 
//{{0,16, 0},{0,14, 0},{0,12, 0},{0,10, 0},{0, 9, 0},{0, 8, 0},{0, 7, 0},{0, 6, 0},{0,16, 7}},  // chr
//==(42%,45%)==//       
//{{0,16, 0},{1,15, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{0,12, 7}},  // lum 
//{{0,16, 0},{0,15, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0,13, 7}},  // chr
//==(41%,43%)==//
//{{0,16, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{1, 8, 0},{0,11, 7}},  // lum 
//{{0,16, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0, 8, 0},{0,12, 7}},  // chr
//==(41%,43%)==//       
/*{{0,16, 0},{1,14, 0},{1,13, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{1, 8, 0},{1,11, 7}},*/// lum 
/*{{0,16, 0},{0,14, 0},{0,13, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0, 8, 0},{1,12, 7}},*/// chr
//==(40%,42%)==//
  {{0,16, 0},{1,14, 0},{1,12, 0},{1,11, 0},{1,10, 0},{1, 9, 0},{1, 8, 0},{1, 7, 0},{0,10, 7}},  // lum //6
  {{0,16, 0},{0,14, 0},{0,12, 0},{0,11, 0},{0,10, 0},{0, 9, 0},{0, 8, 0},{0, 7, 0},{0,12, 7}},  // chr
//==(39%,41%)==//       
  {{0,16, 0},{1,14, 8},{1,13, 7},{1,12, 6},{1,11, 5},{1,10, 4},{1, 9, 3},{1, 8, 2},{0,10, 7}},  // lum //8
  {{0,16, 0},{0,14, 8},{0,13, 7},{0,12, 6},{0,11, 5},{0,10, 4},{0, 9, 3},{0, 8, 2},{0,12, 7}},  // chr
//------------------------------------------------------------------------------
//==(35%,35%)==//       
  {{0,15,15},{0,13,13},{0,11,11},{0, 9, 9},{0, 7, 7},{0, 5, 5},{0, 3, 3},{0, 1, 1},{0,10, 7}},  // lum //10
  {{0,15,15},{0,13,13},{0,11,11},{0, 9, 9},{0, 7, 7},{0, 5, 5},{0, 3, 3},{0, 1, 1},{0,12, 7}},  // chr
//==(30%,30%)==//       
  {{0,16, 0},{1,14,14},{1,12,12},{1,10,10},{1, 8, 8},{1, 6, 6},{1, 4, 4},{1, 2, 2},{1,10, 7}},  // lum //12
  {{0,16, 0},{0,14,14},{0,12,12},{0,10,10},{0, 8, 8},{0, 6, 6},{0, 4, 4},{0, 2, 2},{1,12, 7}},  // chr
//==(10%,10%)==//       
  {{1, 1, 0},{1, 1, 0},{1, 1, 0},{1, 1, 0},{1, 1, 0},{1, 1, 0},{1, 1, 0},{1, 1, 0},{0,16, 7}},  // lum //14
  {{0, 1, 0},{0, 1, 0},{0, 1, 0},{0, 1, 0},{0, 1, 0},{0, 1, 0},{0, 1, 0},{0, 1, 0},{0,16, 7}}   // chr
};
#endif

#if (H264ENC_TNR_EN)
static MMPF_H264ENC_TNR m_TnrDefCfg = {
    0x0040, // TNR_MCTF_MVX_THR_LOW
    0x0040, // TNR_MCTF_MVY_THR_LOW  
    4,     // TNR_ZERO_MVX_THR_LOW
    4,     // TNR_ZERO_MVY_THR_LOW
    0xc,    // TNR_ZERO_MV_LUMA_PIXEL_DIFF_THR
    0xc,    // TNR_ZERO_MV_CHRO_PIXEL_DIFF_THR
    0x37,    // NR_ZERO_MV_THR_4X4
    {0x90,0xe4,0x00e5}, // low  motion
    {0xe4,0xf9,0x50fa}, // zero motion
    {0x50,0xe4,0x0094}   
} ;
#endif

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void rt_enc_end_init_callback(void* arg);
void rt_enc_restart_callback(void* arg);
void rt_enc_scaledfs_restart_callback(void* arg); 

static MMP_ERR MMPF_H264ENC_SetRuntimeParam(MMPF_H264ENC_ENC_INFO *pEnc,
                                            MMPF_VIDENC_ATTRIBUTE attrib,
                                            MMP_ULONG ulValue);
static MMP_ERR MMPF_H264ENC_SetInstanceParam(MMPF_H264ENC_ENC_INFO *pEnc,
                                             MMPF_VIDENC_ATTRIBUTE attrib,
                                             MMP_ULONG ulValue);
static MMP_ERR MMPF_H264ENC_UpdateRateControl(MMPF_H264ENC_ENC_INFO *pEnc,
                                              MMP_ULONG   ulFpsInRes,
                                              MMP_ULONG   ulFpsInInc,
                                              MMP_ULONG   ulFpsOutRes,
                                              MMP_ULONG   ulFpsOutInc);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

void rt_enc_end_init_callback(void* arg)
{
    MMP_ULONG               InstanceId;
    MMP_ULONG               ulBufId, ulDts;
    MMP_ULONG               ulMaxDup, ulVidCnt = 1;
    MMPF_3GPMGR_FRAME_TYPE  FrameType;
    MMPF_H264ENC_ENC_INFO   *pEnc = (MMPF_H264ENC_ENC_INFO *)arg;

    InstanceId = get_vidinst_id(pEnc->priv_data);

    #if (FPS_CTL_EN)
    /* Frame rate control */
    if (MMPF_FpsCtl_IsEnabled(InstanceId)) {
        MMP_BOOL    bDrop;
        MMP_ULONG   ulDupCnt;

        MMPF_FpsCtl_Operation(InstanceId, &bDrop, &ulDupCnt);
        if (bDrop) {
            gbRtEncDropFrameEn = MMP_TRUE;
            MMPF_Scaler_EnableInterrupt(m_usVidRecPath[InstanceId] ,MMP_SCAL_EVENT_FRM_END, MMP_TRUE);
            MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[InstanceId], MMP_SCAL_EVENT_FRM_END, rt_enc_end_init_callback, (void *)pEnc);

            if (pEnc->module->bWorking == MMP_FALSE) {
                if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_PAUSE) {
                    pEnc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE;
                    MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(pEnc->enc_id), MMPF_OS_FLAG_SET);
                }
                if ((pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) || MMPF_HIF_GetCmdStatus(MEDIA_FILE_OVERFLOW)) {
                    MMPF_VIDENC_SetEncodeDisable(pEnc);
                }
            }
            return;
        }
        else if (ulDupCnt) {
        	pEnc->dummydropinfo.ulDummyFrmCnt += ulDupCnt;
        }
    }
    #endif

    // drop frame or dummy frame process
    if (pEnc->dummydropinfo.ulDropFrmCnt || pEnc->dummydropinfo.ulDummyFrmCnt) {
        if (pEnc->dummydropinfo.ulDropFrmCnt) {
            gbRtEncDropFrameEn = MMP_TRUE;
            pEnc->dummydropinfo.ulDropFrmCnt--;
            MMPF_Scaler_EnableInterrupt(m_usVidRecPath[InstanceId] ,MMP_SCAL_EVENT_FRM_END, MMP_TRUE);
            MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[InstanceId], MMP_SCAL_EVENT_FRM_END, rt_enc_end_init_callback, (void *)pEnc);

            RTNA_DBG_Str3("Vid drop\r\n");
            return;
        }

        if (pEnc->dummydropinfo.ulDummyFrmCnt) {
            #if (FPS_CTL_EN)
            ulMaxDup = pEnc->dummydropinfo.ulDummyFrmCnt;
            #else
            // max insert 3 continuous duplicated frames
            ulMaxDup = (pEnc->dummydropinfo.ulDummyFrmCnt > 3) ? 3 : pEnc->dummydropinfo.ulDummyFrmCnt;
            #endif
            ulVidCnt += ulMaxDup;

            do {
                // insert one dummy frame
                // TODO: How to indicated a dummy frame needed? by Alterman
                pEnc->dummydropinfo.ulDummyFrmCnt--;
                ulMaxDup--;
                RTNA_DBG_Str3("Vid dummy\r\n");
            } while(ulMaxDup);
	    }
    }

    if (gbRtEncDropFrameEn == TRUE) {
        MMPF_Scaler_EnableInterrupt(m_usVidRecPath[InstanceId], MMP_SCAL_EVENT_FRM_END, MMP_FALSE);
        gbRtEncDropFrameEn = MMP_FALSE;
    }

    ulBufId = 0;
    ulDts = OSTime;
    FrameType = MMPF_VIDENC_GetVidRecdFrameType(pEnc);

    MMPF_H264ENC_TriggerEncode(pEnc, FrameType, ulDts, &(pEnc->cur_frm[ulBufId]));
}

void rt_enc_restart_callback(void* arg)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;
    AITPS_H264ENC     pH264ENC = AITC_BASE_H264ENC;
    MMP_ULONG              InstanceId;
    MMPF_H264ENC_ENC_INFO  *pEnc = (MMPF_H264ENC_ENC_INFO *)arg;

    InstanceId = get_vidinst_id(pEnc->priv_data);

    if (m_bRtCurBufOvlp || m_bRtScaleDFT) {
        /* Here we don't want to call MMPF_SYS_ResetHModule() to reset
         * H.264 module, because it takes long time.
         */
        m_bRtCurBufOvlp = MMP_FALSE;
        m_bRtScaleDFT = MMP_FALSE;
        pGBL->GBL_SW_RST_EN[0] = GBL_RST_H264 |
                                (GBL_RST_IBC0 << m_usVidRecPath[InstanceId])|
                                (GBL_RST_ICON0 << m_usVidRecPath[InstanceId]);
        pGBL->GBL_SW_RST_DIS[0] = GBL_RST_H264 |
                                (GBL_RST_IBC0 << m_usVidRecPath[InstanceId])|
                                (GBL_RST_ICON0 << m_usVidRecPath[InstanceId]);

        pH264ENC->H264ENC_INT_CPU_SR  = 0xFFFF;
#if (WORK_AROUND_EP3)
        pH264ENC->H264ENC_INT_CPU_EN |= FRAME_ENC_DONE_CPU_INT_EN | ENC_CUR_BUF_OVWR_INT | EP_BYTE_CPU_INT_EN;
#else
        pH264ENC->H264ENC_INT_CPU_EN |= FRAME_ENC_DONE_CPU_INT_EN | ENC_CUR_BUF_OVWR_INT;
#endif
        MMPF_H264ENC_TriggerEncode(pEnc, MMPF_3GPMGR_FRAME_TYPE_I, OSTime,
                                   &(pEnc->cur_frm[0]));
    }
}

void rt_enc_scaledfs_restart_callback(void* arg) 
{
    AITPS_GBL              pGBL = AITC_BASE_GBL;
    MMP_ULONG              InstanceId;
    MMPF_H264ENC_ENC_INFO  *pEnc = (MMPF_H264ENC_ENC_INFO *)arg;

    InstanceId = get_vidinst_id(pEnc->priv_data);

    /* Here we don't want to call MMPF_SYS_ResetHModule() to reset
     * H.264 module, because it takes long time.
     */
    m_bRtScaleDFT = MMP_TRUE;
    pGBL->GBL_SW_RST_EN[0] = GBL_RST_H264;
    printc("Double Frame Start H264\r\n");
    pGBL->GBL_SW_RST_DIS[0] = GBL_RST_H264;
    MMPF_IBC_SetStoreEnable(m_usVidRecPath[InstanceId], MMP_FALSE);
}

/** @brief Returns pointers to H264 info structure
 @param[in] ubEncId ID of encoder to get handle
 @retval MMP_ERR_NONE Success.
*/
MMPF_H264ENC_ENC_INFO *MMPF_H264ENC_GetHandle(MMP_UBYTE ubEncId)
{
    return &(MMPF_VIDENC_GetInstance(ubEncId)->h264e);
}

/** @brief Set reference frame to HW OPR

 @param[in] enc points to encoder info structure
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPF_H264ENC_RestoreRefList (MMPF_H264ENC_ENC_INFO *enc)
{
    AITPS_H264ENC            pH264ENC      = AITC_BASE_H264ENC;
    AITPS_H264DEC_DBLK       pH264DBLK     = AITC_BASE_H264DEC_DBLK;
    #if (H264ENC_ICOMP_EN)
    AITPS_H264DEC_DBLK_ROT   pH264DBLK_ROT = AITC_BASE_H264DEC_DBLK_ROT;
    #endif
    MMP_ULONG                ulFrameYSize = enc->mb_num << 8;
    MMPF_VIDENC_FRAME        *pGen, *pRef;
    MMPF_VIDENC_FRAMEBUF_BD  *pGenBd, *pRefBd;

    switch(enc->cur_frm_type) {
    case MMPF_3GPMGR_FRAME_TYPE_I:
        pRef = &(enc->ref_frm);
        pGen = &(enc->rec_frm);
        pRefBd = &(enc->RefBufBound);
        pGenBd = &(enc->GenBufBound);
        break;
    case MMPF_3GPMGR_FRAME_TYPE_P:
        pRef = &(enc->ref_frm);
        pGen = &(enc->rec_frm);
        pRefBd = &(enc->RefBufBound);
        pGenBd = &(enc->GenBufBound);
        break;
    default:
        DBG_S(0, "Unknown frame type\r\n");
        return MMP_ERR_NONE;
    }

    pH264ENC->H264ENC_REF_Y_ADDR        = pRef->ulYAddr;
	pH264ENC->H264ENC_REF_UV_ADDR       = pRef->ulUAddr;
    pH264ENC->H264ENC_REFBD_Y_LOW       = pRef->ulYAddr;
    pH264ENC->H264ENC_REFBD_Y_HIGH      = pRef->ulYAddr + ulFrameYSize;
    pH264ENC->H264ENC_REFBD_UV_LOW      = pRef->ulUAddr;
    pH264ENC->H264ENC_REFBD_UV_HIGH     = pRef->ulUAddr + ulFrameYSize/2;
    pH264DBLK->DEST_ADDR                = pGen->ulYAddr; // Deblock out addr
    pH264DBLK->DEBLOCKING_CTRL          &= ~(DBLK_OUT_ROT_MODE);
    #if (H264ENC_ICOMP_EN)
    pH264DBLK_ROT->Y_LOWBD_ADDR         = pGen->ulYAddr;
    pH264DBLK_ROT->Y_HIGHBD_ADDR        = pGen->ulYAddr + ulFrameYSize;;
    pH264DBLK_ROT->UV_ST_ADDR           = pGen->ulUAddr;
    pH264DBLK_ROT->UV_LOWBD_ADDR        = pGen->ulUAddr;
    pH264DBLK_ROT->UV_HIGHBD_ADDR       = pGen->ulUAddr + ulFrameYSize/2;
    pH264DBLK->DEBLOCKING_CTRL          |= DBLK_OUT_ROT_MODE;
    #endif
	
    return MMP_ERR_NONE;
}

/** @brief Apply H/W OPR settings by transform modes

 @param[in] pEnc pointer to encoder info
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPF_H264ENC_RestoreTransform(   MMP_BOOL    bTrans8x8,
                                                MMP_BOOL    bIntra8x8)
{
    AITPS_H264ENC       pH264ENC = AITC_BASE_H264ENC;
    AITPS_H264DEC_VLD   pH264VLD = AITC_BASE_H264DEC_VLD;
    MMP_BOOL            bIsIntra8x8;

    bIsIntra8x8 = (pH264ENC->H264ENC_INTRA_PRED_MODE & INTRA_8x8_EN)?
                                            MMP_TRUE: MMP_FALSE;

    if (bTrans8x8) {
        pH264VLD->HIGH_PROF_REG_1   |= (H264_TRANS_8X8_FLAG|H264_DIRECT_8X8_FLAG);
        pH264ENC->H264ENC_TRANS_CTL |= TRANS_8x8_FLAG;
        if (bIntra8x8) {
            pH264ENC->H264ENC_INTRA_PRED_MODE |= INTRA_8x8_EN;
        }
        else {
            pH264ENC->H264ENC_INTRA_PRED_MODE &= ~(INTRA_8x8_EN);
        }
    }
    else {
        pH264VLD->HIGH_PROF_REG_1   &= ~(H264_TRANS_8X8_FLAG|H264_DIRECT_8X8_FLAG);
        pH264ENC->H264ENC_TRANS_CTL &= ~(TRANS_8x8_FLAG);
        pH264ENC->H264ENC_INTRA_PRED_MODE &= ~(INTRA_8x8_EN);
    }

    if (((pH264ENC->H264ENC_INTRA_PRED_MODE & INTRA_8x8_EN) == 0) && bIsIntra8x8) {
        AITPS_GBL pGBL = AITC_BASE_GBL;

        pGBL->GBL_SW_RST_EN[0] = GBL_RST_H264;
        pGBL->GBL_SW_RST_DIS[0] = GBL_RST_H264;
    }

    return MMP_ERR_NONE;
}

/** @brief Apply H/W OPR settings by profile/entropy modes

 @param[in] pEnc pointer to encoder info
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPF_H264ENC_RestoreProfile (MMPF_H264ENC_MODULE *pModule,
                                            MMP_USHORT          usProfile,
                                            VIDENC_ENTROPY      Entropy)
{
    AITPS_H264DEC_VLD       pH264VLD = AITC_BASE_H264DEC_VLD;
    AITPS_H264ENC           pH264ENC = AITC_BASE_H264ENC;

    if ((pModule->HwState.ProfileIdc == usProfile)
            && (pModule->HwState.EntropyMode == Entropy))
    {
        return MMP_ERR_NONE;
    }

    if (Entropy == H264ENC_ENTROPY_CABAC) {
        pH264VLD->HIGH_PROF_REG_1   |= H264_CABAC_MODE_FLAG;
        pH264VLD->HIGH_PROF_REG_2   = ((pH264VLD->HIGH_PROF_REG_2 & ~(H264_CABAC_INIT_IDC_MASK))
                                        | H264_CABAC_INIT_IDC(0));

        switch (usProfile) {
        case FREXT_HP:
            MMPF_H264ENC_RestoreTransform (MMP_TRUE, MMP_TRUE);
            pH264VLD->HIGH_PROF_REG_2   &= ~(H264_MAIN_PROFILE_FLAG);
            pH264VLD->HIGH_PROF_REG_6   |= H264_HIGH_PROFILE_FLAG;
            break;
        case MAIN_PROFILE:
            MMPF_H264ENC_RestoreTransform (MMP_FALSE, MMP_FALSE);
            pH264VLD->HIGH_PROF_REG_2   |= H264_MAIN_PROFILE_FLAG;
            pH264VLD->HIGH_PROF_REG_6   &= ~(H264_HIGH_PROFILE_FLAG);
            break;
        case BASELINE_PROFILE:
        default:
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
        }
    }
    else {
        pH264VLD->HIGH_PROF_REG_1 &= ~(H264_CABAC_MODE_FLAG);

        switch (usProfile) {
        case FREXT_HP:
            MMPF_H264ENC_RestoreTransform(MMP_TRUE, MMP_TRUE);
            pH264VLD->HIGH_PROF_REG_2   &= ~(H264_MAIN_PROFILE_FLAG);
            pH264VLD->HIGH_PROF_REG_6   |= H264_HIGH_PROFILE_FLAG;
            break;
        case MAIN_PROFILE:
            MMPF_H264ENC_RestoreTransform(MMP_FALSE, MMP_FALSE);
            pH264VLD->HIGH_PROF_REG_2   |= H264_MAIN_PROFILE_FLAG;
            pH264VLD->HIGH_PROF_REG_6   &= ~(H264_HIGH_PROFILE_FLAG);
            break;
        case BASELINE_PROFILE:
        default:
            MMPF_H264ENC_RestoreTransform(MMP_FALSE, MMP_FALSE);
            pH264VLD->HIGH_PROF_REG_2   &= ~H264_MAIN_PROFILE_FLAG;
            pH264VLD->HIGH_PROF_REG_6   &= ~(H264_HIGH_PROFILE_FLAG);
            break;
        }
    }

    pH264ENC->H264ENC_PROFILE_IDC = usProfile;
    if (usProfile == FREXT_HP) {
        pH264VLD->HIGH_PROF_REG_4 &= ~H264_SEQ_SCAL_LIST_PRESENT_FLAG;
        pH264VLD->HIGH_PROF_REG_5 &= ~H264_PIC_SCAL_LIST_PRESENT_FLAG;
        if (1) {
            pH264VLD->HIGH_PROF_REG_4 &= ~H264_SEQ_SCAL_MATRIX_PRESENT_FLAG;
        }
        else {
            pH264VLD->HIGH_PROF_REG_4 |= H264_SEQ_SCAL_MATRIX_PRESENT_FLAG;
        }
        if (1) {
            pH264VLD->HIGH_PROF_REG_5 &= ~H264_PIC_SCAL_MATRIX_PRESENT_FLAG;
        }
        else {
            pH264VLD->HIGH_PROF_REG_5 |= H264_PIC_SCAL_MATRIX_PRESENT_FLAG;
        }
        // qpc index offset
    }

    pModule->HwState.ProfileIdc  = usProfile;
    pModule->HwState.EntropyMode = Entropy;

    return MMP_ERR_NONE;
}

/** @brief Context switch encoder settings

 @param[in] pEnc pointer to encoder info
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPF_H264ENC_RestoreReg (MMPF_H264ENC_ENC_INFO *pEnc)
{
    AITPS_H264ENC       pH264ENC = AITC_BASE_H264ENC;
    AITPS_H264DEC_VLD   pH264VLD = AITC_BASE_H264DEC_VLD;
    AITPS_H264DEC_REF   pH264REF = AITC_BASE_H264DEC_REF;

    pH264VLD->PICTURE_SIZE_IN_MBS = pEnc->mb_num;
    pH264VLD->MB_WIDTH  = pEnc->mb_w;
    pH264VLD->MB_HEIGHT = pEnc->mb_h;
    pH264REF->Y_SIZE    = pEnc->mb_num << 8;

    pH264ENC->H264ENC_ME_INTER_COST_THRES   = pEnc->inter_cost_th;
    pH264ENC->H264ENC_INTRA_COST_BIAS       = pEnc->intra_cost_adj;
    pH264ENC->H264ENC_INTER_COST_BIAS       = pEnc->inter_cost_bias;

    pH264ENC->H264ENC_ME_STOP_THRES_LOWER_BOUND = pEnc->Memd.usMeStopThr[BD_LOW];
    pH264ENC->H264ENC_ME_STOP_THRES_UPPER_BOUND = pEnc->Memd.usMeStopThr[BD_HIGH];
    pH264ENC->H264ENC_ME_SKIP_THRES_LOWER_BOUND = pEnc->Memd.usMeSkipThr[BD_LOW];
    pH264ENC->H264ENC_ME_SKIP_THRES_UPPER_BOUND = pEnc->Memd.usMeSkipThr[BD_HIGH];

    pH264ENC->H264ENC_IME16X16_MAX_MINUS_1  = pEnc->me_itr_steps;
    pH264ENC->H264ENC_ME16X16_MAX_MINUS_1   = 0;

    // MV/Slice buff
    pH264ENC->H264ENC_MV_BUFF_START_ADDR    = pEnc->mv_addr;
    pH264ENC->H264ENC_SLICE_LEN_BUF_ADDR    = pEnc->slice_addr;

    //Motion vector range setting
    if (pEnc->mb_w > 128) //2048
    	pH264ENC->H264ENC_MV_RANGE_OPT = H264E_MV_RANGE_X_2046 | H264E_MV_RANGE_Y_14;
    else
    	pH264ENC->H264ENC_MV_RANGE_OPT = H264E_MV_RANGE_X_2046 | H264E_MV_RANGE_Y_46;

    if (pEnc->b_frame_num == 0) {
    	pH264VLD->HIGH_PROF_REG_2 = ((pH264VLD->HIGH_PROF_REG_2 & ~(H264_NUM_REF_FRAME_MASK)) | 0x00); // P frame only
    }
    else {
    	pH264VLD->HIGH_PROF_REG_2 = ((pH264VLD->HIGH_PROF_REG_2 & ~(H264_NUM_REF_FRAME_MASK)) | 0x01); // P frame only
    }

    // set ME setting
    pH264ENC->H264ENC_ME_REFINE_COUNT     = pEnc->mb_num;
    pH264ENC->H264ENC_ME_PART_LIMIT_COUNT = pEnc->mb_num;

    // for streams with different profile
    MMPF_H264ENC_RestoreProfile(pEnc->module, pEnc->profile, pEnc->entropy_mode);

    return MMP_ERR_NONE;
}

static void MMPF_H264ENC_UpdateOperation(void *EncHandle)
{
    MMP_ULONG   ulRdWrap = 0, ulRdIdx = 0, ulWrWrap = 0, ulWrIdx = 0, ulParamNum = 0, i = 0;
    MMP_ULONG   layer;
    MMP_ERR     ret;
    VIDENC_CTL  *ctl;
    MMPF_H264ENC_ENC_INFO *pEnc = EncHandle;

    ulRdWrap = pEnc->ulParamQueueRdWrapIdx;
    ulRdIdx = (ulRdWrap & 0xFF);
    ulRdWrap = ((ulRdWrap >> 8)) & 0xFFFFFF;
    ulWrWrap = pEnc->ulParamQueueWrWrapIdx;
    ulWrIdx = (ulWrWrap & 0xFF);
    ulWrWrap = ((ulWrWrap >> 8)) & 0xFFFFFF;

    ulParamNum = 0;
    if (ulWrWrap == ulRdWrap) {
        if (ulWrIdx < ulRdIdx) {
            DBG_S(0, "Video set param queue underflow 1\r\n");
            ulWrWrap = (ulRdWrap = 0);
            ulWrIdx = (ulRdIdx = 0);
        }
        else {
            ulParamNum = ulWrIdx - ulRdIdx;
        }
    }
    else if (ulWrWrap == (ulRdWrap+1)) {
        if (ulWrIdx > ulRdIdx) {
            DBG_S(0, "Video set param queue overflow\r\n");
            ulWrWrap = (ulRdWrap = 0);
            ulParamNum = ulWrIdx - ulRdIdx;
        }
        else {
            ulParamNum = MAX_NUM_PARAM_CTL - (ulRdIdx - ulWrIdx);
        }
    }
    else {
        DBG_S(0, "Video set param queue underflow 2\r\n");
        ulWrWrap = (ulRdWrap = 0);
        ulWrIdx = (ulRdIdx = 0);
    }

    if (ulParamNum == 0)
        return;

    for (i = 0; (i < ulParamNum) && (i < 2); i++) {
        ret = MMP_ERR_NONE;
        ctl = &pEnc->ParamQueue[ulRdIdx].Ctl;

        switch (pEnc->ParamQueue[ulRdIdx].Attrib) {

        case MMPF_VIDENC_ATTRIBUTE_RC_SKIPPABLE:
            MMPF_H264ENC_InitRCConfig(pEnc);
            break;

        case MMPF_VIDENC_ATTRIBUTE_RC_MODE:
            {
                MMPF_VIDENC_RC_MODE_CTL *pRcModeCtl = &(ctl->RcMode);

                if (pRcModeCtl->RcMode >= VIDENC_RC_MODE_MAX) {
                    ret = MMP_MP4VE_ERR_PARAMETER;
                    break;
                }

                #if (H264ENC_LBR_EN)
                if (pRcModeCtl->RcMode == VIDENC_RC_MODE_LOWBR) {
                    pEnc->total_layers = 2;
                    pEnc->layer_bitrate[0] = pEnc->stream_bitrate * 7 / 10;
                    pEnc->layer_bitrate[1] = pEnc->stream_bitrate * 3 / 10;

                    pEnc->Memd.usMeStopThr[BD_HIGH] = 4096;
                    pEnc->Memd.usMeSkipThr[BD_HIGH] = 4096;
                }
                else
                #endif
                {
                    pEnc->total_layers = 1;
                    pEnc->layer_bitrate[0] = pEnc->stream_bitrate;

                    pEnc->Memd.usMeStopThr[BD_HIGH] = 256;
                    pEnc->Memd.usMeSkipThr[BD_HIGH] = 512;
                }
                MMPF_H264ENC_InitRCConfig(pEnc);
            }
            break;

        case MMPF_VIDENC_ATTRIBUTE_LB_SIZE:
            {
                MMPF_VIDENC_LEAKYBUCKET_CTL *pLBCtl = &(ctl->CpbSize);
                MMP_ULONG64 ullBit64;

                #if (H264ENC_LBR_EN)
                if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                    pLBCtl->ubLayerBitMap = 3;
                    pLBCtl->ulLeakyBucket[1] = pLBCtl->ulLeakyBucket[0];
                }
                #endif

                for (layer = 0; layer < pEnc->total_layers; layer++) {
                    if ((pLBCtl->ubLayerBitMap & (1 << layer)) &&
                        pLBCtl->ulLeakyBucket[layer])
                    {
                        pEnc->layer_lb_size[layer] = pLBCtl->ulLeakyBucket[layer];
                        ullBit64 = (MMP_ULONG64)pEnc->layer_bitrate[layer] *
                                                pEnc->layer_lb_size[layer];

                        pEnc->rc_config[layer].VBV_Delay = (MMP_ULONG)(ullBit64 / 1000);
                        MMPF_VidRateCtl_ResetBufSize(pEnc->layer_rc_hdl[layer],
                                                    pEnc->rc_config[layer].VBV_Delay);
                    }
                }
            }
            break;

        case MMPF_VIDENC_ATTRIBUTE_BR:
            {
                MMPF_VIDENC_BITRATE_CTL *pRcCtl = &(ctl->Bitrate);

                pEnc->stream_bitrate = pRcCtl->ulBitrate[0];

                #if (H264ENC_LBR_EN)
                if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                    if(pEnc->bResetLowBrRatio){
                        pRcCtl->ulBitrate[0] = (pEnc->stream_bitrate * 7) / 10;
                        pRcCtl->ulBitrate[1] = (pEnc->stream_bitrate * 3) / 10;
                    }
                    else {
                        pRcCtl->ulBitrate[0] = (pEnc->stream_bitrate * 4) / 10;
                        pRcCtl->ulBitrate[1] = (pEnc->stream_bitrate * 6) / 10;
                    }
                }
                #endif

                for(layer = 0; layer < pEnc->total_layers; layer++) {
                    pEnc->layer_bitrate[layer] = pRcCtl->ulBitrate[layer];
                }
                MMPF_H264ENC_UpdateRateControl( pEnc,
                                                pEnc->ulSnrFpsRes,
                                                pEnc->ulSnrFpsInc,
                                                pEnc->ulEncFpsRes,
                                                pEnc->ulEncFpsInc);
            }
            break;

        case MMPF_VIDENC_ATTRIBUTE_ENC_FPS:
            {
                MMP_VID_FPS *Fps = &(ctl->Fps);
                ret = MMPF_H264ENC_SetInstanceParam(pEnc, MMPF_VIDENC_ATTRIBUTE_ENC_FPS, (MMP_ULONG)Fps);
            }
            break;

        case MMPF_VIDENC_ATTRIBUTE_GOP_CTL:
            {
                MMPF_VIDENC_GOP_CTL *pGopCtl = &(ctl->Gop);

                if (pGopCtl->usMaxContBFrameNum != pEnc->b_frame_num) {
                	RTNA_DBG_Str0("\r\n##Err:Not support dynamic increase b frame\r\n");
                    ret = MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
                }
                else {
                    ret = MMPF_H264ENC_SetInstanceParam(pEnc, MMPF_VIDENC_ATTRIBUTE_GOP_CTL, (MMP_ULONG)pGopCtl);
                    if (ret != MMP_ERR_NONE) {
                        break;
                    }
                }

                #if (H264ENC_LBR_EN)
                if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                    pEnc->layer_fps_ratio_low_br[0]  = 1;
                    pEnc->layer_fps_ratio_low_br[1]  = pEnc->gop_size - 1;
                    pEnc->ubAccResetLowBrRatioTimes  = 0;
                    pEnc->ubAccResetHighBrRatioTimes = 0;
                    pEnc->bResetLowBrRatio           = MMP_TRUE;
                    pEnc->bResetHighBrRatio          = MMP_FALSE;

                    MMPF_H264ENC_UpdateRateControl( pEnc,
                                                    pEnc->ulFpsInputRes,
                                                    pEnc->ulFpsInputInc,
                                                    pEnc->ulFpsOutputRes,
                                                    pEnc->ulFpsOutputInc);
                }
                #endif
            }
            break;

        case MMPF_VIDENC_ATTRIBUTE_FORCE_I:
            pEnc->gop_frame_num = 0;
            pEnc->total_frames = 0;
            pEnc->conus_i_frame_num = ctl->ConusI;
            break;

        default:
            break;
        }
        if (pEnc->ParamQueue[ulRdIdx].CallBack) {
            pEnc->ParamQueue[ulRdIdx].CallBack(ret);
            pEnc->ParamQueue[ulRdIdx].CallBack = NULL;
        }
        ulRdIdx++;
        if (ulRdIdx >= MAX_NUM_PARAM_CTL) {
            ulRdIdx -= MAX_NUM_PARAM_CTL;
            ulRdWrap++;
        }
        pEnc->ulParamQueueRdWrapIdx = ((ulRdWrap << 8) | (ulRdIdx & 0xFF));

        if (ret != MMP_ERR_NONE) {
            DBG_S(0, " Update op failed\r\n");
        }
    }

    return;
}

/**
 @brief Set video frame height padding

 @param[in] ubPaddingType padding type
 @param[in] ubPaddingCnt  padding count 0 disable padding, and needs to small than 16
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_SetPadding(MMPF_H264ENC_ENC_INFO *pEnc, MMPF_H264ENC_PADDING_TYPE type, MMP_UBYTE ubCnt)
{
    if (ubCnt > 15) {
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }
    
    if (type == MMPF_H264ENC_PADDING_NONE)
    	pEnc->paddinginfo.type = 0;
    else if (type == MMPF_H264ENC_PADDING_ZERO) {
    	pEnc->paddinginfo.type         = MMPF_H264ENC_PADDING_ZERO;
    	pEnc->paddinginfo.ubPaddingCnt = ubCnt;
    }
    else if (type == MMPF_H264ENC_PADDING_REPEAT) {
    	pEnc->paddinginfo.type         = MMPF_H264ENC_PADDING_REPEAT;
    	pEnc->paddinginfo.ubPaddingCnt = ubCnt;
    }
    else {
    	RTNA_DBG_Str(0, "H264 padding wrong type\r\n");
        pEnc->paddinginfo.type         = MMPF_H264ENC_PADDING_REPEAT;
        pEnc->paddinginfo.ubPaddingCnt = ubCnt;
    }

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetH264ByteCnt(MMP_USHORT usByteCnt)
{
	AITPS_H264ENC pH264Enc = AITC_BASE_H264ENC;
	
	if (usByteCnt == 128)
		pH264Enc->H264ENC_FRAME_CTL |= H264E_128B_CNT_MODE;
	else
		pH264Enc->H264ENC_FRAME_CTL &= ~(H264E_128B_CNT_MODE);

	return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_MODE ubMode,
                                    MMP_USHORT                      usPeriod,
                                    MMP_USHORT                      usOffset)
{
    AITPS_H264ENC pH264Enc = AITC_BASE_H264ENC;
    
    if (ubMode == MMPF_H264ENC_INTRA_REFRESH_DIS)
        pH264Enc->H264ENC_INTRA_REFRESH_MODE = MMPF_H264ENC_INTRA_REFRESH_DIS;

    pH264Enc->H264ENC_INTRA_REFRESH_MODE = ubMode;
    pH264Enc->H264ENC_INTRA_REFRESH_PERIOD = usPeriod;
    pH264Enc->H264ENC_INTRA_REFRESH_OFFSET = usOffset;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetBSBuf(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulLowBound,
                                MMP_ULONG               ulHighBound)
{
    pEnc->cur_frm_bs_addr    = ulLowBound;
    pEnc->cur_frm_bs_low_bd  = ulLowBound;
    pEnc->cur_frm_bs_high_bd = ulHighBound;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetMiscBuf(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulMVBuf,
                                MMP_ULONG               ulSliceLenBuf)
{
    pEnc->mv_addr    = ulMVBuf;
    pEnc->slice_addr = ulSliceLenBuf;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetRefListBound(MMPF_H264ENC_ENC_INFO  *pEnc,
                                    MMP_ULONG               ulLowBound,
                                    MMP_ULONG               ulHighBound)
{
    pEnc->RefGenBufLowBound  = ulLowBound;
    pEnc->RefGenBufHighBound = ulHighBound;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_SetParameter(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_VIDENC_ATTRIBUTE   attrib,
                                    void                    *arg)
{
    if (pEnc->GlbStatus.VidStatus == MMPF_MP4VENC_FW_STATUS_START)
        return MMPF_H264ENC_SetRuntimeParam(pEnc, attrib, (MMP_ULONG)arg);
    else
    	return MMPF_H264ENC_SetInstanceParam(pEnc, attrib, (MMP_ULONG)arg);
}

/**
 @brief Update encoder parameters, dynamic control
 @param[in]
 @param[in] attrib attributes of paramter
 @param[in] ulValue value paramter
 @retval MMP_ERR_NONE Success.
 @note SHOULD BE SET AFTER START RECORD
*/
static MMP_ERR MMPF_H264ENC_SetRuntimeParam(MMPF_H264ENC_ENC_INFO *pEnc,
                                            MMPF_VIDENC_ATTRIBUTE attrib,
                                            MMP_ULONG ulValue)
{
    MMP_ULONG   ulRdWrap, ulRdIdx, ulWrWrap, ulWrIdx = 0;
    MMP_ULONG   ulEntryNum, ubEncId, val;
    MMP_BOOL    bSendOp = MMP_FALSE;
    MMPF_VIDENC_PARAM_CTL param;
    #if (V4L2_VIDCTL_DBG)
    MMP_BOOL    dbgmsg = MMP_TRUE;
    #endif

    ubEncId = get_vidinst_id(pEnc->priv_data);

    switch (attrib) {
    #if (H264ENC_RDO_EN)
    case MMPF_VIDENC_ATTRIBUTE_RDO_EN:
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, ulValue);
            RTNA_DBG_Str(0, "+RDO\r\n");
        }
        #endif
    case MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P1:
    case MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P2:
        MMPF_H264ENC_SetInstanceParam(pEnc, attrib, ulValue);
        break;
    #endif

    #if (H264ENC_TNR_EN)
    case MMPF_VIDENC_ATTRIBUTE_TNR_EN:
        pEnc->tnr_en = (MMP_UBYTE)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->tnr_en);
            RTNA_DBG_Str(0, "+TNR\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_LOW_MV_THR:
        val = (MMP_USHORT)ulValue;
        pEnc->tnr_ctl.low_mv_x_thr = val;
        pEnc->tnr_ctl.low_mv_y_thr = val;
        break;    

    case MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_THR:
        val = (MMP_USHORT)ulValue;
        pEnc->tnr_ctl.zero_mv_x_thr = val;
        pEnc->tnr_ctl.zero_mv_y_thr = val;
        break;    

    case MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_LUMA_PXL_DIFF_THR:
        val = (MMP_UBYTE)ulValue;
        pEnc->tnr_ctl.zero_luma_pxl_diff_thr = val;
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_CHROMA_PXL_DIFF_THR:
        val = (MMP_UBYTE)ulValue;
        pEnc->tnr_ctl.zero_chroma_pxl_diff_thr = val;
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_4x4_CNT_THR:
        val = (MMP_UBYTE)ulValue;
        pEnc->tnr_ctl.zero_mv_4x4_cnt_thr = val;
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_LOW_MV_FILTER:
        val = ulValue;
        pEnc->tnr_ctl.low_mv_filter.luma_4x4   =  val & 0xff;
        pEnc->tnr_ctl.low_mv_filter.chroma_4x4 = (val >> 8) & 0xff;
        pEnc->tnr_ctl.low_mv_filter.thr_8x8    = (val >> 16) & 0xffff;
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_ZERO_MV_FILTER:
        val = ulValue;
        pEnc->tnr_ctl.zero_mv_filter.luma_4x4   =  val & 0xff;
        pEnc->tnr_ctl.zero_mv_filter.chroma_4x4 = (val >> 8) & 0xff;
        pEnc->tnr_ctl.zero_mv_filter.thr_8x8    = (val >> 16) & 0xffff;
        break;

    case MMPF_VIDENC_ATTRIBUTE_TNR_HIGH_MV_FILTER:
        val = ulValue;
        pEnc->tnr_ctl.high_mv_filter.luma_4x4   =  val & 0xff;
        pEnc->tnr_ctl.high_mv_filter.chroma_4x4 = (val >> 8) & 0xff;
        pEnc->tnr_ctl.high_mv_filter.thr_8x8    = (val >> 16) & 0xffff;
        break;
    #endif

    case MMPF_VIDENC_ATTRIBUTE_RC_SKIPPABLE:
        pEnc->rc_skippable = (MMP_BOOL)ulValue;
        pEnc->ParamQueue[ulWrIdx].Attrib    = attrib;
        pEnc->ParamQueue[ulWrIdx].CallBack  = NULL;
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->rc_skippable);
            RTNA_DBG_Str(0, "+rc_skip\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RC_SKIPTYPE:
        pEnc->rc_skip_smoothdown = (MMP_BOOL)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->rc_skip_smoothdown);
            RTNA_DBG_Str(0, "+rc_skiptype\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RC_MODE:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.RcMode    = *(MMPF_VIDENC_RC_MODE_CTL *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, param.Ctl.RcMode.RcMode);
            RTNA_DBG_Str(0, "+rc_mode\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_FRM_QP:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.Qp        = *(MMPF_VIDENC_QP_CTL *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, param.Ctl.Qp.ubQP[I_FRAME]);
            RTNA_DBG_Byte(0, param.Ctl.Qp.ubQP[P_FRAME]);
            RTNA_DBG_Str(0, "+initqp\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_FRM_QP_BOUND:
        MMPF_H264ENC_SetInstanceParam(pEnc, attrib, ulValue);
        break;

    case MMPF_VIDENC_ATTRIBUTE_LB_SIZE:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.CpbSize   = *(MMPF_VIDENC_LEAKYBUCKET_CTL *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Long(0, param.Ctl.CpbSize.ulLeakyBucket[0]);
            RTNA_DBG_Str(0, "+lbs\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_BR:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.Bitrate   = *(MMPF_VIDENC_BITRATE_CTL *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Long(0, param.Ctl.Bitrate.ulBitrate[0]);
            RTNA_DBG_Str(0, "+br\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_ENC_FPS:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.Fps       = *(MMP_VID_FPS *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, param.Ctl.Fps.ulResol / param.Ctl.Fps.ulIncr);
            RTNA_DBG_Str(0, "+enc_fps\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_GOP_CTL:
        if (!ulValue)
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

        param.Attrib        = attrib;
        param.CallBack      = NULL;
        param.Ctl.Gop       = *(MMPF_VIDENC_GOP_CTL *)(ulValue);
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, param.Ctl.Gop.usGopSize);
            RTNA_DBG_Str(0, "+gop\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_FORCE_I:
        param.Attrib        = attrib;
        param.CallBack      = NULL; //need input
        param.Ctl.ConusI    = ulValue;
        bSendOp = MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, param.Ctl.ConusI);
            RTNA_DBG_Str(0, "+forceI\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_ME_ITR_MAX_STEPS:
        if (ulValue < 16)
            pEnc->me_itr_steps = ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Short(0, pEnc->me_itr_steps);
            RTNA_DBG_Str(0, "+me_itr\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_MEMD_PARAM:
        if (ulValue) {
            pEnc->Memd = *(MMPF_H264ENC_MEMD_PARAM *)ulValue;
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Str(0, "+memd\r\n");
            }
            #endif
        }
        break;

    default:
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    if (bSendOp == MMP_FALSE)
        return MMP_ERR_NONE;

    /* Insert to param queue */
    ulRdWrap = pEnc->ulParamQueueRdWrapIdx;
    ulRdIdx = (ulRdWrap & 0xFF);
    ulRdWrap = ((ulRdWrap >> 8)) & 0xFFFFFF;
    ulWrWrap = pEnc->ulParamQueueWrWrapIdx;
    ulWrIdx = (ulWrWrap & 0xFF);
    ulWrWrap = ((ulWrWrap >> 8)) & 0xFFFFFF;

    #if 1 //prevent overflow
    ulEntryNum = 0;
    if (ulWrWrap == ulRdWrap) {
        if (ulWrIdx < ulRdIdx) {
            DBG_S(0, "VideoParQueue Err");
            DBG_S(0, " 0\r\n");
        }
        else {
            ulEntryNum = MAX_NUM_PARAM_CTL - (ulWrIdx - ulRdIdx);
        }
    }
    else if (ulWrWrap == (ulRdWrap+1)) {
        if (ulWrIdx > ulRdIdx) {
            DBG_S(0, "VideoParQueue Err");
            DBG_S(0, " 1\r\n");
        }
        else {
            ulEntryNum = (ulRdIdx - ulWrIdx);
        }
    }
    else {
        DBG_S(0, "VideoParQueue Err");
        DBG_S(0, " 2\r\n");
    }

    if (ulEntryNum == 0) {
        DBG_S(0, "VideoParQueue Full\r\n");
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }
    #endif

    pEnc->ParamQueue[ulWrIdx].Attrib    = param.Attrib;
    pEnc->ParamQueue[ulWrIdx].CallBack  = param.CallBack;
    pEnc->ParamQueue[ulWrIdx].Ctl       = param.Ctl;

    ulWrIdx++;
    if (ulWrIdx >= MAX_NUM_PARAM_CTL) {
        ulWrIdx -= MAX_NUM_PARAM_CTL;
        ulWrWrap++;
    }
    pEnc->ulParamQueueWrWrapIdx = ((ulWrWrap << 8) | (ulWrIdx & 0xFF));

    return MMP_ERR_NONE;
}

/**
 @brief Set encoder parameters

 This API should be called AFTER H264 initialized, and post BEFORE H264 start instance.
 @param[in]
 @param[in] attrib attributes of parameter
 @param[in] ulValue value parameter
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPF_H264ENC_SetInstanceParam(MMPF_H264ENC_ENC_INFO *pEnc,
                                             MMPF_VIDENC_ATTRIBUTE attrib,
                                             MMP_ULONG ulValue)
{
    #if (H264ENC_RDO_EN)
    MMP_ULONG   val, i, end;
    #endif
    MMP_ULONG   layer;
    #if (V4L2_VIDCTL_DBG)
    MMP_BOOL    dbgmsg = MMP_TRUE;
    #endif

    switch (attrib) {

    #if (H264ENC_RDO_EN)
    case MMPF_VIDENC_ATTRIBUTE_RDO_EN:
        pEnc->bRdoEnable = *(MMP_UBYTE *)ulValue;
        pEnc->bRcEnable  = pEnc->bRdoEnable ? MMP_FALSE : MMP_TRUE;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->bRdoEnable);
            RTNA_DBG_Str(0, "_RDO\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P1:
    case MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P2:
        val = *(MMP_ULONG *)ulValue;
        if (attrib == MMPF_VIDENC_ATTRIBUTE_RDO_QSTEP3_P1) {
            i   = 0;
            end = 6;
        }
        else {
            i   = 6;
            end = 10;
        }

        for(; i < end; i++) {
            pEnc->qstep3[i] = val & 0xf;
            val = val >> 4;
        }
        break;
    #endif

    #if (H264ENC_TNR_EN)
    case MMPF_VIDENC_ATTRIBUTE_TNR_EN:
        pEnc->tnr_en = ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->tnr_en);
            RTNA_DBG_Str(0, "_TNR\r\n");
        }
        #endif
        break;
    #endif

    case MMPF_VIDENC_ATTRIBUTE_PROFILE:
        pEnc->profile = ulValue & 0xFF;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->profile);
            RTNA_DBG_Str(0, "_profile\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_LEVEL:
        pEnc->level = ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->level);
            RTNA_DBG_Str(0, "_level\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_ENTROPY_MODE:
        pEnc->entropy_mode = (VIDENC_ENTROPY)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->entropy_mode);
            RTNA_DBG_Str(0, "_entropy\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RC_MODE:
        if (ulValue) {
            MMPF_VIDENC_RC_MODE_CTL *pRcModeCtl = (MMPF_VIDENC_RC_MODE_CTL *)ulValue;

            if (pRcModeCtl->RcMode >= VIDENC_RC_MODE_MAX)
                return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

            for(layer = 0; layer < MAX_NUM_TMP_LAYERS; layer++) {
                pEnc->rc_config[layer].rc_mode = pRcModeCtl->RcMode;
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pRcModeCtl->RcMode);
                RTNA_DBG_Str(0, "_rc_mode\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_RC_SKIPPABLE:
        pEnc->rc_skippable = (MMP_BOOL)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->rc_skippable);
            RTNA_DBG_Str(0, "_rc_skip\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RC_SKIPTYPE:
        pEnc->rc_skip_smoothdown = (MMP_BOOL)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->rc_skip_smoothdown);
            RTNA_DBG_Str(0, "_rc_skiptype\r\n");
        }
        #endif
        break; 

    case MMPF_VIDENC_ATTRIBUTE_FRM_QP:
        if (ulValue) {
            MMPF_VIDENC_QP_CTL *pQpCtl = (MMPF_VIDENC_QP_CTL *)ulValue;

            for(layer = 0; layer < MAX_NUM_TMP_LAYERS; layer++) {
                if (pQpCtl->ubTypeBitMap & (1 << I_FRAME)) {
                    pEnc->rc_config[layer].InitQP[I_FRAME] = pQpCtl->ubQP[I_FRAME];
                }
                if (pQpCtl->ubTypeBitMap & (1 << P_FRAME)) {
                    pEnc->rc_config[layer].InitQP[P_FRAME] = pQpCtl->ubQP[P_FRAME];
                }
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pEnc->rc_config[0].InitQP[I_FRAME]);
                RTNA_DBG_Byte(0, pEnc->rc_config[0].InitQP[P_FRAME]);
                RTNA_DBG_Str(0, "_initqp\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_FRM_QP_BOUND:
        if (ulValue) {
            MMP_ULONG layer_end;
            MMPF_VIDENC_QP_BOUND_CTL *pQpBound = (MMPF_VIDENC_QP_BOUND_CTL*)ulValue;

            if (pQpBound->ubLayerID == TEMPORAL_ID_MASK) {
                layer = 0;
                layer_end = MAX_NUM_TMP_LAYERS;
            }
            else {
                #if (H264ENC_LBR_EN)
                if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                    layer       = 0;
                    layer_end   = 2;
                }
                else
                #endif
                {
                    layer       = pQpBound->ubLayerID;
                    layer_end   = layer + 1;
                }
            }
            for (;layer < layer_end; layer++) {
                if (pQpBound->ubTypeBitMap & (1 << I_FRAME)) {
                    MMPF_H264ENC_SetQPBound(pEnc, layer, I_FRAME,
                                            pQpBound->ubQPBound[I_FRAME][BD_LOW],
                                            pQpBound->ubQPBound[I_FRAME][BD_HIGH]);
                }
                if (pQpBound->ubTypeBitMap & (1 << P_FRAME)) {
                    MMPF_H264ENC_SetQPBound(pEnc, layer, P_FRAME,
                                            pQpBound->ubQPBound[P_FRAME][BD_LOW],
                                            pQpBound->ubQPBound[P_FRAME][BD_HIGH]);
                }
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pQpBound->ubQPBound[I_FRAME][BD_LOW]);
                RTNA_DBG_Byte(0, pQpBound->ubQPBound[I_FRAME][BD_HIGH]);
                RTNA_DBG_Str(0, "_qp_bound\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_BR:
        if (ulValue) {
            MMPF_VIDENC_BITRATE_CTL *pRcCtl = (MMPF_VIDENC_BITRATE_CTL *)ulValue;

            pEnc->stream_bitrate = pRcCtl->ulBitrate[0];

            #if (H264ENC_LBR_EN)
            if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                MMP_ULONG Bitrate = pRcCtl->ulBitrate[0];

                pRcCtl->ulBitrate[0] = (Bitrate * 7) / 10;
                pRcCtl->ulBitrate[1] = (Bitrate * 3) / 10;
            }
            #endif

            for (layer = 0; layer < pEnc->total_layers; layer++) {
                pEnc->layer_bitrate[layer] = pRcCtl->ulBitrate[layer];
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Long(0, pEnc->stream_bitrate);
                RTNA_DBG_Str(0, "_br\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_LB_SIZE:
        if (ulValue) {
            MMPF_VIDENC_LEAKYBUCKET_CTL *pLBCtl = (MMPF_VIDENC_LEAKYBUCKET_CTL*)ulValue;
            MMP_UBYTE layer = 0;

            for (layer = 0; layer < MAX_NUM_TMP_LAYERS; layer++) {
                if ((pLBCtl->ubLayerBitMap & (1 << layer)) &&
                    pLBCtl->ulLeakyBucket[layer])
                {
                    pEnc->layer_lb_size[layer] = pLBCtl->ulLeakyBucket[layer];
                }
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Long(0, pEnc->layer_lb_size[0]);
                RTNA_DBG_Str(0, "_lbs\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_CROPPING:
        pEnc->crop = *(MMPF_VIDENC_CROPPING *)ulValue;
        #if (V4L2_VIDCTL_DBG)
        if (dbgmsg) {
            RTNA_DBG_Byte(0, pEnc->crop.usTop);
            RTNA_DBG_Byte(0, pEnc->crop.usBottom);
            RTNA_DBG_Byte(0, pEnc->crop.usLeft);
            RTNA_DBG_Byte(0, pEnc->crop.usRight);
            RTNA_DBG_Str(0, "_crop\r\n");
        }
        #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_GOP_CTL:
        if (ulValue) {
            MMPF_VIDENC_GOP_CTL *pGopCtl = (MMPF_VIDENC_GOP_CTL *)ulValue;

            pEnc->gop_size      = pGopCtl->usGopSize;
            pEnc->b_frame_num   = pGopCtl->usMaxContBFrameNum;
            if (pEnc->gop_size && (pEnc->gop_frame_num >= pEnc->gop_size)) {
                pEnc->gop_frame_num = pEnc->gop_frame_num % pEnc->gop_size;
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pEnc->gop_size);
                RTNA_DBG_Str(0, "_gop\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_ENC_FPS:
        if (ulValue) {
        	MMP_VID_FPS *pFpsCtl = (MMP_VID_FPS*)ulValue;
            if (pFpsCtl->ulResol && pFpsCtl->ulIncr) {
                pEnc->ulEncFpsRes      = pFpsCtl->ulResol;
                pEnc->ulEncFpsInc      = pFpsCtl->ulIncr;
                pEnc->ulEncFps1000xInc = pFpsCtl->ulIncrx1000;
            }
            else {
                return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pEnc->ulEncFpsRes / pEnc->ulEncFpsInc);
                RTNA_DBG_Str(0, "_enc_fps\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_SNR_FPS:
        if (ulValue) {
        	MMP_VID_FPS *pFpsCtl = (MMP_VID_FPS*)ulValue;
            if (pFpsCtl->ulResol && pFpsCtl->ulIncr) {
                pEnc->ulSnrFpsRes      = pFpsCtl->ulResol;
                pEnc->ulSnrFpsInc      = pFpsCtl->ulIncr;
                pEnc->ulSnrFps1000xInc = pFpsCtl->ulIncrx1000;
            }
            else {
                return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
            }
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pEnc->ulSnrFpsRes / pEnc->ulSnrFpsInc);
                RTNA_DBG_Str(0, "_snr_fps\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_CURBUF_MODE:
            pEnc->CurBufMode = ulValue & 0xFF;
            if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT)
            	MMPF_IBC_SetH264RT_Enable(MMP_TRUE);
            else if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_FRAME)
            	MMPF_IBC_SetH264RT_Enable(MMP_FALSE);
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Byte(0, pEnc->CurBufMode);
                RTNA_DBG_Str(0, "_curbuf\r\n");
            }
            #endif
        break;

    case MMPF_VIDENC_ATTRIBUTE_RESOLUTION:
    	if (ulValue) {
    		MMPF_VIDENC_RESOLUTION *pResol = (MMPF_VIDENC_RESOLUTION *)ulValue;
			if ((pResol->usWidth & 0x0F) || (pResol->usHeight & 0x0F)) {
				RTNA_DBG_Str(0, "Error : width/height not 16 alignment !\r\n");
				return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
			}
            pEnc->mb_w = (pResol->usWidth >> 4);
            pEnc->mb_h = (pResol->usHeight >> 4);
            pEnc->mb_num = pEnc->mb_w * pEnc->mb_h;
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Short(0, pResol->usWidth);
                RTNA_DBG_Short(0, pResol->usHeight);
                RTNA_DBG_Str(0, "_resol\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_ME_ITR_MAX_STEPS:
        if (ulValue < 16) {
            pEnc->me_itr_steps = ulValue;
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Short(0, pEnc->me_itr_steps);
                RTNA_DBG_Str(0, "_me_itr\r\n");
            }
            #endif
        }
        break;

    case MMPF_VIDENC_ATTRIBUTE_MEMD_PARAM:
        if (ulValue) {
            pEnc->Memd = *(MMPF_H264ENC_MEMD_PARAM *)ulValue;
            #if (V4L2_VIDCTL_DBG)
            if (dbgmsg) {
                RTNA_DBG_Str(0, "_memd\r\n");
            }
            #endif
        }
        break;

    default:
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_GetParameter(MMPF_H264ENC_ENC_INFO *pEnc, MMPF_VIDENC_ATTRIBUTE attrib, void *ulValue)
{
    switch (attrib) {
    case MMPF_VIDENC_ATTRIBUTE_ENC_FPS:
        if (ulValue) {
        	MMP_VID_FPS *pFpsCtl = (MMP_VID_FPS *)ulValue;

            pFpsCtl->ulResol     = pEnc->ulEncFpsRes;
            pFpsCtl->ulIncr      = pEnc->ulEncFpsInc;
            pFpsCtl->ulIncrx1000 = pEnc->ulEncFps1000xInc;
        }
        break;
    case MMPF_VIDENC_ATTRIBUTE_GOP_CTL:
        if (ulValue) {
            MMPF_VIDENC_GOP_CTL *pGopCtl = (MMPF_VIDENC_GOP_CTL *)ulValue;

            pGopCtl->usGopSize          = pEnc->gop_size;
            pGopCtl->usMaxContBFrameNum = pEnc->b_frame_num;
        }
        break;
    case MMPF_VIDENC_ATTRIBUTE_CROPPING:
        if (ulValue) {
            *(MMPF_VIDENC_CROPPING*)ulValue = pEnc->crop;
        }
        break;
    case MMPF_VIDENC_ATTRIBUTE_RESOLUTION:
		if (ulValue) {
			MMPF_VIDENC_RESOLUTION *pResol = (MMPF_VIDENC_RESOLUTION *)ulValue;
			pResol->usWidth = (pEnc->mb_w << 4);
			pResol->usHeight = (pEnc->mb_h << 4);
		}
		break;
    default:
        //
        break;
    }

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_InitRCConfig (MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ERR             	ret;
    MMP_ULONG64             bitrate64;
    MMP_ULONG               layer, total_layer, fps;
    MMP_ULONG               fps_ratio_sum, vbv_min_size, nP;
    MMP_ULONG               InstanceId;
    MMPF_3GPMGR_FRAME_TYPE  frame_type;

    InstanceId = get_vidinst_id(pEnc->priv_data);

    MMPF_H264ENC_UpdateModeDecision(pEnc);

    if ((pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_CBR) ||
        #if (H264ENC_LBR_EN)
        (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) ||
        #endif
        (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_VBR)) {

        if ((pEnc->mb_w & 7) == 0) { // mod 8
            pEnc->qp_tune_mode  = QP_FINETUNE_MB_MODE_EN;
            pEnc->qp_tune_size  = (pEnc->mb_w >> 3);
        }
        else if ((pEnc->mb_w & 3) == 0) { // mod 4
            pEnc->qp_tune_mode  = QP_FINETUNE_MB_MODE_EN;
            pEnc->qp_tune_size  = (pEnc->mb_w >> 2);
        }
        else if ((pEnc->mb_w & 1) == 0) { // mod 2
            pEnc->qp_tune_mode  = QP_FINETUNE_MB_MODE_EN;
            pEnc->qp_tune_size  = (pEnc->mb_w >> 1);
        }
        else if ((pEnc->mb_w % 3) == 0) {
            pEnc->qp_tune_mode  = QP_FINETUNE_MB_MODE_EN;
            pEnc->qp_tune_size  = pEnc->mb_w / 3;
        }
        else if ((pEnc->mb_w % 5) == 0) {
            pEnc->qp_tune_mode  = QP_FINETUNE_MB_MODE_EN;
            pEnc->qp_tune_size  = pEnc->mb_w / 5;
        }
        else {
            pEnc->qp_tune_mode  = QP_FINETUNE_ROW_MODE_EN;
            pEnc->qp_tune_size  = 1;
        }
    }

    #if (H264ENC_LBR_EN)
    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
        total_layer     = 2;
        fps_ratio_sum   = pEnc->gop_size;
    }
    else
    #endif
    {
        total_layer     = 1;
        fps_ratio_sum   = 1;
    }
    fps = (pEnc->ulEncFpsRes + pEnc->ulEncFpsInc - 1) / pEnc->ulEncFpsInc;

    pEnc->stream_bitrate = 0;
    for (layer = 0; layer < total_layer; layer++) {

        pEnc->stream_bitrate += pEnc->layer_bitrate[layer];
        bitrate64 = ((MMP_ULONG64)(pEnc->layer_bitrate[layer]) * 
                                   pEnc->ulFpsOutputInc * fps_ratio_sum) / 
                                  (pEnc->ulFpsOutputRes << 3);
        #if (H264ENC_LBR_EN)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR)
            pEnc->layer_tgt_frm_size[layer] = (MMP_ULONG)(bitrate64 /
                                    pEnc->layer_fps_ratio_low_br[layer]);
        else
        #endif
            pEnc->layer_tgt_frm_size[layer] = (MMP_ULONG)(bitrate64 /
                                        pEnc->layer_fps_ratio[layer]);

        pEnc->layer_frm_thr[layer] = pEnc->layer_tgt_frm_size[layer] * 3;

        vbv_min_size = pEnc->layer_tgt_frm_size[layer] * RC_MIN_VBV_FRM_NUM << 3;
        bitrate64 = (MMP_ULONG64)pEnc->layer_bitrate[layer] *
                                 pEnc->layer_lb_size[layer];
        pEnc->rc_config[layer].VBV_Delay = (MMP_ULONG)(bitrate64 / 1000);
        if (pEnc->rc_config[layer].VBV_Delay < vbv_min_size)
            pEnc->rc_config[layer].VBV_Delay = vbv_min_size;

        switch (pEnc->rc_config[0].rc_mode) {
        case VIDENC_RC_MODE_CBR:
        #if (H264ENC_LBR_EN)
        case VIDENC_RC_MODE_LOWBR:
        #endif
            pEnc->rc_config[layer].TargetVBVLevel       = 350;
            #if (H264ENC_LBR_EN)
            if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
                pEnc->rc_config[layer].bPreSkipFrame    =  (layer == 0) ?
                                                            MMP_FALSE :
                                                            pEnc->rc_skippable;
            }
            else
            #endif
            {
                pEnc->rc_config[layer].bPreSkipFrame    =  pEnc->rc_skippable;
            }
    	    pEnc->rc_config[layer].MaxQPDelta[I_FRAME]  = 2;
    	    pEnc->rc_config[layer].MaxQPDelta[P_FRAME]  = 2;
    	    pEnc->rc_config[layer].MaxQPDelta[B_FRAME]  = 2;
            break;
        case VIDENC_RC_MODE_VBR:
            pEnc->rc_config[layer].VBV_Delay           *= 8;
            pEnc->rc_config[layer].TargetVBVLevel       = 400;
            pEnc->rc_config[layer].bPreSkipFrame        = MMP_FALSE;
    	    pEnc->rc_config[layer].MaxQPDelta[I_FRAME]  = 3;
    	    pEnc->rc_config[layer].MaxQPDelta[P_FRAME]  = 3;
    	    pEnc->rc_config[layer].MaxQPDelta[B_FRAME]  = 3;
            break;
        case VIDENC_RC_MODE_CQP:
            pEnc->rc_config[layer].TargetVBVLevel       = 350;
    		pEnc->rc_config[layer].bPreSkipFrame        = MMP_FALSE;
    	    pEnc->rc_config[layer].MaxQPDelta[I_FRAME]  = 0;
    	    pEnc->rc_config[layer].MaxQPDelta[P_FRAME]  = 0;
    	    pEnc->rc_config[layer].MaxQPDelta[B_FRAME]  = 0;
            break;
        default:
            RTNA_DBG_Str0("Error RC mode\r\n");
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
        }

        pEnc->rc_config[layer].EncID        = InstanceId;
        pEnc->rc_config[layer].Layer        = layer;
        pEnc->rc_config[layer].LayerRelated = 0;
        pEnc->rc_config[layer].MBWidth      = pEnc->mb_w;
        pEnc->rc_config[layer].MBHeight     = pEnc->mb_h;

        #if (H264ENC_LBR_EN)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
            if (layer == 0)
                nP = 0;
            else
                nP = RC_PSEUDO_GOP_SIZE - 1;
        }
        else
        #endif
        {
            nP = pEnc->gop_size - 1;
        }

        ret = MMPF_VidRateCtl_Init (&(pEnc->layer_rc_hdl[layer]),
                                    (InstanceId * MAX_NUM_TMP_LAYERS) + layer,
                                    MMPF_MP4VENC_FORMAT_H264,
                                    pEnc->layer_tgt_frm_size[layer],
                                    pEnc->layer_bitrate[layer],
                                    nP,
                                    0,
                                    MMP_FALSE,
                            		&pEnc->rc_config[layer],
                            		fps);
        if (ret != MMP_ERR_NONE) {
            RTNA_DBG_Str0("RC_Init err!\r\n");
            return ret;
        }

        for (frame_type = MMPF_3GPMGR_FRAME_TYPE_I;
                frame_type < MMPF_3GPMGR_FRAME_TYPE_MAX; frame_type++) {
        	MMPF_H264ENC_SetQPBound(pEnc, layer, frame_type,
        			                pEnc->MbQpBound[layer][frame_type][BD_LOW],
    				                pEnc->MbQpBound[layer][frame_type][BD_HIGH]);

        	pEnc->CurRcQP[layer][frame_type] = pEnc->rc_config[layer].InitQP[frame_type];
        }
    }

    return MMP_ERR_NONE;
}

/**
 @brief Set QP bound
 @param[in] ubEncId indicates which encoder to be set
 @param[in] type frame type to be set
 @param[in] lMinQp low bound for QP
 @param[in] lMaxQp high bound for QP
 @retval MMP_ERR_NONE Success.
 @note CAN BE CALLED 'BEFORE' or 'AFTER' START PREVIEW/RECORD
*/
MMP_ERR MMPF_H264ENC_SetQPBound(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG               ulLayer,
                                MMPF_3GPMGR_FRAME_TYPE  type,
                                MMP_LONG                lMinQp,
                                MMP_LONG                lMaxQp)
{
    MMP_LONG min_basic_qp, max_basic_qp;

    /* parameters check */
    if (lMinQp > lMaxQp)
        lMinQp = lMaxQp;
    if (lMinQp < H264E_MIN_MB_QP)
        lMinQp = H264E_MIN_MB_QP;
    if (lMinQp > H264E_MAX_MB_QP)
        lMinQp = H264E_MAX_MB_QP;

    pEnc->MbQpBound[ulLayer][type][BD_LOW]  = lMinQp;
    pEnc->MbQpBound[ulLayer][type][BD_HIGH] = lMaxQp;

    if (pEnc->layer_rc_hdl[ulLayer]) {
        RC_CONFIG_PARAM *pRcCfg;
        void            *pRc;
        MMP_UBYTE       vop_t;

        pRc     = pEnc->layer_rc_hdl[ulLayer];
        pRcCfg  = &(pEnc->rc_config[ulLayer]);

        switch (type) {
        case MMPF_3GPMGR_FRAME_TYPE_I:
            vop_t = I_FRAME;
            break;
        case MMPF_3GPMGR_FRAME_TYPE_P:
            vop_t = P_FRAME;
            break;
        case MMPF_3GPMGR_FRAME_TYPE_B:
            vop_t = B_FRAME;
            break;
        default:
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
        }

        min_basic_qp = lMinQp + pRcCfg->MaxQPDelta[vop_t];
        max_basic_qp = lMaxQp - pRcCfg->MaxQPDelta[vop_t];

        if (min_basic_qp > max_basic_qp) {
            return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
        }

        MMPF_VidRateCtl_SetQPBoundary(pRc, vop_t, min_basic_qp, max_basic_qp);
    }

    return MMP_ERR_NONE;
}

/** @brief Update reference frame list

 @param[in] EncHandle pointer to encoder info
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_UpdateModeDecision(MMPF_H264ENC_ENC_INFO *pEnc)
{
    #define INSERT_IDR_GAP  (10)

    #if (H264ENC_LBR_EN)
    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
        if (pEnc->decision_mode) { 
            pEnc->inter_cost_th     = 512;
            pEnc->intra_cost_adj    = 16;
            pEnc->inter_cost_bias   = (MMP_USHORT)-512;
            if (pEnc->gop_size >= 50) {
                #if 0
                // prevent too close between regular i and insert i
                if ((pEnc->gop_frame_num >= INSERT_IDR_GAP) &&
                    (pEnc->gop_frame_num < (pEnc->gop_size - INSERT_IDR_GAP)))
                {
                    pEnc->gop_frame_num = 0;
                    pEnc->total_frames  = 0;
                }
                #endif
            }
            pEnc->tnr_en |= (TNR_LOW_MV_EN | TNR_HIGH_MV_EN);
        }
        else  {
            /*
            1. inter_cost_th = 0 
            run (((intra_cost * intra_cost_adj) >> 4) < (min_cost+inter_cost_bias))
            2. inter_cost_th = 512
            run ((min_cost < inter_cost_skip)&&(intra_cost < intra_cost_thr) &&(intra_cost < min_cost ))
            */
            //dbg_printf(0,"--want skip_mb\n");
            pEnc->inter_cost_th     = 512;
            pEnc->intra_cost_adj    = 16;
            pEnc->inter_cost_bias   = (MMP_USHORT)-512;
            pEnc->tnr_en &= ~(TNR_LOW_MV_EN | TNR_HIGH_MV_EN);
        }
    }
    else
    #endif
    {
        pEnc->inter_cost_th     = 512;
        pEnc->intra_cost_adj    = 16;
        pEnc->inter_cost_bias   = (MMP_USHORT)-512;
    }

    return MMP_ERR_NONE;
}

/**
 @brief reset rate control bitrate, input/output framerate
 @param[in]
 @retval MMP_ERR_NONE Success.
 @note SHOULD BE SET 'AFTER' START RECORD
*/
static MMP_ERR MMPF_H264ENC_UpdateRateControl(MMPF_H264ENC_ENC_INFO *pEnc,
                                              MMP_ULONG             ulFpsInRes,
                                              MMP_ULONG             ulFpsInInc,
                                              MMP_ULONG             ulFpsOutRes,
                                              MMP_ULONG             ulFpsOutInc)
{
    MMP_ULONG64 bitrate64;
    MMP_ULONG   layer, total_layer;
    MMP_ULONG   fps_ratio_sum, vbv_min_size;

    if (pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_START) {
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

    if ((ulFpsInRes | ulFpsInInc | ulFpsOutRes | ulFpsOutInc) == 0) {
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    if ((ulFpsOutRes*ulFpsInInc) > (ulFpsInRes*ulFpsOutInc)) {
        RTNA_DBG_Str(0, "Error: enc frame rate too large\r\n");
        ulFpsOutRes = (ulFpsInRes*ulFpsOutInc)/ulFpsInInc;
    }

    if ((pEnc->ulFpsInputRes != ulFpsInRes) ||
        (pEnc->ulFpsInputInc != ulFpsInInc) ||
        (pEnc->ulFpsOutputRes != ulFpsOutRes) ||
        (pEnc->ulFpsOutputInc != ulFpsOutInc))
    {
        pEnc->ulFpsInputRes = ulFpsInRes;
        pEnc->ulFpsInputInc = ulFpsInInc;
        pEnc->ulFpsInputAcc = 0;
        pEnc->ulFpsOutputRes = ulFpsOutRes;
        pEnc->ulFpsOutputInc = ulFpsOutInc;
        pEnc->ulFpsOutputAcc = 0;
    }

    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_CQP) {
        return MMP_ERR_NONE;
    }

    #if (H264ENC_LBR_EN)
    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
        total_layer     = 2;
        fps_ratio_sum   = pEnc->gop_size;
    }
    else
    #endif
    {
        total_layer     = 1;
        fps_ratio_sum   = 1;
    }

    pEnc->stream_bitrate = 0;
    for (layer = 0; layer < total_layer; layer++) {
        // calculates each layers target frame size
        bitrate64 = ((MMP_ULONG64)(pEnc->layer_bitrate[layer]) * 
                                   pEnc->ulFpsOutputInc * fps_ratio_sum) / 
                                  (pEnc->ulFpsOutputRes << 3);
        #if (H264ENC_LBR_EN)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR)
            pEnc->layer_tgt_frm_size[layer] = (MMP_ULONG)(bitrate64 /
                                    pEnc->layer_fps_ratio_low_br[layer]);
        else
        #endif
            pEnc->layer_tgt_frm_size[layer] = (MMP_ULONG)(bitrate64 /
                                        pEnc->layer_fps_ratio[layer]);

        if (pEnc->layer_tgt_frm_size[layer] == 0)
            return MMP_MP4VE_ERR_PARAMETER;

        pEnc->layer_frm_thr[layer] = pEnc->layer_tgt_frm_size[layer] * 3;

        MMPF_VidRateCtl_ResetBitrate(pEnc->layer_rc_hdl[layer],
                                    pEnc->layer_bitrate[layer],
                                    pEnc->layer_tgt_frm_size[layer],
                                    MMP_TRUE,
                                    0,
                                    MMP_TRUE);

        bitrate64 = (MMP_ULONG64)pEnc->layer_bitrate[layer] *
                                 pEnc->layer_lb_size[layer];

        #if (H264ENC_LBR_EN)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
            vbv_min_size = (MMP_ULONG)(((MMP_ULONG64)pEnc->layer_bitrate[layer] *
                                        RC_MIN_VBV_FRM_NUM  *
                                        fps_ratio_sum       *
                                        ulFpsOutInc)        /
                                       (ulFpsOutRes         *
                                        pEnc->layer_fps_ratio_low_br[layer]));
        }
        else
        #endif
        {
            vbv_min_size = (MMP_ULONG)(((MMP_ULONG64)pEnc->layer_bitrate[layer] *
                                        RC_MIN_VBV_FRM_NUM  *
                                        fps_ratio_sum       *
                                        ulFpsOutInc)        /
                                       (ulFpsOutRes         *
                                        pEnc->layer_fps_ratio[layer]));
        }

        pEnc->rc_config[layer].VBV_Delay = (MMP_ULONG)(bitrate64 / 1000);
        if (pEnc->rc_config[layer].VBV_Delay < vbv_min_size) {
            pEnc->rc_config[layer].VBV_Delay = vbv_min_size;
            MMPF_VidRateCtl_ResetBufSize(pEnc->layer_rc_hdl[layer],
                                        pEnc->rc_config[layer].VBV_Delay);
        }

        pEnc->stream_bitrate += pEnc->layer_bitrate[layer];
    }

    MMPF_H264ENC_UpdateModeDecision(pEnc);

    return MMP_ERR_NONE;
}

void instance_parameter_config(MMPF_H264ENC_ENC_INFO *enc)
{
    MMP_ULONG layer;

    enc->profile    = BASELINE_PROFILE;
    enc->level      = 40;

    enc->CurBufMode = MMPF_MP4VENC_CURBUF_FRAME;
    enc->CurRTSrcPipeId     = 0;

    enc->ulEncFpsRes        = 30 * 1000;
    enc->ulEncFpsInc        = 1000;
    enc->ulEncFps1000xInc   = 1000 * 1000;
    enc->ulSnrFpsRes        = 30 * 1000;
    enc->ulSnrFpsInc        = 1000;
    enc->ulSnrFps1000xInc   = 1000 * 1000;

    enc->gop_size           = 300;
    enc->b_frame_num        = 0;

    enc->paddinginfo.type   = MMP_FALSE;
    enc->paddinginfo.ubPaddingCnt = 0;
    #if (H264ENC_RDO_EN)
    enc->bRdoEnable         = MMP_FALSE;
    enc->bRdoMadValid       = MMP_FALSE;
    enc->ulRdoMadFrmPre     = 0;
    enc->ulRdoInitFac       = 0;
    enc->ulRdoMadLastRowPre = 0;
    enc->ubRdoMissThrLoAdj  = 12;
    enc->ubRdoMissThrHiAdj  = 24;
    enc->ubRdoMissTuneQp    = 0x03;
    #endif

    enc->bRcEnable          = MMP_TRUE;

    enc->rc_skippable       = MMP_FALSE;
    enc->rc_skip_smoothdown = MMP_FALSE;
    enc->rc_skip_bypass     = MMP_FALSE;

    enc->qp_tune_mode   = QP_FINETUNE_DIS;
    enc->qp_tune_size   = 0;

    enc->total_layers   = 1;
    enc->stream_bitrate = 0;
    enc->target_bitrate = 0;//for ramp-up bitrate target
    enc->rampup_bitrate = 0;
    for (layer = 0; layer < MAX_NUM_TMP_LAYERS; layer++) {
        enc->layer_rc_hdl[layer]    = NULL;

        enc->layer_bitrate[layer]   = 1000000;
        enc->stream_bitrate        += enc->layer_bitrate[layer];
        enc->layer_lb_size[layer]   = 1000; //ms

        enc->MbQpBound[layer][0][BD_LOW]    = H264E_MIN_MB_QP;
        enc->MbQpBound[layer][0][BD_HIGH]   = H264E_MAX_MB_QP;
        enc->MbQpBound[layer][1][BD_LOW]    = H264E_MIN_MB_QP;
        enc->MbQpBound[layer][1][BD_HIGH]   = H264E_MAX_MB_QP;
        enc->MbQpBound[layer][2][BD_LOW]    = H264E_MIN_MB_QP;
        enc->MbQpBound[layer][2][BD_HIGH]   = H264E_MAX_MB_QP;
        // set RC default parameter ------------------------------
        enc->rc_config[layer].MaxIWeight    = gRcConfig.MaxIWeight;
        enc->rc_config[layer].MinIWeight    = gRcConfig.MinIWeight;
        enc->rc_config[layer].MaxBWeight    = gRcConfig.MaxBWeight;
        enc->rc_config[layer].MinBWeight    = gRcConfig.MinBWeight;
        enc->rc_config[layer].InitWeight[0] = gRcConfig.InitWeight[0];
        enc->rc_config[layer].InitWeight[1] = gRcConfig.InitWeight[1];
        enc->rc_config[layer].InitWeight[2] = gRcConfig.InitWeight[2];
        enc->rc_config[layer].SkipFrameThreshold = gRcConfig.SkipFrameThreshold;

        enc->rc_config[layer].rc_mode       = VIDENC_RC_MODE_VBR;
        enc->rc_config[layer].InitQP[0]     = gRcConfig.InitQP[0];
        enc->rc_config[layer].InitQP[1]     = gRcConfig.InitQP[1];
        enc->rc_config[layer].InitQP[2]     = gRcConfig.InitQP[2];
        enc->rc_config[layer].MaxQPDelta[0] = gRcConfig.MaxQPDelta[0];
        enc->rc_config[layer].MaxQPDelta[1] = gRcConfig.MaxQPDelta[1];
        enc->rc_config[layer].MaxQPDelta[2] = gRcConfig.MaxQPDelta[2];
        // end set RC default parameter --------------------------
    }

//    MEMSET((void*)&(enc->RefListInfo), 0, sizeof(MMPF_H264ENC_REFLIST_INFO));
    MEMSET((void*)&(enc->crop), 0, sizeof(MMPF_VIDENC_CROPPING));
//    MEMSET((void*)&(enc->pps), 0, sizeof(MMPF_H264ENC_PPS_INFO));

    enc->entropy_mode = H264ENC_ENTROPY_CAVLC;

    // Mode Decision
    // (min_cost > inter_cost_thr) &&
    // (((intra_cost * intra_cost_adj) >> 4) < (min_cost+inter_cost_bias))
    // TRUE    : intra
    // FALSE   : inter
    enc->inter_cost_th      = 512;
    enc->intra_cost_adj     = 16;
    enc->inter_cost_bias    = (MMP_USHORT) -512;

    enc->me_itr_steps       = 0;

    enc->Memd.usMeStopThr[BD_LOW]   = 128;
    enc->Memd.usMeStopThr[BD_HIGH]  = 256;
    enc->Memd.usMeSkipThr[BD_LOW]   = 256;
    enc->Memd.usMeSkipThr[BD_HIGH]  = 512;

    enc->temporal_id        = 0;
    enc->video_full_range   = MMP_TRUE;
    enc->cur_frm_bs_addr    = 0;
    enc->cur_frm_bs_low_bd  = 0;
    enc->cur_frm_bs_high_bd = 0x80000000;

    enc->EncReStartCallback = rt_enc_restart_callback;
    enc->EncEndCallback     = rt_enc_end_init_callback;

    enc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_RECORD;
    enc->GlbStatus.VidOp     = VIDEO_RECORD_NONE;
    enc->GlbStatus.VideoCaptureStart = 0;
    enc->module             = NULL;
    enc->priv_data          = NULL;
    enc->Container          = NULL;
    gbRtEncDropFrameEn = MMP_FALSE;

    #if (H264ENC_TNR_EN)
    enc->tnr_ctl            = m_TnrDefCfg;
    #endif
    
    #if H264ENC_GDR_EN
    enc->intra_refresh_en       = MMP_TRUE  ;
    enc->intra_refresh_trigger  = MMP_FALSE ;
    enc->intra_refresh_period   = 5 ;
    enc->intra_refresh_offset   = 0 ;
    #endif

}

MMP_ERR MMPF_H264ENC_InitModule(MMPF_H264ENC_MODULE *pModule)
{
    AITPS_H264ENC       pH264ENC  = AITC_BASE_H264ENC;
	AITPS_H264DEC_CTL   pH264DEC  = AITC_BASE_H264DEC_CTL;
    AITPS_H264DEC_VLD   pH264VLD  = AITC_BASE_H264DEC_VLD;
    MMP_UBYTE           rdo_step_3[] = {6, 6, 4, 4, 3, 3, 12, 2, 2, 2};

    pModule->bWorking = MMP_FALSE;
    pModule->pH264Inst = NULL;
    pModule->HwState.ProfileIdc = 0xFF;
    pModule->HwState.EntropyMode = H264ENC_ENTROPY_NONE;

    //_SetGlobalConfig()...
	{
	    // Motion Estimation Setting
    	pH264ENC->H264ENC_ME_COMPLEXITY         = 2;
    	pH264ENC->H264ENC_ME_REFINE_COUNT       = 0x1FFF;
    	pH264ENC->H264ENC_ME_PART_LIMIT_COUNT   = 0x7FFF;
    	pH264ENC->H264ENC_ME_PART_COST_THRES    = 0;
    	pH264ENC->H264ENC_ME_NO_SUBBLOCK        = 0;
    	pH264ENC->H264ENC_ME_EARLY_STOP_THRES   = 15;

        //Mode Decision
        //(min_cost > inter_cost_thr) && (((intra_cost * intra_cost_adj) >> 4) < (min_cost+inter_cost_bias))
        //TRUE    : intra
        //FALSE   : inter
    	pH264ENC->H264ENC_ME_INTER_COST_THRES = 512;
    	pH264ENC->H264ENC_INTRA_COST_BIAS     = 16;
    	pH264ENC->H264ENC_INTER_COST_BIAS     = (MMP_USHORT)-512;

    	pH264ENC->H264ENC_ME_STOP_THRES_UPPER_BOUND = 256;
    	pH264ENC->H264ENC_ME_STOP_THRES_LOWER_BOUND = 128;
    	pH264ENC->H264ENC_ME_SKIP_THRES_UPPER_BOUND = 512;
    	pH264ENC->H264ENC_ME_SKIP_THRES_LOWER_BOUND = 256;

	    pH264ENC->H264ENC_FRAME_CTL         |= H264_SPEED_UP;
        pH264ENC->H264ENC_ME_INTER_CTL      |= INTER_PIPE_MODE_EN;
        pH264ENC->H264ENC_ME_INTER_CTL      |= INTER_8_PIXL_ONLY;
        pH264ENC->H264ENC_ME_NO_SUBBLOCK    |= REFINE_PART_NO_SUBBLK;

        //Intra setting
        pH264ENC->H264ENC_TRANS_CTL            = CBP_CTL_EN;
        pH264ENC->H264ENC_LUMA_COEFF_COST      = (0x5 << 4) | 0x4;
        pH264ENC->H264ENC_CHROMA_COEFF_COST    = 0x4;
        pH264ENC->H264ENC_COST_LARGER_THAN_ONE = 0x9;
        pH264ENC->H264ENC_INTRA_PRED_MODE      = INTRA_PRED_IN_INTER_SLICE_EN;

        pH264ENC->H264ENC_ME16X16_MAX_MINUS_1   = 0;
        pH264ENC->H264ENC_IME16X16_MAX_MINUS_1  = 0;
        pH264ENC->H264ENC_ME_INTER_CTL         |= SKIP_CAND_INCR_ME_STEP_IME;

        // HPB related setting
        pH264VLD->FRAME_DATA_INFO_REG |= H264_CURR_NV12; // cur CbCr interleave mode
        pH264VLD->DATA_INFO_REG |= H264_REC_NV12;        // ref CbCr interleave mode
        pH264ENC->H264ENC_IME_COST_WEIGHT0 = 0x64;
        pH264ENC->H264ENC_IME_COST_WEIGHT1 = 0x86;

        pH264ENC->H264ENC_QSTEP_2[0] = 0;
        pH264ENC->H264ENC_QSTEP_2[1] = 0;
        pH264ENC->H264ENC_QSTEP_2[2] = 0;
        pH264ENC->H264ENC_QSTEP_2[3] = 0;
        pH264ENC->H264ENC_QSTEP_2[4] = 0;
        pH264ENC->H264ENC_QSTEP_2[5] = 0;
        pH264ENC->H264ENC_QSTEP_3[0] = rdo_step_3[0] | (rdo_step_3[1] << 4);
        pH264ENC->H264ENC_QSTEP_3[1] = rdo_step_3[2] | (rdo_step_3[3] << 4);
        pH264ENC->H264ENC_QSTEP_3[2] = rdo_step_3[4] | (rdo_step_3[5] << 4);
        pH264ENC->H264ENC_QSTEP_3[3] = rdo_step_3[6] | (rdo_step_3[7] << 4);
        pH264ENC->H264ENC_QSTEP_3[4] = rdo_step_3[8] | (rdo_step_3[9] << 4);

        // Deblocking setting
        if (1) {
            if (0) { //alpha/beta adjusting
                pH264ENC->H264ENC_DBLK_CTRL_PRESENT |= H264ENC_DBLK_EN;
                pH264VLD->ALPHA_OFFSET_DIV2 = 0;//Alpha offset divide 2
                pH264VLD->BETA_OFFSET_DIV2 =  0;//Beta  offset divide 2
            }
            else {
                pH264ENC->H264ENC_DBLK_CTRL_PRESENT &= ~H264ENC_DBLK_EN;
            }
            pH264VLD->DATA_INFO_REG &= ~(0x06); // 0xC88A[2:3]=0
        }
        else {
        	pH264ENC->H264ENC_DBLK_CTRL_PRESENT |= H264ENC_DBLK_EN;
            pH264VLD->DATA_INFO_REG |= 0x02;
        }
        pH264ENC->H264ENC_VLC_CTRL2 |= H264E_INSERT_EP3_EN;

        pH264VLD->HIGH_PROF_REG_2   |= H264_FRM_MBS_ONLY_FLAG;
        // enable module to encode mode
        pH264DEC->FRAME_CTL_REG = (AVC_DEC_ENABLE|AVC_ENC_ENABLE);//acquired mci req

        pH264VLD->HIGH_PROF_REG_3 = 0;
        pH264VLD->HIGH_PROF_REG_4 &= ~(H264_SEQ_SCAL_LIST_PRESENT_FLAG |
                                       H264_SEQ_SCAL_MATRIX_PRESENT_FLAG);
        pH264VLD->HIGH_PROF_REG_5 &= ~(H264_PIC_SCAL_LIST_PRESENT_FLAG |
                                       H264_PIC_SCAL_MATRIX_PRESENT_FLAG);
        pH264VLD->HIGH_PROF_REG_4 &= ~(H264_POC_TYPE_MASK);
        pH264VLD->HIGH_PROF_REG_4 &= ~(H264_SPS_ID_MASK);
        pH264VLD->HIGH_PROF_REG_6 &= ~(H264_PPS_ID_MASK);
        pH264VLD->HIGH_PROF_REG_7 &= ~(H264_LONG_TERM_REF_FLAG);
	}

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_DeinitModule (MMPF_H264ENC_MODULE *pModule)
{
	MMPF_H264ENC_ENC_INFO *pEnc	  = pModule->pH264Inst;
	AITPS_H264ENC		  pH264ENC = AITC_BASE_H264ENC;
	AITPS_H264DEC_CTL	  pH264DEC = AITC_BASE_H264DEC_CTL;

    if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        pH264ENC->H264ENC_INT_CPU_EN &= ~(ENC_CUR_BUF_OVWR_INT);
        pH264ENC->H264ENC_FRAME_CTL  &= ~(H264_CUR_PP_MODE_EN);
    }

    pH264ENC->TNR_ENABLE = 0;
    pH264DEC->FRAME_CTL_REG &= ~(AVC_DEC_ENABLE); // release vsram mci request

    return MMP_ERR_NONE;
}

/** @brief Initialize H264 encoder setting
 @param[in]
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_InitInstance(MMPF_H264ENC_ENC_INFO *enc, MMPF_H264ENC_MODULE *attached_mod, void *priv)
{
    instance_parameter_config(enc);

    enc->module    = attached_mod;
    enc->priv_data = priv;
    enc->enc_id    = get_vidinst_id(enc->priv_data);

    return MMP_ERR_NONE;
}

/** @brief Deinitialize H264 encoder setting
 @param[in]
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_DeInitInstance(MMPF_H264ENC_ENC_INFO *enc, MMP_ULONG InstId)
{
    MMP_ULONG   layer, fps;

    fps = (enc->ulEncFpsRes+ enc->ulEncFpsInc - 1) / enc->ulEncFpsInc;
    for(layer = 0; layer < enc->total_layers; layer++) {
        MMPF_VidRateCtl_DeInit( enc->layer_rc_hdl[layer],
                                (InstId * MAX_NUM_TMP_LAYERS) + layer,
                                enc->layer_tgt_frm_size[layer],
                                enc->layer_bitrate[layer],
                                &enc->rc_config[layer],
                                fps);
        enc->layer_rc_hdl[layer] = NULL;
    }

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_H264ENC_InitRefListConfig (MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ULONG   bufsize, bufstart;
    MMP_USHORT  usWidth = (pEnc->mb_w << 4);
    MMP_USHORT  usHeight = (pEnc->mb_h << 4);

    bufsize = usWidth * usHeight;

    bufstart = pEnc->RefGenBufLowBound;

    bufstart = ALIGN32(bufstart);
    pEnc->RefBufBound.LowBound.ulYAddr  = bufstart;
    bufstart += bufsize;
    pEnc->RefBufBound.HighBound.ulYAddr = bufstart;

	bufsize /= 2;
	pEnc->RefBufBound.LowBound.ulUAddr   = bufstart;
	bufstart += bufsize;
    pEnc->RefBufBound.HighBound.ulUAddr  = bufstart;

    pEnc->RefBufBound.LowBound.ulVAddr   = 0;
    pEnc->RefBufBound.HighBound.ulVAddr  = 0;

    bufstart = ALIGN32(bufstart);

    #if (SHARE_REF_GEN_BUF == 1)
    pEnc->GenBufBound = pEnc->RefBufBound;

    pEnc->rec_frm = pEnc->RefBufBound.LowBound;
    pEnc->ref_frm = pEnc->RefBufBound.LowBound;
    #else
    #if (H264ENC_ICOMP_EN)
	bufsize = usWidth * usHeight;

	pEnc->GenBufBound.LowBound.ulYAddr  = bufstart;
	bufstart += bufsize;
	pEnc->GenBufBound.HighBound.ulYAddr = bufstart;
	pEnc->GenBufBound.LowBound.ulUAddr  = bufstart;
	bufstart += bufsize/2;
	pEnc->GenBufBound.HighBound.ulUAddr = bufstart;
    pEnc->GenBufBound.LowBound.ulVAddr  = 0;
    pEnc->GenBufBound.HighBound.ulVAddr = 0;
    #else
	bufsize = (usWidth * usHeight * 3)/2; // Frame total size
	pEnc->GenBufBound.LowBound.ulYAddr  = bufstart;
	bufstart += bufsize;
	pEnc->GenBufBound.HighBound.ulVAddr = bufstart;
    #endif//(H264ENC_ICOMP_EN)
    pEnc->rec_frm = pEnc->GenBufBound.LowBound;
    pEnc->ref_frm = pEnc->RefBufBound.LowBound;
    #endif


    if (bufstart > pEnc->RefGenBufHighBound) {
        RTNA_DBG_Str(0, "#Error reserved refgen buf not enough!\n");
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    return MMP_ERR_NONE;
}

/**
 @brief START operation of H264ENC.

 This function clears interrupt status and enables. It initalizes parameters used
 in filling 3GP file, ptr of AV repack buffer and frame table buffer, and the sync
 buffer. It fills the 3GP header and open file if it is card mode. It also enables
 the SD and timer1 interrupt. At last, it starts the video engine.
 @retval MMP_ERR_NONE Success.
 @retval MMP_FS_ERR_OPEN_FAIL Open file failed.
*/
MMP_ERR MMPF_H264ENC_Open(MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ERR ret;

    if (pEnc->profile == BASELINE_PROFILE) {
        if (pEnc->entropy_mode == H264ENC_ENTROPY_CABAC) {
            pEnc->entropy_mode = H264ENC_ENTROPY_CAVLC;
            RTNA_DBG_Str(0, "#Error : baseline not support CABAC\r\n");
        }
    }

    if (pEnc->b_frame_num) {
        if ((pEnc->profile == BASELINE_PROFILE)) {
            pEnc->profile = FREXT_HP;
            pEnc->entropy_mode = H264ENC_ENTROPY_CABAC;
            RTNA_DBG_Str(0, "#Error : b frame force high profile\r\n");
        }
    }

    if (pEnc->b_frame_num) {
        pEnc->usMaxNumRefFrame = 2;
    }
    else {
        pEnc->usMaxNumRefFrame = 1;
    }
    pEnc->usMaxFrameNumAndPoc = (2 << 4) | 2;

    pEnc->insert_sps_pps    = MMP_TRUE;
	pEnc->TotalEncodeSize	= 0;
    pEnc->total_frames      = 0;
    pEnc->conus_i_frame_num = 1;
    pEnc->gop_frame_num     = 0;
    pEnc->co_mv_valid       = MMP_FALSE;
    pEnc->prev_ref_num      = 0;
    pEnc->timestamp         = 0;
    pEnc->ulParamQueueRdWrapIdx = 0;
    pEnc->ulParamQueueWrWrapIdx = 0;
    pEnc->OpFrameType       = MMPF_3GPMGR_FRAME_TYPE_I;
    pEnc->OpIdrPic          = MMP_FALSE;
    pEnc->OpInsParset       = MMP_FALSE;
    pEnc->mb_num            = pEnc->mb_w * pEnc->mb_h;

	pEnc->ulFpsInputRes = pEnc->ulSnrFpsRes;
	pEnc->ulFpsInputInc = pEnc->ulSnrFpsInc;
	pEnc->ulFpsInputAcc = 0;
	pEnc->ulFpsOutputRes = pEnc->ulEncFpsRes;
	pEnc->ulFpsOutputInc = pEnc->ulEncFpsInc;
	pEnc->ulFpsOutputAcc = 0;

    #if (H264ENC_LBR_EN)
    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
        pEnc->layer_fps_ratio_low_br[0]  = 1;
        pEnc->layer_fps_ratio_low_br[1]  = pEnc->gop_size - 1;
        pEnc->ubAccResetLowBrRatioTimes  = 0;
        pEnc->ubAccResetHighBrRatioTimes = 0;
        pEnc->bResetLowBrRatio  = MMP_TRUE;
        pEnc->bResetHighBrRatio = MMP_FALSE;
        pEnc->intra_mb = 0;
    }
    else
    #endif
    {
        pEnc->layer_fps_ratio[0] = 1;
    }

    #if (VID_CTL_DBG_BR)
    pEnc->br_total_size     = 0;
    pEnc->br_base_time      = 0;
    pEnc->br_last_time      = 0;
    #endif

    //Initialize Ref List Config, store REF/GEN buffer address
    ret = MMPF_H264ENC_InitRefListConfig(pEnc);
    if (ret != MMP_ERR_NONE) {
        return ret;
    }

	DBG_S(3, "Init RC\r\n");

    #if (H264ENC_LBR_EN)
    if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
        pEnc->layer_bitrate[0] = (pEnc->stream_bitrate * 7) / 10;
        pEnc->layer_bitrate[1] = (pEnc->stream_bitrate * 3) / 10;
    }
    #endif

	ret = MMPF_H264ENC_InitRCConfig(pEnc);
    if (ret != MMP_ERR_NONE) {
        return ret;
    }

	#if (H264ENC_ICOMP_EN)
    pEnc->ICompConfig.bICompEnable        = MMP_TRUE;
    pEnc->ICompConfig.bICompCurMbLsyEn    = MMP_TRUE;
    pEnc->ICompConfig.bICompLsyLvlCtlEn   = MMP_TRUE;
    pEnc->ICompConfig.ubICompRatio        = 90;
    pEnc->ICompConfig.ubICompMinLsyLvlLum = 0;
    pEnc->ICompConfig.ubICompMinLsyLvlChr = 0;
    pEnc->ICompConfig.ubICompMaxLsyLvlLum = 7;
    pEnc->ICompConfig.ubICompMaxLsyLvlChr = 7;
    pEnc->ICompConfig.ubICompIniLsyLvlLum = (pEnc->ICompConfig.ubICompMaxLsyLvlLum+1)/2;
    pEnc->ICompConfig.ubICompIniLsyLvlChr = (pEnc->ICompConfig.ubICompMaxLsyLvlChr+1)/2;
    pEnc->ICompConfig.ulICompFrmSize      = (pEnc->mb_w << 4)*(pEnc->mb_h << 4);
	#endif

    gbRtEncDropFrameEn = MMP_FALSE;

    DBG_S(3, "MMPF_H264ENC_Open end\r\n");

    return MMP_ERR_NONE;
}

/**
 @brief RESUME operation of H264ENC

 It clears video interrupt and enables.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_Resume(MMPF_H264ENC_ENC_INFO *pEnc)
{
    AITPS_H264ENC pH264ENC = AITC_BASE_H264ENC;

	pEnc->GlbStatus.VidOp = VIDEO_RECORD_RESUME;
#if (WORK_AROUND_EP3)
    pH264ENC->H264ENC_INT_CPU_EN |= (FRAME_ENC_DONE_CPU_INT_EN | EP_BYTE_CPU_INT_EN);
#else
    pH264ENC->H264ENC_INT_CPU_EN |= (FRAME_ENC_DONE_CPU_INT_EN);
#endif

    pEnc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_RESUME;

    if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        gbRtEncDropFrameEn = MMP_FALSE;
        MMPF_VIDENC_TriggerEncode();
    }

    return MMP_ERR_NONE;
}

/**
 @brief Close h264enc module

 It clears h264enc setting
 @retval MMP_ERR_NONE Success.
*/
//MMP_ERR MMPF_H264ENC_Close (MMPF_H264ENC_ENC_INFO *pEnc)
//{
//    AITPS_H264ENC pH264ENC = AITC_BASE_H264ENC;

//    pH264ENC->H264ENC_INT_CPU_EN &= ~FRAME_ENC_DONE_CPU_INT_EN;

//    if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
//        pH264ENC->H264ENC_INT_CPU_EN &= ~ENC_CUR_BUF_OVWR_INT;
//        pH264ENC->H264ENC_FRAME_CTL  &= ~H264_CUR_PP_MODE_EN;
//    }

//    pH264DEC->FRAME_CTL_REG &= ~AVC_DEC_ENABLE; // release vsram mci request

//    return MMP_ERR_NONE;
//}

#if (H264ENC_LBR_EN)&&(H264ENC_LBR_FLOAT_RATIO)
/**
 @brief Adjust the ratio of I/P layer bitrates according to motion or static scene.
 @param[in] pEnc    pointer to encoder info
 @param[in] bMotion higher motion scene
*/
static void MMPF_H264ENC_AdjustLowbrRatio(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                            MMP_BOOL                bMotion)
{
    MMP_ULONG I_br, gop = pEnc->gop_size;
    MMP_LONG  I_br_ratio;

    // gop > 120 & gop < 30 is no good for low bitrate mode
    if (gop > 120) {
        gop = 120;
    }
    else if (gop <= 1) {
        pEnc->layer_bitrate[0] = pEnc->stream_bitrate;
        pEnc->layer_bitrate[1] = 0;
        pEnc->decision_mode = bMotion;
        return;
    }

    if (!bMotion) {
        if (gop <= 30)
            I_br_ratio = LBR_STATIC_I_RATIO_30FPS;
        else if (gop <= 60)
            I_br_ratio = LBR_STATIC_I_RATIO_60FPS;
        else if (gop <= 90)
            I_br_ratio = LBR_STATIC_I_RATIO_90FPS;
        else
            I_br_ratio = LBR_STATIC_I_RATIO_120FPS;

        I_br = (pEnc->stream_bitrate * I_br_ratio) / 100;  
    }
    else {
        if (gop <= 30)
            I_br_ratio = LBR_MOTION_I_RATIO_30FPS;
        else if (gop <= 60)
            I_br_ratio = LBR_MOTION_I_RATIO_60FPS;
        else if (gop <= 90)
            I_br_ratio = LBR_MOTION_I_RATIO_90FPS;
        else
            I_br_ratio = LBR_MOTION_I_RATIO_120FPS;

        if (pEnc->intra_mb) {
            I_br_ratio -= (I_br_ratio * (pEnc->intra_mb << 1)) / pEnc->mb_num;
        }

        if (I_br_ratio <= 0)
            I_br_ratio = LBR_MOTION_I_RATIO_120FPS;

        I_br = (pEnc->stream_bitrate * I_br_ratio) / 100;    
    }           

    pEnc->layer_bitrate[0]  = I_br;
    pEnc->layer_bitrate[1]  = pEnc->stream_bitrate - I_br;
    pEnc->decision_mode     = bMotion;
}
#endif

/**
 @brief Trigger to encode next frame.
 @param[in] ulBaseAddr. Y Address for storing the ready buffer for encode.
 @param[in] ulBaseUAddr. U Address for storing the ready buffer for encode.
 @param[in] ulBaseVAddr. V Address for storing the ready buffer for encode.
*/
MMP_ERR MMPF_H264ENC_TriggerEncode( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMPF_3GPMGR_FRAME_TYPE  FrameType,
                                    MMP_ULONG               ulFrameTime,
                                    MMPF_VIDENC_FRAME       *pFrameInfo)
{
    AITPS_H264ENC		   pH264ENC = AITC_BASE_H264ENC;
    AITPS_H264DEC_CTL	   pH264DEC = AITC_BASE_H264DEC_CTL;
    AITPS_H264DEC_VLD	   pH264VLD = AITC_BASE_H264DEC_VLD;
    #if (H264ENC_ICOMP_EN)
    AITPS_H264DEC_DBLK_ROT pH264DBLK_ROT = AITC_BASE_H264DEC_DBLK_ROT;
    #endif
    MMP_BOOL    bAbort = MMP_FALSE, bSkipFrame = MMP_FALSE;
    MMP_ULONG   ulMbBitsX256;
    MMP_ULONG   ulTargetSize, InstId;
    MMP_ULONG   qp_delta, qp_delta_low;
    MMP_ULONG   ulEncOrder;
    MMP_ULONG   ulOffset, ulBufId, ulDts;
    MMP_ERR     qState;
    MMP_ULONG   layer = 0;

	pEnc->module->pH264Inst = pEnc;
    InstId = get_vidinst_id(pEnc->priv_data);

    #if (H264ENC_LBR_EN)
    if ((FrameType == MMPF_3GPMGR_FRAME_TYPE_P) &&
        (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR)) {
        layer = 1;
    }
    #endif

    bAbort = MMPF_VStream_ShouldFrameSkip(InstId, pEnc->layer_frm_thr[layer]);

    if (!bAbort) {
        if (pEnc->insert_sps_pps) {
    		pH264ENC->H264ENC_VLC_CTRL2 |= INSERT_SPS_PPS_BEFORE_I_SLICE;
    		pEnc->insert_sps_pps = MMP_FALSE;
    	}

    	#if H264ENC_GDR_EN
        if (pEnc->intra_refresh_en) {
            if (pEnc->cur_frm_type != MMPF_3GPMGR_FRAME_TYPE_I) {
                pEnc->intra_refresh_offset = (pEnc->intra_refresh_offset + 1) < pEnc->intra_refresh_period ?
                                        pEnc->intra_refresh_offset + 1 : 0;
            }
            else {
                pEnc->intra_refresh_offset = 0;
                pEnc->intra_refresh_period = (pEnc->mb_w / 10) * 7; // 10 intra MB inside 7 MB rows
            }
            MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_MB, pEnc->intra_refresh_period, pEnc->intra_refresh_offset);
        }
        else {
            MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_DIS, 0, 0); 
        }
        #endif

        // set each-resolution settings
        pEnc->cur_frm_type = FrameType;
        ulEncOrder = pEnc->gop_frame_num % pEnc->gop_size;

        switch(pEnc->cur_frm_type) {
        case MMPF_3GPMGR_FRAME_TYPE_I:
            //i slice use full intra search
            pH264ENC->H264ENC_INTRA_PRED_MODE &= ~INTRA_6_MODE_EN;

            //num_ref_idx_l0_active_minus1
            pH264VLD->DATA_INFO_REG     &= ~H264_NUM_REF_IDX_L0_MINUS1_MASK;
            //num_ref_idx_l1_active_minus1
            pH264VLD->HIGH_PROF_REG_1   &= ~H264_NUM_REF_IDX_L1_MINUS1_MASK;
            //num_ref_idx_override_flag
            pH264VLD->HIGH_PROF_REG_1   &= ~H264_NUM_REF_IDX_OVERRIDE_FLAG;

            pH264VLD->HIGH_PROF_REG_3   = 0x00;

            //b slice flag
            pH264VLD->FRAME_DATA_INFO_REG &= ~H264_B_SLICE_FLAG;
            //p slice flag
            pH264VLD->FRAME_DATA_INFO_REG &= ~H264_P_SLICE_FLAG;
            //i slice flag
            pH264VLD->DATA_INFO_REG     |= H264_I_SLICE_FLAG;
            //insert SPS/PPS for every I 
            pH264ENC->H264ENC_VLC_CTRL2 |= INSERT_SPS_PPS_BEFORE_I_SLICE;

            pH264ENC->H264ENC_IDR_FRAME = pEnc->gop_frame_num % 2;
            pH264ENC->H264ENC_POC       = 0;
            pH264ENC->H264ENC_FRAME_NUM = 0;

            pEnc->prev_ref_num   = 0;
            pEnc->rc_skip_bypass = MMP_TRUE;
            break;

        case MMPF_3GPMGR_FRAME_TYPE_P:
            //intra6 speed up
            if (pEnc->mb_num > 3600)
                pH264ENC->H264ENC_INTRA_PRED_MODE |= INTRA_6_MODE_EN;

            //num_ref_idx_l0_active_minus1
            pH264VLD->DATA_INFO_REG     &= ~H264_NUM_REF_IDX_L0_MINUS1_MASK;
            //num_ref_idx_l1_active_minus1
            pH264VLD->HIGH_PROF_REG_1   &= ~H264_NUM_REF_IDX_L1_MINUS1_MASK;
            //num_ref_idx_override_flag
            pH264VLD->HIGH_PROF_REG_1   &= ~H264_NUM_REF_IDX_OVERRIDE_FLAG;

            pH264VLD->HIGH_PROF_REG_3 = 0x01; // p frame force MVx <= 160

            //p slice flag
            pH264VLD->FRAME_DATA_INFO_REG |= H264_P_SLICE_FLAG;
            //p slice flag
            pH264VLD->FRAME_DATA_INFO_REG &= ~H264_B_SLICE_FLAG;
            //i slice flag
            pH264VLD->DATA_INFO_REG       &= ~H264_I_SLICE_FLAG;
            if (pEnc->co_mv_valid)
                pH264ENC->H264ENC_FIRST_P_AFTER_I_IDX &= ~H264E_COLOCATE_MV_REF_DIS;
            else
                pH264ENC->H264ENC_FIRST_P_AFTER_I_IDX |= H264E_COLOCATE_MV_REF_DIS;

            pH264ENC->H264ENC_POC       = (ulEncOrder + pEnc->b_frame_num) * 2;
            pH264ENC->H264ENC_FRAME_NUM = pEnc->prev_ref_num + 1;

            pEnc->rc_skip_bypass = MMP_FALSE;
            break;

        default:
            DBG_S(0, "Unknown frame type\r\n");
            return MMP_ERR_NONE;
        }

    	#if (H264ENC_I2MANY_EN)
        pH264ENC->H264ENC_I2MANY_START_MB_LOW  = (((pEnc->mb_num >> 3) + 1) &
                                                    H264E_I2MANY_LO_BYTE_MASK);
        pH264ENC->H264ENC_I2MANY_START_MB_HIGH = ((((pEnc->mb_num >> 3) + 1) >> 8) &
                                                    H264E_I2MANY_HI_BYTE_MASK);
        pH264ENC->H264ENC_I2MANY_QPSTEP_TABLE  = 0x11;
    	#endif

    	#if (H264ENC_HBR_EN)
        if (pH264VLD->HIGH_PROF_REG_1 & H264_CABAC_MODE_FLAG) {
        	if (m_ubHbrSel == MMPF_H264ENC_HBR_MODE_60P) { //60fps
        		pH264ENC->H264ENC_QP_FINE_TUNE_EN |= H264E_HBR_QP_EN;
        		pH264ENC->H264ENC_HBR_TABLE_0 = 4130;
        		pH264ENC->H264ENC_HBR_TABLE_1 = 3104;
        		pH264ENC->H264ENC_HBR_TABLE_2 = 2078;
        		pH264ENC->H264ENC_HBR_TABLE_3 = 1564;
        		pH264ENC->H264ENC_HBR_TABLE_4 = 1050;
        		pH264ENC->H264ENC_HBR_TABLE_5 = 793;
        		pH264ENC->H264ENC_HBR_TABLE_6 = 536;
        		pH264ENC->H264ENC_HBR_TABLE_7 = 279;
        	} else if (m_ubHbrSel == MMPF_H264ENC_HBR_MODE_30P) {//30fps
        		pH264ENC->H264ENC_QP_FINE_TUNE_EN |= H264E_HBR_QP_EN;
        		pH264ENC->H264ENC_HBR_TABLE_0 = 4126;
        		pH264ENC->H264ENC_HBR_TABLE_1 = 3100;
        		pH264ENC->H264ENC_HBR_TABLE_2 = 2074;
        		pH264ENC->H264ENC_HBR_TABLE_3 = 1560;
        		pH264ENC->H264ENC_HBR_TABLE_4 = 1046;
        		pH264ENC->H264ENC_HBR_TABLE_5 = 789;
        		pH264ENC->H264ENC_HBR_TABLE_6 = 532;
        		pH264ENC->H264ENC_HBR_TABLE_7 = 274;
        	} else {
        		pH264ENC->H264ENC_QP_FINE_TUNE_EN &= ~(H264E_HBR_QP_EN);
        	}
        }
    	#endif

    	#if (H264ENC_ICOMP_EN)
        MMPF_H264ENC_InitImageComp(pEnc, &(pEnc->ICompConfig));
    	#endif

        #if (H264ENC_LBR_EN)&&(H264ENC_LBR_FLOAT_RATIO)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
            if ( pEnc->ubAccResetLowBrRatioTimes >= 5) {
                // higher motion, share more bitrate to P-frame
                if (pEnc->bResetLowBrRatio) {
                    MMPF_H264ENC_AdjustLowbrRatio(pEnc, MMP_TRUE);
                    MMPF_H264ENC_UpdateRateControl( pEnc,
                                                    pEnc->ulFpsInputRes,
                                                    pEnc->ulFpsInputInc,
                                                    pEnc->ulFpsOutputRes,
                                                    pEnc->ulFpsOutputInc);

                    pEnc->bResetLowBrRatio  = MMP_FALSE;
                    pEnc->bResetHighBrRatio = MMP_TRUE;
                    pEnc->ubAccResetLowBrRatioTimes = 0;
                }
            }
            else if (pEnc->ubAccResetHighBrRatioTimes > 20) {
                // lower motion, share more bitrate to I-frame
                if (pEnc->bResetHighBrRatio) {
                    MMPF_H264ENC_AdjustLowbrRatio(pEnc, MMP_FALSE);
                    MMPF_H264ENC_UpdateRateControl( pEnc,
                                                    pEnc->ulFpsInputRes,
                                                    pEnc->ulFpsInputInc,
                                                    pEnc->ulFpsOutputRes,
                                                    pEnc->ulFpsOutputInc);

                    pEnc->bResetLowBrRatio  = MMP_TRUE;
                    pEnc->bResetHighBrRatio = MMP_FALSE;
                    pEnc->ubAccResetHighBrRatioTimes = 0;
                }
            }
        }
        #endif

        pEnc->CurRcQP[layer][pEnc->cur_frm_type] =
                MMPF_VidRateCtl_Get_VOP_QP (pEnc->layer_rc_hdl[layer],
                                            pEnc->cur_frm_type,
                                            &ulTargetSize,
                                            &qp_delta,
                                            &bSkipFrame,
                                            pEnc->layer_frm_thr[layer]);

        if (pEnc->rc_skip_bypass)
            bSkipFrame = MMP_FALSE;

        if (!bSkipFrame) {
            pH264VLD->QP               = pEnc->CurRcQP[layer][pEnc->cur_frm_type];
            pH264ENC->H264ENC_BASIC_QP = pEnc->CurRcQP[layer][pEnc->cur_frm_type];

            qp_delta_low = qp_delta;

            if (pEnc->bRcEnable) {
                ulMbBitsX256 = ((ulTargetSize << 11) / pEnc->mb_num);

                pH264ENC->H264ENC_TGT_MB_SIZE_X256_LSB = (MMP_USHORT)(ulMbBitsX256 & 0xFFFF);
            	pH264ENC->H264ENC_TGT_MB_SIZE_X256_MSB = (MMP_UBYTE)(ulMbBitsX256 >> 16);
                pH264ENC->H264ENC_QP_TUNE_STEP = ALIGN2((qp_delta + 1) >> 1);
            	pH264ENC->H264ENC_QP_FINE_TUNE_EN      = pEnc->qp_tune_mode;
            	pH264ENC->H264ENC_QP_BASICUNIT_SIZE    = pEnc->qp_tune_size;
            }
            #if (H264ENC_RDO_EN)
            else if (pEnc->bRdoEnable) {
                if (pEnc->bRdoMadValid) {
                	if (pEnc->ulRdoMadFrmPre < (pEnc->mb_num << 2)) {
                		pEnc->ulRdoMadFrmPre = pEnc->mb_num << 2;
                    }
                	else if (pEnc->ulRdoMadLastRowPre > (pEnc->ulRdoMadFrmPre >> 2)) {
                		pEnc->ulRdoMadFrmPre =  pEnc->ulRdoMadFrmPre -
                                                pEnc->ulRdoMadLastRowPre +
                				                pEnc->ulRdoMadFrmPre / pEnc->mb_h;
                    }
                    ulTargetSize += (ulTargetSize >> 3); //*1.125
                    if (ulTargetSize > ((1 << 21) - 1))
                        ulTargetSize = ((1 << 21) - 1);
                	pEnc->ulRdoMadFrmPre += (pEnc->ulRdoMadFrmPre >> 3); //*1.125

                	pEnc->ulRdoInitFac = (ulTargetSize << 13) / pEnc->ulRdoMadFrmPre;
                    pH264ENC->H264ENC_QP_FINE_TUNE_EN = H264E_FRM_MAD_EN | H264E_RDO_EN;

                    pH264ENC->H264ENC_TGT_MB_SIZE_X256_LSB = (MMP_USHORT)(ulTargetSize << 3);
                	pH264ENC->H264ENC_TGT_MB_SIZE_X256_MSB = (MMP_UBYTE)((ulTargetSize << 3) >> 16);
                    qp_delta_low = 4; // designer's suggestion
                }
                else {
                    MMP_ULONG mb_bits_x256 = (ulTargetSize << 11) / pEnc->mb_num;

                    // RDO init
                    pEnc->ulRdoMadFrmPre = 0;
                    pEnc->ulRdoInitFac   = 0;
                    pH264ENC->H264ENC_QP_FINE_TUNE_EN = (pH264ENC->H264ENC_QP_FINE_TUNE_EN |
                                                        H264E_FRM_MAD_EN) &
                                                        ~(H264E_RDO_EN);

                    pH264ENC->H264ENC_TGT_MB_SIZE_X256_LSB  = (MMP_USHORT)(mb_bits_x256);
                    pH264ENC->H264ENC_TGT_MB_SIZE_X256_MSB  = (MMP_UBYTE)(mb_bits_x256 >> 16);
                }

            	pH264ENC->H264ENC_PREV_AVG_MB_MAD   = (pEnc->ulRdoMadFrmPre / pEnc->mb_num);
            	pH264ENC->H264ENC_PREV_FRM_MAD      = pEnc->ulRdoMadFrmPre;
            	pH264ENC->H264ENC_RDO_FRM_INIT_FAC  = pEnc->ulRdoInitFac;
            	pH264ENC->H264ENC_QP_BASICUNIT_SIZE = pEnc->mb_w; //designer suggestion
            	pH264ENC->H264ENC_QP_TUNE_STEP      = ALIGN2((qp_delta+1)>>1);
            }
            #endif
            else {
                pH264ENC->H264ENC_QP_FINE_TUNE_EN = QP_FINETUNE_DIS;
            }

            /* check QP up/low boundary */
            pH264ENC->H264ENC_QP_UP_BOUND = pEnc->CurRcQP[layer][pEnc->cur_frm_type] + qp_delta;
        	if (pH264ENC->H264ENC_QP_UP_BOUND > pEnc->MbQpBound[layer][pEnc->cur_frm_type][BD_HIGH])
        		pH264ENC->H264ENC_QP_UP_BOUND = pEnc->MbQpBound[layer][pEnc->cur_frm_type][BD_HIGH];

            pH264ENC->H264ENC_QP_LOW_BOUND = pEnc->CurRcQP[layer][pEnc->cur_frm_type] - qp_delta_low;
        	if (pH264ENC->H264ENC_QP_LOW_BOUND < pEnc->MbQpBound[layer][pEnc->cur_frm_type][BD_LOW])
        		pH264ENC->H264ENC_QP_LOW_BOUND = pEnc->MbQpBound[layer][pEnc->cur_frm_type][BD_LOW];

            pH264ENC->H264ENC_FRAME_CROPPING_TOP    = (pEnc->crop.usTop >> 1);
            pH264ENC->H264ENC_FRAME_CROPPING_BOTTOM = (pEnc->crop.usBottom >> 1);
            pH264ENC->H264ENC_FRAME_CROPPING_LEFT   = (pEnc->crop.usLeft >> 1);
            pH264ENC->H264ENC_FRAME_CROPPING_RIGHT  = (pEnc->crop.usRight >> 1);

            if (pEnc->crop.usTop || pEnc->crop.usBottom || pEnc->crop.usLeft || pEnc->crop.usRight) {
                pH264ENC->H264ENC_FRAME_CROPPING_FLAG |= FRAME_CROPPING_EN;
            }
            else {
                pH264ENC->H264ENC_FRAME_CROPPING_FLAG &= ~FRAME_CROPPING_EN;
            }
            pH264ENC->H264ENC_LEVEL_IDC = pEnc->level;

            MMPF_H264ENC_RestoreRefList(pEnc);

            pH264VLD->HIGH_PROF_REG_7 = ((pH264VLD->HIGH_PROF_REG_7 & ~H264_NAL_REF_IDC_MASK) | 3);

            pEnc->timestamp = ulFrameTime;

            // Set Cur frame
            if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
                // For real-time encode mode, no need to update current frame buffer.
                pH264ENC->H264ENC_FRAME_CTL |= H264_CUR_PP_MODE_EN;
            }
            else {
                pH264ENC->H264ENC_FRAME_CTL &= ~(H264E_OVWR_STRICT_CHK);
                pH264ENC->H264ENC_FRAME_CTL &= ~(H264_CUR_PP_MODE_EN);
                pH264ENC->H264ENC_CURR_Y_ADDR = pFrameInfo->ulYAddr;
                pH264ENC->H264ENC_CURR_U_ADDR = pFrameInfo->ulUAddr;
                pH264ENC->H264ENC_CURR_V_ADDR = pFrameInfo->ulVAddr;
            }
        }
        else {
            MMP_ULONG padding;

            MMPF_VidRateCtl_UpdateModel(pEnc->layer_rc_hdl[layer],
                                        pEnc->cur_frm_type,
                                        0, 0, 0, MMP_FALSE,
                                        &bSkipFrame, &padding);
        }
    }

	if (bSkipFrame || bAbort) {

	    if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_FRAME) {
            // pop frame from ready queue and timestamp queue
            ulOffset = (MMPF_3GPMGR_FRAME_TYPE_P == pEnc->cur_frm_type) ? pEnc->b_frame_num : 0;
            qState = MMPF_VIDENC_PopQueue(&gVidRecdRdyQueue[InstId], ulOffset, &ulBufId, MMP_TRUE);
            if (qState == MMP_ERR_NONE) {
                // the ready frame is really poped from queue, set it as free one
                MMPF_VIDENC_PopQueue(&gVidRecdDtsQueue[InstId], 0, &ulDts, MMP_FALSE);
                MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[InstId], ulBufId, MMP_FALSE);
            }
        }

	    if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) {
	        MMPF_VIDENC_SetEncodeDisable(pEnc);
            return MMP_ERR_NONE;
        }

        RTNA_DBG_Long3(pEnc->total_frames);
        RTNA_DBG_Byte(0, InstId);
        RTNA_DBG_Str(0, "#Vskip\r\n");

        if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            gbRtEncDropFrameEn = MMP_TRUE;
            MMPF_Scaler_EnableInterrupt(m_usVidRecPath[InstId],
                                        MMP_SCAL_EVENT_FRM_END,
                                        MMP_TRUE);
	        MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[InstId],
                                            MMP_SCAL_EVENT_FRM_END,
                                            rt_enc_end_init_callback,
                                            (void *)pEnc);
        }

        // trigger streamer to consume bitstream
        MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(pEnc->enc_id), MMPF_OS_FLAG_SET);
        // trigger encode by task
        MMPF_OS_SetFlags(VID_REC_Flag, SYS_FLAG_VIDENC, MMPF_OS_FLAG_SET);
	}
	else {
		m_ulFrameStartAddr = MMPF_VStream_CompBufAddr2WriteFrame(InstId);

        // Set output bitstream buffer address, should be 32byte align
        pH264ENC->H264ENC_BS_START_ADDR = m_ulFrameStartAddr;
        // bitstream low/up
        pH264ENC->H264ENC_BS_LOWER_BOUND_ADDR = pEnc->cur_frm_bs_low_bd;
        pH264ENC->H264ENC_BS_UPPER_BOUND_ADDR = pEnc->cur_frm_bs_high_bd;

        pH264ENC->H264ENC_PADDING_MODE_SWT = pEnc->paddinginfo.type;
        pH264ENC->H264ENC_PADDING_CNT      = pEnc->paddinginfo.ubPaddingCnt;

        MMPF_H264ENC_RestoreReg(pEnc);

        pH264ENC->H264ENC_SLICE_MODE = ((pH264ENC->H264ENC_SLICE_MODE & ~H264E_SLICE_MODE_MASK)
        		| H264E_SLICE_MODE_FRM); // Set 1 slice/frame

        #if (H264ENC_TNR_EN)
        MMPF_H264ENC_EnableTnr(pEnc);
        #endif

        #if (H264ENC_RDO_EN)
        if (pEnc->bRdoEnable && pEnc->bRdoMadValid)
            MMPF_H264ENC_EnableRdo(pEnc);
        #endif

        pH264ENC->H264ENC_INT_CPU_SR = FRAME_ENC_DONE_CPU_INT_EN;
        pH264ENC->H264ENC_INT_CPU_EN = FRAME_ENC_DONE_CPU_INT_EN;
#if (WORK_AROUND_EP3)
        pH264ENC->H264ENC_INT_CPU_SR = EP_BYTE_CPU_INT_EN;
        pH264ENC->H264ENC_INT_CPU_EN |= EP_BYTE_CPU_INT_EN;
        m_bEP3BHappens = MMP_FALSE;
#endif
        if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            pH264ENC->H264ENC_INT_CPU_SR |= ENC_CUR_BUF_OVWR_INT;
            pH264ENC->H264ENC_INT_CPU_EN |= ENC_CUR_BUF_OVWR_INT;
        }

        pEnc->module->bWorking = MMP_TRUE;

        pH264DEC->FRAME_CTL_REG |= AVC_NEW_FRAME_START; //frame start

        if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            // Enable IBC to store one frame
            MMPF_IBC_SetStoreEnable(m_usVidRecPath[InstId], MMP_TRUE);
            MMPF_IBC_SetSingleFrmEnable(m_usVidRecPath[InstId], MMP_TRUE);
        }
    }

    switch (pEnc->GlbStatus.VidStatus) {
    case MMPF_MP4VENC_FW_STATUS_RECORD:
    case MMPF_MP4VENC_FW_STATUS_STOP:
        if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_START) {
        	MMPF_MP4VENC_SetStatus(InstId, MMPF_MP4VENC_FW_STATUS_START);
        }
        else if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_PREENCODE) {
        	MMPF_MP4VENC_SetStatus(InstId, MMPF_MP4VENC_FW_STATUS_PREENCODE);
        }
        break;
    case MMPF_MP4VENC_FW_STATUS_PREENCODE:
        if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_START) {
        	pEnc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_START;
        }
        break;
    default:
        break;
    }

    return MMP_ERR_NONE;
}
/**
 @brief ISR of H264ENC

 This ISR processes the interrupt from frame end. In the status of frame end, it
 keeps the frame size in queue for Mergr3GP task.
*/
void MMPF_H264ENC_ISR(void)
{
#if MIPI_TEST_PARAM_EN==1
extern MMP_ULONG  UI_GetPFrameTH(void);
extern MMP_ULONG  mipi_test_ng;
    MMP_ULONG psize_th ;
#endif
  
    AITPS_GBL               pGBL = AITC_BASE_GBL;
    AITPS_H264ENC           pH264ENC = AITC_BASE_H264ENC;
    AITPS_H264DEC_VLD       pH264VLD = AITC_BASE_H264DEC_VLD;
    MMP_USHORT              status;
    MMP_USHORT              sliceNumber, i;
    MMP_USHORT              MBHeight;
    MMP_ULONG               frameSize;
    MMP_ULONG               InstId;
    MMP_ULONG               *sliceLenBuf;
    MMP_ULONG               frameYSize;
    MMPF_H264ENC_ENC_INFO   *pEnc;
    MMP_ULONG               pending_bytes;
    MMP_BOOL                bSkipFrame = MMP_FALSE;
    MMP_ULONG               ulOffset, ulBufId, ulDts;
    MMP_ERR                 qState;
    MMP_ULONG               layer = 0;
    #if (H264ENC_ICOMP_EN)
    MMP_ULONG               ulICompMbXOv, ulICompMbYOv, ulICompMbOffset;
    MMP_ULONG               ulICompMbCbCrXOv, ulICompMbCbCrYOv, ulICompMbCbCrOffset;
    #endif
    MMPF_VIDENC_FRAME       tmpFramePtr;
    MMPF_VSTREAM_FRAMEINFO  info;

    pEnc = MMPF_VIDENC_GetModule()->H264EMod.pH264Inst;
    InstId = get_vidinst_id(pEnc->priv_data);

    status = pH264ENC->H264ENC_INT_CPU_EN & pH264ENC->H264ENC_INT_CPU_SR;
    pH264ENC->H264ENC_INT_CPU_SR = status;

    if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        if ((status & ENC_CUR_BUF_OVWR_INT) && MMPF_IBC_IsH264Overlap(m_usVidRecPath[InstId])) {
            m_bRtCurBufOvlp = MMP_TRUE;
#if (WORK_AROUND_EP3)
            pH264ENC->H264ENC_INT_CPU_EN &= ~(FRAME_ENC_DONE_CPU_INT_EN | ENC_CUR_BUF_OVWR_INT | EP_BYTE_CPU_INT_EN);
#else
            pH264ENC->H264ENC_INT_CPU_EN &= ~(FRAME_ENC_DONE_CPU_INT_EN | ENC_CUR_BUF_OVWR_INT);
#endif
            pGBL->GBL_SW_RST_EN[0] = GBL_RST_H264;
            RTNA_DBG_Str(0, "264 ov\r\n");
            pGBL->GBL_SW_RST_DIS[0] = GBL_RST_H264;
            MMPF_IBC_SetStoreEnable(m_usVidRecPath[InstId], MMP_FALSE);
        }
    }
#if MIPI_TEST_PARAM_EN==1
    psize_th = UI_GetPFrameTH();
#endif
    #if (H264ENC_ICOMP_EN)
    if (status & ICOMP_LUMA_OVLP_INT) {
        RTNA_DBG_Str(0, "IComp luma ovlp\r\n");
    }
    if (status & ICOMP_CHROMA_OVLP_INT) {
        RTNA_DBG_Str(0, "IComp chroma ovlp\r\n");
    }
    #endif

    if (status & FRAME_ENC_DONE_CPU_INT_EN) {

        #if (WORK_AROUND_EP3)
        // Reset VLC for EP3 insert interrupt. Otherwise, interrupt happens always.
        pH264ENC->H264ENC_SW_RST |= H264_VLC_RESET;
        pH264ENC->H264ENC_SW_RST &= ~(264_VLC_RESET);
        #endif

        MBHeight    = pH264VLD->MB_HEIGHT;
        frameYSize  = pEnc->mb_num << 8;
        sliceLenBuf = (MMP_ULONG *)pH264ENC->H264ENC_SLICE_LEN_BUF_ADDR;
        sliceNumber = pH264ENC->H264ENC_CODED_SLICE_NUM;

        // Encode done
        if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        }
        else {
            // pop frame from ready queue and timestamp queue
            ulOffset = ((MMPF_3GPMGR_FRAME_TYPE_P == pEnc->cur_frm_type)? pEnc->b_frame_num: 0);
            qState = MMPF_VIDENC_PopQueue(&gVidRecdRdyQueue[InstId], ulOffset, &ulBufId, MMP_TRUE);
            if (qState == MMP_ERR_NONE) {
                // the ready frame is really poped from queue, set it as free one
                MMPF_VIDENC_PopQueue(&gVidRecdDtsQueue[InstId], 0, &ulDts, MMP_FALSE);
                MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[InstId], ulBufId, MMP_FALSE);
            }
            else if (qState == MMP_MP4VE_ERR_QUEUE_WEIGHTED) {
                // decrease the weighting
                qState = MMPF_VIDENC_PopQueue(&gVidRecdRdyQueue[InstId], ulOffset, &ulBufId, MMP_TRUE);
                // insert duplicated frame by container
                // TODO: How to indicated a dummy frame needed? by Alterman
                if (qState == MMP_ERR_NONE) {
                    // the ready frame is really poped from queue, set it as free one
                    MMPF_VIDENC_PopQueue(&gVidRecdDtsQueue[InstId], 0, &ulDts, MMP_FALSE);
                    MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[InstId], ulBufId, MMP_FALSE);
                }
            }
        }

        pEnc->module->bWorking = MMP_FALSE;

        frameSize   = 0;

        if (pH264VLD->DATA_INFO_REG & H264_I_SLICE_FLAG) {
            // hw sps/pps
            if (pH264ENC->H264ENC_VLC_CTRL2 & INSERT_SPS_PPS_BEFORE_I_SLICE) {
                pH264ENC->H264ENC_VLC_CTRL2 &= ~(INSERT_SPS_PPS_BEFORE_I_SLICE);
                sliceNumber += 2;
            }

            pEnc->gop_frame_num = 0;
        }

        #if (H264ENC_LBR_EN)
        if (pEnc->rc_config[0].rc_mode == VIDENC_RC_MODE_LOWBR) {
            if (pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_P)
                layer = 1;

            #if (H264ENC_LBR_FLOAT_RATIO)
            // Dynamically change bitrate ratio of layers
            if (pEnc->total_frames > 60) {

                if (pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_I) {
                    pEnc->ubAccResetLowBrRatioTimes  = 0;
                    pEnc->ubAccResetHighBrRatioTimes = 0;
                }
                else {
                    pEnc->intra_mb = pH264ENC->H264ENC_INTRAMB_NUM;

                    if (pEnc->intra_mb > 10) {
                        pEnc->ubAccResetLowBrRatioTimes++;
                    }
                    else if (pEnc->intra_mb < 5) {
                        pEnc->ubAccResetHighBrRatioTimes++;
                        pEnc->ubAccResetLowBrRatioTimes = 0;
                    }
                    else {
                        pEnc->ubAccResetLowBrRatioTimes = 0;
                    }
                }
            }
            #endif
        }
        #endif

        for (i = 0; i < sliceNumber; i++)
            frameSize += sliceLenBuf[i];

        //RTNA_DBG_Long(0, frameSize);
        //RTNA_DBG_Byte(0, layer);
        //RTNA_DBG_Str(0, "\r\n");
        if ((frameSize * 2) > pEnc->layer_frm_thr[layer])
            pEnc->layer_frm_thr[layer] = frameSize * 2;

		MMPF_VidRateCtl_UpdateModel (pEnc->layer_rc_hdl[layer],
			                         pEnc->cur_frm_type,
                                     frameSize,
                                     (pH264ENC->H264ENC_INTRAMB_BITS_COUNT + pH264ENC->H264ENC_MV_BITS_COUNT + pH264ENC->H264ENC_HEADER_BITS_COUNT)/8,
                                     (pH264ENC->H264ENC_QP_SUM / pEnc->mb_num),
                                     MMP_FALSE,
                                     &bSkipFrame,
                                     &pending_bytes);

        if (pEnc->co_mv_valid == MMP_FALSE) {
            if ((pEnc->cur_frm_type != MMPF_3GPMGR_FRAME_TYPE_I) &&
                !(pH264ENC->H264ENC_PMV_CTL & H264E_COLOCATE_MV_DIS))
            {
                pEnc->co_mv_valid = MMP_TRUE;
            }
        }

        #if (H264ENC_ICOMP_EN)
        if ((pH264ENC->H264ENC_ICOMP_OVERFLOW_FLAG_MP && H264ENC_ICOMP_Y_OVWR) == 1) {
            ulICompMbXOv = (pH264ENC->H264ENC_ICOMP_Y_MB_X_OVERFLOW_MP | 
                                ((pH264ENC->H264ENC_ICOMP_MSB_MB_LOC_MP & H264ENC_ICOMP_MSB_Y_MB_X_OVFLOW)<<8));
            ulICompMbYOv = (pH264ENC->H264ENC_ICOMP_Y_MB_Y_OVERFLOW_MP | 
                                ((pH264ENC->H264ENC_ICOMP_MSB_MB_LOC_MP & H264ENC_ICOMP_MSB_Y_MB_Y_OVFLOW)<<8));
            ulICompMbOffset = ulICompMbYOv * pEnc->mb_w + ulICompMbXOv;
        }
        if ((pH264ENC->H264ENC_ICOMP_OVERFLOW_FLAG_MP && H264ENC_ICOMP_CBCR_OVWR) == 2) {
            ulICompMbCbCrXOv = (pH264ENC->H264ENC_ICOMP_CBCR_MB_X_OVERFLOW_MP | 
                                ((pH264ENC->H264ENC_ICOMP_MSB_MB_LOC_MP & H264ENC_ICOMP_MSB_CBCR_MB_X_OVFLOW)<<8));
            ulICompMbCbCrYOv = (pH264ENC->H264ENC_ICOMP_CBCR_MB_Y_OVERFLOW_MP | 
                                ((pH264ENC->H264ENC_ICOMP_MSB_MB_LOC_MP & H264ENC_ICOMP_MSB_CBCR_MB_Y_OVFLOW)<<8));
            ulICompMbCbCrOffset = ulICompMbCbCrYOv * pEnc->mb_w * 2 + ulICompMbCbCrXOv * 2;
        }

        if (ulICompMbOffset > ulICompMbCbCrOffset)
            ulICompMbOffset = ulICompMbCbCrOffset;

        if (ulICompMbOffset)
            MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_MB, 1, ulICompMbOffset);
        else
            MMPF_H264ENC_SetIntraRefresh(MMPF_H264ENC_INTRA_REFRESH_DIS, 0, 0);
        #endif

        info.size = frameSize;
        info.time = pEnc->timestamp;
        info.type = pEnc->cur_frm_type;
    #if MIPI_TEST_PARAM_EN==1
    if (pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_P) {
        if(info.size >= psize_th) {
          printc("[MIPI.ERR]: P frame %d > Th %d\r\n",info.size,psize_th);
          //while(1);
          mipi_test_ng++ ;
        }  
        //printc("P[%d]\r\n",info.size);
    }
    else {
        //printc("I[%d]\r\n",info.size);  
    }
    #endif
        
		#if (WORK_AROUND_EP3)
        info.ep3  = m_bEP3BHappens;
        #endif
        MMPF_VStream_SaveFrameInfo(InstId, &info);

        pEnc->TotalEncodeSize += frameSize;
        pEnc->total_frames++;
        pEnc->gop_frame_num++;
        if (pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_P) {
            pEnc->prev_ref_num++;
        }
        if ((pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_I) ||
            (pEnc->cur_frm_type == MMPF_3GPMGR_FRAME_TYPE_P))
        {
            tmpFramePtr = pEnc->ref_frm;
            pEnc->ref_frm = pEnc->rec_frm;
            pEnc->rec_frm = tmpFramePtr;
        }

        #if (VID_CTL_DBG_BR)
        if (pEnc->br_base_time == 0) {
            pEnc->br_total_size  = 0;
            pEnc->br_base_time   = OSTime;
            pEnc->br_last_time   = pEnc->br_base_time;
        }
        else {
            pEnc->br_total_size += frameSize;
            pEnc->br_last_time   = OSTime;
        }
        #endif

        #if (H264ENC_RDO_EN)
        if (pEnc->bRdoEnable) {
            pEnc->bRdoMadValid = MMP_TRUE;
            pEnc->ulRdoMadFrmPre = pH264ENC->H264ENC_MAD;
            pEnc->ulRdoMadLastRowPre = (pH264ENC->H264ENC_LAST_MB_ROW_MAD & H264ENC_LAST_MB_ROW_MAD_MASK);
        }
        #endif

        MMPF_H264ENC_UpdateOperation(pEnc);

        if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_PAUSE) {
#if (WORK_AROUND_EP3)
            pH264ENC->H264ENC_INT_CPU_EN &= ~(FRAME_ENC_DONE_CPU_INT_EN | EP_BYTE_CPU_INT_EN);
#else
            pH264ENC->H264ENC_INT_CPU_EN &= ~(FRAME_ENC_DONE_CPU_INT_EN);
#endif
            pEnc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE;
            RTNA_DBG_Str3("H264ENC_ISR : pause\r\n");
        }
        if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) {

            MMPF_VIDENC_SetEncodeDisable(pEnc);
            if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) {
                RTNA_DBG_Str0("H264ENC_ISR:stop\r\n");
            }
            else {
                RTNA_DBG_Str0("H264ENC_ISR:MEDIA_FILE_OVERFLOW\r\n");
                MMPF_H264ENC_DeInitInstance(pEnc, InstId);
            }
        }
        else if (pEnc->GlbStatus.VidOp != VIDEO_RECORD_PAUSE) {
            /* Use 2ms timer causes latency. If encode bandwidth is tight (enc time > frame input), 
             * the latency may make current frames underflowed.
             * Trigger encode after encode done to eliminate the latency. */
            if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
                if (pEnc->EncEndCallback)
                    pEnc->EncEndCallback(pEnc);
                else
			        RTNA_DBG_Str(0,"\r\nError, H264-RT callback is NULL.");
            }
        }
        // trigger encode by task
        MMPF_OS_SetFlags(VID_REC_Flag, SYS_FLAG_VIDENC, MMPF_OS_FLAG_SET);
        // trigger streamer
        MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(pEnc->enc_id), MMPF_OS_FLAG_SET);
    }

#if (WORK_AROUND_EP3)
    if (status & EP_BYTE_CPU_INT_EN) {
        // Remove bWorking condition. Because H264 VLC reset is useless for high profile format. 
    	// IF EP3 occurred, EP3 interrupt will always interrupt system befer next frame trigger.
        //if (H264EncInfo->module->bWorking) {
        	// After EP3 insert happens, EP3 interrupt always happens. Disable interrupt here.
	        pH264ENC->H264ENC_INT_CPU_EN &= ~EP_BYTE_CPU_INT_EN; 
	        m_bEP3BHappens = MMP_TRUE;
        //}
    }
#endif
}

#if (H264ENC_TNR_EN)
/**
 @brief enable TNR of H264ENC.

 This function enables the TNR function and sets the parameters of TNR function.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_H264ENC_EnableTnr(MMPF_H264ENC_ENC_INFO *pEnc)
{
    AITPS_H264ENC pH264ENC = AITC_BASE_H264ENC;
    MMPF_H264ENC_TNR *tnr = &pEnc->tnr_ctl;

    if (pEnc->tnr_en) {
        if (pEnc->tnr_en & TNR_LOW_MV_EN)
            pH264ENC->TNR_ENABLE |= TNR_MCTF_ENABLE;
        else
            pH264ENC->TNR_ENABLE &= ~(TNR_MCTF_ENABLE);

        pH264ENC->TNR_MCTF_MVX_THR_LOW  = tnr->low_mv_x_thr & 0xff;
        pH264ENC->TNR_MCTF_MVX_THR_HIGH = (tnr->low_mv_x_thr >> 8) & 0xff;

        pH264ENC->TNR_MCTF_MVY_THR_LOW  = tnr->low_mv_y_thr & 0xff;
        pH264ENC->TNR_MCTF_MVY_THR_HIGH = (tnr->low_mv_y_thr >> 8) & 0xff;

        // SATD cost 65535
        pH264ENC->TNR_MCTF_SATD_THR_LOW  = 0xFF;
        pH264ENC->TNR_MCTF_SATD_THR_HIGH = 0xFF;

        pH264ENC->TNR_MCTF_FIL_PARA_LUMA_4X4 = tnr->low_mv_filter.luma_4x4;
        pH264ENC->TNR_MCTF_FIL_PARA_CHRO_4X4 = tnr->low_mv_filter.chroma_4x4;
        pH264ENC->TNR_MCTF_FIL_PARA_LUMA_8X8_LOW  = tnr->low_mv_filter.thr_8x8 & 0xff;
        pH264ENC->TNR_MCTF_FIL_PARA_LUMA_8X8_HIGH = (tnr->low_mv_filter.thr_8x8 >> 8) & 0xff;

        if (pEnc->tnr_en & TNR_ZERO_MV_EN)
            pH264ENC->TNR_ENABLE |= TNR_ZERO_MV_FUNC_ENABLE;
        else
            pH264ENC->TNR_ENABLE &= ~(TNR_ZERO_MV_FUNC_ENABLE);

        pH264ENC->TNR_ZERO_MV_LUMA_PIXEL_DIFF_THR = tnr->zero_luma_pxl_diff_thr;
        pH264ENC->TNR_ZERO_MV_CHRO_PIXEL_DIFF_THR = tnr->zero_chroma_pxl_diff_thr;

        pH264ENC->TNR_ZERO_MV_THR_4X4 = tnr->zero_mv_4x4_cnt_thr;
        pH264ENC->TNR_ZERO_MVX_THR_LOW  = tnr->zero_mv_x_thr & 0xff;
        pH264ENC->TNR_ZERO_MVX_THR_HIGH = (tnr->zero_mv_x_thr >> 8) & 0xff;
        pH264ENC->TNR_ZERO_MVY_THR_LOW  = tnr->zero_mv_y_thr & 0xff;
        pH264ENC->TNR_ZERO_MVY_THR_HIGH = (tnr->zero_mv_y_thr >> 8) & 0xff;

        pH264ENC->TNR_ZERO_MV_FIL_PARA_LUMA_4X4 = tnr->zero_mv_filter.luma_4x4;
        pH264ENC->TNR_ZERO_MV_FIL_PARA_CHRO_4X4 = tnr->zero_mv_filter.chroma_4x4;
        pH264ENC->TNR_ZERO_MV_FIL_PARA_LUMA_8X8_LOW  = tnr->zero_mv_filter.thr_8x8 & 0xff;
        pH264ENC->TNR_ZERO_MV_FIL_PARA_LUMA_8X8_HIGH = (tnr->zero_mv_filter.thr_8x8 >> 8) & 0xff;

        if (pEnc->tnr_en &TNR_HIGH_MV_EN)
            pH264ENC->TNR_ENABLE |= TNR_INTRA_MB_FUNC_ENABLE;
        else
            pH264ENC->TNR_ENABLE &= ~(TNR_INTRA_MB_FUNC_ENABLE);

        pH264ENC->TNR_INTRA_MB_FIL_PARA_LUMA_4X4 = tnr->high_mv_filter.luma_4x4;
        pH264ENC->TNR_INTRA_MB_FIL_PARA_CHRO_4X4 = tnr->high_mv_filter.chroma_4x4;
        pH264ENC->TNR_INTRA_MB_FIL_PARA_LUMA_8X8_LOW  = tnr->high_mv_filter.thr_8x8 & 0xff;
        pH264ENC->TNR_INTRA_MB_FIL_PARA_LUMA_8X8_HIGH = (tnr->high_mv_filter.thr_8x8 >> 8) & 0xff;   
    }
    else {
        pH264ENC->TNR_ENABLE = 0;
    }

    return MMP_ERR_NONE;
}
#endif

#if (H264ENC_RDO_EN)
void MMPF_H264ENC_EnableRdo(MMPF_H264ENC_ENC_INFO *pEnc)
{
    AITPS_H264ENC pH264ENC = AITC_BASE_H264ENC;

    pH264ENC->H264ENC_QSTEP_3[0] = pEnc->qstep3[0] | (pEnc->qstep3[1] << 4);
    pH264ENC->H264ENC_QSTEP_3[1] = pEnc->qstep3[2] | (pEnc->qstep3[3] << 4);
    pH264ENC->H264ENC_QSTEP_3[2] = pEnc->qstep3[4] | (pEnc->qstep3[5] << 4);
    pH264ENC->H264ENC_QSTEP_3[3] = pEnc->qstep3[6] | (pEnc->qstep3[7] << 4);
    pH264ENC->H264ENC_QSTEP_3[4] = pEnc->qstep3[8] | (pEnc->qstep3[9] << 4);
}
#endif

MMP_ERR MMPF_H264ENC_InitImageComp(MMPF_H264ENC_ENC_INFO *pEnc, MMPF_H264ENC_ICOMP *pConfig)
{
#if (H264ENC_ICOMP_EN)
    AITPS_H264ENC pH264ENC = AITC_BASE_H264ENC;
    MMP_UBYTE i, ubLsyRatio, ubLsyRatioCr;
    MMP_ULONG ulBgtMbRow, ulBgtMbRowCr;
    MMP_ULONG ulLvlIncThr, ulLvlIncThrCr;
    MMP_ULONG ulLvlDecThr, ulLvlDecThrCr;

    pH264ENC->H264ENC_INT_CPU_SR |= ICOMP_LUMA_OVLP_INT | ICOMP_CHROMA_OVLP_INT;
    pH264ENC->H264ENC_INT_CPU_EN |= ICOMP_LUMA_OVLP_INT | ICOMP_CHROMA_OVLP_INT;

    if (pConfig->ubICompRatio == 100) {
        ubLsyRatio = 100;
        ubLsyRatioCr = 100;
    } else if (pConfig->ubICompRatio >= 50) {
        ubLsyRatio = (63*pConfig->ubICompRatio)/50;
        ubLsyRatioCr = (24*pConfig->ubICompRatio)/50;
    } else {
        ubLsyRatio = (60*pConfig->ubICompRatio)/45;
        ubLsyRatioCr = (20*pConfig->ubICompRatio)/45;
    }

    if ((pConfig->ubICompRatio < 100) && (pConfig->bICompLsyLvlCtlEn)) {
        if (pConfig->ubICompRatio >= 55)
            pConfig->ubICompRatioIndex = 0;
        else if (pConfig->ubICompRatio >= 50)
            pConfig->ubICompRatioIndex = 2;
        else if (pConfig->ubICompRatio >= 45)
            pConfig->ubICompRatioIndex = 4;
        else if (pConfig->ubICompRatio >= 40)
            pConfig->ubICompRatioIndex = 6;
        else if (pConfig->ubICompRatio >= 39)
            pConfig->ubICompRatioIndex = 8;
        else if (pConfig->ubICompRatio >= 35)
            pConfig->ubICompRatioIndex = 10;
        else if (pConfig->ubICompRatio >= 30)
            pConfig->ubICompRatioIndex = 12;
        else
            pConfig->ubICompRatioIndex = 14;
    } else {
        pConfig->ubICompRatioIndex = 0;
    }

    if (pConfig->bICompLsyLvlCtlEn) {
        ulBgtMbRow = ((pEnc->mb_w << 4) * 16 * ubLsyRatio)/100;
        ulBgtMbRowCr = ((pEnc->mb_w << 4)/2 * 8 * 2 * ubLsyRatioCr)/100;

        ulLvlIncThr = ulBgtMbRow;
        ulLvlIncThrCr = ulBgtMbRowCr;
        ulLvlDecThr = ulBgtMbRow*31/32;
        ulLvlDecThrCr = ulBgtMbRowCr*31/32;
    }

    if (pConfig->bICompEnable == MMP_TRUE) {
        //Lossless compression
        pH264ENC->H264ENC_ICOMP_CTL_MP |= H264E_ICOMP_ENABLE;
        //overfolw budget
        pH264ENC->H264ENC_ICOMP_Y_OVFLOW_BGT_MP = pConfig->ulICompFrmSize;
        pH264ENC->H264ENC_ICOMP_CBCR_OVFLOW_BGT_MP = pConfig->ulICompFrmSize/2;
        //decompression frame length
        pH264ENC->H264ENC_ICOMP_DECOMP_Y_LENGTH_MP = pConfig->ulICompFrmSize;
        pH264ENC->H264ENC_ICOMP_DECOMP_CBCR_LENGTH_MP = pConfig->ulICompFrmSize/2;

        //Lossy compression
        if (pConfig->bICompCurMbLsyEn) {
            pH264ENC->H264ENC_ICOMP_CTL_MP |= H264E_ICOMP_CUR_MB_LSY_EN;

            //setting level //min, ini
            pH264ENC->H264ENC_ICOMP_LSY_LVL_MP = (pConfig->ubICompMinLsyLvlLum & H264E_ICOMP_Y_MIN_LSY_LVL_MSK);
            pH264ENC->H264ENC_ICOMP_LSY_LVL_MP |= ((pConfig->ubICompMinLsyLvlChr << 3) & H264E_ICOMP_CBCR_MIN_LSY_LVL_MSK);
            pH264ENC->H264ENC_ICOMP_LSY_LVL_MP |= ((pConfig->ubICompIniLsyLvlLum << 6) & H264E_ICOMP_Y_INI_LSY_LVL_MSK);
            pH264ENC->H264ENC_ICOMP_LSY_LVL_MP |= ((pConfig->ubICompIniLsyLvlChr << 9) & H264E_ICOMP_CBCR_INI_LSY_LVL_MSK);
            //blk size
            pH264ENC->H264ENC_ICOMP_Y_LSY_LVL_TABLE_MP =
                (blk_lsy_tbl[pConfig->ubICompRatioIndex][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_BLK_SIZE_IDX]
                 & H264E_ICOMP_Y_LSY_BLK_SIZE_SEL);
            pH264ENC->H264ENC_ICOMP_CBCR_LSY_LVL_TABLE_MP =
                (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_BLK_SIZE_IDX] 
                 & H264E_ICOMP_CBCR_LSY_BLK_SIZE_SEL);
            //decide yuvrange
            pH264ENC->H264ENC_ICOMP_Y_LSY_LVL_TABLE_MP |=
                ((blk_lsy_tbl[pConfig->ubICompRatioIndex][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_YUVRNG_IDX]
                 << ICOMP_LSY_YUVRNG_OFFSET) & H264E_ICOMP_Y_LSY_YUVRNG_MSK);
            pH264ENC->H264ENC_ICOMP_CBCR_LSY_LVL_TABLE_MP |=
                ((blk_lsy_tbl[pConfig->ubICompRatioIndex+1][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_YUVRNG_IDX]
                 << ICOMP_LSY_YUVRNG_OFFSET) & H264E_ICOMP_CBCR_LSY_YUVRNG_MSK);
            //decide max level
            pH264ENC->H264ENC_ICOMP_Y_LSY_LVL_TABLE_MP |=
            ((blk_lsy_tbl[pConfig->ubICompRatioIndex][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_MAX_LVL_IDX]
                 << ICOMP_LSY_MAX_LVL_OFFSET) & H264E_ICOMP_Y_LSY_LVL_MAX_MSK);
            pH264ENC->H264ENC_ICOMP_CBCR_LSY_LVL_TABLE_MP |=
            ((blk_lsy_tbl[pConfig->ubICompRatioIndex+1][ICOMP_LSY_STATIC_REDUCTION_IDX][ICOMP_LSY_MAX_LVL_IDX]
                 << ICOMP_LSY_MAX_LVL_OFFSET) & H264E_ICOMP_CBCR_LSY_LVL_MAX_MSK);

            for (i = 0; i < 8; i++) {
                //decide mean flag 
                pH264ENC->H264ENC_ICOMP_Y_MEAN_MP |=
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex][i][ICOMP_LSY_MEAN_OFFSET] << i);
                pH264ENC->H264ENC_ICOMP_CBCR_MEAN_MP |=
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][i][ICOMP_LSY_MEAN_OFFSET] << i);
            }

            for (i = 0; i < 4; i++) {
                //decide contrast
                pH264ENC->H264ENC_ICOMP_Y_CONTRAST_MP[i] = 
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex][i*2][ICOMP_LSY_CONTRAST_OFFSET] | 
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex][i*2+1][ICOMP_LSY_CONTRAST_OFFSET] << 4));
                pH264ENC->H264ENC_ICOMP_CBCR_CONTRAST_MP[i] =  
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][i*2][ICOMP_LSY_CONTRAST_OFFSET] | 
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][i*2+1][ICOMP_LSY_CONTRAST_OFFSET] << 4));
                //decide different threshold
                pH264ENC->H264ENC_ICOMP_Y_DIFF_THR_MP[i] =  
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex][i*2][ICOMP_LSY_DIFF_THR_OFFSET] | 
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex][i*2+1][ICOMP_LSY_DIFF_THR_OFFSET] << 4));
                pH264ENC->H264ENC_ICOMP_CBCR_DIFF_THR_MP[i] =  
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][i*2][ICOMP_LSY_DIFF_THR_OFFSET] | 
                    (blk_lsy_tbl[pConfig->ubICompRatioIndex+1][i*2+1][ICOMP_LSY_DIFF_THR_OFFSET] << 4));
            }

            //Dynamic lossy level control compression
            if (pConfig->bICompLsyLvlCtlEn) {
                pH264ENC->H264ENC_ICOMP_CTL_MP |= H264E_ICOMP_LSY_LVL_CTL_EN;

                //icomp lossy mb row budget average for one frame
                //(frame resolution * dram reduction ratio) / number of mb row
                pH264ENC->H264ENC_ICOMP_LSY_Y_MB_ROW_BGT_MP = ulBgtMbRow;
                pH264ENC->H264ENC_ICOMP_LSY_CBCR_MB_ROW_BGT_MP = ulBgtMbRowCr;
                //icomp lossy level increase threshold
                pH264ENC->H264ENC_ICOMP_Y_LSY_LVL_INCR_THR_MP = ulLvlIncThr;
                pH264ENC->H264ENC_ICOMP_CBCR_LSY_LVL_INCR_THR_MP = ulLvlIncThrCr;
                //icomp lossy level decrease threshold
                //(frame resolution * (dram reduction ratio - 3%)) / number of mb row
                pH264ENC->H264ENC_ICOMP_Y_LSY_LVL_DECR_THR_MP = ulLvlDecThr;
                pH264ENC->H264ENC_ICOMP_CBCR_LSY_LVL_DECR_THR_MP = ulLvlDecThrCr;
            } else {
                pH264ENC->H264ENC_ICOMP_CTL_MP &= ~(H264E_ICOMP_LSY_LVL_CTL_EN);
            }
        } else {
            pH264ENC->H264ENC_ICOMP_CTL_MP &= ~(H264E_ICOMP_CUR_MB_LSY_EN);
        }
    }
    else {
        pH264ENC->H264ENC_ICOMP_CTL_MP &= ~(H264E_ICOMP_ENABLE|H264E_ICOMP_LSY_LVL_CTL_EN|H264E_ICOMP_CUR_MB_LSY_EN);
    }
#endif
    return MMP_ERR_NONE;
}

#endif //(VIDEO_R_EN)
