//------------------------------------------------------------------------------
//
//  File        : mmpf_bayerscaler.c
//  Description : Firmware Bayer Scaler Control Function
//  Author      : Eroy Yang
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "lib_retina.h"
#include "mmp_reg_color.h"
#include "mmpf_scaler.h"
#include "mmpf_bayerscaler.h"

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMP_BAYER_SCALER_INFO m_BayerScalerInfo[MMP_BAYER_SCAL_MAX_MODE];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_BayerScaler_SetEngine
//  Description : This function set bayer scaler engine.
//------------------------------------------------------------------------------
/** 
 * @brief This function set bayer scaler engine.
 * 
 *  This function set bayer scaler engine.
 * @param[in]     bUserdefine  : stands for use user define range.
 * @param[in/out] fitrange     : stands for input/output range.
 * @param[in/out] grabInfo     : stands for output grab range. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_BayerScaler_SetEngine(	MMP_SCAL_FIT_RANGE 	*fitrange,
                        			MMP_SCAL_GRAB_CTRL	*grabInfo)
{
    AITPS_BAYER_SCAL pBSCAL = AITC_BASE_BAYERSCAL;

    AIT_REG_W   *OprGrabInHStart, *OprGrabInHEnd, *OprGrabInVStart, *OprGrabInVEnd;
    AIT_REG_W   *OprGrabOutHStart, *OprGrabOutHEnd, *OprGrabOutVStart, *OprGrabOutVEnd;
    AIT_REG_W   *OprNh, *OprMh, *OprNv, *OprMv;

    OprGrabInHStart = &(pBSCAL->BAYER_SCAL_GRAB_IN_H_ST);
    OprGrabInHEnd   = &(pBSCAL->BAYER_SCAL_GRAB_IN_H_ED);
    OprGrabInVStart = &(pBSCAL->BAYER_SCAL_GRAB_IN_V_ST);
    OprGrabInVEnd   = &(pBSCAL->BAYER_SCAL_GRAB_IN_V_ED);
    OprGrabOutHStart= &(pBSCAL->BAYER_SCAL_GRAB_OUT_H_ST);
    OprGrabOutHEnd  = &(pBSCAL->BAYER_SCAL_GRAB_OUT_H_ED);
    OprGrabOutVStart= &(pBSCAL->BAYER_SCAL_GRAB_OUT_V_ST);
    OprGrabOutVEnd  = &(pBSCAL->BAYER_SCAL_GRAB_OUT_V_ED);
    OprNh           = &(pBSCAL->BAYER_SCAL_H_N);
    OprMh           = &(pBSCAL->BAYER_SCAL_H_M);
    OprNv           = &(pBSCAL->BAYER_SCAL_V_N);
    OprMv           = &(pBSCAL->BAYER_SCAL_V_M);

    *OprGrabInHStart 	= fitrange->ulInGrabX;
    *OprGrabInHEnd   	= fitrange->ulInGrabX + fitrange->ulInGrabW - 1 + fitrange->ulDummyInPixelX;
    *OprGrabInVStart 	= fitrange->ulInGrabY;
    *OprGrabInVEnd   	= fitrange->ulInGrabY + fitrange->ulInGrabH - 1 + fitrange->ulDummyInPixelY;

    *OprGrabOutHStart 	= grabInfo->ulOutStX;
    *OprGrabOutHEnd   	= grabInfo->ulOutEdX + fitrange->ulDummyOutPixelX;
    *OprGrabOutVStart 	= grabInfo->ulOutStY;
    *OprGrabOutVEnd   	= grabInfo->ulOutEdY + fitrange->ulDummyOutPixelY;

	if (fitrange->fitmode == MMP_SCAL_FITMODE_OUT ||
		fitrange->fitmode == MMP_SCAL_FITMODE_IN) 
	{
	    *OprNh	= grabInfo->ulScaleN;
	    *OprMh	= grabInfo->ulScaleM;
	    *OprNv	= grabInfo->ulScaleN;
	    *OprMv	= grabInfo->ulScaleM;
	}
	else if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL)
	{
	    *OprNh	= grabInfo->ulScaleXN;
	    *OprMh	= grabInfo->ulScaleXM;
	    *OprNv	= grabInfo->ulScaleYN;
	    *OprMv	= grabInfo->ulScaleYM;
	}
	
	#ifdef SCAL_FUNC_DBG
    {
        static MMP_ULONG oldxn = 0, oldxm = 0, oldyn = 0, oldym = 0, oldinw = 0, oldinh = 0;

        if ((oldxn != grabInfo->ulScaleXN)  || (oldxm != grabInfo->ulScaleXM) || 
            (oldyn != grabInfo->ulScaleYN)  || (oldym != grabInfo->ulScaleYM) || 
            (oldinw != fitrange->ulInGrabW) || (oldinh != fitrange->ulInGrabH)) {

			MMPF_Scaler_DumpSetting((void *)__func__, fitrange, grabInfo);

            oldxn  = grabInfo->ulScaleXN;
            oldxm  = grabInfo->ulScaleXM;
            oldyn  = grabInfo->ulScaleYN;
            oldym  = grabInfo->ulScaleYM;
            oldinw = fitrange->ulInGrabW;
            oldinh = fitrange->ulInGrabH;
        }
    }
	#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BayerScaler_SetEnable
//  Description : This function enable bayer scaler.
//------------------------------------------------------------------------------
/** 
 * @brief This function enable bayer scaler.
 * 
 *  This function enable bayer scaler.
 * @param[in]     bEnable  : stands for enable bayer scaler or not.
 * @return It return the function status.  
 */
