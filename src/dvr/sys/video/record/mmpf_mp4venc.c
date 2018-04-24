/**
 @file mmpf_mp4venc.c
 @brief Control functions of video encoder
 @author Will Tseng
 @version 1.0
*/

#include "includes_fw.h"
#if (VIDEO_R_EN)
#include "lib_retina.h"
#include "mmp_reg_h264enc.h"
#include "mmp_reg_h264dec.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_ibc.h"

#include "mmpf_sensor.h"
#include "mmpf_3gpmgr.h"
#include "mmpf_mp4venc.h"
#include "mmpf_h264enc.h"
#include "mmpf_audio_ctl.h"
#include "mmpf_display.h"
#include "mmpf_scaler.h"
#include "mmpf_timer.h"
#include "mmpf_ibc.h"
#include "mmpf_rawproc.h"
#if (HANDLE_EVENT_BY_TASK == 1)
#include "mmpf_event.h"
#endif
#if (FPS_CTL_EN)
#include "mmpf_fpsctl.h"
#endif
#include "mmpf_dram.h"
#include "mmpf_system.h"
#if (CODE_FOR_DEMO)
#include "mmpf_pio.h"
#endif
#include "mmph_hif.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

MMP_USHORT          gsVidRecdCurBufMode;		///< current buf mode of video encode.

MMPF_VIDENC_QUEUE   gVidRecdRdyQueue[MAX_VIDEO_STREAM_NUM];    ///< queue for cur buf management
MMPF_VIDENC_QUEUE   gVidRecdFreeQueue[MAX_VIDEO_STREAM_NUM];   ///< queue for cur buf management
MMPF_VIDENC_QUEUE   gVidRecdDtsQueue[MAX_VIDEO_STREAM_NUM];    ///< stores frame time stamp

MMP_USHORT          m_usVidRecPath[MAX_VIDEO_STREAM_NUM];

static MMPF_VIDENC_INSTANCE m_VidInstance[MAX_VIDEO_STREAM_NUM];
static MMPF_VIDENC_MODULE   m_VidModule;
volatile MMP_BOOL			m_bRtCurBufOvlp = MMP_FALSE;
volatile MMP_BOOL			m_bRtScaleDFT = MMP_FALSE;          ///< Check Scaler double frme start

MMP_BOOL            gbRtEncDropFrameEn = MMP_FALSE;

extern MMP_BOOL     gbIBCReady[];
extern MMP_UBYTE    gbIBCLinkEncId[MMP_IBC_PIPE_MAX];

extern MMPF_OS_FLAGID   VID_REC_Flag;
extern MMPF_OS_FLAGID 	DSC_UI_Flag;

//==============================================================================
//
//                              FUNCTIONS PROTOTYPE
//
//==============================================================================

extern void MMPF_MMU_FlushDCacheMVA(MMP_ULONG ulRegion, MMP_ULONG ulSize);

extern void rt_enc_end_init_callback(void* arg);
extern void rt_enc_restart_callback(void* arg);
extern void rt_enc_scaledfs_restart_callback(void* arg);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

MMPF_VIDENC_MODULE *MMPF_VIDENC_GetModule(void)
{
	return &m_VidModule;
}

MMP_BOOL MMPF_VIDENC_IsModuleInit(void)
{
	return m_VidModule.bInitMod;
}

MMP_ERR MMPF_VIDENC_InitModule(void)
{
    AITPS_AIC           pAIC = AITC_BASE_AIC;
	MMPF_VIDENC_MODULE  *pMod = &m_VidModule;

    // Check whether module is already referred
	if (pMod->ubRefCnt++)
		return MMP_ERR_NONE;

    // Enable module clock
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_H264, MMP_TRUE);

    // Initialize module
    if (MMPF_H264ENC_InitModule(&(pMod->H264EMod)) != MMP_ERR_NONE) {
		return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
	}

    RTNA_AIC_Open(pAIC, AIC_SRC_H264, h264enc_isr_a,
                  AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_H264);

	pMod->bInitMod  = MMP_TRUE;
	pMod->Format    = MMPF_MP4VENC_FORMAT_H264;

    //RTNA_DBG_Str0("# InitModule\r\n");

	return MMP_ERR_NONE;
}

