/// @ait_only
/**
 @file mmpd_mp4venc.c
 @brief Retina Video Encoder Control Driver Function
 @author Will Tseng
 @version 1.0
*/

#ifdef BUILD_CE
#undef BUILD_FW
#endif

#include "mmp_lib.h"
#include "mmpd_mp4venc.h"
#include "mmpd_system.h"
#include "mmph_hif.h"
#include "mmp_reg_h264enc.h"
#include "mmp_reg_h264dec.h"
#include "mmp_reg_gbl.h"
#include "ait_utility.h"
#ifdef BUILD_CE
#include "lib_retina.h"
#endif
#include "mmpf_mp4venc.h"
#include "mmpf_mci.h"
#include "mmpf_vif.h"

/** @addtogroup MMPD_MP4VENC
 *  @{
 */

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
/**
 @brief Acquire one encoder instance and initialize it
 @param[out] InstId Encoder instance id
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_InitInstance(MMP_ULONG *InstId)
{
    return MMPF_VIDENC_InitInstance(InstId);
}

/**
 @brief Release a encoder instance with the specified id
 @param[in] InstId Encoder instance id
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_DeInitInstance(MMP_ULONG InstId)
{
    return MMPF_VIDENC_DeInitInstance(InstId);
}

/**
 @brief Set encoding resolution, width and height.
 @param[in] usWidth Encode width.
 @param[in] usHeight Encode height.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetResolution(  MMP_ULONG   ulEncId,
                                    MMP_USHORT  usWidth,
                                    MMP_USHORT  usHeight)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 4, usWidth);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 6, usHeight);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_RESOLUTION |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set video cropping
 @param[in] usTop       cropping lines in top
 @param[in] usBottom    cropping lines in bottom
 @param[in] usLeft      cropping lines in left
 @param[in] usRight     cropping lines in right
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetCropping(MMP_ULONG   ulEncId,
                                MMP_USHORT  usTop,
                                MMP_USHORT  usBottom,
                                MMP_USHORT  usLeft,
                                MMP_USHORT  usRight)
{
    MMP_ERR err;

    if ((usTop >= 16) || (usBottom >= 16) ||
        (usLeft >= 16) || (usRight >= 16) ||
        (usTop & 0x01) || (usBottom & 0x01) ||
        (usLeft & 0x01) || (usRight & 0x01)) {
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 4, usTop);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 6, usBottom);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 8, usLeft);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 10, usRight);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_CROPPING |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set video padding for video height
 @param[in] usType padding type 0: zero, 1: repeat
 @param[in] usCnt  the height line offset which need to pad
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetPadding( MMP_ULONG   ulEncId,
                                MMP_USHORT  usType,
                                MMP_USHORT  usCnt)
{
    MMP_ERR err;

    if (usCnt > 15) {
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;
    }

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 4, usType);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 6, usCnt);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_PADDING |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encoding current buffer mode to H264, MPEG-4 or H.263 & API I/F.
 @param[in] VideoFormat Video current buffer mode.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetCurBufMode(MMP_ULONG                 ulEncId,
                                MMPD_MP4VENC_CURBUF_MODE    VideoCurBufMode)
{
    MMP_ERR err;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterW(GRP_IDX_VID, 4, VideoCurBufMode);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_CURBUFMODE |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encode profile
 @param[in] profile Video profile.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetProfile(MMP_ULONG ulEncId, VIDENC_PROFILE profile)
{
    MMP_ULONG   ulProfile;
    MMP_ERR	    err = MMP_ERR_NONE;

    if ((profile <= H264ENC_PROFILE_NONE) || (profile >= H264ENC_PROFILE_MAX))
        return MMP_MP4VE_ERR_NOT_SUPPORTED_PARAMETERS;

    switch(profile) {
    case H264ENC_BASELINE_PROFILE:
    	ulProfile = BASELINE_PROFILE;
        break;
    case H264ENC_MAIN_PROFILE:
    	ulProfile = MAIN_PROFILE;
        break;
    case H264ENC_HIGH_PROFILE:
    	ulProfile = FREXT_HP;
        break;
    default:
        PRINTF("Unsupported profile\r\n");
        break;
    }

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulProfile);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_PROFILE |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set video encode level
 @param[in] level Video encode level.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetLevel(MMP_ULONG ulEncId, MMP_ULONG level)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, level);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_LEVEL |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set video encode entropy coding
 @param[in] entropy Video entropy coding
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetEntropy(MMP_ULONG ulEncId, VIDENC_ENTROPY entropy)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, entropy);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_ENTROPY |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set P frame number
 @param[in] ubFrameCnt 
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetGOP( MMP_ULONG   ulEncId,
                            MMP_USHORT  usPFrame,
                            MMP_USHORT  usBFrame)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
	MMPH_HIF_SetParameterW(GRP_IDX_VID, 4, usPFrame);
	MMPH_HIF_SetParameterW(GRP_IDX_VID, 6, usBFrame);
	err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_GOP |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

	return err;
}

/**
 @brief Set video bitrate in unit of bps
 @param[in] ulBitrate bitrate
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetBitrate(MMP_ULONG ulEncId, MMP_ULONG ulBitrate)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulBitrate);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_BITRATE |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

	return err;
}

/**
 @brief Set video target bitrate for ramp-up in unit of bps
 @param[in] ulBitrate bitrate
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetTargetBitrate(MMP_ULONG ulEncId, MMP_ULONG ulTargetBr,MMP_ULONG ulRampupBr)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulTargetBr);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, ulRampupBr);
    
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_BITRATE_TARGET |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

	return err;
}


MMP_ERR MMPD_VIDENC_SetInitalQP(MMP_ULONG   ulEncId,
                                MMP_UBYTE   *ubInitQP)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ubInitQP[0]);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, ubInitQP[1]);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, SET_QP_INIT |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encode QP high/low boundary
 @param[in] lowBound QP low boundary
 @param[in] highBound QP high boundary
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetQPBoundary(  MMP_ULONG   ulEncId,
                                    MMP_ULONG   lowBound,
                                    MMP_ULONG   highBound)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, lowBound);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, highBound);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, SET_QP_BOUNDARY |
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);
    
    return err;
}

/**
 @brief Set encode rate control mode
 @param[in] mode rate control mode
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetRcMode(MMP_ULONG ulEncId, VIDENC_RC_MODE mode)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, mode);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_RC_MODE |
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encode rate control skip
 @param[in] enable enable or disable rate control skip
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetRcSkip(MMP_ULONG ulEncId, MMP_BOOL skip)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, skip);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_RC_SKIP|
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encode rate control skip in direct mode or smooth mode
 @param[in] type skip method
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetRcSkipType(MMP_ULONG ulEncId, VIDENC_RC_SKIPTYPE type)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, type);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_RC_SKIPTYPE |
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set encode rate leaky bucket size
 @param[in] lbs leaky bucket size
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetRcLbSize(MMP_ULONG ulEncId, MMP_ULONG lbs)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, lbs);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_RC_LBS |
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Control encode TNR feature
 @param[in] tnr TNR features
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetTNR(MMP_ULONG ulEncId, MMP_ULONG tnr)
{
	MMP_ERR	err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, tnr);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_TNR_EN |
                                        HIF_VID_CMD_RECD_PARAMETER);    
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}


/**
 @brief Set encode frame rate and time frame factor.
 @param[in] usTimeIncrement Time increment.
 @param[in] usTimeResol Time resolution.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetEncFrameRate(MMP_ULONG   ulEncId,
                                    MMP_ULONG   ulIncr,
                                    MMP_ULONG   ulResol)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulResol);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, ulIncr);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_FRAME_RATE |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Dynamically update encode frame rate and time frame factor.
 @param[in] usTimeIncrement Time increment.
 @param[in] usTimeResol Time resolution.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_UpdateEncFrameRate( MMP_ULONG   ulEncId,
                                        MMP_ULONG   ulIncr,
                                        MMP_ULONG   ulResol)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulResol);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, ulIncr);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_FRAME_RATE_UPD |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Set sensor input frame rate and time frame factor.
 @param[in] usTimeIncrement Time increment.
 @param[in] usTimeResol Time resolution.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetSnrFrameRate(MMP_ULONG   ulEncId,
                                    MMP_ULONG   ulIncr,
                                    MMP_ULONG   ulResol)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulResol);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, ulIncr);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, SNR_FRAME_RATE |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Force to encode contiguous I-frame with the specified count
 @param[in] ulCnt The number of contiguous I-frames
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_ForceI(MMP_ULONG ulEncId, MMP_ULONG ulCnt)
{
    MMP_ERR err = MMP_ERR_NONE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ulEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, ulCnt);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_FORCE_I |
                                        HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

MMP_ERR MMPD_VIDENC_EnableClock(MMP_BOOL bEnable)
{
    MMPD_System_EnableClock(MMPD_SYS_CLK_H264, bEnable);
	MMPD_System_EnableClock(MMPD_SYS_CLK_AUD, bEnable);
    MMPD_System_EnableClock(MMPD_SYS_CLK_ADC, bEnable);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPD_VIDENC_GetStatus
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Check the firmware video encoding engine status.
 @param[out] status Firmware video engine status.
 @retval MMP_ERR_NONE Success.
 @note

 The parameter @a status can not be changed because it sync with the firmware
 video engine status definitions. It can be
 - 0x0000 MMP_VIDRECD_FW_STATUS_OP_NONE
 - 0x0001 MMP_VIDRECD_FW_STATUS_OP_START
 - 0x0002 MMP_VIDRECD_FW_STATUS_OP_PAUSE
 - 0x0003 MMP_VIDRECD_FW_STATUS_OP_RESUME
 - 0x0004 MMP_VIDRECD_FW_STATUS_OP_STOP
*/
MMP_ERR MMPD_VIDENC_GetStatus(MMP_ULONG ubEncId, MMPD_MP4VENC_FW_OP *status)
{
	MMP_ERR	err = MMP_ERR_NONE;
    
    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
    err = MMPH_HIF_SendCmd(GRP_IDX_VID, VIDRECD_STATUS |
                                        HIF_VID_CMD_RECD_PARAMETER);
	if (err) {
        MMPH_HIF_ReleaseSem(GRP_IDX_VID);
		return err;
    }

    *status = MMPH_HIF_GetParameterW(GRP_IDX_VID, 0);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return err;
}

