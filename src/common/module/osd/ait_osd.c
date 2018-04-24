/**
 @file ait_osd.c
 @brief OSD general control Function
 @author 
 @version 1.0
*/
/*===========================================================================
 * Include files
 *===========================================================================*/ 

#include "includes_fw.h"
#include "ait_osd.h"

#if (SUPPORT_OSD_FUNC)

/*===========================================================================
 * Macro define
 *===========================================================================*/ 


/*===========================================================================
 * Static varible
 *===========================================================================*/ 
static const MMP_UBYTE  DaysPerMonth[12] = {
    31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

//static MMP_USHORT m_usOsdStartXOffset = 0, m_usOsdStartYoffset = 0;

//static MMP_UBYTE  m_usDateFormat = AIT_OSD_DATE_FORMAT_1 ;
//static MMP_UBYTE  m_usTimeFormat = AIT_OSD_TIME_FORMAT_2 ;

//static MMP_UBYTE  gbYColor = 225,gbUColor = 16 ,gbVColor = 148 ,gbAlpha = 100 ;
//static MMP_UBYTE  gbYColor = 52,gbUColor = 242 ,gbVColor = 147 ,gbAlpha = 100 ;


/*===========================================================================
 * Prototype 
 *===========================================================================*/ 

/*===========================================================================
 * Main body
 *===========================================================================*/ 

void _____OSD_Function_________(void){return;} //dummy

#if (ITCM_PUT)
void ait_osd_Init (AIT_OSD_HANDLE *pOsd) ITCMFUNC;
void ait_osd_GetFontPixelSize(MMP_USHORT *HSize, MMP_USHORT *VSize) ITCMFUNC;
void ait_osd_SetColor(AIT_OSD_HANDLE *pOsd, MMP_UBYTE yColor,MMP_UBYTE uColor,MMP_UBYTE vColor) ITCMFUNC;
void ait_osd_SetPos(AIT_OSD_HANDLE *pOsd, MMP_USHORT xPos,MMP_USHORT yPos) ITCMFUNC;
void ait_osd_SetFDWidthHeight(AIT_OSD_HANDLE *pOsd, MMP_USHORT width,MMP_USHORT height) ITCMFUNC;
void ait_osd_DrawColor(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, MMP_USHORT w,MMP_USHORT h) ITCMFUNC;

void ait_osd_DrawStr(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen) ITCMFUNC;
void ait_osd_DrawWatermark(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf) ITCMFUNC;
void ait_osd_DrawFDFrame(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen) ITCMFUNC;
MMP_ERR ait_rtc_SetClock(AIT_RTC_TIME *pRtcTime, MMP_USHORT pwYear, MMP_USHORT pwMonth,
        MMP_USHORT pwDay, MMP_USHORT pwHour,MMP_USHORT pwMinute, MMP_USHORT pwSecond, MMP_BOOL bOn) ITCMFUNC;

MMP_ERR ait_rtc_ConvertDateTime2String(AIT_RTC_TIME* psRtcTime, MMP_BYTE* pdDateTime) ITCMFUNC;
static void ait_rtc_Int2Str(MMP_ULONG value, MMP_BYTE *string) ITCMFUNC;
MMP_ERR ait_rtc_UpdateTime(AIT_RTC_TIME *pRtcTime, MMP_ULONG nSec) ITCMFUNC;
MMP_ERR ait_rtc_SetDateTimeFormat(AIT_RTC_TIME *pRtcTime, AIT_OSD_DATE_FORMAT_CONFIG DateFormatConfig,
            AIT_OSD_TIME_FORMAT_CONFIG TimeFormatConfig) ITCMFUNC;
#endif

MMP_ULONG osdYMap[16] = {
		0x00000000, 0xff000000, 0x00ff0000, 0xffff0000,
		0x0000ff00, 0xff00ff00, 0x00ffff00, 0xffffff00,
		0x000000ff, 0xff0000ff, 0x00ff00ff, 0xffff00ff,
		0x0000ffff, 0xff00ffff, 0x00ffffff, 0xffffffff
};
void ait_osd_genYUVMatrix(AIT_OSD_HANDLE *pOsd);//chrison
void ait_osd_Init (AIT_OSD_HANDLE *pOsd)
{
    pOsd->FgColor[COLR_Y] = 225;
    pOsd->FgColor[COLR_U] = 16;
    pOsd->FgColor[COLR_V] = 148;
    pOsd->FgColor[COLR_A] = 100;
    pOsd->usStartXOffset = 0;
    pOsd->usStartYOffset = 0;
    pOsd->usWidth= 0;
    pOsd->usHeight= 0;
}

extern AIT_OSD_DOUBLEBUF desc_osd_rtc_db;
extern AIT_OSD_FONT desc_osd_font;
void ait_osd_GetFontPixelSize(MMP_USHORT *HSize, MMP_USHORT *VSize)
{
    *HSize = desc_osd_font.font_w;
    *VSize = desc_osd_font.font_h;
}

void ait_osd_SetColor(AIT_OSD_HANDLE *pOsd, MMP_UBYTE yColor,MMP_UBYTE uColor,MMP_UBYTE vColor)
{
    pOsd->FgColor[COLR_Y] = yColor;
    pOsd->FgColor[COLR_U] = uColor;
    pOsd->FgColor[COLR_V] = vColor;
}

void ait_osd_SetPos(AIT_OSD_HANDLE *pOsd, MMP_USHORT xPos,MMP_USHORT yPos)
{
    pOsd->usStartXOffset = (xPos & ~0x01);
    pOsd->usStartYOffset = (yPos & ~0x01);
}

void ait_osd_SetFDWidthHeight(AIT_OSD_HANDLE *pOsd, MMP_USHORT width,MMP_USHORT height)
{
    pOsd->usWidth = (width & ~0x01);
    pOsd->usHeight = (height & ~0x01);
}

#if AIT_COLOR_EN
void ait_osd_DrawColor(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, MMP_USHORT w,MMP_USHORT h)
{
    MMP_UBYTE   *pLineSt, *pLineEd;
    MMP_USHORT  CbrX;

    if ((w == 0) || (h == 0) || (pOsd->usStartXOffset + w > pBuf->ulBufWidth)
            || (pOsd->usStartYOffset + h > pBuf->ulBufHeight))
    {
        return;
    }

    pLineSt = (MMP_UBYTE *)(pBuf->ulAddr[COLR_Y] + pOsd->usStartYOffset * pBuf->ulBufWidth
                + pOsd->usStartXOffset);
    pLineEd = pLineSt + h * pBuf->ulBufWidth;

    for ( ; pLineSt < pLineEd; pLineSt += pBuf->ulBufWidth) {
        MEMSET(pLineSt, pOsd->FgColor[COLR_Y], w);
    }

    pLineSt = (MMP_UBYTE *)(pBuf->ulAddr[COLR_U] + (pOsd->usStartYOffset >> 1) * pBuf->ulBufWidth
                + (pOsd->usStartXOffset & ~0x1));
    pLineEd = pLineSt + (h >> 1) * pBuf->ulBufWidth;

    for ( ; pLineSt < pLineEd; pLineSt += pBuf->ulBufWidth) {
        for (CbrX = 0; CbrX < w; ) {
            pLineSt[CbrX++] = pOsd->FgColor[COLR_U];
            pLineSt[CbrX++] = pOsd->FgColor[COLR_V];
        }
    }
}
#endif

#if AIT_STRING_EN
void ait_osd_DrawFontChar(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char pChar, MMP_SHORT index)
{

    MMP_ULONG       *CurYPtr, *CurUVPtr;
    MMP_USHORT      h, w ,i , nWinHPxls, nWinVPxls;//, nBlackBarXEnd;
    MMP_USHORT    	OSDFontW, OSDFontH, OSDPadding;
    MMP_UBYTE      	*CurFontTable = 0;

	
	MMP_UBYTE 		*Value;
	MMP_ULONG		YCOLOR,UCOLOR,VCOLOR,UVCOLOR,pLine;
	MMP_ULONG		pos_off;
	MMP_UBYTE		*pix;

	GUI_CHARINFO *CurFontMap;
	MMP_USHORT      font_off,char_off;

	if ((pOsd->usStartXOffset > pBuf->ulBufWidth)
		|| (pOsd->usStartYOffset > pBuf->ulBufHeight))
	{
	    return;
	}

    OSDFontW = desc_osd_font.font_w;
    OSDFontH = desc_osd_font.font_h;
	OSDPadding = desc_osd_font.padding; 

    nWinHPxls = OSDFontW;
    nWinVPxls = OSDFontH;
//	printc("#nWinHPxls = %d\r\n",nWinHPxls);
	pos_off = index*(nWinHPxls)+OSDPadding;

    if ((pOsd->usStartXOffset + nWinHPxls) > pBuf->ulBufWidth) {
        nWinHPxls = pBuf->ulBufWidth - pOsd->usStartXOffset;
    }
    if ((pOsd->usStartYOffset + nWinVPxls) > pBuf->ulBufHeight) {
        nWinVPxls = pBuf->ulBufHeight - pOsd->usStartYOffset;
    }

    CurYPtr = (MMP_ULONG *)(pBuf->ulAddr[COLR_Y] + pOsd->usStartYOffset * pBuf->ulBufWidth + pOsd->usStartXOffset + pos_off);
    CurUVPtr = (MMP_ULONG *)(pBuf->ulAddr[COLR_U] + (pOsd->usStartYOffset >>1) * pBuf->ulBufWidth + pOsd->usStartXOffset + pos_off);

	CurFontMap = desc_osd_font.fontmap;
	YCOLOR = (pOsd->FgColor[COLR_Y]<<24)|(pOsd->FgColor[COLR_Y]<<16)
				|(pOsd->FgColor[COLR_Y]<<8)|(pOsd->FgColor[COLR_Y]);

	UVCOLOR = (pOsd->FgColor[COLR_V]<<24)|(pOsd->FgColor[COLR_U]<<16)
				|(pOsd->FgColor[COLR_V]<<8)|(pOsd->FgColor[COLR_U]);
	
	pLine = CurFontMap[font_off].BytesPerLine;
	font_off = 0;
	//Y
	char_off = (unsigned char)(pChar - ' ');
	for (h = 0; h < nWinVPxls; h++) {
		Value = (MMP_UBYTE*)(CurFontMap[char_off].pData + h*pLine);
		for (w = 0; w < pLine; w++) {
			*(CurYPtr + w*2) = osdYMap[(*(Value+w) >> 4)&0x0f]&YCOLOR;
			*(CurYPtr + w*2 +1) = osdYMap[(*(Value+w) >> 0)&0x0f]&YCOLOR;
			
			if ((h%2)==0) {
				*(CurUVPtr + w*2) = UVCOLOR;
				*(CurUVPtr +  w*2 +1) = UVCOLOR;
			}
			
		}
		CurYPtr += (pBuf->ulBufWidth/sizeof(MMP_ULONG)); 
		if((h%2)==0)
			CurUVPtr += pBuf->ulBufWidth/sizeof(MMP_ULONG); 
	}
}

void ait_osd_DrawFontStr(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen)
{
	MMP_UBYTE 		idx;
	for (idx = 0;idx < nStrLen; idx++)	
		ait_osd_DrawFontChar(pOsd,pBuf, pStr[idx], idx);
}

#endif

#if (AIT_WATERMARK_EN)            
void ait_osd_DrawWatermark(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf)
{
    //extern unsigned char WatermarkTable128x48[];
    extern unsigned char WatermarkTable160x64[];
    extern unsigned char WatermarkTable80x32[];

    MMP_UBYTE       Value, *CurYPtr, *CurUVPtr;
    MMP_USHORT      h, w, nWinHPxls, nWinVPxls;//, nBlackBarXEnd;
    MMP_USHORT    OSDFontW, OSDFontH;
    MMP_UBYTE      *CurFontTable = 0;
    MMP_USHORT   StartXOffset, StartYOffset;
    MMP_UBYTE      FgColorY, FgColorU, FgColorV;

    if((pBuf->ulBufWidth > 640) || (pBuf->ulBufHeight > 480)){
        OSDFontW = 160;
        OSDFontH = 64;

        StartXOffset = pBuf->ulBufWidth - OSDFontW - 16;
        StartYOffset = pBuf->ulBufHeight - OSDFontH - 16;

        CurFontTable = &WatermarkTable160x64[0];
    }
    else{
        OSDFontW = 80;
        OSDFontH = 32;

        StartXOffset = pBuf->ulBufWidth - OSDFontW - 16;
        StartYOffset = pBuf->ulBufHeight - OSDFontH - 16;

        CurFontTable = &WatermarkTable80x32[0];
    }
	
    FgColorY = 102;
    FgColorU = 126;
    FgColorV = 127;
	
    nWinHPxls = OSDFontW;
    nWinVPxls = OSDFontH;

    CurYPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_Y] + StartYOffset * pBuf->ulBufWidth + StartXOffset);
    CurUVPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_U] + (StartYOffset >>1) * pBuf->ulBufWidth + StartXOffset);

	
        for (h = 0; h < nWinVPxls; h++) {
            MMP_USHORT      font_off, font_addr_base, font_addr, font_w_off = 0 ;
            for (w = 0; w < nWinHPxls; w++) {
                font_off = 0;
                font_addr_base= (unsigned short)(font_off *  (((OSDFontW+7) >> 3) * nWinVPxls) );
                font_addr = font_addr_base + h * ((OSDFontW+7) >> 3 ) + (font_w_off >> 3);
                Value = CurFontTable[font_addr];
                if (Value & (1 << (((w%OSDFontW) & 7)))) {
                    *(CurYPtr + w) =  FgColorY;						
                    if ((!(w&1)) && (!(h&1)))
                    {
                        *(CurUVPtr + w) = (MMP_UBYTE)(FgColorU);
                        *(CurUVPtr + w + 1) = (MMP_UBYTE)(FgColorV);
                    }
                }
          #if BLACK_Y > 0 
		  else{
                    *(CurYPtr + w) =  BLACK_Y;  // fill background color						
                    if((!(w&1)) && (!(h&1))) 
		      {
                        *(CurUVPtr + w) = BLACK_U;
                        *(CurUVPtr + w + 1) = BLACK_V;
                    }
		  }
          #endif
                if (font_w_off >= (OSDFontW-1)) {
                    font_w_off = 0 ;
                }
                else {
                    font_w_off++ ;
                }
            }
            CurYPtr += pBuf->ulBufWidth; 
            if((h&1)==0){
                CurUVPtr += pBuf->ulBufWidth; 
            }
        }
		
}
#endif


