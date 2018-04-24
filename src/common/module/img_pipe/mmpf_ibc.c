/// @ait_only
//==============================================================================
//
//  File        : mmpf_ibc.c
//  Description : Ritina IBC Module Control driver function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "lib_retina.h"
#include "mmp_reg_ibc.h"
#include "mmp_reg_vif.h"
#include "mmpf_ibc.h"
#include "mmpf_mci.h"
#include "mmpf_system.h"
#include "mmpf_sensor.h"
#include "mmpf_display.h"
#include "mmpf_mp4venc.h"

/** @addtogroup MMPF_IBC
 *  @{
 */

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static AITPS_IBCP   	m_pIBC[MMP_IBC_PIPE_MAX];
static IbcCallBackFunc 	*CallBackFuncIbc[MMP_IBC_PIPE_MAX][MMP_IBC_EVENT_MAX];
static void            	*CallBackArguIbc[MMP_IBC_PIPE_MAX][MMP_IBC_EVENT_MAX];
static MMP_IBC_PIPE_BUF m_curIBCBuf[MMP_IBC_PIPE_MAX] ;

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMP_UBYTE            gbIBCLinkEncId[];
extern MMP_IBC_LINK_TYPE  	gIBCLinkType[];
extern MMPF_OS_SEMID   		m_StartPreviewFrameEndSem[];
extern MMPF_OS_SEMID   		m_StopPreviewCtrlSem[];
extern MMP_USHORT           gsVidRecdCurBufMode;
extern MMP_BOOL        		m_bReceiveStopPreviewSig[];
extern MMP_BOOL        		m_bStopPreviewCloseVifInSig[];
extern MMP_BOOL 			m_bStartPreviewFrameEndSig[];
extern MMP_UBYTE            gbPipeLinkedSnr[];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_ISR
//  Description : 
//------------------------------------------------------------------------------
void MMPF_IBC_ISR(void)
{
#if (SENSOR_EN)

    AITPS_IBC   pIBC    = AITC_BASE_IBC;
	MMP_UBYTE	intsrc;
    AITPS_VIF   pVIF    = AITC_BASE_VIF;
    MMP_BOOL    bVIFInEn, bVIFOutEn;

    #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
    MMP_USHORT  vidRecStatus;
    #endif
	MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;
	MMP_UBYTE   i, ubSnrIdx;
	
    AIT_REG_B   *oprIntCpuEn[MMP_IBC_PIPE_MAX];
    AIT_REG_B   *oprIntCpuSR[MMP_IBC_PIPE_MAX];
    AIT_REG_B   *oprBufCfg[MMP_IBC_PIPE_MAX];
 
    oprIntCpuEn[0]  = &pIBC->IBC_P0_INT_CPU_EN;
    oprIntCpuSR[0]  = &pIBC->IBC_P0_INT_CPU_SR;
    oprBufCfg[0]    = &pIBC->IBCP_0.IBC_BUF_CFG;
    
    oprIntCpuEn[1]  = &pIBC->IBC_P1_INT_CPU_EN;
    oprIntCpuSR[1]  = &pIBC->IBC_P1_INT_CPU_SR;
    oprBufCfg[1]    = &pIBC->IBCP_1.IBC_BUF_CFG;
       
    oprIntCpuEn[2]  = &pIBC->IBC_P2_INT_CPU_EN;
    oprIntCpuSR[2]  = &pIBC->IBC_P2_INT_CPU_SR;
    oprBufCfg[2]    = &pIBC->IBCP_2.IBC_BUF_CFG;

    oprIntCpuEn[3]  = &pIBC->IBC_P3_INT_CPU_EN;
    oprIntCpuSR[3]  = &pIBC->IBC_P3_INT_CPU_SR;
    oprBufCfg[3]    = &pIBC->IBCP_3.IBC_BUF_CFG;

    oprIntCpuEn[4]  = &pIBC->IBC_P4_INT_CPU_EN;
    oprIntCpuSR[4]  = &pIBC->IBC_P4_INT_CPU_SR;
    oprBufCfg[4]    = &pIBC->IBC_P4_BUF_CFG;

    for (i = MMP_IBC_PIPE_0; i < MMP_IBC_PIPE_MAX; i++)
    {
    	intsrc = *oprIntCpuEn[i] & *oprIntCpuSR[i];
    	*oprIntCpuSR[i] = intsrc;

        if (intsrc & IBC_INT_PRE_FRM_RDY) 
        {        
            /* IBC bug: pre frame ready is level trigger interrupt,
             * so pre frame ready interrupt will be trigger several times.
             * To avoid this problem, we should disable it as it happens. */
            *oprIntCpuEn[i] &= ~(IBC_INT_PRE_FRM_RDY);

            #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
            if (gIBCLinkType[i] & MMP_IBC_LINK_VIDEO) {
                MMPF_Display_FrameDoneTrigger(i);
            }    
            #endif

            if (CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_PRERDY]) {
                CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_PRERDY](CallBackArguIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_PRERDY]);
        	}
        }

        if (intsrc & IBC_INT_FRM_RDY) 
        {
            // keep cur address
            MMPF_IBC_GetCurBuf(i, &m_curIBCBuf[i] );
            // sean : Bad patch for UV shift, I have no time
            if (gIBCLinkType[i] & MMP_IBC_LINK_VIDEO) {
            #if (BIND_SENSOR_PS5220==1) || ( BIND_SENSOR_PS5250==1)
            MMPF_Icon_ResetModule(i);
            MMPF_IBC_ResetModule(i);
            #endif
            }
            
            #if (VR_SINGLE_CUR_FRAME == 1)||(VR_ENC_EARLY_START == 1)
            if (!(gIBCLinkType[i] & MMP_IBC_LINK_VIDEO)) {
                MMPF_Display_FrameDoneTrigger(i);
            }
            else {
                /* For single current frame mode, enable IBC pre frame ready interrupt
                 * again in frame done. */
                vidRecStatus = MMPF_MP4VENC_GetStatus(gbIBCLinkEncId[i]);
                
                if ((vidRecStatus == MMPF_MP4VENC_FW_STATUS_START) ||
                    (vidRecStatus == MMPF_MP4VENC_FW_STATUS_RESUME)||
                    (vidRecStatus == MMPF_MP4VENC_FW_STATUS_PREENCODE)) {
                    *oprIntCpuSR[i] = IBC_INT_PRE_FRM_RDY;
                    *oprIntCpuEn[i] |= IBC_INT_PRE_FRM_RDY;
                }
            }
            #else
            if (gsVidRecdCurBufMode == MMPF_MP4VENC_CURBUF_RT) {
                // For real-time encode mode, we don't need to keep current frame
                if (!(gIBCLinkType[i] & MMP_IBC_LINK_VIDEO)) {
                    MMPF_Display_FrameDoneTrigger(i);
                }
            }
            else {
                MMPF_Display_FrameDoneTrigger(i);
            }
            #endif

            if (CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_RDY]) {
                CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_RDY](CallBackArguIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_RDY]);
        	}
        }

        if (intsrc & IBC_INT_FRM_END) 
        {
            for (ubSnrIdx = 0; ubSnrIdx < VIF_SENSOR_MAX_NUM; ubSnrIdx++) 
            {
                if (gbPipeLinkedSnr[i] == ubSnrIdx) 
                {
            		if (m_bStartPreviewFrameEndSig[ubSnrIdx]) {
            			m_bStartPreviewFrameEndSig[ubSnrIdx] = MMP_FALSE;
            			MMPF_OS_ReleaseSem(m_StartPreviewFrameEndSem[ubSnrIdx]);
            		}

                   	if (m_bReceiveStopPreviewSig[i]) {
                   		/* Stop all preview pipe */
            			if (MMPF_Display_GetActivePipeNum(ubSnrIdx) == 0) {

            				ubVifId = MMPF_Sensor_GetVIFPad(ubSnrIdx);

            				MMPF_VIF_IsOutputEnable(ubVifId, &bVIFOutEn);

            				if (bVIFOutEn == MMP_FALSE) {
                           		*oprIntCpuEn[i] &= ~(IBC_INT_FRM_END);

                           		if (i <= MMP_IBC_PIPE_3)
                           			*oprBufCfg[i] &= ~(IBC_STORE_EN);
                           		else
                           			*oprBufCfg[i] &= ~(IBC_P4_STORE_EN);
                           		
                                MMPF_VIF_IsInterfaceEnable(ubVifId, &bVIFInEn);
                                
                                if (bVIFInEn) {
                                	/* Inform VIF to close input */
                                    m_bStopPreviewCloseVifInSig[ubSnrIdx] = MMP_TRUE;
                                    pVIF->VIF_INT_CPU_SR[ubVifId] |= VIF_INT_FRM_END;
                                    pVIF->VIF_INT_CPU_EN[ubVifId] |= VIF_INT_FRM_END;
                                }
                                else {
                                    MMPF_OS_ReleaseSem(m_StopPreviewCtrlSem[ubSnrIdx]);
            				    }
            				}
            			}
            			else {
            				/* Stop single preview pipe */
                            *oprIntCpuEn[i] &= ~(IBC_INT_FRM_END);
                            
                            if (i <= MMP_IBC_PIPE_3)
                            	*oprBufCfg[i] &= ~(IBC_STORE_EN);
                            else
                            	*oprBufCfg[i] &= ~(IBC_P4_STORE_EN);

                            MMPF_OS_ReleaseSem(m_StopPreviewCtrlSem[ubSnrIdx]);
            			}
            		}
            	}
            }

            if (CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_END]) {
                CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_END](CallBackArguIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_END]);
            }
        }
 
		if (intsrc & IBC_INT_FRM_ST) 
		{
            if (CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_ST]) {
                CallBackFuncIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_ST](CallBackArguIbc[MMP_IBC_PIPE_0 + i][MMP_IBC_EVENT_FRM_ST]);
            }
		}
        
        if (intsrc & IBC_INT_H264_RT_BUF_OVF)
        {
            RTNA_DBG_Str(3, FG_RED("H264_RT_BUF_OVF")"\r\n");
        }
    }
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_Init
//  Description : This function initial IBC module.
//------------------------------------------------------------------------------
/** 
 * @brief This function initial IBC module.
 * 
 *  This function initial IBC module.
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_Init(void)
{
	static MMP_BOOL bInitFlag = MMP_FALSE;
	AITPS_AIC pAIC = AITC_BASE_AIC;
	AITPS_IBC pIBC = AITC_BASE_IBC;

	if (!bInitFlag) 
	{
	    RTNA_AIC_Open(pAIC, AIC_SRC_IBC, ibc_isr_a,
	                  AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
	    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_IBC);
	    
		m_pIBC[0] = &pIBC->IBCP_0;
        m_pIBC[1] = &pIBC->IBCP_1;
		m_pIBC[2] = &pIBC->IBCP_2;
        m_pIBC[3] = &pIBC->IBCP_3;
        
        bInitFlag = MMP_TRUE;
	}
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetH264RT_Enable
//  Description : This function set IBC to H264 realtime mode enable.
//------------------------------------------------------------------------------
/** 
 * @brief This function set IBC to H264 realtime mode enable.
 * 
 *  This function set IBC to H264 realtime mode enable.
 * @param[in] ubEn : stands for IBC realtime mode enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetH264RT_Enable(MMP_BOOL ubEn)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
    
	if(ubEn == MMP_TRUE) {
    	pIBC->IBC_IMG_PIPE_CTL |= IBC_H264_RT_EN;
    }
    else {
    	pIBC->IBC_IMG_PIPE_CTL &= ~(IBC_H264_RT_EN);
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetRtPingPongAddr
//  Description : This function set IBC realtime to H264 ping-pong buffer.
//------------------------------------------------------------------------------
/** 
 * @brief This function set IBC realtime to H264 ping-pong buffer.
 * 
 *  This function set IBC realtime to H264 ping-pong buffer. 
 * @param[in] pipeID   : stands for IBC pipe ID.
 * @param[in] ulYAddr  : stands for IBC ping-pong Y line buffer. 
 * @param[in] ulUAddr  : stands for IBC ping-pong Cb line buffer. 
 * @param[in] ulVAddr  : stands for IBC ping-pong Cr line buffer. 
 * @param[in] ulY1Addr : stands for IBC ping-pong Y1 line buffer. 
 * @param[in] ulU1Addr : stands for IBC ping-pong Cb1 line buffer.         
 * @param[in] ulV1Addr : stands for IBC ping-pong Cr1 line buffer.
 * @param[in] usFrmW   : stands for IBC frame width for encode.
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetRtPingPongAddr(MMP_IBC_PIPEID pipeID, 
                                   MMP_ULONG ulYAddr,  MMP_ULONG ulUAddr, MMP_ULONG ulVAddr,
                                   MMP_ULONG ulY1Addr, MMP_ULONG ulU1Addr, MMP_ULONG ulV1Addr, 
                                   MMP_USHORT usFrmW)
{
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];
        
    if (pipeID > MMP_IBC_PIPE_3) {
        return MMP_IBC_ERR_PARAMETER;
    }
    
    if (ulYAddr)
       pIbcPipe->IBC_ADDR_Y_ST = ulYAddr;
    if (ulUAddr)
       pIbcPipe->IBC_ADDR_U_ST = ulUAddr;
    if (ulVAddr)
       pIbcPipe->IBC_ADDR_V_ST = ulVAddr; 

    if (ulY1Addr)
       pIbcPipe->IBC_ADDR_Y_END = ulY1Addr;
    if (ulU1Addr)
       pIbcPipe->IBC_ADDR_U_END = ulU1Addr;
    if (ulV1Addr)
       pIbcPipe->IBC_ADDR_V_END = ulV1Addr; 

    // For MCR_V2 IBC limitation
    pIbcPipe->IBC_FRM_WIDTH = usFrmW >> 4;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetMCI_ByteCount
//  Description : This function set MCI byte count for IBC.
//------------------------------------------------------------------------------
/** 
 * @brief This function set MCI byte count for IBC.
 * 
 *  This function set MCI byte count for IBC.
 * @param[in] pipeID       : stands for IBC pipe ID.
 * @param[in] ubByteCntSel : stands for byte count selection.
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetMCI_ByteCount(MMP_IBC_PIPEID pipeID, MMP_USHORT ubByteCntSel)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
	    
	if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_128BYTE)
	{	
		pIBC->IBC_MCI_CFG &= ~(1 << (3 + pipeID));
	}
    else if (ubByteCntSel == MMPF_MCI_BYTECNT_SEL_256BYTE)
    {
		pIBC->IBC_MCI_CFG |= (1 << (3 + pipeID));
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_ResetModule
//  Description : This function reset IBC module.
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_ResetModule(MMP_IBC_PIPEID pipeID)
{
	if (pipeID == MMP_IBC_PIPE_0)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_IBC0, MMP_FALSE);
	else if (pipeID == MMP_IBC_PIPE_1)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_IBC1, MMP_FALSE);
	else if (pipeID == MMP_IBC_PIPE_2)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_IBC2, MMP_FALSE);
	else if (pipeID == MMP_IBC_PIPE_3)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_IBC3, MMP_FALSE);
	else if (pipeID == MMP_IBC_PIPE_4)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_IBC4, MMP_FALSE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_UpdateStoreAddress
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_UpdateStoreAddress(MMP_IBC_PIPEID pipeID, MMP_ULONG ulBaseAddr,
                                    MMP_ULONG ulBaseUAddr, MMP_ULONG ulBaseVAddr)
{
    AITPS_IBC   pIBC        = AITC_BASE_IBC;
    AITPS_IBCP  pIbcPipe    = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        pIBC->IBC_P4_ADDR_Y_ST = ulBaseAddr;
    }
    else {
        pIbcPipe->IBC_ADDR_Y_ST = ulBaseAddr;
        pIbcPipe->IBC_ADDR_U_ST = ulBaseUAddr;
        pIbcPipe->IBC_ADDR_V_ST = ulBaseVAddr;
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetColorFmt
//  Description : This function set IBC input/output color format.
//------------------------------------------------------------------------------
/** 
 * @brief This function set IBC input/output color format.
 * 
 *  This function set IBC input/output color format.
 * @param[in] pipeID : stands for IBC pipe ID.
 * @param[in] fmt    : stands for color format index. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetColorFmt(MMP_IBC_PIPEID pipeID, MMP_IBC_COLOR fmt)
{
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMP_IBC_ERR_PARAMETER;
    }
    
    pIbcPipe->IBC_SRC_SEL &= ~(IBC_SRC_FMT_MASK);
    pIbcPipe->IBC_BUF_CFG &= ~(IBC_FMT_MASK);

    switch (fmt) {
    case MMP_IBC_COLOR_RGB565:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_RGB565;
        break;
    case MMP_IBC_COLOR_RGB888:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_RGB888;
        break;        
    case MMP_IBC_COLOR_I420:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420 | IBC_420_STORE_CBR;
        break;
    case MMP_IBC_COLOR_YUV420_LUMA_ONLY:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420;
        break;
    case MMP_IBC_COLOR_NV12:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420 | IBC_420_STORE_CBR | IBC_INTERLACE_MODE_EN;
        pIbcPipe->IBC_BUF_CFG |= IBC_NV12_EN;
        break;
    case MMP_IBC_COLOR_NV21:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420 | IBC_420_STORE_CBR | IBC_INTERLACE_MODE_EN;
        pIbcPipe->IBC_BUF_CFG |= IBC_NV21_EN;
        break;
    case MMP_IBC_COLOR_M420_CBCR:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420 | IBC_420_STORE_CBR | IBC_INTERLACE_MODE_EN;
        pIbcPipe->IBC_BUF_CFG |= IBC_M420_EN;
        break;
    case MMP_IBC_COLOR_M420_CRCB:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV420 | IBC_420_STORE_CBR | IBC_INTERLACE_MODE_EN;
        pIbcPipe->IBC_BUF_CFG |= IBC_M420_EN | IBC_NV21_EN;
        break;
    case MMP_IBC_COLOR_YUV422:
    case MMP_IBC_COLOR_YUV422_UYVY:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV422;    
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_UYVY;
        break;
    case MMP_IBC_COLOR_YUV422_YUYV:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_YUYV;
        break;
    case MMP_IBC_COLOR_YUV422_YVYU:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_YVYU;
        break;
    case MMP_IBC_COLOR_YUV422_VYUY:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_VYUY;
        break;
    case MMP_IBC_COLOR_YUV444_2_YUV422_YUYV:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_YUYV;
        break;
    case MMP_IBC_COLOR_YUV444_2_YUV422_YVYU:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_YVYU;
        break;
    case MMP_IBC_COLOR_YUV444_2_YUV422_UYVY:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_UYVY;
        break;
    case MMP_IBC_COLOR_YUV444_2_YUV422_VYUY:
        pIbcPipe->IBC_SRC_SEL |= IBC_SRC_YUV444_2_YUV422;
        pIbcPipe->IBC_422_OUTPUT_SEQ = IBC_422_SEQ_VYUY;
        break;
    default:
        return MMP_IBC_ERR_PARAMETER;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_GetColorFmt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_GetColorFmt(MMP_IBC_PIPEID pipeID, MMP_IBC_COLOR *pFmt)
{
    MMP_UBYTE   ubReg   = 0;
	AITPS_IBCP 	pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        *pFmt = MMP_IBC_COLOR_Y_ONLY;
        return MMP_ERR_NONE;
    }

    ubReg = pIbcPipe->IBC_SRC_SEL;

	switch(ubReg & IBC_SRC_FMT_MASK1)
	{
	case IBC_SRC_YUV444_2_YUV422:
		if ((pIbcPipe->IBC_422_OUTPUT_SEQ & IBC_422_SEQ_YUYV) == IBC_422_SEQ_YUYV)
        	*pFmt = MMP_IBC_COLOR_YUV444_2_YUV422_YUYV;
        else if ((pIbcPipe->IBC_422_OUTPUT_SEQ & IBC_422_SEQ_UYVY) == IBC_422_SEQ_UYVY)
        	*pFmt = MMP_IBC_COLOR_YUV444_2_YUV422_UYVY;
        else if ((pIbcPipe->IBC_422_OUTPUT_SEQ & IBC_422_SEQ_YVYU) == IBC_422_SEQ_YVYU)
        	*pFmt = MMP_IBC_COLOR_YUV444_2_YUV422_YVYU;
        else if ((pIbcPipe->IBC_422_OUTPUT_SEQ & IBC_422_SEQ_VYUY) == IBC_422_SEQ_VYUY)
        	*pFmt = MMP_IBC_COLOR_YUV444_2_YUV422_VYUY;
		break;
	case IBC_SRC_YUV444_2_YUV420:
		if ((ubReg & IBC_420_STORE_CBR) == IBC_420_STORE_CBR)
		{
			if ((ubReg & IBC_INTERLACE_MODE_EN) == IBC_INTERLACE_MODE_EN)
			{
			    if ((pIbcPipe->IBC_BUF_CFG & IBC_M420_EN) == IBC_M420_EN) {
    			    if ((pIbcPipe->IBC_BUF_CFG & IBC_NV12_EN) == IBC_NV12_EN) {
    				    *pFmt = MMP_IBC_COLOR_M420_CBCR;
			        }
			        else if ((pIbcPipe->IBC_BUF_CFG & IBC_NV21_EN) == IBC_NV21_EN) {
			            *pFmt = MMP_IBC_COLOR_M420_CRCB;
			        }
			    }
			    else {
    			    if ((pIbcPipe->IBC_BUF_CFG & IBC_NV12_EN) == IBC_NV12_EN) {
    				    *pFmt = MMP_IBC_COLOR_NV12;
			        }
			        else if ((pIbcPipe->IBC_BUF_CFG & IBC_NV21_EN) == IBC_NV21_EN) {
			            *pFmt = MMP_IBC_COLOR_NV21;
			        }
			    }
			}
			else {
				*pFmt = MMP_IBC_COLOR_I420;
		    }
		}
		else {
			*pFmt = MMP_IBC_COLOR_YUV420_LUMA_ONLY;
		}
		break;
    case IBC_SRC_RGB565: //IBC_SRC_YUV422
        *pFmt = MMP_IBC_COLOR_RGB565;
        break;
    case IBC_SRC_RGB888: //IBC_SRC_YUV444
        *pFmt = MMP_IBC_COLOR_RGB888;
        break;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetPartialStoreAttr
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_SetPartialStoreAttr(MMP_IBC_PIPEID 	pipeID, 
									 MMP_IBC_PIPE_ATTR  *pAttr,
									 MMP_IBC_RECT       *pRect)
{
	AITPS_IBCP 	pIbcPipe = m_pIBC[pipeID];
	MMP_ULONG	longtmp = 0;
	
    if (pipeID > MMP_IBC_PIPE_3) {
        return MMP_IBC_ERR_PARAMETER;
    }

   	if ( (pAttr->colorformat == MMP_IBC_COLOR_I420) ||
   	     (pAttr->colorformat == MMP_IBC_COLOR_NV12) ||
   	     (pAttr->colorformat == MMP_IBC_COLOR_NV21))
   	{
	    longtmp = (pAttr->ulBaseAddr)
    			+ (pAttr->usBufWidth * pRect->usTop) + pRect->usLeft;   
        pIbcPipe->IBC_ADDR_Y_ST = longtmp;

	    if (pAttr->colorformat == MMP_IBC_COLOR_NV12 ||
	    	pAttr->colorformat == MMP_IBC_COLOR_NV21) {
	   		longtmp = (pAttr->ulBaseUAddr)
	    			+ (pAttr->usBufWidth * (pRect->usTop >> 1)) + ((pRect->usLeft >> 1) << 1);
	        pIbcPipe->IBC_ADDR_U_ST = longtmp;
	        pIbcPipe->IBC_ADDR_V_ST = longtmp;
	    }
	    else {

       		longtmp = (pAttr->ulBaseUAddr)
        			+ ((pAttr->usBufWidth >> 1) * (pRect->usTop >> 1)) + (pRect->usLeft >> 1);
	        pIbcPipe->IBC_ADDR_U_ST = longtmp;

	       	longtmp = (pAttr->ulBaseVAddr)
    	    		+ ((pAttr->usBufWidth >> 1) * (pRect->usTop >> 1)) + (pRect->usLeft >> 1);
        	pIbcPipe->IBC_ADDR_V_ST = longtmp;
   		}
		
        pIbcPipe->IBC_LINE_OFST	 	= pAttr->ulLineOffset; 
   	    pIbcPipe->IBC_CBR_LINE_OFST = pAttr->ulCbrLineOffset;
   	
   	    pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
   		pIbcPipe->IBC_SRC_SEL |= IBC_LINEOFFSET_EN;
   	}
    else if (pAttr->colorformat >= MMP_IBC_COLOR_YUV422_YUYV &&
    		 pAttr->colorformat <= MMP_IBC_COLOR_YUV444_2_YUV422_VYUY) 
    {
	    longtmp = (pAttr->ulBaseAddr)
    	    	+ ((pAttr->usBufWidth) * (pRect->usTop)) 
	    	    + (pRect->usLeft * 2);

    	pIbcPipe->IBC_ADDR_Y_ST = longtmp;

        pIbcPipe->IBC_LINE_OFST = pAttr->ulLineOffset;
        pIbcPipe->IBC_CBR_LINE_OFST = 0;
        
        pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
        pIbcPipe->IBC_SRC_SEL |= IBC_LINEOFFSET_EN;
   	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetSubPipeAttr
//  Description : Subpipe means IBC4
//------------------------------------------------------------------------------
/** @brief
  @param[in] usPipeID 	    which IBC will be followed by subpipe
  @param[in] SubPipeAttr 	the IBC4 attribute buffer
  @return It reports the status of the operation.
*/
static MMP_ERR MMPF_IBC_SetSubPipeAttr(MMP_IBC_PIPE_ATTR *PipeAttr)
{    
    AITPS_IBC pIBC = AITC_BASE_IBC;

    pIBC->IBC_P4_ADDR_Y_ST 	= PipeAttr->ulBaseAddr;
    pIBC->IBC_P4_ADDR_Y_END = PipeAttr->ulBaseEndAddr;
    pIBC->IBC_P4_BUF_CFG 	= IBC_P4_STORE_PIX_CONT | IBC_P4_ICON_PAUSE_EN;

    if (PipeAttr->bMirrorEnable) {
        pIBC->IBC_P4_FRM_WIDTH = PipeAttr->usMirrorWidth;
        pIBC->IBC_P4_BUF_CFG  |= IBC_P4_MIRROR_EN;
    }
    else {
        pIBC->IBC_P4_FRM_WIDTH = 0;	
        pIBC->IBC_P4_BUF_CFG  &= ~(IBC_P4_MIRROR_EN);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_GetSubPipeAttr
//  Description : Subpipe means IBC4
//------------------------------------------------------------------------------
/** @brief
  @param[in] usPipeID 	    which IBC will be followed by subpipe
  @param[in] SubPipeAttr 	the IBC4 attribute buffer
  @return It reports the status of the operation.
*/
static MMP_ERR MMPF_IBC_GetSubPipeAttr(MMP_IBC_PIPE_ATTR *PipeAttr)
{    
    AITPS_IBC pIBC = AITC_BASE_IBC;

    PipeAttr->ulBaseAddr	= pIBC->IBC_P4_ADDR_Y_ST;
    PipeAttr->ulBaseEndAddr	= pIBC->IBC_P4_ADDR_Y_END;
    PipeAttr->InputSource	= MMP_ICO_PIPE_4;
    PipeAttr->usMirrorWidth	= pIBC->IBC_P4_FRM_WIDTH ;
    PipeAttr->bMirrorEnable = (pIBC->IBC_P4_BUF_CFG & IBC_P4_MIRROR_EN) ? (MMP_TRUE) : (MMP_FALSE);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetSubPipeEnable
//  Description : 
//------------------------------------------------------------------------------
/** @brief The function enables or disables subpipe

  @param[in] bEnable enable or disable IBC
  @return It reports the status of the operation.
*/
static MMP_ERR MMPF_IBC_SetSubPipeEnable(MMP_BOOL bEnable)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;

    if (bEnable)
        pIBC->IBC_P4_BUF_CFG |= IBC_P4_STORE_EN;
    else
        pIBC->IBC_P4_BUF_CFG &= ~(IBC_P4_STORE_EN);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetSubPipeSingleFrmEnable
//  Description : 
//------------------------------------------------------------------------------
static MMP_ERR MMPF_IBC_SetSubPipeSingleFrmEnable(MMP_BOOL bEnable)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;

    if (bEnable) {
        pIBC->IBC_P4_BUF_CFG |= IBC_P4_STORE_SING_FRAME;
    }
    else {
        pIBC->IBC_P4_BUF_CFG &= ~(IBC_P4_STORE_SING_FRAME);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetInputSrc
//  Description : The function set input source of IBC module.
//------------------------------------------------------------------------------
/** 
 * @brief The function set input source of IBC module
 * 
 *  The function set IBC input source.
 * @param[in] pipeID the IBC ID
 * @param[in] source the IBC input source
 * @return It return the function status.  
 */
void MMPF_IBC_SetInputSrc(MMP_IBC_PIPEID pipeID, MMP_ICO_PIPEID source)
{    
    AITPS_IBCP  pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3)
        return;

    pIbcPipe->IBC_SRC_SEL = (pIbcPipe->IBC_SRC_SEL & ~IBC_SRC_SEL_MASK) |
                            IBC_SRC_SEL_ICO(source);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetAttributes
//  Description : 
//------------------------------------------------------------------------------
/** @brief The function sets the attributes to the specified icon with its icon ID

The function sets the attributes to the specified icon with its icon ID. These attributes include icon buffer
starting address, the width, the height and its display location. It is implemented by programming Icon
Controller registers to set those attributes.

  @param[in] usPipeID the IBC ID
  @param[in] pipattribute the IBC attribute buffer
  @return It reports the status of the operation.
*/
MMP_ERR MMPF_IBC_SetAttributes(MMP_IBC_PIPEID pipeID, MMP_IBC_PIPE_ATTR *pipeAttr)
{
    AITPS_IBC   pIBC     = AITC_BASE_IBC;
	AITPS_IBCP 	pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMPF_IBC_SetSubPipeAttr(pipeAttr);
    }

    pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_SING_FRAME);
    pIbcPipe->IBC_SRC_SEL &= ~(IBC_LINEOFFSET_EN);

    if (pipeAttr->function == MMP_IBC_FX_JPG) 
    {
        /* For JPEG enc path, IBC refer JPEG settings for output format, don't set to YUV444 -> YUV422 */
        MMPF_IBC_SetColorFmt(pipeID, MMP_IBC_COLOR_YUV422);
        
        /* JPEG encode doesn't need to set STORE_EN bit */
        pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_EN | IBC_STORE_SING_FRAME);
        
        pIBC->IBC_JPEG_PIPE_SEL = (pIBC->IBC_JPEG_PIPE_SEL & ~IBC_JPEG_SRC_SEL_MASK) | IBC_JPEG_SRC_SEL(pipeID);  
    }
    else if (pipeAttr->function == MMP_IBC_FX_H264_RT) 
    {	
        if (pipeAttr->colorformat == MMP_IBC_COLOR_NV12 || 
            pipeAttr->colorformat == MMP_IBC_COLOR_I420) 
        {
            MMPF_IBC_SetColorFmt(pipeID, pipeAttr->colorformat);
        }
        else {
            return MMP_IBC_ERR_PARAMETER;
        }

        pIbcPipe->IBC_BUF_CFG |= IBC_STORE_SING_FRAME;

        pIBC->IBC_JPEG_PIPE_SEL = (pIBC->IBC_JPEG_PIPE_SEL & ~IBC_H264_SRC_SEL_MASK) | IBC_H264_SRC_SEL(pipeID);
    }
    else if (pipeAttr->function == MMP_IBC_FX_RING_BUF)
    {
        pIbcPipe->IBC_ADDR_Y_ST = pipeAttr->ulBaseAddr;
        
        if (pipeAttr->ulBaseEndAddr >= (pipeAttr->ulBaseAddr + 32)) {
            pIbcPipe->IBC_ADDR_Y_END = pipeAttr->ulBaseEndAddr - 32; //EROY CHECK : Error Trigger
        }
        
        MMPF_IBC_SetColorFmt(pipeID, pipeAttr->colorformat);

        pIbcPipe->IBC_BUF_CFG |= (IBC_RING_BUF_EN);
    }
	else 
	{
        if (pipeAttr->ulBaseAddr != 0) {
            pIbcPipe->IBC_ADDR_Y_ST = pipeAttr->ulBaseAddr;
            pIbcPipe->IBC_ADDR_U_ST = pipeAttr->ulBaseUAddr;
            pIbcPipe->IBC_ADDR_V_ST = pipeAttr->ulBaseVAddr;
        }

        MMPF_IBC_SetColorFmt(pipeID, pipeAttr->colorformat);
	}

    pIbcPipe->IBC_BUF_CFG |= (IBC_STORE_PIX_CONT | IBC_ICON_PAUSE_EN);

	/* Set Line-Offset attribute */
	if ((pipeAttr->function == MMP_IBC_FX_RING_BUF) || 
		(pipeAttr->function == MMP_IBC_FX_TOFB) ||
		(pipeAttr->function == MMP_IBC_FX_FB_GRAY)) 
	{
        if (pipeAttr->ulLineOffset) 
        {
            pIbcPipe->IBC_LINE_OFST = pipeAttr->ulLineOffset;
            pIbcPipe->IBC_CBR_LINE_OFST = pipeAttr->ulCbrLineOffset;
            pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);       
            pIbcPipe->IBC_SRC_SEL |= IBC_LINEOFFSET_EN;
        }
        else {
            pIbcPipe->IBC_LINE_OFST = 0;
            pIbcPipe->IBC_CBR_LINE_OFST = 0;
        }
	}
    
    /* Set Mirror attribute */
    if (pipeAttr->bMirrorEnable)
    {   
        pIbcPipe->IBC_SRC_SEL |= IBC_MIRROR_EN;
        pIbcPipe->IBC_FRM_WIDTH = pipeAttr->usMirrorWidth;
        pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    }
    else 
    {   
        pIbcPipe->IBC_SRC_SEL &= ~(IBC_MIRROR_EN);
        pIbcPipe->IBC_BUF_CFG |= (IBC_STORE_PIX_CONT);
    }

    MMPF_IBC_SetInputSrc(pipeID, pipeAttr->InputSource);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_GetAttributes
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_GetAttributes(MMP_IBC_PIPEID pipeID, MMP_IBC_PIPE_ATTR *pipeAttr)
{
	AITPS_IBCP 	pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMPF_IBC_GetSubPipeAttr(pipeAttr);
    }
	
    pipeAttr->ulBaseAddr	    = pIbcPipe->IBC_ADDR_Y_ST;
	pipeAttr->ulBaseUAddr	    = pIbcPipe->IBC_ADDR_U_ST;
    pipeAttr->ulBaseVAddr	    = pIbcPipe->IBC_ADDR_V_ST;
    pipeAttr->ulLineOffset      = pIbcPipe->IBC_LINE_OFST;
    pipeAttr->InputSource		= (MMP_ICO_PIPEID)((pIbcPipe->IBC_SRC_SEL & IBC_SRC_SEL_MASK) >> 2);
    pipeAttr->ulCbrLineOffset   = pIbcPipe->IBC_CBR_LINE_OFST;
    pipeAttr->usMirrorWidth     = pIbcPipe->IBC_FRM_WIDTH;
    pipeAttr->bMirrorEnable     = (pIbcPipe->IBC_SRC_SEL & IBC_MIRROR_EN) ? (MMP_TRUE) : (MMP_FALSE);
    
    MMPF_IBC_GetColorFmt(pipeID, &pipeAttr->colorformat);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_GetCurBuf
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_GetCurBuf(MMP_IBC_PIPEID pipeID, MMP_IBC_PIPE_BUF *pipeBuf)
{
	AITPS_IBCP 	pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        AITPS_IBC pIBC = AITC_BASE_IBC;
        pipeBuf->ulBaseAddr =  pIBC->IBC_P4_ADDR_Y_ST ; 
        pipeBuf->ulBaseUAddr = 0 ;
        pipeBuf->ulBaseVAddr = 0 ;
    }
	
    pipeBuf->ulBaseAddr	        = pIbcPipe->IBC_ADDR_Y_ST;
	pipeBuf->ulBaseUAddr	    = pIbcPipe->IBC_ADDR_U_ST;
    pipeBuf->ulBaseVAddr	    = pIbcPipe->IBC_ADDR_V_ST;

	return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_GetBackupCurBuf
//  Description : 
//------------------------------------------------------------------------------

MMP_ERR MMPF_IBC_GetBackupCurBuf(MMP_IBC_PIPEID pipeID, MMP_IBC_PIPE_BUF *pipeBuf)
{
    *pipeBuf = m_curIBCBuf[pipeID] ;
	return MMP_ERR_NONE;    
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_IsStoreEnable
//  Description : 
//------------------------------------------------------------------------------
MMP_BOOL MMPF_IBC_IsStoreEnable(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];
    
    if (pipeID > MMP_IBC_PIPE_3) {
        return (pIBC->IBC_P4_BUF_CFG & IBC_P4_STORE_EN) ? (MMP_TRUE) : (MMP_FALSE);
    }
    else {
        return (pIbcPipe->IBC_BUF_CFG & IBC_STORE_EN) ? (MMP_TRUE) : (MMP_FALSE);
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetStoreEnable
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_SetStoreEnable(MMP_IBC_PIPEID pipeID, MMP_BOOL bEnable)
{
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMPF_IBC_SetSubPipeEnable(bEnable);
    }
    
    if (bEnable) {
        pIbcPipe->IBC_BUF_CFG |= IBC_STORE_EN;
    }
    else {
        pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_EN);
    }
		
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetSingleFrmEnable
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_SetSingleFrmEnable(MMP_IBC_PIPEID pipeID, MMP_BOOL bEnable)
{
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMPF_IBC_SetSubPipeSingleFrmEnable(bEnable);
    }
    
    if (bEnable) {
        pIbcPipe->IBC_BUF_CFG |= IBC_STORE_SING_FRAME;
    }
    else {
        pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_SING_FRAME);
    }
		
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetPreFrameRdy
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_SetPreFrameRdy(MMP_IBC_PIPEID pipeID, MMP_USHORT usLineNum, MMP_BOOL bEnable)
{
    AITPS_IBC   pIBC    = AITC_BASE_IBC;
    AIT_REG_W   *OprLineCnt;
    AIT_REG_B   *OprCPUIntEn, *OprCPUIntSR;
    
    switch(pipeID)
    {
        case MMP_IBC_PIPE_0:
            OprLineCnt  = &(pIBC->IBCP_0.IBC_INT_LINE_CNT0);
            OprCPUIntEn = &(pIBC->IBC_P0_INT_CPU_EN);
            OprCPUIntSR = &(pIBC->IBC_P0_INT_CPU_SR);
        break;
        case MMP_IBC_PIPE_1:
            OprLineCnt  = &(pIBC->IBCP_1.IBC_INT_LINE_CNT0);
            OprCPUIntEn = &(pIBC->IBC_P1_INT_CPU_EN);
            OprCPUIntSR = &(pIBC->IBC_P1_INT_CPU_SR);
        break;
        case MMP_IBC_PIPE_2:
            OprLineCnt  = &(pIBC->IBCP_2.IBC_INT_LINE_CNT0);
            OprCPUIntEn = &(pIBC->IBC_P2_INT_CPU_EN);
            OprCPUIntSR = &(pIBC->IBC_P2_INT_CPU_SR);
        break;
        case MMP_IBC_PIPE_3:
            OprLineCnt  = &(pIBC->IBCP_3.IBC_INT_LINE_CNT0);
            OprCPUIntEn = &(pIBC->IBC_P3_INT_CPU_EN);
            OprCPUIntSR = &(pIBC->IBC_P3_INT_CPU_SR);
        break;
        case MMP_IBC_PIPE_4:
            OprLineCnt  = &(pIBC->IBC_P4_INT_LINE_CNT0);
            OprCPUIntEn = &(pIBC->IBC_P4_INT_CPU_EN);
            OprCPUIntSR = &(pIBC->IBC_P4_INT_CPU_SR);
        break;
        default:
            //
        break;
    }
    
    if (bEnable) {
        *OprLineCnt     = usLineNum;
        *OprCPUIntSR    = IBC_INT_PRE_FRM_RDY;
        *OprCPUIntEn   |= IBC_INT_PRE_FRM_RDY;
    }
    else {
    	*OprLineCnt     = 0;
        *OprCPUIntEn   &= ~(IBC_INT_PRE_FRM_RDY);
        *OprCPUIntSR    = IBC_INT_PRE_FRM_RDY;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_ClearFrameEnd
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_ClearFrameEnd(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;

    if (pipeID == MMP_IBC_PIPE_0) {
        pIBC->IBC_P0_INT_HOST_SR = IBC_INT_FRM_END;
    }        
    else if (pipeID == MMP_IBC_PIPE_1) {
        pIBC->IBC_P1_INT_HOST_SR = IBC_INT_FRM_END;
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        pIBC->IBC_P2_INT_HOST_SR = IBC_INT_FRM_END;
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        pIBC->IBC_P3_INT_HOST_SR = IBC_INT_FRM_END;
    }
    else if (pipeID == MMP_IBC_PIPE_4) {
        pIBC->IBC_P4_INT_HOST_SR = IBC_INT_FRM_END;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_ClearFrameReady
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_ClearFrameReady(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;

    if (pipeID == MMP_IBC_PIPE_0) {
        pIBC->IBC_P0_INT_HOST_SR = IBC_INT_FRM_RDY;
    }        
    else if (pipeID == MMP_IBC_PIPE_1) {
        pIBC->IBC_P1_INT_HOST_SR = IBC_INT_FRM_RDY;
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        pIBC->IBC_P2_INT_HOST_SR = IBC_INT_FRM_RDY;
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        pIBC->IBC_P3_INT_HOST_SR = IBC_INT_FRM_RDY;
    }
    else if (pipeID == MMP_IBC_PIPE_4) {
        pIBC->IBC_P4_INT_HOST_SR = IBC_INT_FRM_RDY;
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_CheckFrameEnd
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_CheckFrameEnd(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
    MMP_ULONG ulTimeout = IBC_WHILE_TIMEOUT;

    if (pipeID == MMP_IBC_PIPE_0) {
        while(!(pIBC->IBC_P0_INT_HOST_SR & IBC_INT_FRM_END) && (--ulTimeout > 0));
    }        
    else if (pipeID == MMP_IBC_PIPE_1) {
        while(!(pIBC->IBC_P1_INT_HOST_SR & IBC_INT_FRM_END) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        while(!(pIBC->IBC_P2_INT_HOST_SR & IBC_INT_FRM_END) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        while(!(pIBC->IBC_P3_INT_HOST_SR & IBC_INT_FRM_END) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_4) {
        while(!(pIBC->IBC_P4_INT_HOST_SR & IBC_INT_FRM_END) && (--ulTimeout > 0));
    }

    if (ulTimeout == 0) {
        RTNA_DBG_Str(0, "IBC_CheckFrameEnd TimeOut, pipe:"); 
        RTNA_DBG_Byte(0, pipeID); 
        RTNA_DBG_Str(0, "\r\n");
        return MMP_IBC_ERR_TIMEOUT;
    }
      
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_CheckFrameReady
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_IBC_CheckFrameReady(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
    MMP_ULONG ulTimeout = IBC_WHILE_TIMEOUT;
    
    if (pipeID == MMP_IBC_PIPE_0) {
        while(!(pIBC->IBC_P0_INT_HOST_SR & IBC_INT_FRM_RDY) && (--ulTimeout > 0));
    }        
    else if (pipeID == MMP_IBC_PIPE_1) {
        while(!(pIBC->IBC_P1_INT_HOST_SR & IBC_INT_FRM_RDY) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        while(!(pIBC->IBC_P2_INT_HOST_SR & IBC_INT_FRM_RDY) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        while(!(pIBC->IBC_P3_INT_HOST_SR & IBC_INT_FRM_RDY) && (--ulTimeout > 0));
    }
    else if (pipeID == MMP_IBC_PIPE_4) {
        while(!(pIBC->IBC_P4_INT_HOST_SR & IBC_INT_FRM_RDY) && (--ulTimeout > 0));
    }
   
    if (ulTimeout == 0) {
        RTNA_DBG_Str(0, "IBC_CheckFrameReady TimeOut, pipe:"); 
        RTNA_DBG_Byte(0, pipeID); 
        RTNA_DBG_Str(0, "\r\n");
        return MMP_IBC_ERR_TIMEOUT;
    }
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_CheckH264Overlap
//  Description : 
//------------------------------------------------------------------------------
MMP_BOOL MMPF_IBC_IsH264Overlap(MMP_IBC_PIPEID pipeID)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;

    if (pipeID == MMP_IBC_PIPE_0) {
        if (pIBC->IBC_P0_INT_HOST_SR & IBC_INT_H264_RT_BUF_OVF) {
            pIBC->IBC_P0_INT_HOST_SR = IBC_INT_H264_RT_BUF_OVF;
            return MMP_TRUE;
        }
    }
    else if (pipeID == MMP_IBC_PIPE_1) {
        if (pIBC->IBC_P1_INT_HOST_SR & IBC_INT_H264_RT_BUF_OVF) {
            pIBC->IBC_P1_INT_HOST_SR = IBC_INT_H264_RT_BUF_OVF;
            return MMP_TRUE;
        }
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        if (pIBC->IBC_P2_INT_HOST_SR & IBC_INT_H264_RT_BUF_OVF) {
            pIBC->IBC_P2_INT_HOST_SR = IBC_INT_H264_RT_BUF_OVF;
            return MMP_TRUE;
        }
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        if (pIBC->IBC_P3_INT_HOST_SR & IBC_INT_H264_RT_BUF_OVF) {
            pIBC->IBC_P3_INT_HOST_SR = IBC_INT_H264_RT_BUF_OVF;
            return MMP_TRUE;
        }
    }
    else if (pipeID == MMP_IBC_PIPE_4) {
        if (pIBC->IBC_P4_INT_HOST_SR & IBC_INT_H264_RT_BUF_OVF) {
            pIBC->IBC_P4_INT_HOST_SR = IBC_INT_H264_RT_BUF_OVF;
            return MMP_TRUE;
        }
    }

    return MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetMirrorEnable
//  Description : This function enable mirror function.
//------------------------------------------------------------------------------
/** 
 * @brief This function enable mirror function.
 * 
 *  This function enable mirror function.
 * @param[in] pipeID  : stands for IBC pipe ID.
 * @param[in] bEnable : stands for mirror enable.
 * @param[in] usWidth : stands for frame width.  
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetMirrorEnable(MMP_IBC_PIPEID pipeID, MMP_BOOL bEnable, MMP_USHORT usWidth)
{
	AITPS_IBCP pIbcPipe = m_pIBC[pipeID];

    if (pipeID > MMP_IBC_PIPE_3) {
        return MMP_IBC_ERR_PARAMETER;
    }

    if (bEnable) 
    {
    	pIbcPipe->IBC_FRM_WIDTH = usWidth;
    	pIbcPipe->IBC_SRC_SEL |= IBC_MIRROR_EN;
    	pIbcPipe->IBC_BUF_CFG &= ~(IBC_STORE_PIX_CONT);
    }
    else {
        pIbcPipe->IBC_SRC_SEL &= ~(IBC_MIRROR_EN);
        pIbcPipe->IBC_BUF_CFG |= IBC_STORE_PIX_CONT;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_SetInterruptEnable
//  Description : This function enable interrupt event.
//------------------------------------------------------------------------------
/** 
 * @brief This function enable interrupt event.
 * 
 *  This function enable interrupt event.
 * @param[in] pipeID  : stands for IBC pipe ID.
 * @param[in] event   : stands for interrupt event.
 * @param[in] bEnable : stands for interrupt enable. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_SetInterruptEnable(MMP_IBC_PIPEID pipeID, MMP_IBC_EVENT event, MMP_BOOL bEnable)
{
    AITPS_IBC pIBC = AITC_BASE_IBC;
    static MMP_UBYTE ubFlags[MMP_IBC_EVENT_MAX] = {IBC_INT_FRM_ST, IBC_INT_FRM_RDY, IBC_INT_FRM_END, IBC_INT_PRE_FRM_RDY};

    if (event >= MMP_IBC_EVENT_MAX) {
        return MMP_IBC_ERR_PARAMETER;
    }

    if (pipeID == MMP_IBC_PIPE_0) {
        if (bEnable) {
            pIBC->IBC_P0_INT_CPU_SR = ubFlags[event];
            pIBC->IBC_P0_INT_CPU_EN |= ubFlags[event];
        }
        else {
            pIBC->IBC_P0_INT_CPU_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_IBC_PIPE_1) {
        if (bEnable) {
            pIBC->IBC_P1_INT_CPU_SR = ubFlags[event];
            pIBC->IBC_P1_INT_CPU_EN |= ubFlags[event];
        }
        else {
            pIBC->IBC_P1_INT_CPU_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_IBC_PIPE_2) {
        if (bEnable) {
            pIBC->IBC_P2_INT_CPU_SR = ubFlags[event];
            pIBC->IBC_P2_INT_CPU_EN |= ubFlags[event];
        }
        else {
            pIBC->IBC_P2_INT_CPU_EN &= ~(ubFlags[event]);
        }
    }
    else if (pipeID == MMP_IBC_PIPE_3) {
        if (bEnable) {
            pIBC->IBC_P3_INT_CPU_SR = ubFlags[event];
            pIBC->IBC_P3_INT_CPU_EN |= ubFlags[event];
        }
        else {
            pIBC->IBC_P3_INT_CPU_EN &= ~(ubFlags[event]);
        }
    }  
    else if (pipeID == MMP_IBC_PIPE_4) {
        if (bEnable) {
            pIBC->IBC_P4_INT_CPU_SR = ubFlags[event];
            pIBC->IBC_P4_INT_CPU_EN |= ubFlags[event];
        }
        else {
            pIBC->IBC_P4_INT_CPU_EN &= ~(ubFlags[event]);
        }
    }
    else {
        return MMP_IBC_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_IBC_RegisterIntrCallBack
//  Description : This function register interrupt callback function.
//------------------------------------------------------------------------------
/** 
 * @brief This function register interrupt callback function.
 * 
 *  This function register interrupt callback function.
 * @param[in] pipeID    : stands for IBC pipe ID.
 * @param[in] event     : stands for interrupt event.
 * @param[in] pCallBack : stands for interrupt callback function. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_IBC_RegisterIntrCallBack(MMP_IBC_PIPEID pipeID, MMP_IBC_EVENT event, IbcCallBackFunc *pCallBack, void *pArgu)
{
    if (pCallBack) {
        CallBackFuncIbc[pipeID][event] = pCallBack;
        CallBackArguIbc[pipeID][event] = pArgu;
    }
    else {
        CallBackFuncIbc[pipeID][event] = NULL;
        CallBackArguIbc[pipeID][event] = NULL;
    }

    return MMP_ERR_NONE;
}

/// @}
/// @end_ait_only