MMP_ERR MMPF_VIDENC_DeinitModule(void)
{
    MMPF_VIDENC_MODULE  *pMod = &m_VidModule;
    MMP_ERR             status = MMP_ERR_NONE;

    if (!pMod->bInitMod) {
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

    pMod->ubRefCnt--;
    // Check whether module is still referred
    if (pMod->ubRefCnt)
        return MMP_ERR_NONE;

    status = MMPF_H264ENC_DeinitModule(&pMod->H264EMod);
    if (status != MMP_ERR_NONE)
        return MMP_MP4VE_ERR_WRONG_STATE_OP;

    pMod->bInitMod = MMP_FALSE;

    // Disable module clock
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_H264, MMP_FALSE);
    RTNA_DBG_Str0("# DeInitModule\r\n");

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_VIDENC_InitInstance(MMP_ULONG *InstId)
{
	MMP_ULONG               inst_id;
	MMPF_VIDENC_INSTANCE    *Inst;
	MMPF_VIDENC_MODULE      *pMod;
    MMP_ERR                 status = MMP_ERR_NONE;

    if (MMPF_VIDENC_InitModule())
        return MMP_MP4VE_ERR_WRONG_STATE_OP;

	pMod = &m_VidModule;

	for (inst_id = 0; inst_id < MAX_VIDEO_STREAM_NUM; inst_id++) {
		Inst = MMPF_VIDENC_GetInstance(inst_id);
		if (!Inst->bInitInst) {
			break;
		}
	}
	if (inst_id >= MAX_VIDEO_STREAM_NUM) {
		return MMP_MP4VE_ERR_WRONG_STATE_OP;
	}
	*InstId = inst_id;
	
  //  printc("# InitInstance: %d\r\n", inst_id);

	status = MMPF_H264ENC_InitInstance(&(Inst->h264e), &(pMod->H264EMod), Inst);
    if (status == MMP_ERR_NONE) {
        Inst->bInitInst = MMP_TRUE;
        Inst->Module = pMod;
    }
	return status;
}

MMP_ERR MMPF_VIDENC_DeInitInstance(MMP_ULONG InstId)
{
    MMP_ERR				 status = MMP_ERR_NONE;
    MMPF_VIDENC_INSTANCE *Inst = MMPF_VIDENC_GetInstance(InstId);

    if (!Inst->bInitInst)
        return MMP_MP4VE_ERR_WRONG_STATE_OP;

    printc("# DeInitInstance: %d\r\n", InstId);

    status = MMPF_H264ENC_DeInitInstance(&(Inst->h264e), InstId);
    if (status == MMP_ERR_NONE) {
        Inst->Module    = NULL;
        Inst->bInitInst = MMP_FALSE;
        Inst->preEncExec= NULL ;
        Inst->preEncArgc= 0;
        
    }
    MMPF_VIDENC_DeinitModule();

    return status;
}

/**
 @brief Keep the IBC pipe which output frame for encoder
 @param[in] usIBCPipe. pipe number
*/
void MMPF_VIDENC_SetVideoPath(MMP_USHORT usIBCPipe)
{
	m_usVidRecPath[gbIBCLinkEncId[usIBCPipe]] = usIBCPipe;
}

/** @brief Returns pointers to videnc instance structure
 @param[in] InstId instance ID of encoder to get handle
 @retval MMP_ERR_NONE Success.
 */
MMPF_VIDENC_INSTANCE * MMPF_VIDENC_GetInstance (MMP_UBYTE InstId)
{
	if (InstId >= MAX_VIDEO_STREAM_NUM) {
		RTNA_DBG_Str(0, "\r\nGetInstance is wrong, id:");
		RTNA_DBG_Byte(0, InstId);
	}	

	return (InstId < MAX_VIDEO_STREAM_NUM)? &(m_VidInstance[InstId])
			: &(m_VidInstance[0]);
}

/** @brief Register callback before encode
 @param[in] InstId instance ID of encoder to get handle
 @param[in] cb callback function
 @param[in] cb_argc callback function input parameter
 @retval MMP_ERR_NONE Success.
 */

MMP_ERR MMPF_VIDENC_RegisterPreEncodeCallBack(MMP_UBYTE InstId,MMPF_VIDENC_Callback *cb,void *cb_argc)
{
    MMPF_VIDENC_INSTANCE *pInst = MMPF_VIDENC_GetInstance(InstId) ;
    if(pInst) {
        pInst->preEncExec = cb ; 
        pInst->preEncArgc = cb_argc ;
        printc("#Registered Pre-encode callback:%x,%d\r\n",cb,cb_argc);
    }
    return MMP_ERR_NONE ;
}

/**
 @brief Check if queue is empty
 @param[in] queue. current queue
 @param[in] weighted. get number of elements or total weighting of all elements.
*/
MMP_ULONG MMPF_VIDENC_GetQueueDepth (MMPF_VIDENC_QUEUE *queue, MMP_BOOL weighted)
{
    if (weighted)
        return queue->weighted_size;
    else
        return queue->size;
}

/**
 @brief Shows queue element by offset
 @param[in] queue. current queue
 @param[in] offset. offset of element or accumlated weighting in queue to read
 @param[out] data. data of the specified element
 @param[in] weighted. if the queue uses weighting for each element
*/
MMP_ERR MMPF_VIDENC_ShowQueue(MMPF_VIDENC_QUEUE *queue, MMP_ULONG offset, MMP_ULONG *data, MMP_BOOL weighted)
{
    MMP_ULONG target, acc_weight;

    if (weighted) {
        // for weighted queue, get the element whose accumlated weighting of
        // all preceding elements large than the specified offset
        if (offset >= queue->weighted_size) {
            DBG_S(0, "show Q invalid offset\r\n");
            return MMP_MP4VE_ERR_ARRAY_IDX_OUT_OF_BOUND;
        }
        else if (queue->weighted_size == 0) {
            DBG_S(0, "show Q underflow\r\n");
            return MMP_MP4VE_ERR_QUEUE_UNDERFLOW;
        }

        target = queue->head;
        acc_weight = queue->weight[target];
        while(offset >= acc_weight) {
            acc_weight += queue->weight[target++];
            if (target >= MMPF_VIDENC_MAX_QUEUE_SIZE)
                target -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        }
    }
    else {
        if (offset >= queue->size) {
            DBG_S(0, "show Q invalid offset\r\n");
            return MMP_MP4VE_ERR_ARRAY_IDX_OUT_OF_BOUND;
        }
        else if (queue->size == 0) {
            DBG_S(0, "show Q underflow\r\n");
            return MMP_MP4VE_ERR_QUEUE_UNDERFLOW;
        }

        target = queue->head + offset;
        if (target >= MMPF_VIDENC_MAX_QUEUE_SIZE) {
            target -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        }
    }

    *data = queue->buffers[target];
    if (weighted && (queue->weight[target] > 1))
        return MMP_MP4VE_ERR_QUEUE_WEIGHTED;

    return MMP_ERR_NONE;
}

/**
 @brief Reset and clear queue
 @param[in] queue. current queue
*/
void MMPF_VIDENC_ResetQueue(MMPF_VIDENC_QUEUE *queue)
{
    queue->head = 0;
    queue->size = 0;
    queue->weighted_size = 0;
}

/**
 @brief Push 1 element to end of queue
 @param[in] queue. current queue
 @param[in] buffer. enqueue data
 @param[in] weighted. enqueue with one more element or increase the weighting of last element
*/
MMP_ERR MMPF_VIDENC_PushQueue(MMPF_VIDENC_QUEUE *queue, MMP_ULONG buffer, MMP_BOOL weighted)
{
    MMP_ULONG idx;

    queue->weighted_size++;
    
    if (queue->size >= MMPF_VIDENC_MAX_QUEUE_SIZE) {
        RTNA_DBG_Byte(0, weighted);
        RTNA_DBG_Long(0, queue->weighted_size);
        RTNA_DBG_Str(0, " #Q full\r\n");
        // increase the weighting of the last frame in queue
        idx = queue->head + queue->size - 1;
        if (idx >= MMPF_VIDENC_MAX_QUEUE_SIZE)
            idx -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        queue->weight[idx]++;
        return MMP_MP4VE_ERR_QUEUE_OVERFLOW;
    }

    if (weighted) {
        // to increase the weighting of last element in queue, the queue must not be empty
        if (queue->size == 0) {
            DBG_S(0, "weighted Q underflow\r\n");
            return MMP_MP4VE_ERR_QUEUE_UNDERFLOW;
        }
        idx = queue->head + queue->size - 1;
        if (idx >= MMPF_VIDENC_MAX_QUEUE_SIZE)
            idx -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        queue->weight[idx]++;
        // don't change the buffer value
        RTNA_DBG_Str(0, "Q weight++\r\n");
        return MMP_ERR_NONE;
    }
    else {
        idx = queue->head + queue->size++;
        if (idx >= MMPF_VIDENC_MAX_QUEUE_SIZE)
            idx -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        queue->weight[idx] = 1;
    }

    queue->buffers[idx] = buffer;

    return MMP_ERR_NONE;
}

/**
 @brief Pop 1 element from queue
 @param[in] queue. current queue
 @param[in] offset. the offset of queue element to pop or the offset of acc weighting to pop
 @param[out] data. data queued in the specified offset
 @param[in] weighted. if the queue uses weighting for each element
*/
MMP_ERR MMPF_VIDENC_PopQueue(MMPF_VIDENC_QUEUE *queue, MMP_ULONG offset, MMP_ULONG *data, MMP_BOOL weighted)
{
    MMP_ULONG target, source, acc_weight;

    if (weighted) {
        // for weighted queue, pop operation selects the element with 
        // accumlated weighting of all preceding elements equal to the specified offset
        if (offset >= queue->weighted_size) {
            DBG_S(0, "pop Q invalid offset\r\n");
            return MMP_MP4VE_ERR_ARRAY_IDX_OUT_OF_BOUND;
        }
        else if (queue->weighted_size == 0) {
            DBG_S(0, "pop Q underflow\r\n");
            return MMP_MP4VE_ERR_QUEUE_UNDERFLOW;
        }

        target = queue->head;
        acc_weight = queue->weight[target];
        while(offset >= acc_weight) {
            acc_weight += queue->weight[target++];
            if (target >= MMPF_VIDENC_MAX_QUEUE_SIZE)
                target -= MMPF_VIDENC_MAX_QUEUE_SIZE;
        }
    }
    else {
        // for non-weighted queue, pop the element with the specified offset
        if (offset >= queue->size) {
            DBG_S(0, "pop Q invalid offset\r\n");
            return MMP_MP4VE_ERR_ARRAY_IDX_OUT_OF_BOUND;
        }
        else if (queue->size == 0) {
            DBG_S(0, "pop Q underflow\r\n");
            return MMP_MP4VE_ERR_QUEUE_UNDERFLOW;
        }

        target = queue->head + offset;
        if (target >= MMPF_VIDENC_MAX_QUEUE_SIZE)
            target -= MMPF_VIDENC_MAX_QUEUE_SIZE;
    }

    queue->weighted_size--;

    *data = queue->buffers[target];

    // if queue is weighted and the weighting value is not zero,
    // pop queue will just decrease the weighting, instead of really dequeue.
    if (weighted && (queue->weight[target] > 1)) {
        queue->weight[target]--;
        return MMP_MP4VE_ERR_QUEUE_WEIGHTED;
    }

    while (target != queue->head) { //shift elements before offset
        source = (target)? (target-1): (MMPF_VIDENC_MAX_QUEUE_SIZE-1);
        queue->buffers[target] = queue->buffers[source];
        queue->weight[target] = queue->weight[source];
        target = source;
    }
    queue->size--;

    queue->head++;
    if (queue->head >= MMPF_VIDENC_MAX_QUEUE_SIZE) {
        queue->head -= MMPF_VIDENC_MAX_QUEUE_SIZE;
    }

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_VIDENC_GetParameter(MMP_UBYTE ubEncId, MMPF_VIDENC_ATTRIBUTE attrib, void *arg)
{
	MMPF_VIDENC_INSTANCE *pInst = MMPF_VIDENC_GetInstance(ubEncId);

	switch (get_vidinst_format(pInst)) {
	case MMPF_MP4VENC_FORMAT_H264:
		return MMPF_H264ENC_GetParameter(&(pInst->h264e), attrib, arg);
	default:
		return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
	};
}

/**
 @brief set input frame buffer timestamp

 This function set frame timestamp in frame info list
 @retval none
*/
MMP_ERR MMPF_VIDENC_SetFrameTimestamp(MMPF_H264ENC_ENC_INFO *pEnc, MMP_UBYTE ubBufId, MMP_ULONG ulTimestamp)
{
	pEnc->cur_frm[ubBufId].ulTimestamp = ulTimestamp;

    return MMP_ERR_NONE;
}

/**
 @brief set input frame buffer address

 This function configs input frame buffer address
 @retval none
*/
void MMPF_VIDENC_SetFrameInfoList(MMP_UBYTE ubEncID, MMP_ULONG ulYBuf[], MMP_ULONG ulUBuf[], MMP_ULONG ulVBuf[], MMP_UBYTE ubBufCnt)
{
    MMP_ULONG   i;
	MMPF_VIDENC_INSTANCE  *pInst = MMPF_VIDENC_GetInstance(ubEncID);
	MMPF_H264ENC_ENC_INFO *pEnc = &(pInst->h264e);

    for (i = 0; i < ubBufCnt; i++) {
		pEnc->cur_frm[i].ulTimestamp = 0;
		pEnc->cur_frm[i].ulYAddr = ulYBuf[i];
		pEnc->cur_frm[i].ulUAddr = ulUBuf[i];
		pEnc->cur_frm[i].ulVAddr = ulVBuf[i];
    }
}

MMPF_3GPMGR_FRAME_TYPE MMPF_VIDENC_GetVidRecdFrameType(MMPF_H264ENC_ENC_INFO *pEnc)
{
	MMP_ULONG ulLocalFrameNum = 0;

	if (pEnc->gop_size)
/* #if (USE_DIV_CONST) */
		/* ulLocalFrameNum = pEnc->gop_frame_num % 300; */
/* #else */
		ulLocalFrameNum = pEnc->gop_frame_num % pEnc->gop_size;
/* #endif */

	if (ulLocalFrameNum == 0) {
    	#if H264ENC_GDR_EN
	    if( pEnc->intra_refresh_en ) {
	        pEnc->intra_refresh_offset = 0 ;
	        pEnc->intra_refresh_trigger = MMP_TRUE ;
	        return MMPF_3GPMGR_FRAME_TYPE_P ;
	    }
	    #endif
		return MMPF_3GPMGR_FRAME_TYPE_I;
	}
	else {
/* #if (USE_DIV_CONST) */
		/* if ((ulLocalFrameNum-1) % (1+ 0 )) */
/* #else */
		if ((ulLocalFrameNum-1) % (1+pEnc->b_frame_num))
/* #endif */
			return MMPF_3GPMGR_FRAME_TYPE_B;
		else
			return MMPF_3GPMGR_FRAME_TYPE_P;
	}
}

/**
 @brief Returns total video encoded size

 This function returns total video encoded size
 @retval total size encoder generated
*/
MMP_ULONG MMPF_VIDENC_GetTotalEncodeSize(MMP_ULONG ulEncId)
{
	MMPF_H264ENC_ENC_INFO *pEnc = MMPF_H264ENC_GetHandle((MMP_UBYTE)ulEncId);

    return pEnc->TotalEncodeSize;
}

/**
 @brief Returns the threshold to skip frame encoding

 This function returns the threshold in unit of byte to skip encoding
 @retval Threshold in byte
*/
MMP_ULONG MMPF_VIDENC_GetSkipThreshold(MMP_ULONG ulEncId)
{
    MMP_ULONG layer, thr = 0;
    MMPF_H264ENC_ENC_INFO *pEnc = MMPF_H264ENC_GetHandle((MMP_UBYTE)ulEncId);

    for(layer = 0; layer < pEnc->total_layers; layer++) {
        if (pEnc->layer_frm_thr[layer] > thr)
            thr = pEnc->layer_frm_thr[layer];
    }

    return thr;
}

/**
 @brief Reset encoder state

 This function resets the encoder state
 @retval none.
*/
static void MMPF_VIDENC_ResetEncodeState(MMPF_H264ENC_ENC_INFO *pEnc)
{
	pEnc->TotalEncodeSize	   = 0;
	pEnc->prev_ref_num		   = 0;
	pEnc->total_frames		   = 0;
    pEnc->gop_frame_num        = 0;
	pEnc->GlbStatus.VidStatus  = MMPF_MP4VENC_FW_STATUS_RECORD;

	pEnc->dummydropinfo.ulDummyFrmCnt		= 0;
	pEnc->dummydropinfo.ulDropFrmCnt		= 0;
	pEnc->dummydropinfo.usAccumSkipFrames   = 0;
	pEnc->dummydropinfo.ulBeginSkipTimeInMS = 0;

    #if (VID_CTL_DBG_BR)
    pEnc->br_total_size = 0;
    pEnc->br_base_time  = 0;
    pEnc->br_last_time  = 0;
    #endif
}

/**
 @brief Enable video, audio encode

 This function initial video related paramters
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_SetEncodeEnable(MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ERR				stat		= MMP_ERR_NONE;
	MMP_UBYTE			ubEncID		= get_vidinst_id(pEnc->priv_data);

	if ((pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_RECORD) &&
		(pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_STOP)) {
        RTNA_DBG_Str(0, "VIDENC_ENC_ENABLE wrong state op\r\n");
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

    stat = MMPF_H264ENC_Open(pEnc);
    if (stat != MMP_ERR_NONE) {
        return stat;
    }

	MMPF_VIDENC_ResetQueue(&gVidRecdRdyQueue[ubEncID]);
	MMPF_VIDENC_ResetQueue(&gVidRecdDtsQueue[ubEncID]);
    
	MMPF_VIDENC_ResetEncodeState(pEnc);

    return MMP_ERR_NONE;
}

/**
 @brief post process for stop operation

 This function clears : interrupt enables, module setting by invoke driver API,
 clear buffer queue, clear cropsetting, send stop command to audio task, and
 set stop flag for merger
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_SetEncodeDisable(MMPF_H264ENC_ENC_INFO *pEnc)
{
	MMP_UBYTE ubEncID = get_vidinst_id(pEnc->priv_data);

    #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
	MMPF_IBC_SetPreFrameRdy(m_usVidRecPath[ubEncID], 0, MMP_FALSE);
    #endif

	if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
        m_bRtCurBufOvlp = MMP_FALSE;
        m_bRtScaleDFT = MMP_FALSE;
		MMPF_IBC_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_IBC_EVENT_FRM_RDY, NULL, NULL);
		MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, NULL, NULL);
		MMPF_Scaler_EnableInterrupt(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, MMP_FALSE);
    }

	pEnc->GlbStatus.VideoCaptureStart = 0;
    if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) {
        MMPF_MP4VENC_SetStatus(ubEncID, MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_STOP);
    }

	MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(ubEncID), MMPF_OS_FLAG_SET);

    return MMP_ERR_NONE;
}

/**
 @brief Pre-Encode operation of video encoder.

 This function stops video engine. It also disables the audio engine and sets the
 end frame being done.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_PreEncode (MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ERR     stat    = MMP_ERR_NONE;
    MMP_ULONG   mb_cnt  = 0, fps = 0;
	MMP_UBYTE   ubEncID = get_vidinst_id(pEnc->priv_data);
    #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
    MMP_ULONG   ulPreFrameLine;
    #endif

	if ((pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_RECORD) &&
		(pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_STOP)) {
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

	mb_cnt = pEnc->mb_w * pEnc->mb_h;
    fps = (pEnc->ulSnrFpsRes + pEnc->ulSnrFpsInc - 1) / pEnc->ulSnrFpsInc;
 
    if (MMPF_VIDENC_CheckCapability(mb_cnt, fps) == MMP_FALSE) {
        return MMP_MP4VE_ERR_CAPABILITY;
    }
	if ((stat = MMPF_VIDENC_SetEncodeEnable(pEnc)) != MMP_ERR_NONE) {
        return stat;
    }
    
    #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
	if ((pEnc->mb_w << 4) >= 1920)
		ulPreFrameLine = (pEnc->mb_h << 4) - 256;
	else if ((pEnc->mb_w << 4) >= 1280)
		ulPreFrameLine = (pEnc->mb_h << 4) - 128;
	else if ((pEnc->mb_w << 4) >= 720)
		ulPreFrameLine = (pEnc->mb_h << 4) - 64;
    else
		ulPreFrameLine = (pEnc->mb_h << 4) - 16;
    
    MMPF_IBC_SetPreFrameRdy(m_usVidRecPath[ubEncID], ulPreFrameLine, MMP_TRUE);
    #endif
    
    pEnc->GlbStatus.VidOp = VIDEO_RECORD_PREENCODE;
    pEnc->GlbStatus.VideoCaptureStart = 1;

	if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
		MMPF_IBC_SetStoreEnable(m_usVidRecPath[ubEncID], MMP_FALSE);
		MMPF_IBC_SetSingleFrmEnable(m_usVidRecPath[ubEncID], MMP_TRUE);

		MMPF_IBC_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_IBC_EVENT_FRM_RDY, rt_enc_restart_callback, (void *)pEnc);
		MMPF_Scaler_EnableInterrupt(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, MMP_TRUE);
		MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, rt_enc_scaledfs_restart_callback, (void *)pEnc);

        MMPF_VIDENC_TriggerEncode();
    }

    return MMP_ERR_NONE;
}

/**
 @brief START operation of video record.
 @retval MMP_ERR_NONE Success.
 @retval  Open file failed.
*/
MMP_ERR MMPF_VIDENC_Start(MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_ERR				stat		= MMP_ERR_NONE;
    MMP_ULONG			mb_cnt		= 0, fps = 0;
	MMP_UBYTE			ubEncID		= get_vidinst_id(pEnc->priv_data);
    #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
    MMP_ULONG ulPreFrameLine;
    #endif

	if ((pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_RECORD) &&
		(pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_STOP) &&
		(pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_PREENCODE)) {
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

	mb_cnt = pEnc->mb_w * pEnc->mb_h;
   
/* #if (USE_DIV_CONST) */
    /* fps = ((30*1000 + 1000 - 1 ) / 1000); */
/* #else */
	fps = (pEnc->ulSnrFpsRes + pEnc->ulSnrFpsInc - 1) / pEnc->ulSnrFpsInc;

 
    if (MMPF_VIDENC_CheckCapability(mb_cnt, fps) == MMP_FALSE) {
        return MMP_MP4VE_ERR_CAPABILITY;
    }
	if (pEnc->GlbStatus.VidStatus != MMPF_MP4VENC_FW_STATUS_PREENCODE) {
		if ((stat = MMPF_VIDENC_SetEncodeEnable(pEnc)) != MMP_ERR_NONE) {
            return stat;
        }
		pEnc->GlbStatus.VidOp			   = VIDEO_RECORD_START;
		pEnc->GlbStatus.VideoCaptureStart = 1;
        
        #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
		if ((pEnc->mb_w << 4) >= 1920)
			ulPreFrameLine = (pEnc->mb_h << 4) - 256;
		else if ((pEnc->mb_w << 4) >= 1280)
			ulPreFrameLine = (pEnc->mb_h << 4) - 128;
		else if ((pEnc->mb_w << 4) >= 720)
			ulPreFrameLine = (pEnc->mb_h << 4) - 64;
        else
			ulPreFrameLine = (pEnc->mb_h << 4) - 16;
		
		MMPF_IBC_SetPreFrameRdy(m_usVidRecPath[ubEncID], ulPreFrameLine, MMP_TRUE);
        #endif

		if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            m_bRtCurBufOvlp = MMP_FALSE;
            m_bRtScaleDFT = MMP_FALSE;
			MMPF_IBC_SetStoreEnable(m_usVidRecPath[ubEncID], MMP_FALSE);
			MMPF_IBC_SetSingleFrmEnable(m_usVidRecPath[ubEncID], MMP_TRUE);
			MMPF_IBC_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_IBC_EVENT_FRM_RDY, rt_enc_restart_callback, (void *)pEnc);
			MMPF_Scaler_EnableInterrupt(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, MMP_TRUE);
			MMPF_Scaler_RegisterIntrCallBack(m_usVidRecPath[ubEncID], MMP_SCAL_EVENT_DBL_FRM_ST, rt_enc_scaledfs_restart_callback, (void *)pEnc);

            MMPF_VIDENC_TriggerEncode();
        }
    }
    else {
		pEnc->GlbStatus.VidOp = VIDEO_RECORD_START;
    }

    return MMP_ERR_NONE;
}

/**
 @brief PAUSE operation of video encoder.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_Pause(MMP_ULONG ulEncId)
{
	MMPF_H264ENC_ENC_INFO *pEnc = &(MMPF_VIDENC_GetInstance((MMP_UBYTE)ulEncId)->h264e);

	pEnc->GlbStatus.VidOp = VIDEO_RECORD_PAUSE;

    return MMP_ERR_NONE;
}

/**
 @brief RESUME operation of video encoder.

 It clears video interrupt and enables.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_Resume(MMP_ULONG ulEncId)
{
	MMPF_H264ENC_ENC_INFO *pEnc = &(MMPF_VIDENC_GetInstance((MMP_UBYTE)ulEncId)->h264e);

    MMPF_H264ENC_Resume(pEnc);

    return MMP_ERR_NONE;
}

/**
 @brief STOP operation of video encoder.

 This function stops video engine. It also disables the audio engine and sets the
 end frame being done.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPF_VIDENC_Stop(MMPF_H264ENC_ENC_INFO *pEnc)
{
    MMP_UBYTE ubEncID = get_vidinst_id(pEnc->priv_data);

	pEnc->GlbStatus.VidOp = VIDEO_RECORD_STOP;

	if (pEnc->GlbStatus.VidStatus != (MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_STOP)) {
		if (pEnc->GlbStatus.VidStatus == (MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE)) {
			pEnc->GlbStatus.VideoCaptureStart = 0; // Pause -> Stop

            MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(ubEncID), MMPF_OS_FLAG_SET);
        }
    }
	
    RTNA_DBG_Str3("running out video stop\r\n");

    return MMP_ERR_NONE;
}

/**
 @brief Control the flow about deciding encode frame type, frame order
 @param[in][out] pbCurBuf. point to index of IBC exposure done frame buffer
 @param[in][out] pbIBCBuf. point to index of IBC current output frame buffer
*/
MMP_ERR MMPF_VIDENC_SetRecdFrameReady(MMP_USHORT usIBCPipe, MMP_ULONG *pulCurBuf, MMP_ULONG *pulIBCBuf)
{
    MMP_ERR				    qState;
    MMP_ULONG			    ulMaxDup;
	MMP_UBYTE			    ubEncID = gbIBCLinkEncId[usIBCPipe];
    MMPF_H264ENC_ENC_INFO   *pEnc = &(MMPF_VIDENC_GetInstance(ubEncID)->h264e);
    #if (CODE_FOR_DEMO)
    static MMP_BOOL         firstEncFrame = MMP_TRUE;
    #endif

    if (!pEnc->GlbStatus.VideoCaptureStart ||
        (MMPF_Sensor_Is3AConverge(PRM_SENSOR) == MMP_FALSE))
    {
        *pulCurBuf = *pulIBCBuf;
		if (MMPF_VIDENC_GetQueueDepth(&gVidRecdFreeQueue[ubEncID], MMP_FALSE)) {
			MMPF_VIDENC_PopQueue(&gVidRecdFreeQueue[ubEncID], 0, pulIBCBuf, MMP_FALSE);
			MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[ubEncID], *pulCurBuf, MMP_FALSE); //pre-push to free queue
        }
        return MMP_ERR_NONE;
    }

	if (pEnc->GlbStatus.VideoCaptureStart) {
        MMP_ULONG   ulVidCnt = 0; // num of frames will be put to ready queue

        *pulCurBuf = *pulIBCBuf;

        #if (CODE_FOR_DEMO)
        // Just for demo, trigger GPIO0 low
        if (firstEncFrame) {
            MMPF_PIO_EnableOutputMode(MMP_GPIO122, MMP_TRUE, MMP_FALSE);
            MMPF_PIO_SetData(MMP_GPIO122, GPIO_LOW, MMP_FALSE);
            firstEncFrame = MMP_FALSE;
        }
        #endif

        if (pEnc->GlbStatus.VidStatus == (MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE)) {
            MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[ubEncID], *pulIBCBuf, MMP_FALSE); //direct to free queue
            MMPF_VIDENC_PopQueue(&gVidRecdFreeQueue[ubEncID], 0, pulIBCBuf, MMP_FALSE);
            return MMP_ERR_NONE;
        }

        #if (VID_CTL_DBG_FPS)
        RTNA_DBG_Str0("*");
        #endif

        /* Frame rate control */
        #if (FPS_CTL_EN)
        if (MMPF_FpsCtl_IsEnabled(ubEncID)) {
            MMP_BOOL    bDrop;
            MMP_ULONG   ulDupCnt;

            MMPF_FpsCtl_Operation(ubEncID, &bDrop, &ulDupCnt);

            if (bDrop) {

                MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[ubEncID], *pulIBCBuf, MMP_FALSE); //direct to free queue
                MMPF_VIDENC_PopQueue(&gVidRecdFreeQueue[ubEncID], 0, pulIBCBuf, MMP_FALSE);

                if (pEnc->module->bWorking == MMP_FALSE) {
                    if (pEnc->GlbStatus.VidOp == VIDEO_RECORD_PAUSE) {
                        pEnc->GlbStatus.VidStatus = MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE;
                        MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_VSTREAM(ubEncID), MMPF_OS_FLAG_SET);
                    }
                    if ((pEnc->GlbStatus.VidOp == VIDEO_RECORD_STOP) || MMPF_HIF_GetCmdStatus(MEDIA_FILE_OVERFLOW)) {
                        MMPF_VIDENC_SetEncodeDisable(pEnc);
                    }
                }
                return MMP_ERR_NONE;
            }
            else if (ulDupCnt) {
                pEnc->dummydropinfo.ulDummyFrmCnt += ulDupCnt;
            }
        }
        #endif

        // to counteract duplicated frame count and drop frame count
        if (pEnc->dummydropinfo.ulDropFrmCnt && pEnc->dummydropinfo.ulDummyFrmCnt) {
            if (pEnc->dummydropinfo.ulDropFrmCnt > pEnc->dummydropinfo.ulDummyFrmCnt) {
            	pEnc->dummydropinfo.ulDropFrmCnt -= pEnc->dummydropinfo.ulDummyFrmCnt;
                pEnc->dummydropinfo.ulDummyFrmCnt = 0;
            }
            else {
            	pEnc->dummydropinfo.ulDummyFrmCnt -= pEnc->dummydropinfo.ulDropFrmCnt;
            	pEnc->dummydropinfo.ulDropFrmCnt = 0;
            }
        }

		if (MMPF_VIDENC_GetQueueDepth(&gVidRecdFreeQueue[ubEncID], MMP_FALSE))
        {
            // push to ready queue only if free queue was not underflowed
            if (pEnc->dummydropinfo.ulDropFrmCnt == 0) {
                qState = MMPF_VIDENC_PushQueue(&gVidRecdRdyQueue[ubEncID], *pulIBCBuf, MMP_FALSE);
                ulVidCnt = 1;

                if (qState == MMP_ERR_NONE) {
                    MMPF_VIDENC_PushQueue(&gVidRecdDtsQueue[ubEncID], OSTime, MMP_FALSE);
                    MMPF_VIDENC_SetFrameTimestamp(pEnc, *pulIBCBuf, OSTime);
                }
            }
            else {
                pEnc->dummydropinfo.ulDropFrmCnt--;
                MMPF_VIDENC_PushQueue(&gVidRecdFreeQueue[ubEncID], *pulIBCBuf, MMP_FALSE);
                RTNA_DBG_Str0("*drop\r\n");
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
                } while(ulMaxDup);
            }
            MMPF_VIDENC_PopQueue(&gVidRecdFreeQueue[ubEncID], 0, pulIBCBuf, MMP_FALSE);
        }
        else {
            // free queue underflowed means ready queue is full, push ready queue
            // with weight to indicated the last ready frame should be duplicated.
            MMPF_VIDENC_PushQueue(&gVidRecdRdyQueue[ubEncID], *pulIBCBuf, MMP_TRUE);
            ulVidCnt = 1;
        }

        #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
        MMPF_VIDENC_TriggerEncode();
        #endif

        // trigger encode by task
        MMPF_OS_SetFlags(VID_REC_Flag, SYS_FLAG_VIDENC, MMPF_OS_FLAG_SET);
    }

    return MMP_ERR_NONE;
}

