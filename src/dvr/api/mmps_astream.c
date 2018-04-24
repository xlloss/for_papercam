/**
 @file mmps_astream.c
 @brief Audio Stream Control Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmps_astream.h"
#include "mmpf_system.h"
#include "mmpf_astream.h"

#include "aac_encoder.h"
#if SRC_SUPPORT
#include "mmpf_src.h"
#endif
/** @addtogroup MMPS_ASTREAM
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
static MMPS_ASTREAM_CLASS   m_AStream[MAX_AUD_STREAM_NUM];

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPS_AStream_ReserveHeap(void);
static MMP_ERR MMPS_AStream_AssignBuf(  MMPS_ASTREAM_CLASS  *obj,
                                        ST_AUDIO_CAP        *feat,
                                        MMP_AUD_ENCBUF      *buf);
#if (DBG_ASTREAM_MEM_MAP)
static void    MMPS_AStream_MemMapDbg(  MMPS_ASTREAM_CLASS  *obj);
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____AStream_Module_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize audio stream module

 @retval It reports the status of the operation.
*/
int MMPS_AStream_ModInit(void)
{
    static MMP_BOOL _init = MMP_FALSE;
    MMP_ULONG s;
    MMPS_ASTREAM_CLASS *obj = NULL;

    if (_init == MMP_FALSE) {

        for(s = 0; s < MAX_AUD_STREAM_NUM; s++) {
            obj = &m_AStream[s];
            MEMSET(obj, 0, sizeof(MMPS_ASTREAM_CLASS));
            if (s < gulMaxAStreamNum)
                obj->cap = &gstAStreamCap[s];
        }

        MMPS_AStream_ReserveHeap(); // Reserve heap buffer for each stream

        _init = MMP_TRUE;
    }
    return 0 ;
}

#if 0
void ____AStream_Obj_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of audio stream object

 @param[in] cap     Requested feature of a audio stream

 @retval The object instance of audio streamer
