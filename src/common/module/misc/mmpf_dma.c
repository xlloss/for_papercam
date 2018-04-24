//==============================================================================
//
//  File        : mmpf_dma.c
//  Description : Firmware Graphic Control Function (DMA portion)
//  Author      : Alan Wu
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
#include "mmp_reg_dma.h"
#include "mmpf_dma.h"
#include "mmpf_system.h"

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

static DmaCallBackFunc *CallBackFuncM0 = NULL;
static DmaCallBackFunc *CallBackFuncM1 = NULL;
static DmaCallBackFunc *CallBackFuncR0 = NULL;
static DmaCallBackFunc *CallBackFuncR1 = NULL;

static void *CallBackArguM0 = NULL;
static void *CallBackArguM1 = NULL;
static void *CallBackArguR0 = NULL;
static void *CallBackArguR1 = NULL;

static MMPF_OS_SEMID  gDMAMoveSemID;
static MMPF_OS_SEMID  gDMARotSemID;

static MMP_BOOL gbDmaFreeM0;
static MMP_BOOL gbDmaFreeM1;
static MMP_BOOL gbDmaFreeR0;
static MMP_BOOL gbDmaFreeR1;

static MMPF_DMA_ROT_DATA gDmaRotateData[DMA_R_NUM];

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPF_DMA_Rotate(	MMPF_DMA_R_ID 		Dmarid,
								MMP_ULONG 			ulSrcAddr,
								MMP_ULONG 			ulDstAddr,
                                MMP_USHORT 			usSrcWidth,
                                MMP_USHORT 			usSrcHeight,
                                MMP_GRAPHICS_COLORDEPTH colordepth,
                                MMPF_DMA_R_TYPE 	rotatetype,
                                MMP_USHORT 			usSrcOffest,
                                MMP_USHORT 			usDstOffset,
                                MMP_BOOL 			bMirrorEn,
                                MMP_DMA_MIRROR_DIR 	mirrortype);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_DMA_Initialize
