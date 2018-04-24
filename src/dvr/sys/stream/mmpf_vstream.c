/**
 @file mmpf_vstream.c
 @brief Control functions of Video Streamer
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#if (VIDEO_R_EN)
#include "mmph_hif.h"

#include "mmpf_system.h"
#include "mmpf_sensor.h"
#include "mmpf_dma.h"
#include "mmpf_fpsctl.h"
#include "mmpf_mp4venc.h"
#include "mmpf_vstream.h"

#include "ipc_cfg.h"

/** @addtogroup MMPF_VSTREAM
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
static MMPF_OS_SEMID        m_MoveDMASemID; ///< for DMA operation control
static MMPF_VSTREAM_CLASS   m_VStreamObj[MAX_VIDEO_STREAM_NUM];
static MMP_USHORT           VSTREAM_RT_CONUS_I = 1 ;

/*
 * Global Variables
 */

/*
 * External Variables
 */
extern MMPF_OS_FLAGID       VID_REC_Flag;
extern MMPF_OS_FLAGID       DSC_UI_Flag;

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define VSTREAM_OBJ(id)     (&m_VStreamObj[id])
#define VSTREAM_ID(obj)     (obj - &m_VStreamObj[0])

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

#if (V4L2_H264)
extern MMP_BOOL aitcam_ipc_send_frame(  MMP_ULONG   v4l2_id,
                                        MMP_ULONG   size,
                                        MMP_ULONG   ts);
extern MMP_ULONG aitcam_ipc_get_slot(    MMP_ULONG   v4l2_id,
                                        MMP_ULONG   *slot);
#endif

extern void MMPF_Streamer_SetBaseTime(MMP_ULONG ts);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____BASIC_BUFFER_OPERATION_____(){}
#endif

#if (V4L2_H264)&&(V4L2_H264_DBG == 0)
//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_ReleaseDma
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Callback function to release DMA semaphore.

 @param[in] argu    Callback argument, no used

 @retval None.
