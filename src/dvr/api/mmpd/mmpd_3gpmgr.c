/// @ait_only
/**
 @file mmpd_3gpmgr.c
 @brief Retina 3GP Merger Control Driver Function
 @author Will Tseng
 @version 1.0
*/

/** @addtogroup MMPD_3GPMGR
 *  @{
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_3gpmgr.h"
#include "mmpd_mp4venc.h"
#include "mmph_hif.h"
#include "mmp_reg_gbl.h"
#include "ait_utility.h"
#include "mmpf_vstream.h"

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
//  Function    : MMPD_3GPMGR_StartCapture
//  Description : 
//------------------------------------------------------------------------------
/**
 @brief Start capture audio/video
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_3GPMGR_StartCapture(MMP_ULONG ubEncId)
{
    MMP_ERR	    status = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
	MMPH_HIF_SendCmd(GRP_IDX_VID, HIF_VID_CMD_MERGER_OPERATION | MERGER_START);
	status = MMPH_HIF_GetParameterL(GRP_IDX_VID, 4);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_3GPMGR_StopCapture
//  Description : 
//------------------------------------------------------------------------------
/**
 @brief Stop capture audio/video
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_3GPMGR_StopCapture(MMP_ULONG ubEncId)
{
	MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
	MMPH_HIF_SendCmd(GRP_IDX_VID, HIF_VID_CMD_MERGER_OPERATION | MERGER_STOP);
	MMPH_HIF_ReleaseSem(GRP_IDX_VID);
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_3GPMGR_SetEncodeCompBuf
//  Description : 
//------------------------------------------------------------------------------
/**
 @brief Set start address and size of firmware compressed buffer.
 @param[in] *BufInfo Pointer of encode buffer structure.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_3GPMGR_SetEncodeCompBuf(MMP_ULONG ubEncId, MMP_ULONG addr, MMP_ULONG size)
{
    MMPF_VStream_SetCompBuf(ubEncId, addr, size);
    return MMP_ERR_NONE;
}

/// @}

/// @end_ait_only
