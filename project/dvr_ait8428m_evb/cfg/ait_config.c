/**
  @file ait_config.c
  @brief It contains the configurations need to be ported to the customer platform.
  @author Rogers

  @version
- 1.0 Original version
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "ait_config.h"
#include "mmps_system.h"
#if defined(ALL_FW)
#include "ait_utility.h"
#endif

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPC_System_InitConfig
//  Description :
//------------------------------------------------------------------------------
MMP_BOOL MMPC_System_InitConfig(void)
{
    MMPS_SYSTEM_CONFIG *pConfig = MMPS_System_GetConfig();

    pConfig->ulStackMemoryStart 	= 0x1000000;
    pConfig->ulStackMemorySize 		= 64*1024*1024;

    #if 1 //DDR
    pConfig->stackMemoryType 		= MMPS_DRAM_TYPE_AUTO;
    pConfig->stackMemoryMode 		= MMPS_DRAM_MODE_DDR;
    #endif
    #if 0 //DDR2
    pConfig->stackMemoryType 		= MMPS_DRAM_TYPE_EXT;
    pConfig->stackMemoryMode 		= MMPS_DRAM_MODE_DDR2;
    #endif

    return MMP_TRUE;
}
