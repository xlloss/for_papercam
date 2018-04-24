/// @ait_only
//==============================================================================
//
//  File        : mmpd_system.c
//  Description : Ritian System Control Device Driver Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

/**
 *  @file mmpd_system.c
 *  @brief The header File for the Host System Control Device Driver Function
 *  @author Penguin Torng
 *  @version 1.0
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_system.h"
#include "mmph_hif.h"
#include "mmp_reg_gbl.h"
#include "mmpf_pll.h"
#include "ait_utility.h"
#include "mmpf_pll.h"
#include "mmpf_system.h"

/** @addtogroup MMPD_System
 *  @{
 */

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPD_System_ResetHModule
//  Description :
//------------------------------------------------------------------------------
/** @brief The function resets hardware module

The function resets hardware module
@param[in] moduletype the module
@param[in] bResetRegister MMP_TRUE: reset the registers and state of the module; MMP_FALSE: only reset the state of the module.
@return It reports the status of the operation.
*/
MMP_ERR MMPD_System_ResetHModule(MMPD_SYS_MDL moduletype, MMP_BOOL bResetRegister)
{
	return MMPF_SYS_ResetHModule((MMPF_SYS_MDL)moduletype, bResetRegister);
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_GetSramEndAddr
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the end address of the FW in SRAM region
 
 This API gets the end address of FW codes in SRAM region.
 @param[out] ulAddress End address of FW in SRAM region.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_System_GetSramEndAddr(MMP_ULONG *ulAddress)
{
#if defined(ALL_FW)
	return MMPF_SYS_GetFWFBEndAddr(ulAddress);
#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_ReadCoreID
//  Description :
//------------------------------------------------------------------------------
/** @brief Check the chip code ID

This function Get the code ID of the chip
@return It reports the status of the operation.
*/
MMP_UBYTE MMPD_System_ReadCoreID(void)
{
	return MMPF_SYS_ReadCoreID();
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_EnableClock
//  Description :
//------------------------------------------------------------------------------
/** @brief The function enables or disables the specified clock

The function enables or disables the specified clock from the clock type input by programming the
Global controller registers.

@param[in] ulClockType the clock type to be selected
@param[in] bEnableclock enable or disable the clock
@return It reports the status of the operation.
*/
MMP_ERR MMPD_System_EnableClock(MMPD_SYS_CLK clocktype, MMP_BOOL bEnable)
{
    return MMPF_SYS_EnableClock(clocktype, bEnable);
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_TVInitialize
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPD_System_TVInitialize(MMP_BOOL bInit)
{
    MMP_ERR	mmpstatus = MMP_ERR_NONE;

    return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_TVColorBar
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPD_System_TVColorBar(MMP_TV_TYPE tvType, MMP_BOOL turnOn, MMP_UBYTE colorBarType)
{
    MMP_ERR	mmpstatus = MMP_ERR_NONE;

    return mmpstatus;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_System_GetGroupFreq
//  Description :
//------------------------------------------------------------------------------
/** @brief Get the group freq.
@param[in]  ubGroup Select the group.
@param[out] ulGroupFreq The current group freq.
@return Always return success.
*/
MMP_ERR MMPD_System_GetGroupFreq(CLK_GRP ubGroup, MMP_ULONG *ulGroupFreq)
{
    MMP_ERR	mmpstatus = MMP_ERR_NONE;

#if defined(ALL_FW)
    mmpstatus = MMPF_PLL_GetGroupFreq(ubGroup, ulGroupFreq);
#endif
    return  mmpstatus;
}

/// @}
/// @end_ait_only

