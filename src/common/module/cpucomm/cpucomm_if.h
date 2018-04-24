#ifndef _INCLUDE_CPU_COMM_COMMUNICATION_IF_H_
#define _INCLUDE_CPU_COMM_COMMUNICATION_IF_H_

#include "cpucomm_id.h"
//#include "cpucomm_if.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	 LOCK_CORE()   OS_LockCore()
#define	 UNLOCK_CORE() OS_UnlockCore()

#define  CPU_LOCK_INIT() OS_CRITICAL_INIT()
	
#define  CPU_LOCK() OS_ENTER_CRITICAL(); \
					LOCK_CORE()
					
#define  CPU_UNLOCK() UNLOCK_CORE();		\
		 			OS_EXIT_CRITICAL()
				


//------------------------------------------------------------------------------
//  Enumeration : CPU_COMM_TYPE
//  Description : Define type of commnucation
 //------------------------------------------------------------------------------
typedef enum
{
    CPU_COMM_TYPE_SOCKET,              // A socket service between 2 CPUs
    CPU_COMM_TYPE_DATA = CPU_COMM_TYPE_SOCKET,
    CPU_COMM_TYPE_FLAG,                 // A flag between two CPUs
    CPU_COMM_TYPE_SEM = CPU_COMM_TYPE_FLAG
} CPU_COMM_TYPE;

// Error message for socket operation
typedef enum __CPU_COMM_ERR
{
    CPU_COMM_ERR_NONE = 0,  // no error
    CPU_COMM_ERR_INIT_FAIL, // communication service is not init yet
    CPU_COMM_ERR_DESTROY,   // communication service is destroied
    CPU_COMM_ERR_OPENED,    // socket is opend alerady
    CPU_COMM_ERR_CLOSED,    // socket is closed already
    CPU_COMM_ERR_BUSY,      // request is porcessing data or client did not read yet
    CPU_COMM_ERR_IDLE,      // no request is send yet
    CPU_COMM_ERR_TIMEOUT,   // swap data timeout
    CPU_COMM_ERR_UNSUPPORT, // Unsupport method
    CPU_COMM_ERR_EVENT,     // Error from event flag operation
    CPU_COMM_ERR_OVERFLOW   // Data size is too big
} CPU_COMM_ERR;



CPU_COMM_ERR CpuComm_RegisterEntry( CPU_COMM_ID ulCommId, CPU_COMM_TYPE ulCommType );
CPU_COMM_ERR CpuComm_UnregisterEntry( CPU_COMM_ID ulCommId );
CPU_COMM_ERR CpuComm_RegisterISRService(CPU_COMM_ID ulCommId , unsigned int (*recv_cb)(void *slot)) ;

// Flag operations (for dual CPU)
CPU_COMM_ERR CpuComm_FlagSet( CPU_COMM_ID ulCommId );
CPU_COMM_ERR CpuComm_FlagGet( CPU_COMM_ID ulCommId, MMP_ULONG ulTimeout );

// Socket operations for client (service rountine is declared as callback)
CPU_COMM_ERR CpuComm_SocketSend( CPU_COMM_ID ulCommId, MMP_UBYTE *pbyData, MMP_ULONG ulDataSize );
CPU_COMM_ERR CpuComm_SocketReceive( CPU_COMM_ID ulCommId, MMP_UBYTE *pbyData, MMP_ULONG ulDataSize, MMP_BOOL bPost);

// Peterson's 2-process Mutual Exclusion Algorithm
void OS_LockCore(void) ;
void OS_UnlockCore(void/*int cpu_sr*/);
void OS_CPULockInfo(void)  ;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _INCLUDE_CPU_COMM_COMMUNICATION_IF_H_
