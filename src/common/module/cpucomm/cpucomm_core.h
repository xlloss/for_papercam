#ifndef __CPU_COMM_BUS_H__
#define __CPU_COMM_BUS_H__

#include "cpucomm.h"
#if defined(OS_TYPE) && (OS_TYPE==OS_LINUX)
#include "mach/mmp_cpucomm.h"
#else
#include "mmp_cpucomm.h"
#endif

// 2 CPU may access the adjacent entry which allocated in the same cache line ther is no alignment with cache line size.
// it would be a issue when cache is flushed/invalidated.
#define CACHE_LINE_SIZE (32)

// Buffer size, current is 32-4=28 byte, because the global swap buffer is implement by register AITPS_CPU_SHARE
#define SWAP_BUFFER_SIZE (sizeof(((AITPS_CPU_SHARE)0)->CPU_SAHRE_REG)/2-sizeof(MMP_ULONG)/*-sizeof(MMPF_OS_FLAGS)-sizeof(MMPF_OS_FLAG_OPT)*/)

// Swap buffer between two CPU
typedef volatile struct __CPU_COMM_SWAP_DATA
{
    MMP_ULONG               ulCommId;
    MMP_UBYTE               pbyBuffer[SWAP_BUFFER_SIZE];
} CPU_COMM_SWAP_DATA;

// Socket data structure
typedef struct __CPU_COMM_DATA
{
    MMPF_OS_SEMID           ulFlagId;
    MMPF_OS_SEMID           ulSemId;
    CPU_COMM_SWAP_DATA      sData;
} CPU_COMM_DATA;

// Entry of socket table
typedef struct __CPU_COMM_ENTRY
{
    MMP_ULONG               ulCommId;
    CPU_COMM_TYPE           ulCommType;
    CPU_COMM_DATA           sCommData;
} CPU_COMM_ENTRY;

typedef struct __CPU_LOCKCORE 
{
	char _q_[2] ;
	char _turn_ ;
	char _corenesting ;
} CPU_LOCKCORE ;

void CpuComm_HwInit( _CPU_ID ulCpuId );
void CpuComm_SwapISR( void );
CPU_COMM_ERR CpuComm_SocketPost( CPU_COMM_ID ulCommId, unsigned long subCommandID );

#endif // __CPU_COMM_BUS_H__
