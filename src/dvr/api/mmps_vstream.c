/**
 @file mmps_vstream.c
 @brief Video Stream Control Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmps_sensor.h"
#include "mmps_vstream.h"
#include "mmpd_ptz.h"
#include "mmpd_3gpmgr.h"
#include "mmpd_bayerscaler.h"
#include "mmpf_system.h"
#include "mmpf_vstream.h"
#include "dualcpu_v4l2.h"
#include "ait_osd.h"

/** @addtogroup MMPS_VSTREAM
@{
*/

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 *  Local Variables
 */
static MMPS_VSTREAM_CLASS   m_VStream[MAX_VIDEO_STREAM_NUM];

// Shared MV buffer for all stream
static MMPS_VSTREAM_MEMBLK  m_ShareBufMV;
// Shared slice length buffer for all stream
static MMPS_VSTREAM_MEMBLK  m_ShareBufSliceLen;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPS_VStream_ReserveHeap(void);

static MMP_ERR MMPS_VStream_AssignBuf(  MMPS_VSTREAM_CLASS      *obj);

#if (DBG_VSTREAM_MEM_MAP)
static void    MMPS_VStream_MemMapDbg(  MMPS_VSTREAM_CLASS      *obj);
#endif

static MMP_ERR MMPS_VStream_ConfigPipe( MMPS_VSTREAM_CLASS      *obj,
                                        MMPD_MP4VENC_INPUT_BUF 	*pInputBuf);

static MMP_ERR MMPS_VStream_EnablePipe( MMPS_VSTREAM_CLASS      *obj,
                                        MMP_BOOL                bEnable);

static MMP_ERR MMPS_VStream_OpenEncoder(MMPS_VSTREAM_CLASS *obj);
static MMP_ERR MMPS_VStream_CloseEncoder(MMPS_VSTREAM_CLASS *obj);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____VStream_Module_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize video stream module

 @retval It reports the status of the operation.
*/
int MMPS_VStream_ModInit(void)
{
    static MMP_BOOL _init = MMP_FALSE;
    MMP_ULONG s;
    MMPS_VSTREAM_CLASS *obj = NULL;

    if (_init == MMP_FALSE) {
        MEMSET(&m_ShareBufMV, 0, sizeof(m_ShareBufMV));
        MEMSET(&m_ShareBufSliceLen, 0, sizeof(m_ShareBufSliceLen));

        for(s = 0; s < MAX_VIDEO_STREAM_NUM; s++) {
            obj = &m_VStream[s];
            MEMSET(obj, 0, sizeof(MMPS_VSTREAM_CLASS));
            if (s < gulMaxVStreamNum)
                obj->cap = &gstVStreamCap[s];
        }

        MMPS_VStream_ReserveHeap(); // Reserve heap buffer for each stream

        _init = MMP_TRUE;
    }
    return 0;
}

#if 0
void ____VStream_Obj_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of video stream object

 @param[in] propt   Requested properties of a video stream

 @retval The object instance of video streamer