/**
 @brief Check the capability of video encoding engine.
 @param[out] If the specified resolution and frame rate can be supported.
 @retval MMP_ERR_NONE for Supported.
 @note
*/
MMP_ERR MMPD_VIDENC_CheckCapability(MMP_ULONG w, MMP_ULONG h, MMP_ULONG fps)
{
    MMP_ERR	    ret = MMP_ERR_NONE;
    MMP_BOOL    supported = MMP_FALSE;

    MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ((w + 15) >> 4) * ((h + 15) >> 4));
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, fps);
    ret = MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_CAPABILITY |
                                        HIF_VID_CMD_RECD_PARAMETER);
    if (ret == MMP_ERR_NONE)
        supported = MMPH_HIF_GetParameterB(GRP_IDX_VID, 0);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    if (!supported) // unsupported resol & frame rate
        ret = MMP_MP4VE_ERR_CAPABILITY;

    return ret;
}

/**
 @brief Fine tune MCI priority to fit VGA size encoding. It's an access issue.
 @param[in] ubMode Mode.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_TuneMCIPriority(MMPD_VIDENC_MCI_MODE mciMode)
{
    return MMPF_MCI_TunePriority(mciMode);
}

/**
 @brief Fine tune MCI priority of encode pipe
 @param[in] ubPipe Encode pipe
 @retval MMP_ERR_NONE Success.
*/
void MMPD_VIDENC_TuneEncodePipeMCIPriority(MMP_UBYTE ubPipe)
{
    MMPF_MCI_SetIBCMaxPriority(ubPipe);
}

