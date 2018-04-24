/**
 *  @file mmps_system.h
 *  @brief Header file the host system API
 *  @author Jerry Tsao, Truman Yang
 *  @version 1.1
 */
#ifndef _MMPS_SYSTEM_H_
#define _MMPS_SYSTEM_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "ait_bsp.h"
#include "config_fw.h"
#include "mmpd_system.h"

/** @addtogroup MMPS_System
@{
*/

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum _MMPS_DRAM_TYPE
{
    MMPS_DRAM_TYPE_NONE = 0,	// no stack memory
    MMPS_DRAM_TYPE_1,			// first used
    MMPS_DRAM_TYPE_2,			// second used
    MMPS_DRAM_TYPE_3,			// third used
    MMPS_DRAM_TYPE_EXT,
    MMPS_DRAM_TYPE_AUTO
} MMPS_DRAM_TYPE;

typedef enum _MMPS_DRAM_MODE
{
    MMPS_DRAM_MODE_SDRAM = 0,	// SD RAM
    MMPS_DRAM_MODE_DDR,			// DDR RAM
    MMPS_DRAM_MODE_DDR2,
    MMPS_DRAM_MODE_DDR3,
    MMPS_DRAM_MODE_NUM
} MMPS_DRAM_MODE;

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

/** @brief Type of by pass pin controller call back function.

The responsibility of this function should set the bypass ping according to the input
@param[in] value Value 1 for high, value 0 for low.
@return If the initalize successful
*/
typedef struct _MMPS_SYSTEM_CONFIG {
    MMPS_DRAM_TYPE		stackMemoryType;		///< Stack memory type inside AIT chip
    MMPS_DRAM_MODE      stackMemoryMode;    	///< DDR or SDRAM
	MMP_ULONG           ulStackMemoryStart;		///< Stack memory start address
	MMP_ULONG           ulStackMemorySize;		///< Stack memory size
} MMPS_SYSTEM_CONFIG;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMPS_SYSTEM_CONFIG* MMPS_System_GetConfig(void);

/// @}

#endif // _MMPS_SYSTEM_H_
