/**
 @file mmpf_jstream.c
 @brief Control functions of JPEG Streamer
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmph_hif.h"

#include "mmpf_system.h"
#include "mmpf_sensor.h"
#include "mmpf_dma.h"
#include "mmpf_dsc.h"
#include "mmpf_jstream.h"


/** @addtogroup MMPF_JSTREAM
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
static MMPF_JSTREAM_CLASS   m_JStreamObj[MAX_JPG_STREAM_NUM];

static MMPF_OS_SEMID        m_JDmaSemID;

/*
 * External Variables
 */
extern MMPF_OS_FLAGID       DSC_Flag;

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define JSTREAM_OBJ(id)     (&m_JStreamObj[id])
#define JSTREAM_ID(obj)     (obj - &m_JStreamObj[0])

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static void MMPF_JStream_Reset(MMPF_JSTREAM_CLASS *jobj);
static MMP_ERR MMPF_JStream_PopFrameInfo(   MMPF_JSTREAM_CLASS  *jobj,
                                            MMP_ULONG           *addr,
                                            MMP_ULONG           *size,
                                            MMP_ULONG           *time);
#if (V4L2_JPG)
extern MMP_BOOL aitcam_ipc_send_frame(  MMP_ULONG   v4l2_id,
                                        MMP_ULONG   size,
                                        MMP_ULONG   ts);
extern MMP_ULONG aitcam_ipc_get_slot(    MMP_ULONG   v4l2_id,
                                        MMP_ULONG   *slot);
