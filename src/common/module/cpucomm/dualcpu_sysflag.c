//==============================================================================
//
//  File        : dualcpu_sysflag.c
//  Description : Sys flag for system communication between dual CPU
//  Author      : Chiket Lin
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
#include <stdio.h>
#include <stdarg.h>
#include "includes_fw.h"
#include "lib_retina.h"

#include "cpucomm.h"
#include "cpucomm_bus.h"

//==============================================================================
//
//                              Macro & Define
//
//==============================================================================

//==============================================================================
//
//                              Global Variable
//
//==============================================================================
void __UartWrite( const char *pWrite_Str );
//------------------------------------------------------------------------------
//  Variable    : A flag for the notification between dual CPU
//  Description :
//------------------------------------------------------------------------------
CPU_COMM_ERR DualCpu_SysflagInit(void)
{
//printc( "DualCpu_SysflagInit\r\n" );

    return CpuComm_RegisterEntry( CPU_COMM_ID_SYSFLAG, CPU_COMM_TYPE_FLAG );
}
CPUCOMM_MODULE_INIT(DualCpu_SysflagInit)


CPU_COMM_ERR DualCpu_SysflagExit(void)
{
    return CpuComm_UnregisterEntry( CPU_COMM_ID_SYSFLAG );
}
CPUCOMM_MODULE_EXIT(DualCpu_SysflagExit)

//==============================================================================
//
//                              Functions
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : DualCpu_SysFlag_CpuBInitDone
//  Description : CPU B trigger a flag to notify CPU A that the init is done
//------------------------------------------------------------------------------
void DualCpu_SysFlag_CpuBInitDone(void)
{
    CPU_COMM_ERR ulErr;
    char msg[] = "0!!!\r\n";

    ulErr = CpuComm_FlagSet( CPU_COMM_ID_SYSFLAG );
    if( ulErr != CPU_COMM_ERR_NONE )
    {
        msg[0] = '0' + ulErr;
        printc( msg );
    }
}

//------------------------------------------------------------------------------
//  Function    : DualCpu_SysFlag_WaitTerminateCmd
//  Description : CPU B main thread wait the termination flag from CPU A
//------------------------------------------------------------------------------
void DualCpu_SysFlag_WaitTerminateCmd(void)
{
    CpuComm_FlagGet( CPU_COMM_ID_SYSFLAG, 0 );
}
