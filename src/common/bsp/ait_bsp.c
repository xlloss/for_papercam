/**
  @file ait_bsp.c
  @brief It contains the BSP dependent configurations need to be ported to the customer platform.
  @author Philip

  @version
- 1.0 Original version
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//============================================================================== 

#include "ait_bsp.h"
#include "ait_utility.h"

//==============================================================================
//
//                              FUNCTIONS
//
//============================================================================== 

void MMPC_System_WaitCount(MMP_ULONG count) 
{
    while (count--);
}

void MMPC_System_WaitMs(MMP_ULONG ulMs)
{
	MMPC_System_WaitCount((HOST_CLK_M / HOST_WHILE_CYCLE) * ulMs * 1000);
}

void MMPC_System_WaitUs(MMP_ULONG ulUs)
{
	MMPC_System_WaitCount((HOST_CLK_M / HOST_WHILE_CYCLE) * ulUs);
}