/**
 @brief Control the flow about deciding encode frame type, frame order
 @param[in][out] pbCurBuf. point to index of IBC exposure done frame buffer
 @param[in][out] pbIBCBuf. point to index of IBC current output frame buffer
*/
MMP_ERR MMPF_VIDENC_TriggerEncode(void)
{
	MMP_ULONG   i;
    MMP_ULONG   ulTargetFrmOffset;
    MMP_ULONG   ulBufId = 0, ulDts = 0;
    MMPF_3GPMGR_FRAME_TYPE FrameType;
    MMP_ERR     queue_status;
    MMP_ULONG   ulRdyFrmCnt;
	MMP_UBYTE   ubEncID;
	MMPF_VIDENC_INSTANCE  *pInst;
	MMPF_H264ENC_ENC_INFO *pEnc;
    #if (VID_CTL_DBG_GOP)
    static MMPF_3GPMGR_FRAME_TYPE _FrmType = MMPF_3GPMGR_FRAME_TYPE_I;
    static MMP_ULONG _FrmTypeAcc = 0;
    #endif

    if (MMPF_VIDENC_GetModule()->H264EMod.bWorking) {
		return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

	for (i = 0; i < MMP_IBC_PIPE_MAX; i++) {
		if (gbIBCReady[i]) {
			ubEncID = gbIBCLinkEncId[i];
			gbIBCReady[i] = MMP_FALSE;

            pInst = MMPF_VIDENC_GetInstance(ubEncID);
            pEnc  = &(pInst->h264e);
            if (pInst->bInitInst && pEnc->GlbStatus.VideoCaptureStart)
                break;
		}
	}
	if (i == MMP_IBC_PIPE_MAX)
		return MMP_ERR_NONE;
    // running callback before encode
    if(pInst->preEncExec) {
        pInst->preEncExec( pInst->preEncArgc );
    }
 
#if 0
    printc("[sean] : br: %d(%d,%d)\r\n",pEnc->stream_bitrate,pEnc->layer_bitrate[0],pEnc->layer_bitrate[1] );
    printc("[sean] : init qp: I(%d,%d),P(%d,%d)\r\n", 
            pEnc->rc_config[0].InitQP[0],pEnc->rc_config[0].InitQP[1],
            pEnc->rc_config[1].InitQP[0],pEnc->rc_config[1].InitQP[1]
    );
    printc("[sean] : qp-boud: I(%d,%d),P(%d,%d)\r\n",
            pEnc->MbQpBound[0][MMPF_3GPMGR_FRAME_TYPE_I][0],
            pEnc->MbQpBound[0][MMPF_3GPMGR_FRAME_TYPE_I][1],
            pEnc->MbQpBound[1][MMPF_3GPMGR_FRAME_TYPE_P][0],
            pEnc->MbQpBound[1][MMPF_3GPMGR_FRAME_TYPE_P][1]);

#endif
    
	switch (get_vidinst_format(pInst)) {
	case MMPF_MP4VENC_FORMAT_H264:

        if (pEnc->total_frames < pEnc->conus_i_frame_num)
            FrameType = MMPF_3GPMGR_FRAME_TYPE_I;
        else
		    FrameType = MMPF_VIDENC_GetVidRecdFrameType(pEnc);

		ulTargetFrmOffset = ((MMPF_3GPMGR_FRAME_TYPE_P == FrameType) ? pEnc->b_frame_num: 0);

		if (pEnc->CurBufMode == MMPF_MP4VENC_CURBUF_RT) {
            ulBufId = 0;
            ulDts = OSTime;
        }
        else {
			// get how many frames are ready for encode, excluding dummy frames
			ulRdyFrmCnt = MMPF_VIDENC_GetQueueDepth(&gVidRecdRdyQueue[ubEncID], MMP_FALSE);

			// if the 1st frame in ready queue has weighting over 1, insert duplicated frames
			if (ulRdyFrmCnt) {
				while(MMP_MP4VE_ERR_QUEUE_WEIGHTED ==
						MMPF_VIDENC_ShowQueue(&gVidRecdRdyQueue[ubEncID], 0, &ulBufId, MMP_TRUE))
				{
					// decrease the weighting
					MMPF_VIDENC_PopQueue(&gVidRecdRdyQueue[ubEncID], 0, &ulBufId, MMP_TRUE);
					// insert duplicated frame by container
					// TODO: How to indicated a dummy frame needed? by Alterman
				}

				if (ulRdyFrmCnt >= (ulTargetFrmOffset+1)) {
					// get frame buffer & timestamp for encode
					queue_status = MMPF_VIDENC_ShowQueue(&gVidRecdRdyQueue[ubEncID],
							ulTargetFrmOffset,
							&ulBufId,
							MMP_FALSE);
					queue_status = MMPF_VIDENC_ShowQueue(&gVidRecdDtsQueue[ubEncID],
							ulTargetFrmOffset,
							&ulDts,
							MMP_FALSE);
				}
				else {
					return MMP_ERR_NONE;
				}
			}
			else {
				return MMP_ERR_NONE;
			}
		}

        /* Always insert SPS/PPS for IDR frame */
        if (FrameType == MMPF_3GPMGR_FRAME_TYPE_I)
            pEnc->insert_sps_pps = MMP_TRUE;

        #if (VID_CTL_DBG_GOP)
        if (FrameType != _FrmType) {
            MMPF_DBG_Int(_FrmTypeAcc, -3);
            if (_FrmType == MMPF_3GPMGR_FRAME_TYPE_I) {
                RTNA_DBG_Str(0, "I");
            }
            else {
                RTNA_DBG_Str(0, "P");
            }
            _FrmType = FrameType;
            _FrmTypeAcc = 1;
        }
        else {
            _FrmTypeAcc++;
        }
        #endif
        #if (VID_CTL_DBG_FPS)
        RTNA_DBG_Str0("e");
        #endif

    	MMPF_H264ENC_TriggerEncode (
				pEnc,
				FrameType,
				ulDts,
				&(pEnc->cur_frm[ulBufId]));

        #if (VID_CTL_DBG_BR)
        if ((pEnc->br_last_time - pEnc->br_base_time) > 5000) {
            MMP_ULONG64 bitrate;

            bitrate = pEnc->br_total_size << 3;
            bitrate *= 1000;
            bitrate /= (pEnc->br_last_time - pEnc->br_base_time);
            //RTNA_DBG_Str0("Bitrate: ");
            //MMPF_DBG_Int((MMP_ULONG)bitrate, -8);
            //RTNA_DBG_Str0("\r\n");
            printc("Bitrate[%d]:%d\r\n",pEnc->enc_id,bitrate );
            pEnc->br_base_time = 0;
        }
        #endif

        break;

    default:
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    return MMP_ERR_NONE;
}

/**
 @brief Check encode capability according to CHIP ID
*/
MMP_BOOL MMPF_VIDENC_CheckCapability(MMP_ULONG total_mb, MMP_ULONG fps)
{
    MMP_BOOL    support = MMP_FALSE;
    MMP_ULONG   mb_rate = total_mb * fps;

    if (gbSystemCoreID == CHIP_CORE_ID_MCR_V2) {
        // MB count of 1080p = (1920/16) * (1088/16) = 8160
        if (mb_rate <= (8160 * 60))
            support = MMP_TRUE;
    }
    else if (gbSystemCoreID == CHIP_CORE_ID_MCR_V2_LE) {
        // MB count of 1296p = (2304/16) * (1296/16) = 11664
        if (mb_rate <= (11664 * 30))
            support = MMP_TRUE;
    }

    if (support)
        RTNA_DBG_Str(3, "Supported");
    else
        RTNA_DBG_Str(3, "UnSupported");

    RTNA_DBG_Str(3, " encode capability\r\n");

    return support;
}

/**
 @brief Initialize the interrupt of video and timer2.
*/
void MMPF_MP4VENC_TaskInit(void)
{
    MMP_UBYTE i;
    MMPF_H264ENC_ENC_INFO *pEnc;

    for(i = 0; i < MAX_VIDEO_STREAM_NUM; i++) {
        pEnc = &(MMPF_VIDENC_GetInstance(i)->h264e);
        instance_parameter_config(pEnc);
    }
}

/**
 @brief Set video engine status by Mergr3GP task.
 @param[in] status Status of video engine.
 @note

 The parameter @a status can be:
 - 0x0001 : MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_START
 - 0x0002 : MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE
 - 0x0003 : MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_RESUME
 - 0x0004 : MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_STOP

 And the value can not be changed because it sync with the host video status definitions.
*/
void MMPF_MP4VENC_SetStatus(MMP_ULONG ulEncId, MMP_USHORT status)
{
	MMPF_H264ENC_ENC_INFO *pEnc = &(MMPF_VIDENC_GetInstance((MMP_UBYTE)ulEncId)->h264e);
	
	pEnc->GlbStatus.VidStatus = status;
}

/**
 @brief Return video engine status by Mergr3GP task.
 @retval 0x0001 MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_START
 @retval 0x0002 MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_PAUSE
 @retval 0x0003 MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_RESUME
 @retval 0x0004 MMPF_MP4VENC_FW_STATUS_RECORD | MMPF_MP4VENC_FW_STATUS_STOP
 @note The return value can not be changed because it sync with the host video
 status definitions.
*/
MMP_USHORT MMPF_MP4VENC_GetStatus(MMP_ULONG ulEncId)
{
    MMPF_H264ENC_ENC_INFO *pEnc = &(MMPF_VIDENC_GetInstance((MMP_UBYTE)ulEncId)->h264e);

	return pEnc->GlbStatus.VidStatus;
}

/**
 @brief Set video cropping from top/bottom/left/right

 The input paramters should be even and < 16
 @param[in] usTop top offset
 @param[in] usBottom bottom offset
 @param[in] usLeft left offset
 @param[in] usRight right offset
 @retval frame count encoder generated
*/
MMP_ERR MMPF_MP4VENC_SetCropping(   MMPF_H264ENC_ENC_INFO   *pEnc,
								    MMP_USHORT              usTop,
								    MMP_USHORT              usBottom,
								    MMP_USHORT              usLeft,
								    MMP_USHORT              usRight)
{
    MMPF_VIDENC_CROPPING VidRecdCropping;

    if ((usTop >= 16) || (usBottom >= 16) ||
        (usLeft >= 16) || (usRight >= 16) ||
        (usTop & 0x01) || (usBottom & 0x01) ||
        (usLeft & 0x01) || (usRight & 0x01)) {
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    VidRecdCropping.usTop       = usTop;
    VidRecdCropping.usBottom    = usBottom;
    VidRecdCropping.usLeft      = usLeft;
    VidRecdCropping.usRight     = usRight;

	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_CROPPING,
                                        &VidRecdCropping);
}

/**
 @brief Get the width and height from host parameters.
 @param[in] ResolW Encode width.
 @param[in] ResolH Encode height.
*/
MMP_ERR MMPF_MP4VENC_SetVideoResolution(MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_USHORT              usWidth,
                                        MMP_USHORT              usHeight)
{
    MMPF_VIDENC_RESOLUTION VidRecdResol;

	VidRecdResol.usWidth = usWidth;
	VidRecdResol.usHeight = usHeight;
	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_RESOLUTION,
                                        &VidRecdResol);
}

MMP_ERR MMPF_MP4VENC_SetVideoProfile(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_ULONG               ulProfile)
{
	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_PROFILE,
                                        (void *)ulProfile);
}