MMP_ERR MMPF_BayerScaler_SetEnable(MMP_BOOL bEnable)
{
    AITPS_BAYER_SCAL pBSCAL = AITC_BASE_BAYERSCAL;
    AIT_REG_B   *OprScalCtl;

    OprScalCtl = &(pBSCAL->BAYER_SCAL_CTL);

    if (bEnable == MMP_TRUE) {
        (*OprScalCtl) &= ~(BAYER_SCAL_BYPASS);
    }
    else {
        (*OprScalCtl) |= BAYER_SCAL_BYPASS;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BayerScaler_GetResolution
//  Description : This function get bayer scaler resolution.
//------------------------------------------------------------------------------
/** 
 * @brief This function get bayer scaler resolution.
 * 
 *  This function get bayer scaler resolution.
 * @param[in]     ubInOut  	: stands for bayer scaler input or output range.
 * @param[out]    pusW  	: stands for horizontal resolution.
 * @param[out]    pusH  	: stands for vertical resolution.
 * @return It return the function status.  
 */
MMP_ERR MMPF_BayerScaler_GetResolution(MMP_UBYTE ubInOut, MMP_USHORT *pusW, MMP_USHORT *pusH)
{
    AITPS_BAYER_SCAL pBSCAL = AITC_BASE_BAYERSCAL;
	
	if (ubInOut == 0) {
		*pusW = pBSCAL->BAYER_SCAL_GRAB_IN_H_ED - pBSCAL->BAYER_SCAL_GRAB_IN_H_ST + 1;
		*pusH = pBSCAL->BAYER_SCAL_GRAB_IN_V_ED - pBSCAL->BAYER_SCAL_GRAB_IN_V_ST + 1;
	}
	else {
		*pusW = pBSCAL->BAYER_SCAL_GRAB_OUT_H_ED - pBSCAL->BAYER_SCAL_GRAB_OUT_H_ST + 1;
		*pusH = pBSCAL->BAYER_SCAL_GRAB_OUT_V_ED - pBSCAL->BAYER_SCAL_GRAB_OUT_V_ST + 1;
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BayerScaler_GetZoomInfo
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_BayerScaler_GetZoomInfo(MMP_BAYER_SCALER_MODE 	nBayerMode, 
								     MMP_SCAL_FIT_RANGE 	*fitrange, 
								     MMP_SCAL_GRAB_CTRL 	*grabctl)
{
    if (nBayerMode >= MMP_BAYER_SCAL_MAX_MODE) {
        return MMP_BAYER_ERR_PARAMETER;
    }

    MEMCPY((void*)fitrange, (void*)&m_BayerScalerInfo[nBayerMode].sFitRange, sizeof(MMP_SCAL_FIT_RANGE));
    MEMCPY((void*)grabctl,  (void*)&m_BayerScalerInfo[nBayerMode].sGrabCtl, sizeof(MMP_SCAL_GRAB_CTRL));
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BayerScaler_SetZoomInfo
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_BayerScaler_SetZoomInfo(MMP_BAYER_SCALER_MODE 	nBayerMode, 
									 MMP_SCAL_FIT_MODE 		sFitMode,
				                     MMP_ULONG ulSnrInputW,	MMP_ULONG ulSnrInputH,
				                     MMP_ULONG ulFovInStX,	MMP_ULONG ulFovInStY, 
				                     MMP_ULONG ulFovInputW, MMP_ULONG ulFovInputH, 
				                     MMP_ULONG ulOutputW,   MMP_ULONG ulOutputH, 
				                     MMP_ULONG ulDummyOutX, MMP_ULONG ulDummyOutY)
{
    MMP_SCAL_FIT_RANGE *psFitRange;
    MMP_SCAL_GRAB_CTRL *psGrabCtl;

	/* Parameter Check */
    if (nBayerMode >= MMP_BAYER_SCAL_MAX_MODE) {
        return MMP_BAYER_ERR_PARAMETER;
    }
    
    if (ulSnrInputW < ulOutputW || 
    	ulSnrInputH < ulOutputH ||
    	ulFovInputW < ulOutputW || 
    	ulFovInputH < ulOutputH) {
    	RTNA_DBG_Str(0, "BayerScaler can't scale up!\r\n");
    	return MMP_BAYER_ERR_PARAMETER;
    }
	
    if (((ulFovInStX + ulFovInputW - 1) > ulSnrInputW) ||
    	((ulFovInStY + ulFovInputH - 1) > ulSnrInputH)) {
    	RTNA_DBG_Str(0, "BayerScaler_SetZoomInfo Error1!\r\n");	
    }
    
    psFitRange = &m_BayerScalerInfo[nBayerMode].sFitRange;
    psGrabCtl  = &m_BayerScalerInfo[nBayerMode].sGrabCtl;
    
    psFitRange->fitmode 		= sFitMode;
    psFitRange->scalerType		= MMP_SCAL_TYPE_BAYERSCALER;
    psFitRange->ulInWidth 		= ulSnrInputW;
    psFitRange->ulInHeight 		= ulSnrInputH;
    psFitRange->ulInGrabX 		= ulFovInStX;//((ulSnrInputW - ulFovInputW) / 2) + 1;
    psFitRange->ulInGrabY 		= ulFovInStY;//((ulSnrInputH - ulFovInputH) / 2) + 1;
    psFitRange->ulInGrabW 		= ulFovInputW;
    psFitRange->ulInGrabH 		= ulFovInputH;
    psFitRange->ulOutWidth 		= ulOutputW;
    psFitRange->ulOutHeight		= ulOutputH;

    MMPF_Scaler_GetGCDBestFitScale(psFitRange, psGrabCtl);

	/* Calculate the dummy input pixels by required dummy output pixels. */
    psFitRange->ulDummyInPixelX = (psGrabCtl->ulScaleN == 0) ? ((ulDummyOutX * psGrabCtl->ulScaleXM + (psGrabCtl->ulScaleXN - 1)) / psGrabCtl->ulScaleXN) :
                                                               ((ulDummyOutX * psGrabCtl->ulScaleM + (psGrabCtl->ulScaleN - 1)) / psGrabCtl->ulScaleN);
    psFitRange->ulDummyInPixelY = (psGrabCtl->ulScaleN == 0) ? ((ulDummyOutY * psGrabCtl->ulScaleYM + (psGrabCtl->ulScaleYN - 1)) / psGrabCtl->ulScaleYN) : 
                                                               ((ulDummyOutY * psGrabCtl->ulScaleM + (psGrabCtl->ulScaleN - 1)) / psGrabCtl->ulScaleN);

    psFitRange->ulDummyOutPixelX = ulDummyOutX;
    psFitRange->ulDummyOutPixelY = ulDummyOutY;

    if ((ulSnrInputW < (psFitRange->ulInGrabW + psFitRange->ulDummyInPixelX)) || 
        (ulSnrInputH < (psFitRange->ulInGrabH + psFitRange->ulDummyInPixelY)))
    {
        RTNA_DBG_Str(0, "BayerScaler_SetZoomInfo Error2!\r\n");
        return MMP_BAYER_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}