*/
#if SUPPORT_OSD_FUNC
extern AIT_OSD_BUFDESC desc_osd[MMP_IBC_PIPE_MAX] ;
#endif
MMPS_VSTREAM_CLASS *MMPS_VStream_Open(MMPS_VSTREAM_PROPT *propt)
{
    MMP_ERR   err;
    MMP_ULONG s, pipe;
    MMP_ULONG req_frm_size, req_buf_size;
    MMP_ULONG frm_size = 0, buf_size = 0;
    MMP_ULONG cand_frm_size = 0, cand_buf_size = 0;
    MMPS_VSTREAM_CLASS *obj = NULL;

    if (!propt)
    	return NULL;

    req_frm_size = ALIGN16(propt->w) * ALIGN16(propt->h);
    req_buf_size = ((propt->ctl.bitrate >> 3) / 1000) * propt->outbuf_ms;

    /* Search a stream whose capability is best-fit with the request feature */
    for(s = 0; s < MAX_VIDEO_STREAM_NUM; s++) {
        if (m_VStream[s].state == IPC_STREAMER_IDLE) {
            frm_size = ALIGN16(m_VStream[s].cap->img_w) *
                       ALIGN16(m_VStream[s].cap->img_h);
            buf_size = ((m_VStream[s].cap->bitrate >> 3) / 1000) *
                        m_VStream[s].cap->buftime;
            if ((frm_size == req_frm_size) && (buf_size == req_buf_size)) {
                obj = &m_VStream[s];
                break;
            }
            else if ((frm_size >= req_frm_size) && (buf_size >= req_buf_size)) {
                if (!obj) {
                    obj = &m_VStream[s];
                    cand_frm_size = frm_size;
                    cand_buf_size = buf_size;
                }
                else if ((frm_size < cand_frm_size) ||
                        (buf_size < cand_buf_size))
                {
                    obj = &m_VStream[s];
                    break;
                }
            }
        }
    }

    if (!obj)
        return NULL;

    pipe = MMPD_Fctl_AllocatePipe(ALIGN16(propt->w), PIPE_LINK_FB);
    if (pipe > MMP_IBC_PIPE_MAX) {
        RTNA_DBG_Str(0, "#VStream alloc pipe err\r\n");
		return NULL;
    }

    FCTL_PIPE_TO_LINK(pipe, obj->pipe);

    err = MMPD_VIDENC_InitInstance(&obj->enc_id);
    if (err != MMP_ERR_NONE) {
        RTNA_DBG_Str(0, "#VStream init instance err\r\n");
        goto _inst_err;
	}

    propt->CallBackArgc = propt->CallBackArgc | obj->pipe.ibcpipeID;
    obj->propt = *propt;
    #if SUPPORT_OSD_FUNC
    //using to set width and height for osd drawer when not get img_info from ipc
    desc_osd[pipe].ulBufWidth  = propt->w;
    desc_osd[pipe].ulBufHeight = propt->h;
    MMPS_VStream_RegisterDrawer(obj, (ipc_osd_drawer *)propt->CallBackFunc, (void *)propt->CallBackArgc) ;
    #endif
    err = MMPS_VStream_OpenEncoder(obj);
    if (err != MMP_ERR_NONE) {
        RTNA_DBG_Str(0, "#VStream open enc err\r\n");
        goto _enc_err;
    }
    
    obj->state = IPC_STREAMER_OPEN;

    return obj;

_enc_err:
    MMPD_VIDENC_DeInitInstance(obj->enc_id);

_inst_err:
    MMPD_Fctl_ReleasePipe(pipe);

    return NULL;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start the video stream

 @param[in] obj     Video stream object
 @param[in] v4l2_id Corresponded V4L2 stream id
 @param[in] opt     Option of streaming

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_VStream_Start( MMPS_VSTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!obj)
		return MMP_VSTREAM_ERR_INVALID_PARAMETERS;
	if (obj->state != IPC_STREAMER_OPEN)
		return MMP_VSTREAM_ERR_STATE;

    err = MMPF_VStream_Start(obj->enc_id, v4l2_id, opt);
    if (err) {
        RTNA_DBG_Str(0, "#VStream start err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the video stream

 @param[in] obj     Video stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_VStream_Stop(MMPS_VSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
        return MMP_VSTREAM_ERR_INVALID_PARAMETERS;
    if (obj->state != IPC_STREAMER_START)
        return MMP_VSTREAM_ERR_STATE;

    err = MMPF_VStream_Stop(obj->enc_id);
    if (err) {
        RTNA_DBG_Str(0, "#VStream stop err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close a instance of video stream object

 @param[in] obj     Video stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_VStream_Close(MMPS_VSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
		return MMP_VSTREAM_ERR_INVALID_PARAMETERS;
	if (obj->state != IPC_STREAMER_OPEN)
		return MMP_VSTREAM_ERR_STATE;

    err = MMPS_VStream_CloseEncoder(obj);
    if (err != MMP_ERR_NONE) {
		RTNA_DBG_Str(0, "#VStream close enc err\r\n");
    }

    MMPD_Fctl_ReleasePipe(obj->pipe.ibcpipeID);

    err = MMPD_VIDENC_DeInitInstance(obj->enc_id);
    if (err != MMP_ERR_NONE) {
		RTNA_DBG_Str(0, "#VStream deinit instance err\r\n");
		return err;
	}
    obj->state = IPC_STREAMER_IDLE;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_Control
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set the control of a started video stream

 @param[in] obj     Video stream object
 @param[in] ctrl    Video control type
 @param[in] param   Video control parameters

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_VStream_Control(   MMPS_VSTREAM_CLASS  *obj,
                                IPC_VIDEO_CTL       ctrl,
                                VIDENC_CTL_SET      *param)
{
    MMP_ERR     err = MMP_VSTREAM_ERR_PARAMETER;
    MMP_ULONG   enc_id = 0;
    VIDENC_CTL_SET *ctl = &obj->propt.ctl;

    if (!obj)
        return MMP_VSTREAM_ERR_INVALID_PARAMETERS;
    if ((obj->state != IPC_STREAMER_OPEN) && (obj->state != IPC_STREAMER_START))
        return MMP_VSTREAM_ERR_STATE;

    enc_id = obj->enc_id;

    switch(ctrl) {
    case IPC_VIDCTL_BITRATE:
        if (ctl->bitrate != param->bitrate) {
            ctl->bitrate = param->bitrate;
            err = MMPD_VIDENC_SetBitrate(enc_id, ctl->bitrate);
        }
        break;

    case IPC_VIDCTL_GOP:
        if (ctl->gop != param->gop) {
            ctl->gop = param->gop;
            err = MMPD_VIDENC_SetGOP(enc_id, ctl->gop - 1, 0);
        }
        break;

    case IPC_VIDCTL_ENC_FPS:
        if ((ctl->enc_fps.ulIncr != param->enc_fps.ulIncr) ||
            (ctl->enc_fps.ulResol != param->enc_fps.ulResol))
        {
            ctl->enc_fps = param->enc_fps;
            err = MMPD_VIDENC_UpdateEncFrameRate(enc_id,
                                                ctl->enc_fps.ulIncr,
                                                ctl->enc_fps.ulResol);
        }
        break;

    case IPC_VIDCTL_FORCE_I:
        if (ctl->force_idr != param->force_idr) {
            ctl->force_idr = param->force_idr;
            // Encode next frame as I-frame
            if (ctl->force_idr)
                err = MMPD_VIDENC_ForceI(enc_id, 1);
            else
                err = MMP_ERR_NONE;
        }
        break;

    case IPC_VIDCTL_RC_SKIP:
        if (ctl->rc_skip != param->rc_skip) {
            ctl->rc_skip = param->rc_skip;
            err = MMPD_VIDENC_SetRcSkip(enc_id, ctl->rc_skip);
        }
        break;

    case IPC_VIDCTL_RC_SKIP_TYPE:
        if (ctl->rc_skiptype != param->rc_skiptype) {
            ctl->rc_skiptype = param->rc_skiptype;
            err = MMPD_VIDENC_SetRcSkipType(enc_id, ctl->rc_skiptype);
        }
        break;

    case IPC_VIDCTL_QP_BOUND:
        if ((ctl->qp_bound[0] != param->qp_bound[0]) ||
            (ctl->qp_bound[1] != param->qp_bound[1]))
        {
            ctl->qp_bound[0] = param->qp_bound[0];
            ctl->qp_bound[1] = param->qp_bound[1];
            err = MMPD_VIDENC_SetQPBoundary(enc_id,
                                            ctl->qp_bound[0],
                                            ctl->qp_bound[1]);
        }
        break;

    case IPC_VIDCTL_LB_SIZE:
        if (ctl->lb_size != param->lb_size) {
            ctl->lb_size = param->lb_size;
            err = MMPD_VIDENC_SetRcLbSize(enc_id, ctl->lb_size);
        }
        break;

    case IPC_VIDCTL_TNR:
        if (ctl->tnr != param->tnr) {
            ctl->tnr = param->tnr;
            err = MMPD_VIDENC_SetTNR(enc_id, ctl->tnr);
        }
        break;

    default:
        return MMP_VSTREAM_ERR_PARAMETER;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_RegisterDrawer
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Register draw function before encode

 @param[in] obj     Video stream object
 @param[in] cb      Draw function
 @param[in] param   Draw function parameter

 @retval It reports the status of the operation.
*/

MMP_ERR MMPS_VStream_RegisterDrawer(   MMPS_VSTREAM_CLASS  *obj, MMPF_VIDENC_Callback *cb,void *cb_argc )
{
    if(obj) {
        return MMPF_VIDENC_RegisterPreEncodeCallBack( obj->enc_id ,cb,cb_argc );  
    }
    return MMP_ERR_NONE ;
}


#if 0
void ____VStream_Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for video stream.
*/
static MMP_ERR MMPS_VStream_ReserveHeap(void)
{
    MMP_ULONG s, buf_size;
    MMP_ULONG w, h, frm;
    MMP_ULONG max_w = 0, max_h = 0, max_frm = 0;
    MMPS_VSTREAM_CLASS *obj = NULL;

    for(s = 0; s < MAX_VIDEO_STREAM_NUM; s++) {
        obj = &m_VStream[s];
        if (obj->cap) {
            w = ALIGN16(obj->cap->img_w);
            h = ALIGN16(obj->cap->img_h);
            frm = w * h;
            if (w > max_w) max_w = w;
            if (h > max_h) max_h = h;
            if (frm > max_frm) max_frm = frm;
        }
    }

    // Slice length buffer is shared by all stream
    m_ShareBufSliceLen.size = ALIGN_X(((max_h >> 4) + 2) << 2, 64);
    m_ShareBufSliceLen.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                  m_ShareBufSliceLen.size, 64);
    m_ShareBufSliceLen.end  = m_ShareBufSliceLen.base + m_ShareBufSliceLen.size;

    // MV buffer is shared by all stream too
    m_ShareBufMV.size = max_frm >> 2;
    m_ShareBufMV.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                            m_ShareBufMV.size, 64);
    m_ShareBufMV.end  = m_ShareBufMV.base + m_ShareBufMV.size;

    // Assign heap for each stream according to its capability
    for (s = 0; s < MAX_VIDEO_STREAM_NUM; s++) {
        obj = &m_VStream[s];
        if (obj->cap) {
            w = ALIGN16(obj->cap->img_w);
            h = ALIGN16(obj->cap->img_h);
            frm = ALIGN256((w * h * 3) >> 1);   // NV12
            // input frame buffer + reference frame
            obj->heap.size = frm * (obj->cap->infrmslots + 1);
            // compress buffer
            buf_size = ((obj->cap->bitrate >> 3) / 1000) * obj->cap->buftime;
            buf_size = ALIGN32(buf_size);
            obj->heap.size += buf_size;

            obj->heap.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                 obj->heap.size, 256);
            if (obj->heap.base == SYS_HEAP_MEM_INVALID)
                return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
            obj->heap.end  = obj->heap.base + obj->heap.size;
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_AssignBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign buffers for a video stream object

 @param[in] obj     Video stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_VStream_AssignBuf(MMPS_VSTREAM_CLASS *obj)
{
    MMP_USHORT  i;
    MMP_UBYTE   *buf;
    MMP_ULONG   ulCuraddr, ulAlignW, ulAlignH;
    MMP_ULONG   ulYSize, ulUVSize, ulFrmSize;
    MMPS_VSTREAM_PROPT  *propt = &obj->propt;
    MMPD_MP4VENC_VIDEOBUF videohwbuf;

    ulCuraddr = obj->heap.base;

    ulAlignW = ALIGN16(propt->w);
    ulAlignH = ALIGN16(propt->h);

    /*
	 * Assign buffers for the length of encoded slices & MV from the shared buf
	 */
	// Buffer to save the length of encoded slices
	obj->lenbuf.base = m_ShareBufSliceLen.base;
    obj->lenbuf.size = ALIGN_X(((ulAlignH >> 4) + 2) << 2, 64);
    obj->lenbuf.end  = obj->lenbuf.base + obj->lenbuf.size;
    if (obj->lenbuf.end > m_ShareBufSliceLen.end)
        return MMP_VSTREAM_ERR_MEM_EXHAUSTED;

	videohwbuf.miscbuf.ulSliceLenBuf = obj->lenbuf.base;

    // Buffer to keep MV: MBs * (16 sub-MB per MB)* (4 Bytes)
    obj->mvbuf.base = m_ShareBufMV.base;
    obj->mvbuf.size = ((ulAlignW >> 4) * (ulAlignH >> 4)) << 6;
    obj->mvbuf.end  = obj->mvbuf.base + obj->mvbuf.size;
    if (obj->mvbuf.end > m_ShareBufMV.end)
        return MMP_VSTREAM_ERR_MEM_EXHAUSTED;

    videohwbuf.miscbuf.ulMVBuf = obj->mvbuf.base;

    /*
     * Allocate Input & reference frame size
     */
    ulYSize  = ulAlignW * ulAlignH;
    ulUVSize = ALIGN256(ulYSize >> 1);
    ulFrmSize = ulYSize + ulUVSize;
    // Set input frame buffers
    if (obj->inbuf.ulBufCnt == 0)
        obj->inbuf.ulBufCnt = obj->cap->infrmslots;
    for (i = 0; i < obj->inbuf.ulBufCnt; i++) {
        obj->inbuf.ulY[i] = ALIGN256(ulCuraddr);
        obj->inbuf.ulU[i] = obj->inbuf.ulY[i] + ulYSize;
        obj->inbuf.ulV[i] = obj->inbuf.ulU[i];
        ulCuraddr += ulFrmSize;
    }
    // Fill the last 8 line as black color
    for (i = 0; i < obj->inbuf.ulBufCnt; i++) {
        buf = (MMP_UBYTE *)(obj->inbuf.ulU[i] - (ulAlignW * 8));
        MEMSET(buf, 0x00, ulAlignW * 8);
        buf = (MMP_UBYTE *)(obj->inbuf.ulU[i] + ulUVSize - (ulAlignW * 4));
        MEMSET(buf, 0x80, ulAlignW * 4);
    }
	// Set reference/reconstruct buffer
	ulCuraddr = ALIGN256(ulCuraddr);
	MMPD_VIDENC_CalculateRefBuf(ulAlignW, ulAlignH, &(videohwbuf.refgenbd),
                                                                &ulCuraddr);
    obj->refbuf.base = videohwbuf.refgenbd.ulYStart;
    obj->refbuf.end  = videohwbuf.refgenbd.ulUEnd;
    obj->refbuf.size = obj->refbuf.end - obj->refbuf.base;
    obj->genbuf.base = videohwbuf.refgenbd.ulGenStart;
    obj->genbuf.end  = videohwbuf.refgenbd.ulGenEnd;
    obj->genbuf.size = obj->genbuf.end - obj->genbuf.base;

	/*
     * Allocate compress buffer
     */
	ulCuraddr = ALIGN32(ulCuraddr);
    obj->outbuf.base = ulCuraddr;
// 
//  obj->outbuf.size = ((propt->ctl.bitrate >> 3) / 1000) * propt->outbuf_ms;
//  use capability setting to allocate buffer to instead of current bitrate 
    
    obj->outbuf.size = ((obj->cap->bitrate >> 3) / 1000) * obj->cap->buftime ;
      
    obj->outbuf.size = ALIGN32(obj->outbuf.size - 32 );
    obj->outbuf.end  = obj->outbuf.base + obj->outbuf.size;

    ulCuraddr += obj->outbuf.size;
    videohwbuf.bsbuf.ulStart = obj->outbuf.base;
    videohwbuf.bsbuf.ulEnd   = obj->outbuf.end;

	MMPD_3GPMGR_SetEncodeCompBuf(obj->enc_id, obj->outbuf.base, obj->outbuf.size);
	MMPD_VIDENC_SetBitstreamBuf(obj->enc_id, &(videohwbuf.bsbuf));
	MMPD_VIDENC_SetRefGenBound(obj->enc_id, &(videohwbuf.refgenbd));
	MMPD_VIDENC_SetMiscBuf(obj->enc_id, &(videohwbuf.miscbuf));

    if (ulCuraddr > obj->heap.end) {
        printc("\t= [HeapMemErr] Vstream =\r\n");
        return MMP_VSTREAM_ERR_MEM_EXHAUSTED;
    }

    #if (DBG_VSTREAM_MEM_MAP)
    MMPS_VStream_MemMapDbg(obj);
    #endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_MemoryMap
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign memory for a video stream object

 @param[in] obj     Video stream object

 @retval None.
*/
#if (DBG_VSTREAM_MEM_MAP)
static void MMPS_VStream_MemMapDbg(MMPS_VSTREAM_CLASS *obj)
{
    MMP_ULONG i;

    printc("=============================================================\r\n");
    printc("\tvstream %d mem map, x%X-x%X\r\n",         obj->enc_id,
                                                        obj->heap.base,
                                                        obj->heap.end);
    printc("-------------------------------------------------------------\r\n");
    printc("\tSliceLen: x%X-x%X (%d)\r\n",  obj->lenbuf.base,
                                            obj->lenbuf.end,
                                            obj->lenbuf.size);
    printc("\tMV      : x%X-x%X (%d)\r\n",  obj->mvbuf.base,
                                            obj->mvbuf.end,
                                            obj->mvbuf.size);
    for(i = 0; i < obj->inbuf.ulBufCnt; i++) {
        printc("\tIn[%d]   : x%X\r\n",      i,
                                            obj->inbuf.ulY[i]);
    }
    printc("\tRef     : x%X-x%X (%d)\r\n",  obj->refbuf.base,
                                            obj->refbuf.end,
                                            obj->refbuf.size);
    printc("\tGen     : x%X-x%X (%d)\r\n",  obj->genbuf.base,
                                            obj->genbuf.end,
                                            obj->genbuf.size);
    printc("\tComp    : x%X-x%X (%d)\r\n",  obj->outbuf.base,
                                            obj->outbuf.end,
                                            obj->outbuf.size);
    printc("=============================================================\r\n");
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_ConfigPipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Configure video stream pipe

 @param[in] *obj        Video stream object
 @param[in] *pInputBuf  Buffer for HW

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_VStream_ConfigPipe( MMPS_VSTREAM_CLASS      *obj, 
                                        MMPD_MP4VENC_INPUT_BUF 	*pInputBuf)
{
#ifdef PROJECT_Q  
#define SCAL_UP_R   (105) // 105% scaling up ( 1.05x up )
#else
#define SCAL_UP_R   (100) //  100% scaling up ( no up    )
#endif
    MMP_ULONG           ulScalInW, ulScalInH;
    MMP_SCAL_FIT_RANGE  fitrange;
    MMP_SCAL_GRAB_CTRL  grabEnc;
    MMPD_FCTL_ATTR      fctlAttr;
    MMP_USHORT          i;
    MMP_SCAL_FIT_RANGE  fitRangeBayer;
    MMP_SCAL_GRAB_CTRL  grabBayer;
    MMPS_VSTREAM_PROPT  *propt = &obj->propt;

    /* Parameter Check */
    if (!obj || !pInputBuf) {
        return MMP_VSTREAM_ERR_INVALID_PARAMETERS;
    }

    /* Get the sensor parameters */
    MMPS_Sensor_GetCurPrevScalInputRes(propt->snrID, &ulScalInW, &ulScalInH);

    MMPD_BayerScaler_GetZoomInfo(MMP_BAYER_SCAL_DOWN, &fitRangeBayer, &grabBayer);

    // Config video stream pipe
    fitrange.fitmode        = MMP_SCAL_FITMODE_OUT;
    fitrange.scalerType 	= MMP_SCAL_TYPE_SCALER;
    //#if SCAL_UP_R > 100
    //fitrange.ulInWidth 	    = (ulScalInW*100) / SCAL_UP_R;
    //fitrange.ulInHeight	    = (ulScalInH*100) / SCAL_UP_R;
    //#else
    fitrange.ulInWidth 	    = ulScalInW;
    fitrange.ulInHeight	    = ulScalInH;    
    //#endif
    fitrange.ulOutWidth     = propt->w;
    fitrange.ulOutHeight    = propt->h;
    fitrange.ulInGrabX 		= 1;
    fitrange.ulInGrabY 		= 1;
    fitrange.ulInGrabW 		= fitrange.ulInWidth;
    fitrange.ulInGrabH 		= fitrange.ulInHeight;

    MMPD_PTZ_InitPtzInfo(obj->pipe.scalerpath,
                        fitrange.fitmode,
                        fitrange.ulInWidth, fitrange.ulInHeight,
                        fitrange.ulOutWidth, fitrange.ulOutHeight);
    
    MMPD_PTZ_InitPtzRange(obj->pipe.scalerpath ,2,100,5,-5,5,-5);
    MMPD_PTZ_CalculatePtzInfo(obj->pipe.scalerpath, SCAL_UP_R-100 , 0, 0);
    
    MMPD_PTZ_GetCurPtzInfo(obj->pipe.scalerpath, &fitrange, &grabEnc);
    fctlAttr.colormode          = MMP_DISPLAY_COLOR_YUV420_INTERLEAVE;
    fctlAttr.fctllink           = obj->pipe;
    fctlAttr.fitrange           = fitrange;
    fctlAttr.grabctl            = grabEnc;
    fctlAttr.scalsrc            = MMP_SCAL_SOURCE_ISP;
    fctlAttr.bSetScalerSrc      = MMP_TRUE;
    fctlAttr.ubPipeLinkedSnr    = propt->snrID;
    fctlAttr.usBufCnt           = pInputBuf->ulBufCnt;
    fctlAttr.bUseRotateDMA      = MMP_FALSE;

    for (i = 0; i < fctlAttr.usBufCnt; i++) {
        if (pInputBuf->ulY[i] != 0)
            fctlAttr.ulBaseAddr[i] = pInputBuf->ulY[i];
        if (pInputBuf->ulU[i] != 0)
            fctlAttr.ulBaseUAddr[i] = pInputBuf->ulU[i];
        if (pInputBuf->ulV[i] != 0)
            fctlAttr.ulBaseVAddr[i] = pInputBuf->ulV[i];
    }

    MMPD_IBC_SetH264RT_Enable(MMP_FALSE);
    MMPD_Fctl_SetPipeAttrForH264FB(&fctlAttr);
    MMPD_Fctl_LinkPipeToVideo(obj->pipe.ibcpipeID, obj->enc_id);

    // Tune MCI priority of encode pipe for frame based mode
    MMPD_VIDENC_TuneEncodePipeMCIPriority(obj->pipe.ibcpipeID);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_EnablePipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on or off pipe for video stream.

 @param[in] *obj    Video stream object
 @param[in] bEnable Enable and disable pipe path for video stream.
 
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPS_VStream_EnablePipe( MMPS_VSTREAM_CLASS  *obj,
                                        MMP_BOOL            bEnable)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_IBC_PIPEID ibcID = obj->pipe.ibcpipeID;
    MMPS_VSTREAM_PROPT *propt = &obj->propt;

    if ((obj->pipe_en == MMP_FALSE) && bEnable)
        err = MMPD_Fctl_EnablePreview(propt->snrID, ibcID, bEnable, MMP_FALSE);
    else if (obj->pipe_en && (bEnable == MMP_FALSE))
        err = MMPD_Fctl_EnablePreview(propt->snrID, ibcID, bEnable, MMP_FALSE);

    obj->pipe_en = bEnable;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_OpenEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on the video stream encoder

 @param[in] *obj        Video stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_VStream_OpenEncoder(MMPS_VSTREAM_CLASS *obj)
{
    MMP_ERR             err = MMP_ERR_NONE;
    MMP_ULONG           enc_id = 0;
    MMPD_MP4VENC_FW_OP  status_vid;
    MMP_ULONG           EncWidth, EncHeight;
    MMP_ULONG           ulTimeout = 100000;
    MMPS_VSTREAM_PROPT  *propt = &obj->propt;
    VIDENC_CTL_SET      *ctl = &obj->propt.ctl;

    enc_id = obj->enc_id;

    EncWidth  = ALIGN16(propt->w);
    EncHeight = ALIGN16(propt->h);

    MMPD_VIDENC_GetStatus(obj->enc_id, &status_vid);

    if ((status_vid == MMPD_MP4VENC_FW_OP_NONE) ||
        (status_vid == MMPD_MP4VENC_FW_OP_STOP))
    {
        if (MMPS_VStream_AssignBuf(obj)) {
            printc("Alloc mem for vstream failed\r\n");
            #if (DBG_VSTREAM_MEM_MAP)
            MMPS_VStream_MemMapDbg(obj);
            #endif
            return MMP_VSTREAM_ERR_MEM_EXHAUSTED;
        }

        MMPS_VStream_ConfigPipe(obj, &obj->inbuf);
        if (MMPS_VStream_EnablePipe(obj, MMP_TRUE) != MMP_ERR_NONE) {
            printc("Enable Video Record: Fail\r\n");
            return MMP_VSTREAM_ERR_PIPE;
        }

        // Set current buf mode
        MMPD_VIDENC_SetCurBufMode(enc_id, MMPD_MP4VENC_CURBUF_FRAME);

        // Encoder parameters
        MMPD_VIDENC_SetCropping(enc_id, 0, EncHeight - propt->h,
                                        0, EncWidth - propt->w);
        MMPD_VIDENC_SetPadding(enc_id, MMPD_H264ENC_PADDING_NONE, 0);

        if (MMPD_VIDENC_SetResolution(enc_id, EncWidth, EncHeight)) {
            err = MMP_VSTREAM_ERR_UNSUPPORTED_PARAMETERS;
            goto _open_enc_err;
        }

        if (MMPD_VIDENC_SetProfile(enc_id, ctl->profile)) {
            err = MMP_VSTREAM_ERR_UNSUPPORTED_PARAMETERS;
            goto _open_enc_err;
        }
        MMPD_VIDENC_SetLevel(enc_id, ctl->level);
        MMPD_VIDENC_SetEntropy(enc_id, ctl->entropy);

        MMPD_VIDENC_SetGOP(enc_id, ctl->gop - 1, 0);

        // Bitrate control
        MMPD_VIDENC_SetRcMode(enc_id, ctl->rc_mode);
        MMPD_VIDENC_SetBitrate(enc_id, ctl->bitrate);
        MMPD_VIDENC_SetTargetBitrate(enc_id, ctl->target_bitrate,ctl->rampup_bitrate);

        MMPD_VIDENC_SetInitalQP(enc_id, ctl->qp_init);
        MMPD_VIDENC_SetQPBoundary(enc_id, ctl->qp_bound[0], ctl->qp_bound[1]);

        // Rate control features
        MMPD_VIDENC_SetRcSkip(enc_id, ctl->rc_skip);
        MMPD_VIDENC_SetRcSkipType(enc_id, ctl->rc_skiptype);
        MMPD_VIDENC_SetRcLbSize(enc_id, ctl->lb_size);
        MMPD_VIDENC_SetTNR(enc_id, ctl->tnr);

        // Sensor input frame rate
    	MMPD_VIDENC_SetSnrFrameRate(enc_id, propt->snr_fps.ulIncr,
                                            propt->snr_fps.ulResol);
        // Encode frame rate
        MMPD_VIDENC_SetEncFrameRate(enc_id, ctl->enc_fps.ulIncr,
                                            ctl->enc_fps.ulResol);

        
        
        // Set RT stream start I frames
        MMPF_VStream_SetStreamRTStartIFrames( ctl->start_i_frames ) ;

        MMPD_VIDENC_EnableClock(MMP_TRUE);

     	if (MMPD_3GPMGR_StartCapture(obj->enc_id) != MMP_ERR_NONE) {
            MMPD_VIDENC_EnableClock(MMP_FALSE);
            err = MMP_3GPRECD_ERR_GENERAL_ERROR;
            goto _open_enc_err;
        }
        do {
            MMPD_VIDENC_GetStatus(obj->enc_id, &status_vid);
            MMPF_OS_Sleep(1); // prevent lock
        } while ((status_vid != MMPD_MP4VENC_FW_OP_START) && (--ulTimeout) > 0);

        if (ulTimeout == 0) {
            RTNA_DBG_Str(0, "Vstream start timeout...\r\n");
            err = MMP_VSTREAM_ERR_OPERATION;
        }
    }
    else {
        return MMP_VSTREAM_ERR_STATE;
    }

_open_enc_err:
    if (err)
        MMPS_VStream_EnablePipe(obj, MMP_FALSE);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_CloseEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn off the video stream encoder

 @param[in] *obj        Video stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_VStream_CloseEncoder(MMPS_VSTREAM_CLASS *obj)
{
    MMPD_MP4VENC_FW_OP  status_vid;

    MMPD_VIDENC_GetStatus(obj->enc_id, &status_vid);

    if ((status_vid == MMPD_MP4VENC_FW_OP_START) ||
        (status_vid == MMPD_MP4VENC_FW_OP_PAUSE) ||
        (status_vid == MMPD_MP4VENC_FW_OP_RESUME) ||
        (status_vid == MMPD_MP4VENC_FW_OP_STOP)) {

        MMPD_3GPMGR_StopCapture(obj->enc_id);

        do {
            MMPD_VIDENC_GetStatus(obj->enc_id, &status_vid);
            MMPF_OS_Sleep(10);//prevent lock
        } while (status_vid != MMPD_MP4VENC_FW_OP_STOP);

        MMPS_VStream_EnablePipe(obj, MMP_FALSE);

        MMPD_VIDENC_EnableClock(MMP_FALSE);
    }
    else {
        return MMP_VSTREAM_ERR_STATE;
    }

    return MMP_ERR_NONE;
}

//#pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall" 
//#pragma O0
ait_module_init(MMPS_VStream_ModInit);
//#pragma
//#pragma arm section rodata, rwdata, zidata

/// @}
