//==============================================================================
//
//  File        : mmpf_ldc_ctl.c
//  Description : LDC Control function
//  Author      : Eroy Yang
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "lib_retina.h"
#include "mmp_reg_ldc.h"
#include "mmpf_system.h"
#include "mmpf_graphics.h"
#include "mmpf_ldc.h"
#include "mmpf_ldc_ctl.h"

#if (HANDLE_LDC_EVENT_BY_TASK)

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

MMPF_LDC_CTL_QUEUE              m_sLdcCtlQueue;

MMPF_OS_SEMID 		            m_LdcCtlSem;

static MMP_ULONG                m_ulLdcReqCnt = 0;

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void _____Oueue_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_LDCCTL_ResetQueue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reset and clear queue
 @param[in] queue. current queue
*/
void MMPF_LDCCTL_ResetQueue(MMPF_LDC_CTL_QUEUE *queue)
{
    queue->head = 0;
    queue->size = 0;
    queue->weighted_size = 0;
    
    m_ulLdcReqCnt = 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDCCTL_GetQueueDepth
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Check if queue is empty
 @param[in] queue. current queue
 @param[in] weighted. get number of elements or total weighting of all elements.
*/
MMP_ULONG MMPF_LDCCTL_GetQueueDepth(MMPF_LDC_CTL_QUEUE *queue, MMP_BOOL weighted)
{
    if (weighted)
        return queue->weighted_size;
    else
        return queue->size;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDCCTL_PushQueue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Push 1 element to end of queue
 @param[in] queue. current queue
 @param[in] buffer. enqueue data
 @param[in] weighted. enqueue with one more element or increase the weighting of last element
*/
MMP_ERR MMPF_LDCCTL_PushQueue(MMPF_LDC_CTL_QUEUE *queue, MMPF_LDC_CTL_ATTR attr, MMP_BOOL weighted)
{
    MMP_ULONG idx;

    queue->weighted_size++;
    
    if (queue->size >= MMPF_LDC_MAX_QUEUE_SIZE) {
        
        RTNA_DBG_Byte(0, weighted);
        RTNA_DBG_Long(0, queue->weighted_size);
        RTNA_DBG_Str(0, " #Q full\r\n");
        
        // Increase the weighting of the last frame in queue
        idx = queue->head + queue->size - 1;
        if (idx >= MMPF_LDC_MAX_QUEUE_SIZE)
            idx -= MMPF_LDC_MAX_QUEUE_SIZE;
        
        queue->weight[idx]++;
        return MMP_LDC_ERR_QUEUE_OPERATION;
    }

    if (weighted) {
        // To increase the weighting of last element in queue, the queue must not be empty
        if (queue->size == 0) {
            DBG_S(0, "Weighted Q underflow\r\n");
            return MMP_LDC_ERR_QUEUE_OPERATION;
        }
        
        idx = queue->head + queue->size - 1;
        if (idx >= MMPF_LDC_MAX_QUEUE_SIZE)
            idx -= MMPF_LDC_MAX_QUEUE_SIZE;
            
        queue->weight[idx]++;
        // Don't change the buffer value
        RTNA_DBG_Str(3, "Q weight++\r\n");
        return MMP_ERR_NONE;
    }
    else {
        idx = queue->head + queue->size++;
        if (idx >= MMPF_LDC_MAX_QUEUE_SIZE)
            idx -= MMPF_LDC_MAX_QUEUE_SIZE;
        queue->weight[idx] = 1;
    }

    MEMCPY(&(queue->attr[idx]), &attr, sizeof(MMPF_LDC_CTL_ATTR));

    m_ulLdcReqCnt++;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_LDCCTL_PopQueue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Pop 1 element from queue
 @param[in] queue. current queue
 @param[in] offset. the offset of queue element to pop or the offset of acc weighting to pop
 @param[out] data. data queued in the specified offset
 @param[in] weighted. if the queue uses weighting for each element
*/
MMP_ERR MMPF_LDCCTL_PopQueue(MMPF_LDC_CTL_QUEUE *queue, MMP_ULONG offset, MMPF_LDC_CTL_ATTR *pData, MMP_BOOL weighted)
{
    MMP_ULONG target, source, acc_weight;

    if (weighted) {
        // For weighted queue, pop operation selects the element with 
        // accumlated weighting of all preceding elements equal to the specified offset
        if (offset >= queue->weighted_size) {
            DBG_S(0, "Pop Q invalid offset\r\n");
            return MMP_LDC_ERR_QUEUE_OPERATION;
        }
        else if (queue->weighted_size == 0) {
            DBG_S(0, "Pop Q underflow\r\n");
            return MMP_LDC_ERR_QUEUE_OPERATION;
        }

        target = queue->head;
        acc_weight = queue->weight[target];
        
        while (offset >= acc_weight) {
            acc_weight += queue->weight[target++];
            if (target >= MMPF_LDC_MAX_QUEUE_SIZE)
                target -= MMPF_LDC_MAX_QUEUE_SIZE;
        }
    }
    else {
        // For non-weighted queue, pop the element with the specified offset
        if (offset >= queue->size) {
            DBG_S(0, "Pop Q invalid offset\r\n");
            return MMP_LDC_ERR_QUEUE_OPERATION;
        }
        else if (queue->size == 0) {
            DBG_S(0, "Pop Q underflow\r\n");
            return MMP_LDC_ERR_QUEUE_OPERATION;
        }

        target = queue->head + offset;
        
        if (target >= MMPF_LDC_MAX_QUEUE_SIZE)
            target -= MMPF_LDC_MAX_QUEUE_SIZE;
    }

    queue->weighted_size--;

    MEMCPY(pData, &(queue->attr[target]), sizeof(MMPF_LDC_CTL_ATTR));

    if (m_ulLdcReqCnt > 0)
        m_ulLdcReqCnt--;

    // If queue is weighted and the weighting value is not zero,
    // pop queue will just decrease the weighting, instead of really dequeue.
    if (weighted && (queue->weight[target] > 1)) {
        queue->weight[target]--;
        return MMP_LDC_ERR_QUEUE_OPERATION;
    }
    
    // Shift elements before offset to tail direction.
    while (target != queue->head) { 
        source = (target)? (target-1): (MMPF_LDC_MAX_QUEUE_SIZE-1);
        MEMCPY(&(queue->attr[target]), &(queue->attr[source]), sizeof(MMPF_LDC_CTL_ATTR));
        queue->weight[target] = queue->weight[source];
        target = source;
    }
    
    queue->size--;

    queue->head++;
    
    if (queue->head >= MMPF_LDC_MAX_QUEUE_SIZE) {
        queue->head -= MMPF_LDC_MAX_QUEUE_SIZE;
    }

    return MMP_ERR_NONE;
}

#if 0
void _____LDC_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_ExeLoopBack
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_LDC_ExeLoopBack(MMPF_LDC_CTL_ATTR* pAttr)
{
	extern MMP_BOOL		m_bRetriggerGRA;
	extern MMP_USHORT	m_usLdcSliceIdx;
	
	MMP_GRAPHICS_BUF_ATTR 	src, dst;

	src.ulBaseAddr 	= (src.ulBaseUAddr = (src.ulBaseVAddr = 0));
	dst.ulBaseAddr 	= (dst.ulBaseUAddr = (dst.ulBaseVAddr = 0));
	
	src.colordepth  = MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE;
	
	/* LoopBack the first slice */
	if (m_usLdcSliceIdx >= MMPF_LDC_GetMultiSliceNum())
	{
	    m_bRetriggerGRA = !MMPF_Graphics_IsScaleIdle();

	    if (m_bRetriggerGRA == MMP_FALSE) {

			m_usLdcSliceIdx = 0;

			#if (LDC_DEBUG_MSG_EN)
			if (m_ulLdcFrmDoneCnt < LDC_DEBUG_FRAME_MAX_NUM) {
				#if (LDC_DEBUG_TIME_TYPE == LDC_DEBUG_TIME_STORE_TBL)
				extern MMP_ULONG gulLdcSliceStartTime[MAX_LDC_SLICE_NUM][LDC_DEBUG_TIME_TBL_MAX_NUM];
				gulLdcSliceStartTime[m_usLdcSliceIdx][m_ulLdcFrmDoneCnt] = OSTimeGet();
				#else
				printc(FG_RED(">Event S%d %d")"\r\n", m_usLdcSliceIdx, OSTimeGet());
				#endif
			}
			#endif
			
			MMPF_LDC_MultiSliceUpdateSnrId(pAttr->ubSensorId);
			
			/* Disable encode pipe and wait loopback done to re-enable it */
			MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)pAttr->ubEncodePipe, MMP_FALSE);
            MMPF_Scaler_SetEnable((MMP_SCAL_PIPEID)pAttr->ubCapturePipe, MMP_FALSE);
            
		    MMPF_LDC_MultiSliceUpdateSrcBufAddr(pAttr->sGraBufAttr.ulBaseAddr,
		                                        pAttr->sGraBufAttr.ulBaseUAddr,
		                                        pAttr->sGraBufAttr.ulBaseVAddr);
            
			MMPF_LDC_MultiSliceUpdatePipeAttr(m_usLdcSliceIdx);

		    MMPF_Graphics_ScaleStart(&src, &dst, 
		                             MMPF_LDC_MultiSliceNormalGraCallback, 
		                             MMPF_LDC_MultiSliceRetriggerGraCallback);
	    }
	}

    return MMP_ERR_NONE;
}

#if 0
void _____OS_Functions_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_LDC_Ctl_Task
//  Description :
//------------------------------------------------------------------------------
void MMPF_LDC_Ctl_Task(void)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;
    
    RTNA_DBG_Str(3, "MMPF_LDC_Ctl_Task()\r\n");

    m_LdcCtlSem = MMPF_OS_CreateSem(1);

    if (m_LdcCtlSem >= MMPF_OS_CREATE_SEM_EXCEED_MAX) {
        RTNA_DBG_Str(0, "m_LdcCtlSem Create Failed\r\n");
    }

    MMPF_LDCCTL_ResetQueue(&m_sLdcCtlQueue);

    wait_flags |= LDC_FLAG_EXE_LOOPBACK | LDC_FLAG_DMA_MOVE_FRAME;

    while(1) {

        MMPF_OS_WaitFlags(LDC_Ctl_Flag, wait_flags,
            			  MMPF_OS_FLAG_WAIT_SET_ANY| MMPF_OS_FLAG_CONSUME, 0, &flags);
        
        /* Pop Queue and execute LDC loopback */
        if (flags & LDC_FLAG_EXE_LOOPBACK) {

            MMPF_LDC_CTL_ATTR   sLdcCtlAttr;
            MMP_ERR             QueueErr	= MMP_ERR_NONE;
            MMP_ERR             LdcErr 		= MMP_ERR_NONE;
            
            if (MMPF_LDCCTL_GetQueueDepth(&m_sLdcCtlQueue, MMP_FALSE) != 0) {
   
                QueueErr = MMPF_LDCCTL_PopQueue(&m_sLdcCtlQueue, 0, &sLdcCtlAttr, MMP_FALSE);
                
                if (QueueErr == MMP_ERR_NONE) {
                    
                    if (MMPF_OS_AcquireSem(m_LdcCtlSem, LDC_DONE_SEM_TIMEOUT)) {
                    	RTNA_DBG_Str(0, FG_CYAN("LDC Acquire Sem TimeOut")"\r\n");
                    }
                    
                    LdcErr = MMPF_LDC_ExeLoopBack(&sLdcCtlAttr);
                    
                    if (m_ulLdcReqCnt > 0) {
                        MMPF_OS_SetFlags(LDC_Ctl_Flag, LDC_FLAG_EXE_LOOPBACK, MMPF_OS_FLAG_SET);
                    }
                }
                else {
                	RTNA_DBG_Str(3, FG_CYAN("LDC Pop Queue Fail")"\r\n");
                }
            }
            else {
            	RTNA_DBG_Str(3, FG_CYAN("LDC Queue Empty")"\r\n");
            }
        }
        
        if (flags & LDC_FLAG_DMA_MOVE_FRAME) {
        	MMPF_LDC_MultiSliceDmaMoveFrame();
        }
    }
}

#endif // HANDLE_LDC_EVENT_BY_TASK