#ifndef _INCLUDE_CPU_COMM_COMMUNICATION_H_
#define _INCLUDE_CPU_COMM_COMMUNICATION_H_

#include "mmpf_typedef.h"
#include "os_wrap.h"
#include "cpucomm_if.h"

//------------------------------------------------------------------------------
//  Macro       : CpuComm_CriticalSectionInit()
//  Description : Encapsulated for critical section
//------------------------------------------------------------------------------
#define CpuComm_CriticalSectionDeclare()
#define CpuComm_CriticalSectionInit()      OS_CRITICAL_INIT()
#define CpuComm_CriticalSectionEnter()     OS_ENTER_CRITICAL()
#define CpuComm_CriticalSectionLeave()     OS_EXIT_CRITICAL()

#endif // _INCLUDE_CPU_COMM_COMMUNICATION_H_
