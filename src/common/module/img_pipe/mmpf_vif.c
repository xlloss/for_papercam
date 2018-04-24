//==============================================================================
//
//  File        : mmpf_vif.c
//  Description : MMPF_VIF functions
//  Author      : Penguin Torng
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
#include "hdr_cfg.h"
#include "mmp_reg_vif.h"
#include "mmpf_vif.h"
#include "mmpf_system.h"

/** @addtogroup MMPF_VIF
@{
*/
//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

MMPF_VIF_IN_TYPE 	m_VifInterface[MMPF_VIF_MDL_NUM] = {MMPF_VIF_PARALLEL, MMPF_VIF_PARALLEL};
MMPF_VIF_SNR_TYPE   m_VifSnrType[MMPF_VIF_MDL_NUM] = {MMPF_VIF_SNR_TYPE_BAYER, MMPF_VIF_SNR_TYPE_BAYER};

MMP_UBYTE           m_ubVifToRawStoreFmt[MMPF_VIF_MDL_NUM] = {MMPF_VIF_TO_RAW_STORE_FMT_BAYER, MMPF_VIF_TO_RAW_STORE_FMT_BAYER};

VifCallBackFunc     *CallBackFuncVif[MMPF_VIF_MDL_NUM][MMPF_VIF_INT_EVENT_NUM];
void                *CallBackArguVif[MMPF_VIF_MDL_NUM][MMPF_VIF_INT_EVENT_NUM];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetPIODir
//  Description : This function set VIF PIO direction.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF PIO direction.
 * 
 *  This function set VIF PIO direction.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] mask      : stands for bit mask identifying the PIOs
 * @param[in] bOutput   : stands for input or output data.
 * @return It return the function status.   
 */
MMP_ERR	MMPF_VIF_SetPIODir(MMP_UBYTE ubVifId, MMP_UBYTE mask, MMP_BOOL bOutput)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	switch(ubVifId) 
	{
		case MMPF_VIF_MDL_ID0:
			if(bOutput == MMP_TRUE)
				pVIF->VIF_0_SENSR_SIF_EN |= mask;
			else
				pVIF->VIF_0_SENSR_SIF_EN &= ~(mask);
		break;
		case MMPF_VIF_MDL_ID1:
			if(bOutput == MMP_TRUE)
				pVIF->VIF_1_SENSR_SIF_EN |= mask;
			else	
				pVIF->VIF_1_SENSR_SIF_EN &= ~(mask);
		break;
		default:
		break;    		    	
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetPIOOutput
//  Description : This function set VIF PIO output data.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF PIO output data.
 * 
 *  This function set VIF PIO output data.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] mask      : stands for bit mask identifying the PIOs
 * @param[in] bSetHigh  : stands for output data high/low. 
 * @return It return the function status.   
 */
MMP_ERR	MMPF_VIF_SetPIOOutput(MMP_UBYTE ubVifId, MMP_UBYTE mask, MMP_BOOL bSetHigh)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	switch(ubVifId) 
	{
		case MMPF_VIF_MDL_ID0:
			if(bSetHigh == MMP_TRUE)
				pVIF->VIF_0_SENSR_SIF_DATA |= mask;
			else
				pVIF->VIF_0_SENSR_SIF_DATA &= ~(mask);
		break;
		case MMPF_VIF_MDL_ID1:
			if(bSetHigh == MMP_TRUE)
				pVIF->VIF_1_SENSR_SIF_DATA |= mask;
			else	
				pVIF->VIF_1_SENSR_SIF_DATA &= ~(mask);
		break;
		default:
		break;    		    	
	}

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_GetPIOOutput
//  Description : This function get VIF PIO data.
//------------------------------------------------------------------------------
/** 
 * @brief This function get VIF PIO output data.
 * 
 *  This function get VIF PIO output data.
 * @param[in] ubVifId 	: stands for VIF module index. 
 * @param[in] mask      : stands for bit mask identifying the PIOs.
 * @return It return the mask mapping result.  
 */
MMP_BOOL MMPF_VIF_GetPIOOutput(MMP_UBYTE ubVifId, MMP_UBYTE mask)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	switch(ubVifId)
	{
		case MMPF_VIF_MDL_ID0:
			return (pVIF->VIF_0_SENSR_SIF_DATA & mask)?(MMP_TRUE):(MMP_FALSE);
		break;
		case MMPF_VIF_MDL_ID1:
			return (pVIF->VIF_1_SENSR_SIF_DATA & mask)?(MMP_TRUE):(MMP_FALSE);
		break;
		default:
			return MMP_FALSE;
		break;
	}
}

#if !defined(MINIBOOT_FW) && !defined(MBOOT_EX_FW)
//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_ResetModule
//  Description : This function reset VIF module.
//------------------------------------------------------------------------------
/** 
 * @brief This function reset VIF module.
 * 
 *  This function reset VIF module.
 * @param[in] ubVifId   : stands for VIF module index.
 * @return It return the function status.  
 */
MMP_ERR	MMPF_VIF_ResetModule(MMP_UBYTE ubVifId)
{
	if (ubVifId == MMPF_VIF_MDL_ID0) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_VIF0, MMP_FALSE);
	}
	else if (ubVifId == MMPF_VIF_MDL_ID1) {
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_VIF1, MMP_FALSE);
	}
	
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetInputInterface
//  Description : This function register VIF input type.
//------------------------------------------------------------------------------
/** 
 * @brief This function register VIF input type.
 * 
 *  This function register VIF input type.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] type      : stands for VIF input type.
 * @return It return the function status.  
 */
