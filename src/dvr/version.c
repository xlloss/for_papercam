//==============================================================================
//
//  File        : version.c
//  Description : Version Control Function of Firmware Core
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "version.h"

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : DVR_Version
//  Description :
//------------------------------------------------------------------------------
/** @brief The function returns the current version of firmware core

The function returns the version of firmware
@param[out] major the major number of version.
@param[out] minor the minor number of version.
@param[out] patch the patch number of version.
@return none.
*/
void DVR_Version(MMP_USHORT *major, MMP_USHORT *minor, MMP_USHORT *patch)
{
    *major = VERSION_MAJOR;
    *minor = VERSION_MINOR;
    *patch = VERSION_PATCH;
}