#if (AIT_FDFRAME_EN)            
void ait_osd_DrawFDFrame(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen)
{
    extern unsigned char FDFrameUpTable32x32[];
    extern unsigned char FDFrameDownTable32x32[];
    extern unsigned char FontTable20x32[];

    MMP_UBYTE       Value, *CurYPtr, *CurUVPtr;
    MMP_USHORT      h, w, nWinHPxls, nWinVPxls;//, nBlackBarXEnd;
    MMP_USHORT    OSDFontW, OSDFontH;
    MMP_UBYTE      *CurFontTable = 0;
    MMP_USHORT   StartXOffset, StartYOffset;
    MMP_UBYTE      FgColorY, FgColorU, FgColorV;
    MMP_USHORT    i;

    if ((pOsd->usStartXOffset > pBuf->ulBufWidth) || (pOsd->usStartYOffset > pBuf->ulBufHeight))
    {
        return;
    } 

    if((pBuf->ulBufWidth > 640) || (pBuf->ulBufHeight > 480)){
        OSDFontW = 32;
        OSDFontH = 32;
    }
    else{
        OSDFontW = 32;
        OSDFontH = 32;
    }
	/*
    FgColorY = 76;
    FgColorU = 84;
    FgColorV = 255;
  */
    FgColorY = pOsd->FgColor[0] ;
    FgColorU = pOsd->FgColor[1] ;
    FgColorV = pOsd->FgColor[2] ;
    
    
    // Draw FD Up frame and Down frame	
    for(i = 0 ; i < 2 ; i++){
        nWinHPxls = OSDFontW;
        nWinVPxls = OSDFontH;

	 if( i == 0){
            StartXOffset = pOsd->usStartXOffset;
            StartYOffset = pOsd->usStartYOffset;

           CurFontTable = &FDFrameUpTable32x32[0];
        }
	 else{
            StartXOffset = pOsd->usStartXOffset + pOsd->usWidth - OSDFontW;
            StartYOffset = pOsd->usStartYOffset + pOsd->usHeight -OSDFontH;

            CurFontTable = &FDFrameDownTable32x32[0];
 	}

        if ((StartXOffset + nWinHPxls) > pBuf->ulBufWidth) {
	     if(StartXOffset < pBuf->ulBufWidth)
                nWinHPxls = pBuf->ulBufWidth - StartXOffset;
	    else
		  nWinHPxls = 0;	
        }
        if ((StartYOffset + nWinVPxls) > pBuf->ulBufHeight) {
	     if(StartYOffset < pBuf->ulBufHeight)
                nWinVPxls = pBuf->ulBufHeight - StartYOffset;
	    else
		  nWinVPxls = 0;	
        }

        CurYPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_Y] + StartYOffset * pBuf->ulBufWidth + StartXOffset);
        CurUVPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_U] + (StartYOffset >>1) * pBuf->ulBufWidth + StartXOffset);
	
        for (h = 0; h < nWinVPxls; h++) {
            MMP_USHORT      font_off, font_addr_base, font_addr, font_w_off = 0 ;
            for (w = 0; w < nWinHPxls; w++) {
                font_off = 0;
                font_addr_base= (unsigned short)(font_off *  (((OSDFontW+7) >> 3) * nWinVPxls) );
                font_addr = font_addr_base + h * ((OSDFontW+7) >> 3 ) + (font_w_off >> 3);
                Value = CurFontTable[font_addr];
                if (Value & (1 << (((w%OSDFontW) & 7)))) {
                    *(CurYPtr + w) =  FgColorY;						
                    if ((!(w&1)) && (!(h&1)))
                    {
                        *(CurUVPtr + w) = (MMP_UBYTE)(FgColorU);
                        *(CurUVPtr + w + 1) = (MMP_UBYTE)(FgColorV);
                    }
                }
          #if BLACK_Y > 0 
		  else{
                    *(CurYPtr + w) =  BLACK_Y;  // fill background color						
                    if((!(w&1)) && (!(h&1))) 
		      {
                        *(CurUVPtr + w) = BLACK_U;
                        *(CurUVPtr + w + 1) = BLACK_V;
                    }
		  }
          #endif
                if (font_w_off >= (OSDFontW-1)) {
                    font_w_off = 0 ;
                }
                else {
                    font_w_off++ ;
                }
            }
            CurYPtr += pBuf->ulBufWidth; 
            if((h&1)==0){
                CurUVPtr += pBuf->ulBufWidth; 
            }
        }
    }

    if ((nStrLen) && (pStr))
    {
        OSDFontW = 20;
        OSDFontH = 32;

        nWinHPxls = OSDFontW * nStrLen;
        nWinVPxls = OSDFontH;

        StartXOffset = pOsd->usStartXOffset + 48;
        StartYOffset = pOsd->usStartYOffset;


        if ((StartXOffset + nWinHPxls) > pBuf->ulBufWidth) {
            nWinHPxls = pBuf->ulBufWidth - StartXOffset;
        }
        if ((StartYOffset + nWinVPxls) > pBuf->ulBufHeight) {
            nWinVPxls = pBuf->ulBufHeight - StartYOffset;
        }

        CurYPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_Y] + StartYOffset * pBuf->ulBufWidth + StartXOffset);
        CurUVPtr = (MMP_UBYTE *)(pBuf->ulAddr[COLR_U] + (StartYOffset >>1) * pBuf->ulBufWidth + StartXOffset);

        CurFontTable = &FontTable20x32[0];
	
        for (h = 0; h < nWinVPxls; h++) {
            MMP_USHORT      font_off, font_addr_base, font_addr, font_w_off = 0 ;
            for (w = 0; w < nWinHPxls; w++) {
                font_off = (unsigned char)(pStr[(w / OSDFontW)] - ' ') ;
                font_addr_base= (unsigned short)(font_off *  (((OSDFontW+7) >> 3) * nWinVPxls) );
                font_addr = font_addr_base + h * ((OSDFontW+7) >> 3 ) + (font_w_off >> 3);
                Value = CurFontTable[font_addr];
                if (Value & (1 << (((w%OSDFontW) & 7)))) {
                    *(CurYPtr + w) =  FgColorY;						
                    if ((!(w&1)) && (!(h&1)))
                    {
                        *(CurUVPtr + w) = (MMP_UBYTE)(FgColorU);
                        *(CurUVPtr + w + 1) = (MMP_UBYTE)(FgColorV);
                    }
                }
          #if BLACK_Y > 0 
		  else{
                    *(CurYPtr + w) =  BLACK_Y;  // fill background color						
                    if((!(w&1)) && (!(h&1))) 
		      {
                        *(CurUVPtr + w) = BLACK_U;
                        *(CurUVPtr + w + 1) = BLACK_V;
                    }
		  }
          #endif
                if (font_w_off >= (OSDFontW-1)) {
                    font_w_off = 0 ;
                }
                else {
                    font_w_off++ ;
                }
            }
            CurYPtr += pBuf->ulBufWidth; 
            if((h&1)==0){
                CurUVPtr += pBuf->ulBufWidth; 
            }
        }


    }
		
}
#endif

