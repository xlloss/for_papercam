//==============================================================================
//
//  File        : mmpf_graphics.c
//  Description : Firmware Graphic Control Function
//  Author      : Ben Lu
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
#include "mmp_reg_ibc.h"
#include "mmp_reg_graphics.h"
#include "mmpf_graphics.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define GRAPHIC_DBG_LEVEL (0)

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

MMPF_OS_SEMID m_GraphicSemID;
static GraphicScaleCallBackFunc *GraNormalCBFunc = NULL;
static GraphicScaleCallBackFunc *GraSelfTriggerCBFunc = NULL;

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_Initialize
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_Initialize(void)
{
	AITPS_AIC pAIC = AITC_BASE_AIC;
	
    RTNA_AIC_Open(pAIC, AIC_SRC_GRA, gra_isr_a, AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_GRA);
	
	m_GraphicSemID = MMPF_OS_CreateSem(1);

    if (m_GraphicSemID >= MMPF_OS_CREATE_SEM_EXCEED_MAX) {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "m_GraphicSemID Failed");
        return MMP_SYSTEM_ERR_HW;
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_ISR
//  Description : 
//------------------------------------------------------------------------------
void MMPF_Graphics_ISR(void)
{
    AITPS_GRA   pGRA = AITC_BASE_GRA;
    MMP_USHORT  intsrc;
    
    intsrc = pGRA->GRA_SCAL_INT_CPU_SR & pGRA->GRA_SCAL_INT_CPU_EN;
    pGRA->GRA_SCAL_INT_CPU_SR = intsrc;

	if (intsrc & GRA_YUV420_SCAL_DONE) 
	{
	    pGRA->GRA_SCAL_INT_CPU_EN &= ~(GRA_YUV420_SCAL_DONE);

	    if (GraNormalCBFunc) {
	    	(*GraNormalCBFunc)();
	    }
	    if (MMPF_OS_ReleaseSem(m_GraphicSemID) != OS_NO_ERR) {
		    return;
	    }
	    /* For LDC self trigger case */
	    if (GraSelfTriggerCBFunc) {
	    	(*GraSelfTriggerCBFunc)();
	    }
	}

	if (intsrc & GRA_SCAL_DONE) 
	{
		pGRA->GRA_SCAL_INT_CPU_EN &= ~(GRA_SCAL_DONE);
	
	    if (GraNormalCBFunc) {
	    	(*GraNormalCBFunc)();
	    }
	    if (MMPF_OS_ReleaseSem(m_GraphicSemID) != OS_NO_ERR) {
		    return;
	    }
	    /* For LDC self trigger case */
	    if (GraSelfTriggerCBFunc) {
	    	(*GraSelfTriggerCBFunc)();
	    }
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetDelayType
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_SetDelayType(MMP_GRAPHICS_DELAY_TYPE ubType)
{
    AITPS_GRA pGRA = AITC_BASE_GRA;
    
    pGRA->GRA_SCAL_FLOW_CTL &= ~(GRA_SCAL_DLY_CTL_MASK);
    
    if (ubType == MMP_GRAPHICS_DELAY_CHK_SCA_BUSY)
        pGRA->GRA_SCAL_FLOW_CTL |= GRA_SCAL_DLY_CHK_SCA_BUSY;
    else if (ubType == MMP_GRAPHICS_DELAY_CHK_LINE_END)
        pGRA->GRA_SCAL_FLOW_CTL |= GRA_SCAL_DLY_CHK_LN_END;
   
    return MMP_ERR_NONE;
}   

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetPixDelay
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_SetPixDelay(MMP_UBYTE ubPixDelayN, MMP_UBYTE ubPixDelayM)
{
    AITPS_GRA pGRA = AITC_BASE_GRA;

	/* Actual pixel delay = Output ubPixDelayN pixel every ubPixDelayM tick */
	pGRA->GRA_SCAL_PIXL_DLY_N = GRA_SCAL_SET_PIX_DELAY(ubPixDelayN);
	pGRA->GRA_SCAL_PIXL_DLY   = GRA_SCAL_SET_PIX_DELAY(ubPixDelayM);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetLineDelay
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_SetLineDelay(MMP_USHORT usLineDelay)
{
    AITPS_GRA pGRA = AITC_BASE_GRA;

    pGRA->GRA_SCAL_LINE_DLY = usLineDelay;
   
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetScaleAttr
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_SetScaleAttr(MMP_GRAPHICS_BUF_ATTR  	*bufAttr,
								   MMP_GRAPHICS_RECT   		*rect, 
								   MMP_USHORT          		usUpScale)
{
	AITPS_GRA   pGRA = AITC_BASE_GRA;
	MMP_ULONG	longtmp;
    MMP_USHORT  usCount = 0;
    MMP_UBYTE   ubRet 	= 0;

    ubRet = MMPF_OS_AcquireSem(m_GraphicSemID, GRAPHICS_SEM_TIMEOUT);
    
    if (ubRet == OS_ERR_PEND_ISR) {
        MMPF_OS_AcceptSem(m_GraphicSemID, &usCount);
        if (usCount == 0) {
            MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "m_GraphicSemID OSSemAccept:Fail1");
            return MMP_GRA_ERR_BUSY;
        }
    }
    else if (ubRet != OS_NO_ERR) {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "m_GraphicSemID AcquireSem:Fail1");
        return MMP_GRA_ERR_BUSY;
	}
	
	#if 0//EROY CHECK : YUV420 need grab full range?
  	if ( (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
  	     (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) )
  	{
	  	if ((rect->usLeft != 0) || bufAttr->usLineOffset != rect->usWidth) {
			MMPF_OS_ReleaseSem(m_GraphicSemID);
			return MMP_GRA_ERR_PARAMETER;
	    }
	}
	#endif

    if ((bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
        (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE))
   	{
	    longtmp = (bufAttr->ulBaseAddr)
    			+ (bufAttr->usLineOffset * rect->usTop) + rect->usLeft;   
        pGRA->GRA_SCAL_ADDR_Y_ST = longtmp;

	    if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) {
	   		longtmp = (bufAttr->ulBaseUAddr)
	    			+ (bufAttr->usLineOffset * (rect->usTop >> 1)) + ((rect->usLeft >> 1) << 1);
	        pGRA->GRA_SCAL_ADDR_U_ST = longtmp;
	        pGRA->GRA_SCAL_ADDR_V_ST = longtmp;
	    }
	    else {

       		longtmp = (bufAttr->ulBaseUAddr)
        			+ ((bufAttr->usLineOffset >> 1) * (rect->usTop >> 1)) + (rect->usLeft >> 1);
	        pGRA->GRA_SCAL_ADDR_U_ST = longtmp;

	       	longtmp = (bufAttr->ulBaseVAddr)
    	    		+ ((bufAttr->usLineOffset >> 1) * (rect->usTop >> 1)) + (rect->usLeft >> 1);
        	pGRA->GRA_SCAL_ADDR_V_ST = longtmp;
   		}
		
		/* For YUV420 case, Y_OFST/U_OFST/V_OFST is the difference of usLineOffset and scaling width 
		   Ref: UserGuide P.18 */
        pGRA->GRA_SCAL_ADDR_Y_OFST = bufAttr->usLineOffset - rect->usWidth;
        
        if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) {
   	        pGRA->GRA_SCAL_ADDR_U_OFST = bufAttr->usLineOffset - rect->usWidth;
        }
        else {
            pGRA->GRA_SCAL_ADDR_U_OFST = ((bufAttr->usLineOffset - rect->usWidth) >> 1);
            pGRA->GRA_SCAL_ADDR_V_OFST = pGRA->GRA_SCAL_ADDR_U_OFST;
        }
   	}
    else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY ||
             bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY ||
             bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV ||
             bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU) 
    {
	    longtmp = (bufAttr->ulBaseAddr)
    	    	+ ((bufAttr->usLineOffset) * (rect->usTop)) 
	    	    + (rect->usLeft * 2);

    	pGRA->GRA_SCAL_ADDR_Y_ST = longtmp;
    	/* For Non-YUV420 case, GRA_SCAL_ADDR_U_ST is the LineOffset, Ref: UserGuide P.16 */
        pGRA->GRA_SCAL_ADDR_U_ST = (bufAttr->usLineOffset);
   	}
    else 
    {
        longtmp = (bufAttr->ulBaseAddr)
		    	+ ((bufAttr->usLineOffset) * (rect->usTop)) 
    			+ ((rect->usLeft) * (MMP_USHORT)(bufAttr->colordepth));
    			
    	pGRA->GRA_SCAL_ADDR_Y_ST = longtmp;
    	/* For Non-YUV420 case, GRA_SCAL_ADDR_U_ST is the LineOffset, Ref: UserGuide P.16 */
        pGRA->GRA_SCAL_ADDR_U_ST = (bufAttr->usLineOffset);
    }       

    pGRA->GRA_SCAL_W = (rect->usWidth);
    pGRA->GRA_SCAL_H = (rect->usHeight);
	pGRA->GRA_SCAL_UP_FACT = GRA_SCALUP(usUpScale);

	if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_16) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_RGB565;
	}		
	else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_24) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_RGB888;
	}		
	else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_YUV422 | GRA_SCAL_SRC_YUV422_UYVY;
	}
	else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_YUV422 | GRA_SCAL_SRC_YUV422_VYUY;
	}
	else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_YUV422 | GRA_SCAL_SRC_YUV422_YUYV;
	}
	else if (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU) {
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_YUV422 | GRA_SCAL_SRC_YUV422_YVYU;
	}
	else if ((bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
	         (bufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE))
	{
		pGRA->GRA_SCAL_FMT = GRA_SCAL_MEM_RGB565;
	}

	MMPF_OS_ReleaseSem(m_GraphicSemID);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetMCIByteCnt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_SetMCIByteCnt(MMP_USHORT usByteCnt)
{
	AITPS_GRA   pGRA = AITC_BASE_GRA;
	
	if (usByteCnt == 128)
		pGRA->GRA_SCAL_UP_FACT |= GRA_SCAL_MCI_128_BYTE;
	else
		pGRA->GRA_SCAL_UP_FACT &= ~(GRA_SCAL_MCI_128_BYTE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_IsScaleIdle
//  Description :
//------------------------------------------------------------------------------
MMP_BOOL MMPF_Graphics_IsScaleIdle(void)
{
    MMP_USHORT usSemCount = 0;

    if (MMPF_OS_QuerySem(m_GraphicSemID, &usSemCount) == 0) {
        if (usSemCount) {
            return MMP_TRUE;
    	}
    }

    return MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_ScaleStart
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_ScaleStart(MMP_GRAPHICS_BUF_ATTR		*src,
                                 MMP_GRAPHICS_BUF_ATTR		*dst,
                                 GraphicScaleCallBackFunc 	*GraCallBack,
                                 GraphicScaleCallBackFunc 	*GraSelfTriggerCB)
{
    AITPS_GRA   pGRA = AITC_BASE_GRA;
    AITPS_IBC   pIBC = AITC_BASE_IBC;
    MMP_USHORT  usCount = 0;
    MMP_UBYTE   ubRet = 0;
    AITPS_IBCP  pIbcPipeCtl = &(pIBC->IBCP_0);
    MMP_UBYTE   bIBCPipeNum = 0; //TBD

    ubRet = MMPF_OS_AcquireSem(m_GraphicSemID, GRAPHICS_SEM_TIMEOUT);

    if (ubRet == OS_ERR_PEND_ISR) {
        MMPF_OS_AcceptSem(m_GraphicSemID, &usCount);
        if(usCount == 0){
            MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "m_GraphicSemID OSSemAccept:Fail2");
            return MMP_GRA_ERR_BUSY;
        }
    }
    else if (ubRet != OS_NO_ERR) {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "m_GraphicSemID OSSemPend:Fail2");
        return MMP_GRA_ERR_BUSY;
    }

	pGRA->GRA_SCAL_FLOW_CTL = GRA_SCAL_FRM_BASE_EN | GRA_SCAL_DLY_CHK_SCA_BUSY;

    if (bIBCPipeNum == 0) {
        pIbcPipeCtl = &(pIBC->IBCP_0);
    }
    else if (bIBCPipeNum == 1) {
        pIbcPipeCtl = &(pIBC->IBCP_1);
    }
    else if (bIBCPipeNum == 2) {
        pIbcPipeCtl = &(pIBC->IBCP_2);
    }
    else if (bIBCPipeNum == 3) {
        pIbcPipeCtl = &(pIBC->IBCP_3);
    }

    if ((src->colordepth == MMP_GRAPHICS_COLORDEPTH_8)  ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_16) ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_24) ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY) ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY) ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV) ||
        (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU)) 
    {
    	if (pGRA->GRA_SCAL_EN & GRA_SCAL_ST) {
            MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "GRA SCAL BUSY");
    		return MMP_ERR_NONE;
        }
        
        if (src->ulBaseAddr) {
        	pGRA->GRA_SCAL_ADDR_Y_ST = src->ulBaseAddr;
		}

        if (dst->ulBaseAddr) {
            pIbcPipeCtl->IBC_ADDR_Y_ST = dst->ulBaseAddr;
        }

        GraNormalCBFunc        		= GraCallBack;
        GraSelfTriggerCBFunc		= GraSelfTriggerCB;
        
        pGRA->GRA_SCAL_INT_CPU_SR   = GRA_SCAL_DONE;
        pGRA->GRA_SCAL_INT_CPU_EN   = GRA_SCAL_DONE;

        pGRA->GRA_SCAL_EN           = GRA_SCAL_ST;
    }
    else if ((src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
             (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE))
    {
        if (pGRA->GRA_SCAL_EN & GRA_YUV420_SCAL_ST) {
    	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "GRA SCAL420 BUSY");
    	    return MMP_ERR_NONE;
        }
        
        if (src->ulBaseAddr) {
        	pGRA->GRA_SCAL_ADDR_Y_ST = src->ulBaseAddr;
        	pGRA->GRA_SCAL_ADDR_U_ST = src->ulBaseUAddr;
        	pGRA->GRA_SCAL_ADDR_V_ST = src->ulBaseVAddr;
        }
        
        if (dst->ulBaseAddr) {
            pIbcPipeCtl->IBC_ADDR_Y_ST = dst->ulBaseAddr;
            pIbcPipeCtl->IBC_ADDR_U_ST = dst->ulBaseUAddr;
            pIbcPipeCtl->IBC_ADDR_V_ST = dst->ulBaseVAddr;
        }

        GraNormalCBFunc       		= GraCallBack;
        GraSelfTriggerCBFunc		= GraSelfTriggerCB;

        pGRA->GRA_SCAL_INT_CPU_SR   = GRA_YUV420_SCAL_DONE;
        pGRA->GRA_SCAL_INT_CPU_EN   = GRA_YUV420_SCAL_DONE;
        
        if (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420){
            pGRA->GRA_SCAL_EN = GRA_YUV420_SCAL_ST;
        }
        else if (src->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE){
            pGRA->GRA_SCAL_EN = GRA_YUV420_SCAL_ST | GRA_YUV420_INTERLEAVE;
    	}
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_TriggerAndWaitDone
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_TriggerAndWaitDone(MMP_GRAPHICS_COLORDEPTH colordepth)
{
	AITPS_GRA pGRA = AITC_BASE_GRA;
	MMP_ULONG ulTimeOutTick = GRAPHICS_WHILE_TIMEOUT;

	if (colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) {
	    pGRA->GRA_SCAL_EN |= GRA_YUV420_SCAL_ST;
		while ((pGRA->GRA_SCAL_EN & GRA_YUV420_SCAL_ST) && (--ulTimeOutTick > 0));
	}
	else if(colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) {
	    pGRA->GRA_SCAL_EN |= (GRA_YUV420_SCAL_ST | GRA_YUV420_INTERLEAVE);
	    while ((pGRA->GRA_SCAL_EN & GRA_YUV420_SCAL_ST) && (--ulTimeOutTick > 0));
	}
	else {
	    pGRA->GRA_SCAL_EN |= GRA_SCAL_ST;
		while ((pGRA->GRA_SCAL_EN & GRA_SCAL_ST) && (--ulTimeOutTick > 0));
	}
	
	if (ulTimeOutTick == 0) {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
	    return MMP_GRA_ERR_TIMEOUT;
	}

	return MMP_ERR_NONE;
}

#if 0
void ____BLT_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_CopyImageBuftoBuf
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_CopyImageBuftoBuf(MMP_GRAPHICS_BUF_ATTR 	*srcBufAttr, 
                        				MMP_GRAPHICS_RECT 		*srcrect, 
                        				MMP_GRAPHICS_BUF_ATTR	*dstBufAttr, 
										MMP_USHORT              usDstStartx, 
										MMP_USHORT              usDstStarty, 
										MMP_GRAPHICS_ROP        ropcode, 
										MMP_UBYTE 				ubTranspActive, 
										MMP_ULONG 				ulKeyColor)
{
	AITPS_GRA   pGRA = AITC_BASE_GRA;
	MMP_ULONG   longtmp;
    MMP_USHORT  cut_width, cut_height;
    MMP_USHORT  raster_dir;
    MMP_UBYTE  	ubColorDepth    = 2;
    MMP_ULONG   ulTimeOutTick   = GRAPHICS_WHILE_TIMEOUT;
    
	MMPF_OS_AcquireSem(m_GraphicSemID, GRAPHICS_SEM_TIMEOUT);

    /* Error Parameter Check */
	if (srcrect->usLeft >= srcBufAttr->usWidth || 
		srcrect->usTop  >= srcBufAttr->usHeight||
		usDstStartx >= dstBufAttr->usWidth 	   || 
		usDstStarty >= dstBufAttr->usHeight    ||
		!srcrect->usWidth 					   || 
		!srcrect->usHeight) 
	{
		MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
    }

	if (srcBufAttr->colordepth != dstBufAttr->colordepth) {
		MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
	}			

    if( (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
        (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) ||
        (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_32))
    {
        /* I420/NV12 use MMP_GRAPHICS_COLORDEPTH_8 to do BLT operation in multiple-times */
    	MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
	}

	pGRA->GRA_BLT_BG = ulKeyColor;

    if (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU) {
        ubColorDepth = 2;
	}	
    else {
        ubColorDepth = dstBufAttr->colordepth;
    }     
    
    /* Adjust Cut Width/Height */
	if ((srcrect->usWidth + srcrect->usLeft) > srcBufAttr->usWidth)
		cut_width = srcBufAttr->usWidth - srcrect->usLeft;
	else	
		cut_width = srcrect->usWidth;

	if ((srcrect->usHeight + srcrect->usTop) > srcBufAttr->usHeight)
		cut_height = srcBufAttr->usHeight - srcrect->usTop;
	else	
		cut_height = srcrect->usHeight;

	if ((cut_width + usDstStartx) > dstBufAttr->usWidth)
		cut_width = dstBufAttr->usWidth - usDstStartx;

	if ((cut_height + usDstStarty) > dstBufAttr->usHeight)
		cut_height = dstBufAttr->usHeight - usDstStarty;

	/* Set BitBLT direction */
	raster_dir = GRA_LEFT_TOP_RIGHT;
	
	if (srcBufAttr->ulBaseAddr == dstBufAttr->ulBaseAddr) 
	{
		if (srcrect->usLeft == usDstStartx) 
		{
			if (srcrect->usTop > usDstStarty) {
				raster_dir = GRA_LEFT_TOP_RIGHT;
			}
			else {
				raster_dir = GRA_LEFT_BOT_RIGHT;
			}	
		}	
		else if (srcrect->usTop == usDstStarty) 
		{
			if (srcrect->usLeft > usDstStartx) {
				raster_dir = GRA_LEFT_TOP_RIGHT;
			}	
			else {		
				raster_dir = GRA_RIGHT_BOT_LEFT;
			}
		}
		// Detect Overlap	
		else if (srcrect->usLeft > usDstStartx) 
		{
			if ((usDstStartx + srcrect->usWidth) > srcrect->usLeft) {
				//(S:down-right d:upper-left)	
				if (srcrect->usTop > usDstStarty) {
					raster_dir = GRA_LEFT_TOP_RIGHT;
				}
				//(S:upper-right d:down-left)
				else if (srcrect->usTop < usDstStarty) {
					raster_dir = GRA_LEFT_BOT_RIGHT;
				}		
			}	
		}
		else if (srcrect->usLeft < usDstStartx) 
		{
			if ((srcrect->usLeft + srcrect->usWidth) > usDstStartx) {
				//(S:down-left d:upper-right)
				if (srcrect->usTop > usDstStarty) {
					raster_dir = GRA_LEFT_TOP_RIGHT;
				}
				//(S:upper-left d:down-right)
				else if (srcrect->usTop < usDstStarty) {
					raster_dir = GRA_LEFT_BOT_RIGHT;
				}
			}	
		}	
	}
	
	/* Set BitBLT operation mode and address */ 
	switch (raster_dir) 
	{
		case GRA_LEFT_TOP_RIGHT:
			pGRA->GRA_BLT_ROP_CTL = GRA_LEFT_TOP_RIGHT;

			longtmp = (srcBufAttr->ulBaseAddr)
					+ ((srcBufAttr->usLineOffset) * (srcrect->usTop)) 
					+ ((srcrect->usLeft) * ubColorDepth);

        	pGRA->GRA_BLT_SRC_ADDR = longtmp;

			longtmp = (dstBufAttr->ulBaseAddr)
					+ ((dstBufAttr->usLineOffset) * (usDstStarty))
					+ (usDstStartx * ubColorDepth);

        	pGRA->GRA_BLT_DST_ADDR = longtmp;
        break;

        case GRA_RIGHT_BOT_LEFT:
			pGRA->GRA_BLT_ROP_CTL = GRA_RIGHT_BOT_LEFT;

			longtmp = (srcBufAttr->ulBaseAddr)
				+ ((srcBufAttr->usLineOffset) * (srcrect->usTop + cut_height - 1)) 
				+ ((srcrect->usLeft + cut_width - 1) * ubColorDepth);

        	pGRA->GRA_BLT_SRC_ADDR = longtmp;

			longtmp = (dstBufAttr->ulBaseAddr)
				+ ((dstBufAttr->usLineOffset) * (usDstStarty + cut_height - 1))
				+ ((usDstStartx + cut_width - 1) * ubColorDepth);

        	pGRA->GRA_BLT_DST_ADDR = longtmp;
        break;

        case GRA_LEFT_BOT_RIGHT:
			pGRA->GRA_BLT_ROP_CTL = GRA_LEFT_BOT_RIGHT;

			longtmp = (srcBufAttr->ulBaseAddr)
				+ ((srcBufAttr->usLineOffset) * (srcrect->usTop + cut_height - 1)) 
				+ ((srcrect->usLeft) * ubColorDepth);

        	pGRA->GRA_BLT_SRC_ADDR = longtmp;

			longtmp = (dstBufAttr->ulBaseAddr)
				+ ((dstBufAttr->usLineOffset) * (usDstStarty + cut_height - 1))
				+ (usDstStartx * ubColorDepth);

        	pGRA->GRA_BLT_DST_ADDR = longtmp;
        break;

        case GRA_RIGHT_TOP_LEFT:
			pGRA->GRA_BLT_ROP_CTL = GRA_RIGHT_TOP_LEFT;

			longtmp = (srcBufAttr->ulBaseAddr)
				+ ((srcBufAttr->usLineOffset) * (srcrect->usTop)) 
				+ ((srcrect->usLeft + cut_width - 1) * ubColorDepth);

        	pGRA->GRA_BLT_SRC_ADDR = longtmp;

			longtmp = (dstBufAttr->ulBaseAddr)
				+ ((dstBufAttr->usLineOffset) * (usDstStarty))
				+ ((usDstStartx + cut_width - 1) * ubColorDepth);

        	pGRA->GRA_BLT_DST_ADDR = longtmp;
        break;
    }
    
    /* Set Graphics Control OPR */
    pGRA->GRA_BLT_W   	   = cut_width;
    pGRA->GRA_BLT_H   	   = cut_height;   
    pGRA->GRA_BLT_FMT 	   = GRA_DES_CLR_FMT((ubColorDepth - 1)) | GRA_SRC_CLR_FMT((ubColorDepth - 1));    
    pGRA->GRA_BLT_ROP 	   = ((MMP_UBYTE)ropcode & GRA_ROP_MASK);
    pGRA->GRA_BLT_ROP_CTL |= (GRA_DO_ROP | GRA_MEM_2_MEM);

	if (ubTranspActive)
		pGRA->GRA_BLT_ROP_CTL |= GRA_DO_ROP_WITH_TP;
	else
		pGRA->GRA_BLT_ROP_CTL &= ~(GRA_DO_ROP_WITH_TP);

	pGRA->GRA_BLT_SRC_PITCH = srcBufAttr->usLineOffset;
	pGRA->GRA_BLT_DST_PITCH = dstBufAttr->usLineOffset;
	
	/* Start BitBLT */
    pGRA->GRA_BLT_EN = GRA_BLT_ST;
    while ((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

    if (ulTimeOutTick == 0) {
	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
	    return MMP_GRA_ERR_TIMEOUT;
    }

	MMPF_OS_ReleaseSem(m_GraphicSemID);
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_RotateImageBuftoBuf
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_RotateImageBuftoBuf(MMP_GRAPHICS_BUF_ATTR 	*srcBufAttr,
						                  MMP_GRAPHICS_RECT    		*srcrect, 
						                  MMP_GRAPHICS_BUF_ATTR 	*dstBufAttr, 
						                  MMP_USHORT                usDstStartx, 
						                  MMP_USHORT                usDstStarty, 
						                  MMP_GRAPHICS_ROP         	ropcode,
						                  MMP_GRAPHICS_ROTATE_TYPE 	rotate, 
						                  MMP_UBYTE 				ubTranspActive, 
						                  MMP_ULONG 				ulKeyColor)
{
	AITPS_GRA   pGRA = AITC_BASE_GRA;
	MMP_ULONG	longtmp;
	MMP_USHORT	cut_width, cut_height;
	MMP_USHORT  usDstRectW = 0, usDstRectH = 0;
	MMP_UBYTE   ubColorDepth    = 2;
    MMP_ULONG   ulTimeOutTick   = GRAPHICS_WHILE_TIMEOUT;

	MMPF_OS_AcquireSem(m_GraphicSemID, GRAPHICS_SEM_TIMEOUT);

	/* Error Parameter Check */
	if (srcrect->usLeft >= srcBufAttr->usWidth || 
		srcrect->usTop  >= srcBufAttr->usHeight||
		usDstStartx >= dstBufAttr->usWidth     || 
		usDstStarty >= dstBufAttr->usHeight    ||
		!srcrect->usWidth                      || 
		!srcrect->usHeight) 
	{
		MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
    }

	if (srcBufAttr->colordepth != dstBufAttr->colordepth) {
		MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
	}			

    if( (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
        (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) ||
        (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_32))
    {
        /* I420/NV12 use MMP_GRAPHICS_COLORDEPTH_8 to do BLT operation in multiple-times */
    	MMPF_OS_ReleaseSem(m_GraphicSemID);
        return MMP_GRA_ERR_PARAMETER;
	}

    pGRA->GRA_BLT_BG = ulKeyColor;

    if (dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV ||
        dstBufAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU) {
        ubColorDepth = 2;
	}	
    else {
        ubColorDepth = dstBufAttr->colordepth;
    }
    
    /* Adjust Cut Width/Height */
	if ((srcrect->usWidth + srcrect->usLeft) > srcBufAttr->usWidth)
		cut_width = srcBufAttr->usWidth - srcrect->usLeft;
	else	
		cut_width = srcrect->usWidth;

	if ((srcrect->usHeight + srcrect->usTop) > srcBufAttr->usHeight)
		cut_height = srcBufAttr->usHeight - srcrect->usTop;
	else
		cut_height = srcrect->usHeight;

	if ((cut_width + usDstStartx) > dstBufAttr->usWidth)
		cut_width = dstBufAttr->usWidth - usDstStartx;

	if ((cut_height + usDstStarty) > dstBufAttr->usHeight)
		cut_height = dstBufAttr->usHeight - usDstStarty;

    /* Set Graphics Control OPR */
	pGRA->GRA_BLT_W 		= cut_width;
	pGRA->GRA_BLT_H 		= cut_height;
	pGRA->GRA_BLT_FMT		= GRA_DES_CLR_FMT((ubColorDepth - 1)) | GRA_SRC_CLR_FMT((ubColorDepth - 1));    
	pGRA->GRA_BLT_ROP 		= ((MMP_UBYTE)ropcode & GRA_ROP_MASK);	
	pGRA->GRA_BLT_ROP_CTL 	= GRA_DO_ROP | GRA_MEM_2_MEM | GRA_LEFT_TOP_RIGHT;

	if (ubTranspActive)
		pGRA->GRA_BLT_ROP_CTL |= GRA_DO_ROP_WITH_TP;
	else
		pGRA->GRA_BLT_ROP_CTL &= ~(GRA_DO_ROP_WITH_TP);
	
	/* Set Source Buffer */	
	longtmp = (srcBufAttr->ulBaseAddr)
    			+ ((srcBufAttr->usLineOffset) * (srcrect->usTop))
	    		+ ((srcrect->usLeft) * ubColorDepth);

    pGRA->GRA_BLT_SRC_ADDR = longtmp;
	pGRA->GRA_BLT_SRC_PITCH = (srcBufAttr->usLineOffset);
    
    /* Set Destination Buffer */
    if(rotate == MMP_GRAPHICS_ROTATE_NO_ROTATE || 
       rotate == MMP_GRAPHICS_ROTATE_RIGHT_180 ){
        usDstRectW = cut_width;
        usDstRectH = cut_height;
    }
    else if(rotate == MMP_GRAPHICS_ROTATE_RIGHT_90 || 
            rotate == MMP_GRAPHICS_ROTATE_RIGHT_270 ){
        usDstRectW = cut_height;
        usDstRectH = cut_width;
    }

	switch(rotate)
	{
	case MMP_GRAPHICS_ROTATE_NO_ROTATE :
    	longtmp = (dstBufAttr->ulBaseAddr)
					+ ((dstBufAttr->usLineOffset) * (usDstStarty))
					+ ((usDstStartx * ubColorDepth));
    	
    	pGRA->GRA_ROTE_PIXL_OFFSET  = 0;
		pGRA->GRA_BLT_DST_PITCH     = (dstBufAttr->usLineOffset);
		pGRA->GRA_ROTE_CTL          = 0;
		break;
	case MMP_GRAPHICS_ROTATE_RIGHT_90:
        longtmp = (dstBufAttr->ulBaseAddr)
					+ ((dstBufAttr->usLineOffset) * (usDstStarty))
					+ ((usDstStartx + usDstRectW - 1) * ubColorDepth);

        pGRA->GRA_ROTE_PIXL_OFFSET  = (GRA_PIXOFST_POS | dstBufAttr->usLineOffset);
        pGRA->GRA_BLT_DST_PITCH     = ubColorDepth;
		pGRA->GRA_ROTE_CTL          = (GRA_ROTE_MODE | GRA_LINEOST_NEG);
		break;
	case MMP_GRAPHICS_ROTATE_RIGHT_180:
        longtmp = (dstBufAttr->ulBaseAddr)
					+ ((dstBufAttr->usLineOffset) * (usDstStarty + usDstRectH - 1))
					+ ((usDstStartx + usDstRectW-1) * ubColorDepth);

	    pGRA->GRA_ROTE_PIXL_OFFSET  = (GRA_PIXOFST_NEG | ubColorDepth);
		pGRA->GRA_BLT_DST_PITCH     = (dstBufAttr->usLineOffset);
		pGRA->GRA_ROTE_CTL          = (GRA_ROTE_MODE | GRA_LINEOST_NEG);
		break;
	case MMP_GRAPHICS_ROTATE_RIGHT_270:
        longtmp = (dstBufAttr->ulBaseAddr)
					+ ((dstBufAttr->usLineOffset) * (usDstStarty + usDstRectH - 1))
					+ (usDstStartx * ubColorDepth);

		pGRA->GRA_ROTE_PIXL_OFFSET  = (GRA_PIXOFST_NEG | dstBufAttr->usLineOffset);
        pGRA->GRA_BLT_DST_PITCH     = ubColorDepth;
		pGRA->GRA_ROTE_CTL          = (GRA_ROTE_MODE | GRA_LINEOST_POS);
		break;
	default :
		MMPF_OS_ReleaseSem(m_GraphicSemID);
		return MMP_GRA_ERR_PARAMETER;
		break;
	}

	pGRA->GRA_BLT_DST_ADDR = longtmp;
	
	/* Start BitBLT */
    pGRA->GRA_BLT_EN = GRA_BLT_ST;
    while ((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

    if (ulTimeOutTick == 0) {
	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
        return MMP_GRA_ERR_TIMEOUT;
    }

	pGRA->GRA_ROTE_CTL &= ~(GRA_ROTE_MODE);

	MMPF_OS_ReleaseSem(m_GraphicSemID);
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_DrawRectToBuf
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Graphics_DrawRectToBuf(MMP_GRAPHICS_DRAWRECT_ATTR 	*drawAttr, 
                                    MMP_GRAPHICS_RECT          	*rect,
                                    MMP_ULONG                   *pOldColor)
{
    AITPS_GRA   pGRA = AITC_BASE_GRA;
    MMP_ULONG   longtmp, oldcolor;
    MMP_USHORT  cut_width, cut_height;
    MMP_UBYTE   ubColorDepth    = 2;
    MMP_ULONG   ulTimeOutTick   = GRAPHICS_WHILE_TIMEOUT;

    /* Error Parameter Check */
    if ( drawAttr->bUseRect == MMP_TRUE      &&
        (rect->usLeft >= drawAttr->usWidth   || 
         rect->usTop  >= drawAttr->usHeight  ||
         !rect->usWidth                      || 
         !rect->usHeight) )
    {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Invalid parameters for graphic");
        return MMP_GRA_ERR_PARAMETER;
    }
    
    if( drawAttr->type == MMP_GRAPHICS_LINE_FILL &&
        drawAttr->bUseRect == MMP_TRUE            &&
        (rect->usWidth <= (2 * drawAttr->ulPenSize)   ||
        rect->usHeight <= (2 * drawAttr->ulPenSize)) )     
    {
        MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Invalid parameters for graphic drawing");
        return MMP_GRA_ERR_PARAMETER;
    }

    if( (drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420) ||
        (drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE) ||
        (drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_32))
    {
        /* I420/NV12 use MMP_GRAPHICS_COLORDEPTH_8 to do BLT operation in multiple-times */
        return MMP_GRA_ERR_PARAMETER;
	}
    
    /* Adjust Cut Width/Height */
    if (drawAttr->bUseRect == MMP_TRUE)
    {
    	if ((rect->usWidth + rect->usLeft) > drawAttr->usWidth)
    		cut_width = drawAttr->usWidth - rect->usLeft;
        else
            cut_width = rect->usWidth;

    	if ((rect->usHeight + rect->usTop) > drawAttr->usHeight)
    		cut_height = drawAttr->usHeight - rect->usTop;
        else
            cut_height = rect->usHeight;
    }
    else 
    {
    	cut_width  = drawAttr->usWidth;
    	cut_height = drawAttr->usHeight;
    }
    
    /* Set Color Format and ROP */
	if (drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_UYVY ||
	    drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_VYUY ||
	    drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YUYV ||
	    drawAttr->colordepth == MMP_GRAPHICS_COLORDEPTH_YUV422_YVYU) {
	    ubColorDepth = 2;
		pGRA->GRA_BLT_FMT = GRA_DES_CLR_FMT(GRA_CLR_FMT_16BPP);
	}
	else{
	    ubColorDepth = drawAttr->colordepth;
		pGRA->GRA_BLT_FMT = GRA_DES_CLR_FMT((drawAttr->colordepth - 1));
	}
		
    pGRA->GRA_BLT_ROP = (MMP_UBYTE)drawAttr->ropcode & GRA_ROP_MASK;
    pGRA->GRA_BLT_ROP_CTL = (GRA_DO_ROP | GRA_SOLID_FILL | GRA_LEFT_TOP_RIGHT);

    oldcolor = MMPF_Graphics_SetKeyColor(MMP_GRAPHICS_FG_COLOR, drawAttr->ulColor);
    if (pOldColor) {
        *pOldColor = oldcolor;
    }
 
    if (drawAttr->type == MMP_GRAPHICS_SOLID_FILL) 
    {
        pGRA->GRA_BLT_W = drawAttr->usWidth;
        pGRA->GRA_BLT_H = drawAttr->usHeight;
        
        if (drawAttr->bUseRect == MMP_FALSE) // Full Range Filling
        {
            pGRA->GRA_BLT_DST_ADDR  = drawAttr->ulBaseAddr;
        	pGRA->GRA_BLT_DST_PITCH = drawAttr->usLineOfst;     
        }
        else //Crop Range Filling
        {
    		longtmp = (drawAttr->ulBaseAddr)
					+ ((drawAttr->usLineOfst) * (rect->usTop))
					+ ((rect->usLeft) * ubColorDepth);

            pGRA->GRA_BLT_DST_ADDR = longtmp;
    		pGRA->GRA_BLT_DST_PITCH = drawAttr->usLineOfst;
        }
        
        pGRA->GRA_BLT_EN = GRA_BLT_ST;
        while((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

        if (ulTimeOutTick == 0) {
    	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
            return MMP_GRA_ERR_TIMEOUT;
        }
	}
    else if (drawAttr->type == MMP_GRAPHICS_LINE_FILL)
    { 
        pGRA->GRA_BLT_DST_PITCH = drawAttr->usLineOfst;

        // Top Line
        pGRA->GRA_BLT_W = cut_width;
        pGRA->GRA_BLT_H = drawAttr->ulPenSize;

		longtmp = (drawAttr->ulBaseAddr)
				+ ((drawAttr->usLineOfst) * (rect->usTop))
				+ ((rect->usLeft) * ubColorDepth);
    				
        pGRA->GRA_BLT_DST_ADDR = longtmp;

        pGRA->GRA_BLT_EN = GRA_BLT_ST;
        while((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

        if (ulTimeOutTick == 0) {
    	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
            return MMP_GRA_ERR_TIMEOUT;
        }

        // Bottom Line
        if(cut_height > 2 * drawAttr->ulPenSize)
        {
            pGRA->GRA_BLT_W = cut_width;
            pGRA->GRA_BLT_H = drawAttr->ulPenSize;

        	longtmp = (drawAttr->ulBaseAddr)
        			+ ((drawAttr->usLineOfst) * (rect->usTop + cut_height - drawAttr->ulPenSize))
        			+ ((rect->usLeft) * ubColorDepth);
            			
            pGRA->GRA_BLT_DST_ADDR = longtmp;

            pGRA->GRA_BLT_EN = GRA_BLT_ST;
            while((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

            if (ulTimeOutTick == 0) {
        	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
                return MMP_GRA_ERR_TIMEOUT;
            }
        }
        
        // Left Line
        if (cut_height > 2 * drawAttr->ulPenSize)
        { 
            pGRA->GRA_BLT_W = drawAttr->ulPenSize;
            pGRA->GRA_BLT_H = cut_height - drawAttr->ulPenSize * 2;

    	    longtmp = (drawAttr->ulBaseAddr)
    				+ ((drawAttr->usLineOfst) * (rect->usTop + drawAttr->ulPenSize))
    				+ ((rect->usLeft) * ubColorDepth);
        				
            pGRA->GRA_BLT_DST_ADDR = longtmp;

            pGRA->GRA_BLT_EN = GRA_BLT_ST;
            while((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

            if (ulTimeOutTick == 0) {
        	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
                return MMP_GRA_ERR_TIMEOUT;
            }
        }
        
        // Right Line
        if (cut_height > 2 * drawAttr->ulPenSize && 
            cut_width > drawAttr->ulPenSize) 
        {
            pGRA->GRA_BLT_W = drawAttr->ulPenSize;
            pGRA->GRA_BLT_H = cut_height - drawAttr->ulPenSize * 2;

            longtmp = (drawAttr->ulBaseAddr)
        			+ ((drawAttr->usLineOfst) * (rect->usTop + drawAttr->ulPenSize))
        			+ ((rect->usLeft + cut_width - drawAttr->ulPenSize) * ubColorDepth);
            			
            pGRA->GRA_BLT_DST_ADDR = longtmp;

            pGRA->GRA_BLT_EN = GRA_BLT_ST;
            while((pGRA->GRA_BLT_EN & GRA_BLT_ST) && (--ulTimeOutTick > 0));

            if (ulTimeOutTick == 0) {
        	    MMP_PRINT_RET_ERROR(GRAPHIC_DBG_LEVEL, 0, "Graphics While TimeOut");
                return MMP_GRA_ERR_TIMEOUT;
            }
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Graphics_SetKeyColor
//  Description : 
//------------------------------------------------------------------------------
MMP_ULONG MMPF_Graphics_SetKeyColor(MMP_GRAPHICS_KEYCOLOR keyColorSel, MMP_ULONG ulColor)
{
    AITPS_GRA   pGRA = AITC_BASE_GRA;
    MMP_ULONG   ulOldColor;

    if (keyColorSel == MMP_GRAPHICS_FG_COLOR) {
        ulOldColor = pGRA->GRA_BLT_FG;
        pGRA->GRA_BLT_FG = ulColor;
    }
    else {
        ulOldColor = pGRA->GRA_BLT_BG;
        pGRA->GRA_BLT_BG = ulColor;
    }

    return ulOldColor;
}