#if 0
void ____VidEnc_Buffer_Control____(){ruturn;} //dummy
#endif

/**
 @brief Set video Reference/Generate buffer bound
 @param[in] *refgenbd Pointer of encode buffer structure.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetRefGenBound( MMP_ULONG               ubEncId,
                                    MMPD_MP4VENC_REFGEN_BD  *refgenbd)
{
    // Set ref buf
    // Set ref buf bound
	
	MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, refgenbd->ulYStart);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, refgenbd->ulUEnd);

	MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_REFGENBD | HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return MMP_ERR_NONE;
}

/**
 @brief Set video compressed buffer for video encoding bitstream.

 Assign this in the internal frame buffer can get better performance. Minus buffer
 end address by 16 is to avoid buffer overflow.
 @param[in] *bsbuf Pointer of encode buffer structure.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetBitstreamBuf(MMP_ULONG                   ubEncId,
                                    MMPD_MP4VENC_BITSTREAM_BUF  *bsbuf)
{
	MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, bsbuf->ulStart);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, bsbuf->ulEnd);

	MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_BSBUF | HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return MMP_ERR_NONE;
}

/**
 @brief Set slice length, MV buffer.
 @param[in] *miscbuf Pointer of encode buffer structure.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_SetMiscBuf( MMP_ULONG               ubEncId,
                                MMPD_MP4VENC_MISC_BUF   *miscbuf)
{
	MMPH_HIF_WaitSem(GRP_IDX_VID, 0);
	MMPH_HIF_SetParameterL(GRP_IDX_VID, 0, ubEncId);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 4, miscbuf->ulMVBuf);
    MMPH_HIF_SetParameterL(GRP_IDX_VID, 8, miscbuf->ulSliceLenBuf);

	MMPH_HIF_SendCmd(GRP_IDX_VID, ENCODE_MISCBUF | HIF_VID_CMD_RECD_PARAMETER);
    MMPH_HIF_ReleaseSem(GRP_IDX_VID);

    return MMP_ERR_NONE;
}

/**
 @brief Calculate and Generate the REF/GEN buffer for Video Encode Module.

 Depends on encoded resolution to generate the REF/GEN buffer
 @param[in] usWidth Encode width.
 @param[in] usHeight Encode height.
 @param[out] refgenbd Mp4 engine require REF/GEN buffer.
 @param[in,out] ulCurAddr Available start address for buffer start.
 @retval MMP_ERR_NONE Success.
*/
MMP_ERR MMPD_VIDENC_CalculateRefBuf(MMP_USHORT              usWidth,
                                    MMP_USHORT              usHeight, 
                                    MMPD_MP4VENC_REFGEN_BD  *refgenbd,
                                    MMP_ULONG               *ulCurAddr)
{
	MMP_ULONG	bufsize;

    *ulCurAddr = ALIGN32(*ulCurAddr);

	bufsize = usWidth * usHeight;
	refgenbd->ulYStart  = *ulCurAddr;
	*ulCurAddr += bufsize;
	refgenbd->ulYEnd    = *ulCurAddr;

	bufsize /= 2;
	refgenbd->ulUStart  = *ulCurAddr;
	*ulCurAddr += bufsize;
	refgenbd->ulUEnd    = *ulCurAddr;
	refgenbd->ulVStart  = 0;
	refgenbd->ulVEnd    = 0;

    *ulCurAddr = ALIGN32(*ulCurAddr);

    #if (SHARE_REF_GEN_BUF == 1)
    refgenbd->ulGenStart    = refgenbd->ulYStart;
	refgenbd->ulGenEnd      = refgenbd->ulUEnd;
    #else
    #if (H264ENC_ICOMP_EN)
	bufsize = usWidth * usHeight;

	refgenbd->ulGenYStart = *ulCurAddr;
	*ulCurAddr += bufsize;
	refgenbd->ulGenYEnd = *ulCurAddr;
	refgenbd->ulGenUVStart = *ulCurAddr;
	*ulCurAddr += bufsize/2;
	refgenbd->ulGenUVEnd = *ulCurAddr;
    #else
	bufsize = (usWidth * usHeight * 3)/2; // Frame total size
	refgenbd->ulGenStart    = *ulCurAddr;
	*ulCurAddr += bufsize;
	refgenbd->ulGenEnd      = *ulCurAddr;
    #endif//(H264ENC_ICOMP_EN)
    #endif

	return	MMP_ERR_NONE;
}

#ifdef BUILD_CE
#define BUILD_FW
#endif

/// @}
/// @end_ait_only