*/
MMPS_ASTREAM_CLASS *MMPS_AStream_Open(MMPS_ASTREAM_PROPT *propt)
{
    MMP_ERR err;
    MMP_ULONG s;
    MMP_AUD_ENCBUF enc_buf;
    ST_AUDIO_CAP *feat = &propt->feat;
    MMPS_ASTREAM_CLASS *obj = NULL;


    if (!feat)
    	return NULL;

    /* Search a stream whose capability is best-fit with the request feature */
    for(s = 0; s < MAX_AUD_STREAM_NUM; s++) {
        if ((m_AStream[s].state == IPC_STREAMER_IDLE) &&
            (m_AStream[s].cap->src == feat->src))
        {
            if ((m_AStream[s].cap->fs == feat->fs) &&
                (m_AStream[s].cap->bitrate == feat->bitrate)) {
                obj = &m_AStream[s];
                break;
            }
            else if ((m_AStream[s].cap->fs >= feat->fs) &&
                     (m_AStream[s].cap->bitrate >= feat->bitrate))
            {
                if (!obj) {
                    obj = &m_AStream[s];
                }
                else if ((m_AStream[s].cap->fs < obj->cap->fs) ||
                        (m_AStream[s].cap->bitrate < obj->cap->bitrate))
                {
                    obj = &m_AStream[s];
                    break;
                }
            }
            break;
        }
    }

    if (!obj)
        return NULL;

    /* New an astreamer instance */
    obj->propt = *propt;
    obj->id = MMPF_AStream_New(feat);
    if (obj->id >= MAX_AUD_STREAM_NUM) {
        printc("AStream open err\r\n");
        return NULL;
    }

    /* Open encoder */
    err = MMPS_AStream_AssignBuf(obj, feat, &enc_buf);
    if (err) {
        printc("AStream buf err x%X\r\n", err);
        #if (DBG_ASTREAM_MEM_MAP)
        MMPS_AStream_MemMapDbg(obj);
        #endif
        return NULL;
    }
    err = MMPF_AStream_OpenEncoder(obj->id, &enc_buf);
    if (err) {
        printc("AStream openEnc err x%X\r\n", err);
        goto _enc_open_err;
    }

    /* Open ADC module */
    err = MMPF_AStream_OpenADC(obj->id, feat->src);
    if (err) {
        printc("AStream openADC err x%X\r\n", err);
        goto _adc_open_err;
    }

    /* open SRC module*/
    #if SRC_SUPPORT
    printc("[SRC][%d] : %d <- %d Hz\r\n",obj->id,MMPF_AStream_GetEncoderFs(obj->id),MMPF_AStream_GetADCFs(obj->id));
    if ( MMPF_AStream_GetEncoderFs(obj->id) > MMPF_AStream_GetADCFs(obj->id) ) {
       MMPF_SRC_Init(obj->id,2);
    }
    
    #endif  
    
    obj->state = IPC_STREAMER_OPEN;
    return obj;

_adc_open_err:
    MMPF_AStream_CloseEncoder(obj->id);

_enc_open_err:
    MMPF_AStream_Delete(obj->id);

    return NULL;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of audio stream object

 @param[in] obj     Audio stream object
 @param[in] v4l2_id Corresponded V4L2 stream id
 @param[in] opt     Option of streaming

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_AStream_Start( MMPS_ASTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!obj)
		return MMP_ASTREAM_ERR_PARAMETER;
	if (obj->state != IPC_STREAMER_OPEN)
		return MMP_ASTREAM_ERR_STATE;

    err = MMPF_AStream_Start(obj->id, v4l2_id, opt);
    if (err) {
        printc("AStream start err x%X\r\n", err);
        return err;
    }

    obj->state = IPC_STREAMER_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the audio stream

 @param[in] obj     Audio stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_AStream_Stop(MMPS_ASTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
		return MMP_ASTREAM_ERR_PARAMETER;
	if (obj->state != IPC_STREAMER_START)
		return MMP_ASTREAM_ERR_STATE;

    err = MMPF_AStream_Stop(obj->id);
    if (err) {
        printc("AStream stop err x%X\r\n", err);
        return err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close a instance of audio stream object

 @param[in] obj     Audio stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_AStream_Close(MMPS_ASTREAM_CLASS *obj)
{
    if (!obj)
        return MMP_ASTREAM_ERR_PARAMETER;
    if (obj->state != IPC_STREAMER_OPEN)
        return MMP_ASTREAM_ERR_STATE;

    /* Close ADC module */
    MMPF_AStream_CloseADC(obj->id);
    #if SRC_SUPPORT
    /* Close SRC */
    MMPF_SRC_Disable(obj->id);
    #endif
    /* Close encoder */
    MMPF_AStream_CloseEncoder(obj->id);
    /* Release the astreamer instance */
    MMPF_AStream_Delete(obj->id);

    obj->state = IPC_STREAMER_IDLE;

    return MMP_ERR_NONE;
}

#if 0
void ____AStream_Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for each audio stream.
*/
static MMP_ERR MMPS_AStream_ReserveHeap(void)
{
    MMP_ULONG s;
    MMP_ULONG heap_sz = 0;
    ST_AUDIO_CAP *cap;
    MMPS_ASTREAM_CLASS *obj;

    for(s = 0; s < gulMaxAStreamNum; s++) {
        obj = &m_AStream[s];
        cap = obj->cap;

        if (cap) {
            switch(cap->fmt) {
            case MMP_ASTREAM_AAC:
                heap_sz  = AACENC_WORKBUF_SIZE;  // working buffer
                heap_sz += AACENC_INFRM_SAMPLES * AACENC_INBUF_SLOT * 2;// in buf
                heap_sz += ((cap->bitrate >> 3) * cap->buftime) / 1000; // out buf
                heap_sz += AACENC_MAX_FRM_SIZE;  // out frame buffer

                obj->heap.size = heap_sz;
                obj->heap.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                     obj->heap.size, 32);
                if (obj->heap.base == SYS_HEAP_MEM_INVALID)
                    return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
                obj->heap.end  = obj->heap.base + obj->heap.size;
                break;
            case MMP_ASTREAM_PCM:
                obj->heap.size = 0;
                obj->heap.base = 0;
                obj->heap.end  = 0;
                break;
            default:
                printc("Unsupported astream fmt %d\r\n", cap->fmt);
                break;
            }
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_VStream_AssignBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign buffers for a audio stream object

 @param[in] obj     Audio stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_AStream_AssignBuf(  MMPS_ASTREAM_CLASS  *obj,
                                        ST_AUDIO_CAP        *feat,
                                        MMP_AUD_ENCBUF      *buf)
{
    MMP_ULONG cur = obj->heap.base;

    switch(feat->fmt) {
    case MMP_ASTREAM_AAC:
        /* Working buffer */
        buf->work.addr = cur;
        buf->work.size = AACENC_WORKBUF_SIZE;
        cur += buf->work.size;
        if ((buf->work.addr + buf->work.size) > obj->heap.end)
            return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
        /* Input PCM buffer */
        buf->in.addr = cur;
        buf->in.size = AACENC_INFRM_SAMPLES * AACENC_INBUF_SLOT * 2;
        cur += buf->in.size;
        if ((buf->in.addr + buf->in.size) > obj->heap.end)
            return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
        /* Output bitstream buffer */
        buf->out.addr = cur;
        buf->out.size = ((feat->bitrate >> 3) * feat->buftime) / 1000;
        cur += buf->out.size;
        if ((buf->out.addr + buf->out.size) > obj->heap.end)
            return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
        /* Output frame buffer */
        buf->frm.addr = cur;
        buf->frm.size = AACENC_MAX_FRM_SIZE;
        cur += buf->frm.size;
        if ((buf->frm.addr + buf->frm.size) > obj->heap.end)
            return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
        break;
    case MMP_ASTREAM_PCM:
    default:
        printc("Unsupported astream fmt\r\n");
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    #if (DBG_ASTREAM_MEM_MAP)
    MMPS_AStream_MemMapDbg(obj);
    #endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_AStream_MemoryMap
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign memory for a audio stream object

 @param[in] obj     Audio stream object

 @retval None.
*/
#if (DBG_ASTREAM_MEM_MAP)
static void MMPS_AStream_MemMapDbg(MMPS_ASTREAM_CLASS *obj)
{
    printc("=============================================================\r\n");
    printc("\tastream %d mem map, x%X-x%X\r\n", obj->id,
                                                obj->heap.base,
                                                obj->heap.end);
    printc("=============================================================\r\n");
}
#endif
//#pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall" 
//#pragma O0
ait_module_init(MMPS_AStream_ModInit);
//#pragma
//#pragma arm section rodata, rwdata, zidata

/// @}
