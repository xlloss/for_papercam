/// @ait_only
//==============================================================================
//
//  File        : mmpf_icon.c
//  Description : Retina Icon Module Control driver function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

/**
*  @file mmpf_icon.c
*  @brief The Icon Module Control functions
*  @author Penguin Torng
*  @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmp_reg_icon.h"
#include "mmpf_icon.h"
#include "mmpf_system.h"

/** @addtogroup MMPF_Icon
 *  @{
 */

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

static MMP_STICKER_ATTR m_StickerIconInfo[MMP_STICKER_ID_NUM];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_ResetModule
//  Description : This function reset Icon module.
//------------------------------------------------------------------------------
MMP_ERR MMPF_Icon_ResetModule(MMP_ICO_PIPEID pipeID)
{
	if (pipeID == MMP_ICO_PIPE_0)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_ICON0, MMP_FALSE);
	else if (pipeID == MMP_ICO_PIPE_1)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_ICON1, MMP_FALSE);
	else if (pipeID == MMP_ICO_PIPE_2)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_ICON2, MMP_FALSE);
	else if (pipeID == MMP_ICO_PIPE_3)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_ICON3, MMP_FALSE);
	else if (pipeID == MMP_ICO_PIPE_4)
		MMPF_SYS_ResetHModule(MMPF_SYS_MDL_ICON4, MMP_FALSE);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_GetAttributes
//  Description : This function get icon sticker relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function get icon sticker relative attribute.
 * 
 *  This function get Icon relative attribute.
 * @param[in] ubIconID  : stands for icon sticker module index. 
 * @param[in] ubIconSel : stands for icon sticker type selection.  
 * @param[out] pBufAttr : stands for pointer to Icon attribute structure. 
 * @return It return the function status.  
 */