MMP_ERR MMPF_MP4VENC_SetVideoLevel( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_ULONG               ulLevel)
{
	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_LEVEL,
                                        (void *)ulLevel);
}

MMP_ERR MMPF_MP4VENC_SetVideoEntropy(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        VIDENC_ENTROPY          entropy)
{
	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_ENTROPY_MODE,
                                        (void *)entropy);
}

MMP_ERR MMPF_MP4VENC_SetVideoGOP(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_USHORT              usPFrame,
                                    MMP_USHORT              usBFrame)
{
	MMPF_VIDENC_GOP_CTL   GopCtl;

	GopCtl.usGopSize            = 1 + (usPFrame * (usBFrame + 1));
	GopCtl.usMaxContBFrameNum   = usBFrame;

	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_GOP_CTL,
                                        &GopCtl);
}

MMP_ERR MMPF_MP4VENC_SetRcMode( MMPF_H264ENC_ENC_INFO   *pEnc,
                                VIDENC_RC_MODE          mode)
{
    MMPF_VIDENC_RC_MODE_CTL RcModeCtl;

    RcModeCtl.RcMode = mode;

    return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_RC_MODE,
                                        &RcModeCtl);
}

MMP_ERR MMPF_MP4VENC_SetRcSkip(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG skip)
{
    return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_RC_SKIPPABLE,
                                        (void *)  skip);
}

