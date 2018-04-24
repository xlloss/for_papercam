/**
 @file mmpf_astream.c
 @brief Control functions of Audio Streamer
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmph_hif.h"
#include "mmu.h"

#include "mmpf_system.h"
#include "mmpf_dma.h"
#include "mmpf_alsa.h"
#include "mmpf_astream.h"
#if (SUPPORT_AEC)
#include "mmpf_aec.h"
#endif
#if (SRC_SUPPORT)
#include "mmpf_src.h"
#endif
#include "aac_encoder.h"
#include "dualcpu_v4l2.h"

/** @addtogroup MMPF_ASTREAM
@{
*/
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
static MMPF_ASTREAM_CLASS   m_AStreamObj[MAX_AUD_STREAM_NUM];

static MMPF_OS_SEMID        m_DmaSemID;

/*
 * External Variables
 */
extern MMPF_OS_FLAGID       DSC_UI_Flag;

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define ASTREAM_OBJ(id)     (&m_AStreamObj[id])
#define ASTREAM_ID(obj)     (obj - &m_AStreamObj[0])

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static void     MMPF_AStream_Encode(            MMPF_ASTREAM_CLASS  *aobj);

static void     MMPF_AStream_Reset(             MMPF_ASTREAM_CLASS  *aobj);

#if (V4L2_AAC)
extern MMP_BOOL aitcam_ipc_send_frame(  MMP_ULONG   v4l2_id,
                                        MMP_ULONG   size,
                                        MMP_ULONG   ts);
extern MMP_ULONG aitcam_ipc_get_slot(    MMP_ULONG   v4l2_id,
                                        MMP_ULONG   *slot);
#endif

