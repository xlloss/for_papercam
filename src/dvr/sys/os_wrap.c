//==============================================================================
//
//  File        : os_wrap.c
//  Description : OS wrapper functions
//  Author      : Jerry Tsao
//  Revision    : 1.0
//
//==============================================================================

#include "includes_fw.h"
#include "os_wrap.h"
#include "lib_retina.h"

/** @addtogroup MMPF_OS
@{
*/

//------------------------------
// User Specified Configuration
//------------------------------

OS_EVENT                *os_sem_tbl[MMPF_OS_SEMID_MAX];
OS_FLAG_GRP             *os_flag_tbl[MMPF_OS_FLAGID_MAX];
OS_EVENT		        *os_mutex_tbl[MMPF_OS_MUTEXID_MAX];
OS_EVENT				*os_mq_tbl[MMPF_OS_MQID_MAX];
MMPF_OS_TMR             os_tmr_tbl[MMPF_OS_TMRID_MAX];

#if defined(ALL_FW)&&(OS_MEM_EN == 1)&&((OGG_EN)||(MSDRM_WMA == 1))
#if (OGG_EN)
#define S_BUF_CNT (2)
#define S_BUF_PER_SIZE (32)
#define L_BUF_CNT (2)
#define L_BUF_PER_SIZE (0x100)
#endif

#if (MSDRM_WMA == 1)
#define S_BUF_CNT (21) // Must >=21, otherwise CPSIP cannot response SIP msg
//Andy could be 1 for DRM
#define S_BUF_PER_SIZE (200)
//Andy could be 0x20 for DRM
#define L_BUF_CNT (12) // Must >=12, otherwise CPSIP cannot response SIP msg
//Andy could be 1 for DRM
#define L_BUF_PER_SIZE (0xD034)
//Andy could be 0x20 for DRM
#endif

MMP_UBYTE MMPF_OS_SmallBufPool[S_BUF_CNT][S_BUF_PER_SIZE];
MMP_UBYTE MMPF_OS_LargeBufPool[L_BUF_CNT][L_BUF_PER_SIZE];
OS_MEM  *MMPF_OS_MemSmallPtr;
OS_MEM  *MMPF_OS_MemLargePtr;
#endif

//==============================================================================
//
//                              OS Wrap Functions
//
//==============================================================================
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
/** @brief To do os wrapper layer initialization process, but be called before
any other OS related API.

@return None.
*/

