#ifndef _TEXT_CTL_H
#define _TEXT_CTL_H

#include "isp_if.h"

#ifndef ON
#define ON  1
#endif
#ifndef OFF
#define OFF  0
#endif


// enable draw text feature
#define DRAW_TEXT_FEATURE_EN		OFF//ON

typedef enum {
	ISP_PREVIEW_FORMAT_YUV444					= 0,
	ISP_PREVIEW_FORMAT_YUV422					= 1,
	ISP_PREVIEW_FORMAT_YUV420					= 2,
	ISP_PREVIEW_FORMAT_RGB888					= 3,
	ISP_PREVIEW_FORMAT_RGB565					= 4,
	ISP_PREVIEW_FORMAT_RGB555					= 5,
	ISP_PREVIEW_FORMAT_RGB444					= 6,
	ISP_PREVIEW_FORMAT_BAYER					= 7,
	ISP_PREVIEW_FORMAT_RGB666					= 8,
	ISP_PREVIEW_FORMAT_YUV422_UYVY				= 9,
	ISP_PREVIEW_FORMAT_YUV422COLOR				= 10
} VENUS_PREVIEW_FORMAT;

#if DRAW_TEXT_FEATURE_EN==ON


#define DRAW_TEXT_BUF_LEN			256

extern ISP_UINT8 gDrawTextEn;
extern ISP_UINT8 gDrawAFWinEn;

//extern char gDrawTextBuf[DRAW_TEXT_BUF_LEN];
extern ISP_INT8 gDrawTextBuf[DRAW_TEXT_BUF_LEN];


// print string interface
void VR_PrintString(	ISP_INT8* txt_buf,			// text buffer
						ISP_UINT16 txt_v_offset,	// vertical offset
						ISP_UINT16 txt_h_offset,	// horizontal offset
						ISP_UINT16 txt_front_color,	// txt front color		// in RGB565
						ISP_UINT16 txt_back_color);	// txt background color	// in RGB565


void PrintString1BytePixel(	ISP_UINT8 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT32 offset,		//string offset in text line
							ISP_UINT8 front_color,
							ISP_UINT8 back_color,
							ISP_UINT8 txt_interval,	//interval between text in pixel
							ISP_INT8* text	);

void PrintString2BytePixel(	ISP_UINT16 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT32 offset,		//string offset in text line
							ISP_UINT16 front_color,
							ISP_UINT16 back_color,
							ISP_UINT8 txt_interval,	//interval between text in pixel
							ISP_INT8* text	);

void PrintString4BytePixel(	ISP_UINT32 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT32 offset,		//string offset in text line
							ISP_UINT32 front_color,
							ISP_UINT32 back_color,
							ISP_UINT8 txt_interval,	//interval between text in pixel
							ISP_INT8* text	);

void PrintStringYuv422(		ISP_UINT16 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT32 offset,		//string offset in text line
							ISP_UINT16 front_color,
							ISP_UINT16 back_color,
							ISP_UINT8 txt_interval,	//interval between text in pixel
							ISP_INT8* text	);

void PrintStringYuv422Color(ISP_UINT16 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT32 offset,		//string offset in text line
							ISP_UINT16 front_color,
							ISP_UINT16 back_color,
							ISP_UINT8 txt_interval,	//interval between text in pixel
							ISP_INT8* text);

void PrintPixelYuv422Color(ISP_UINT16 *dest_buf,	//image buffer
							ISP_UINT32 line_width,	//pixel per line
							ISP_UINT16 front_color,
							ISP_UINT16 back_color,
							ISP_UINT32 x,
							ISP_UINT32 y ) ;
void PrintLineYuv422Color( ISP_UINT32 line_width,	//pixel per line
							ISP_UINT16 front_color,
							ISP_UINT16 back_color,
							ISP_UINT32 x0,ISP_UINT32 y0,
							ISP_UINT32 x1,ISP_UINT32 y1 );

ISP_INT8* _strcpy(ISP_INT8* dest, ISP_INT8* src);
ISP_INT32 _strlen(ISP_INT8* str);
ISP_INT8* _strcat(ISP_INT8* dest, ISP_INT8* src);
ISP_INT8* itohs(ISP_UINT32 val, ISP_INT8* buf);
ISP_INT8* ui_to_ds(ISP_UINT32 val, ISP_INT8* buf);
ISP_INT8* i_to_ds(ISP_INT32 val, ISP_INT8* buf);
ISP_INT32 _sprintf(ISP_INT8* buf, ISP_INT8* fmt,...);

#endif // DRAW_TEXT_FEATURE_EN

#endif // _TEXT_CTL_H