extern MMP_ULONG MMPF_Streamer_GetBaseTime(void);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____ASTREAM_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_New
//  Description :
//------------------------------------------------------------------------------
/**
 @brief New a audio streamer

 @param[in] feat    Request feature of the audio stream

 @retval Id of the new astream.
*/
MMP_ULONG MMPF_AStream_New(ST_AUDIO_CAP *feat)
{
    MMP_ULONG i;
    MMPF_ASTREAM_CLASS *aobj = NULL;

    for(i = 0; i < MAX_AUD_STREAM_NUM; i++) {
        if (m_AStreamObj[i].state == ASTREAM_STATE_NONE) {
            aobj = &m_AStreamObj[i];
            aobj->state = ASTREAM_STATE_OPEN;
            break;
        }
    }

    if (!aobj) {
        RTNA_DBG_Str(0, "AStream no obj\r\n");
        return MAX_AUD_STREAM_NUM;
    }

    /* Assign AStream properties, and enable encoder */
    aobj->fmt       = feat->fmt;
    aobj->fs        = feat->fs;
    aobj->bitrate   = feat->bitrate;

    return ASTREAM_ID(aobj);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_Delete
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release the audio streamer

 @param[in] id      Id of astream object

 @retval None.
*/
void MMPF_AStream_Delete(MMP_ULONG id)
{
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    if (aobj->state != ASTREAM_STATE_OPEN) {
        RTNA_DBG_Str0("Stop astream first!\r\n");
    }

    MMPF_AStream_Reset(ASTREAM_OBJ(id));
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_OpenADC
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open the ADC module

 @param[in] id      Id of associated astream object
 @param[in] path    ADC path

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_AStream_OpenADC(MMP_ULONG id, MMP_AUD_INOUT_PATH path)
{
    MMP_ERR err;
    MMPF_ADC_CLASS *adc = NULL;
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    /* Initialize ADC module, and turn on it */
    adc = MMPF_ADC_InitModule(path, aobj->fs);
    if (!adc) {
        RTNA_DBG_Str(0, "AStream_InitMod err\r\n");
        return MMP_ASTREAM_ERR_ADC;
    }

    aobj->adc = adc;

    err = MMPF_ADC_OpenModule(aobj->adc);
    if (err) {
        RTNA_DBG_Str(0, "AStream_ModOpen:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
        MMPF_ADC_UninitModule(aobj->adc);
        return err;
    }
    
    err = MMPF_ADC_StartModule(aobj->adc);
    if (err) {
        RTNA_DBG_Str(0, "AStream_ModOn:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
        MMPF_ADC_CloseModule(aobj->adc);
        MMPF_ADC_UninitModule(aobj->adc);
        return err;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_CloseADC
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close the ADC module

 @param[in] id      Id of associated astream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_AStream_CloseADC(MMP_ULONG id)
{
    MMP_ERR err;
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    err = MMPF_ADC_StopModule(aobj->adc);
    if (err) {
        RTNA_DBG_Str(0, "AStream_ModOff:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
    }
    err = MMPF_ADC_CloseModule(aobj->adc);
    if (err) {
        RTNA_DBG_Str(0, "AStream_ModClose:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
    }
    err = MMPF_ADC_UninitModule(aobj->adc);
    if (err) {
        RTNA_DBG_Str(0, "AStream_UninitMod:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
    }
    aobj->adc = NULL;
    return err;
}

MMP_USHORT MMPF_AStream_GetADCFs(MMP_ULONG id)
{
  MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);
  return aobj->adc->fs ;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_OpenEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on audio encoder for the specified AStream object

 @param[in] id      AStream object ID
 @param[in] buf     Buffers for the stream

 @retval None.
*/
MMP_ERR MMPF_AStream_OpenEncoder(MMP_ULONG id, MMP_AUD_ENCBUF *buf)
{
    int ret;
    MMP_AUD_ENCODER *enc;
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    enc = &aobj->enc;

    /* Acquire the encoder resource */
    switch(aobj->fmt) {
    case MMP_ASTREAM_AAC:
        ret = AACEnc_New(enc);
        break;
    case MMP_ASTREAM_PCM:
        // TODO: support PCM stream, by Alterman
    default:
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    if (ret) {
        RTNA_DBG_Str(0, "New audenc failed\r\n");
        return MMP_ASTREAM_ERR_ENCODER;
    }

    /* Use cacheable VA for better performance */
    aobj->buf = *buf;
    aobj->buf.work.addr = DRAM_CACHE_VA(aobj->buf.work.addr);
    aobj->buf.in.addr   = DRAM_CACHE_VA(aobj->buf.in.addr);
    aobj->buf.out.addr  = DRAM_CACHE_VA(aobj->buf.out.addr);
    aobj->buf.frm.addr  = DRAM_CACHE_VA(aobj->buf.frm.addr);

    /* Initialize buffers */
    AUTL_RingBuf_Init(&aobj->inbuf,  aobj->buf.in.addr,  aobj->buf.in.size);
    AUTL_RingBuf_Init(&aobj->outbuf, aobj->buf.out.addr, aobj->buf.out.size);
    AUTL_RingBuf_Init(&aobj->infobuf.queue, (MMP_ULONG)aobj->infobuf.buf,
                                            AUD_FRM_INFO_Q_DEPTH);

    /* Set encode properties */
    enc->set_propt(enc, AENC_PROPT_FS,           &aobj->fs);
    enc->set_propt(enc, AENC_PROPT_BITRATE,      &aobj->bitrate);
    enc->set_propt(enc, AENC_PROPT_WORKBUF_ADDR, &aobj->buf.work.addr);
    enc->set_propt(enc, AENC_PROPT_WORKBUF_SIZE, &aobj->buf.work.size);
    enc->set_propt(enc, AENC_PROPT_OUTPUT,       (void *)aobj->buf.frm.addr);

    /* Reset encoder & start encoder */
    ret = enc->enc_op(enc, AENC_OP_OPEN);
    if (ret) {
        RTNA_DBG_Str(0, "Open audenc failed\r\n");
        enc->enc_op(enc, AENC_OP_RELEASE);
        return MMP_ASTREAM_ERR_ENCODER;
    }

    return MMP_ERR_NONE;
}

MMP_USHORT MMPF_AStream_GetEncoderFs(MMP_ULONG id)
{
    MMP_AUD_ENCODER *enc;
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);
    MMP_ULONG fs;
    enc = &aobj->enc;
    enc->get_propt(enc, AENC_PROPT_FS,           (void*)&fs);
    //printc("[AS].get fs:%x,%d\r\n",&fs,fs);
    
    return (MMP_USHORT)fs;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_CloseEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn off audio encoder for the specified AStream object

 @param[in] id      AStream object ID

 @retval None.
*/
MMP_ERR MMPF_AStream_CloseEncoder(MMP_ULONG id)
{
    int ret;
    MMP_ERR err = MMP_ERR_NONE;
    MMP_AUD_ENCODER *enc;
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    enc = &aobj->enc;

    /* Close encoder & release encoder resource */
    ret = enc->enc_op(enc, AENC_OP_CLOSE);
    if (ret) {
        RTNA_DBG_Str(0, "Close aud enc failed\r\n");
        err = MMP_ASTREAM_ERR_ENCODER;
    }
    enc->enc_op(enc, AENC_OP_RELEASE);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start streaming the specified AStream object

 @param[in] id      AStream object ID
 @param[in] v4l2_id Corresponded V4L2 stream id
 @param[in] opt     Option of streaming

 @retval None.
*/
MMP_ERR MMPF_AStream_Start( MMP_ULONG           id,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    if (aobj->state != ASTREAM_STATE_OPEN)
        return MMP_ASTREAM_ERR_STATE;

    aobj->mode      = IPC_STREAMER_MODE_NONE;
    aobj->opt       = opt;
    aobj->v4l2_id   = v4l2_id;
    aobj->state     = ASTREAM_STATE_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop streaming the specified AStream object

 @param[in] id      AStream object ID

 @retval None.
*/
MMP_ERR MMPF_AStream_Stop(MMP_ULONG id)
{
    MMPF_ASTREAM_CLASS *aobj = ASTREAM_OBJ(id);

    if (aobj->state != ASTREAM_STATE_START)
        return MMP_ASTREAM_ERR_STATE;

    aobj->state = ASTREAM_STATE_OPEN;

    return MMP_ERR_NONE;
}


#if 0
void _____ASTREAM_INTERNAL_FUNC_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_PushFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Save the frame info. into the queue.

 @param[in] aobj    AStream object
 @param[in] size    Encoded frame size
 @param[in] time    Encoded frame timestamp

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_AStream_PushFrameInfo(  MMPF_ASTREAM_CLASS  *aobj,
                                            MMP_ULONG           size,
                                            MMP_ULONG           time)
{
    MMP_ULONG free;
    AUDIO_FRM_INFO  *pInfo = NULL;

    AUTL_RingBuf_SpaceAvailable(&aobj->infobuf.queue, &free);
    if (free >= 1) {
        pInfo = &aobj->infobuf.buf[aobj->infobuf.queue.ptr.wr];
        pInfo->size = size;
        pInfo->dts  = time;
        AUTL_RingBuf_CommitWrite(&aobj->infobuf.queue, 1);
    }
    else {
        RTNA_DBG_Str0("No space for frame info\r\n");
        return MMP_ASTREAM_ERR_OVERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_PopFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Read out the frame info. into the queue.

 @param[in] aobj    AStream object
 @param[out] size   Encoded frame size
 @param[out] time   Encoded frame timestamp

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_AStream_PopFrameInfo(   MMPF_ASTREAM_CLASS  *aobj,
                                            MMP_ULONG           *size,
                                            MMP_ULONG           *time)
{
    MMP_ULONG available;
    AUDIO_FRM_INFO  *pInfo = NULL;

    AUTL_RingBuf_DataAvailable(&aobj->infobuf.queue, &available);
    if (available >= 1) {
        pInfo = &aobj->infobuf.buf[aobj->infobuf.queue.ptr.rd];
        *size = pInfo->size;
        *time = pInfo->dts;
        AUTL_RingBuf_CommitRead(&aobj->infobuf.queue, 1);
    }
    else {
        RTNA_DBG_Str0("No available frame info\r\n");
        return MMP_ASTREAM_ERR_UNDERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_QueryFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Query the frame info. into the queue, not update the read pointer.

 @param[in] aobj    AStream object
 @param[out] size   Encoded frame size
 @param[out] time   Encoded frame timestamp

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_AStream_QueryFrameInfo( MMPF_ASTREAM_CLASS  *aobj,
                                            MMP_ULONG           *size,
                                            MMP_ULONG           *time)
{
    MMP_ULONG available;
    AUDIO_FRM_INFO  *pInfo = NULL;

    AUTL_RingBuf_DataAvailable(&aobj->infobuf.queue, &available);
    if (available >= 1) {
        pInfo = &aobj->infobuf.buf[aobj->infobuf.queue.ptr.rd];
        *size = pInfo->size;
        *time = pInfo->dts;
    }
    else {
        return MMP_ASTREAM_ERR_UNDERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_FeedInData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Feed input data samples to the specified stream

 @param[in] aobj    AStream object
 @param[in] samples Input samples

 @retval None.
*/
static void MMPF_AStream_FeedInData(MMPF_ASTREAM_CLASS *aobj, MMP_ULONG samples)
{
    MMP_ULONG id;
    MMP_ULONG free, available, ofst2end, frmsamples;
    MMP_SHORT *buf;
    MMP_AUD_ENCODER *enc = &aobj->enc;

    id = ASTREAM_ID(aobj);

    AUTL_RingBuf_SpaceAvailable(&aobj->inbuf, &free);
    free = free >> 1;   // in unit of sample (16-bit)
    if (free >= samples) {
        ofst2end = (aobj->inbuf.size - aobj->inbuf.ptr.wr) >> 1;
        buf = (MMP_SHORT *)(aobj->inbuf.buf + aobj->inbuf.ptr.wr);
        if (ofst2end >= samples) {
            MMPF_Audio_FifoInRead(buf, samples, 2, 0);
        }
        else {
            MMPF_Audio_FifoInRead(buf, ofst2end, 2, 0);
            buf = (MMP_SHORT *)aobj->inbuf.buf;
            MMPF_Audio_FifoInRead(buf, samples - ofst2end, 2, ofst2end);
        }
        AUTL_RingBuf_CommitWrite(&aobj->inbuf, samples << 1);
        AUTL_RingBuf_DataAvailable(&aobj->inbuf, &available);
        available = available >> 1; // in unit of sample (16-bit)

        /* Trigger encode only if input samples are enough */
        enc->get_propt(enc, AENC_PROPT_FRAME_SAMPLES, &frmsamples);
        if (available < frmsamples)
            return;
    }
    else {
        RTNA_DBG_Byte0(id);
        RTNA_DBG_Str0(" Audin_OV\r\n");
    }

    MMPF_OS_SetFlags(AUD_REC_Flag, AUD_FLAG_ENC(id), MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_ShouldDropFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Decide whether the encoded frames in output buffer should be dropped by
        checking the free space of compress buffer is less or more then the
        specified threshold.

 @param[in] aobj    AStream object

 @retval None.
*/
MMP_BOOL MMPF_AStream_ShouldFrameSkip(MMPF_ASTREAM_CLASS *aobj)
{
    MMP_ERR err;
    MMP_ULONG size, time, base;
    MMP_ULONG free, thr;

    /* Drop audio frames with timestamp is less than base time */
    base = MMPF_Streamer_GetBaseTime();
    err = MMPF_AStream_QueryFrameInfo(aobj, &size, &time);
    if (err)
        return MMP_FALSE;

    if (time < base)
        return MMP_TRUE;

    /* Or, if the free space is less than 15% */
    AUTL_RingBuf_SpaceAvailable(&aobj->outbuf, &free);
    thr = aobj->outbuf.size / 10; // 10 %

    if (free < thr)
        return MMP_TRUE;

	return MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_StoreOutData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Store the encoded data into bitstream buffer

 @param[in] aobj    AStream object

 @retval None.
*/
static MMP_ERR MMPF_AStream_StoreOutData(MMPF_ASTREAM_CLASS *aobj)
{
    MMP_ULONG frmsize, frmtime, free, ofst2end;
    MMP_UBYTE *dst, *src;
    MMP_AUD_ENCODER *enc = &aobj->enc;

    enc->get_propt(enc, AENC_PROPT_FRAME_SIZE, &frmsize);
    AUTL_RingBuf_SpaceAvailable(&aobj->outbuf, &free);

    if ((aobj->mode & IPC_STREAMER_MODE_RT) &&
        (aobj->state == ASTREAM_STATE_START))
    {
        MMP_ULONG available = 0;

        AUTL_RingBuf_DataAvailable(&aobj->infobuf.queue, &available);
        if (available > ASTREAM_RT_SKIP_THR)
            return MMP_ASTREAM_ERR_OVERFLOW;
    }

    if (free >= frmsize) {
        /* Copy the frame data into the out buffer */
        src = (MMP_UBYTE *)aobj->buf.frm.addr;
        dst = (MMP_UBYTE *)(aobj->outbuf.buf + aobj->outbuf.ptr.wr);

        ofst2end = aobj->outbuf.size - aobj->outbuf.ptr.wr;
        if (ofst2end >= frmsize) {
            MEMCPY(dst, src, frmsize);
        }
        else {
            MEMCPY(dst, src, ofst2end);
            dst = (MMP_UBYTE *)aobj->outbuf.buf;
            MEMCPY(dst, src + ofst2end, frmsize - ofst2end);
        }
        AUTL_RingBuf_CommitWrite(&aobj->outbuf, frmsize);

        /* Save frame information */
        enc->get_propt(enc, AENC_PROPT_FRAME_TS, &frmtime);
        MMPF_AStream_PushFrameInfo(aobj, frmsize, frmtime);
    }
    else {
        RTNA_DBG_Byte0(ASTREAM_ID(aobj));
        RTNA_DBG_Str0("#Askip\r\n");
        return MMP_ASTREAM_ERR_OVERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_Encode
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Encode one frame for the specified stream ID

 @param[in] aobj    AStream object

 @retval None.
*/
static void MMPF_AStream_Encode(MMPF_ASTREAM_CLASS *aobj)
{
    int ret;
    MMP_ERR err;
    MMP_SHORT *in;
    MMP_ULONG id;
    MMP_ULONG in_samples, available;
    int out_samples;
    MMP_AUD_ENCODER *enc;

    id  = ASTREAM_ID(aobj);
    enc = &aobj->enc;

    /* Check whether the input samples are enough for encoding or not */
    AUTL_RingBuf_DataAvailable(&aobj->inbuf, &available);
    available = available >> 1; // in unit of sample (16-bit)
    enc->get_propt(enc, AENC_PROPT_FRAME_SAMPLES, (int *)&in_samples);
    #if SRC_SUPPORT
    if( MMPF_SRC_IsEnable(id) ) {
      in_samples = in_samples >> 1 ;
    }
    #endif
    if (available < in_samples)
        return;

    /* Specified input data address */
    in = (MMP_SHORT *)(aobj->inbuf.buf + aobj->inbuf.ptr.rd);
    #if SRC_SUPPORT
    if( MMPF_SRC_IsEnable(id) ) {
      in = MMPF_SRC_Process(id, in , in_samples, &out_samples);
    }
    #endif
    
    enc->set_propt(enc, AENC_PROPT_INPUT, in);

    /* Encode one frame */
    ret = enc->encode(enc);

    /* Consume the input samples */
    //enc->get_propt(enc, AENC_PROPT_FRAME_SAMPLES, &in_samples);
    
    AUTL_RingBuf_CommitRead(&aobj->inbuf, in_samples << 1);
    AUTL_RingBuf_DataAvailable(&aobj->inbuf, &available);
    available = available >> 1; // in unit of sample (16-bit)

    /* Copy the encoded data into */
    err = MMPF_AStream_StoreOutData(aobj);

    /* Trigger streamer to send audio frames */
    MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_ASTREAM(id), MMPF_OS_FLAG_SET);

    /* Trigger encode again if there are samples remained */
    if ((available >= in_samples) && (err = MMP_ERR_NONE))
        MMPF_OS_SetFlags(AUD_REC_Flag, AUD_FLAG_ENC(id), MMPF_OS_FLAG_SET);
}

#if (V4L2_AAC_DBG == 0)
//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_ReleaseDma
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Callback function to release DMA semaphore.

 @param[in] argu    Callback argument, no used

 @retval None.
*/
static void MMPF_AStream_ReleaseDma(void *argu)
{
	if (MMPF_OS_ReleaseSem(m_DmaSemID) != OS_NO_ERR) {
		RTNA_DBG_Str0("m_DmaSemID OSSemPost: Fail\r\n");
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_DmaMoveData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Copy audio data to another buffer by move DMA.

 @param[in] src     Source address of data
 @param[in] dst     Destination address
 @param[in] sz      Data size

 @retval None.
*/
static void MMPF_AStream_DmaMoveData(MMP_ULONG src, MMP_ULONG dst, MMP_ULONG sz)
{
    if (sz == 0)
        return;

	if (MMPF_DMA_MoveData(src, dst, sz, MMPF_AStream_ReleaseDma,
	                      0, MMP_FALSE, NULL))
    {
        RTNA_DBG_Str(0, "MoveData fail\r\n");
        return;
	}
	if (MMPF_OS_AcquireSem(m_DmaSemID, 10000)) {
        RTNA_DBG_Str(0, "m_DmaSemPend fail\r\n");
        return;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_MoveFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Move frame data using DMA

 @param[in] src     Source buffer
 @param[in] dst     Destination buffer
 @param[in] size    Frame size

 @retval None.
*/
static void MMPF_AStream_MoveFrame( MMPF_ASTREAM_CLASS  *aobj,
                                    MMP_ULONG           slot,
                                    MMP_ULONG           size)
{
    MMP_ULONG frm;
    MMP_ULONG sz2end;

    frm = DRAM_NONCACHE_VA(aobj->outbuf.buf + aobj->outbuf.ptr.rd);
    sz2end = aobj->outbuf.size - aobj->outbuf.ptr.rd;

    if (sz2end > size) {
        MMPF_MMU_FlushDCacheMVA(frm, size);
        MMPF_AStream_DmaMoveData(frm, slot, size);
    }
    else {
        MMPF_MMU_FlushDCacheMVA(frm, sz2end);
        MMPF_AStream_DmaMoveData(frm, slot, sz2end);
        frm = DRAM_NONCACHE_VA(aobj->outbuf.buf);
        MMPF_MMU_FlushDCacheMVA(frm, size - sz2end);
        MMPF_AStream_DmaMoveData(frm, slot + sz2end, size - sz2end);
    }
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_CompBufUsage
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get current compress buffer usage, in percentage.

 @param[in] aobj    AStream object

 @retval Usage percentage of compress buffer.
*/
#if 0
static MMP_ULONG MMPF_AStream_CompBufUsage(MMPF_ASTREAM_CLASS *aobj)
{
    MMP_ULONG used = 0, usage;
    MMP_ULONG rd, wr, rd_wrap, wr_wrap;
    AUTL_RINGBUF *ring = &aobj->outbuf;
    OS_CRITICAL_INIT();

    // get the snapshot of queue pointer
    OS_ENTER_CRITICAL();
    rd      = ring->ptr.rd;
    wr      = ring->ptr.wr;
    rd_wrap = ring->ptr.rd_wrap;
    wr_wrap = ring->ptr.wr_wrap;
    OS_EXIT_CRITICAL();

    if (wr_wrap > rd_wrap) {
        if (wr == rd)
            used = ring->size;
        else if (wr < rd)
            used = ring->size - rd + wr;
    }
    else if (wr_wrap == rd_wrap) {
        if (wr == rd)
            used = 0;
        else if (wr > rd)
            used = wr - rd;
    }

    usage = (used * 100) / ring->size;
    //printc("usage: %d%\r\n", usage);

    return usage;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_FlushData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Flush all video data in compress buffer.

 @param[in] vobj    VStream object

 @retval None.
*/
static void MMPF_AStream_FlushData(MMPF_ASTREAM_CLASS *aobj)
{
    OS_CRITICAL_INIT();

    OS_ENTER_CRITICAL();
    /* flush bitstream buffer */
    AUTL_RingBuf_Flush(&aobj->outbuf);

    /* flush info queue buffer */
    AUTL_RingBuf_Flush(&aobj->infobuf.queue);
    OS_EXIT_CRITICAL();
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_TransferFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer frames via V4L2 interface

 @param[in] aobj    AStream object

 @retval None.
*/
static MMP_ERR MMPF_AStream_TransferFrame(MMPF_ASTREAM_CLASS *aobj)
{
    MMP_ERR   err;
    MMP_ULONG available, size, time;
    MMP_ULONG freesize=0;
    
    #if (V4L2_AAC_DBG == 0)
    MMP_ULONG id, slotbuf;
    #endif

    AUTL_RingBuf_DataAvailable(&aobj->infobuf.queue, &available);
    if (available == 0)
        return MMP_ASTREAM_ERR_NODATA;

    if (aobj->state == ASTREAM_STATE_OPEN) {
        while(MMPF_AStream_ShouldFrameSkip(aobj)) {
            /* Drop oldest frames in buffer */
            err = MMPF_AStream_PopFrameInfo(aobj, &size, &time);
            if (err) {
                RTNA_DBG_Str0("PopFrmInfo err\r\n");
                return err;
            }
            AUTL_RingBuf_CommitRead(&aobj->outbuf, size);
            //RTNA_DBG_Str0("#a_drop\r\n");

            aobj->ts = time;
        }
    }
    else if (aobj->state == ASTREAM_STATE_START) {
        /* Flush all audio data in buffer? */
        if (aobj->opt & IPC_STREAMER_OPT_FLUSH) {
            MMPF_AStream_FlushData(aobj);
            aobj->opt &= ~(IPC_STREAMER_OPT_FLUSH);
            // Don't queue much bitstream in compress buffer
            aobj->mode |= IPC_STREAMER_MODE_RT;
            return MMP_ASTREAM_ERR_NODATA;
        }

        #if (V4L2_AAC_DBG == 0)
        id = ASTREAM_ID(aobj);
        freesize = aitcam_ipc_get_slot(aobj->v4l2_id, &slotbuf);
        #else
        freesize = 4096 ;
        #endif

        if (freesize) {
            MMPF_AStream_PopFrameInfo(aobj, &size, &time);
            #if (V4L2_AAC_DBG == 0)
            MMPF_AStream_MoveFrame(aobj, slotbuf, size);
            #endif
            AUTL_RingBuf_CommitRead(&aobj->outbuf, size);

            #if (V4L2_AAC_DBG == 0)
            
            if( aitcam_ipc_is_debug_ts() ) {
                // Inform with frame info
                MMPF_DBG_Int(ASTREAM_ID(aobj), -1);
                //MMPF_DBG_Int(info.size, -7);
                MMPF_DBG_Int(time, -6);
                if (aobj->ts) {
                    MMPF_DBG_Int(time - aobj->ts, -2);
                }
                else {
                    RTNA_DBG_Str0(" 00");
                }
                RTNA_DBG_Str0("_A\r\n");
            }
            
            
            aitcam_ipc_send_frame(aobj->v4l2_id, size, time);
            #endif

            aobj->ts = time;
            aobj->tx_cnt++;
        }
        else {
            //MMPF_OS_Sleep(5);
            //RTNA_DBG_Str(0, "A++\r\n");
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_TransferVideo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer the audio data

 @param[in] id      AStream object ID

 @retval It reports the status of the operation.
*/
void MMPF_AStream_TransferData(MMP_ULONG id)
{
    MMP_ULONG   times = 5;
    MMP_ERR     status;

    while(times--) {
        // move audio data from bitstream buffer to V4L2 slot buffers
        status = MMPF_AStream_TransferFrame(ASTREAM_OBJ(id));
        if (status != MMP_ERR_NONE)
            break;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_Reset
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset the AStream object.

 @param[in] aobj    AStream object

 @retval None.
*/
static void MMPF_AStream_Reset(MMPF_ASTREAM_CLASS *aobj)
{
    /* Reset AStream state & properties */
    aobj->state = ASTREAM_STATE_NONE;
    aobj->fmt   = MMP_ASTREAM_FMT_UNKNOWN;
    aobj->opt   = IPC_STREAMER_OPT_NONE;
    aobj->mode  = IPC_STREAMER_MODE_NONE;

    aobj->ts     = 0;
    aobj->tx_cnt = 0;

    /* Reset ADC module */
    aobj->adc   = NULL;

    /* Reset encoder */
    aobj->enc.state = AENC_STATE_INVALID;
    aobj->enc.enc_op    = NULL;
    aobj->enc.set_propt = NULL;
    aobj->enc.get_propt = NULL;
    aobj->enc.encode    = NULL;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AStream_TaksInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize routine in AStream task startup.

 @retval None.
*/
static void MMPF_AStream_TaksInit(void)
{
    MMP_ULONG i;

    m_DmaSemID = MMPF_OS_CreateSem(0);

    /* Reset all AStream objects */
    for(i = 0; i < MAX_AUD_STREAM_NUM; i++)
        MMPF_AStream_Reset(ASTREAM_OBJ(i));
}

#if 0
void _____ASTREAM_TASK_ROUTINES_____(){}
#endif

/**
 @brief Task for copying PCM data from FIFO buffer
*/
void MMPF_Audio_CriticalTask(void)
{
    MMPF_OS_FLAGS wait_flags, flags;

    RTNA_DBG_Str3("Aud_CriticalTask()\r\n");

    wait_flags = AUD_FLAG_IN_READY;

    while(1) {
        MMPF_OS_WaitFlags(  AUD_REC_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        if (flags & AUD_FLAG_IN_READY) {
            MMP_ULONG s, samples;
            MMPF_ASTREAM_CLASS *aobj = NULL;
            samples = MMPF_Audio_FifoInSamples();

            #if (SUPPORT_AEC)
            if (samples < AEC_BLOCK_SIZE)
                continue;
            #else
            if (samples == 0)
                continue;
            #endif

            #if (SUPPORT_AEC)
            samples = AEC_BLOCK_SIZE;

            if (MMPF_AEC_IsEnable()) {
                /* AEC process uses an identical block size */
                MMPF_AEC_Process();
            }
            #endif

            samples = MMPF_Alsa_Read(ALSA_ID_MIC_IN, samples);
            /* Copy data to the input buffer of each audio stream */
            for(s = 0; s < MAX_AUD_STREAM_NUM; s++) {
                aobj = ASTREAM_OBJ(s);
                if (aobj->enc.state == AENC_STATE_ON)
                    MMPF_AStream_FeedInData(aobj, samples);
            }
            /* Advance the read pointer of fifo input buffer */
            MMPF_Audio_FifoInCommitRead(samples);
        }
    }
}

/**
 @brief Task for encoding audio frames
*/
void MMPF_Audio_EncodeTask(void)
{
    MMP_ULONG i;
    MMPF_OS_FLAGS wait_flags = 0, flags;
    MMPF_ASTREAM_CLASS *aobj = NULL;

    RTNA_DBG_Str3("Aud_EncodeTask()\r\n");

    MMPF_AStream_TaksInit();

    for (i = 0; i < MAX_AUD_STREAM_NUM; i++) {
        wait_flags |= AUD_FLAG_ENC(i);
    }

    while(1) {
        MMPF_OS_WaitFlags(  AUD_REC_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        for (i = 0; i < MAX_AUD_STREAM_NUM; i++) {
            if (flags & AUD_FLAG_ENC(i)) {
                aobj = ASTREAM_OBJ(i);
                if (aobj->enc.state == AENC_STATE_ON)
                    MMPF_AStream_Encode(aobj);
            }
        }
    }
}

/// @}