MMP_ERR MMPF_MP4VENC_SetRcSkipType( MMPF_H264ENC_ENC_INFO   *pEnc,
                                    VIDENC_RC_SKIPTYPE      type)
{
    return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_RC_SKIPTYPE,
                                        (void *)type);
}

MMP_ERR MMPF_MP4VENC_SetRcLbSize(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG lbs)
{
    MMPF_VIDENC_LEAKYBUCKET_CTL LBCtl;

    LBCtl.ubLayerBitMap = 1;
    LBCtl.ulLeakyBucket[0] = lbs;

    return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_LB_SIZE,
                                        &LBCtl);
}

MMP_ERR MMPF_MP4VENC_SetTNR(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG tnr)
{
    return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_TNR_EN,
                                        (void *)tnr);
}

MMP_ERR MMPF_MP4VENC_SetInitQP( MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_UBYTE               ubIQP,
                                MMP_UBYTE               ubPQP)
{
	MMPF_VIDENC_QP_CTL  pQpCtl;

    pQpCtl.ubTID            = 0;
    pQpCtl.ubTypeBitMap     = (1 << I_FRAME) | (1 << P_FRAME);
    pQpCtl.ubQP[I_FRAME]    = ubIQP;
    pQpCtl.ubQP[P_FRAME]    = ubPQP;

	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_FRM_QP,
                                        &pQpCtl);
}

