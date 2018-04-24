/**
 @file mmpf_ystream.c
 @brief Control functions of Gray Streamer
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#if (V4L2_GRAY)
#include "mmph_hif.h"

#include "mmpf_system.h"
#include "mmpf_dma.h"
#include "mmpf_ystream.h"

#include "ipc_cfg.h"
#include "dualcpu_v4l2.h"


/** @addtogroup MMPF_YSTREAM
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
static MMPF_OS_SEMID        m_YMoveDMASemID; ///< for DMA operation control
static MMPF_YSTREAM_CLASS   m_YStreamObj[MAX_VIDEO_STREAM_NUM];

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define YSTREAM_OBJ(id)     (&m_YStreamObj[id])
#define YSTREAM_ID(obj)     (obj - &m_YStreamObj[0])

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern MMP_BOOL aitcam_ipc_send_frame(  MMP_ULONG   v4l2_id,
                                        MMP_ULONG   size,
                                        MMP_ULONG   ts);
extern MMP_ULONG aitcam_ipc_get_slot(    MMP_ULONG   v4l2_id,
                                        MMP_ULONG   *slot);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____BASIC_BUFFER_OPERATION_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_ReleaseDma
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Callback function to release DMA semaphore.

 @param[in] argu    Callback argument, no used

 @retval None.
*/
static void MMPF_YStream_ReleaseDma(void *argu)
{
	if (MMPF_OS_ReleaseSem(m_YMoveDMASemID) != OS_NO_ERR) {
		RTNA_DBG_Str(3, "m_YMoveDMASemID OSSemPost: Fail\r\n");
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_MoveFrame
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Copy gray frame to another buffer by move DMA.

 @param[in] src     Source address of data
 @param[in] dst     Destination address
 @param[in] sz      Data size

 @retval None.
*/
static void MMPF_YStream_MoveFrame(MMP_ULONG src, MMP_ULONG dst, MMP_ULONG sz)
{
    if (sz == 0)
        return;

	if (MMPF_DMA_MoveData(src, dst, sz, MMPF_YStream_ReleaseDma,
	                      0, MMP_FALSE, NULL))
    {
        RTNA_DBG_Str(0, "DMA_MoveData fail\r\n");
        return;
	}
    // waitting DMA done
	if (MMPF_OS_AcquireSem(m_YMoveDMASemID, 10000)) {
        RTNA_DBG_Str(0, "m_YMoveDMASemPend fail\r\n");
        return;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_FrameRdy
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Inform that there is one gray frame ready

 @param[in] id      YStream object ID
 @param[in] frm     Gray frame address
 @param[in] ts      Gray frame timestamp

 @retval None.
*/
void MMPF_YStream_FrameRdy(MMP_ULONG id, MMP_ULONG frm, MMP_ULONG ts)
{
    MMPF_YSTREAM_CLASS *yobj = YSTREAM_OBJ(id);

    yobj->frm   = frm;
    yobj->ts    = ts;

    MMPF_OS_SetFlags(STREAMER_Flag, SYS_FLAG_YSTREAM(id), MMPF_OS_FLAG_SET);
}

#if 0
void _____BASIC_YSTREAM_OPERATION_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_Reset
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset the YStream object.

 @param[in] yobj    YStream object

 @retval None.
*/
static void MMPF_YStream_Reset(MMPF_YSTREAM_CLASS *yobj)
{
    yobj->state     = YSTREAM_STATE_NONE;
    yobj->frm       = 0;
    yobj->frm_size  = 0;
    yobj->ts        = 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_TaskInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize routine in YStream.

 @retval None.
*/
static void MMPF_YStream_Init(void)
{
    MMP_ULONG i;

	m_YMoveDMASemID = MMPF_OS_CreateSem(0);

    for (i = 0; i < MAX_GRAY_STREAM_NUM; i++) {
        MMPF_YStream_Reset(YSTREAM_OBJ(i));
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_New
//  Description :
//------------------------------------------------------------------------------
/**
 @brief @brief New a Gray streamer

 @param[in] w       Width of gray stream
 @param[in] h       Height of gray stream

 @retval Id of the new YStream.
*/
MMP_ULONG MMPF_YStream_New(MMP_ULONG w, MMP_ULONG h,MMP_BOOL y_only)
{
    MMP_ULONG i;
    static MMP_BOOL _init = MMP_FALSE;
    MMPF_YSTREAM_CLASS *yobj;

    if (!_init) {
        MMPF_YStream_Init();
        _init = MMP_TRUE;
    }

    for (i = 0; i < MAX_GRAY_STREAM_NUM; i++) {
        yobj = YSTREAM_OBJ(i);
        if (yobj->state == YSTREAM_STATE_NONE) {
            yobj->state = YSTREAM_STATE_IDLE;
            if(y_only) {
                yobj->frm_size = w * h;
            }
            else {
                yobj->frm_size = (w * h * 3 ) >> 1;
            }
            break;
        }
    }

    if (i == MAX_GRAY_STREAM_NUM) {
        RTNA_DBG_Str(0, "YStream exhausted\r\n");
        return MAX_GRAY_STREAM_NUM;
    }

    return i;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_Delete
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release the gray streamer

 @param[in] id      YStream object ID

 @retval None.
*/
void MMPF_YStream_Delete(MMP_ULONG id)
{
    MMPF_YSTREAM_CLASS *yobj = YSTREAM_OBJ(id);

    if (yobj->state != YSTREAM_STATE_IDLE) {
        RTNA_DBG_Str0("Stop ystream first!\r\n");
    }

    MMPF_YStream_Reset(yobj);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_TransferData
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Transfer the gray frame

 @param[in] id      YStream object ID

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_YStream_TransferData(MMP_ULONG id)
{
    MMPF_YSTREAM_CLASS *yobj = YSTREAM_OBJ(id);

    if (yobj->state == YSTREAM_STATE_OPEN) {
        MMP_ULONG slot;
        if(yobj->v4l2_id < AITCAM_NUM_CONTEXTS ) {
            if (aitcam_ipc_get_slot(yobj->v4l2_id, &slot)) {
                MMP_ULONG frm, ts, size;

                // move gray fram to V4L2 slot buffers
                frm     = yobj->frm;
                ts      = yobj->ts;
                size    = yobj->frm_size;
                MMPF_YStream_MoveFrame(frm, slot, size);
                aitcam_ipc_send_frame(yobj->v4l2_id, size, ts);
            }
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start streaming of the specified YStream.

 @param[in] id      YStream object ID
 @param[in] v4l2_id Corresponded V4L2 stream ID

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_YStream_Start(MMP_ULONG id, MMP_ULONG v4l2_id)
{
    MMPF_YSTREAM_CLASS *yobj = YSTREAM_OBJ(id);

    if ((id < MAX_GRAY_STREAM_NUM) && (yobj->state == YSTREAM_STATE_IDLE)) {
        /* Reset transfer info. */
        yobj->v4l2_id   = v4l2_id;
        yobj->state     = YSTREAM_STATE_OPEN;
        return MMP_ERR_NONE;
    }

    return MMP_YSTREAM_ERR_STATE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_YStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop streaming of the specified YStream.

 @param[in] id      YStream object ID

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_YStream_Stop(MMP_ULONG id)
{
    MMPF_YSTREAM_CLASS *yobj = YSTREAM_OBJ(id);

    if ((id < MAX_GRAY_STREAM_NUM) && (yobj->state == YSTREAM_STATE_OPEN)) {
        yobj->state = YSTREAM_STATE_IDLE;
        return MMP_ERR_NONE;
    }

    return MMP_YSTREAM_ERR_STATE;
}
#endif //(V4L2_GRAY)

/// @}
