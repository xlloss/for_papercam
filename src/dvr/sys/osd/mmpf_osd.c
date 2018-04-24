#include "mmpf_ibc.h"
#include "mmp_snr_inc.h"
#include "mmp_ipc_inc.h"
#include "mmpf_system.h"
#include "ex_vidrec.h"
#include "ait_osd.h"
#include "mmpf_rtc.h"
#include "mmpf_vidcmn.h"
#include "mmps_vstream.h"
#include "dualcpu_v4l2.h"
#include "mmp_dma_inc.h"
#include "mmu.h"


#if AIT_RTC_EN==1
char DateStr[MAX_OSD_STR_LEN];
extern MMP_ULONG AccumTicks;
extern MMP_ULONG AccumTicks500ms;
static MMP_ULONG UpdateSec=0;
AIT_RTC_TIME RTC_TIME;
#endif
char  cmp_str[MAX_OSD_STR_LEN];
static MMP_BOOL  debug_osd = MMP_FALSE;  // debug message on/off
#if  SUPPORT_OSD_FUNC
AIT_OSD_HANDLE  h_osd_draw ;
AIT_OSD_BUFDESC desc_osd_str;
AIT_OSD_BUFDESC desc_osd_rtc;
AIT_OSD_DOUBLEBUF desc_osd_rtc_db;
AIT_OSD_FONT desc_osd_font;


ait_fdframe_config *osd_base_fdfr;
extern int draw_width_str, draw_width_rtc;
#endif
//------------------------------------------------------------------------------
//  Function    : MMPF_OSD_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for OSD.
*/
extern MMP_ULONG        UI_Get_Resolution(MMP_USHORT *w,MMP_USHORT *h);
static void MMPF_OSD_ReserveBuf(void)
{
#if SUPPORT_OSD_FUNC  
    MMP_SHORT MaxOSDFontH = 48,  MaxOSDFontW = 28, MaxOSDPadding = OSD_PADDING_ENABLE;
	MMP_SHORT cnt, max_draw_width_rtc, max_draw_width_str;
	
    #if AIT_RTC_EN 
    //draw_width_rtc = RTC_str_len*MaxOSDFontW;
	max_draw_width_rtc = RTC_str_len*MaxOSDFontW;
//    desc_osd_rtc.ulAddr[0] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,draw_width_rtc*MaxOSDFontH,32);
//    memset((MMP_ULONG*)desc_osd_rtc.ulAddr[0],Y_BLACK, draw_width_rtc*MaxOSDFontH);
//    desc_osd_rtc.ulAddr[1] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,draw_width_rtc*MaxOSDFontH/2,32);
//    memset((MMP_ULONG*)desc_osd_rtc.ulAddr[1],U_BLACK, draw_width_rtc*MaxOSDFontH/2);

	for (cnt = 0; cnt < SUPPORT_OSD_RTC_BUF; cnt++) {
		//draw_width_rtc = RTC_str_len*OSDFontW;
    	desc_osd_rtc_db.osd_buf[cnt].ulAddr[0] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,(max_draw_width_rtc+MaxOSDPadding)*MaxOSDFontH,32);
    	memset((MMP_ULONG*)desc_osd_rtc_db.osd_buf[cnt].ulAddr[0],Y_BLACK, (max_draw_width_rtc+MaxOSDPadding)*MaxOSDFontH);
    	desc_osd_rtc_db.osd_buf[cnt].ulAddr[1] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,(max_draw_width_rtc+MaxOSDPadding)*MaxOSDFontH*MaxOSDPadding/2,32);
    	memset((MMP_ULONG*)desc_osd_rtc_db.osd_buf[cnt].ulAddr[1],U_BLACK, (max_draw_width_rtc+MaxOSDPadding)*MaxOSDFontH*MaxOSDPadding/2);
		desc_osd_rtc = desc_osd_rtc_db.osd_buf[0];
		//printc("SUPPORT_OSD_RTC_DOUBLEBUF buffer set:%d done \r\n",cnt);
	}
	desc_osd_font.font_h = desc_osd_font.font_w = desc_osd_font.padding = 0;
	
    #endif

    #if AIT_STRING_EN
	
    //draw_width_str = String_str_len*MaxOSDFontW;
	max_draw_width_str =  String_str_len*MaxOSDFontW;
    desc_osd_str.ulAddr[0] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,(max_draw_width_str+MaxOSDPadding)*MaxOSDFontH,32);
    memset((MMP_ULONG*)desc_osd_str.ulAddr[0],Y_BLACK, (max_draw_width_str+MaxOSDPadding)*MaxOSDFontH);
    desc_osd_str.ulAddr[1] = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,(max_draw_width_str+MaxOSDPadding)*MaxOSDFontH/2,32);
    memset((MMP_ULONG*)desc_osd_str.ulAddr[1],U_BLACK, (max_draw_width_str+MaxOSDPadding)*MaxOSDFontH/2);
    
    #endif