MMP_ERR MMPF_MP4VENC_SetQPBound(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_UBYTE               ubLowBound,
                                MMP_UBYTE               ubHighBound)
{
	MMPF_VIDENC_QP_BOUND_CTL pQpBound;

	pQpBound.ubLayerID      = 0;
    pQpBound.ubTypeBitMap   = (1 << I_FRAME) | (1 << P_FRAME);
    pQpBound.ubQPBound[I_FRAME][BD_LOW]  =
    pQpBound.ubQPBound[P_FRAME][BD_LOW]  = ubLowBound;
    pQpBound.ubQPBound[I_FRAME][BD_HIGH] =
    pQpBound.ubQPBound[P_FRAME][BD_HIGH] = ubHighBound;

	return MMPF_H264ENC_SetParameter(   pEnc,
                                        MMPF_VIDENC_ATTRIBUTE_FRM_QP_BOUND,
                                        &pQpBound);
}

/**
 @brief Set encode bitrate
 @param[in] bitrate Bitrate.
*/
void MMPF_MP4VENC_SetBitRate(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG bitrate)
{
    MMPF_VIDENC_BITRATE_CTL VidRecdRcCtl;

	VidRecdRcCtl.ubLayerBitMap  = 0x01;
	VidRecdRcCtl.ulBitrate[0]   = bitrate;

	MMPF_H264ENC_SetParameter(pEnc, MMPF_VIDENC_ATTRIBUTE_BR, &VidRecdRcCtl);
}