MMP_ERR	MMPF_VIF_SetInputInterface(MMP_UBYTE ubVifId, MMPF_VIF_IN_TYPE type)
{
    if (type == MMPF_VIF_MIPI){
        RTNA_DBG_Str3("Set MIPI Input\r\n");
    }
    else {
        RTNA_DBG_Str3("Set Parallel Input\r\n");
    }

    m_VifInterface[ubVifId] = type;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetSensorType
//  Description : This function set VIF sensor type.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF sensor type.
 * 
 *  This function set VIF sensor type.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] type      : stands for VIF sensor type.
 * @return It return the function status.  
 */
MMP_ERR	MMPF_VIF_SetSensorType(MMP_UBYTE ubVifId, MMPF_VIF_SNR_TYPE type)
{
    m_VifSnrType[ubVifId] = type;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_IsInterfaceEnable
//  Description : This function check VIF input interface enable.
//------------------------------------------------------------------------------
/** 
 * @brief This function check VIF input interface enable.
 * 
 *  This function check VIF input interface enable.
 * @param[in]  ubVifId 	 : stands for VIF module index. 
 * @param[out] bEnable   : stands for input interface enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_IsInterfaceEnable(MMP_UBYTE ubVifId, MMP_BOOL *bEnable)
{
    AITPS_VIF   pVIF 	= AITC_BASE_VIF;
    AITPS_MIPI  pMIPI 	= AITC_BASE_MIPI;

    *bEnable = MMP_FALSE;

    if (m_VifInterface[ubVifId] == MMPF_VIF_MIPI) 
    {	
        if (pMIPI->MIPI_CLK_CFG[ubVifId] & MIPI_CSI2_EN)
            *bEnable = MMP_TRUE;
    }
    else 
    {
        if (pVIF->VIF_IN_EN[ubVifId] & VIF_IN_ENABLE)
            *bEnable = MMP_TRUE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_EnableInputInterface
//  Description : This function set VIF input interface.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF input interface.
 * 
 *  This function set VIF input interface.
 * @param[in] ubVifId 	: stands for VIF module index. 
 * @param[in] bEnable   : stands for input interface enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_EnableInputInterface(MMP_UBYTE ubVifId, MMP_BOOL bEnable)
{
    AITPS_VIF  	pVIF 	= AITC_BASE_VIF;
    AITPS_MIPI  pMIPI 	= AITC_BASE_MIPI;

    if (bEnable == MMP_TRUE) 
    {
        if (m_VifInterface[ubVifId] == MMPF_VIF_MIPI)
        {
            pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_CSI2_EN;
        }
        else
        {
            pVIF->VIF_OUT_EN[ubVifId] &= ~(VIF_OUT_ENABLE);
            pVIF->VIF_IN_EN[ubVifId]  |= VIF_IN_ENABLE;
    	}
    }
    else 
    {
        if (m_VifInterface[ubVifId] == MMPF_VIF_MIPI) 
            pMIPI->MIPI_CLK_CFG[ubVifId] &= ~(MIPI_CSI2_EN);
        else 
           	pVIF->VIF_IN_EN[ubVifId] &= ~(VIF_IN_ENABLE);
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_IsOutputEnable
//  Description : This function check VIF output enable.
//------------------------------------------------------------------------------
/** 
 * @brief This function check VIF output enable.
 * 
 *  This function check VIF output enable.
 * @param[in]  ubVifId     : stands for VIF module index. 
 * @param[out] bEnable   : stands for output enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_IsOutputEnable(MMP_UBYTE ubVifId, MMP_BOOL *bEnable)
{
    AITPS_VIF pVIF = AITC_BASE_VIF; 

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_VIF, MMP_TRUE);
    
    if (pVIF->VIF_OUT_EN[ubVifId] & VIF_OUT_ENABLE) {
        *bEnable = MMP_TRUE;
    }
    else {
        *bEnable = MMP_FALSE;
    }
    
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_VIF, MMP_FALSE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_EnableOutput
//  Description : This function set VIF output enable.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF output enable.
 * 
 *  This function set VIF output enable.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] bEnable   : stands for output frame enable.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_EnableOutput(MMP_UBYTE ubVifId, MMP_BOOL bEnable)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    if (bEnable == MMP_TRUE) 
    {
        pVIF->VIF_OUT_EN[ubVifId] |= VIF_OUT_ENABLE;
        
        if (ubVifId == MMPF_VIF_MDL_ID1) {
        	pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_1_TO_ISP;
    	}
    }
    else 
    {
        pVIF->VIF_OUT_EN[ubVifId] &= ~(VIF_OUT_ENABLE);

        if (ubVifId == MMPF_VIF_MDL_ID1) {
        	pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_1_TO_ISP);
        }
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetGrabPosition
//  Description : This function set VIF grab range.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF grab range.
 * 
 *  This function set VIF grab range.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] pGrab     : stands for VIF grab information.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetGrabPosition(MMP_UBYTE ubVifId, MMPF_VIF_GRAB_INFO *pGrab)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
	
    pVIF->VIF_GRAB[ubVifId].PIXL_ST = pGrab->usStartX;
    pVIF->VIF_GRAB[ubVifId].PIXL_ED = pGrab->usStartX + pGrab->usGrabW - 1;
    pVIF->VIF_GRAB[ubVifId].LINE_ST = pGrab->usStartY;
    pVIF->VIF_GRAB[ubVifId].LINE_ED = pGrab->usStartY + pGrab->usGrabH - 1;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetInterruptLine
//  Description : This function set VIF grab range.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF grab range.
 * 
 *  This function set VIF grab range.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] usLineCnt : stands for VIF interrupt line count.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetInterruptLine(MMP_UBYTE ubVifId, MMP_USHORT usLineCnt)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
	
    pVIF->VIF_INT_LINE_NUM_0[ubVifId] = usLineCnt;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_GetGrabPosition
//  Description : This function get VIF grab position.
//------------------------------------------------------------------------------
/** 
 * @brief This function get VIF grab position.
 * 
 *  This function get VIF grab position.
 * @param[in]  ubVifId        : stands for VIF module index. 
 * @param[out] usPixelStart : stands for VIF grab pixel start position.
 * @param[out] usPixelEnd   : stands for VIF grab pixel end position.
 * @param[out] usLineStart  : stands for VIF grab line start position.
 * @param[out] usLineEnd    : stands for VIF grab line end position.   
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_GetGrabPosition(MMP_UBYTE ubVifId,
                                 MMP_USHORT *usPixelStart, MMP_USHORT *usPixelEnd,
								 MMP_USHORT *usLineStart, MMP_USHORT *usLineEnd)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
	
    *usPixelStart 	= pVIF->VIF_GRAB[ubVifId].PIXL_ST;
    *usLineStart	= pVIF->VIF_GRAB[ubVifId].LINE_ST;
    *usPixelEnd 	= pVIF->VIF_GRAB[ubVifId].PIXL_ED;
    *usLineEnd 		= pVIF->VIF_GRAB[ubVifId].LINE_ED;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_GetGrabResolution
//  Description : This function get VIF grab width and height.
//------------------------------------------------------------------------------
/** 
 * @brief This function get VIF grab width and height.
 * 
 *  This function get VIF grab width and height.
 * @param[in]  ubVifId     : stands for VIF module index. 
 * @param[out] ulWidth   : stands for VIF grab width.
 * @param[out] ulHeight  : stands for VIF grab height. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_GetGrabResolution(MMP_UBYTE ubVifId, MMP_ULONG *ulWidth, MMP_ULONG *ulHeight)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	*ulWidth  = (pVIF->VIF_GRAB[ubVifId].PIXL_ED - pVIF->VIF_GRAB[ubVifId].PIXL_ST + 1);
	*ulHeight = (pVIF->VIF_GRAB[ubVifId].LINE_ED - pVIF->VIF_GRAB[ubVifId].LINE_ST + 1);
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_EnableInterrupt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_VIF_EnableInterrupt(MMP_UBYTE ubVifId, MMP_UBYTE ubFlag, MMP_BOOL bEnable)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

    if (bEnable) {
        pVIF->VIF_INT_CPU_SR[ubVifId] = ubFlag;
        pVIF->VIF_INT_CPU_EN[ubVifId] |= ubFlag;
    } 
    else {
        pVIF->VIF_INT_CPU_EN[ubVifId] &= ~ubFlag;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_OpenInterrupt
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_VIF_OpenInterrupt(MMP_BOOL bEnable)
{
#if defined(ALL_FW)
    AITPS_AIC pAIC = AITC_BASE_AIC;

    if (bEnable) {
        RTNA_AIC_Open(pAIC, AIC_SRC_VIF, vif_isr_a,
                		    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    	RTNA_AIC_IRQ_En(pAIC, AIC_SRC_VIF);
    } 
    else {
        RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_VIF);
    }
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_CheckFrameSig
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_VIF_CheckFrameSig(MMP_UBYTE ubVifId, MMPF_VIF_INT_EVENT ubEvent, MMP_USHORT usFrameCount)
{
    MMP_USHORT  i;
    MMP_BOOL    bEnable;
    AITPS_VIF   pVIF = AITC_BASE_VIF;
    MMP_UBYTE   ubFlags[MMPF_VIF_INT_EVENT_NUM] = {VIF_INT_FRM_ST, VIF_INT_FRM_END, VIF_INT_GRAB_END, VIF_INT_LINE_0};
    MMP_ULONG   ulTimeOutTick = VIF_WHILE_TIMEOUT;

    if (ubFlags[ubEvent] == VIF_INT_FRM_ST) {
    
        MMPF_VIF_IsInterfaceEnable(ubVifId, &bEnable);

        for (i = 0; i < usFrameCount; i++) {
            if (bEnable) {
				ulTimeOutTick = VIF_WHILE_TIMEOUT;
                pVIF->VIF_INT_HOST_SR[ubVifId] = ubFlags[ubEvent];
                while ((!(pVIF->VIF_INT_HOST_SR[ubVifId] & ubFlags[ubEvent])) && (--ulTimeOutTick > 0));
            }
            else {
                MMPF_OS_Sleep(100);
            }
        }
    }
    else {
    	for (i = 0; i < usFrameCount; i++) {
			ulTimeOutTick = VIF_WHILE_TIMEOUT;
	        pVIF->VIF_INT_HOST_SR[ubVifId] = ubFlags[ubEvent];
	        while ((!(pVIF->VIF_INT_HOST_SR[ubVifId] & ubFlags[ubEvent])) && (--ulTimeOutTick > 0));   
    	}
    }

	if (ulTimeOutTick == 0) {
	    RTNA_DBG_Str(0, "VIF While TimeOut\r\n");
	    return MMP_VIF_ERR_TIMEOUT;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetRawOutPath
//  Description : This function set VIF input/output format and path.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF output path.
 * 
 *  This function set VIF input/output format and path.
 * @param[in] ubVifId  : stands for VIF module index. 
 * @param[in] path   : stands for VIF path selection. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetRawOutPath(MMP_UBYTE ubVifId, MMP_BOOL bOutISP, MMP_BOOL bOutRaw)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_OUT_MASK);

	if (bOutISP) {
		pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_2_ISP_EN;
		
		if (ubVifId == MMPF_VIF_MDL_ID1) {
			pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_1_TO_ISP;
		}
	}
	else {
		pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_2_ISP_EN);
    	
		if (ubVifId == MMPF_VIF_MDL_ID1) {
			pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_1_TO_ISP);
    	}
    }
    
	if (bOutRaw) {

    	if (m_VifSnrType[ubVifId] == MMPF_VIF_SNR_TYPE_YUV ||
    	    m_VifSnrType[ubVifId] == MMPF_VIF_SNR_TYPE_YUV_TV_DEC) 
    	{
    	    if (m_ubVifToRawStoreFmt[ubVifId] == MMPF_VIF_TO_RAW_STORE_FMT_YUV420)
    	        pVIF->VIF_RAW_OUT_EN[ubVifId] = 0;
    	    else if (m_ubVifToRawStoreFmt[ubVifId] == MMPF_VIF_TO_RAW_STORE_FMT_YUV422)
    	        pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_2_RAW_EN;
    	}
    	else {
    	    pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_2_RAW_EN;
    	}
	}
	else {
		pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_2_RAW_EN);
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetFormatAndPath
//  Description : This function set VIF input/output format and path.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF input/output format and path.
 * 
 *  This function set VIF input/output format and path.
 * @param[in] ubVifId  : stands for VIF module index. 
 * @param[in] path   : stands for VIF path selection. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetFormatAndPath(MMP_UBYTE ubVifId, MMPF_VIF_PATH path)
{
    AITPS_VIF  	pVIF 	= AITC_BASE_VIF;
    AITPS_MIPI  pMIPI 	= AITC_BASE_MIPI;
    
    switch(path)
    {
        case MMPF_VIF_PATH1_BAYER_TO_ISP:
            pVIF->VIF_YUV_CTL[ubVifId] &= ~(VIF_YUV_EN);
            pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_2_RAW_EN);
            pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_2_ISP_EN;
        break;
        case MMPF_VIF_PATH2_YCbCr422_2_YCbCr444_BYPASS_ISP:
            pVIF->VIF_YUV_CTL[ubVifId] |= VIF_YUV_EN;
        break;
        case MMPF_VIF_PATH3_YCbCr422_2_BAYER:
            pVIF->VIF_YUV_CTL[ubVifId] |= (VIF_YUV_EN | VIF_Y2B_EN);
        break;
        case MMPF_VIF_PATH4_YCbCr422_2_YUV444:
            pVIF->VIF_YUV_CTL[ubVifId] |= (VIF_YUV_EN | VIF_YUV_PATH_OUT_YUV);
        break;
        case MMPF_VIF_PATH5_BAYER_TO_RAWPROC:
            pVIF->VIF_YUV_CTL[ubVifId] &= ~(VIF_YUV_EN);
            pVIF->VIF_RAW_OUT_EN[ubVifId] &= ~(VIF_2_ISP_EN);
            pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_2_RAW_EN;
        break;
        case MMPF_VIF_PATH6_JPG2RAWPROC:
            pVIF->VIF_YUV_CTL[ubVifId] &= ~(VIF_YUV_EN);
            pVIF->VIF_RAW_OUT_EN[ubVifId] |= VIF_JPG_2_RAW_EN;        
        break;
        case MMPF_VIF_PATH7_YCbCr422_2_YCbCr420_TO_RAWPROC:
            pVIF->VIF_YUV420_CTL[ubVifId] &= ~(VIF_MIPI_YUV420_EN);
            pVIF->VIF_YUV420_CTL[ubVifId] |= VIF_PARA_YUV422T0420_EN;
        break;
        case MMPF_VIF_PATH8_VC_DATA:
            pMIPI->MIPI_VC_CTL[ubVifId] |= MIPI_VC_EN;
        break;                                                       
        default:
        break;
    }
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetFixedDataOut
//  Description : This function set VIF fixed data output.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF fixed data output.
 * 
 *  This function set VIF fixed data output.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] bEnable : stands for VIF fixed data output enable. 
 * @param[in] ubData  : stands for VIF fixed data. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetFixedDataOut(MMP_UBYTE ubVifId, MMP_BOOL bEnable, MMP_UBYTE ubData)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    if(bEnable == MMP_TRUE){
    
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_FIXED_OUT_EN;
        pVIF->VIF_FIXED_OUT_DATA[ubVifId] = ubData;
    }
    else{    
        pVIF->VIF_SENSR_CTL[ubVifId] &= ~(VIF_FIXED_OUT_EN);
        pVIF->VIF_FIXED_OUT_DATA[ubVifId] = 0;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetColorID
//  Description : This function set VIF color ID (Pixel ID, Line ID).
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF color ID (Pixel ID, Line ID).
 * 
 *  This function set VIF color ID (Pixel ID, Line ID).
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] clrId : stands for VIF color ID index. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetColorID(MMP_UBYTE ubVifId, MMPF_VIF_COLOR_ID clrId)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

	pVIF->VIF_SENSR_CTL[ubVifId] &= ~(VIF_COLORID_FORMAT_MASK);
    
    switch(clrId) {
    case MMPF_VIF_COLORID_00:
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_COLORID_00;
        break;
    case MMPF_VIF_COLORID_01:
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_COLORID_01;
        break;
    case MMPF_VIF_COLORID_10:
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_COLORID_10;
        break;
    case MMPF_VIF_COLORID_11:
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_COLORID_11;
        break;
    default:
        //
        break;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetFrameSkip
//  Description : This function set VIF frame skip mechanism.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF frame skip mechanism.
 * 
 *  This function set VIF frame skip mechanism.
 * @param[in] ubVifId     : stands for VIF module index. 
 * @param[in] bEnable   : stands for VIF frame skip enable. 
 * @param[in] ubSkipIdx : stands for VIF frame skip index. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetFrameSkip(MMP_UBYTE ubVifId, MMP_BOOL bEnable, MMP_UBYTE ubSkipIdx)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    if (bEnable == MMP_TRUE) {
        pVIF->VIF_FRME_SKIP_EN[ubVifId] |= VIF_FRME_SKIP_ENABLE;
        pVIF->VIF_FRME_SKIP_NO[ubVifId] = ubSkipIdx;
    }
    else {
        pVIF->VIF_FRME_SKIP_EN[ubVifId] &= ~(VIF_FRME_SKIP_ENABLE);
        pVIF->VIF_FRME_SKIP_NO[ubVifId] = 0;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetDownSample
//  Description : This function set VIF down-sample mechanism.
//------------------------------------------------------------------------------
/** 
 * @brief This function set VIF down-sample mechanism.
 * 
 *  This function set VIF down-sample mechanism.
 * @param[in] ubVifId    : stands for VIF module index. 
 * @param[in] bEnable  : stands for VIF down-sample enable. 
 * @param[in] ubHratio : stands for VIF horizontal down-sample ratio (0~3). 
 * @param[in] ubVratio : stands for VIF vertical down-sample ratio (0~3).
 * @param[in] bHsmooth : stands for VIF horizontal down-sample average enable. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetDownSample(MMP_UBYTE ubVifId, MMP_BOOL bEnable, 
                               MMP_UBYTE ubHratio, MMP_UBYTE ubVratio, MMP_BOOL bHsmooth)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    if (bEnable == MMP_TRUE) {
        pVIF->VIF_DNSPL_RATIO_CTL[ubVifId] |= (ubHratio & VIF_DNSPL_H_RATIO_MASK);
        pVIF->VIF_DNSPL_RATIO_CTL[ubVifId] |= ((ubVratio << 2) & VIF_DNSPL_V_RATIO_MASK);
        
        if(bHsmooth)
            pVIF->VIF_DNSPL_RATIO_CTL[ubVifId] |= VIF_DNSPL_H_AVG_EN;
        else
            pVIF->VIF_DNSPL_RATIO_CTL[ubVifId] &= ~(VIF_DNSPL_H_AVG_EN);     
    }
    else {
        pVIF->VIF_DNSPL_RATIO_CTL[ubVifId] = 0;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_AdjustInputPixel
//  Description : This function adjust indicated component input data value.
//------------------------------------------------------------------------------
/** 
 * @brief This function adjust indicated component input data value.
 * 
 *  This function adjust indicated component input data value.
 * @param[in] ubVifId   : stands for VIF module index. 
 * @param[in] bEnable : stands for VIF adjust input data enable. 
 * @param[in] pOffset : stands for pointer to data offset structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_AdjustInputPixel(MMP_UBYTE ubVifId, MMP_BOOL bEnable, MMPF_VIF_DATA_OFFSET* pOffset)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    if (bEnable == MMP_TRUE) {
    
        pVIF->VIF_COMP_ID00_OFFSET[ubVifId] &= ~(VIF_COMP_OFFSET_SIGN_MASK);
        
        switch(pOffset->ubCompId){
        
            case MMPF_VIF_COLORID_00:
                pVIF->VIF_COMP_ID00_OFFSET[ubVifId] = (pOffset->ubOffVal & VIF_COMP_OFFSET_MASK);
                pVIF->VIF_COMP_ID00_OFFSET[ubVifId] |= ((pOffset->bPostive)?(VIF_COMP_OFFSET_SIGN_POS):(VIF_COMP_OFFSET_SIGN_NEG));
            break;
            case MMPF_VIF_COLORID_01:
                pVIF->VIF_COMP_ID01_OFFSET[ubVifId] = (pOffset->ubOffVal & VIF_COMP_OFFSET_MASK);
                pVIF->VIF_COMP_ID01_OFFSET[ubVifId] |= ((pOffset->bPostive)?(VIF_COMP_OFFSET_SIGN_POS):(VIF_COMP_OFFSET_SIGN_NEG));
            break;
            case MMPF_VIF_COLORID_10:
                pVIF->VIF_COMP_ID10_OFFSET[ubVifId] = (pOffset->ubOffVal & VIF_COMP_OFFSET_MASK);
                pVIF->VIF_COMP_ID10_OFFSET[ubVifId] |= ((pOffset->bPostive)?(VIF_COMP_OFFSET_SIGN_POS):(VIF_COMP_OFFSET_SIGN_NEG));
            break;            
            case MMPF_VIF_COLORID_11:
                pVIF->VIF_COMP_ID11_OFFSET[ubVifId] = (pOffset->ubOffVal & VIF_COMP_OFFSET_MASK);
                pVIF->VIF_COMP_ID11_OFFSET[ubVifId] |= ((pOffset->bPostive)?(VIF_COMP_OFFSET_SIGN_POS):(VIF_COMP_OFFSET_SIGN_NEG));
            break;        
        }
    }
    else{
        switch(pOffset->ubCompId){
        
            case MMPF_VIF_COLORID_00:
                pVIF->VIF_COMP_ID00_OFFSET[ubVifId] = 0;
            break;
            case MMPF_VIF_COLORID_01:
                pVIF->VIF_COMP_ID01_OFFSET[ubVifId] = 0;
            break;
            case MMPF_VIF_COLORID_10:
                pVIF->VIF_COMP_ID10_OFFSET[ubVifId] = 0;
            break;            
            case MMPF_VIF_COLORID_11:
                pVIF->VIF_COMP_ID11_OFFSET[ubVifId] = 0;
            break;        
        }    
    }
    
    return MMP_ERR_NONE;  
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetSensorMClockAttr
//  Description : This function set sensor clock relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set sensor clock relative attribute.
 * 
 *  This function set sensor clock relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to sensor clock attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetSensorMClockAttr(MMP_UBYTE ubVifId, MMPF_VIF_MCLK_ATTR* pAttr)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

    /* Set sensor main clock freqency */
    if (pAttr->ubClkFreqDiv != 0) {
        pVIF->VIF_SENSR_CLK_FREQ[ubVifId] = VIF_SENSR_CLK_PLL_DIV(pAttr->ubClkFreqDiv);
    }
    else {
        pVIF->VIF_SENSR_CLK_FREQ[ubVifId] = VIF_SENSR_CLK_PLL_DIV((pAttr->ulMClkFreq / pAttr->ulDesiredFreq));
    }

    /* Set sensor main clock phase */
    pVIF->VIF_SENSR_MCLK_CTL[ubVifId] &= ~(VIF_SENSR_PHASE_DELAY_MASK);
    
    switch(pAttr->ubClkPhase){
        case MMPF_VIF_SNR_PHASE_DELAY_NONE:
            pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_PHASE_DELAY_NONE;
        break;
        case MMPF_VIF_SNR_PHASE_DELAY_0_5F:
            pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_PHASE_DELAY_0_5F;
        break;
        case MMPF_VIF_SNR_PHASE_DELAY_1_0F:
            pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_PHASE_DELAY_1_0F;
        break;
        case MMPF_VIF_SNR_PHASE_DELAY_1_5F:
            pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_PHASE_DELAY_1_5F;
        break;                
    }
    
    /* Set sensor main clock polarity */
    if (pAttr->ubClkPolarity == MMPF_VIF_SNR_CLK_POLARITY_POS) {
        pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_MCLK_POLAR_PST;
    }
    else {
        pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_MCLK_POLAR_NEG;
    }

    /* Set sensor main clock output enable */
    if (pAttr->bClkOutEn) {
        pVIF->VIF_SENSR_MCLK_CTL[ubVifId] |= VIF_SENSR_MCLK_EN;
    }
    else {
        pVIF->VIF_SENSR_MCLK_CTL[ubVifId] &= ~(VIF_SENSR_MCLK_EN);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetParallelTimingAttr
//  Description : This function set parallel sensor timing relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set parallel sensor timing relative attribute.
 * 
 *  This function set parallel sensor timing relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to parallel sensor timing attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetParallelTimingAttr(MMP_UBYTE ubVifId, MMPF_VIF_PARAL_ATTR* pAttr)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    pVIF->VIF_SENSR_CTL[ubVifId] &= ~(VIF_PCLK_LATCH_MASK | VIF_SYNCSIG_POLAR_MASK);
        
    /* Set sensor pixel clock latch timing */
    if (pAttr->ubLatchTiming == MMPF_VIF_SNR_LATCH_POS_EDGE) {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_PCLK_LATCH_PST;
    }
    else {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_PCLK_LATCH_NEG;
	}
	
    /* Set H-Sync signal polartiy */
    if (pAttr->ubHsyncPolarity == MMPF_VIF_SNR_CLK_POLARITY_POS) {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_HSYNC_POLAR_PST;
    }
    else {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_HSYNC_POLAR_NEG;
	}
	
    /* Set V-Sync signal polartiy */
    if (pAttr->ubVsyncPolarity == MMPF_VIF_SNR_CLK_POLARITY_POS) {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_VSYNC_POLAR_PST;
    }
    else {
        pVIF->VIF_SENSR_CTL[ubVifId] |= VIF_VSYNC_POLAR_NEG;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetSensorMipiAttr
//  Description : This function set sensor MIPI relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set sensor MIPI relative attribute.
 * 
 *  This function set sensor MIPI relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to sensor MIPI attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetSensorMipiAttr(MMP_UBYTE ubVifId, MMPF_MIPI_RX_ATTR* pAttr)
{
    AITPS_MIPI  pMIPI = AITC_BASE_MIPI;
    MMP_UBYTE   i = 0;

    /* Set clock lane delay enable */
    if (pAttr->bClkDelayEn) {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_CLK_DLY_EN;
    }
    else {
        pMIPI->MIPI_CLK_CFG[ubVifId] &= ~(MIPI_CLK_DLY_EN);
    }
        
    /* Set clock lane swap enable */
    if(pAttr->bClkLaneSwapEn) {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_CLK_LANE_SWAP_EN;
    }
    else {
        pMIPI->MIPI_CLK_CFG[ubVifId] &= ~(MIPI_CLK_LANE_SWAP_EN);
    }
    
    /* Set clock lane delay */
    if (pAttr->bClkDelayEn) {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_CLK_DLY(pAttr->usClkDelay);
    }    
    else {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_CLK_DLY(0);
    }
        
    /* Set byte clock latch timing */
    pMIPI->MIPI_CLK_CFG[ubVifId] &= ~(MIPI_BCLK_LATCH_EDGE_MASK);
    
    if (pAttr->ubBClkLatchTiming == MMPF_VIF_SNR_LATCH_POS_EDGE) {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_BCLK_LATCH_EDGE_POS;
    }
    else {
        pMIPI->MIPI_CLK_CFG[ubVifId] |= MIPI_BCLK_LATCH_EDGE_NEG;
    }
    
    /* Set data lane configuration */
    for (i = 0; i< MAX_MIPI_DATA_LANE_NUM; i++)
    {
        if (pAttr->bDataLaneEn[i])
        {
            /* Set data lane enable */
            pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_LANE_EN;

            /* Set data lane delay enable */
            if (pAttr->bDataDelayEn[i]) {
                pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_DLY_EN;
            }
            else {
                pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_DLY_EN);
			}
			
            /* Set data lane swap enable */
            if(pAttr->bDataLaneSwapEn[i]) {
                pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_LANE_SWAP_EN;
            }
            else {
                pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_SWAP_EN);
            }
            
            /* Set data lane D-PHY source */
            pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_SRC_SEL_MASK);
                
            switch(pAttr->ubDataLaneSrc[i])
            {
                case MMPF_VIF_MIPI_DATA_SRC_PHY_0:
                    pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_SRC_PHY_0;
                    break;
                case MMPF_VIF_MIPI_DATA_SRC_PHY_1:
                    pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_SRC_PHY_1;
                    break;
                case MMPF_VIF_MIPI_DATA_SRC_PHY_2:
                    pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_SRC_PHY_2;
                    break;
                case MMPF_VIF_MIPI_DATA_SRC_PHY_3:
                    pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] |= MIPI_DAT_SRC_PHY_3;
                    break;
                default:
                	break;
            }
            
            /* Set data lane delay */
            if (pAttr->bDataDelayEn[i]) {
                pMIPI->DATA_LANE[i].MIPI_DATA_DELAY[ubVifId] |= MIPI_DATA_DLY(pAttr->usDataDelay[i]);
            }
            else {
                pMIPI->DATA_LANE[i].MIPI_DATA_DELAY[ubVifId] &= ~(MIPI_DATA_DLY_MASK);
            }
            
        	/* Set data lane SOT counter */
        	pMIPI->DATA_LANE[i].MIPI_DATA_DELAY[ubVifId] |= MIPI_DATA_SOT_CNT(pAttr->ubDataSotCnt[i]);
        	
        }
        else {    
            pMIPI->DATA_LANE[i].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_EN);
        }

        #if defined(ALL_FW)
        if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
        	(gsHdrCfg.ubMode == HDR_MODE_STAGGER)) {
        	pMIPI->DATA_LANE[i].MIPI_DATA_DELAY[ubVifId] |= MIPI_DATA_RECOVERY;
    	}
    	#endif
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetVirtualChannel
//  Description : This function set virtual channel relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set virtual channel relative attribute.
 * 
 *  This function set virtual channel relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to virtual channel attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetVirtualChannel(MMP_UBYTE ubVifId, MMPF_MIPI_VC_ATTR* pAttr)
{
    AITPS_MIPI 	pMIPI = AITC_BASE_MIPI;
    MMP_UBYTE 	i = 0;
    
    /* Set virtual channel enable */
    if (pAttr->bEnable == MMP_TRUE) {
        pMIPI->MIPI_VC_CTL[ubVifId] |= MIPI_VC_EN;
    }    
    else {
        pMIPI->MIPI_VC_CTL[ubVifId] &= ~(MIPI_VC_EN);
    }
    
    /* Set virtual channel to ISP enable */
    pMIPI->MIPI_VC2ISP_CTL[ubVifId] &= ~(MIPI_VC2ISP_CH_EN_MASK);
    
    for (i = 0; i < MAX_MIPI_VC_NUM; i++) {
    	if (pAttr->bVC2Isp[i]) {
    		pMIPI->MIPI_VC2ISP_CTL[ubVifId] |= MIPI_VC2ISP_CH_EN(i); 
    	}
    }
    
    if (pAttr->bAllChannel2Isp == MMP_TRUE) {
    	pMIPI->MIPI_VC_CTL[ubVifId] |= MIPI_VC2ISP_ALL_CH_EN;
    }
    else {
    	pMIPI->MIPI_VC_CTL[ubVifId] &= ~(MIPI_VC2ISP_ALL_CH_EN);
    }
    
    /* Set virtual channel to Raw enable */
    pMIPI->MIPI_VC_CTL[ubVifId] &= ~(MIPI_VC2RAW_CH_EN_MASK);
    
    for (i = 0; i < MAX_MIPI_VC_NUM; i++) {
    	if (pAttr->bVC2Raw[i]) {
    		pMIPI->MIPI_VC_CTL[ubVifId] |= MIPI_VC2RAW_CH_EN(i); 
    	}
    }
    
    /* Set slow frame start for stagger mode */
    if (pAttr->bSlowFsForStagger == MMP_TRUE) {
    	pMIPI->MIPI_VC2ISP_CTL[ubVifId] |= MIPI_VC_SLOW_FS_FOR_STAGGER_MODE;
    }
    else {
    	pMIPI->MIPI_VC2ISP_CTL[ubVifId] &= ~(MIPI_VC_SLOW_FS_FOR_STAGGER_MODE);
    }
    
    #if (HDR_DBG_RAW_ENABLE)
    pMIPI->MIPI_VC_CTL[ubVifId] = MIPI_VC_EN | MIPI_VC2RAW_CH_EN(0) | MIPI_VC2RAW_CH_EN(1);
    pMIPI->MIPI_VC2ISP_CTL[ubVifId] = MIPI_VC_SLOW_FS_FOR_STAGGER_MODE | MIPI_VC2ISP_CH_EN(0) | MIPI_VC2ISP_CH_EN(1);
    #endif
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetIGBT
//  Description : This function set IGBT relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set IGBT relative attribute.
 * 
 *  This function set IGBT relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to IGBT attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetIGBT(MMP_UBYTE ubVifId, MMPF_VIF_IGBT_ATTR* pAttr)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;
    
    /* Set IBGT enable */
    if (pAttr->bEnable == MMP_TRUE) {
        pVIF->VIF_IGBT_EN[ubVifId] |= VIF_IGBT_OUT_EN;
    }    
    else {
        pVIF->VIF_IGBT_EN[ubVifId] &= ~(VIF_IGBT_OUT_EN);               
    }
    
    /* Set IBGT start line number */
    pVIF->VIF_IGBT_LINE_ST[ubVifId] = pAttr->usStartLine; 
    
    /* Set IBGT start offset cycle number [Unit:16 x VI clock cycle] */
    pVIF->VIF_IGBT_OFST_ST[ubVifId] = pAttr->usStartOfst; 

    /* Set IBGT end cycle number [Unit:16 x VI clock cycle] */
    pVIF->VIF_IGBT_LINE_CYC[ubVifId] = pAttr->usEndCycle; 
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_PowerDownMipiRx
//  Description : This function powers down MIPI Rx
//------------------------------------------------------------------------------
/** 
 * @brief This function powers down MIPI Rx.
 * 
 *  This function powers down MIPI Rx.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_PowerDownMipiRx(void)
{
    AITPS_MIPI pMIPI = AITC_BASE_MIPI;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_VIF, MMP_TRUE);

    pMIPI->DATA_LANE[0].MIPI_DATA_CFG[0] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[1].MIPI_DATA_CFG[0] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[2].MIPI_DATA_CFG[0] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[3].MIPI_DATA_CFG[0] &= ~(MIPI_DAT_LANE_EN);

    pMIPI->DATA_LANE[0].MIPI_DATA_CFG[1] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[1].MIPI_DATA_CFG[1] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[2].MIPI_DATA_CFG[1] &= ~(MIPI_DAT_LANE_EN);
    pMIPI->DATA_LANE[3].MIPI_DATA_CFG[1] &= ~(MIPI_DAT_LANE_EN);

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_VIF, MMP_FALSE);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_SetYUVAttr
//  Description : This function set YUV relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set YUV relative attribute.
 * 
 *  This function set YUV relative attribute.
 * @param[in] ubVifId : stands for VIF module index. 
 * @param[in] pAttr : stands for pointer to YUV attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_SetYUVAttr(MMP_UBYTE ubVifId, MMPF_VIF_YUV_ATTR* pAttr)
{
    AITPS_VIF pVIF = AITC_BASE_VIF;

    if (pAttr->bYuv422ToYuv420) {
    	pVIF->VIF_YUV_CTL[ubVifId]      = 0;
	    pVIF->VIF_YUV420_CTL[ubVifId]   = VIF_PARA_YUV422T0420_EN;
	
	    switch(pAttr->ubYuv422Order){
	    case MMPF_VIF_YUV422_YUYV: 
	        pVIF->VIF_YUV420_CTL[ubVifId] |= VIF_PARA_YUV422_FMT_YUYV;
	    break;
	    case MMPF_VIF_YUV422_YVYU: 
	        pVIF->VIF_YUV420_CTL[ubVifId] |= VIF_PARA_YUV422_FMT_YVYU;
	    break;
	    case MMPF_VIF_YUV422_UYVY: 
	        pVIF->VIF_YUV420_CTL[ubVifId] |= VIF_PARA_YUV422_FMT_UYVY;
	    break;   	    
	    case MMPF_VIF_YUV422_VYUY: 
	        pVIF->VIF_YUV420_CTL[ubVifId] |= VIF_PARA_YUV422_FMT_VYUY;
	    break;
	    }
	
	    m_ubVifToRawStoreFmt[ubVifId]   = MMPF_VIF_TO_RAW_STORE_FMT_YUV420;
	}
	else if (pAttr->bYuv422ToYuv422) {
	
	    if (m_VifSnrType[ubVifId] == MMPF_VIF_SNR_TYPE_YUV) {
    		// For YUV real time and bypass isp path.
    	    pVIF->VIF_YUV_CTL[ubVifId]  = VIF_YUV_EN;
	    }
	    else if (m_VifSnrType[ubVifId] == MMPF_VIF_SNR_TYPE_YUV_TV_DEC) {
	        pVIF->VIF_YUV_CTL[ubVifId]  = 0;
	    }
	    pVIF->VIF_YUV420_CTL[ubVifId]   = 0;
        
	    switch(pAttr->ubYuv422Order){
	    case MMPF_VIF_YUV422_YUYV:
	        pVIF->VIF_YUV_CTL[ubVifId] |= VIF_YUV422_FMT_YUYV;
	    break;
	    case MMPF_VIF_YUV422_YVYU:
	        pVIF->VIF_YUV_CTL[ubVifId] |= VIF_YUV422_FMT_YVYU;
	    break;
	    case MMPF_VIF_YUV422_UYVY:
	        pVIF->VIF_YUV_CTL[ubVifId] |= VIF_YUV422_FMT_UYVY;
	    break;
	    case MMPF_VIF_YUV422_VYUY:
	        pVIF->VIF_YUV_CTL[ubVifId] |= VIF_YUV422_FMT_VYUY;
	    break;
	    }

	    m_ubVifToRawStoreFmt[ubVifId]   = MMPF_VIF_TO_RAW_STORE_FMT_YUV422;
    }
	else if (pAttr->bYuv422ToBayer) {
	    pVIF->VIF_YUV_CTL[ubVifId]      = VIF_YUV_EN | VIF_Y2B_EN | VIF_Y2B_COLR_ID(0, 0);
	    pVIF->VIF_YUV420_CTL[ubVifId]   = 0;

	    m_ubVifToRawStoreFmt[ubVifId]   = MMPF_VIF_TO_RAW_STORE_FMT_BAYER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_VIF_RegisterIntrCallBack
//  Description : This function register VIF interrupt callback function.
//------------------------------------------------------------------------------
/** 
 * @brief This function registers VIF interrupt callback function.
 * 
 *  This function registers VIF interrupt callback function.
 * @param[in] ubVifId   : stands for VIF module index.
 * @param[in] event     : stands for interrupt event.
 * @param[in] pCallBack : stands for interrupt callback function.
 * @param[in] pArgument : stands for argument for the callback function.
 * @return It return the function status.  
 */
MMP_ERR MMPF_VIF_RegisterIntrCallBack(  MMP_UBYTE           ubVifId,
                                        MMPF_VIF_INT_EVENT  event,
                                        VifCallBackFunc     *pCallBack,
                                        void                *pArgument)
{
    if (pCallBack) {
        CallBackFuncVif[ubVifId][event] = pCallBack;
        CallBackArguVif[ubVifId][event] = (void *)pArgument;
    }
    else {
        CallBackFuncVif[ubVifId][event] = NULL;
        CallBackArguVif[ubVifId][event] = NULL;
    }

    return MMP_ERR_NONE;
}

#endif

/** @}*/ //end of MMPF_VIF