void MMPF_OS_Initialize(void)
{
#if defined(ALL_FW)&&(OS_MEM_EN == 1)&&((OGG_EN)||(MSDRM_WMA == 1))
    MMP_UBYTE err;
#endif
    MMP_ULONG   i;

    OSInit();

    for (i = 0; i < MMPF_OS_SEMID_MAX; i++) {
        os_sem_tbl[i] = 0x0;
    }

    for (i = 0; i < MMPF_OS_FLAGID_MAX; i++) {
        os_flag_tbl[i] = 0x0;
    }

    for (i = 0; i < MMPF_OS_MUTEXID_MAX; i++) {
        os_mutex_tbl[i] = 0x0;
    }

    for (i = 0; i < MMPF_OS_MQID_MAX; i++) {
        os_mq_tbl[i] = 0x0;
    }

#if defined(ALL_FW)&&(OS_MEM_EN == 1)&&((OGG_EN)||(MSDRM_WMA == 1))
    MMPF_OS_MemSmallPtr = OSMemCreate(MMPF_OS_SmallBufPool,S_BUF_CNT,S_BUF_PER_SIZE,&err);
    if (OS_NO_ERR != err) {
        RTNA_DBG_Str1("OSMemCreate fail:S.\r\n");
    }
    MMPF_OS_MemLargePtr = OSMemCreate(MMPF_OS_LargeBufPool,L_BUF_CNT,L_BUF_PER_SIZE,&err);
    if (OS_NO_ERR != err) {
        RTNA_DBG_Str1("OSMemCreate fail:L.\r\n");
    }
#endif
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_StartTask
//  Description :
//------------------------------------------------------------------------------
/** @brief To start the OS multi-task working.

@return None.
*/
void MMPF_OS_StartTask(void)
{
    OSStart();
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_CreateTask
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used to have OS manage the execution of a task.  Tasks can either
              be created prior to the start of multitasking or by a running task.  A task cannot be
              created by an ISR.

@param[in] taskproc : the task function entry pointer
@param[in] task_cfg : pointert to @ref MMPF_TASK_CFG structure, that deal with stack top/bottom address and task priority.
@param[in] param : input paramter to the taskproc
@retval 0xFE for bad input priority
		0xFF for createtask internal error from OS
		others, return the priority of the task.
*/
MMPF_OS_TASKID  MMPF_OS_CreateTask(void (*taskproc)(void *param), MMPF_TASK_CFG *task_cfg,
                    void *param)
{
    MMP_UBYTE ret;

    if (task_cfg->ubPriority > OS_LOWEST_PRIO) {
        return 0xFE;
    }

    ret = OSTaskCreateExt(taskproc,
                    param,
                    (OS_STK  *)(task_cfg->ptos),
                    task_cfg->ubPriority,
                    task_cfg->ubPriority,
                    (OS_STK  *)(task_cfg->pbos),
                    (task_cfg->ptos - task_cfg->pbos) / sizeof(OS_STK) + 1,
                    (void *) 0,
                    OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

    if (OS_NO_ERR != ret) {
        return 0xFF;
    }

    return task_cfg->ubPriority;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_SetTaskNam
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_OS_SetTaskName(MMP_UBYTE ubNewPriority, MMP_UBYTE *pTaskName)
{
    MMP_UBYTE ubRetVal = 0;
    MMP_ULONG ulLoop = 0;

    for (ulLoop=0; ulLoop < OS_TASK_NAME_SIZE; ++ulLoop) {
        if (*(pTaskName + ulLoop) == 0x0) {
            break;
        }
    }
    
    if (ulLoop >= OS_TASK_NAME_SIZE) {
        ubRetVal = 0xFF;
        RTNA_DBG_Str(0, "MMPF_OS_SetTaskName error(1)!\r\n");
        return ubRetVal;
    }

    OSTaskNameSet((INT8U)ubNewPriority, (INT8U *)pTaskName, (INT8U *)&ubRetVal);

    if (OS_NO_ERR != ubRetVal) {
        ubRetVal = 0xFF;
        RTNA_DBG_Str(0, "MMPF_OS_SetTaskName error(2)!\r\n");
        return ubRetVal;
    }

    return ubRetVal;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_ChangeTaskPriority
//  Description :
//------------------------------------------------------------------------------
/** @brief This function allows you to change the priority of a task dynamically.  Note that the new
*              priority MUST be available..

@param[in] taskid : the task ID that return by @ref MMPF_OS_CreateTask 
@param[in] ubNewPriority : new priority of the taskid.
@retval 0xFE for bad input task id,
		0xFF for createtask internal error from OS
		others, return the new priority of the task.
*/
MMPF_OS_TASKID  MMPF_OS_ChangeTaskPriority(MMPF_OS_TASKID taskid, MMP_UBYTE ubNewPriority)
{
    MMP_UBYTE ret;

    if (taskid > OS_LOWEST_PRIO && taskid != 0xFF) {
        return 0xFE;
    }

    ret = OSTaskChangePrio(taskid, ubNewPriority);

    if (OS_NO_ERR != ret) {
        return 0xFF;
    }

    return ubNewPriority;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_OS_DeleteTask
//  Description :
//------------------------------------------------------------------------------
/** @brief This function allows you to delete a task.  The calling task can delete itself by its own priority number.  
           The deleted task is returned to the dormant state and can be re-activated by creating the deleted task again.

@param[in] taskid : is the task ID to delete.  Note that you can explicitely delete the current task without knowing its 
					priority level by setting taskid to 0xFF.
@retval 0xFE for bad input task id,
		0xFF for deltask internal error from OS
		0, return delete success.
*/
MMP_UBYTE  MMPF_OS_DeleteTask(MMPF_OS_TASKID taskid)
{
    MMP_UBYTE ret;

    if (taskid > OS_LOWEST_PRIO && taskid != 0xFF) {
        return 0xFE;
    }

    ret = OSTaskDel(taskid);

    if (OS_NO_ERR != ret) {
        return 0xFF;
    }

    return 0;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_SuspendTask
//  Description :
//------------------------------------------------------------------------------
/** @brief This function allows you to suspend a task dynamically.

@param[in] taskid : the task ID that return by @ref MMPF_OS_CreateTask 
@retval 0xFE for bad input task id,
		0xFF for suspend task internal error from OS
		0, return suspend success.
*/
MMP_UBYTE  MMPF_OS_SuspendTask(MMPF_OS_TASKID taskid)
{
    MMP_UBYTE ret;

    if (taskid > OS_LOWEST_PRIO && taskid != 0xFF) {
        return 0xFE;
    }

    ret = OSTaskSuspend(taskid);

    if (OS_NO_ERR != ret) {
        return 0xFF;
    }

    return 0;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_ResumeTask
//  Description :
//------------------------------------------------------------------------------
/** @brief This function allows you to resume a suspended task.

@param[in] taskid : the task ID that return by @ref MMPF_OS_CreateTask 
@retval 0xFE for bad input task id,
		0xFF for resume task internal error from OS
		0, return resume success.
*/
MMP_UBYTE  MMPF_OS_ResumeTask(MMPF_OS_TASKID taskid)
{
    MMP_UBYTE ret;

    if (taskid > OS_LOWEST_PRIO && taskid != 0xFF) {
        return 0xFE;
    }

    ret = OSTaskResume(taskid);

    if (OS_NO_ERR != ret) {
        return 0xFF;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_LockSchedule
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used to prevent rescheduling to take place.
           This allows your application to prevent context switches until
           you are ready to permit context switching.

@retval 0xFF for not supported by OS
		0, success.
*/
MMP_UBYTE  MMPF_OS_LockSchedule(void)
{
#if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    return 0;
#else
    return 0xFF;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_UnlockSchedule
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used to re-allow rescheduling.

@retval 0xFF for not supported by OS
		0, success.
*/
MMP_UBYTE  MMPF_OS_UnlockSchedule(void)
{
#if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    return 0;
#else
    return 0xFF;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_CreateSem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function creates a semaphore.

@param[in] ubSemValue : is the initial value for the semaphore.  If the value is 0, no resource is
                        available.  You initialize the semaphore to a non-zero value to specify how many resources are available.
@retval 0xFF for create semaphore internal error from OS
		0xFE the system maximum semaphore counts exceed.
		others, the ID to access the semaphore
*/
MMPF_OS_SEMID MMPF_OS_CreateSem(MMP_UBYTE ubSemValue)
{
    MMP_ULONG   i;
    OS_EVENT    *ev;

    // find out a free entry in semaphore table first
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    for (i = 0; i < MMPF_OS_SEMID_MAX; i++) {
        if (os_sem_tbl[i] == 0) {
            os_sem_tbl[i] = (OS_EVENT *)0xFF; // just note as occupied
            break;
        }
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    if (i < MMPF_OS_SEMID_MAX) {
        ev = OSSemCreate(ubSemValue);
        if (ev) {
            os_sem_tbl[i] = ev;
            return i;
        }
        // create semaphore failed
        os_sem_tbl[i] = 0;
        return 0xFF;
    }

    return 0xFE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_DeleteSem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function deletes a semaphore and readies all tasks pending on the semaphore.

@param[in] ulSemId : The semaphore ID that return by @ref MMPF_OS_CreateSem 
@retval 0xFF for delete semaphore internal error from OS
		0, return delete success.
*/

MMP_UBYTE  MMPF_OS_DeleteSem(MMPF_OS_SEMID ulSemId)
{
    MMP_UBYTE   ret;
    OS_EVENT    *ev;

    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFF;
    }

    if (0x0 != os_sem_tbl[ulSemId]) {
        ev = os_sem_tbl[ulSemId];
        OSSemDel(ev, OS_DEL_ALWAYS, &ret);
        if (ret == OS_NO_ERR)
            os_sem_tbl[ulSemId] = 0x0;
        else
            return 0xFF;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_AcquireSem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function waits for a semaphore.

@param[in] ulSemId : The semaphore ID that return by @ref MMPF_OS_CreateSem 
@param[in] ulTimeout : is an optional timeout period (in clock ticks).  If non-zero, your task will
                            wait for the resource up to the amount of time specified by this argument.
                            If you specify 0, however, your task will wait forever at the specified
                            semaphore or, until the resource becomes available.
@retval 0xFE for bad input semaphore id,
		0xFF for acquire semaphore internal error from OS
		0 for getting the resource.
		1 for time out happens
*/

MMP_UBYTE MMPF_OS_AcquireSem(MMPF_OS_SEMID ulSemId, MMP_ULONG ulTimeout)
{
    MMP_UBYTE       ret;

    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFE;
    }

    OSSemPend(os_sem_tbl[ulSemId], ulTimeout, &ret);

	switch (ret) {
	case OS_NO_ERR:
		return 0;
	case OS_TIMEOUT:
		return 1;
    case OS_ERR_PEND_ISR:
        return 2;
	default:
        return 0xFF;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_ReleaseSem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function signals a semaphore

@param[in] ulSemId : The semaphore ID that return by @ref MMPF_OS_CreateSem 
@retval 0xFE for bad input semaphore id,
		0xFF for release semaphore internal error from OS
		0 for getting the resource.
		1 If the semaphore count exceeded its limit.
*/
MMP_UBYTE MMPF_OS_ReleaseSem(MMPF_OS_SEMID ulSemId)
{
    MMP_UBYTE       ret;

    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFE;
    }

    ret = OSSemPost(os_sem_tbl[ulSemId]);

    switch (ret) {
	case OS_NO_ERR:
		return 0;
	case OS_SEM_OVF:
		return 1;
    default:
        return 0xFF;
    }
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_AcceptSem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function requests for a semaphore.

@param[in] ulSemId : The semaphore ID that return by @ref MMPF_OS_CreateSem 
@param[out] usCount : The count of the ulSemId before accept
@retval 0xFE for bad input semaphore id,
		0xFF for acquire semaphore internal error from OS
		0 for getting the resource.
		1 for time out happens
*/
MMP_UBYTE MMPF_OS_AcceptSem(MMPF_OS_SEMID ulSemId, MMP_USHORT *usCount)
{
    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFE;
    }

    *usCount = OSSemAccept(os_sem_tbl[ulSemId]);
        
    return 0;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_QuerySem
//  Description :
//------------------------------------------------------------------------------
/** @brief This function obtains count about a semaphore

@param[in] ulSemId : The semaphore ID that return by @ref MMPF_OS_CreateSem 
@param[out] usCount : The count of the ulSemId
@retval 0xFE for bad input semaphore id,
		0xFF for query semaphore internal error from OS
		0 for no error
*/
MMP_UBYTE MMPF_OS_QuerySem(MMPF_OS_SEMID ulSemId, MMP_USHORT *usCount)
{
    OS_SEM_DATA 	semDataP;
    MMP_UBYTE       ret;

    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFE;
    }

    ret = OSSemQuery(os_sem_tbl[ulSemId], &semDataP);
    *usCount = semDataP.OSCnt;

    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
}

// MMPF_OS_QuerySemEx()
/*
 * return Sem cound - *usCount and
 *        Number of task waiting for sem - *usWait
 */
MMP_UBYTE MMPF_OS_QuerySemEx(MMPF_OS_SEMID ulSemId, MMP_USHORT *usCount, MMP_USHORT *usWait)
{
    OS_SEM_DATA 	semDataP;
    MMP_UBYTE       ret;
    int				i;

    if (ulSemId >= MMPF_OS_SEMID_MAX) {
        return 0xFE;
    }

    ret = OSSemQuery(os_sem_tbl[ulSemId], &semDataP);
    *usCount = semDataP.OSCnt;
    *usWait  = 0;
    for (i = 0; i < OS_EVENT_TBL_SIZE; i++) {
    	int		j;
        #if OS_LOWEST_PRIO <= 63
    	for (j = 0; j < 8; j++)
    		if ((semDataP.OSEventTbl[i] >> j) & 1) (*usWait)++;
        #else
    	for (j = 0; j < 16; j++)
    		if ((semDataP.OSEventTbl[i] >> j) & 1) (*usWait)++;
        #endif
	}
    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
}


//wilson@110704: for uvc porting
MMP_UBYTE MMPF_OS_SetSem(MMPF_OS_SEMID semID,MMP_USHORT count)
{
    MMP_UBYTE ret;

    if (semID >= MMPF_OS_SEMID_MAX) {
        return 0xFE; //[TBD]
    }

    OSSemSet(os_sem_tbl[semID],count,&ret);

    if (ret) {
        RTNA_DBG_Str(3, "<<OSSemSet err:");
        RTNA_DBG_Byte(3, ret);
        RTNA_DBG_Str(3, ">>\r\n");
    }
    return ret ;    
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_CreateMutex
//  Description :
//------------------------------------------------------------------------------
/** @brief This function creates a mutual exclusion semaphore.

@param[in] ubPriority : is the priority to use when accessing the mutual exclusion semaphore.  In
                            other words, when the semaphore is acquired and a higher priority task
                            attempts to obtain the semaphore then the priority of the task owning the
                            semaphore is raised to this priority.  It is assumed that you will specify
                            a priority that is LOWER in value than ANY of the tasks competing for the
                            mutex.
@retval 0xFF for create semaphore internal error from OS
		0xFE the system maximum semaphore counts exceed.
		others, the ID to access the semaphore
*/
MMPF_OS_MUTEXID MMPF_OS_CreateMutex(MMP_UBYTE ubPriority)
{
    MMP_ULONG   i;
    MMP_UBYTE   ret;
    OS_EVENT    *ev;

    // find out a free entry in mutex table first
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    for (i = 0; i < MMPF_OS_MUTEXID_MAX; i++) {
        if (os_mutex_tbl[i] == 0) {
            os_mutex_tbl[i] = (OS_EVENT *)0xFF; // just note as occupied
            break;
        }
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    if (i < MMPF_OS_MUTEXID_MAX) {
        ev = OSMutexCreate(ubPriority, &ret);
        if (ev) {
            os_mutex_tbl[i] = ev;
            return i;
        }

        os_mutex_tbl[i] = 0;
        return 0xFF;
    }

    return 0xFE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_DeleteMutex
//  Description :
//------------------------------------------------------------------------------
/** @brief This function deletes a mutual exclusion semaphore and readies all tasks pending on the it.

@param[in] ulMutexId : The mutex ID that return by @ref MMPF_OS_CreateMutex 
@retval 0xFF for delete mutex internal error from OS
		0, return delete success.
*/

MMP_UBYTE  MMPF_OS_DeleteMutex(MMPF_OS_MUTEXID ulMutexId)
{
    MMP_UBYTE       err;
    OS_EVENT    *ev;

    if (ulMutexId >= MMPF_OS_MUTEXID_MAX) {
        return 0xFF;
    }

    if (0x0 != os_mutex_tbl[ulMutexId]) {
        ev = os_mutex_tbl[ulMutexId];
        OSMutexDel(ev, OS_DEL_ALWAYS, &err);
        if (err == OS_NO_ERR)
            os_mutex_tbl[ulMutexId] = 0x0;
        else
            return 0xFF;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_AcquireMutex
//  Description :
//------------------------------------------------------------------------------
/** @brief This function waits for a mutual exclusion semaphore

@param[in] ulMutexId : The mutex ID that return by @ref MMPF_OS_CreateMutex 
@param[in] ulTimeout : is an optional timeout period (in clock ticks).  If non-zero, your task will
                            wait for the resource up to the amount of time specified by this argument.
                            If you specify 0, however, your task will wait forever at the specified
                            semaphore or, until the resource becomes available.
@retval 0xFE for bad input mutex id,
		0xFF for acquire mutex internal error from OS
		0 for getting the resource.
		1 for time out happens
*/

MMP_UBYTE MMPF_OS_AcquireMutex(MMPF_OS_MUTEXID ulMutexId, MMP_ULONG ulTimeout)
{
    MMP_UBYTE       ret;

    if (ulMutexId >= MMPF_OS_MUTEXID_MAX) {
        return 0xFE;
    }

    OSMutexPend(os_mutex_tbl[ulMutexId], ulTimeout, &ret);

	switch (ret) {
	case OS_NO_ERR:
		return 0;
	case OS_TIMEOUT:
		return 1;
	default:
        return 0xFF;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_ReleaseMutex
//  Description :
//------------------------------------------------------------------------------
/** @brief This function signals a mutual exclusion semaphore

@param[in] ulMutexId : The mutex ID that return by @ref MMPF_OS_CreateSem 
@retval 0xFE for bad input mutex id,
		0xFF for release mutex internal error from OS
		0 for getting the resource.
*/
MMP_UBYTE MMPF_OS_ReleaseMutex(MMPF_OS_MUTEXID ulMutexId)
{
    MMP_UBYTE       ret;

    if (ulMutexId >= MMPF_OS_MUTEXID_MAX) {
        return 0xFE;
    }

    ret = OSMutexPost(os_mutex_tbl[ulMutexId]);

    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_WaitFlags
//  Description :
//------------------------------------------------------------------------------
/** @brief This function waits for a semaphore.

@param[in] ulFlagID : The Flag ID that return by @ref MMPF_OS_CreateEventFlagGrp 
@param[in] flags : Is a bit pattern indicating which bit(s) (i.e. flags) you wish to wait for.
@param[in] waitType : specifies whether you want ALL bits to be set or ANY of the bits to be set.
                            You can specify the following argument:

                            MMPF_OS_FLAG_WAIT_CLR_ALL   You will wait for ALL bits in 'mask' to be clear (0)
                            MMPF_OS_FLAG_WAIT_SET_ALL   You will wait for ALL bits in 'mask' to be set   (1)
                            MMPF_OS_FLAG_WAIT_CLR_ANY   You will wait for ANY bit  in 'mask' to be clear (0)
                            MMPF_OS_FLAG_WAIT_SET_ANY   You will wait for ANY bit  in 'mask' to be set   (1)

                            NOTE: Add MMPF_OS_FLAG_CONSUME if you want the event flag to be 'consumed' by
                                  the call.  Example, to wait for any flag in a group AND then clear
                                  the flags that are present, set 'wait_type' to:

                                  MMPF_OS_FLAG_WAIT_SET_ANY + MMPF_OS_FLAG_CONSUME
@param[in] ulTimeout : is an optional timeout (in clock ticks) that your task will wait for the
                            desired bit combination.  If you specify 0, however, your task will wait
                            forever at the specified event flag group or, until a message arrives.
@param[out] ret_flags : The flags in the event flag group that made the task ready or, 0 if a timeout or an error
		*              occurred.
@retval 0xFE for bad input flag id,
		0xFF for wait flag internal error from OS
		0 for getting the flag.
		1 for time out happens
*/

MMP_UBYTE MMPF_OS_WaitFlags(MMPF_OS_FLAGID ulFlagID, MMPF_OS_FLAGS flags, MMPF_OS_FLAG_WTYPE waitType, 
							MMP_ULONG ulTimeout, MMPF_OS_FLAGS *ret_flags)
{
    MMP_UBYTE       ret;

    if (ulFlagID >= MMPF_OS_FLAGID_MAX) {
        return 0xFE;
    }

    *ret_flags = OSFlagPend(os_flag_tbl[ulFlagID], flags, waitType, ulTimeout, &ret);


    switch (ret) {
	case OS_NO_ERR:
		return 0;
	case OS_TIMEOUT:
		return 1;
    default:
        return 0xFF;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_SetFlags
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is called to set or clear some bits in an event flag group.  The bits to
              set or clear are specified by a 'bit mask'.

@param[in] ulFlagID : The flag ID that return by @ref MMPF_OS_CreateEventFlagGrp 
@param[in] flags : If 'opt' (see below) is MMPF_OS_FLAG_SET, each bit that is set in 'flags' will
                   set the corresponding bit in the event flag group.
				   If 'opt' (see below) is MMPF_OS_FLAG_CLR, each bit that is set in 'flags' will
				   CLEAR the corresponding bit in the event flag group.
@param[in] opt : MMPF_OS_FLAG_CLR for flag clear
				 MMPF_OS_FLAG_SET for flag set	
@retval 0xFE for bad input semaphore id,
		0xFF for setflag internal error from OS
		0 for calling was successfull
*/

MMP_UBYTE MMPF_OS_SetFlags(MMPF_OS_FLAGID ulFlagID, MMPF_OS_FLAGS flags, MMPF_OS_FLAG_OPT opt)
{
    MMP_UBYTE       ret;

    if (ulFlagID >= MMPF_OS_FLAGID_MAX) {
        return 0xFE;
    }

    OSFlagPost(os_flag_tbl[ulFlagID], flags, opt, &ret);

    switch (ret) {
    case OS_NO_ERR:
    	return 0;
    default:
        return 0xFF;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_QueryFlags
//  Description :
//------------------------------------------------------------------------------
/** @brief This function obtains count about a semaphore

@param[in] ulFlagID : The flag ID that return by @ref MMPF_OS_CreateEventFlagGrp 
@param[out] ret_flags : The current value of the event flag group.
@retval 0xFE for bad input flag id,
		0xFF for query flag internal error from OS
		0 for no error
*/
MMP_UBYTE MMPF_OS_QueryFlags(MMPF_OS_FLAGID ulFlagID, MMPF_OS_FLAGS *ret_flags)
{
    MMP_UBYTE		ret;

    if (ulFlagID >= MMPF_OS_FLAGID_MAX) {
        return ulFlagID;
    }

    *ret_flags = OSFlagQuery(os_flag_tbl[ulFlagID], &ret);

    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_CreateEventFlagGrp
//  Description :
//------------------------------------------------------------------------------
/** @brief This function creates a flag.

@retval 0xFF for create flag internal error from OS
		0xFE the system maximum flag counts exceed.
		others, the ID to access the flag
*/
MMPF_OS_FLAGID MMPF_OS_CreateEventFlagGrp(MMP_ULONG ulFlagValues)
{
    MMP_ULONG		i;
    MMP_UBYTE		ret;
    OS_FLAG_GRP		*flags;

    // find out a free entry in flag table first
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    for (i = 0; i < MMPF_OS_FLAGID_MAX; i++) {
        if (os_flag_tbl[i] == 0) {
            os_flag_tbl[i] = (OS_FLAG_GRP *)0xFF; // just note as occupied
            break;
        }
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    if (i < MMPF_OS_FLAGID_MAX) {
        flags = OSFlagCreate(ulFlagValues, &ret);
        if (flags) {
            os_flag_tbl[i] = flags;
            return i;
        }

        os_flag_tbl[i] = 0;
        return 0xFF;
    }

    return 0xFE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_DeleteMutex
//  Description :
//------------------------------------------------------------------------------
/** @brief This function deletes an event flag group and readies all tasks pending on the event flag
           group.

@param[in] ulFlagId : The mutex ID that return by @ref MMPF_OS_CreateEventFlagGrp 
@retval 0xFF for delete flag group internal error from OS
		0, return delete success.
*/

MMP_UBYTE  MMPF_OS_DeleteEventFlagGrp(MMPF_OS_FLAGID ulFlagId)
{
    MMP_UBYTE       ret;
    OS_FLAG_GRP    	*ev;

    if (ulFlagId >= MMPF_OS_FLAGID_MAX) {
        return 0xFF;
    }
    
    if (0x0 != os_flag_tbl[ulFlagId]) {
        ev = os_flag_tbl[ulFlagId];
        OSFlagDel(ev, OS_DEL_ALWAYS, &ret);
        if (ret == OS_NO_ERR)
            os_flag_tbl[ulFlagId] = 0x0;
        else
            return 0xFF;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_CreateMQueue
//  Description :
//------------------------------------------------------------------------------
/** @brief This function creates a message queue if free event control blocks are available.

@param[in] msg : is a pointer to the base address of the message queue storage area.  The
                            storage area MUST be declared as an array of pointers to 'void'.
@param[in] ubQueueSize : is the number of elements in the storage area
@retval 0xFF for create message queue internal error from OS
		0xFE the system maximum message queues counts exceed.
		others, the ID to access the message queue
*/

MMPF_OS_MQID MMPF_OS_CreateMQueue(void **msg, MMP_USHORT ubQueueSize)
{
    MMP_ULONG   i;
    OS_EVENT    *ev;

    // find out a free entry in msgQ table first
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    for (i = 0; i < MMPF_OS_MQID_MAX; i++) {
        if (os_mq_tbl[i] == 0) {
            os_mq_tbl[i] = (OS_EVENT *)0xFF; // just note as occupied
            break;
        }
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    if (i < MMPF_OS_MQID_MAX) {
        ev = OSQCreate(msg, ubQueueSize);
        if (ev) {
            os_mq_tbl[i] = ev;
            return i;
        }

        os_mq_tbl[i] = 0;
        return 0xFF;
    }

    return 0xFE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_DeleteMQueue
//  Description :
//------------------------------------------------------------------------------
/** @brief This function deletes a message queue and readies all tasks pending on the queue.

@param[in] ulMQId : The message queue ID that return by @ref MMPF_OS_CreateMQueue 
@retval 0xFF for delete message queue internal error from OS
		0, return delete success.
*/

MMP_UBYTE  MMPF_OS_DeleteMQueue(MMPF_OS_MQID ulMQId)
{
    MMP_UBYTE   ret;
    OS_EVENT    *ev;

    if (ulMQId >= MMPF_OS_MQID_MAX) {
        return 0xFF;
    }

    if (0x0 != os_mq_tbl[ulMQId]) {
        ev = os_mq_tbl[ulMQId];
        OSQDel(ev, OS_DEL_ALWAYS, &ret);
        if (ret == OS_NO_ERR)
            os_mq_tbl[ulMQId] = 0x0;
        else
            return 0xFF;
    }

    return 0;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_GetMessage
//  Description :
//------------------------------------------------------------------------------
/** @brief This function waits for a message to be sent to a queue

@param[in] ulMQId : The message queue ID that return by @ref MMPF_OS_CreateMQueue 
@param[in] ulTimeout : is an optional timeout period (in clock ticks).  If non-zero, your task will
                            wait for a message to arrive at the queue up to the amount of time
                            specified by this argument.  If you specify 0, however, your task will wait
                            forever at the specified queue or, until a message arrives.
@retval 0xFE for bad message queue id,
		0xFF for acquire message queue internal error from OS
		0 for getting the message.
		1 for time out happens
*/

MMP_UBYTE MMPF_OS_GetMessage(MMPF_OS_MQID ulMQId, void **msg, MMP_ULONG ulTimeout)
{
    MMP_UBYTE       ret;

    if (ulMQId >= MMPF_OS_MQID_MAX) {
        return 0xFE;
    }

    *msg = OSQPend(os_mq_tbl[ulMQId], ulTimeout, &ret);

	switch (ret) {
	case OS_NO_ERR:
		return 0;
	case OS_TIMEOUT:
		return 1;
	default:
        return 0xFF;
    }
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OS_PutMessage
//  Description :
//------------------------------------------------------------------------------
/** @brief This function sends a message to a queue

@param[in] ulMQId : The message queue ID that return by @ref MMPF_OS_CreateMQueue 
@param[in] msg : is a pointer to the message to send.
@retval 0xFE for bad message queue id,
		0xFF for put message queue internal error from OS
		0 for getting the message.
*/

MMP_UBYTE MMPF_OS_PutMessage(MMPF_OS_MQID ulMQId, void *msg)
{
    MMP_UBYTE       ret;

    if (ulMQId >= MMPF_OS_MQID_MAX) {
        return 0xFE;
    }

    ret = OSQPost(os_mq_tbl[ulMQId], msg);

	switch (ret) {
	case OS_NO_ERR:
		return 0;
	default:
        return 0xFF;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_FlushMQueue
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_OS_FlushMQueue(MMPF_OS_MQID ulMQId)
{
    MMP_UBYTE       ret;

    if (ulMQId >= MMPF_OS_MQID_MAX) {
        return 0xFE;
    }

    ret = OSQFlush(os_mq_tbl[ulMQId]);

    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_QueryMQueue
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_OS_QueryMQueue(MMPF_OS_MQID ulMQId, MMP_USHORT *usCount)
{
    OS_Q_DATA       pData;
    MMP_UBYTE       ret;

    if (ulMQId >= MMPF_OS_MQID_MAX) {
        return 0xFE;
    }

    ret = OSQQuery(os_mq_tbl[ulMQId], &pData);
    *usCount = pData.OSNMsgs;

    switch (ret) {
	case OS_NO_ERR:
		return 0;
    default:
        return 0xFF;
    }
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_InInterrupt
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used to check whether current CPU state is in 
           interrupt mode (IRQ/FIQ).

@retval MMP_TRUE if CPU state is interrupt mode, otherwise returns MMP_FALSE
*/
MMP_BOOL MMPF_OS_InInterrupt(void)
{
    if (OSIntNesting == 0)
        return MMP_FALSE;
    else
        return MMP_TRUE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_Sleep
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is called to delay execution of the currently running task until the
              specified number of system ticks expires.  This, of course, directly equates to delaying
              the current task for some time to expire.  No delay will result If the specified delay is
              0.  If the specified delay is greater than 0 then, a context switch will result.

@param[in] usTickCount : is the time delay that the task will be suspended in number of clock 'ticks'.
                        Note that by specifying 0, the task will not be delayed.
@retval 0 always return 0 for success
*/
MMP_UBYTE MMPF_OS_Sleep(MMP_USHORT usTickCount)
{
    OSTimeDly(usTickCount);
    
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_SleepMs
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is called to delay execution of the currently running
           task until the specified millisecond expires. 

@param[in] ulMiliSecond : is the time delay that the task will be suspended
                          in number of millisecode.
                          Note that by specifying 0, the task will not be delayed.
@retval 0 always return 0 for success
*/
MMP_UBYTE MMPF_OS_SleepMs(MMP_ULONG ulMiliSecond)
{
    MMP_USHORT usDelay;

    while(ulMiliSecond > 0) {
        usDelay = (ulMiliSecond < 5000) ? ulMiliSecond : 5000;
        OSTimeDly(usDelay);
        ulMiliSecond -= usDelay;
    }
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_SetTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function sets the 32-bit counter which keeps track of the number of clock ticks.

@param[in] ulTickCount : specifies the new value that OSTime needs to take.
@retval 0 always return 0 for success
*/
MMP_UBYTE MMPF_OS_SetTime(MMP_ULONG ulTickCount)
{
    OSTimeSet(ulTickCount);
    
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_GetTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function sets the 32-bit counter which keeps track of the number of clock ticks.

@param[out] ulTickCount : specifies the new value that OSTime needs to take.
@retval 0 always return 0 for success
*/
MMP_UBYTE MMPF_OS_GetTime(MMP_ULONG *ulTickCount)
{
    *ulTickCount = OSTimeGet();
    
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_GetTimeMs
//  Description :
//------------------------------------------------------------------------------
/** @brief This function sets the 32-bit counter which keeps track of the number of minisecond.

@param[out] ulTickCount : specifies the new value that OSTime needs to take.
@retval 0 always return 0 for success
*/
MMP_UBYTE MMPF_OS_GetTimeMs(MMP_ULONG *ulTickCount)
{
    *ulTickCount = OSTimeGet()*(1000/OS_TICKS_PER_SEC);
    
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_LocalTimerCallback
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is the OS timer callback in wrap layer. This function
           have to free the timer itself if it is a one-shot timer, and call
           the application specified callback.

@retval None
*/
void MMPF_OS_LocalTimerCallback(void *ptmr, void *parg)
{
    MMPF_OS_TMR *tmr_id = (MMPF_OS_TMR *)parg;
    void        *arg;
    OS_TMR_CALLBACK pfnct;

    pfnct = tmr_id->OSTmrCallback;
    arg = tmr_id->OSTmrCallbackArg;

    /* For one-shot timer, free it when it is expired.
     * Application may call MMPF_OS_StopTimer() in callback, so here we release
     * tmr before executing callback.
     */
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    if (tmr_id->bStarted && (tmr_id->tmr->OSTmrOpt == OS_TMR_OPT_ONE_SHOT)) {
        tmr_id->OSTmrCallback = NULL;
        tmr_id->OSTmrCallbackArg = NULL;
        tmr_id->tmr = NULL;
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    /* Execute callback function if available */
    if (pfnct != (OS_TMR_CALLBACK)0) {
        (*pfnct)(ptmr, arg);
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_StartTimer
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is called by your application code to create and start
           a timer.

@param[in] ulPeriod : The time (in OS ticks) before the timer expires.
                      If you specified 'MMPF_OS_TMR_OPT_PERIODIC' as an option,
                      when the timer expires, it will automatically restart
                      with the same period.
@param[in] opt      : Specifies either: 
                      MMPF_OS_TMR_OPT_ONE_SHOT  The timer counts down only once and then is deleted
                      MMPF_OS_TMR_OPT_PERIODIC  The timer counts down and then reloads itself
@param[in] callback : Is a pointer to a callback function that will be called when the timer expires.  The
                      callback function must be declared as follows:
                      void MyCallback (OS_TMR *ptmr, void *p_arg);
@param[in] callback_arg : Is an argument (a pointer) that is passed to the
                      callback function when it is called.

@retval NULL for start timer internal error from OS
		others, the ID to access the timer
*/
MMPF_OS_TMRID MMPF_OS_StartTimer(MMP_ULONG ulPeriod, MMPF_OS_TMR_OPT opt,
                            MMPF_OS_TMR_CALLBACK callback, void *callback_arg)
{
    MMP_ULONG   i;
    MMP_UBYTE   ret;

    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    for (i = 0; i < MMPF_OS_TMRID_MAX; i++) {
        /* Find an idle one */
        if (os_tmr_tbl[i].bStarted == MMP_FALSE) {
            os_tmr_tbl[i].bStarted = MMP_TRUE;
            os_tmr_tbl[i].OSTmrCallback = callback;
            os_tmr_tbl[i].OSTmrCallbackArg = callback_arg;
            break;
        }
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    if (i < MMPF_OS_TMRID_MAX) {
        os_tmr_tbl[i].tmr = OSTmrStart(ulPeriod, opt, MMPF_OS_LocalTimerCallback,
                                       (void *)&os_tmr_tbl[i], NULL, &ret);
        if (os_tmr_tbl[i].tmr) {
            return i;
        }

        os_tmr_tbl[i].OSTmrCallback = NULL;
        os_tmr_tbl[i].OSTmrCallbackArg = NULL;
        os_tmr_tbl[i].bStarted = MMP_FALSE;
        return 0xFF;
    }

    return 0xFE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_OS_StopTimer
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is called by your application code to stop and delete
           a timer.

@param[in] ulTmrId : The timer ID that return by @ref MMPF_OS_StartTimer 
@param[in] opt     : Allows you to specify an option to this functions which can be:
                   OS_TMR_OPT_NONE      Do nothing special but stop the timer
                   OS_TMR_OPT_CALLBACK  Execute the callback function, pass it
                                        the callback argument specified when
                                        the timer was created.
@warning Please don't call MMPF_OS_StopTimer() inside the registered callback
         of MMPF_OS_StartTimer() for non one-shot timer, otherwise, you will
         get a dead-lock.

@retval 0xFF for stop timer internal error from OS
        0xFE for input ulTmrId is out of range
		0, for stop timer sucessfully
*/
MMP_UBYTE MMPF_OS_StopTimer(MMPF_OS_TMRID ulTmrId, MMPF_OS_TMR_OPT opt)
{
    MMP_UBYTE   ret = OS_NO_ERR;
    OS_TMR      *tmr;

    if (MMPF_OS_TMRID_MAX <= ulTmrId) {
        return 0xFE;
    }
    /* We have to make sure one-shot timer will not expired during we are
     * stopping it. So, here we lock OS scheduling.
     */
    #if OS_SCHED_LOCK_EN > 0
    OSSchedLock();
    #endif
    if (os_tmr_tbl[ulTmrId].bStarted) {
        os_tmr_tbl[ulTmrId].bStarted = MMP_FALSE;

        tmr = os_tmr_tbl[ulTmrId].tmr;
        os_tmr_tbl[ulTmrId].tmr = NULL;
        os_tmr_tbl[ulTmrId].OSTmrCallback = NULL;
        os_tmr_tbl[ulTmrId].OSTmrCallbackArg = NULL;

        #if OS_SCHED_LOCK_EN > 0
        OSSchedUnlock();
        #endif

        if (tmr && tmr->OSTmrActive)
            OSTmrStop(tmr, opt, NULL, &ret);
        if (ret != OS_NO_ERR)
            return 0xFF;

        return 0;
    }
    #if OS_SCHED_LOCK_EN > 0
    OSSchedUnlock();
    #endif

    return 0;
}

#if OS_TASK_STAT_EN > 0
void MMPF_OSTaskStatHook(void)
{
		printc("CPU Usage = %d%%\r\n", OSCPUUsage);
}
#endif

#if defined(ALL_FW)&&(OS_MEM_EN == 1)&&((OGG_EN)||(MSDRM_WMA == 1))

void *MMPF_OS_Malloc(int len)
{
    unsigned char err;
    void *tmpAddr=0;

    if (len > L_BUF_PER_SIZE) {
        RTNA_DBG_Long(0, len);
        RTNA_DBG_Str(0, " = alloc len, malloc resource unavailable.\r\n");
        return NULL;
    }

	RTNA_DBG_Str(0, "malloc : ");
	RTNA_DBG_Long(0, len);
	RTNA_DBG_Str(0, " : ");

    if(len <= S_BUF_PER_SIZE) {
        tmpAddr = (void *)OSMemGet(MMPF_OS_MemSmallPtr,&err);
        if (OS_NO_ERR!=err) {
            return 0; //[TBD]
        }
        return tmpAddr;
    }
    else if(len >= S_BUF_PER_SIZE) {
        tmpAddr = (void *)OSMemGet(MMPF_OS_MemLargePtr,&err);

        if (OS_NO_ERR != err) {
            return 0; //[TBD]
        }

		RTNA_DBG_Str(0, "get : ");
		RTNA_DBG_Long(0, (MMP_ULONG)tmpAddr);
		RTNA_DBG_Str(0, "\r\n");

        return tmpAddr;
    }
    return NULL;
}

void *MMPF_OS_Calloc(int num, int size)
{
    unsigned char   err;
    void            *tmpAddr;
    unsigned long   wantSize;

    wantSize = num*size;
    if (wantSize > L_BUF_PER_SIZE) {
        RTNA_DBG_Str(0, "calloc resource unavailable.\r\n");
        return NULL;
    }
    if(wantSize <= S_BUF_PER_SIZE) {
        tmpAddr = (void *)OSMemGet(MMPF_OS_MemSmallPtr,&err);
        if (OS_NO_ERR!=err) {
            return 0; //[TBD]
        }
        memset(tmpAddr, 0, wantSize);

        return tmpAddr;
    }
    else if(wantSize >= S_BUF_PER_SIZE) {
        tmpAddr = (void *)OSMemGet(MMPF_OS_MemLargePtr,&err);

        if (OS_NO_ERR!=err) {
            return 0; //[TBD]
        }
        memset(tmpAddr, 0, wantSize);

        return tmpAddr;
    }
    return NULL;
}

void MMPF_OS_MemFree(char *mem_ptr)
{
    unsigned char err;
    /**
    Philip: Here using a very easy mechanism to identify memory pool.
    It DID NOT handle "memory access violation" issue.
    */
	RTNA_DBG_Str(0, "free : ");
	RTNA_DBG_Long(0, (MMP_ULONG)mem_ptr);
	RTNA_DBG_Str(0, "\r\n");
    
    if( (unsigned)((unsigned long)mem_ptr - (unsigned long)&MMPF_OS_LargeBufPool[0][0]) < (&MMPF_OS_LargeBufPool[L_BUF_CNT-1][L_BUF_PER_SIZE]-&MMPF_OS_LargeBufPool[0][0]) ){
        err = OSMemPut(MMPF_OS_MemLargePtr,mem_ptr);
        if (OS_NO_ERR != err) {
            return ; //[TBD]
        }
    }
    else if( (unsigned)((unsigned long)mem_ptr - (unsigned long)&MMPF_OS_SmallBufPool[0][0]) < (&MMPF_OS_SmallBufPool[S_BUF_CNT-1][S_BUF_PER_SIZE]-&MMPF_OS_SmallBufPool[0][0])) {
        err = OSMemPut(MMPF_OS_MemSmallPtr,mem_ptr);
        if (OS_NO_ERR!=err) {
            return ; //[TBD]
        }
    }
    else {
        RTNA_DBG_Str(0, "MMPF_OS_MemFree fail: UNKNOWN!\r\n");
        return ; //[TBD]
    }
}
#endif
/** @} */ // MMPF_OS