MMP_ERR MMPF_Icon_GetAttributes(MMP_UBYTE ubIconID, MMP_STICKER_ATTR *pBufAttr)
{
    if (ubIconID < MMP_STICKER_ID_NUM) {
        *pBufAttr = m_StickerIconInfo[ubIconID];
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetAttributes
//  Description : This function set icon sticker relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief This function set icon sticker relative attribute.
 *
 * @param[in] ubIconID  : stands for Icon sticker module index. 
 * @param[in] ubIconSel : stands for Icon sticker type selection.  
 * @param[in] pBufAttr  : stands for pointer to Icon attribute structure. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetAttributes(MMP_UBYTE ubIconID, MMP_STICKER_ATTR *pBufAttr)
{
    AITPS_ICOB  pICONB = AITC_BASE_ICOB;
    MMP_USHORT  shortmp = 0;   

    if (ubIconID < MMP_STICKER_ID_NUM) 
    {
        m_StickerIconInfo[ubIconID] = *pBufAttr;

        pICONB->ICOJ[ubIconID].ICO_ADDR_ST = pBufAttr->ulBaseAddr;
    
        pICONB->ICOJ[ubIconID].ICO_X_ST = pBufAttr->usStartX;
        pICONB->ICOJ[ubIconID].ICO_Y_ST = pBufAttr->usStartY;
        /* Workaround : The sticker End Position needs plus 1 */
        pICONB->ICOJ[ubIconID].ICO_X_ED = pBufAttr->usStartX + pBufAttr->usWidth;
        pICONB->ICOJ[ubIconID].ICO_Y_ED = pBufAttr->usStartY + pBufAttr->usHeight;

        pICONB->ICOJ[ubIconID].ICO_TP_COLR = pBufAttr->ulTpColor;

        if (pBufAttr->bTpEnable == MMP_TRUE) {
            shortmp = ICO_TP_EN;
        }
        else if (pBufAttr->bSemiTpEnable == MMP_TRUE) {
            shortmp = ICO_SEMITP_EN;
            
            pICONB->ICOJ[ubIconID].ICO_SEMI_WT_ICON = pBufAttr->ubIconWeight;
            pICONB->ICOJ[ubIconID].ICO_SEMI_WT_SRC  = pBufAttr->ubDstWeight;
        }

        if (pBufAttr->colorformat == MMP_ICON_COLOR_INDEX8) {
            shortmp |= ICO_FMT_INDEX8;
        }
        else if (pBufAttr->colorformat == MMP_ICON_COLOR_RGB565) {
            shortmp |= ICO_FMT_RGB565;
        }
        else if (pBufAttr->colorformat == MMP_ICON_COLOR_INDEX1) {
            shortmp |= ICO_FMT_INDEX1;
        }
        else if (pBufAttr->colorformat == MMP_ICON_COLOR_INDEX2) {
            shortmp |= ICO_FMT_INDEX2;
        }                
        
        pICONB->ICOJ[ubIconID].ICO_CTL = shortmp;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetSemiTP
//  Description : The function set semi-transparent relative attribute.
//------------------------------------------------------------------------------
/** 
 * @brief The function set semi-transparent relative attribute.
 *
 * @param[in] ubIconID  : stands for icon sticker module index. 
 * @param[in] ubIconSel : stands for icon sticker type selection.  
 * @param[in] bSemiTPEn : stands for enable semi-transparent feature. 
 * @param[in] ulWeight  : stands for semi-transparent weight for sticker. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetSemiTP(MMP_UBYTE ubIconID, MMP_BOOL bSemiTPEn, MMP_ULONG ulWeight)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;
    
    if (ubIconID < MMP_STICKER_ID_NUM) 
    {
        if(bSemiTPEn){
            pICONB->ICOJ[ubIconID].ICO_CTL |= ICO_SEMITP_EN;
            pICONB->ICOJ[ubIconID].ICO_SEMI_WT_ICON = ulWeight;
            pICONB->ICOJ[ubIconID].ICO_SEMI_WT_SRC  = 16 - ulWeight;
        }
        else{
            pICONB->ICOJ[ubIconID].ICO_CTL &= ~ICO_SEMITP_EN;
            pICONB->ICOJ[ubIconID].ICO_SEMI_WT_ICON = 0;
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_GetSemiTP_Weight
//  Description : The function get semi-transparent weight.
//------------------------------------------------------------------------------
/** 
 * @brief The function get semi-transparent weight.
 *
 * @param[in] ubIconID  : stands for icon sticker module index. 
 * @param[in] ubIconSel : stands for icon sticker type selection.  
 * @param[out] usWeight : stands for semi-transparent weight for sticker. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_GetSemiTP_Weight(MMP_UBYTE ubIconID, MMP_USHORT *usWeight)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;
    
    if (ubIconID < MMP_STICKER_ID_NUM) {
        *usWeight = pICONB->ICOJ[ubIconID].ICO_SEMI_WT_ICON;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetTransparent
//  Description : The function set transparent attribute.
//------------------------------------------------------------------------------
/** 
 * @brief The function set transparent attribute.
 *
 * @param[in] ubIconID      : stands for icon sticker module index. 
 * @param[in] ubIconSel     : stands for icon sticker type selection.  
 * @param[in] bTranspActive : stands for enable transparent feature.  
 * @param[in] ulTranspColor : stands for transparent key color. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetTransparent(MMP_UBYTE ubIconID, MMP_BOOL bTranspActive, MMP_ULONG ulTranspColor)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;

    if (ubIconID < MMP_STICKER_ID_NUM) 
    {
        if (bTranspActive) {
            pICONB->ICOJ[ubIconID].ICO_CTL |= ICO_TP_EN;
            pICONB->ICOJ[ubIconID].ICO_TP_COLR = ulTranspColor;
        }
        else {
            pICONB->ICOJ[ubIconID].ICO_CTL &= ~(ICO_TP_EN);
            pICONB->ICOJ[ubIconID].ICO_TP_COLR = 0;        
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetEnable
//  Description : The function enable the icon sticker.
//------------------------------------------------------------------------------
/** 
 * @brief The function enable the Icon sticker.
 *
 * @param[in] ubIconID  : stands for icon sticker module index. 
 * @param[in] ubIconSel : stands for icon sticker type selection.  
 * @param[in] bEnable   : stands for enable Icon sticker. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetEnable(MMP_UBYTE ubIconID, MMP_BOOL bEnable)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;

    if (ubIconID < MMP_STICKER_ID_NUM) {
        if (bEnable) {
            pICONB->ICOJ[ubIconID].ICO_CTL |= ICO_STICKER_EN;
        }
        else {
            pICONB->ICOJ[ubIconID].ICO_CTL &= ~(ICO_STICKER_EN);
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_LoadIndexColorTable
//  Description : The function load the index color table.
//------------------------------------------------------------------------------
/** 
 * @brief The function load the index color table..
 *
 * @param[in] pLUT  	 : stands for the pointer to index color table. 
 * @param[in] usColorNum : stands for the index color number.   
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_LoadIndexColorTable(MMP_UBYTE ubIconID, MMP_ICON_COLOR ubColor, 
									  MMP_USHORT* pLUT, MMP_USHORT usColorNum)
{
	AITPS_ICON_LUT  pIconLUT = AITC_BASE_ICON_LUT;
	AITPS_ICOB  	pICONB 	 = AITC_BASE_ICOB;
	MMP_ULONG		i = 0;
	
	if (ubColor != MMP_ICON_COLOR_INDEX8 && 
		ubColor != MMP_ICON_COLOR_INDEX1 &&
		ubColor != MMP_ICON_COLOR_INDEX2) {
		return MMP_ICON_ERR_PARAMETER;
	}	
	
	if (ubColor == MMP_ICON_COLOR_INDEX8)
	{	
		if (usColorNum > 256)
			return MMP_ICON_ERR_PARAMETER;
	
		for(i = 0; i< usColorNum; i++) {
        	pIconLUT->INDEX_TBL[i] = *(pLUT + i); 
    	}
    }
    else
    {
		if (usColorNum > 4)
			return MMP_ICON_ERR_PARAMETER;

		for(i = 0; i< usColorNum; i++) {
        	pICONB->ICOJ[ubIconID].ICO_IDX_COLOR[i] = *(pLUT + i); 
    	}    
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetDLAttributes
//  Description : The function set delay line attribute.
//------------------------------------------------------------------------------
/** 
 * @brief The function set delay line attribute.
 *
 * @param[in] pipeID    : stands for icon pipe index. 
 * @param[in] pPipeAttr : stands for icon delay line attribute.
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetDLAttributes(MMP_ICO_PIPEID pipeID, MMP_ICO_PIPE_ATTR *pPipeAttr)
{
    AITPS_ICOB  pICONB = AITC_BASE_ICOB;

	// Always use delay line
	if (pipeID <= MMP_ICO_PIPE_3) {
    	pICONB->ICO_DLINE_CFG[pipeID] = ICO_USE_DLINE | ICO_DLINE_SRC_SEL(pPipeAttr->inputsel);
	}
	else {
		pICONB->ICO_P4_DLINE_CFG = ICO_USE_DLINE | ICO_DLINE_SRC_SEL(pPipeAttr->inputsel);
	}
	
    pICONB->ICO_DLINE_FRM_WIDTH[pipeID] = pPipeAttr->usFrmWidth;

   	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_SetDLEnable
//  Description : The function enalbe icon delay line.
//------------------------------------------------------------------------------
/** 
 * @brief The function enalbe icon delay line.
 *
 * @param[in] pipeID  : stands for icon pipe index. 
 * @param[in] bEnable : stands for enable icon delay line.   
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_SetDLEnable(MMP_ICO_PIPEID pipeID, MMP_BOOL bEnable)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;
    
    if (bEnable) {
    	if (pipeID <= MMP_ICO_PIPE_3) {
        	pICONB->ICO_DLINE_CFG[pipeID] &= ~(ICO_DIS_DLINE); 
    	}
    	else {
    		pICONB->ICO_P4_DLINE_CFG &= ~(ICO_DIS_DLINE); 
    	}
    }
    else {
    	if (pipeID <= MMP_ICO_PIPE_3) {
        	pICONB->ICO_DLINE_CFG[pipeID] |= ICO_DIS_DLINE; 
    	}
    	else {
    		pICONB->ICO_P4_DLINE_CFG |= ICO_DIS_DLINE;
    	}
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_GetDLMaxUsage
//  Description : The function get icon delay line max usage.
//------------------------------------------------------------------------------
/** 
 * @brief The function get icon delay line max usage.
 *
 * @param[in] pipeID  : stands for icon pipe index.    
 * @return It return the function status. 
 */
MMP_ULONG MMPF_Icon_GetDLMaxUsage(MMP_ICO_PIPEID pipeID)
{
    AITPS_ICOB pICONB = AITC_BASE_ICOB;
    
	if (pipeID <= MMP_ICO_PIPE_3) {
    	return pICONB->ICO_DLINE_MAX_USE[pipeID]; 
	}
	else {
		return pICONB->ICO_P4_DLINE_MAX_USE;
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_GetDLMaxLimit
//  Description : The function get icon delay line max limitation.
//------------------------------------------------------------------------------
/** 
 * @brief The function get icon delay line max limitation.
 *
 * @param[in] pipeID  : stands for icon pipe index.
 * @return It return the function status. 
 */
MMP_ULONG MMPF_Icon_GetDLMaxLimit(MMP_ICO_PIPEID pipeID)
{
	if ((pipeID == MMP_ICO_PIPE_0) || 
        (pipeID == MMP_ICO_PIPE_1) || 
        (pipeID == MMP_ICO_PIPE_3)) {
    	return 0x7FF; // Define by register spec. 
	}
	else {
		return 0x3FF; // Define by register spec. 
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Icon_InitLinkSrc
//  Description :
//------------------------------------------------------------------------------
/** 
 * @brief The function initial icon delay line link source.
 *   
 * @return It return the function status. 
 */
MMP_ERR MMPF_Icon_InitLinkSrc(void)
{
    MMP_ICO_PIPE_ATTR   IconAttr;
    MMP_UBYTE           i = 0;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_TRUE);
	for (i = MMP_ICO_PIPE_0; i < MMP_ICO_PIPE_NUM; i++) {
        IconAttr.inputsel 	= (MMP_SCAL_PIPEID)i;
        IconAttr.bDlineEn 	= MMP_TRUE;
        IconAttr.usFrmWidth	= 1280;
	    MMPF_Icon_SetDLAttributes(i, &IconAttr);
    }
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ICON, MMP_FALSE);

    return MMP_ERR_NONE;
}

/// @}
/// @end_ait_only