void _____RTC_Function_________(void){return;} //dummy
#if AIT_RTC_EN
/**
 @brief Set RTC

 Set RTC value.
 Parameters:
 @param[in] pwYear Set year
 @param[in] pwMonth Set Month
 @param[in] pwDay Set Day
 @param[in] pwHour Set Hour
 @param[in] pwMinute Set Minute
 @param[in] pwSecond Set Second
 @retval MMP_ERR_NONE or PCAM_SYS_ERR. // MMP_ERR_NONE: Success, PCAM_SYS_ERR: Fail
*/
MMP_ERR ait_rtc_SetClock(AIT_RTC_TIME *pRtcTime, MMP_USHORT pwYear, MMP_USHORT pwMonth,
            MMP_USHORT pwDay, MMP_USHORT pwHour,MMP_USHORT pwMinute, MMP_USHORT pwSecond, MMP_BOOL bOn)
{
    if (bOn) {
        pRtcTime->uwSecond = pwSecond;
        pRtcTime->uwMinute = pwMinute;
        pRtcTime->uwHour = pwHour;
        pRtcTime->uwDay = pwDay;
        pRtcTime->uwMonth = pwMonth;
        pRtcTime->uwYear = pwYear;

        //pRtcTime->byDateCfg = AIT_OSD_DATE_FORMAT_1;
        //pRtcTime->byTimeCfg = AIT_OSD_TIME_FORMAT_1;

        pRtcTime->bOn = MMP_TRUE;
    } else {
        pRtcTime->bOn = MMP_FALSE;
    }

    return MMP_ERR_NONE;
}

