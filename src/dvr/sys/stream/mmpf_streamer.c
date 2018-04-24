/**
 @file mmpf_streamer.c
 @brief Control functions of Streamer
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmph_hif.h"

#include "mmpf_vstream.h"
#include "mmpf_astream.h"
#include "mmpf_jstream.h"
#include "mmpf_ystream.h"

/** @addtogroup MMPF_STREAMER
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
static MMP_ULONG    m_StreamerBaseTime = 0;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_Streamer_SetBaseTime
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set the streamer base time which is the timestamp of frame existed in
        buffer with the longest duration.

 @param[in] ts      Timestamp

 @retval It reports the status of the operation.
*/
void MMPF_Streamer_SetBaseTime(MMP_ULONG ts)
{
    m_StreamerBaseTime = ts;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Streamer_GetBaseTime
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the streamer base time which is the timestamp of frame existed in
        buffer with the longest duration.

 @retval The streamer base time.
*/
MMP_ULONG MMPF_Streamer_GetBaseTime(void)
{
    return m_StreamerBaseTime;
}

#if 0
void _____STREAMER_TASK_ROUTINES_____(){}
#endif

/**
 @brief Main routine of Streamer task.
*/
void MMPF_Streamer_Task(void)
{
    MMP_ULONG i;
    MMPF_OS_FLAGS wait_flags = 0, flags;

    RTNA_DBG_Str3("Streamer_Task()\r\n");

    #if (V4L2_AAC)
    for (i = 0; i < MAX_AUD_STREAM_NUM; i++) {
        wait_flags |= SYS_FLAG_ASTREAM(i);
    }
    #endif
    #if (V4L2_H264)
    for (i = 0; i < MAX_VIDEO_STREAM_NUM; i++) {
        wait_flags |= SYS_FLAG_VSTREAM(i);
    }
    #endif
    #if (V4L2_JPG)
    for (i = 0; i < MAX_JPG_STREAM_NUM; i++) {
        wait_flags |= SYS_FLAG_JSTREAM(i);
    }
    #endif
    #if (V4L2_GRAY)
    for (i = 0; i < MAX_GRAY_STREAM_NUM; i++) {
        wait_flags |= SYS_FLAG_YSTREAM(i);
    }
    #endif

    while(1) {
        MMPF_OS_WaitFlags(  STREAMER_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        #if (V4L2_H264)
        for (i = 0; i < MAX_VIDEO_STREAM_NUM; i++) {
            if (flags & SYS_FLAG_VSTREAM(i))
                MMPF_VStream_TransferData(i);
        }
        #endif
        #if (V4L2_AAC)
        for (i = 0; i < MAX_AUD_STREAM_NUM; i++) {
            if (flags & SYS_FLAG_ASTREAM(i)) {
                MMPF_AStream_TransferData(i);
            }
        }
        #endif
        #if (V4L2_JPG)
        for (i = 0; i < MAX_JPG_STREAM_NUM; i++) {
            if (flags & SYS_FLAG_JSTREAM(i)) {
                MMPF_JStream_TransferData(i);
            }
        }
        #endif
        #if (V4L2_GRAY)
        for (i = 0; i < MAX_GRAY_STREAM_NUM; i++) {
            if (flags & SYS_FLAG_YSTREAM(i)) {
                MMPF_YStream_TransferData(i);
            }
        }
        #endif
    }
}

/// @}
