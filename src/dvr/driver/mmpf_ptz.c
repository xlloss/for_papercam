//------------------------------------------------------------------------------
//
//  File        : mmpf_ptz.c
//  Description : Firmware PTZ Control Function
//  Author      : Eroy Yang
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_vif.h"
#include "mmp_lib.h"
#include "ptz_cfg.h"
#include "mmpf_sensor.h"
#include "mmpf_vif.h"
#include "mmpf_rawproc.h"
#include "mmpf_scaler.h"
#include "mmpf_bayerscaler.h"
#include "mmpf_ibc.h"
#include "mmpf_ptz.h"
#include "mmpf_mp4venc.h"

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMP_PTZ_SCAL_INFO    m_PtzScalerInfo[MMP_SCAL_PIPE_NUM];
static MMP_PTZ_STEP_INFO	m_PtzStepInfo[MMP_SCAL_PIPE_NUM];

static MMP_BOOL				m_bZoomPathActive[MMP_SCAL_PIPE_NUM] = {MMP_FALSE, MMP_FALSE};
static MMP_BOOL     		m_bUnderDigitalZoom 	= MMP_FALSE;
volatile MMP_BOOL   		m_bZoomOpStart 			= MMP_FALSE;

static MMP_ULONG 			m_ulPtzInputGrabW[MMP_SCAL_PIPE_NUM] = {0};
static MMP_ULONG			m_ulPtzInputGrabH[MMP_SCAL_PIPE_NUM] = {0};

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMP_UBYTE            gbIBCLinkEncId[];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_GetActivePathNum
//  Description :
//------------------------------------------------------------------------------
static MMP_UBYTE MMPF_PTZ_GetActivePathNum(void)
{
	MMP_UBYTE ubIdx = 0;
	MMP_UBYTE ubSum = 0;

    for (ubIdx = MMP_SCAL_PIPE_0; ubIdx < MMP_SCAL_PIPE_NUM; ubIdx++) {

	    if (m_bZoomPathActive[ubIdx] == MMP_TRUE) {
	        ubSum++;
	    }    
    }
    return ubSum;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_GetActivePath
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_PTZ_GetActivePath(MMP_UBYTE *pPipe, MMP_UBYTE ubSearchNum)
{
	MMP_UBYTE ubTempIdx = 0;
	MMP_UBYTE ubIdx = 0;
	
    for (ubIdx = MMP_SCAL_PIPE_0; ubIdx < MMP_SCAL_PIPE_NUM; ubIdx++) {

	    if (m_bZoomPathActive[ubIdx] == MMP_TRUE) {
	        pPipe[ubTempIdx] = ubIdx;
	        ubTempIdx++;
	     
	        if (ubTempIdx == ubSearchNum) 
	        	break;
	        else
	        	continue;
	    }
    }
	return MMP_ERR_NONE;
}

#if 0
void ____PTZ_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_InitPtzRange
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_InitPtzRange(MMP_SCAL_PIPEID pipeID,
                              MMP_USHORT usMaxZoomRatio, 	MMP_USHORT usMaxZoomSteps,  
                              MMP_SHORT  sMaxPanSteps, 		MMP_SHORT  sMinPanSteps,
                              MMP_SHORT  sMaxTiltSteps, 	MMP_SHORT  sMinTiltSteps)
{      
    if (pipeID >= MMP_SCAL_PIPE_NUM) {
        return MMP_SCALER_ERR_PARAMETER;
    }

    m_PtzScalerInfo[pipeID].usMaxZoomRatio	= usMaxZoomRatio;
    m_PtzScalerInfo[pipeID].usMaxZoomSteps	= usMaxZoomSteps;
    m_PtzScalerInfo[pipeID].usCurZoomStep	= 0;

    m_PtzScalerInfo[pipeID].sMaxPanSteps 	= sMaxPanSteps;
    m_PtzScalerInfo[pipeID].sMinPanSteps 	= sMinPanSteps;
    m_PtzScalerInfo[pipeID].sCurPanStep 	= 0;

    m_PtzScalerInfo[pipeID].sMaxTiltSteps	= sMaxTiltSteps;
    m_PtzScalerInfo[pipeID].sMinTiltSteps 	= sMinTiltSteps;
    m_PtzScalerInfo[pipeID].sCurTiltStep 	= 0;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_InitPtzInfo
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_InitPtzInfo(MMP_SCAL_PIPEID 	pipeID,
							 MMP_SCAL_FIT_MODE 	sFitMode,
                       		 MMP_USHORT usInputW,	MMP_USHORT usInputH, 
                         	 MMP_USHORT usOutputW, 	MMP_USHORT usOutputH)
{
    MMP_USHORT gcd = 0;
        
    if (pipeID >= MMP_SCAL_PIPE_NUM) {
        return MMP_SCALER_ERR_PARAMETER;
    }

    m_PtzScalerInfo[pipeID].sFitRange.fitmode 			= sFitMode;
    m_PtzScalerInfo[pipeID].sFitRange.ulInWidth 		= usInputW;
    m_PtzScalerInfo[pipeID].sFitRange.ulInHeight 		= usInputH;
    m_PtzScalerInfo[pipeID].sFitRange.ulInGrabX 		= 1;
    m_PtzScalerInfo[pipeID].sFitRange.ulInGrabY 		= 1;
    m_PtzScalerInfo[pipeID].sFitRange.ulInGrabW			= usInputW;
    m_PtzScalerInfo[pipeID].sFitRange.ulInGrabH 		= usInputH;
    m_PtzScalerInfo[pipeID].sFitRange.ulOutWidth 		= usOutputW;
    m_PtzScalerInfo[pipeID].sFitRange.ulOutHeight 		= usOutputH;

    m_PtzScalerInfo[pipeID].sFitRange.ulDummyInPixelX 	= 0;
    m_PtzScalerInfo[pipeID].sFitRange.ulDummyInPixelY 	= 0;
    m_PtzScalerInfo[pipeID].sFitRange.ulDummyOutPixelX 	= 0;
    m_PtzScalerInfo[pipeID].sFitRange.ulDummyOutPixelY 	= 0;

    gcd = Greatest_Common_Divisor(usInputW, usInputH);
    m_PtzScalerInfo[pipeID].sFitRange.ulInputRatioH 	= usInputW / gcd;
    m_PtzScalerInfo[pipeID].sFitRange.ulInputRatioV 	= usInputH / gcd;

    gcd = Greatest_Common_Divisor(usOutputW, usOutputH);
    m_PtzScalerInfo[pipeID].sFitRange.ulOutputRatioH 	= usOutputW / gcd;
    m_PtzScalerInfo[pipeID].sFitRange.ulOutputRatioV 	= usOutputH / gcd;

    MMPF_Scaler_GetGCDBestFitScale(&m_PtzScalerInfo[pipeID].sFitRange, 
        						   &m_PtzScalerInfo[pipeID].sGrabCtlBase);

    MEMCPY((void *)&m_PtzScalerInfo[pipeID].sGrabCtlCur, 
           (void *)&m_PtzScalerInfo[pipeID].sGrabCtlBase, 
           sizeof(MMP_SCAL_GRAB_CTRL));
    
    m_ulPtzInputGrabW[pipeID] = 0;
    m_ulPtzInputGrabH[pipeID] = 0;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_GetCurPtzInfo
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_GetCurPtzInfo(MMP_SCAL_PIPEID 		pipeID, 
							   MMP_SCAL_FIT_RANGE 	*pFitRange, 
							   MMP_SCAL_GRAB_CTRL 	*pGrabCtl)
{
    if (pipeID >= MMP_SCAL_PIPE_NUM) { 
        return MMP_SCALER_ERR_PARAMETER;
    }
	
	if (pFitRange != NULL) {
    	MEMCPY((void *)pFitRange, (void *)&m_PtzScalerInfo[pipeID].sFitRange, sizeof(MMP_SCAL_FIT_RANGE));
    }
    if (pGrabCtl != NULL) {
    	MEMCPY((void *)pGrabCtl, (void *)&m_PtzScalerInfo[pipeID].sGrabCtlCur, sizeof(MMP_SCAL_GRAB_CTRL));
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_GetCurPtzStep
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_GetCurPtzStep(MMP_SCAL_PIPEID 	pipeID,
							   MMP_SHORT		*psDir,
							   MMP_USHORT		*pusZoomStep,
							   MMP_SHORT		*psPanStep,
							   MMP_SHORT		*psTiltStep)
{
    if (pipeID >= MMP_SCAL_PIPE_NUM) {
        return MMP_SCALER_ERR_PARAMETER;
    }
	
	if (psDir != NULL) {
		*psDir = m_PtzStepInfo[pipeID].sZoomDir;
	}
	if (pusZoomStep != NULL) {
		*pusZoomStep = m_PtzStepInfo[pipeID].usCurZoomStep;
	}	
	if (psPanStep != NULL) {
		*psPanStep = m_PtzStepInfo[pipeID].sCurPanStep;
	}	
	if (psTiltStep != NULL)	{
		*psTiltStep = m_PtzStepInfo[pipeID].sCurTiltStep;
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_SetTargetPtzStep
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_SetTargetPtzStep(MMP_SCAL_PIPEID 	pipeID,
								  MMP_SHORT 		sDir,
								  MMP_USHORT 		usZoomStep,
								  MMP_SHORT 		sPanStep,
								  MMP_SHORT 		sTiltStep)
{
	if (pipeID >= MMP_SCAL_PIPE_NUM) {
		return MMP_SCALER_ERR_PARAMETER;
	}
	
	m_PtzStepInfo[pipeID].sZoomDir		= sDir;
	m_PtzStepInfo[pipeID].usTarZoomStep	= usZoomStep;
	m_PtzStepInfo[pipeID].sTarPanStep	= sPanStep;
	m_PtzStepInfo[pipeID].sTarTiltStep	= sTiltStep;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_CalculatePtzInfo
//  Description : Calculate input and output grab range.
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_CalculatePtzInfo(MMP_SCAL_PIPEID 	pipeID, 
								  MMP_USHORT 		usZoomStep,
                                  MMP_SHORT  		sPanStep,  
                                  MMP_SHORT  		sTiltStep)
{
    MMP_SCAL_FIT_RANGE *fitrange;
    MMP_SCAL_GRAB_CTRL *grabctlBase;
    MMP_SCAL_GRAB_CTRL *grabctlCur;
    
    MMP_ULONG 	ulBaseXM = 0, ulNewXN = 0;
    MMP_ULONG 	ulBaseYM = 0, ulNewYN = 0;
    MMP_ULONG 	ulMaxZoomRatio = 0, ulMaxZoomSteps = 0;
    MMP_LONG 	lMaxPanSteps = 0, lMinPanSteps = 0, lMaxTiltSteps = 0, lMinTiltSteps = 0;
    
    ulMaxZoomRatio	= m_PtzScalerInfo[pipeID].usMaxZoomRatio; 
    ulMaxZoomSteps 	= m_PtzScalerInfo[pipeID].usMaxZoomSteps;

    lMaxPanSteps 	= m_PtzScalerInfo[pipeID].sMaxPanSteps;
    lMinPanSteps 	= m_PtzScalerInfo[pipeID].sMinPanSteps;

    lMaxTiltSteps 	= m_PtzScalerInfo[pipeID].sMaxTiltSteps;
    lMinTiltSteps 	= m_PtzScalerInfo[pipeID].sMinTiltSteps;

    fitrange 		= &m_PtzScalerInfo[pipeID].sFitRange;
    grabctlBase 	= &m_PtzScalerInfo[pipeID].sGrabCtlBase;
    grabctlCur 		= &m_PtzScalerInfo[pipeID].sGrabCtlCur;
    
    /* Parameter Check */
    if ((pipeID >= MMP_SCAL_PIPE_NUM) || 
        (usZoomStep > ulMaxZoomSteps) ||
        (sPanStep > lMaxPanSteps) || (sPanStep < lMinPanSteps) ||
        (sTiltStep > lMaxTiltSteps) || (sTiltStep < lMinTiltSteps))
    {
        printc("Exceed Max/Min Range Path %d\r\n",pipeID);
    	m_bZoomPathActive[pipeID] = MMP_FALSE;
        return MMP_SCALER_ERR_PARAMETER;
    }
    
    /* Calculate new N ratio */
    if (fitrange->fitmode == MMP_SCAL_FITMODE_OPTIMAL) 
    {
	    ulNewXN = (grabctlBase->ulScaleXN * 1000) + 
	              (grabctlBase->ulScaleXN * (ulMaxZoomRatio - 1)
	            * (MMP_ULONG)usZoomStep * 1000 / ulMaxZoomSteps);

	    ulBaseXM = grabctlBase->ulScaleXM * 1000;
    
	    ulNewYN = (grabctlBase->ulScaleYN * 1000) + 
	              (grabctlBase->ulScaleYN * (ulMaxZoomRatio - 1)
	            * (MMP_ULONG)usZoomStep * 1000 / ulMaxZoomSteps);

	    ulBaseYM = grabctlBase->ulScaleYM * 1000;
    }
    else
    {
	    ulNewXN = ulNewYN = 
	    		 (grabctlBase->ulScaleN * 1000) + 
	             (grabctlBase->ulScaleN * (ulMaxZoomRatio - 1)
	           * (MMP_ULONG)usZoomStep * 1000 / ulMaxZoomSteps);

	    ulBaseXM = ulBaseYM = grabctlBase->ulScaleM * 1000;
	}

#if 1
	/* Calculate new input grab width/height by output ratio */
    if ((grabctlBase->ulScaleXN * grabctlBase->ulScaleYM) >= 
        (grabctlBase->ulScaleYN * grabctlBase->ulScaleXM))
    {
        fitrange->ulInGrabW = (usZoomStep == 0) ? fitrange->ulInWidth : ((fitrange->ulOutWidth * ulBaseXM + (ulNewXN - 1)) / ulNewXN);

		// Align output ratio
        fitrange->ulInGrabW = (fitrange->ulInGrabW + fitrange->ulOutputRatioH - 1) / fitrange->ulOutputRatioH; 
        fitrange->ulInGrabW *= fitrange->ulOutputRatioH;

		// Fine tune the input range
        fitrange->ulInGrabH = (fitrange->ulInGrabW * fitrange->ulOutputRatioV / fitrange->ulOutputRatioH);
    }
    else 
    {
        fitrange->ulInGrabH = (usZoomStep == 0) ? fitrange->ulInHeight : ((fitrange->ulOutHeight * ulBaseYM + (ulNewYN - 1)) / ulNewYN);

		// Align output ratio
        fitrange->ulInGrabH = (fitrange->ulInGrabH + fitrange->ulOutputRatioV - 1) / fitrange->ulOutputRatioV; 
        fitrange->ulInGrabH *= fitrange->ulOutputRatioV;

        // Fine tune the input range
        fitrange->ulInGrabW = (fitrange->ulInGrabH * fitrange->ulOutputRatioH / fitrange->ulOutputRatioV);
    }
#else
    fitrange->ulInGrabW = (usZoomStep == 0) ? fitrange->ulInWidth : ((fitrange->ulOutWidth * ulBaseXM + (ulNewXN - 1)) / ulNewXN);
    fitrange->ulInGrabH = (usZoomStep == 0) ? fitrange->ulInHeight : ((fitrange->ulOutHeight * ulBaseYM + (ulNewYN - 1)) / ulNewYN);
#endif

    if (fitrange->ulInGrabW > fitrange->ulInWidth) {
        RTNA_DBG_Str(0, "CalculatePtzInfo Error0 !\r\n");
        fitrange->ulInGrabW = fitrange->ulInWidth;
    }
        
    if (fitrange->ulInGrabH > fitrange->ulInHeight) {
        RTNA_DBG_Str(0, "CalculatePtzInfo Error1 !\r\n");
        fitrange->ulInGrabH = fitrange->ulInHeight;
    }
    
    /* Check the grab range is the same or not */
    if (m_ulPtzInputGrabW[pipeID] == fitrange->ulInGrabW && 
    	m_ulPtzInputGrabH[pipeID] == fitrange->ulInGrabH) 
    {
    	RTNA_DBG_Str(3, "PTZ_SAME_RANGE\r\n");
    	return MMP_SCALER_ERR_PTZ_SAME_RANGE;
    }
    
    m_ulPtzInputGrabW[pipeID] = fitrange->ulInGrabW;
    m_ulPtzInputGrabH[pipeID] = fitrange->ulInGrabH;
    
    /* Calculate new input grab position */
    fitrange->ulInGrabX = ((MMP_LONG)(fitrange->ulInWidth >> 1) + 
				        ((MMP_LONG)(fitrange->ulInWidth - fitrange->ulInGrabW) * (MMP_LONG)sPanStep / (lMaxPanSteps - lMinPanSteps)) - 
				        (MMP_LONG)(fitrange->ulInGrabW >> 1));
    
    fitrange->ulInGrabY = ((MMP_LONG)(fitrange->ulInHeight >> 1) + 
				        ((MMP_LONG)(fitrange->ulInHeight - fitrange->ulInGrabH) * (MMP_LONG)sTiltStep / (lMaxTiltSteps - lMinTiltSteps)) - 
				        (MMP_LONG)(fitrange->ulInGrabH >> 1));   
    
    fitrange->ulInGrabX = (fitrange->ulInGrabX == 0) ? 1 : fitrange->ulInGrabX;
    fitrange->ulInGrabY = (fitrange->ulInGrabY == 0) ? 1 : fitrange->ulInGrabY;

	/* Get new output grab range */
    MMPF_Scaler_GetGCDBestFitScale(fitrange, grabctlCur);

    /* Update PTZ step information */
    m_PtzScalerInfo[pipeID].usCurZoomStep 	= m_PtzStepInfo[pipeID].usCurZoomStep 	= usZoomStep;
    m_PtzScalerInfo[pipeID].sCurPanStep  	= m_PtzStepInfo[pipeID].sCurPanStep 	= sPanStep;
    m_PtzScalerInfo[pipeID].sCurTiltStep  	= m_PtzStepInfo[pipeID].sCurTiltStep 	= sTiltStep;
	
	/* Check the step is exceed the target or not */
    if ((m_PtzStepInfo[pipeID].sZoomDir == MMP_PTZ_ZOOM_INC_IN) && 
    	(usZoomStep >= m_PtzStepInfo[pipeID].usTarZoomStep))
    {
    	RTNA_DBG_Str(0, "Zoom-In stop\r\n");
    	m_bZoomPathActive[pipeID] = MMP_FALSE;
    }
    else if ((m_PtzStepInfo[pipeID].sZoomDir == MMP_PTZ_ZOOM_INC_OUT) && 
    		 (usZoomStep <= m_PtzStepInfo[pipeID].usTarZoomStep))
    {
    	RTNA_DBG_Str(0, "Zoom-Out stop\r\n");
    	m_bZoomPathActive[pipeID] = MMP_FALSE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_ReCalculateGrabRange
//  Description : For PTZ operation
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_ReCalculateGrabRange(MMP_SCAL_FIT_RANGE	*pFitrange, 
                            		  MMP_SCAL_GRAB_CTRL	*pGrabctl)
{
	pFitrange->ulInWidth  	= pFitrange->ulInGrabW + pFitrange->ulDummyInPixelX;
	pFitrange->ulInHeight 	= pFitrange->ulInGrabH + pFitrange->ulDummyInPixelY;

	pFitrange->bUseLpfGrab	= MMP_TRUE;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_SetDigitalZoom
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_SetDigitalZoom(MMP_SCAL_PIPEID 	pipeID, 
								MMP_PTZ_ZOOM_DIR	zoomdir, 
                                MMP_BOOL   			bStartOP)
{
    if ((zoomdir == MMP_PTZ_ZOOMIN) || (zoomdir == MMP_PTZ_ZOOMOUT)) 
    {    
        if (bStartOP == MMP_TRUE) 
        {
        	m_bZoomOpStart 				= MMP_TRUE;
    		m_bUnderDigitalZoom 		= MMP_TRUE;
			m_bZoomPathActive[pipeID] 	= MMP_TRUE;
        }
    }
    else
    {
        m_bZoomPathActive[pipeID] = MMP_FALSE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_SetDigitalZoomOP
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_SetDigitalZoomOP(MMP_UBYTE         ubSnrSel,
                                  MMP_SCAL_PIPEID	pipeID,
                                  MMP_PTZ_ZOOM_DIR	zoomdir, 
                                  MMP_BOOL    		bStartOP)
{
    MMP_ERR status = MMP_ERR_NONE;

    if ((zoomdir == MMP_PTZ_ZOOMIN) || (zoomdir == MMP_PTZ_ZOOMOUT)) 
    {
        if (bStartOP == MMP_TRUE) 
        {
			m_bZoomOpStart 				= MMP_TRUE;
        	m_bUnderDigitalZoom 		= MMP_TRUE;
        	m_bZoomPathActive[pipeID] 	= MMP_TRUE;
        
            while (m_bZoomOpStart) {
            
                status = MMPF_PTZ_ExecutePTZ(ubSnrSel);
                if (status != MMP_ERR_NONE) {
                    break;
                }
            }
    	}
	}
    else
    {
        m_bZoomPathActive[pipeID] = MMP_FALSE;
    }
	
    return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_ExecutePTZ
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_ExecutePTZ(MMP_UBYTE ubSnrSel)
{
    MMP_ERR					err = MMP_ERR_NONE;
	AITPS_VIF               pVIF = AITC_BASE_VIF;
    MMP_USHORT              opr_upd_ctl;
    MMP_SCAL_FIT_RANGE   	sFitRange;
    MMP_SCAL_GRAB_CTRL 		sGrabctl;
    MMP_SCAL_FIT_RANGE   	sSubFitRange;
    MMP_SCAL_GRAB_CTRL 		sSubGrabctl;
    MMP_SCAL_PIPEID        	mainpipe, subpipe;
    MMP_UBYTE               ubVifId = MMPF_Sensor_GetVIFPad(ubSnrSel);
	MMP_UBYTE 				ubActivePipe[MMP_SCAL_PIPE_NUM];

	if (!MMP_IsVidPtzEnable() && !MMP_IsDscPtzEnable()) {
		return MMP_ERR_NONE;
	}
	
    if (m_bZoomOpStart == MMP_FALSE) {
        return MMP_ERR_NONE;
    }
	
    /* No Zoom */
    if (MMPF_PTZ_GetActivePathNum() == 0)
    {
        m_bZoomOpStart 		= MMP_FALSE;
        m_bUnderDigitalZoom = MMP_FALSE;

        return MMP_ERR_NONE;
    }

	MMPF_PTZ_GetActivePath(ubActivePipe, 2);

    mainpipe = ubActivePipe[0];
	subpipe  = ubActivePipe[1];

    /* Calculate Main pipe parameter */
    if (MMPF_PTZ_GetActivePathNum() >= 1)
    {
    	/* Check the step is exceed the target or not */
        if ((m_PtzStepInfo[mainpipe].sZoomDir == MMP_PTZ_ZOOM_INC_IN) && 
        	(m_PtzStepInfo[mainpipe].usCurZoomStep >= m_PtzStepInfo[mainpipe].usTarZoomStep))
        {
        	RTNA_DBG_Str(0, "Zoom-In stop\r\n");
        	m_bZoomPathActive[mainpipe] = MMP_FALSE;
    		goto L_SubPipe;
        }
        else if ((m_PtzStepInfo[mainpipe].sZoomDir == MMP_PTZ_ZOOM_INC_OUT) && 
        		 (m_PtzStepInfo[mainpipe].usCurZoomStep <= m_PtzStepInfo[mainpipe].usTarZoomStep))
        {
        	RTNA_DBG_Str(0, "Zoom-Out stop\r\n");
        	m_bZoomPathActive[mainpipe] = MMP_FALSE;
    		goto L_SubPipe;
        }

        do {
            if (m_PtzStepInfo[mainpipe].usCurZoomStep != m_PtzStepInfo[mainpipe].usTarZoomStep) {
                m_PtzStepInfo[mainpipe].usCurZoomStep += m_PtzStepInfo[mainpipe].sZoomDir;

                err = MMPF_PTZ_CalculatePtzInfo(mainpipe, 
                                                m_PtzStepInfo[mainpipe].usCurZoomStep, 
                                                m_PtzStepInfo[mainpipe].sCurPanStep,
                                                m_PtzStepInfo[mainpipe].sCurTiltStep);
            }
            else {
                // Stop zoom
                err = MMP_ERR_NONE;
                m_bZoomPathActive[mainpipe] = MMP_FALSE;
            }
        } while ((err == MMP_SCALER_ERR_PTZ_SAME_RANGE) && (m_bZoomPathActive[mainpipe] == MMP_TRUE));

        MMPF_PTZ_GetCurPtzInfo(mainpipe, &sFitRange, &sGrabctl);

		MMPF_PTZ_ReCalculateGrabRange(&sFitRange, &sGrabctl);

        /* Set Main Pipe, Prevent VIF frame_st and prevent OPR frame sync */
        opr_upd_ctl = pVIF->VIF_OPR_UPD[ubVifId] & (VIF_OPR_UPD_EN | VIF_OPR_UPD_FRAME_SYNC);
        pVIF->VIF_OPR_UPD[ubVifId] &= ~opr_upd_ctl;

        MMPF_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, mainpipe, &sFitRange, &sGrabctl);
		MMPF_Scaler_SetLPF(mainpipe, &sFitRange, &sGrabctl);

        pVIF->VIF_OPR_UPD[ubVifId] |= opr_upd_ctl;
        
        if (sFitRange.ulInGrabW < sFitRange.ulOutWidth || 
        	sFitRange.ulInGrabH < sFitRange.ulOutHeight) {

        	MMPF_Scaler_SetPixelDelay(mainpipe, 9, 10); //TBD

		    if ( (int)mainpipe == (int)MMP_IBC_PIPE_0) { // For Video Record
		    	switch (MMPF_MP4VENC_GetStatus(gbIBCLinkEncId[mainpipe])) {
    	        case MMPF_MP4VENC_FW_STATUS_START:
    	        case MMPF_MP4VENC_FW_STATUS_RESUME:
    	        case MMPF_MP4VENC_FW_STATUS_PREENCODE:
    	            MMPF_Scaler_SetPixelDelay(mainpipe, 11, 20);
    	            break;
    	        }
		    }
		}
    }

L_SubPipe:

	/* Calculate Sub pipe parameter */
    if (MMPF_PTZ_GetActivePathNum() >= 2)
    {
        /* Check the step is exceed the target or not */
        if ((m_PtzStepInfo[subpipe].sZoomDir == MMP_PTZ_ZOOM_INC_IN) && 
        	(m_PtzStepInfo[subpipe].usCurZoomStep >= m_PtzStepInfo[subpipe].usTarZoomStep))
        {
        	RTNA_DBG_Str(0, "Zoom-In stop\r\n");
        	m_bZoomPathActive[subpipe] = MMP_FALSE;
    		return MMP_ERR_NONE;
        }
        else if ((m_PtzStepInfo[subpipe].sZoomDir == MMP_PTZ_ZOOM_INC_OUT) && 
        		 (m_PtzStepInfo[subpipe].usCurZoomStep <= m_PtzStepInfo[subpipe].usTarZoomStep))
        {
        	RTNA_DBG_Str(0, "Zoom-Out stop\r\n");
        	m_bZoomPathActive[subpipe] = MMP_FALSE;
    		return MMP_ERR_NONE;
        }

        do {
            if (m_PtzStepInfo[subpipe].usCurZoomStep != m_PtzStepInfo[subpipe].usTarZoomStep) {
                m_PtzStepInfo[subpipe].usCurZoomStep += m_PtzStepInfo[subpipe].sZoomDir;

                err = MMPF_PTZ_CalculatePtzInfo(subpipe, 
                                                m_PtzStepInfo[subpipe].usCurZoomStep, 
                                                m_PtzStepInfo[subpipe].sCurPanStep,
                                                m_PtzStepInfo[subpipe].sCurTiltStep);
            }
            else {
                // Stop zoom
                err = MMP_ERR_NONE;
                m_bZoomPathActive[subpipe] = MMP_FALSE;
            }
        } while ((err == MMP_SCALER_ERR_PTZ_SAME_RANGE) && (m_bZoomPathActive[subpipe] == MMP_TRUE));

        MMPF_PTZ_GetCurPtzInfo(subpipe, &sSubFitRange, &sSubGrabctl);
		
		MMPF_PTZ_ReCalculateGrabRange(&sSubFitRange, &sSubGrabctl);

		/* Set Sub Pipe, Prevent VIF frame_st and prevent OPR frame sync */
        opr_upd_ctl = pVIF->VIF_OPR_UPD[ubVifId] & (VIF_OPR_UPD_EN | VIF_OPR_UPD_FRAME_SYNC);
        pVIF->VIF_OPR_UPD[ubVifId] &= ~opr_upd_ctl;

        MMPF_Scaler_SetEngine(MMP_SCAL_USER_DEF_TYPE_IN_OUT, subpipe, &sSubFitRange, &sSubGrabctl);
        MMPF_Scaler_SetLPF(subpipe, &sSubFitRange, &sSubGrabctl);

        pVIF->VIF_OPR_UPD[ubVifId] |= opr_upd_ctl;

		if (sSubFitRange.ulInGrabW < sSubFitRange.ulOutWidth || 
			sSubFitRange.ulInGrabH < sSubFitRange.ulOutHeight) {

        	MMPF_Scaler_SetPixelDelay(subpipe, 9, 10);

		    if ( (int)subpipe == (int)MMP_IBC_PIPE_0) { // For Video Record
		    	switch (MMPF_MP4VENC_GetStatus(gbIBCLinkEncId[subpipe])) {
    	        case MMPF_MP4VENC_FW_STATUS_START:
    	        case MMPF_MP4VENC_FW_STATUS_RESUME:
    	        case MMPF_MP4VENC_FW_STATUS_PREENCODE:
    	            MMPF_Scaler_SetPixelDelay(subpipe, 11, 20);
    	            break;
    	        }
		    }
		}
    }

	#if 0
	printc("Sub sCurZoomStep %d InW %d InH %d GX %d GY %d GW %d GH %d\r\n",
			m_PtzStepInfo[subpipe].usCurZoomStep,
			sSubFitRange.ulInWidth, sSubFitRange.ulInHeight,
			sSubFitRange.ulInGrabX, sSubFitRange.ulInGrabY, 
			sSubFitRange.ulInGrabW, sSubFitRange.ulInGrabH);

	printc("Main sCurZoomStep %d InW %d InH %d GX %d GY %d GW %d GH %d\r\n",
			m_PtzStepInfo[mainpipe].usCurZoomStep,
			sFitRange.ulInWidth, sFitRange.ulInHeight,
			sFitRange.ulInGrabX, sFitRange.ulInGrabY, 
			sFitRange.ulInGrabW, sFitRange.ulInGrabH);
	#endif

    /* Set raw fetch delay according to main pipe N, M */
    MMPF_RAWPROC_CalcRawFetchTiming(mainpipe);
	MMPF_RAWPROC_EnableFetchBusyMode(MMP_TRUE);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PTZ_GetZoomStatus
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PTZ_GetZoomStatus(MMP_SCAL_PIPEID pipeID, MMP_BOOL *pbZoomStatus)
{
    *pbZoomStatus = !m_bZoomPathActive[pipeID];
    return MMP_ERR_NONE;
}
