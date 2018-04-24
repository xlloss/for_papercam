/**
 @file mmpf_iva.c
 @brief Control functions of IVA ( human detection )
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmpf_iva.h"
#include "mmu.h"

#include "ipc_cfg.h"
#include "dualcpu_v4l2.h"

#if (SUPPORT_IVA)

/** @addtogroup MMPF_IVA
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
static MMPF_IVA_HANDLE m_IVA;

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define MMPF_IVA_QueueDepth(q)     ((q)->size)

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
/* log functions for IVA library, please porting for it */
void IVA_printk(char *fmt, ...)
{
    //printc(fmt);
    //RTNA_DBG_Str0("\r");
}

#if 0
void _____IVA_INTERNAL_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Enqueue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Insert one element into the end of queue.

 @param[in] q       Queue
 @param[in] data    Data to be inserted
 
 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_IVA_Enqueue(MMPF_IVA_QUEUE *queue, MMP_UBYTE data)
{
    MMP_ULONG idx;
    OS_CRITICAL_INIT();

    if (!queue)
        return MMP_IVA_ERR_POINTER;

    if (queue->size == IVA_LUMA_SLOTS) {
        RTNA_DBG_Str0("iva queue full\r\n");
        return MMP_IVA_ERR_FULL;
    }

    OS_ENTER_CRITICAL();
    idx = queue->head + queue->size++;
    if (idx >= IVA_LUMA_SLOTS)
        idx -= IVA_LUMA_SLOTS;
    queue->data[idx] = data;
    OS_EXIT_CRITICAL();

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Dequeue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Retrieve one element from the head of queue.

 @param[in]  q      Queue
 @param[out] data   Data retrieved from queue

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_IVA_Dequeue(MMPF_IVA_QUEUE *queue, MMP_UBYTE *data)
{
    OS_CRITICAL_INIT();

    if (!queue)
        return MMP_IVA_ERR_POINTER;

    if (queue->size == 0) {
        RTNA_DBG_Str0("iva queue empty\r\n");
        return MMP_IVA_ERR_EMPTY;
    }

    OS_ENTER_CRITICAL();
    *data = queue->data[queue->head];
    queue->size--;
    queue->head++;
    if (queue->head == IVA_LUMA_SLOTS)
        queue->head = 0;
    OS_EXIT_CRITICAL();

    return MMP_ERR_NONE;
}

#if 0
void _____IVA_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_BufSize
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the size of working buffer needed.

 @param[in] w       Input luma width
 @param[in] h       Input luma height
 
 @retval The size of working buffer.
*/
MMP_ULONG MMPF_IVA_BufSize(MMP_ULONG   w,MMP_ULONG   h)
{
    int bufsize = 0 ;//IvaMD_GetBufferInfo(w, h,  (1) , div_x, div_y);
    // tbd
    return bufsize;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize iva engine.

 @param[in] iva      IVA object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_IVA_Init(void)
{
    int ret;
    /*
    3rd party IVA lib initialize
    */
    printc("MMPF_IVA_Init called\r\n");
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Enable motion detection engine.

 @param[in] iva      IVA object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_IVA_Open(MMPS_YSTREAM_CLASS *iva)
{
    MMP_ULONG i;
    printc("MMPF_IVA_Open called\r\n");

    MEMSET(&m_IVA.free_q, 0, sizeof(MMPF_IVA_QUEUE));
    MEMSET(&m_IVA.rdy_q,  0, sizeof(MMPF_IVA_QUEUE));

    /* Put luma slot buffer into free queue. The 1st slot is already occupied */
    for(i = 1; i < IVA_LUMA_SLOTS; i++) {
        MMPF_IVA_Enqueue(&m_IVA.free_q, i);
    }

    m_IVA.busy   = MMP_FALSE;
    m_IVA.obj    = iva;


    /*
    OPEN 3rd party IVA 
    */

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Disable IVA engine.

 @param[in] iva      IVA object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_IVA_Close(void)
{
    m_IVA.obj = NULL;

    /* Wakeup IVA task */
    MMPF_OS_SetFlags(IVA_Flag, IVA_FLAG_RUN, MMPF_OS_FLAG_SET);

    while(m_IVA.busy)
        MMPF_OS_Sleep(5);


    /*De-init IVA library*/

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_FrameReady
//  Description :
//------------------------------------------------------------------------------
/**
 @brief One input luma frame for iva is ready.

 @param[in] buf_idx Index of buffer which stored a ready luma frame

 @retval It reports the status of the operation.
*/
void MMPF_IVA_FrameReady(MMP_UBYTE *buf_idx)
{
    if (!m_IVA.obj)  // not yet open
        return;

    MMPF_IVA_Enqueue(&m_IVA.rdy_q, *buf_idx);

    if (MMPF_IVA_QueueDepth(&m_IVA.free_q) > 0) {
        MMPF_IVA_Dequeue(&m_IVA.free_q, buf_idx);
    }
    else {
        MMPF_IVA_Dequeue(&m_IVA.rdy_q, buf_idx);
    }

    /* Trigger iva detection */
    MMPF_OS_SetFlags(IVA_Flag, IVA_FLAG_RUN, MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IVA_Run
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Do motion detection.

 @retval It reports the status of the operation.
*/
void MMPF_IVA_Run(void)
{
static int last = 0 ;
    int ret ;
    MMP_UBYTE buf_idx;
    MMP_ULONG  now , y_size ,uv_size = 0;
    MMP_UBYTE *luma,*u,*v;
    MMPS_YSTREAM_CLASS *iva = NULL;

    m_IVA.busy = MMP_TRUE;

    iva = m_IVA.obj;
    if (iva) {
        if (MMPF_IVA_QueueDepth(&m_IVA.rdy_q) == 0)
            goto _exit_iva_run;

        /* Updates time gap to library for fine tune internal parameters */
        now = OSTime;

        /* Do IVA */
        MMPF_IVA_Dequeue(&m_IVA.rdy_q, &buf_idx);
        // I420 format
        luma = (MMP_UBYTE *)(DRAM_CACHE_VA(iva->frmbuf[buf_idx].base));
        y_size  = iva->propt.w * iva->propt.h ;
        if ( iva->propt.fmt == V4L2_PIX_FMT_I420) {
            uv_size = y_size >> 2 ;
            u = luma + y_size ;
            v = u + uv_size ;
        }
        else if (iva->propt.fmt == V4L2_PIX_FMT_NV12) {
            uv_size = y_size >> 1 ;
            u = v = luma + uv_size ;
        }
        
        // IVA run a frame
        //printc("Please IVA run!(Y : 0x%08x, U : 0x%08x , V : 0x%08x)\r\n", luma ,u,v);
        
        /*
        3rd party code put here
        */
        
        
        
        /* Release luma buffer */
        MMPF_IVA_Enqueue(&m_IVA.free_q, buf_idx);
    }

_exit_iva_run:

    m_IVA.busy = MMP_FALSE;
}

/**
 @brief Main routine of IVA task.
*/
void MMPF_IVA_Task(void)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;

    RTNA_DBG_Str3("IVA_Task()\r\n");

    wait_flags = IVA_FLAG_RUN;

    while(1) {
        MMPF_OS_WaitFlags(  IVA_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        if (flags & IVA_FLAG_RUN)
            MMPF_IVA_Run();
    }
}

/// @}

#endif // (SUPPORT_IVA)