/**
@brief Set thr presentation string format of data/time

 @param[in] pRtcTime RTC handler
 @param[in] DateFormatConfig format in
 @param[in] TimeFormatConfig
 @retval MMP_ERR_NONE
*/
MMP_ERR ait_rtc_SetDateTimeFormat(AIT_RTC_TIME *pRtcTime, AIT_OSD_DATE_FORMAT_CONFIG DateFormatConfig,
            AIT_OSD_TIME_FORMAT_CONFIG TimeFormatConfig)
{
    if((DateFormatConfig > 0) && (DateFormatConfig < AIT_OSD_DATE_FORMAT_MAX) && 
		(TimeFormatConfig > 0) && (TimeFormatConfig < AIT_OSD_TIME_FORMAT_MAX)){
        pRtcTime->byDateCfg = DateFormatConfig;
        pRtcTime->byTimeCfg = TimeFormatConfig;
    }
    else{
#if (CUSTOMER == SAL)
        pRtcTime->byDateCfg = AIT_OSD_DATE_FORMAT_2;
        pRtcTime->byTimeCfg = AIT_OSD_TIME_FORMAT_1;
#else
        pRtcTime->byDateCfg = AIT_OSD_DATE_FORMAT_1;
        pRtcTime->byTimeCfg = AIT_OSD_TIME_FORMAT_1;
#endif		
    }

    //printk(KERN_ERR "ait_rtc_SetDateTimeFormat: (%d, %d)\n", pRtcTime->byDateCfg, pRtcTime->byTimeCfg);

    return MMP_ERR_NONE;
}