#endif

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____JSTREAM_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_New
//  Description :
//------------------------------------------------------------------------------
/**
 @brief New a JPEG streamer

 @retval Id of the new jstream.
*/
MMP_ULONG MMPF_JStream_New(void)
{
    MMP_ULONG i;
    MMPF_JSTREAM_CLASS *jobj = NULL;

    for(i = 0; i < MAX_JPG_STREAM_NUM; i++) {
        if (m_JStreamObj[i].state == JSTREAM_STATE_NONE) {
            jobj = &m_JStreamObj[i];
            jobj->state = JSTREAM_STATE_OPEN;
            break;
        }
    }

    if (!jobj) {
        RTNA_DBG_Str(0, "JStream no obj\r\n");
        return MAX_JPG_STREAM_NUM;
    }

    return JSTREAM_ID(jobj);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_Delete
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release the JPEG streamer

 @param[in] id      Id of jstream object

 @retval None.
*/
void MMPF_JStream_Delete(MMP_ULONG id)
{
    MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(id);

    if (jobj->state != JSTREAM_STATE_OPEN) {
        RTNA_DBG_Str0("Stop jstream first!\r\n");
    }

    MMPF_JStream_Reset(JSTREAM_OBJ(id));
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start streaming the specified JStream object

 @param[in] id      JStream object ID
 @param[in] v4l2_id Corresponded V4L2 stream id
 @param[in] opt     Option of streaming

 @retval None.
*/
MMP_ERR MMPF_JStream_Start( MMP_ULONG           id,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(id);

    if (jobj->state != JSTREAM_STATE_OPEN)
        return MMP_JSTREAM_ERR_STATE;

    jobj->opt       = opt;
    jobj->v4l2_id   = v4l2_id;
    jobj->state     = JSTREAM_STATE_START;
    MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_JSTREAM(id), MMPF_OS_FLAG_SET);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop streaming the specified JStream object

 @param[in] id      JStream object ID

 @retval None.
*/
MMP_ERR MMPF_JStream_Stop(MMP_ULONG id)
{
    MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(id);

    if (jobj->state != JSTREAM_STATE_START)
        return MMP_JSTREAM_ERR_STATE;

    jobj->state = JSTREAM_STATE_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_TriggerEncode
//  Description : 
//------------------------------------------------------------------------------
/**
 @brief Tigger JPEG encode

 @param[in] id      JStream object ID

 @retval None.
*/
void MMPF_JStream_TriggerEncode(MMP_ULONG id)
{
    MMPF_OS_SetFlags(DSC_Flag, DSC_FLAG_TRIGGER_STILL, MMPF_OS_FLAG_SET);
}

MMP_ERR MMPF_JStream_GetFrame(MMP_ULONG id,MMP_ULONG *jpg_addr , MMP_ULONG *jpg_size)
{
    MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(id);
    MMP_ULONG available, addr, size, time;

    *jpg_addr = 0 ;
    *jpg_size = 0 ;
    if( ! jobj) {
        return  MMP_JSTREAM_ERR_NODATA ;   
    }    
    AUTL_RingBuf_DataAvailable(&jobj->info_q.ring, &available);
    if (available == 0) {
        return MMP_JSTREAM_ERR_NODATA;
    }
    MMPF_JStream_PopFrameInfo(jobj, &addr, &size, &time);
    *jpg_addr = addr ;
    *jpg_size = size ;

    return MMP_ERR_NONE ;
}


#if 0
void _____JSTREAM_INTERNAL_FUNC_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_PushFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Save the frame info. into the queue.

 @param[in] id      JStream object ID
 @param[in] addr    Encoded frame data address
 @param[in] size    Encoded frame size
 @param[in] time    Encoded frame timestamp

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_JStream_PushFrameInfo(  MMP_ULONG               id,
                                            MMPF_JSTREAM_FRAMEINFO  *info)
{
    MMP_ULONG free;
    MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(id);
    MMPF_JSTREAM_FRAMEINFO  *pInfo = NULL;

    AUTL_RingBuf_SpaceAvailable(&jobj->info_q.ring, &free);
    if (free >= 1) {
        pInfo = &jobj->info_q.data[jobj->info_q.ring.ptr.wr];
        pInfo->addr = info->addr;
        pInfo->size = info->size;
        pInfo->time = info->time;
        AUTL_RingBuf_CommitWrite(&jobj->info_q.ring, 1);
    }
    else {
#if SUPPORT_UVC_JPEG==0
        RTNA_DBG_Str0("No space for frame info\r\n");
#endif
        return MMP_JSTREAM_ERR_OVERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_PopFrameInfo
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Read out the frame info. into the queue.

 @param[in] jobj    JStream object
 @param[out] addr   Encoded frame data address
 @param[out] size   Encoded frame size
 @param[out] time   Encoded frame timestamp

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_JStream_PopFrameInfo(   MMPF_JSTREAM_CLASS  *jobj,
                                            MMP_ULONG           *addr,
                                            MMP_ULONG           *size,
                                            MMP_ULONG           *time)
{
    MMP_ULONG available;
    MMPF_JSTREAM_FRAMEINFO  *pInfo = NULL;

    AUTL_RingBuf_DataAvailable(&jobj->info_q.ring, &available);
    if (available >= 1) {
        pInfo = &jobj->info_q.data[jobj->info_q.ring.ptr.rd];
        *addr = pInfo->addr;
        *size = pInfo->size;
        *time = pInfo->time;
        AUTL_RingBuf_CommitRead(&jobj->info_q.ring, 1);
    }
    else {
        RTNA_DBG_Str0("No available frame info\r\n");
        return MMP_JSTREAM_ERR_UNDERFLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_ReleaseDma
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Callback function to release DMA semaphore.

 @param[in] argu    Callback argument, no used

 @retval None.
*/
static void MMPF_JStream_ReleaseDma(void *argu)
{
	if (MMPF_OS_ReleaseSem(m_JDmaSemID) != OS_NO_ERR) {
		RTNA_DBG_Str0("m_JDmaSemID OSSemPost: Fail\r\n");
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_DmaMoveData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Copy JPEG data to another buffer by move DMA.

 @param[in] src     Source address of data
 @param[in] dst     Destination address
 @param[in] sz      Data size

 @retval None.
*/
static void MMPF_JStream_DmaMoveData(MMP_ULONG src, MMP_ULONG dst, MMP_ULONG sz)
{
    if (sz == 0)
        return;

	if (MMPF_DMA_MoveData(src, dst, sz, MMPF_JStream_ReleaseDma,
	                      0, MMP_FALSE, NULL))
    {
        RTNA_DBG_Str0("MoveData fail\r\n");
        return;
	}
	if (MMPF_OS_AcquireSem(m_JDmaSemID, 10000)) {
        RTNA_DBG_Str0("m_JDmaSemPend fail\r\n");
        return;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_TransferFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer frames via V4L2 interface

 @param[in] jobj    JStream object

 @retval None.
*/
static MMP_ERR MMPF_JStream_TransferFrame(MMPF_JSTREAM_CLASS *jobj)
{
    MMP_ULONG available = 0, addr, size = 0, time = 0;
    MMP_ULONG freesize ;
    MMP_ULONG id, slotbuf;

    if (jobj->state != JSTREAM_STATE_START)
        return MMP_JSTREAM_ERR_NODATA;

    if (jobj->opt & IPC_STREAMER_OPT_FLUSH) {
        AUTL_RingBuf_Flush(&jobj->info_q.ring);
        jobj->opt &= ~(IPC_STREAMER_OPT_FLUSH);
    }

    AUTL_RingBuf_DataAvailable(&jobj->info_q.ring, &available);
    if (available == 0)
        return MMP_JSTREAM_ERR_NODATA;

    id = JSTREAM_ID(jobj);
    freesize = aitcam_ipc_get_slot(jobj->v4l2_id, &slotbuf);
    if (freesize) {
        MMPF_JStream_PopFrameInfo(jobj, &addr, &size, &time);
        MMPF_JStream_DmaMoveData(addr, slotbuf, size);
    #if SUPPORT_UVC_JPEG==0
        // Inform with frame info
        RTNA_DBG_Long0(size);
        RTNA_DBG_Long0(time);
        RTNA_DBG_Str0("_J\r\n");
    #endif        
        aitcam_ipc_send_frame(jobj->v4l2_id, size, time);
    }
    else {
        //RTNA_DBG_Str(0, "J++\r\n");
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_TransferData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer the JPEG data

 @param[in] id      JStream object ID

 @retval None.
*/
void MMPF_JStream_TransferData(MMP_ULONG id)
{
    MMP_ULONG   times = 5;
    MMP_ERR     status;

    while(times--) {
        // move JPEG data from bitstream buffer to V4L2 slot buffers
        status = MMPF_JStream_TransferFrame(JSTREAM_OBJ(id));
        if (status != MMP_ERR_NONE)
            break;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_Reset
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset the JStream object.

 @param[in] jobj    JStream object

 @retval None.
*/
static void MMPF_JStream_Reset(MMPF_JSTREAM_CLASS *jobj)
{
    /* Reset JStream state & properties */
    jobj->state = JSTREAM_STATE_NONE;
    AUTL_RingBuf_Init(&jobj->info_q.ring, (MMP_ULONG)jobj->info_q.data,
                                          JSTREAM_INFO_Q_DEPTH);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_JStream_TaksInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize routine in AStream task startup.

 @retval None.
*/
static void MMPF_JStream_TaksInit(void)
{
    MMP_ULONG i;

    m_JDmaSemID = MMPF_OS_CreateSem(0);

    /* Reset all JStream objects */
    for(i = 0; i < MAX_JPG_STREAM_NUM; i++)
        MMPF_JStream_Reset(JSTREAM_OBJ(i));
}

#if 0
void _____JSTREAM_TASK_ROUTINES_____(){}
#endif

/**
 @brief Main routine of JStream task.
*/
void MMPF_JStream_Task(void)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;
    MMPF_JSTREAM_FRAMEINFO frminfo;
    int fram_cnt = 0;
    RTNA_DBG_Str3("JStream_Task()\r\n");

    MMPF_JStream_TaksInit();

    wait_flags = DSC_FLAG_TRIGGER_STILL;

    while(1) {
        MMPF_OS_WaitFlags(  DSC_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        if (flags & DSC_FLAG_TRIGGER_STILL) {
            if (fram_cnt == 0) {
                ISP_IF_AE_SetGain();
            }
            fram_cnt = 1;
            printc("fram_cnt %d\r\n", fram_cnt);

            MMPF_Sensor_Wait3AConverge(PRM_SENSOR);
            MMPF_DSC_EncodeJpeg(PRM_SENSOR, MMPF_DSC_SRC_SENSOR,
                                MMPF_DSC_SHOTMODE_SINGLE);
            MMPF_DSC_GetJpeg(&frminfo.addr, &frminfo.size, &frminfo.time);
            // TODO: here always use ID 0 for JPEG stream
            MMPF_JStream_PushFrameInfo(0, &frminfo);
            MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_JSTREAM(0), MMPF_OS_FLAG_SET);
            MMPF_SYS_DumpTimerMark();
            #if SUPPORT_UVC_JPEG==1
            {
              MMPF_JSTREAM_CLASS *jobj = JSTREAM_OBJ(0);
              if(jobj->state==JSTREAM_STATE_START) {
                MMPF_JStream_TriggerEncode(0) ;
              }
            }
            #endif
        }
    }
}

/// @}