*/
static void MMPF_VStream_ReleaseDma(void *argu)
{
	if (MMPF_OS_ReleaseSem(m_MoveDMASemID) != OS_NO_ERR) {
		RTNA_DBG_Str(3, "m_MoveDMASemID OSSemPost: Fail\r\n");
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_DmaMoveData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Copy video data to another buffer by move DMA.

 @param[in] src     Source address of data
 @param[in] dst     Destination address
 @param[in] sz      Data size

 @retval None.
*/
static void MMPF_VStream_DmaMoveData(MMP_ULONG src, MMP_ULONG dst, MMP_ULONG sz)
{
    if ( (sz == 0) || ( (int)(sz) < 0) )
        return;

	if (MMPF_DMA_MoveData(src, dst, sz, MMPF_VStream_ReleaseDma,
	                      0, MMP_FALSE, NULL))
    {
        RTNA_DBG_Str(0, "DMA_MoveData fail\r\n");
        return;
	}
	if (MMPF_OS_AcquireSem(m_MoveDMASemID, 10000)) {
        RTNA_DBG_Str(0, "m_MoveDMASemPend fail\r\n");
        return;
	}
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_RingBufStatus
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Check current ring buffer status.

 @param[in] ring    Ring buffer

 @retval Full/Empty/Overflow/Underflow/Available
*/
static MMP_ERR MMPF_VStream_RingBufStatus(MMPF_VSTREAM_RINGBUF *ring)
{
    MMP_ULONG rd, wr, rd_wrap, wr_wrap;
    OS_CRITICAL_INIT();

    // get the snapshot of queue pointer
    OS_ENTER_CRITICAL();
    rd      = ring->rd_ptr;
    wr      = ring->wr_ptr;
    rd_wrap = ring->rd_wrap;
    wr_wrap = ring->wr_wrap;
    OS_EXIT_CRITICAL();

    if (rd_wrap < wr_wrap) {
        if ((rd_wrap + 1) == wr_wrap) {
            if (rd > wr)
                return MMP_ERR_NONE;
            else
                return MMP_3GPMGR_ERR_QUEUE_FULL;
        }
        else {
            return MMP_3GPMGR_ERR_QUEUE_OVERFLOW;
        }
    }
    else if (rd_wrap == wr_wrap) {
        if (rd < wr)
            return MMP_ERR_NONE;
        else if (rd == wr)
            return MMP_3GPMGR_ERR_QUEUE_EMPTY;
        else
            return MMP_3GPMGR_ERR_QUEUE_UNDERFLOW;
    }
    else {
        // rd_wrap > wr_wrap
        return MMP_3GPMGR_ERR_QUEUE_UNDERFLOW;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufAddr2WriteFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the address to write a new frame.

 @param[in] id      VStream object ID

 @retval Address to write a new encoded frame.
*/
MMP_ULONG MMPF_VStream_CompBufAddr2WriteFrame(MMP_ULONG id)
{
    MMPF_VSTREAM_RINGBUF *ring = &VSTREAM_OBJ(id)->compbuf;

    ring->wr_ptr = ALIGN32(ring->wr_ptr); // 32 byte align for H264 BS
    if (ring->wr_ptr >= ring->size) {
        ring->wr_ptr = 0;
        ring->wr_wrap++;
    }

	return (ring->base + ring->wr_ptr);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufAddr2ReadFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the address to read the frame.

 @param[in] vobj     VStream object
 @param[in] size2end The distance of read pointer to the buffer end

 @retval Address to read a frame in buffer.
*/
#if (V4L2_H264)
static MMP_ULONG MMPF_VStream_CompBufAddr2ReadFrame(MMPF_VSTREAM_CLASS *vobj,
                                                    MMP_ULONG          *size2end)
{
    MMPF_VSTREAM_RINGBUF *ring = &vobj->compbuf;

    ring->rd_ptr = ALIGN32(ring->rd_ptr);
    if (ring->rd_ptr >= ring->size) {
        ring->rd_ptr = 0;
        ring->rd_wrap++;
    }
	*size2end = ring->size - ring->rd_ptr;
	
    return (ring->base + ring->rd_ptr);
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufWriteAdvance
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Update the write pointer of compress buffer.

 @param[in] vobj    VStream object
 @param[in] size    Written size

 @retval None.
*/
static void MMPF_VStream_CompBufWriteAdvance(MMPF_VSTREAM_CLASS *vobj,
                                             MMP_ULONG          size) 
{
    MMPF_VSTREAM_RINGBUF *ring = &vobj->compbuf;
    OS_CRITICAL_INIT();

    OS_ENTER_CRITICAL();
    ring->wr_ptr += size;
    if (ring->wr_ptr >= ring->size) {
        ring->wr_ptr -= ring->size;
		ring->wr_wrap++;
    }
    OS_EXIT_CRITICAL();
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufReadAdvance
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Update the read pointer of compress buffer.

 @param[in] vobj    VStream object
 @param[in] size    Read size

 @retval None.
*/
static void MMPF_VStream_CompBufReadAdvance(MMPF_VSTREAM_CLASS  *vobj,
                                            MMP_ULONG           size) 
{
    MMPF_VSTREAM_RINGBUF *ring = &vobj->compbuf;
    OS_CRITICAL_INIT();

    OS_ENTER_CRITICAL();
    ring->rd_ptr += size;
    if (ring->rd_ptr >= ring->size) {
        ring->rd_ptr -= ring->size;
		ring->rd_wrap++;
    }
    OS_EXIT_CRITICAL();
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufUsage
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get current compress buffer usage, in percentage.

 @param[in] vobj    VStream object

 @retval Usage percentage of compress buffer.
*/
#if 0
static MMP_ULONG MMPF_VStream_CompBufUsage(MMPF_VSTREAM_CLASS *vobj)
{
    MMP_ULONG used = 0, usage;
    MMP_ULONG rd, wr, rd_wrap, wr_wrap;
    MMPF_VSTREAM_RINGBUF *ring = &vobj->compbuf;
    OS_CRITICAL_INIT();

    // get the snapshot of queue pointer
    OS_ENTER_CRITICAL();
    rd      = ring->rd_ptr;
    wr      = ring->wr_ptr;
    rd_wrap = ring->rd_wrap;
    wr_wrap = ring->wr_wrap;
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
    printc("usage: %d%\r\n", usage);

    return usage;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_CompBufFreeSpace
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get current compress buffer free space.

 @param[in] vobj    VStream object

 @retval Free space of compress buffer.
*/
static MMP_ULONG MMPF_VStream_CompBufFreeSpace(MMPF_VSTREAM_CLASS *vobj)
{
    MMP_ULONG rd, wr, rd_wrap, wr_wrap;
    MMPF_VSTREAM_RINGBUF *ring = &vobj->compbuf;
    OS_CRITICAL_INIT();

    // get the snapshot of queue pointer
    OS_ENTER_CRITICAL();
    rd      = ring->rd_ptr;
    wr      = ring->wr_ptr;
    rd_wrap = ring->rd_wrap;
    wr_wrap = ring->wr_wrap;
    OS_EXIT_CRITICAL();

    if (wr_wrap > rd_wrap) {
        if (wr == rd)
            return 0;
        else if (wr < rd)
            return (rd - wr);
    }
    else if (wr_wrap == rd_wrap) {
        if (wr == rd)
            return ring->size;
        else if (wr > rd)
            return (ring->size - wr + rd);
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_SaveFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Push the encoded frame info. to info queue.

 @param[in] id      VStream object ID
 @param[in] info    Information for one frame

 @retval None.
*/
void MMPF_VStream_SaveFrameInfo(MMP_ULONG id, MMPF_VSTREAM_FRAMEINFO *info)
{
    MMP_ERR     err;
    MMP_ULONG   wr;
    MMPF_VSTREAM_CLASS  *vobj = VSTREAM_OBJ(id);
    MMPF_VSTREAM_INFO_Q *q;
    OS_CRITICAL_INIT();

    q = &vobj->info_q;

    err = MMPF_VStream_RingBufStatus(&q->ring);
    if ((err == MMP_ERR_NONE) || (err == MMP_3GPMGR_ERR_QUEUE_EMPTY)) {

        wr = q->ring.wr_ptr;
        // Advance the info. ring queue by 1
        OS_ENTER_CRITICAL();
        q->ring.wr_ptr++;
        if (q->ring.wr_ptr == q->ring.size) {
            q->ring.wr_ptr = 0;
            q->ring.wr_wrap++;
        }
        OS_EXIT_CRITICAL();

        // Save the frame info into data buffer
        q->data[wr].size = info->size;
        q->data[wr].time = info->time;
        q->data[wr].type = info->type;
        #if (WORK_AROUND_EP3)
        q->data[wr].ep3  = info->ep3;
        #endif

        // Advance the write pointer of compress buffer
        MMPF_VStream_CompBufWriteAdvance(vobj, ALIGN32(info->size));

    }
    else {
        RTNA_DBG_Str(0, "PUSH FRAME_Q ERR!\r\n");
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_QueryFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Query the info. of frame in head of queue, but not pop it from queue.

 @param[in] vobj    VStream object
 @param[out] info   Information for one frame

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_VStream_QueryFrameInfo(MMPF_VSTREAM_CLASS     *vobj,
                                           MMPF_VSTREAM_FRAMEINFO *info)
{
    MMP_ERR     err;
    MMP_ULONG   rd;
    MMPF_VSTREAM_INFO_Q *q = &vobj->info_q;

    err = MMPF_VStream_RingBufStatus(&q->ring);
    if ((err == MMP_ERR_NONE) || (err == MMP_3GPMGR_ERR_QUEUE_FULL)) {

        rd = q->ring.rd_ptr;

        info->size = q->data[rd].size;
        info->time = q->data[rd].time;
        info->type = q->data[rd].type;
        #if (WORK_AROUND_EP3)
        info->ep3  = q->data[rd].ep3;
        #endif
    }
    else {
        info->size = 0;
        info->time = 0;
        info->type = 0;
        #if (WORK_AROUND_EP3)
        info->ep3  = MMP_FALSE;
        #endif

        if (err != MMP_3GPMGR_ERR_QUEUE_EMPTY) {
            RTNA_DBG_Str(0, "POP FRAME_Q ERR!\r\n");
        }
        return err;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_ReadFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Pop the info. of frame from the head of queue.

 @param[in] vobj    VStream object
 @param[out] info   Information for one frame

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_VStream_ReadFrameInfo(MMPF_VSTREAM_CLASS     *vobj,
                                          MMPF_VSTREAM_FRAMEINFO *info)
{
    MMP_ERR err;
    MMPF_VSTREAM_RINGBUF *ring = &vobj->info_q.ring;
    OS_CRITICAL_INIT();

    err = MMPF_VStream_QueryFrameInfo(vobj, info);
    if (err == MMP_ERR_NONE) {

        OS_ENTER_CRITICAL();
        ring->rd_ptr++;
        if (ring->rd_ptr == ring->size) {
            ring->rd_ptr = 0;
            ring->rd_wrap++;
        }
        OS_EXIT_CRITICAL();
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_GetFrameInfoNum
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the number of frame info. in queue.

 @param[in] vobj    VStream object

 @retval It returns the number of frame info. in queue.
*/
static MMP_ULONG MMPF_VStream_GetFrameInfoNum(MMPF_VSTREAM_CLASS *vobj)
{
    MMP_ULONG num = 0;
    MMPF_VSTREAM_RINGBUF *ring = &vobj->info_q.ring;
    OS_CRITICAL_INIT();

    OS_ENTER_CRITICAL();
    if (ring->rd_wrap == ring->wr_wrap) {
        if (ring->wr_ptr >= ring->rd_ptr)
            num = ring->wr_ptr - ring->rd_ptr;
    }
    else if (ring->wr_wrap > ring->rd_wrap) {
        if (ring->rd_ptr >= ring->wr_ptr)
            num = ring->size - ring->rd_ptr + ring->wr_ptr;
    }
    OS_EXIT_CRITICAL();

    return num;
}

#if 0
void _____BASIC_VSTREAM_CONFIG_OPERATION_____(){}
#endif
void MMPF_VStream_SetStreamRTStartIFrames(MMP_USHORT cnt)
{
    if(!cnt) cnt = 1;
    VSTREAM_RT_CONUS_I = cnt ;
    printc("--I_I : %d\r\n",VSTREAM_RT_CONUS_I);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_SetCompBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set the address & size of compress buffer.

 @param[in] id      VStream object ID
 @param[in] base    Address of compress buffer
 @param[in] size    Size of compress buffer

 @retval None.
*/
void MMPF_VStream_SetCompBuf(MMP_ULONG id, MMP_ULONG base, MMP_ULONG size)
{
    MMPF_VSTREAM_CLASS  *vobj = VSTREAM_OBJ(id);

	vobj->compbuf.base = base;
	vobj->compbuf.size = size;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_ShouldFrameSkip
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Decide whether the incoming frame should be skipped by checking the free
        space of compress buffer is less or more then the specified threshold.

 @param[in] id      VStream object ID
 @param[in] th      Threshold to determine skip or not

 @retval None.
*/
MMP_BOOL MMPF_VStream_ShouldFrameSkip(MMP_ULONG id, MMP_ULONG th)
{
    MMPF_VSTREAM_CLASS *vobj = VSTREAM_OBJ(id);

    if ((vobj->mode & IPC_STREAMER_MODE_RT) == 0) {
        if (MMPF_VStream_CompBufFreeSpace(vobj) < th)
            return MMP_TRUE;
    }
    else {
        #if (V4L2_H264_DBG == 0)
        if (MMPF_VStream_CompBufFreeSpace(vobj) < th)
            return MMP_TRUE;

        if (vobj->state == VSTREAM_STATE_OPEN) {
            if (MMPF_VStream_GetFrameInfoNum(vobj) >= VSTREAM_RT_SKIP_THR)
                return MMP_TRUE;
        }
        #endif
    }

	return MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_SkipThreshold
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the threshold for dropping the frames in compress buffer.

 @param[in] vobj    VStream object

 @retval the threshold for dropping frames in compress buffer.
*/
MMP_ULONG MMPF_VStream_SkipThreshold(MMPF_VSTREAM_CLASS *vobj)
{
    #if 1

    return MMPF_VIDENC_GetSkipThreshold(VSTREAM_ID(vobj));

    #else

    /* drop previous frames if compress buffer usage is over 85% */
    return ((vobj->compbuf.size * 15) / 100);

    #endif
}

#if 0
void _____BASIC_VSTREAM_OPERATION_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_Reset
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset the VStream object.

 @param[in] vobj    VStream object

 @retval None.
*/
static void MMPF_VStream_Reset(MMPF_VSTREAM_CLASS *vobj)
{
    MMPF_VSTREAM_RINGBUF *ring;

    vobj->state     = VSTREAM_STATE_NONE;
    vobj->tx_size   = 0;
    vobj->ts        = 0;
    vobj->opt       = IPC_STREAMER_OPT_NONE;
    vobj->mode      = IPC_STREAMER_MODE_NONE;

    // Reset compress buffer status
    ring = &vobj->compbuf;
    ring->rd_ptr = ring->wr_ptr = ring->rd_wrap = ring->wr_wrap = 0;

    // Reset frame info. queue
    ring = &vobj->info_q.ring;
    ring->base = (MMP_ULONG)vobj->info_q.data;
    ring->size = VSTREAM_INFO_Q_DEPTH;
    ring->rd_ptr = ring->wr_ptr = ring->rd_wrap = ring->wr_wrap = 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_TaskInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize routine in VStream task startup.

 @retval None.
*/
static void MMPF_VStream_TaskInit(void)
{
    MMP_ULONG i;

	m_MoveDMASemID = MMPF_OS_CreateSem(0);

    for (i = 0; i < MAX_VIDEO_STREAM_NUM; i++) {
        MMPF_VStream_Reset(VSTREAM_OBJ(i));
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_Initialization
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset VStream before staring it.

 @retval None.
*/
static void MMPF_VStream_Initialization(MMPF_VSTREAM_CLASS *vobj)
{
    MMPF_VStream_Reset(vobj);

    #if (FPS_CTL_EN)
	MMPF_FpsCtl_Init(VSTREAM_ID(vobj));
	#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_StartEnc
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start the VStream input (video encoder).

 @param[in] id      Encoder ID, mapping to VStream object ID

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_VStream_StartEnc(MMP_UBYTE encid)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_USHORT state = MMPF_MP4VENC_GetStatus(encid);
    MMPF_VIDENC_INSTANCE *pEncInst = MMPF_VIDENC_GetInstance(encid);
    MMPF_VSTREAM_CLASS   *vobj = VSTREAM_OBJ(encid);

    if ((state != MMPF_MP4VENC_FW_STATUS_RECORD) &&
        (state != MMPF_MP4VENC_FW_STATUS_STOP) &&
        (state != MMPF_MP4VENC_FW_STATUS_PREENCODE)) {
        RTNA_DBG_Str(0, "VStream start wrong state op\r\n");
        return MMP_MP4VE_ERR_WRONG_STATE_OP;
    }

    if (state != MMPF_MP4VENC_FW_STATUS_PREENCODE) {
        MMPF_VStream_Initialization(vobj);
    }

    err = MMPF_VIDENC_Start(&(pEncInst->h264e));
    if (err != MMP_ERR_NONE) {
    	pEncInst->Module    = NULL;
    	pEncInst->bInitInst = MMP_FALSE;
        RTNA_DBG_Str(0, "VStream_StartCapture:");
        RTNA_DBG_Long(0, err);
        RTNA_DBG_Str(0, "\r\n");
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_StopEnc
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the VStream input (video encoder).

 @param[in] id      Encoder ID, mapping to VStream object ID

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_VStream_StopEnc(MMP_UBYTE encid)
{
    MMPF_VIDENC_INSTANCE *pEncInst = MMPF_VIDENC_GetInstance(encid);

    return MMPF_VIDENC_Stop(&(pEncInst->h264e));
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_AddVideoFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Copy the frame in front of compress buffer into slot buffer.

 @param[in] vobj    VStream object
 @param[in] fsize   Frame size.
 @param[in] slotbuf Slot buffer address.
 @param[in] slotsize Slot buffer size ;

 @retval MMP_ERR_NONE Success.
*/
#if (V4L2_H264)&&(V4L2_H264_DBG == 0)
static MMP_ERR MMPF_VStream_AddVideoFrame(MMPF_VSTREAM_CLASS *vobj,
                                          MMP_ULONG          fsize,
                                          MMP_ULONG          slotbuf,
                                          MMP_ULONG          slotsize)
{
    MMP_ULONG frame_addr;
    MMP_ULONG size2end, size ,sendsize = fsize;
    if(fsize > slotsize) {
        printc("        [fsize :%d > slot : %d]\r\n",fsize,slotsize );
        sendsize = slotsize ;
    }
    frame_addr = MMPF_VStream_CompBufAddr2ReadFrame(vobj, &size2end);

    if (fsize /*sendsize*/ > size2end) {
        
        if( sendsize < size2end ) {  
          MMPF_VStream_DmaMoveData(frame_addr, slotbuf, sendsize);
        }
        else {
          MMPF_VStream_DmaMoveData(frame_addr, slotbuf, size2end);
        }
        MMPF_VStream_CompBufReadAdvance(vobj, size2end);
        slotbuf += size2end;
        frame_addr = MMPF_VStream_CompBufAddr2ReadFrame(vobj, &size);
        if( sendsize > size2end) {
          MMPF_VStream_DmaMoveData(frame_addr, slotbuf, sendsize - size2end);
        }
        MMPF_VStream_CompBufReadAdvance(vobj, fsize - size2end);
    }
    else {
        MMPF_VStream_DmaMoveData(frame_addr, slotbuf, sendsize);
        MMPF_VStream_CompBufReadAdvance(vobj, fsize);
    }

    return MMP_ERR_NONE;
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
static void MMPF_VStream_FlushData(MMPF_VSTREAM_CLASS *vobj)
{
    MMPF_VSTREAM_RINGBUF *ring;
    OS_CRITICAL_INIT();

    OS_ENTER_CRITICAL();
    /* sync read pointer to write pointer */
    ring = &vobj->compbuf;
    ring->rd_ptr = ring->wr_ptr;
    ring->rd_wrap = ring->wr_wrap = 0;

    /* flush info queue buffer */
    ring = &vobj->info_q.ring;
    ring->rd_ptr = ring->wr_ptr;
    ring->rd_wrap = ring->wr_wrap = 0;
    OS_EXIT_CRITICAL();
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_DropVideo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Drop the frames in compress buffer.

 @param[in] vobj    VStream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_VStream_DropVideo(MMPF_VSTREAM_CLASS *vobj)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_ULONG sz2end;
    MMPF_VSTREAM_FRAMEINFO info;

    err = MMPF_VStream_ReadFrameInfo(vobj, &info);
    if (err)
        return err;

    vobj->ts = info.time;   // update streamer timestamp
    MMPF_VStream_CompBufAddr2ReadFrame(vobj, &sz2end);
    MMPF_VStream_CompBufReadAdvance(vobj, info.size);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_TransferVideo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer the frames in compress buffer into V4L2 slot buffer.

 @param[in] vobj    VStream object

 @retval It reports the status of the operation.
*/
extern int aitcam_ipc_is_debug_ts(void);
static MMP_ERR MMPF_VStream_TransferVideo(MMPF_VSTREAM_CLASS *vobj)
{
    MMP_ERR     err = MMP_ERR_NONE;
    #if (V4L2_H264_DBG == 0)
    MMP_ULONG   id, slotbuf;
    #endif
    MMPF_VSTREAM_FRAMEINFO info;

    if (vobj->state == VSTREAM_STATE_OPEN) {
        MMP_ULONG slot_size = 0 ;

        /* Flush all audio data in buffer? */
        if (vobj->opt & IPC_STREAMER_OPT_FLUSH) {
            MMPF_VStream_FlushData(vobj);
            // Force to encode the next frame as an I-frame
            MMPF_MP4VENC_ForceI(MMPF_H264ENC_GetHandle( VSTREAM_ID(vobj)),
                                                        VSTREAM_RT_CONUS_I);
            vobj->opt &= ~(IPC_STREAMER_OPT_FLUSH);
            // Don't queue much bitstream in compress buffer
            vobj->mode |= IPC_STREAMER_MODE_RT;
        }

        err = MMPF_VStream_RingBufStatus(&vobj->info_q.ring);
        if ((err == MMP_3GPMGR_ERR_QUEUE_EMPTY) ||
            (err == MMP_3GPMGR_ERR_QUEUE_UNDERFLOW))
            return MMP_3GPMGR_ERR_AVBUF_EMPTY;

        /* Make sure the 1st frame transferred is an I-frame */

        if (vobj->tx_size == 0) {
            MMPF_VStream_QueryFrameInfo(vobj, &info);
            if (info.type != MMPF_3GPMGR_FRAME_TYPE_I) {
                RTNA_DBG_Str0("Wait_I\r\n");
                return MMPF_VStream_DropVideo(vobj);
            }
            else {
                if( vobj->mode |= IPC_STREAMER_MODE_RT) {
                    vobj->mode |= IPC_STREAMER_MODE_RAMPUP_BR;
                    vobj->ts_rampup = info.time ;
                }
            }
        }
        #if (V4L2_H264_DBG == 0)
        id = VSTREAM_ID(vobj);
        slot_size = aitcam_ipc_get_slot(vobj->v4l2_id, &slotbuf);
        #endif
        if (slot_size)
        {
            int frames = MMPF_VStream_GetFrameInfoNum(vobj);
            // to know how many video queued due to busy
            if(frames > 1) {
                printc("[q-vf]:%d\r\n",frames );
            }
            MMPF_VStream_ReadFrameInfo(vobj, &info);

            #if (V4L2_H264_DBG == 0)
            MMPF_VStream_AddVideoFrame(vobj, info.size , slotbuf,slot_size);
            #else
            MMPF_VStream_CompBufReadAdvance(vobj, info.size);
            #endif

            #if (V4L2_H264_DBG == 0)
            
            if( aitcam_ipc_is_debug_ts() ) {
                // Inform with frame info
                MMPF_DBG_Int(VSTREAM_ID(vobj), -1);
                //MMPF_DBG_Int(info.size, -7);
                MMPF_DBG_Int(info.time, -6);
                if (vobj->ts) {
                    MMPF_DBG_Int(info.time - vobj->ts, -2);
                }
                else {
                    RTNA_DBG_Str0(" 00");
                }
                if (info.type == MMPF_3GPMGR_FRAME_TYPE_I) {
                    RTNA_DBG_Str0("_I\r\n");
                }
                else {
                    RTNA_DBG_Str0("_P\r\n");
                }
            }
            
            if(info.size < slot_size) {
                aitcam_ipc_send_frame(vobj->v4l2_id, info.size, info.time);
            }
            else {
                aitcam_ipc_send_frame(vobj->v4l2_id, slot_size, info.time);
            }
            #endif

            // update streamer tx size & timestamp
            vobj->tx_size += info.size;
            vobj->ts       = info.time;


             if( vobj->mode &= IPC_STREAMER_MODE_RAMPUP_BR ) {
                MMP_ULONG cur_br ;
                MMPF_H264ENC_ENC_INFO *pEnc = MMPF_H264ENC_GetHandle( VSTREAM_ID(vobj)) ; 
                if(pEnc->target_bitrate) {
                    cur_br = pEnc->rampup_bitrate ;
                    if( cur_br < pEnc->target_bitrate ) {
                        if( (vobj->ts - vobj->ts_rampup) >= 1000 ) {
                            vobj->ts_rampup = vobj->ts ;
                            cur_br += 100*1000 ;
                            if(cur_br > pEnc->target_bitrate ) {
                                cur_br =  pEnc->target_bitrate ;
                                
                            }
                            pEnc->rampup_bitrate = cur_br ;
                            MMPF_MP4VENC_SetBitRate(pEnc,cur_br ) ;
                        }
                    } else {
                        vobj->mode &= ~IPC_STREAMER_MODE_RAMPUP_BR ;
                    }        
                } 
                else {
                    vobj->mode &= ~IPC_STREAMER_MODE_RAMPUP_BR ;
                }        
             }


            return MMPF_VStream_RingBufStatus(&vobj->info_q.ring);
        }
        else {
            //MMPF_OS_Sleep(5);
            //RTNA_DBG_Str(0, "V++\r\n");
        }

        return MMP_3GPMGR_ERR_AVBUF_EMPTY;
    }
    else {
        MMP_ULONG skip_th = MMPF_VStream_SkipThreshold(vobj);

        if (MMPF_VStream_ShouldFrameSkip(VSTREAM_ID(vobj), skip_th)) {
            do {
                // drop frames until next I-frame reached
                err = MMPF_VStream_DropVideo(vobj);
                if (err)
                    return err;

                err = MMPF_VStream_QueryFrameInfo(vobj, &info);
                if (err)
                    return err;
                if (info.type == MMPF_3GPMGR_FRAME_TYPE_I)
                    break;
            } while(1);
            MMPF_Streamer_SetBaseTime(info.time);
            //RTNA_DBG_Str0("#v_drop\r\n");
        }

        return MMP_3GPMGR_ERR_AVBUF_EMPTY;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_TransferVideo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer the video data

 @param[in] id      VStream object ID

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_VStream_TransferData(MMP_ULONG id)
{
    MMP_ULONG   times = 5;
    MMP_ERR     status;

    //MMPF_VStream_CompBufUsage(VSTREAM_OBJ(id));

    while(times--) {
        // move video data from bitstream buffer to V4L2 slot buffers
        status = MMPF_VStream_TransferVideo(VSTREAM_OBJ(id));
        if (status != MMP_ERR_NONE)
            break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start streaming of the specified VStream.

 @param[in] id      Encoder ID, mapping to VStream object ID
 @param[in] v4l2_id Corresponded V4L2 stream ID
 @param[in] opt     Option of streaming

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_VStream_Start( MMP_UBYTE           encid,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMPF_VSTREAM_CLASS *vobj = VSTREAM_OBJ(encid);

    if ((encid < MAX_VIDEO_STREAM_NUM) && (vobj->state == VSTREAM_STATE_NONE)) {
        /* Reset transfer info. */
        vobj->tx_size   = 0;
        vobj->mode      = IPC_STREAMER_MODE_NONE;
        vobj->opt       = opt;
        vobj->v4l2_id   = v4l2_id;
        vobj->state     = VSTREAM_STATE_OPEN;
        return MMP_ERR_NONE;
    }

    return MMP_VSTREAM_ERR_STATE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop streaming of the specified VStream.

 @param[in] id      Encoder ID, mapping to VStream object ID

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_VStream_Stop(MMP_UBYTE encid)
{
    MMPF_VSTREAM_CLASS *vobj = VSTREAM_OBJ(encid);

    if ((encid < MAX_VIDEO_STREAM_NUM) && (vobj->state == VSTREAM_STATE_OPEN)) {
        vobj->state = VSTREAM_STATE_NONE;
        return MMP_ERR_NONE;
    }

    return MMP_VSTREAM_ERR_STATE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VStream_ProcessCmd
//  Description :
//------------------------------------------------------------------------------
/**
 @brief VStream commands handler

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_VStream_ProcessCmd(void)
{
    MMP_ERR     ret = MMP_ERR_NONE;
	MMP_ULONG   ulParameter[MAX_HIF_ARRAY_SIZE], i;
    MMP_USHORT  usCommand;

    usCommand = m_ulHifCmd[GRP_IDX_VID];

    for(i = 0; i < MAX_HIF_ARRAY_SIZE; i++) {
        ulParameter[i] = m_ulHifParam[GRP_IDX_VID][i];
    }
    m_ulHifCmd[GRP_IDX_VID] = 0;

    switch (usCommand & (GRP_MASK | FUNC_MASK)) {
    case HIF_VID_CMD_MERGER_PARAMETER:
        switch (usCommand & SUB_MASK) {
        case AV_FILE_LIMIT:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 4, 0);
            break;
        case AUDIO_ENCODE_CTL:
            break;
        case GET_COMPBUF_ADDR:
        	MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, 0);
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 4, 0);
        	break;
        case GET_3GP_FILE_SIZE:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, 0);
            break;
        case GET_3GP_DATA_RECODE:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, 0);
            break;
        case ENCODE_STORAGE_PATH:
            break;
        case ENCODE_FILE_NAME:
            break;
        case SET_USER_DATA_ATOM:
            break;
        #if (VAAC_R_EN)||(VADPCM_R_EN)||(VMP3_R_EN)||(VPCM_R_EN)||(VAMR_R_EN)
		#if (VAAC_R_EN)
        case AUDIO_AAC_MODE:
        #endif
        #if (VADPCM_R_EN)
        case AUDIO_ADPCM_MODE:
        #endif
        #if (VMP3_R_EN)
        case AUDIO_MP3_MODE:
        #endif
        #if (VPCM_R_EN)
        case AUDIO_PCM_MODE:
        #endif
        #if (VAMR_R_EN)
        case AUDIO_AMR_MODE:
        #endif
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 4, 0);
            break;
        #endif
        case GET_RECORD_DURATION:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, VSTREAM_OBJ(ulParameter[0])->ts);
            break;
        case GET_RECORDING_TIME:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, VSTREAM_OBJ(ulParameter[0])->ts);
        	break;
		case GET_RECORDING_OFFSET:
			MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, 0);
			break;
        case GET_RECORDING_FRAME:
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 0, 0);
        	break;
        case AV_TIME_LIMIT:
        	break;
        case AV_TIME_DYNALIMIT:
        	break;
		case VIDEO_BUFFER_THRESHOLD:
			break;
        case GOP_FRAME_TYPE:
            break;
        case SKIP_THRESHOLD:
            break;
        case SEAMLESS_MODE:
            break;
        case SET_3GPMUX_TIMEATOM:
        	break;
        case MODIFY_AVI_LIST_ATOM:
        	break;	               
        default:
            RTNA_DBG_Str(0, "Not support 3GP merger command = ");
            RTNA_DBG_Short(0, usCommand);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_3GPMGR_ERR_PARAMETER;
        }
        break;
    case HIF_VID_CMD_MERGER_TAILSPEEDMODE:
	    break;
    case HIF_VID_CMD_MERGER_OPERATION:
        switch (usCommand & SUB_MASK) {
        case MERGER_START:
            ret = MMPF_VStream_StartEnc(ulParameter[0]);
            MMPF_HIF_FeedbackParamL(GRP_IDX_VID, 4, ret);
            break;
        case MERGER_STOP:
            MMPF_VStream_StopEnc(ulParameter[0]);
            break;
        case MERGER_PAUSE:
            MMPF_VIDENC_Pause(ulParameter[0]);
            break;
        case MERGER_RESUME:
            MMPF_VIDENC_Resume(ulParameter[0]);
            break;
        case MERGER_SKIPMODE_ENABLE:
		case MERGER_MULTISTREAM_USEMODE:
        case MERGER_ENCODE_FORMAT:
        case MERGER_ENCODE_RESOLUTION:
      	case MERGER_ENCODE_FRAMERATE:
      	case MERGER_ENCODE_GOP:
      	case MERGER_ENCODE_SPSPPSHDR:
        #if (SUPPORT_VR_THUMBNAIL)
        case MERGER_SET_THUMB_INFO:
        case MERGER_SET_THUMB_BUF:
        #endif
        default:
        	break;	
        }     
        break;
    default:
        RTNA_DBG_Str(0, "Not support VStream command = ");
        RTNA_DBG_Short(0, usCommand);
        RTNA_DBG_Str(0, "\r\n");
        return MMP_3GPMGR_ERR_PARAMETER;
    }

    MMPF_OS_SetFlags(DSC_UI_Flag, SYS_FLAG_VID_CMD_DONE, MMPF_OS_FLAG_SET);

    return MMP_ERR_NONE;
}

/**
 @brief Main routine of VStream task.
*/
void MMPF_VStream_Task(void *p_arg)
{
	MMPF_OS_FLAGS wait_flags, flags;

    RTNA_DBG_Str3("VStream_Task()\r\n");

	MMPF_VStream_TaskInit();

    wait_flags = CMD_FLAG_VSTREAM;

    while (1) {
        MMPF_OS_WaitFlags(  VID_REC_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        if (flags & CMD_FLAG_VSTREAM) {
            MMPF_VStream_ProcessCmd();
        }
    }
}
#endif

/// @}