#endif    
}
//------------------------------------------------------------------------------
//  Function    : MMPF_OSD_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize OSD module.

 @retval It reports the status of the operation.
*/
int MMPF_OSD_ModInit(void)
{
    MMPF_OSD_ReserveBuf(); // Reserve heap buffer for OSD engine
    return 0;
}

void osd_rtc_drawer()
{
#if SUPPORT_OSD_FUNC
	MMP_SHORT OSDFontW = desc_osd_font.font_w;
	MMP_SHORT OSDFontH = desc_osd_font.font_h;
	MMP_SHORT MaxOSDFontH = desc_osd_font.font_h;
	MMP_SHORT buf_idx;
//    osd_base_fdfr = aitcam_ipc_osd_base();
//	if(!osd_base_fdfr) {
//       printc("failed read osd_base_fdfr \r\n");
//        return ;
//    }
//	ait_fdframe_config  *osd_fdfr;
	#if AIT_RTC_EN || AIT_STRING_EN
    ait_rtc_config *rtc_base,*time_rtc;
    ait_osd_config *osd_base_rtc, *osd_base_str,*osd_rtc,*osd_str; 
    rtc_base = (ait_rtc_config *) (osd_base_fdfr + MAX_OSD_INDEX);
    osd_base_rtc = (ait_osd_config *) (rtc_base + 1);
    osd_base_str = (ait_osd_config *) (osd_base_rtc + 1);
//    osd_fdfr = osd_base_fdfr;
    time_rtc = rtc_base;
    osd_rtc = osd_base_rtc;
    osd_str = osd_base_str;
	#endif
	#if AIT_RTC_EN
	if (ait_rtc_UpdateTime(&RTC_TIME,1) != MMP_ERR_NONE)
		return;
  ait_rtc_ConvertDateTime2String(&RTC_TIME,DateStr);
	buf_idx = RTC_TIME.uwSecond%2;
	desc_osd_rtc_db.osd_buf[buf_idx].ul2N = 4 ;
	desc_osd_rtc_db.osd_buf[buf_idx].ulBufHeight = MaxOSDFontH;
	desc_osd_rtc_db.osd_buf[buf_idx].ulBufWidth  = draw_width_rtc;
	desc_osd_rtc_db.osd_buf[buf_idx].ulSrcHeight = MaxOSDFontH;
	desc_osd_rtc_db.osd_buf[buf_idx].ulSrcWidth  = draw_width_rtc;
	memset((MMP_ULONG*)desc_osd_rtc_db.osd_buf[buf_idx].ulAddr[0],Y_BLACK, draw_width_rtc*MaxOSDFontH);
	memset((MMP_ULONG*)desc_osd_rtc_db.osd_buf[buf_idx].ulAddr[1],U_BLACK, draw_width_rtc*MaxOSDFontH/2);
	ait_osd_SetPos(&h_osd_draw, 0, 0);
	ait_osd_SetColor(&h_osd_draw, osd_rtc->TextColorY,osd_rtc->TextColorU, osd_rtc->TextColorV);
	ait_osd_DrawFontStr(&h_osd_draw, &desc_osd_rtc_db.osd_buf[buf_idx], (const char*)&DateStr, strlen(DateStr));
	desc_osd_rtc = desc_osd_rtc_db.osd_buf[buf_idx];
	#endif
	#if AIT_STRING_EN
	if(strcmp(cmp_str,osd_str->str) != 0 ) {
        desc_osd_str.ul2N = 4 ;
        desc_osd_str.ulBufHeight = MaxOSDFontH;
        desc_osd_str.ulBufWidth  = draw_width_str;
        desc_osd_str.ulSrcHeight = MaxOSDFontH;
        desc_osd_str.ulSrcWidth  = draw_width_str;
      
        ait_osd_SetPos(&h_osd_draw, 0, 0);
        ait_osd_SetColor(&h_osd_draw, osd_str->TextColorY,osd_str->TextColorU, osd_str->TextColorV);

        memset((MMP_ULONG*)desc_osd_str.ulAddr[0],Y_BLACK, draw_width_str*MaxOSDFontH);
        memset((MMP_ULONG*)desc_osd_str.ulAddr[1],U_BLACK, draw_width_str*MaxOSDFontH/2);
        ait_osd_DrawFontStr(&h_osd_draw,&desc_osd_str,osd_str->str,strlen(osd_str->str));
		strcpy(cmp_str,osd_str->str );
	}
    #endif
	
#endif
}