/**
@brief The intialize code that should be called once after system power-up

 The intialize code that should be called once after system power-up
 Parameters:
 @retval MMP_ERR_NONE or PCAM_SYS_ERR. // MMP_ERR_NONE: Success, PCAM_SYS_ERR: Fail
*/
MMP_ERR ait_rtc_UpdateTime(AIT_RTC_TIME *pRtcTime, MMP_ULONG nSec)
{
    if (!pRtcTime->bOn) {
        return MMP_SYSTEM_ERR_PARAMETER;
    }

    for (; nSec; nSec--) {
        pRtcTime->uwSecond++;
        if(pRtcTime->uwSecond > 59){
            pRtcTime->uwSecond = 0;
            pRtcTime->uwMinute++;
            if(pRtcTime->uwMinute > 59){
                pRtcTime->uwMinute = 0;
                pRtcTime->uwHour++;
                if(pRtcTime->uwHour > 23) {
                    MMP_UBYTE nDays = 31;

                    pRtcTime->uwHour = 0;
                    pRtcTime->uwDay++;

                    #if 1 //leap year handle
                    if (pRtcTime->uwMonth == 2) {
                        MMP_USHORT yr = (pRtcTime->uwYear % 400);
                        /* if year is 400x : leap year
                         * else if year is 4x and not 100x : leap year
                         * else : average year
                         */
                        if ((yr & 3) || (yr == 100) || (yr == 200) || (yr == 300)) {
                            nDays = 28;
                        } else {
                            nDays = 29;
                        }
                    } else {
                        nDays = DaysPerMonth[pRtcTime->uwMonth-1];
                    }
                    #endif

                    if(pRtcTime->uwDay > nDays) {
                        pRtcTime->uwDay = 1;
                        pRtcTime->uwMonth++;
                        if(pRtcTime->uwMonth > 12){
                            pRtcTime->uwMonth = 1;
                            pRtcTime->uwYear++;
                        }
                    }
                }
            }
        }
    }

	return MMP_ERR_NONE;
}

