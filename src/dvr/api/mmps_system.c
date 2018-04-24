//==============================================================================
//
//  File        : mmps_system.c
//  Description : Ritian System Control function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

/**
@file mmps_system.c
@brief The System Control functions
@author Penguin Torng
@version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmps_system.h"

/** @addtogroup MMPS_System
@{
*/

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

/**@brief The system configuration

Use @ref MMPS_System_GetConfig to assign the field value of it.
You should read this functions for more information.
*/
static MMPS_SYSTEM_CONFIG	m_systemConfig;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPS_System_GetConfig
//  Description :
//------------------------------------------------------------------------------
/** @brief The function gets the current system configuration for the host application

The function gets the current system configuration for reference by the host application. The current
configuration can be accessed from output parameter pointer. The function calls
MMPD_System_GetConfiguration to get the current settings from Host Device Driver Interface.

@return It return the pointer of the system configuration data structure.
*/
MMPS_SYSTEM_CONFIG *MMPS_System_GetConfig(void)
{
    return &m_systemConfig;
}

/// @}