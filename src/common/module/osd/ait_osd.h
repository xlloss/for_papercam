//==============================================================================
//
//  File        : ait_osd.h
//  Description : INCLUDE File for the OSD general function porting.
//  Author      : 
//  Revision    : 1.0
//
//==============================================================================

#ifndef _AIT_OSD_H_
#define _AIT_OSD_H_

/*===========================================================================
 * Include files
 *===========================================================================*/ 
#include "config_fw.h"


#if (SUPPORT_OSD_FUNC)
#include "GUI.h"
#define AIT_FDFRAME_EN      (1)
#define AIT_WATERMARK_EN    (0)
#define AIT_RTC_EN          (1)
#define AIT_COLOR_EN        (0)
#define AIT_STRING_EN       (1)
#define AIT_MDFRAME_EN      (0)

#define RES_OSD_H16_FONT    (1)
#define RES_OSD_H18_FONT    (0)
#define RES_OSD_H24_FONT    (1)
#define RES_OSD_H32_FONT    (1)
#define RES_OSD_H48_FONT    (0)
#define RES_OSD_H40_FONT	(1)

/*===========================================================================
 *                              CONSTANTS
 *===========================================================================*/  
#define SUPPORT_OSD_RTC_BUF	(2)			

#ifndef STRLEN
    #define STRLEN strlen
#endif
#ifndef STRCPY
    #define STRCPY strcpy
#endif
#ifndef STRCMP
    #define STRCMP strcmp
#endif
#ifndef STRCAT
    #define STRCAT strcat
#endif
#ifndef MEMSET
    #define MEMSET memset
#endif

#define COLR_Y      (0)
#define COLR_U      (1)
#define COLR_V      (2)
#define COLR_A      (3)

//BT.601 yuv420 palette
#define BLACK_Y (0)//(16)
#define BLACK_U (128)
#define BLACK_V (128)
#define YELLOW_Y (210)
#define YELLOW_U (16)
#define YELLOW_V (146)
#define WHITE_Y (235)
#define WHITE_U (128)
#define WHITE_V (128)

#define AIT_RTC_COMPONENT_MAX_FIELD     (8)

#define OSD_PADDING_ENABLE	(1)
#if (RES_OSD_H16_FONT)//640/480
#define OSD_FONT16_W  8
#define OSD_FONT16_H  16
#endif
#if (RES_OSD_H18_FONT)
#define OSD_FONT18_W  16
#define OSD_FONT18_H  18
#endif
#if (RES_OSD_H24_FONT)
#define OSD_FONT24_W  16
#define OSD_FONT24_H  24
#endif
#if (RES_OSD_H32_FONT)//1280/720
#define OSD_FONT32_W  24
#define OSD_FONT32_H  32
#endif
#if (RES_OSD_H40_FONT)//1920/1080
#define OSD_FONT40_W  24
#define OSD_FONT40_H  40
#endif
#if (RES_OSD_H48_FONT)
#define OSD_FONT48_W  28
#define OSD_FONT48_H  48
#endif

#define OSD_TEXT_MAX_FIELD          20 //customer spec.


/*===========================================================================
 * Structure define
 *===========================================================================*/

typedef enum _AIT_OSD_DATE_FORMAT_CONFIG {
    AIT_OSD_DATE_FORMAT_DISABLE         = 0,
    AIT_OSD_DATE_FORMAT_1               = 1,     //%Y-%M-%D
    AIT_OSD_DATE_FORMAT_2               = 2,     //%D-%M-%Y
    AIT_OSD_DATE_FORMAT_3               = 3,     //%Y/%M/%D
    AIT_OSD_DATE_FORMAT_4               = 4,     //%D/%M/%Y
    AIT_OSD_DATE_FORMAT_MAX          = 5
} AIT_OSD_DATE_FORMAT_CONFIG;

typedef enum _AIT_OSD_TIME_FORMAT_CONFIG {
    AIT_OSD_TIME_FORMAT_DISABLE    = 0,
    AIT_OSD_TIME_FORMAT_1               = 1, //%H:%M:%S
    AIT_OSD_TIME_FORMAT_2               = 2, //%I:%M:%S%ampm
    AIT_OSD_TIME_FORMAT_3               = 3, //%H:%M
    AIT_OSD_TIME_FORMAT_MAX          = 4
} AIT_OSD_TIME_FORMAT_CONFIG;