extern unsigned divu10(unsigned n);
static void ait_rtc_Int2Str(MMP_ULONG value, MMP_BYTE *string)
{
    MMP_ULONG   i,j;
    MMP_BYTE    temp[AIT_RTC_COMPONENT_MAX_FIELD];

    for (i = 0; i < AIT_RTC_COMPONENT_MAX_FIELD; i++) {
        j = divu10(value);
        if ( j != 0 ) {
            temp[i] = '0' + (value - j*10);//tbd value % 10;

            value = j;
        }
        else {
            temp[i] = '0' + (value - j*10);//tbd value % 10;
            break;
        }
    }

    for(j = 0; j < (i+1); j++) {
        string[j] = temp[i - j];
    }

    string[j] = 0;
}


/**
@brief  transfer psRtcTime(int) to pdDateTime(string)
*/
MMP_ERR ait_rtc_ConvertDateTime2String(AIT_RTC_TIME* psRtcTime, MMP_BYTE* pdDateTime)
{
    AIT_TIME_STRING sTimeString;

    ait_rtc_Int2Str(psRtcTime->uwYear   , sTimeString.byYear);
    ait_rtc_Int2Str(psRtcTime->uwMonth  , sTimeString.byMonth);
    ait_rtc_Int2Str(psRtcTime->uwDay    , sTimeString.byDay);
    if(psRtcTime->byTimeCfg == AIT_OSD_TIME_FORMAT_2) {
        if (psRtcTime->uwHour > 11) {
            sTimeString.byAmpm[0] = 'P';
            sTimeString.byAmpm[1] = 'M';
            sTimeString.byAmpm[2] = 0;
            if(psRtcTime->uwHour > 12) {
                ait_rtc_Int2Str((psRtcTime->uwHour-12)  , sTimeString.byHour);
            }
            else {
                ait_rtc_Int2Str(psRtcTime->uwHour  , sTimeString.byHour);
            }
        }
        else {
            sTimeString.byAmpm[0] = 'A';
            sTimeString.byAmpm[1] = 'M';
            sTimeString.byAmpm[2] = 0;
            if (psRtcTime->uwHour == 0) {
                ait_rtc_Int2Str(12  , sTimeString.byHour);
            }
            else
            {
                ait_rtc_Int2Str(psRtcTime->uwHour  , sTimeString.byHour);
            }
        }
    }
    else {
        ait_rtc_Int2Str(psRtcTime->uwHour   , sTimeString.byHour);
    }
    ait_rtc_Int2Str(psRtcTime->uwMinute , sTimeString.byMinute);
    ait_rtc_Int2Str(psRtcTime->uwSecond , sTimeString.bySecond);

    //date
    if(psRtcTime->byDateCfg == AIT_OSD_DATE_FORMAT_1) {
        STRCPY(pdDateTime, sTimeString.byYear);
        STRCAT(pdDateTime, "-");
        if(psRtcTime->uwMonth < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byMonth);
        STRCAT(pdDateTime, "-");
        if(psRtcTime->uwDay < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byDay);
    }
    else if(psRtcTime->byDateCfg == AIT_OSD_DATE_FORMAT_2) {
        if(psRtcTime->uwDay < 10) { 
		STRCPY(pdDateTime, "0"); 
              STRCAT(pdDateTime, sTimeString.byDay);
	 }
	 else{
              STRCPY(pdDateTime, sTimeString.byDay);
 	 }
        STRCAT(pdDateTime, "-");
        if(psRtcTime->uwMonth < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byMonth);
        STRCAT(pdDateTime, "-");
        STRCAT(pdDateTime, sTimeString.byYear);
    }
    else if(psRtcTime->byDateCfg == AIT_OSD_DATE_FORMAT_3) {
        STRCPY(pdDateTime, sTimeString.byYear);
        STRCAT(pdDateTime, "/");
        if(psRtcTime->uwMonth < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byMonth);
        STRCAT(pdDateTime, "/");
        if(psRtcTime->uwDay < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byDay);
    }
    else if(psRtcTime->byDateCfg == AIT_OSD_DATE_FORMAT_4) {
        if(psRtcTime->uwDay < 10) { 
		STRCPY(pdDateTime, "0"); 
        STRCAT(pdDateTime, sTimeString.byDay);
	 }
	 else{
              STRCPY(pdDateTime, sTimeString.byDay);
 	 }
        STRCAT(pdDateTime, "/");
        if(psRtcTime->uwMonth < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byMonth);
        STRCAT(pdDateTime, "/");
        STRCAT(pdDateTime, sTimeString.byYear);
    }

    //space
    STRCAT(pdDateTime, " ");

    //time
    if(psRtcTime->byTimeCfg == AIT_OSD_TIME_FORMAT_1) {
        if(psRtcTime->uwHour < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byHour);
        STRCAT(pdDateTime, ":");
        if(psRtcTime->uwMinute < 10) { STRCAT(pdDateTime,"0"); }
        STRCAT(pdDateTime, sTimeString.byMinute);
        STRCAT(pdDateTime, ":");
        if(psRtcTime->uwSecond < 10) { STRCAT(pdDateTime,"0"); }
        STRCAT(pdDateTime, sTimeString.bySecond);
    }
    else if(psRtcTime->byTimeCfg == AIT_OSD_TIME_FORMAT_2) {
        if(psRtcTime->uwHour == 0) { 
		STRCAT(pdDateTime, sTimeString.byHour); 
	 }
        else {
            if(psRtcTime->uwHour < 10) { 
		    STRCAT(pdDateTime, "0"); 
	     }
            if((psRtcTime->uwHour > 12) && ((psRtcTime->uwHour-12) < 10)) { 
		    STRCAT(pdDateTime, "0"); 
	     }
            STRCAT(pdDateTime, sTimeString.byHour);
        }
        STRCAT(pdDateTime, ":");
        if(psRtcTime->uwMinute < 10) { STRCAT(pdDateTime,"0"); }
        STRCAT(pdDateTime, sTimeString.byMinute);
        STRCAT(pdDateTime, ":");
        if(psRtcTime->uwSecond < 10) { STRCAT(pdDateTime,"0"); }
        STRCAT(pdDateTime, sTimeString.bySecond);
        STRCAT(pdDateTime, sTimeString.byAmpm);
    }
    else if(psRtcTime->byTimeCfg == AIT_OSD_TIME_FORMAT_3) {
        if(psRtcTime->uwHour < 10) { STRCAT(pdDateTime, "0"); }
        STRCAT(pdDateTime, sTimeString.byHour);
        STRCAT(pdDateTime, ":");
        if(psRtcTime->uwMinute < 10) { STRCAT(pdDateTime,"0"); }
        STRCAT(pdDateTime, sTimeString.byMinute);
    }

    return MMP_ERR_NONE;
}
#endif

#endif//SUPPORT_OSD_FUNC