//  Description :
//  Note        : put in sys_task:main() or XX_Task() (ex: dsc_Task)
//------------------------------------------------------------------------------
MMP_ERR MMPF_DMA_Initialize(void)
{
    AITPS_AIC   pAIC = AITC_BASE_AIC;
    AITPS_DMA   pDMA = AITC_BASE_DMA;
	static MMP_BOOL bDmaInitFlag = MMP_FALSE;
	
	MMPF_SYS_EnableClock(MMPF_SYS_CLK_DMA, MMP_TRUE);
	
	if (bDmaInitFlag == MMP_FALSE) 
	{
	    RTNA_AIC_Open(pAIC, AIC_SRC_DMA, dma_isr_a,
	                  AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
	    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_DMA);

		gDMAMoveSemID   = MMPF_OS_CreateSem(2);
		gDMARotSemID    = MMPF_OS_CreateSem(2);

        gbDmaFreeM0     = MMP_TRUE;
	    gbDmaFreeM1     = MMP_TRUE;
		gbDmaFreeR0	    = MMP_TRUE;
		gbDmaFreeR1     = MMP_TRUE;
		
	    CallBackFuncM0  = NULL;
	    CallBackFuncM1  = NULL;
	    CallBackFuncR0  = NULL;
		CallBackFuncR1  = NULL;
		
		pDMA->DMA_INT_CPU_SR = DMA_INT_M0_DONE | DMA_INT_M1_DONE | DMA_INT_R0_DONE | DMA_INT_R1_DONE;
		
		bDmaInitFlag = MMP_TRUE;
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DMA_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_DMA_ISR(void)
{
    AITPS_DMA   pDMA = AITC_BASE_DMA;
    MMP_USHORT  intsrc;
    MMP_USHORT  usSemCount_M = 0x0, usSemCount_R = 0x0;
    
    intsrc = pDMA->DMA_INT_CPU_SR & pDMA->DMA_INT_CPU_EN;
    pDMA->DMA_INT_CPU_SR = intsrc;
	
	if (intsrc & DMA_INT_R0_DONE) {
		gDmaRotateData[MMPF_DMA_R_0].BufferIndex++;
            
		if (gDmaRotateData[MMPF_DMA_R_0].BufferIndex < gDmaRotateData[MMPF_DMA_R_0].BufferNum){
			MMPF_DMA_Rotate(MMPF_DMA_R_0,
							gDmaRotateData[MMPF_DMA_R_0].SrcAddr[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
			                gDmaRotateData[MMPF_DMA_R_0].DstAddr[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_0].SrcWidth[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_0].SrcHeight[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_0].ColorDepth, gDmaRotateData[MMPF_DMA_R_0].RotateType, 
                            gDmaRotateData[MMPF_DMA_R_0].SrcLineOffset[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_0].DstLineOffset[gDmaRotateData[MMPF_DMA_R_0].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_0].MirrorEnable, 
                            gDmaRotateData[MMPF_DMA_R_0].MirrorType);              
		}
		else {
			gbDmaFreeR0 = MMP_TRUE;	
			if (MMPF_OS_ReleaseSem(gDMARotSemID)) {
				RTNA_DBG_Str(3, "gDMARotSemID OSSemPost: Fail\r\n");
				return;
			}
			
			if (CallBackFuncR0) {
				(*CallBackFuncR0)(CallBackArguR0);
        	}
        }
	}

	if (intsrc & DMA_INT_R1_DONE) {
		gDmaRotateData[MMPF_DMA_R_1].BufferIndex++;
            
		if (gDmaRotateData[MMPF_DMA_R_1].BufferIndex < gDmaRotateData[MMPF_DMA_R_1].BufferNum){
			MMPF_DMA_Rotate(MMPF_DMA_R_1,
							gDmaRotateData[MMPF_DMA_R_1].SrcAddr[gDmaRotateData[MMPF_DMA_R_1].BufferIndex], 
			                gDmaRotateData[MMPF_DMA_R_1].DstAddr[gDmaRotateData[MMPF_DMA_R_1].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_1].SrcWidth[gDmaRotateData[MMPF_DMA_R_1].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_1].SrcHeight[gDmaRotateData[MMPF_DMA_R_1].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_1].ColorDepth, gDmaRotateData[MMPF_DMA_R_1].RotateType, 
                            gDmaRotateData[MMPF_DMA_R_1].SrcLineOffset[gDmaRotateData[MMPF_DMA_R_1].BufferIndex], 
                            gDmaRotateData[MMPF_DMA_R_1].DstLineOffset[gDmaRotateData[MMPF_DMA_R_1].BufferIndex],
                            gDmaRotateData[MMPF_DMA_R_1].MirrorEnable, 
                            gDmaRotateData[MMPF_DMA_R_1].MirrorType);              
		}
		else {
			gbDmaFreeR1 = MMP_TRUE;
			if (MMPF_OS_ReleaseSem(gDMARotSemID)) {
				RTNA_DBG_Str(3, "gDMARotSemID OSSemPost: Fail\r\n");
				return;
			}
			
			if (CallBackFuncR1) {
				(*CallBackFuncR1)(CallBackArguR1);
        	}
        }
	}

	if (intsrc & DMA_INT_M0_DONE) {
		
        gbDmaFreeM0 = MMP_TRUE;
		if (MMPF_OS_ReleaseSem(gDMAMoveSemID)) {
            RTNA_DBG_Str(3, "gDMAMoveSemID OSSemPost: Fail\r\n");
            return;
        }

        if (CallBackFuncM0) {
        	(*CallBackFuncM0)(CallBackArguM0);
		}
	}	
	
    if (intsrc & DMA_INT_M1_DONE) {
		
        gbDmaFreeM1 = MMP_TRUE;
		if (MMPF_OS_ReleaseSem(gDMAMoveSemID)) {
            RTNA_DBG_Str(3, "gDMAMoveSemID OSSemPost: Fail\r\n");
            return;
        } 
        
		if (CallBackFuncM1) {
			(*CallBackFuncM1)(CallBackArguM1);
		}
	}

	MMPF_SYS_EnableClock(MMPF_SYS_CLK_DMA, MMP_TRUE);

	if ((MMPF_OS_QuerySem(gDMAMoveSemID, &usSemCount_M) == 0x0) && 
	    (MMPF_OS_QuerySem(gDMARotSemID, &usSemCount_R))) 
	{
		if ((usSemCount_M == MMPF_DMA_M_MAX) && (usSemCount_M == MMPF_DMA_R_MAX)) { //No one using DMA !!!
			MMPF_SYS_EnableClock(MMPF_SYS_CLK_DMA, MMP_FALSE);
		}
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DMA_MoveData
//  Description : 
//  Note        : 1. The function doesn't use in ISR, because it called MMPF_OS_AcquireSem.
//                2. The execute time of CallBackFunc must transient, because MMPF_DMA_ISR used the
//                   Callback function.
//------------------------------------------------------------------------------
MMP_ERR MMPF_DMA_MoveData(MMP_ULONG 		ulSrcAddr,
						  MMP_ULONG 		ulDstAddr,
                          MMP_ULONG 		ulCount,
                          DmaCallBackFunc 	*CallBackFunc,
                          void 				*CallBackArg,
                          MMP_BOOL 			bEnLineOfst,
                          MMP_DMA_LINE_OFST *pLineOfst)
{
    MMPF_DMA_M_ID 	Dmamid = 0;
    AITPS_DMA   	pDMA 		= AITC_BASE_DMA;
    MMP_UBYTE   	ubSemRet 	= 0;
    MMP_USHORT  	usSemCount 	= 0x0;

    OS_CRITICAL_INIT();

	ubSemRet = MMPF_OS_AcquireSem(gDMAMoveSemID, DMA_SEM_WAITTIME);
	
	if (ubSemRet == 0x2) { // Acquire semaphore at ISR mode
		MMPF_OS_AcceptSem(gDMAMoveSemID, &usSemCount);
       	
       	if (usSemCount == 0) {
            return MMP_DMA_ERR_BUSY;
		}
	}

	if (ubSemRet != 0x1) 
	{
		MMPF_SYS_EnableClock(MMPF_SYS_CLK_DMA, MMP_TRUE);
		
        if (!MMPF_OS_QuerySem(gDMAMoveSemID, &usSemCount)) 
        {
        	// If usSemCount == 1 using DMA0 first, usSemCount == 0 should use DMA1 first !!
            if (usSemCount == 1) {
	    	    gbDmaFreeM0 	= MMP_FALSE;
		        Dmamid 			= MMPF_DMA_M_0;
		        CallBackFuncM0 	= CallBackFunc;
		        CallBackArguM0 	= CallBackArg;
            }
            else if (usSemCount == 0) {
                /* To support as reentrant function, we should protect the global variables
                 * to be accessed at the same time. */
                OS_ENTER_CRITICAL();

                if (gbDmaFreeM1) {
				    gbDmaFreeM1 	= MMP_FALSE;
			        Dmamid 			= MMPF_DMA_M_1;
			        CallBackFuncM1 	= CallBackFunc;
    		        CallBackArguM1 	= CallBackArg;
		        }
		        else if (gbDmaFreeM0) {
				    gbDmaFreeM0 	= MMP_FALSE;
			        Dmamid 			= MMPF_DMA_M_0;
			        CallBackFuncM0 	= CallBackFunc;
    		        CallBackArguM0 	= CallBackArg;
		        }
                else {
                    RTNA_DBG_Str(0, "Error gbDmaFreeM0 = MMP_FALSE, gbDmaFreeM1 = MMP_FALSE\r\n");
                }
                OS_EXIT_CRITICAL();
            }
			else {
				RTNA_DBG_Str(0, "Error , Move DMA semaphore protect error !\r\n");
			}
		}
		
		/* Set LineOffset Attribute */
		if (bEnLineOfst == MMP_TRUE) {
			if (Dmamid == MMPF_DMA_M_0) {
    			pDMA->DMA_M0_LOFFS.DMA_M_SRC_LOFFS_W    = pLineOfst->ulSrcWidth;
    			pDMA->DMA_M0_LOFFS.DMA_M_SRC_LOFFS_OFFS = pLineOfst->ulSrcOffset;
    			pDMA->DMA_M0_LOFFS.DMA_M_DST_LOFFS_W    = pLineOfst->ulDstWidth;
    			pDMA->DMA_M0_LOFFS.DMA_M_DST_LOFFS_OFFS = pLineOfst->ulDstOffset;
				pDMA->DMA_M_LOFFS_EN |= DMA_M0_LOFFS_EN;
			}
    		else if (Dmamid == MMPF_DMA_M_1) {
    			pDMA->DMA_M1_LOFFS.DMA_M_SRC_LOFFS_W    = pLineOfst->ulSrcWidth;
    			pDMA->DMA_M1_LOFFS.DMA_M_SRC_LOFFS_OFFS = pLineOfst->ulSrcOffset;
    			pDMA->DMA_M1_LOFFS.DMA_M_DST_LOFFS_W    = pLineOfst->ulDstWidth;
    			pDMA->DMA_M1_LOFFS.DMA_M_DST_LOFFS_OFFS = pLineOfst->ulDstOffset;
				pDMA->DMA_M_LOFFS_EN |= DMA_M1_LOFFS_EN;
			}
			else {
				RTNA_DBG_Str(3, "Error DMA LOFFS ID\r\n");
			}
		}
		else {
			if (Dmamid == MMPF_DMA_M_0) {
				pDMA->DMA_M_LOFFS_EN &= ~(DMA_M0_LOFFS_EN);
			}
    		else if (Dmamid == MMPF_DMA_M_1) {
				pDMA->DMA_M_LOFFS_EN &= ~(DMA_M1_LOFFS_EN);
			}
			else {
				RTNA_DBG_Str(3, "Error DMA LOFFS ID\r\n");
			}
		}
		
		if (Dmamid == MMPF_DMA_M_0) {
    		pDMA->DMA_M0.DMA_M_SRC_ADDR = ulSrcAddr;
    		pDMA->DMA_M0.DMA_M_DST_ADDR = ulDstAddr;
    	    pDMA->DMA_M0.DMA_M_BYTE_CNT = ulCount;
	    }
	    else if (Dmamid == MMPF_DMA_M_1) {
    		pDMA->DMA_M1.DMA_M_SRC_ADDR = ulSrcAddr;
    		pDMA->DMA_M1.DMA_M_DST_ADDR = ulDstAddr;
    	    pDMA->DMA_M1.DMA_M_BYTE_CNT = ulCount;
	    }

	    pDMA->DMA_INT_CPU_EN = pDMA->DMA_INT_CPU_EN | (0x1<<Dmamid);

	    if (Dmamid == MMPF_DMA_M_0) {
	    	pDMA->DMA_EN = DMA_M0_EN; 
	    }
	    else {
	    	pDMA->DMA_EN = DMA_M1_EN;
	    }

		return MMP_ERR_NONE;
	}	
	else { 
		RTNA_DBG_Str(3, "gDMAMoveSemID OSSemPend: Fail\r\n");
		return MMP_DMA_ERR_OTHER;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DMA_Rotate
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_DMA_Rotate(	MMPF_DMA_R_ID 		Dmarid,
								MMP_ULONG 			ulSrcAddr,
								MMP_ULONG 			ulDstAddr,
                                MMP_USHORT 			usSrcWidth,
                                MMP_USHORT 			usSrcHeight,
                                MMP_GRAPHICS_COLORDEPTH colordepth,
                                MMPF_DMA_R_TYPE 	rotatetype,
                                MMP_USHORT 			usSrcOffest,
                                MMP_USHORT 			usDstOffset,
                                MMP_BOOL 			bMirrorEn,
                                MMP_DMA_MIRROR_DIR 	mirrortype)
{
     
    AITPS_DMA 	pDMA 	= AITC_BASE_DMA;
    AITPS_DMA_R pRDMA 	= &(pDMA->DMA_R0);

    if (Dmarid == MMPF_DMA_R_0)
        pRDMA = &(pDMA->DMA_R0);
    else if (Dmarid == MMPF_DMA_R_1)
        pRDMA = &(pDMA->DMA_R1);

    pRDMA->DMA_R_SRC_ADDR   = ulSrcAddr;
    pRDMA->DMA_R_SRC_OFST   = usSrcOffest;
    pRDMA->DMA_R_DST_ADDR   = ulDstAddr;
    pRDMA->DMA_R_DST_OFST   = usDstOffset;
    pRDMA->DMA_R_PIX_W      = usSrcWidth - 1;
    pRDMA->DMA_R_PIX_H      = usSrcHeight - 1;
    
    /* Set Rotate Block Size and BPP */
    switch (colordepth) {
    case MMP_GRAPHICS_COLORDEPTH_8:
    case MMP_GRAPHICS_COLORDEPTH_YUV420:
        pRDMA->DMA_R_CTL = DMA_R_BLK_128X128 | DMA_R_BPP_8;
        break;
    case MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE:
    	if (gDmaRotateData[Dmarid].BufferIndex == 0) { //Y
    		pRDMA->DMA_R_CTL = DMA_R_BLK_128X128 | DMA_R_BPP_8;
    	}
    	else if(gDmaRotateData[Dmarid].BufferIndex == 1) { //UV
    		pRDMA->DMA_R_CTL = DMA_R_BLK_64X64 | DMA_R_BPP_16;
    	}
    	break;
    case MMP_GRAPHICS_COLORDEPTH_16:
        pRDMA->DMA_R_CTL = DMA_R_BLK_64X64 | DMA_R_BPP_16;
        break;        
    case MMP_GRAPHICS_COLORDEPTH_24:
        pRDMA->DMA_R_CTL = DMA_R_BLK_64X64 | DMA_R_BPP_24;
        break;
    case MMP_GRAPHICS_COLORDEPTH_32:
        pRDMA->DMA_R_CTL = DMA_R_BLK_64X64 | DMA_R_BPP_32;
        break; 
    default:
        return MMP_DMA_ERR_NOT_SUPPORT;
    }
    
    /* Set Rotate Direction */
    switch (rotatetype) {
    case MMPF_DMA_R_NO:
        pRDMA->DMA_R_CTL |= DMA_R_NO;
        break;
    case MMPF_DMA_R_90:
        pRDMA->DMA_R_CTL |= DMA_R_90;
        break;
    case MMPF_DMA_R_270:
        pRDMA->DMA_R_CTL |= DMA_R_270;
        break;
    case MMPF_DMA_R_180:
        pRDMA->DMA_R_CTL |= DMA_R_180;
        break;
    default:
        return MMP_DMA_ERR_NOT_SUPPORT;
    }

	/* Set Mirror Type */
    if (bMirrorEn == MMP_TRUE) {
        switch(mirrortype) {
        case DMA_MIRROR_H:
            pRDMA->DMA_R_MIRROR_EN = DMA_R_H_ENABLE;
            break;
        case DMA_MIRROR_V:
            pRDMA->DMA_R_MIRROR_EN = DMA_R_V_ENABLE;
            break;
        default:
            pRDMA->DMA_R_MIRROR_EN = DMA_R_MIRROR_DISABLE;
            break;
        }
    }
    else {
        pRDMA->DMA_R_MIRROR_EN = DMA_R_MIRROR_DISABLE;
    }
	
	if (Dmarid == MMPF_DMA_R_0) {
		pDMA->DMA_INT_CPU_EN |= DMA_INT_R0_DONE; 
		pDMA->DMA_EN = DMA_R0_EN;
	}
	else if (Dmarid == MMPF_DMA_R_1) {
		pDMA->DMA_INT_CPU_EN |= DMA_INT_R1_DONE;
		pDMA->DMA_EN = DMA_R1_EN;
	}
	else {
		RTNA_DBG_Str(3, "Error DMA Ratate ID\r\n");
	}
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DMA_RotateImageBuftoBuf
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_DMA_RotateImageBuftoBuf(MMP_GRAPHICS_BUF_ATTR 		*srcBufAttr,
                                     MMP_GRAPHICS_RECT          *srcrect,
                                     MMP_GRAPHICS_BUF_ATTR  	*dstBufAttr,
                                     MMP_USHORT	 				usDstStartX,
                                     MMP_USHORT 				usDstStartY,
                                     MMP_GRAPHICS_ROTATE_TYPE 	rotatetype,
                                     DmaCallBackFunc 			*CallBackFunc,
                                     void 						*CallBackArgu,
                                     MMP_BOOL 					bMirrorEn,
                                     MMP_DMA_MIRROR_DIR			mirrortype)
{
    MMP_ULONG   ulSrcAddrOffset;
    MMP_ULONG   ulDstAddrOffset;
    MMP_ULONG   ulSrcAddrOffset_U;
    MMP_ULONG   ulDstAddrOffset_U;
    MMP_UBYTE   ubSemRet = 0;
	MMP_USHORT  usSemCount = 0x0;
    MMPF_DMA_R_ID Dmarid = MMPF_DMA_R_0;

	OS_CRITICAL_INIT();

	if ((dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_8) &&
		(dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_16) &&
		(dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_24) &&
		(dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_32) &&
		(dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_YUV420)&&
		(dstBufAttr->colordepth !=  MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE)) {
		// Not Support YUV422 format
		RTNA_DBG_Str(3, "MMPF_DMA_RotateImageBuftoBuf: Format Not Supported");
		return MMP_DMA_ERR_PARAMETER;
	}

	ubSemRet = MMPF_OS_AcquireSem(gDMARotSemID, DMA_SEM_WAITTIME);
	
    if (ubSemRet == 0x1) {
        RTNA_DBG_Str(0, "gDMARotSemID OSSemPend: Fail\r\n");
		return MMP_DMA_ERR_OTHER;	
	}
	else 
	{
		MMPF_SYS_EnableClock(MMPF_SYS_CLK_DMA, MMP_TRUE);

		if (ubSemRet == 0x2) { // Acquire at ISR mode
			MMPF_OS_AcceptSem(gDMARotSemID, &usSemCount);

       		if(usSemCount == 0) {
                return MMP_DMA_ERR_BUSY;
			}
		}
		
		if (MMPF_OS_QuerySem(gDMARotSemID, &usSemCount)) {
            RTNA_DBG_Str(0, "gDMARotSemID OSSemPend: Fail\r\n");
            return MMP_DMA_ERR_OTHER;
		}
		else 
		{
			if (usSemCount == 1) { //If usSemCount == 1 using DMA0 first, usSemCount == 0 should use DMA1 first !!
	    		gbDmaFreeR0 	= MMP_FALSE;
                Dmarid          = MMPF_DMA_R_0;
		        CallBackFuncR0 	= CallBackFunc;
		        CallBackArguR0 	= CallBackArgu;
	    	}
			else if (usSemCount == 0) {
                /* To support as reentrant function, we should protect the global variables
                 * to be accessed at the same time. */
                OS_ENTER_CRITICAL();

				if (gbDmaFreeR1) {
					gbDmaFreeR1 	= MMP_FALSE;
			        Dmarid 			= MMPF_DMA_R_1;
			        CallBackFuncR1 	= CallBackFunc;
			        CallBackArguR1 	= CallBackArgu;
		        }
		        else if (gbDmaFreeR0) {
					gbDmaFreeR0 	= MMP_FALSE;
			        Dmarid 			= MMPF_DMA_R_0;
			        CallBackFuncR0 	= CallBackFunc;
		            CallBackArguR0 	= CallBackArgu;
		        }
		        else {
		        	RTNA_DBG_Str(0, "Error gbDmaFreeR0 = MMP_FALSE, gbDmaFreeR1 = MMP_FALSE\r\n");
		    	}
                OS_EXIT_CRITICAL();
			}
			else {
				RTNA_DBG_Str(0, "Error , Rotate DMA semaphore protect error !\r\n");
			}
		}
	}

    gDmaRotateData[Dmarid].ColorDepth       = srcBufAttr->colordepth;
    gDmaRotateData[Dmarid].RotateType       = (MMPF_DMA_R_TYPE)rotatetype;     
    gDmaRotateData[Dmarid].SrcWidth[0]      = (srcrect->usWidth);
    gDmaRotateData[Dmarid].SrcHeight[0]     = (srcrect->usHeight);  
    gDmaRotateData[Dmarid].SrcLineOffset[0] = srcBufAttr->usLineOffset; 
    gDmaRotateData[Dmarid].DstLineOffset[0] = dstBufAttr->usLineOffset; 
    
    switch (srcBufAttr->colordepth){
    case MMP_GRAPHICS_COLORDEPTH_8:
		gDmaRotateData[Dmarid].BufferNum = 1;
		gDmaRotateData[Dmarid].BytePerPixel[0] = 1;
        break;
    case MMP_GRAPHICS_COLORDEPTH_16:
		gDmaRotateData[Dmarid].BufferNum = 1;
	    gDmaRotateData[Dmarid].BytePerPixel[0] = 2;
        break;
    case MMP_GRAPHICS_COLORDEPTH_24:
		gDmaRotateData[Dmarid].BufferNum = 1;
		gDmaRotateData[Dmarid].BytePerPixel[0] = 3;
        break;
    case MMP_GRAPHICS_COLORDEPTH_32:
		gDmaRotateData[Dmarid].BufferNum = 1;
		gDmaRotateData[Dmarid].BytePerPixel[0] = 4;
        break;
    case MMP_GRAPHICS_COLORDEPTH_YUV420:
		gDmaRotateData[Dmarid].BufferNum = 3;
		gDmaRotateData[Dmarid].BytePerPixel[0] = 1;
		gDmaRotateData[Dmarid].BytePerPixel[1] = 1;
		gDmaRotateData[Dmarid].BytePerPixel[2] = 1;
		
	    gDmaRotateData[Dmarid].SrcLineOffset[1] = srcBufAttr->usLineOffset/2;
	    gDmaRotateData[Dmarid].SrcLineOffset[2] = gDmaRotateData[Dmarid].SrcLineOffset[1];        
	    gDmaRotateData[Dmarid].DstLineOffset[1] = dstBufAttr->usLineOffset/2;
	    gDmaRotateData[Dmarid].DstLineOffset[2] = gDmaRotateData[Dmarid].DstLineOffset[1];
	    
        gDmaRotateData[Dmarid].SrcWidth[1]  = (srcrect->usWidth/2);
        gDmaRotateData[Dmarid].SrcHeight[1] = (srcrect->usHeight/2);
        gDmaRotateData[Dmarid].SrcWidth[2]  = (srcrect->usWidth/2);
        gDmaRotateData[Dmarid].SrcHeight[2] = (srcrect->usHeight/2);
       
    	ulSrcAddrOffset_U = srcBufAttr->usLineOffset*srcrect->usTop/4 + srcrect->usLeft*gDmaRotateData[Dmarid].BytePerPixel[1]/2;
    	ulDstAddrOffset_U = dstBufAttr->usLineOffset*usDstStartY/4 + usDstStartX*gDmaRotateData[Dmarid].BytePerPixel[1]/2;
	   
	    gDmaRotateData[Dmarid].SrcAddr[1] = srcBufAttr->ulBaseUAddr + ulSrcAddrOffset_U;
	    gDmaRotateData[Dmarid].SrcAddr[2] = srcBufAttr->ulBaseVAddr + ulSrcAddrOffset_U;  
	    gDmaRotateData[Dmarid].DstAddr[1] = dstBufAttr->ulBaseUAddr + ulDstAddrOffset_U;
	    gDmaRotateData[Dmarid].DstAddr[2] = dstBufAttr->ulBaseVAddr + ulDstAddrOffset_U;
        break;
    case MMP_GRAPHICS_COLORDEPTH_YUV420_INTERLEAVE:
    	gDmaRotateData[Dmarid].BufferNum = 2;
    	gDmaRotateData[Dmarid].BytePerPixel[0] = 1;
    	gDmaRotateData[Dmarid].BytePerPixel[1] = 2;
    	
	    gDmaRotateData[Dmarid].SrcLineOffset[1] = srcBufAttr->usLineOffset;
	    gDmaRotateData[Dmarid].DstLineOffset[1] = dstBufAttr->usLineOffset;
    	gDmaRotateData[Dmarid].SrcWidth[1]  	= (srcrect->usWidth/2);
        gDmaRotateData[Dmarid].SrcHeight[1] 	= (srcrect->usHeight/2);
       
    	ulSrcAddrOffset_U = srcBufAttr->usLineOffset*srcrect->usTop/2 + srcrect->usLeft*gDmaRotateData[Dmarid].BytePerPixel[1]/2;
    	ulDstAddrOffset_U = dstBufAttr->usLineOffset*usDstStartY/2 + usDstStartX*gDmaRotateData[Dmarid].BytePerPixel[1]/2;
    	
	    gDmaRotateData[Dmarid].SrcAddr[1] = srcBufAttr->ulBaseUAddr + ulSrcAddrOffset_U;
	    gDmaRotateData[Dmarid].DstAddr[1] = dstBufAttr->ulBaseUAddr + ulDstAddrOffset_U;
    	break;
    default:
        return MMP_DMA_ERR_NOT_SUPPORT; 
    }
    
	ulSrcAddrOffset = srcBufAttr->usLineOffset*srcrect->usTop + srcrect->usLeft*gDmaRotateData[Dmarid].BytePerPixel[0];
    ulDstAddrOffset = dstBufAttr->usLineOffset*usDstStartY + usDstStartX*gDmaRotateData[Dmarid].BytePerPixel[0];
    
    gDmaRotateData[Dmarid].SrcAddr[0] 	= srcBufAttr->ulBaseAddr + ulSrcAddrOffset;
    gDmaRotateData[Dmarid].DstAddr[0] 	= dstBufAttr->ulBaseAddr + ulDstAddrOffset;
    gDmaRotateData[Dmarid].BufferIndex 	= 0;
    gDmaRotateData[Dmarid].MirrorEnable = bMirrorEn;
    gDmaRotateData[Dmarid].MirrorType   = mirrortype;
    
    if (gDmaRotateData[Dmarid].BufferIndex < gDmaRotateData[Dmarid].BufferNum) {
   	    MMPF_DMA_Rotate(Dmarid,
   	    				gDmaRotateData[Dmarid].SrcAddr[gDmaRotateData[Dmarid].BufferIndex], 
   	                    gDmaRotateData[Dmarid].DstAddr[gDmaRotateData[Dmarid].BufferIndex], 
                        gDmaRotateData[Dmarid].SrcWidth[gDmaRotateData[Dmarid].BufferIndex], 
                        gDmaRotateData[Dmarid].SrcHeight[gDmaRotateData[Dmarid].BufferIndex], 
                        gDmaRotateData[Dmarid].ColorDepth, gDmaRotateData[Dmarid].RotateType, 
                        gDmaRotateData[Dmarid].SrcLineOffset[gDmaRotateData[Dmarid].BufferIndex], 
                        gDmaRotateData[Dmarid].DstLineOffset[gDmaRotateData[Dmarid].BufferIndex], 
                        gDmaRotateData[Dmarid].MirrorEnable, 
                        gDmaRotateData[Dmarid].MirrorType);
	}

    return MMP_ERR_NONE;
}