typedef struct _AIT_RTC_TIME
{
	MMP_USHORT  uwYear;
	MMP_USHORT  uwMonth;
	MMP_USHORT  uwDay;
	MMP_USHORT  uwHour;
	MMP_USHORT  uwMinute;
	MMP_USHORT  uwSecond;
	MMP_BOOL    bOn;

	AIT_OSD_DATE_FORMAT_CONFIG  byDateCfg;
	AIT_OSD_TIME_FORMAT_CONFIG  byTimeCfg;

} AIT_RTC_TIME;

typedef struct _AIT_TIME_STRING
{
	MMP_BYTE byYear[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE byMonth[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE byDay[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE byHour[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE byMinute[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE bySecond[AIT_RTC_COMPONENT_MAX_FIELD];
	MMP_BYTE byAmpm[AIT_RTC_COMPONENT_MAX_FIELD];

} AIT_TIME_STRING;

typedef struct _AIT_OSD_BUFDESC {
    MMP_ULONG   ulAddr[3];
    MMP_ULONG   ulBufWidth;
    MMP_ULONG   ulBufHeight;
    MMP_ULONG   ulSrcWidth ;
    MMP_ULONG   ulSrcHeight;
    MMP_ULONG   ulRatioX ;
    MMP_ULONG   ulRatioY ;
    MMP_ULONG   ul2N ;
    //colorformat
} AIT_OSD_BUFDESC;

typedef struct _AIT_OSD_DOUBLEBUF {
    AIT_OSD_BUFDESC osd_buf[2];
} AIT_OSD_DOUBLEBUF;

typedef struct _AIT_OSD_FONT {
	MMP_USHORT	font_w;
	MMP_USHORT	font_h;
	MMP_USHORT	padding;
	GUI_CHARINFO *fontmap;
	MMP_BOOL	dflags;
} AIT_OSD_FONT;

typedef struct _AIT_OSD_HANDLE {
    MMP_USHORT  usStartXOffset;
    MMP_USHORT  usStartYOffset;
    MMP_USHORT  usWidth;
    MMP_USHORT  usHeight;
    MMP_UBYTE   FgColor[4]; //YUV-A, 225, 16, 148, 100
} AIT_OSD_HANDLE;



/*===========================================================================
 * Macro function
 *===========================================================================*/
#define ait_rtc_IsClockOn(_h)           (((AIT_RTC_TIME *)(_h))->bOn)

/*===========================================================================
 * Function prototype
 *===========================================================================*/

/* Set OSD default config */
void        ait_osd_Init (AIT_OSD_HANDLE *pOsd);
void        ait_osd_GetFontPixelSize(MMP_USHORT *HSize, MMP_USHORT *VSize);
/* Set OSD text color */
void        ait_osd_SetColor(AIT_OSD_HANDLE *pOsd, MMP_UBYTE yColor,MMP_UBYTE uColor,MMP_UBYTE vColor);
/* Set OSD start offset relate to buffer */
void        ait_osd_SetPos(AIT_OSD_HANDLE *pOsd, MMP_USHORT xPos,MMP_USHORT yPos);
void        ait_osd_SetFDWidthHeight(AIT_OSD_HANDLE *pOsd, MMP_USHORT width,MMP_USHORT height);

/* Paint background color */
void        ait_osd_DrawColor(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, MMP_USHORT w,MMP_USHORT h);

/* Do OSD drawing to buffer */
void        ait_osd_DrawStr(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen);
void 		ait_osd_DrawFontStr(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen);

/* Do OSD drawing watermark to buffer */
void        ait_osd_DrawWatermark(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf);

void ait_osd_DrawFDFrame(AIT_OSD_HANDLE *pOsd, AIT_OSD_BUFDESC *pBuf, const char *pStr, MMP_SHORT nStrLen);

/* RTC related */
MMP_ERR     ait_rtc_SetClock(AIT_RTC_TIME *pRtcTime, MMP_USHORT pwYear, MMP_USHORT pwMonth,
                MMP_USHORT pwDay, MMP_USHORT pwHour, MMP_USHORT pwMinute, MMP_USHORT pwSecond, MMP_BOOL bOn);
MMP_ERR     ait_rtc_SetDateTimeFormat(AIT_RTC_TIME *pRtcTime, AIT_OSD_DATE_FORMAT_CONFIG DateFormatConfig,
                AIT_OSD_TIME_FORMAT_CONFIG TimeFormatConfig);
MMP_ERR     ait_rtc_UpdateTime(AIT_RTC_TIME *pRtcTime, MMP_ULONG nSec);
MMP_ERR     ait_rtc_ConvertDateTime2String(AIT_RTC_TIME* psRtcTime, MMP_BYTE* pdDateTime);

#endif //(SUPPORT_OSD_FUNC)

#endif // _AIT_OSD_H_