/**
 @brief Set encode target bitrate for ramp-up
 @param[in] bitrate Bitrate.
*/
void MMPF_MP4VENC_SetTargetBitRate(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG target_br,MMP_ULONG rampup_br)
{
    pEnc->target_bitrate = target_br ;
    pEnc->rampup_bitrate = rampup_br ;
}


/**
 @brief Set encode time resolution and increment from host parameters.
 @param[in] timeresol Time resolution.
 @param[in] timeincrement Time increment.
*/
void MMPF_MP4VENC_SetEncFrameRate(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_ULONG               resol,
                                    MMP_ULONG               incr)
{
    MMP_VID_FPS fps;

    fps.ulResol     = resol;
    fps.ulIncr      = incr;
    fps.ulIncrx1000 = incr * 1000;

    #if (FPS_CTL_EN)
    MMPF_FpsCtl_SetEncFrameRate(pEnc->enc_id, incr, resol);
    #endif
	MMPF_H264ENC_SetParameter(pEnc, MMPF_VIDENC_ATTRIBUTE_ENC_FPS, &fps);
}

/**
 @brief Update encode time resolution and increment from host parameters.
 @param[in] timeresol Time resolution.
 @param[in] timeincrement Time increment.
*/
void MMPF_MP4VENC_UpdateEncFrameRate(   MMPF_H264ENC_ENC_INFO   *pEnc,
                                        MMP_ULONG               resol,
                                        MMP_ULONG               incr)
{
	MMP_VID_FPS fps;

    fps.ulResol     = resol;
    fps.ulIncr      = incr;
    fps.ulIncrx1000 = incr * 1000;

    #if (FPS_CTL_EN)
    MMPF_FpsCtl_UpdateEncFrameRate(pEnc->enc_id, incr, resol);
    #endif
    MMPF_H264ENC_SetParameter(pEnc, MMPF_VIDENC_ATTRIBUTE_ENC_FPS, &fps);
}

/**
 @brief Set sensor input time resolution and increment from host parameters.
 @param[in] timeresol Time resolution.
 @param[in] timeincrement Time increment.
 */
void MMPF_MP4VENC_SetSnrFrameRate(  MMPF_H264ENC_ENC_INFO   *pEnc,
                                    MMP_ULONG               resol,
                                    MMP_ULONG               incr)
{
    MMP_VID_FPS fps;

    fps.ulResol     = resol;
    fps.ulIncr      = incr;
    fps.ulIncrx1000 = incr * 1000;

    #if (FPS_CTL_EN)
    MMPF_FpsCtl_SetSnrFrameRate(pEnc->enc_id, incr, resol);
    #endif
	MMPF_H264ENC_SetParameter(pEnc, MMPF_VIDENC_ATTRIBUTE_SNR_FPS, &fps);
}