/**
 @brief Main routine of OSD task.
*/
extern MMPF_OS_FLAGID	OSD_Flag;
void MMPF_OSD_Timer_callback(void)
{
#if SUPPORT_OSD_FUNC
	if (desc_osd_font.dflags == MMP_TRUE)
		printc("OSD Drawing too late\r\n");
	MMPF_OS_SetFlags(OSD_Flag, OSD_FLAG_RUN, MMPF_OS_FLAG_SET);
#endif	
}

void MMPF_OSD_Task(void)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;
    wait_flags = OSD_FLAG_RUN|OSD_FLAG_OSDRES;
    MMP_BOOL en;
	MMPF_TIMER_ATTR osd_timer_attr;
	MMP_USHORT w,h;

#if SUPPORT_OSD_FUNC	
	extern GUI_CHARINFO GUI_FontEnglish_16_CharInfo[];
	extern GUI_CHARINFO GUI_FontEnglish_24_CharInfo[];
	extern GUI_CHARINFO GUI_FontEnglish_40_CharInfo[];

	//open timer//
	draw_width_str = draw_width_rtc = 0;
	osd_timer_attr.bIntMode = MMPF_TIMER_PERIODIC;
	osd_timer_attr.TimePrecision = MMPF_TIMER_SEC;
	osd_timer_attr.ulTimeUnits = 1;
	osd_timer_attr.Func = MMPF_OSD_Timer_callback;
	desc_osd_font.dflags = MMP_FALSE;
	
	MMPF_Timer_Start(MMPF_TIMER_1, &osd_timer_attr);
    while (1) {
        MMPF_OS_WaitFlags(  OSD_Flag, wait_flags,
                        MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                        0, &flags);

		if (flags & OSD_FLAG_OSDRES) {
			UI_Get_Resolution(&w, &h);
			//draw_width_rtc = 
			if(( w > 1280) || ( h > 720)) {
				desc_osd_font.font_w = OSD_FONT40_W;
				desc_osd_font.font_h = OSD_FONT40_H;
				desc_osd_font.fontmap = &GUI_FontEnglish_40_CharInfo[0];
				
			}else if(( w > 640) || ( h > 480)) {
				desc_osd_font.font_w = OSD_FONT24_W;
				desc_osd_font.font_h = OSD_FONT24_H;
				desc_osd_font.fontmap = &GUI_FontEnglish_24_CharInfo[0];
			}else {
				desc_osd_font.font_w = OSD_FONT16_W;
				desc_osd_font.font_h = OSD_FONT16_H;
				desc_osd_font.fontmap = &GUI_FontEnglish_16_CharInfo[0];
			}
			if (OSD_PADDING_ENABLE)
				desc_osd_font.padding = sizeof(MMP_ULONG);
			
			draw_width_str = String_str_len*desc_osd_font.font_w +desc_osd_font.font_w*OSD_PADDING_ENABLE;
			draw_width_rtc = RTC_str_len*desc_osd_font.font_w+desc_osd_font.font_w*OSD_PADDING_ENABLE;
		}
		desc_osd_font.dflags = MMP_TRUE;
		osd_rtc_drawer();
		desc_osd_font.dflags = MMP_FALSE;
	}
#endif	
}

ait_module_init(MMPF_OSD_ModInit);