/**
 @brief Set current buf mode from host parameters.
 @param[in] curbufmode current buf mode.
*/
void MMPF_MP4VENC_SetCurBufMode(MMPF_H264ENC_ENC_INFO   *pEnc,
                                MMP_ULONG              curbufmode)
{
    gsVidRecdCurBufMode = curbufmode;

	MMPF_H264ENC_SetParameter(  pEnc,
                                MMPF_VIDENC_ATTRIBUTE_CURBUF_MODE,
                                (void *) curbufmode);
}

/**
 @brief Force to encode the next frame as an I-frame
 @param[in] ulEncId     Encoder ID
*/
void MMPF_MP4VENC_ForceI(MMPF_H264ENC_ENC_INFO *pEnc, MMP_ULONG ulCnt)
{
    MMPF_H264ENC_SetParameter(  pEnc,
                                MMPF_VIDENC_ATTRIBUTE_FORCE_I,
                                (void *)ulCnt);
}

/**
 @brief Command process.
 @param[in] usCommand Host command.
 @return Status.
*/
MMP_ERR MMPF_MP4VENC_ProcessCmd(void)
{
    MMP_ULONG   ulParameter[MAX_HIF_ARRAY_SIZE], i;
    MMP_USHORT  usCommand;
    MMPF_H264ENC_ENC_INFO *pEnc;

    usCommand = m_ulHifCmd[GRP_IDX_VID];
    for(i = 0; i < MAX_HIF_ARRAY_SIZE; i++) {
        ulParameter[i] = m_ulHifParam[GRP_IDX_VID][i];
    }    
    m_ulHifCmd[GRP_IDX_VID] = 0;

    pEnc = MMPF_H264ENC_GetHandle(ulParameter[0]);

    switch (usCommand & (GRP_MASK|FUNC_MASK)) {
    case HIF_VID_CMD_RECD_PARAMETER:
        switch (usCommand & SUB_MASK) {
        case ENCODE_RESOLUTION:
            MMPF_MP4VENC_SetVideoResolution(pEnc,
                                            (MMP_USHORT)(ulParameter[1]),
                                            (MMP_USHORT)(ulParameter[1] >> 16));
            break;
        case ENCODE_PROFILE:
            MMPF_MP4VENC_SetVideoProfile(pEnc, ulParameter[1]);
            break;
        case ENCODE_LEVEL:
            MMPF_MP4VENC_SetVideoLevel(pEnc, ulParameter[1]);
            break;
        case ENCODE_ENTROPY:
            MMPF_MP4VENC_SetVideoEntropy(pEnc, ulParameter[1]);
            break;
        case ENCODE_FORCE_I:
            MMPF_MP4VENC_ForceI(pEnc, ulParameter[1]);
            break;
        case ENCODE_GOP:
            MMPF_MP4VENC_SetVideoGOP(pEnc,
                                    (MMP_USHORT)(ulParameter[1]),
                                    (MMP_USHORT)(ulParameter[1]>>16));
            break;
        case ENCODE_RC_MODE:
            MMPF_MP4VENC_SetRcMode(pEnc, ulParameter[1]);
            break;
        case ENCODE_RC_SKIP:
            MMPF_MP4VENC_SetRcSkip(pEnc, ulParameter[1]);
            break;
        case ENCODE_RC_SKIPTYPE:
            MMPF_MP4VENC_SetRcSkipType(pEnc, ulParameter[1]);
            break;
        case ENCODE_RC_LBS:
            MMPF_MP4VENC_SetRcLbSize(pEnc, ulParameter[1]);
            break;
        case ENCODE_TNR_EN:
            MMPF_MP4VENC_SetTNR(pEnc, ulParameter[1]);
            break;
        case SET_QP_INIT:
            MMPF_MP4VENC_SetInitQP(pEnc, (MMP_UBYTE)ulParameter[1],
                                         (MMP_UBYTE)ulParameter[2]);
            break;
        case SET_QP_BOUNDARY:
            MMPF_MP4VENC_SetQPBound(pEnc, (MMP_UBYTE)ulParameter[1],
                                          (MMP_UBYTE)ulParameter[2]);
            break;
        case VIDRECD_STATUS:
            MMPF_HIF_FeedbackParamW(GRP_IDX_VID, 0,
                                    MMPF_MP4VENC_GetStatus(ulParameter[0]));
            break;
        case ENCODE_BITRATE:
            MMPF_MP4VENC_SetBitRate(pEnc, ulParameter[1]);
            break;
        case ENCODE_FRAME_RATE:
            MMPF_MP4VENC_SetEncFrameRate(   pEnc,
                                            ulParameter[1],
                                            ulParameter[2]);
            break;
        case ENCODE_FRAME_RATE_UPD:
            MMPF_MP4VENC_UpdateEncFrameRate(pEnc,
                                            ulParameter[1],
                                            ulParameter[2]);
            break;
        case SNR_FRAME_RATE:
            MMPF_MP4VENC_SetSnrFrameRate(   pEnc,
                                            ulParameter[1],
                                            ulParameter[2]);
            break;
        case ENCODE_CURBUFMODE:
            MMPF_MP4VENC_SetCurBufMode( pEnc,(MMP_USHORT)(ulParameter[1]));
            break;
        case ENCODE_BSBUF:
            MMPF_H264ENC_SetBSBuf(pEnc, ulParameter[1], ulParameter[2]);
            break;
        case ENCODE_MISCBUF:
            MMPF_H264ENC_SetMiscBuf(pEnc, ulParameter[1], ulParameter[2]);
            break;
        case ENCODE_REFGENBD:
            MMPF_H264ENC_SetRefListBound(pEnc, ulParameter[1], ulParameter[2]);
            break;
        case ENCODE_CROPPING:
            MMPF_MP4VENC_SetCropping(pEnc,
                            		(MMP_USHORT)(ulParameter[1] & 0xFFFF),
                            		(MMP_USHORT)((ulParameter[1] >> 16) & 0xFFFF),
                            		(MMP_USHORT)(ulParameter[2] & 0xFFFF),
                            		(MMP_USHORT)((ulParameter[2] >> 16) & 0xFFFF));
            break;
        case ENCODE_PADDING:
            MMPF_H264ENC_SetPadding (pEnc,
            		                (MMP_USHORT)(ulParameter[1] & 0xFFFF),
            		                (MMP_USHORT)((ulParameter[1] >> 16) & 0xFFFF));
            break;
        case ENCODE_BYTE_COUNT:
            MMPF_H264ENC_SetH264ByteCnt((MMP_USHORT)(ulParameter[0] & 0xFFFF));
            break;
        case ENCODE_CAPABILITY:
            MMPF_HIF_FeedbackParamB(GRP_IDX_VID, 0, MMPF_VIDENC_CheckCapability(
                                                            ulParameter[0],
                                                            ulParameter[1]));
            break;
        }
        break;
    default:
        RTNA_DBG_Str(0, "Not support video encoder host command = ");
        RTNA_DBG_Short(0, usCommand);
        RTNA_DBG_Str(0, "\r\n");
        return MMP_MP4VE_ERR_PARAMETER;
    }

    MMPF_OS_SetFlags(DSC_UI_Flag, SYS_FLAG_VID_CMD_DONE, MMPF_OS_FLAG_SET);

    return MMP_ERR_NONE;
}

/**
 @brief Main routine of video recorder task.
*/
void MP4VENC_Task(void *p_arg)
{
	MMPF_OS_FLAGS flags;
    MMPF_OS_FLAGS waitFlags;

    RTNA_DBG_Str3("MP4VENC_Task()\r\n");

    MMPF_MP4VENC_TaskInit();

    waitFlags = CMD_FLAG_VIDRECD | SYS_FLAG_VIDENC;

    while (TRUE) {
    
        MMPF_OS_WaitFlags(VID_REC_Flag, waitFlags,
                         (MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME),
                         0, &flags);

		if (flags & CMD_FLAG_VIDRECD) {
        	MMPF_MP4VENC_ProcessCmd();
		}

        if (flags & SYS_FLAG_VIDENC) {
        	MMPF_VIDENC_TriggerEncode();
		}
    }
}

#endif //(VIDEO_R_EN)
