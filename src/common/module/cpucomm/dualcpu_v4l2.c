//==============================================================================
//
//  File        : dualcpu_uart.c
//  Description : UART wrapper for CPU B
//  Author      : Chiket Lin
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
#include <stdio.h>
#include <stdarg.h>
#include "all_fw.h"
#include "includes_fw.h"
#include "lib_retina.h"
#include "os_cpu.h"
#include "mmpf_uart.h"
#include "mmu.h"
#include "mmp_register.h"
#include "cpucomm.h"
#include "cpucomm_bus.h"
#include "cpu_sharemem.h"
#include "dualcpu_v4l2.h"
#include "mmpf_ibc.h"

#include "mmps_sensor.h"
#include "mmpf_sensor.h"
#include "mmp_snr_inc.h"
#include "mmp_ipc_inc.h"
#include "mmpf_system.h"
#include "ex_vidrec.h"
#include "isp_if.h"
#include "ait_osd.h"
#include "ex_vidrec.h"
#include "mmpf_rtc.h"
#include "ex_misc.h"
#include "mmps_mdtc.h"
#include "mmp_dma_inc.h"
#include "mmpf_jstream.h"

#if (SUPPORT_V4L2)

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
#define MD_OSD_MEM_OFF    (4*1024)
#define MD_OSD_MEM_SZ     (4*1024)
#define MD_OSD_MAX    (MD_OSD_MEM_SZ/ sizeof(ait_fdframe_config) )

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
static MMPF_OS_SEMID    s_ulV4L2RcvSemId;  // Semaphore for critical section

static OS_STK           s_V4L2TaskStk[TASK_B_V4L2_STK_SIZE];   // Task stack

static aitcam_stream    v4l2_stream[AITCAM_NUM_CONTEXTS];

#if (V4L2_H264)
static aitcam_streamtbl vid_stream_tbl[MAX_H264_STREAM_NUM];
#endif
#if (V4L2_AAC)
static aitcam_streamtbl aud_stream_tbl[MAX_AUD_STREAM_NUM];
#endif
#if (V4L2_JPG)
static aitcam_streamtbl jpg_stream_tbl[MAX_JPG_STREAM_NUM];
#endif
#if (V4L2_GRAY)
aitcam_streamtbl gray_stream_tbl[MAX_GRAY_STREAM_NUM];
#endif

static cpu_comm_transfer_data   _V4L2_Proc;
static cpu_comm_transfer_data   _V4L2_Return;

static MMP_LONG         V4L2_PAYLOAD_INFO[4];

static MMP_BOOL         v4l2_debug = MMP_FALSE;  // debug message on/off
static MMP_BOOL         v4l2_debug_ts = MMP_FALSE;  // debug message on/off
static MMP_BOOL         v4l2_debug_osd = MMP_FALSE;  // debug message on/off

static int v4l2_streamer_status = V4L2_CAMERA_AIT_STREAMER_DOWN ;

#if (V4L2_H264)
// used for mapping v4l2 index to h.264 level
static MMP_UBYTE        h264_level[16] = {
    /* V4L2_MPEG_VIDEO_H264_LEVEL_1_0   */ 10,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_1B    */ 9,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_1_1   */ 11,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_1_2   */ 12,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_1_3   */ 13,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_2_0   */ 20,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_2_1   */ 21,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_2_2   */ 22,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_3_0   */ 30,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_3_1   */ 31,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_3_2   */ 32,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_4_0   */ 40,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_4_1   */ 41,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_4_2   */ 42,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_5_0   */ 50,
	/* V4L2_MPEG_VIDEO_H264_LEVEL_5_1   */ 51
};
#endif

#if SUPPORT_OSD_FUNC==1
AIT_OSD_BUFDESC desc_osd[MMP_IBC_PIPE_MAX];
ait_fdframe_config *osd_base_fdfr;
int draw_width_str, draw_width_rtc;
static ait_fdframe_config last_fdfr_osd[MAX_OSD_INDEX] ;
static AIT_OSD_HANDLE  h_osd[MMP_IBC_PIPE_MAX] ;
MMP_ULONG AccumTicks=0;
MMP_ULONG AccumTicks500ms=0;
static MMP_ULONG  RTCTicksS=0;
static MMP_ULONG RTCTicksE=0;
static MMP_BOOL RTCFlag=0;
static MMP_ULONG DiffTicks=0;
static MMP_ULONG UpdateSec=0;
extern AIT_OSD_BUFDESC desc_osd_str ;
extern AIT_OSD_BUFDESC desc_osd_rtc ;
extern MMP_BOOL UI_Get_OsdRtcEnable(MMP_ULONG streamset);
extern MMP_BOOL UI_Get_OsdStrEnable(MMP_ULONG streamset);
extern MMP_ULONG UI_Get_Str_Pos_startX(void);
extern MMP_ULONG UI_Get_Str_Pos_startY(void);
extern MMP_ULONG UI_Get_Rtc_Pos_startX(void);
extern MMP_ULONG UI_Get_Rtc_Pos_startY(void);
extern AIT_OSD_HANDLE  h_osd_draw ;
extern char DateStr[MAX_OSD_STR_LEN];
extern AIT_RTC_TIME RTC_TIME;
static MMP_BOOL init_flag=0;
#if 1//AIT_FDFRAME_EN==1
static MMP_BOOL draw_fdfr_osd = MMP_FALSE ;
#endif
#if AIT_MDFRAME_EN==1
AIT_OSD_BUFDESC desc_md_osd ;
static MMP_BOOL draw_md_osd = MMP_FALSE ;
static int md_roi_cnt = 0 ;
#endif
#endif

#if MIPI_TEST_PARAM_EN==1
MMP_ULONG mipi_test_ng ;
#endif

/*
 * External Variables
 */
extern MMPF_OS_FLAGID   IPC_UI_Flag;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void EstablishV4L2(void);
static void *aitcam_ipc_get_q(MMP_UBYTE streamid);
static void aitcam_ipc_init_vidctrl(int streamid);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if SUPPORT_OSD_FUNC
/*static*/ void aitcam_osd_set_draw_desc(AIT_OSD_BUFDESC *desc,int pid)
{
    MMP_IBC_PIPE_BUF ibc_buf ;
    MMPF_IBC_GetBackupCurBuf( pid ,&ibc_buf );
    desc->ulAddr[0] = ibc_buf.ulBaseAddr  ;
    desc->ulAddr[1] = ibc_buf.ulBaseUAddr ;
    desc->ulAddr[2] = ibc_buf.ulBaseVAddr ;
    //desc->ulBufWidth  = width  ;
    //desc->ulBufHeight = height ;
}


int aitcam_enable_md_osd(MMP_BOOL en,int md_w,int md_h)
{
  #if AIT_MDFRAME_EN
  int i ;
  ait_fdframe_config *osd_base_md =  aitcam_ipc_mdosd_base(),*osd_md ;
  draw_md_osd = en ;
  desc_md_osd.ulSrcWidth  = md_w ;
  desc_md_osd.ulSrcHeight = md_h ;
  md_roi_cnt = 0;
  for(i=0;i<MD_OSD_MAX;i++) {
    osd_md = osd_base_md+ i; 
    osd_md->type = 0 ;
    osd_md->index = 0 ;
  }
  #endif
  //return MD_OSD_MAX ;
  return 0 ;
}

int aitcam_set_md_osd(int at,int md_n,MD_block_info_t *area)
{
  #if AIT_MDFRAME_EN
  int i ;
  ait_fdframe_config *osd_base_md =  aitcam_ipc_mdosd_base(),*osd_md ;
  if(at >= MD_OSD_MAX) {
    return 0;
  }
  
  osd_md = osd_base_md+at;
  osd_md->type = AIT_OSD_TYPE_FDFRAME ;
  osd_md->index  = at ;
  osd_md->str[0] = 0;
  osd_md->pos[0] = area->st_x;
  osd_md->pos[1] = area->st_y;
  osd_md->width  = area->end_x - area->st_x + 1 ;
  osd_md->height = area->end_y - area->st_y + 1 ;
  //printc("[SEAN](%d,%d) md[%d]->[%d,%d,%d,%d]\r\n",MD_OSD_MAX,md_n,at,osd_md->pos[0],osd_md->pos[1],osd_md->width,osd_md->height );
  //printc("[SEAN][%x]md[%d] type : %d\r\n", osd_md,osd_md->index,osd_md->type);
  md_roi_cnt = md_n;
  #endif

  return 0;
}

int aitcam_set_osd_color(int at,int osd_type,AIT_OSD_HANDLE *pOsd)
{
  #if AIT_MDFRAME_EN || AIT_FDFRFRAME_EN
  MMPS_MDTC_CLASS *mdobj = (MMPS_MDTC_CLASS *)MMPS_MDTC_Obj() ;
  MMPF_MDTC_CLASS *md = &mdobj->md ;
  MD_params_out_t md_outinfo ;
  if(osd_type==0) {
    // green
    ait_osd_SetColor(pOsd, 117,61,44);
  }
  else {
    if(at >= MD_OSD_MAX ) {
      return 0;
    }
    md_outinfo = md->result[at] ;
    if(md_outinfo.md_result) {
      ait_osd_SetColor(pOsd, 220,0,180);
    }
    else {
      ait_osd_SetColor(pOsd, 59,94,228);
    }
  }
  #endif
  
  
  return 0 ;
}

static MMPF_OS_SEMID        o_MoveDMASemID; ///< for DMA operation control

static void MMPF_OSD_Init()
{
	o_MoveDMASemID = MMPF_OS_CreateSem(1);
}

extern MMP_ERR MMPF_DMA_MoveData(MMP_ULONG 	ulSrcAddr,MMP_ULONG ulDstAddr,MMP_ULONG ulCount,DmaCallBackFunc
        *CallBackFunc,void *CallBackArg,MMP_BOOL bEnLineOfst,MMP_DMA_LINE_OFST *pLineOfst);

static void MMPF_OSD_ReleaseDma(void *argu)
{
	if (MMPF_OS_ReleaseSem(o_MoveDMASemID) != OS_NO_ERR) {
		RTNA_DBG_Str(3, "o_MoveDMASemID OSSemPost: Fail\r\n");
	}
}

void MMPF_OSD_DmaMoveData(MMP_ULONG src, MMP_ULONG dst, MMP_ULONG sz,DmaCallBackFunc *CallBackFunc, void *CallBackArg, MMP_BOOL bEnLineOfst, MMP_DMA_LINE_OFST *pLineOfst )
{
    if (sz == 0) {
        printc("DMA move size null!!!\r\n");
        return;
    }
	
	if (MMPF_OS_AcquireSem(o_MoveDMASemID, 0)) {
        RTNA_DBG_Str(0, "o_MoveDMASemPend fail\r\n");
        return;
	}

	if (MMPF_DMA_MoveData(src, dst, sz, MMPF_OSD_ReleaseDma,
	                      NULL, MMP_TRUE, pLineOfst))
    {
        RTNA_DBG_Str(0, "DMA_MoveData fail\r\n");
        return;
	}
	
}
extern AIT_OSD_DOUBLEBUF desc_osd_rtc_db;
extern AIT_OSD_FONT desc_osd_font;
void aitcam_osd_drawer(void *argc)
{
#if SUPPORT_OSD_FUNC
    int  i;
    int pid,streamset ;
    unsigned int arg = (unsigned int)argc ;
    unsigned int off_x,off_y, w ,h, pos_x, pos_y ;
    MMP_SHORT MaxOSDFontH = desc_osd_font.font_h;
	MMP_SHORT OSDFontW = desc_osd_font.font_w;
    MMP_ULONG TimeStart, TimeEnd;
    streamset = (arg >> 8) & 0x00FF ;
    pid = (arg & 0x00FF );
    MMP_DMA_LINE_OFST   LineOfstArg1Str;
    MMP_DMA_LINE_OFST   LineOfstArg2Str;
    MMP_DMA_LINE_OFST   LineOfstArg1Rtc;
    MMP_DMA_LINE_OFST   LineOfstArg2Rtc;

    osd_base_fdfr = aitcam_ipc_osd_base();
  #if AIT_MDFRAME_EN
    ait_fdframe_config *osd_base_md =  aitcam_ipc_mdosd_base();
  #endif

    if(!osd_base_fdfr) {
        printc("failed read osd_base_fdfr \r\n");
        return ;
    }

    if(o_MoveDMASemID ==0 ) {
        MMPF_OSD_Init();
    }

    ait_fdframe_config *osd_fdfr ;
    #if AIT_RTC_EN || AIT_STRING_EN
    ait_rtc_config *rtc_base,*time_rtc;
    ait_osd_config *osd_base_rtc, *osd_base_str,*osd_rtc,*osd_str; 
    rtc_base = (ait_rtc_config *) (osd_base_fdfr + MAX_OSD_INDEX);
    osd_base_rtc = (ait_osd_config *) (rtc_base + 1);
    osd_base_str = (ait_osd_config *) (osd_base_rtc + 1);
    osd_fdfr = osd_base_fdfr;
    time_rtc = rtc_base;
    osd_rtc = osd_base_rtc;
    osd_str = osd_base_str;
    #endif
    #if AIT_RTC_EN 
    if( UI_Get_OsdRtcEnable(streamset) )
        osd_rtc->type = AIT_OSD_TYPE_RTC;
    else
        osd_rtc->type = AIT_OSD_TYPE_INACTIVATE;
    #endif

    #if AIT_STRING_EN
    if( UI_Get_OsdStrEnable(streamset) )
        osd_str->type = AIT_OSD_TYPE_CUST_STRING;
    else
        osd_str->type = AIT_OSD_TYPE_INACTIVATE;
    #endif
    

    if(init_flag == 0) {
    #if AIT_RTC_EN 
        RTC_TIME.uwYear =  time_rtc->usYear;
        RTC_TIME.uwMonth = time_rtc->usMonth;
        RTC_TIME.uwDay = time_rtc->usDay;
        RTC_TIME.uwHour = time_rtc->usHour;
        RTC_TIME.uwMinute = time_rtc->usMinute;
        RTC_TIME.uwSecond = time_rtc->usSecond;
        RTC_TIME.bOn = time_rtc->bOn;
        RTC_TIME.byDateCfg = osd_rtc->str[0];
        RTC_TIME.byTimeCfg = osd_rtc->str[1];

    #endif

    #if AIT_STRING_EN
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
        
    #endif
    }

    /* Get draw buffer address */
    aitcam_osd_set_draw_desc( &desc_osd[pid], pid);
    #if AIT_FDFRAME_EN
    for(i=0;i < MAX_OSD_INDEX ;i++) {
        osd_fdfr = osd_base_fdfr + i ;
        if(osd_fdfr->type) 
        if(osd_fdfr->index < MAX_OSD_INDEX) {
            if(osd_fdfr->type== AIT_OSD_TYPE_FDFRAME) {
                if(!draw_fdfr_osd) {
                    break  ;
                }
                if(!desc_osd[pid].ulRatioX) {
                    desc_osd[pid].ulRatioX = (desc_osd[pid].ulBufWidth << desc_osd[pid].ul2N) / desc_osd[pid].ulSrcWidth ;
                }
                if(!desc_osd[pid].ulRatioY) {
                    desc_osd[pid].ulRatioY = (desc_osd[pid].ulBufHeight<< desc_osd[pid].ul2N) / desc_osd[pid].ulSrcHeight ;
                }
                if(desc_osd[pid].ulRatioX && desc_osd[pid].ulRatioY ) {
                    
                    off_x = (desc_osd[pid].ulRatioX * osd_fdfr->pos[0] ) >> desc_osd[pid].ul2N ;
                    off_y = (desc_osd[pid].ulRatioY * osd_fdfr->pos[1] ) >> desc_osd[pid].ul2N ;
                    w     = (desc_osd[pid].ulRatioX * osd_fdfr->width  ) >> desc_osd[pid].ul2N ;
                    h     = (desc_osd[pid].ulRatioY * osd_fdfr->height ) >> desc_osd[pid].ul2N ;

                    if(v4l2_debug_osd) {
                        printc("#osd.pid=%d,[%d] (%d,%d,%d,%d) -> (%d,%d,%d,%d)\r\n",pid,i,osd_fdfr->pos[0],osd_fdfr->pos[1],osd_fdfr->width,osd_fdfr->height,off_x,off_y,w,h );
                        printc("#desc.pid=%d,[%d,%d]->[%d,%d]\r\n",pid,desc_osd[pid].ulSrcWidth,desc_osd[pid].ulSrcHeight,desc_osd[pid].ulBufWidth,desc_osd[pid].ulBufHeight);
                    }
                    aitcam_set_osd_color(0,0, &h_osd[pid] );
                    ait_osd_SetPos( &h_osd[pid] , off_x, off_y); 
                    ait_osd_SetFDWidthHeight( &h_osd[pid] , w, h );  
                    ait_osd_DrawFDFrame(&h_osd[pid],&desc_osd[pid],osd_fdfr->str,strlen(osd_fdfr->str));
                }
          }
          last_fdfr_osd[i] = *osd_fdfr ;
        }  
    }
    #endif  
    #if AIT_MDFRAME_EN
    for(i=0;i < md_roi_cnt ;i++) {
        osd_fdfr = osd_base_md + i ;
        if(osd_fdfr->type) 
        if(osd_fdfr->index < MD_OSD_MAX) {
            if(osd_fdfr->type== AIT_OSD_TYPE_FDFRAME) {
                //if(!MMPS_MDTC_Obj()->state!=MDTC_STATE_START) {
                //    break ;
                //}
                if(!draw_md_osd) {
                  break ;
                }
                if(!desc_md_osd.ulRatioX) {
                    desc_md_osd.ulRatioX = (desc_md_osd.ulBufWidth << desc_md_osd.ul2N) / desc_md_osd.ulSrcWidth ;
                }
                if(!desc_md_osd.ulRatioY) {
                    desc_md_osd.ulRatioY = (desc_md_osd.ulBufHeight<< desc_md_osd.ul2N) / desc_md_osd.ulSrcHeight ;
                }
                if(desc_md_osd.ulRatioX && desc_md_osd.ulRatioY ) {
                    
                    off_x = (desc_md_osd.ulRatioX * osd_fdfr->pos[0] ) >> desc_md_osd.ul2N ;
                    off_y = (desc_md_osd.ulRatioY * osd_fdfr->pos[1] ) >> desc_md_osd.ul2N ;
                    w     = (desc_md_osd.ulRatioX * osd_fdfr->width  ) >> desc_md_osd.ul2N ;
                    h     = (desc_md_osd.ulRatioY * osd_fdfr->height ) >> desc_md_osd.ul2N ;
                    off_x+=3 ;
                    off_y+=3 ;
                    w-=6;
                    h-=6;
                    
                    if(v4l2_debug_osd) {
                        printc("#osd.pid=%d,[%d] (%d,%d,%d,%d) -> (%d,%d,%d,%d)\r\n",pid,i,osd_fdfr->pos[0],osd_fdfr->pos[1],osd_fdfr->width,osd_fdfr->height,off_x,off_y,w,h );
                        printc("#desc.pid=%d,[%d,%d]->[%d,%d]\r\n",pid,desc_md_osd.ulSrcWidth,desc_md_osd.ulSrcHeight,desc_md_osd.ulBufWidth,desc_md_osd.ulBufHeight);
                    }
                    aitcam_set_osd_color(i,1, &h_osd[pid] );
                    ait_osd_SetPos( &h_osd[pid] , off_x, off_y); 
                    ait_osd_SetFDWidthHeight( &h_osd[pid] , w, h );  
                    ait_osd_DrawFDFrame(&h_osd[pid],&desc_osd[pid],osd_fdfr->str,strlen(osd_fdfr->str));
                }
          }
        }  
    }
    #endif  

    #if AIT_RTC_EN 
    if(osd_rtc->type == AIT_OSD_TYPE_RTC ) {

            if(RTCFlag==0) {
                MMPF_OS_GetTime(&RTCTicksS);
                RTCFlag=1;
            }
            else {
                MMPF_OS_GetTime(&RTCTicksE);
                RTCFlag=0;
            }
        
            if(RTCTicksE > RTCTicksS) {
                DiffTicks = RTCTicksE - RTCTicksS;
            }
            else {
                DiffTicks = RTCTicksS - RTCTicksE;
            }

            AccumTicks += DiffTicks;
            AccumTicks500ms += DiffTicks;

        if(( desc_osd[pid].ulBufWidth > 1280) || ( desc_osd[pid].ulBufHeight > 720)) {
            LineOfstArg1Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg1Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulDstOffset = 1920;

            LineOfstArg2Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg2Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulDstOffset = 1920;
        }
        else if(( desc_osd[pid].ulBufWidth > 640) || ( desc_osd[pid].ulBufHeight > 480)) {
            LineOfstArg1Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg1Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulDstOffset = 1280;

            LineOfstArg2Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg2Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulDstOffset = 1280;
        }else {
        	LineOfstArg1Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg1Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg1Rtc.ulDstOffset = 640;

            LineOfstArg2Rtc.ulSrcWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulSrcOffset = draw_width_rtc;
            LineOfstArg2Rtc.ulDstWidth  = draw_width_rtc;
            LineOfstArg2Rtc.ulDstOffset = 640;
        	
        }

        off_x = (osd_rtc->pos[AXIS_X]);  

        if ( UI_Get_Rtc_Pos_startX() ==1) {
            pos_x = ( desc_osd[pid].ulBufWidth - draw_width_rtc - off_x);
        }

        if (UI_Get_Rtc_Pos_startX() == 0) {
            pos_x = off_x;
        }

        pos_y = (osd_rtc->pos[AXIS_Y]);  
    		if (draw_width_rtc > 0) {
	        if ( UI_Get_Rtc_Pos_startY() == 1 ) {
	            MMPF_OSD_DmaMoveData(desc_osd_rtc.ulAddr[0],(desc_osd[pid].ulAddr[0] + desc_osd[pid].ulBufWidth*pos_y + pos_x),draw_width_rtc*MaxOSDFontH,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg1Rtc);
	            MMPF_OSD_DmaMoveData(desc_osd_rtc.ulAddr[1],(desc_osd[pid].ulAddr[1] + desc_osd[pid].ulBufWidth*pos_y/2 + pos_x),draw_width_rtc*MaxOSDFontH/2,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg2Rtc);
	        }
	        else {
	            MMPF_OSD_DmaMoveData(desc_osd_rtc.ulAddr[0],(desc_osd[pid].ulAddr[0] + desc_osd[pid].ulBufWidth*(desc_osd[pid].ulBufHeight - MaxOSDFontH - pos_y) + pos_x),draw_width_rtc*MaxOSDFontH,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg1Rtc);
	            MMPF_OSD_DmaMoveData(desc_osd_rtc.ulAddr[1],(desc_osd[pid].ulAddr[1] + desc_osd[pid].ulBufWidth*(desc_osd[pid].ulBufHeight - MaxOSDFontH - pos_y)/2 + pos_x),draw_width_rtc*MaxOSDFontH/2,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg2Rtc);
	        }
				}
    }
    #endif

    #if AIT_STRING_EN
    if (osd_str->type == AIT_OSD_TYPE_CUST_STRING) {


        if(( desc_osd[pid].ulBufWidth > 1280) || ( desc_osd[pid].ulBufHeight > 720)) {
            LineOfstArg1Str.ulSrcWidth  = draw_width_str;
            LineOfstArg1Str.ulSrcOffset = draw_width_str;
            LineOfstArg1Str.ulDstWidth  = draw_width_str;
            LineOfstArg1Str.ulDstOffset = 1920;

            LineOfstArg2Str.ulSrcWidth  = draw_width_str;
            LineOfstArg2Str.ulSrcOffset = draw_width_str;
            LineOfstArg2Str.ulDstWidth  = draw_width_str;
            LineOfstArg2Str.ulDstOffset = 1920;
        }
        else if(( desc_osd[pid].ulBufWidth > 640) || ( desc_osd[pid].ulBufHeight > 480)) {
            LineOfstArg1Str.ulSrcWidth  = draw_width_str;
            LineOfstArg1Str.ulSrcOffset = draw_width_str;
            LineOfstArg1Str.ulDstWidth  = draw_width_str;
            LineOfstArg1Str.ulDstOffset = 1280;

            LineOfstArg2Str.ulSrcWidth  = draw_width_str;
            LineOfstArg2Str.ulSrcOffset = draw_width_str;
            LineOfstArg2Str.ulDstWidth  = draw_width_str;
            LineOfstArg2Str.ulDstOffset = 1280;
        }else {
        	LineOfstArg1Str.ulSrcWidth  = draw_width_str;
            LineOfstArg1Str.ulSrcOffset = draw_width_str;
            LineOfstArg1Str.ulDstWidth  = draw_width_str;
            LineOfstArg1Str.ulDstOffset = 640;

            LineOfstArg2Str.ulSrcWidth  = draw_width_str;
            LineOfstArg2Str.ulSrcOffset = draw_width_str;
            LineOfstArg2Str.ulDstWidth  = draw_width_str;
            LineOfstArg2Str.ulDstOffset = 640;
        }

        off_x = (osd_str->pos[AXIS_X]);  

        if ( UI_Get_Str_Pos_startX() == 1) {
            pos_x = ( desc_osd[pid].ulBufWidth - draw_width_str - off_x );
        }

        if ( UI_Get_Str_Pos_startX() == 0) {
            pos_x = off_x;
        }

        pos_y = (osd_str->pos[AXIS_Y]);  
				if (draw_width_str > 0) {
	        if ( UI_Get_Str_Pos_startY() == 1) {
	            MMPF_OSD_DmaMoveData(desc_osd_str.ulAddr[0],(desc_osd[pid].ulAddr[0] + desc_osd[pid].ulBufWidth*pos_y + pos_x),draw_width_str*MaxOSDFontH,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg1Str);
	            MMPF_OSD_DmaMoveData(desc_osd_str.ulAddr[1],(desc_osd[pid].ulAddr[1] + desc_osd[pid].ulBufWidth*pos_y/2 + pos_x),draw_width_str*MaxOSDFontH/2,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg2Str);
	        }
	        else {
	            MMPF_OSD_DmaMoveData(desc_osd_str.ulAddr[0],(desc_osd[pid].ulAddr[0] + desc_osd[pid].ulBufWidth*(desc_osd[pid].ulBufHeight - MaxOSDFontH - pos_y) + pos_x),draw_width_str*MaxOSDFontH,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg1Str);
	            MMPF_OSD_DmaMoveData(desc_osd_str.ulAddr[1],(desc_osd[pid].ulAddr[1] + desc_osd[pid].ulBufWidth*(desc_osd[pid].ulBufHeight - MaxOSDFontH - pos_y)/2 + pos_x),draw_width_str*MaxOSDFontH/2,MMPF_OSD_ReleaseDma,NULL, MMP_TRUE, &LineOfstArg2Str);
	        }
      	}
    }
    #endif

    init_flag = 1;

#endif
}
#endif

/*static*/ int aitcam_ipc_stream2obj(int streamid, unsigned long fmt)
{
    MMP_ULONG i, num;
    char      *strfmt;
    aitcam_streamtbl *tbl;

    switch(fmt) {
    #if (V4L2_H264)
    case V4L2_PIX_FMT_H264:
        tbl = vid_stream_tbl;
        num = MAX_H264_STREAM_NUM;
        strfmt = "vid";
        break;
    #endif

    #if (V4L2_AAC)
    case V4L2_PIX_FMT_MPEG:
        tbl = aud_stream_tbl;
        num = MAX_AUD_STREAM_NUM;
        strfmt = "aud";
        break;
    #endif

    #if (V4L2_JPG)
    case V4L2_PIX_FMT_MJPEG:
        tbl = jpg_stream_tbl;
        num = MAX_JPG_STREAM_NUM;
        strfmt = "jpg";
        break;
    #endif

    #if (V4L2_GRAY)
    case V4L2_PIX_FMT_GREY:
        tbl = gray_stream_tbl;
        num = MAX_GRAY_STREAM_NUM;
        strfmt = "gray";
        break;
    case V4L2_PIX_FMT_NV12:
        tbl = gray_stream_tbl;
        num = MAX_GRAY_STREAM_NUM;
        strfmt = "nv12";
        break;
    case V4L2_PIX_FMT_I420:
        tbl = gray_stream_tbl;
        num = MAX_GRAY_STREAM_NUM;
        strfmt = "i420";
        break;
    #endif

    default:
        return V4L2_INVALID_OBJ_ID;
    }

    for(i = 0; i < num; i++) {
        if (tbl[i].used && (tbl[i].streamid == streamid))
            return i;
    }
    printc("no obj for %s stream %d\r\n", strfmt, streamid);
    return V4L2_INVALID_OBJ_ID;
}

/*static*/ unsigned long aitcam_ipc_get_format(int streamid)
{
    aitcam_ipc_info *ipc_info;

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }
    ipc_info = &v4l2_stream[streamid].info;

    return ipc_info->img_info.img_fmt;
}

#if (V4L2_JPG)
static void aitcam_ipc_set_mjpg_propt(  int                 streamid, 
                                        MMPS_JSTREAM_PROPT  *propt,
                                        IPC_STREAMER_OPTION *opt)
{
    aitcam_ipc_info *ipc_info = &v4l2_stream[streamid].info;

    propt->snrID    = PRM_SENSOR;
    propt->w        = ipc_info->img_info.P1.img_width;
    propt->h        = ipc_info->img_info.P2.img_height;
    propt->size     = 1500; //(propt->w * propt->h) >> 4; // TBD
    propt->bufsize  = ipc_info->img_info.max_framesize;

    *opt = IPC_STREAMER_OPT_NONE;
    if (ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_REALTIME_TYPE)
        *opt |= IPC_STREAMER_OPT_FLUSH;
}
#endif

#if (V4L2_AAC)
static void aitcam_ipc_set_audio_propt( int                 streamid,
                                        MMPS_ASTREAM_PROPT  *propt,
                                        IPC_STREAMER_OPTION *opt)
{
    aitcam_ipc_info *ipc_info = &v4l2_stream[streamid].info;

    propt->feat.src     = MMP_AUD_AFE_IN_DIFF;
    propt->feat.fmt     = MMP_ASTREAM_AAC;
    propt->feat.fs      = ipc_info->img_info.P2.samplerate;
    propt->feat.bitrate = ipc_info->img_info.P1.bitrate;
    propt->feat.buftime = 8000; // TBD: 8s
    if (ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_LOOP_RECORDING)
        propt->seamless = MMP_TRUE;
    else
        propt->seamless = MMP_FALSE;

    *opt = IPC_STREAMER_OPT_NONE;
    if (ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_REALTIME_TYPE)
        *opt |= IPC_STREAMER_OPT_FLUSH;

    if (v4l2_debug) {
        printc(" ~fs      : %d\r\n", propt->feat.fs);
        printc(" ~bitrate : %d\r\n", propt->feat.bitrate);
        printc(" ~realtime: %d\r\n", *opt);
    }
}
#endif

#if (V4L2_H264)
extern MMP_ULONG UI_Get_Rtc_Pos_startX(void);
extern MMP_ULONG UI_Get_Rtc_Pos_startY(void);
extern MMP_ULONG UI_Get_Str_Pos_startX(void);
extern MMP_ULONG UI_Get_Str_Pos_startY(void);
static void aitcam_ipc_set_video_propt( int                 streamid,
                                        MMPS_VSTREAM_PROPT  *propt,
                                        IPC_STREAMER_OPTION *opt)
{
    aitcam_ipc_info *ipc_info = &v4l2_stream[streamid].info;
    VIDENC_CTL_SET  *ctrl     = &v4l2_stream[streamid].ctrl_set;
    MMP_VID_FPS     fps;

    MMPS_Sensor_GetCurFpsx10(PRM_SENSOR, &fps.ulResol);
    fps.ulResol    *= 100;
    fps.ulIncr      = 1000;
    fps.ulIncrx1000 = 1000000;
    #if SUPPORT_OSD_FUNC
    propt->rtc_pos_startX = UI_Get_Rtc_Pos_startX();
    propt->rtc_pos_startY = UI_Get_Rtc_Pos_startY();
    propt->str_pos_startX = UI_Get_Str_Pos_startX();
    propt->str_pos_startY = UI_Get_Str_Pos_startY();
    #endif
    propt->snrID        = PRM_SENSOR;
    propt->w            = ipc_info->img_info.P1.img_width;
    propt->h            = ipc_info->img_info.P2.img_height;
    propt->snr_fps      = fps;
    propt->infrmslots   = 2;
    propt->outbuf_ms    = 8000; // TBD: 8s
    propt->ctl          = *ctrl;

    if (ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_LOOP_RECORDING)
        propt->seamless = MMP_TRUE;
    else
        propt->seamless = MMP_FALSE;

    *opt = IPC_STREAMER_OPT_NONE;
    if (ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_REALTIME_TYPE)
        *opt |= IPC_STREAMER_OPT_FLUSH;

    if (v4l2_debug) {
        printc(" ~bitrate : %d\r\n", ctrl->bitrate);
        printc(" ~gop     : %d\r\n", ctrl->gop);
        printc(" ~fps_inc : %d\r\n", ctrl->enc_fps.ulIncr);
        printc(" ~fps_res : %d\r\n", ctrl->enc_fps.ulResol);
        printc(" ~seamless: %d\r\n", propt->seamless);
        printc(" ~realtime: %d\r\n", *opt);
        printc(" ~rcskip  : %d\r\n", ctrl->rc_skip);
        printc(" ~rc_mode : %d\r\n", ctrl->rc_mode);
        printc(" ~profile : %d\r\n", ctrl->profile);
    }
}
#endif

static int aitcam_ipc_open_handle(int streamid)
{
    // Do nothing....
    return 0;
}

static int aitcam_ipc_streamon_handle(int streamid)
{
#if SUPPORT_AEC  
extern void MMPF_AEC_Pause(MMP_BOOL pause);
#endif
  
    MMP_ULONG id;
    MMP_BOOL  success = MMP_FALSE;
    unsigned long fmt = aitcam_ipc_get_format(streamid);

    #if (V4L2_JPG)
    MMPS_JSTREAM_PROPT j_propt;
    #endif
    #if (V4L2_AAC)
    MMPS_ASTREAM_PROPT a_propt;
    #endif
    #if (V4L2_H264)
    MMPS_VSTREAM_PROPT v_propt;
    #endif
    #if (V4L2_JPG)||(V4L2_AAC)||(V4L2_H264)
    IPC_STREAMER_OPTION opt;
    #endif
    #if (V4L2_GRAY)
    aitcam_ipc_info *ipc_info = &v4l2_stream[streamid].info;
    MMP_ULONG width, height;
    #endif

    id = aitcam_ipc_stream2obj(streamid, fmt);
    if (id == V4L2_INVALID_OBJ_ID) {
        v4l2_stream[streamid].state = IPC_V4L2_NOT_SUPPORT;
        return -1;
    }

    switch(fmt) {
    #if (V4L2_JPG)
    case V4L2_PIX_FMT_MJPEG:
        aitcam_ipc_set_mjpg_propt(streamid, &j_propt, &opt);
        if (Jpeg_Open(id, &j_propt) == 0) {
            Jpeg_On(id, streamid, opt);
            success = MMP_TRUE;
        }
        else {
            jpg_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
            jpg_stream_tbl[id].used     = 0;
        }
        break;
    #endif
    #if (V4L2_AAC)
    case V4L2_PIX_FMT_MPEG:
        #if SUPPORT_AEC
        MMPF_AEC_Pause(MMP_TRUE) ;
        #endif
      
        aitcam_ipc_set_audio_propt(streamid, &a_propt, &opt);
        if (Audio_Open(id, &a_propt) == 0) {
            Audio_On(id, streamid, opt);
            success = MMP_TRUE;
        }
        else {
            aud_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
            aud_stream_tbl[id].used     = 0;
        }
        #if SUPPORT_AEC
        MMPF_AEC_Pause(MMP_FALSE) ;
        #endif

        break;
    #endif
    #if (V4L2_H264)
    case V4L2_PIX_FMT_H264:
      #if SUPPORT_AEC
      MMPF_AEC_Pause(MMP_TRUE) ;
      #endif
        aitcam_ipc_set_video_propt(streamid, &v_propt, &opt);
        if (Video_Open(id, &v_propt,STREAM_SELECT_0) == 0) {
            Video_On(id, streamid, opt);
            success = MMP_TRUE;
        }
        else {
            vid_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
            vid_stream_tbl[id].used     = 0;
        }
        #if SUPPORT_OSD_FUNC==1 // OSD test
        if(success && ipc_info->img_info.ctrl_flag & CTRL_FLAG_OSD_EN ) {
            // only stream type = real-time can draw osd
            if ( ipc_info->img_info.streamtype & V4L2_CAMERA_AIT_REALTIME_TYPE ) {
                aitcam_ipc_osd_register_drawer(streamid,fmt, (ipc_osd_drawer *)aitcam_osd_drawer,AIT_OSD_TYPE_FDFRAME) ;
            }
            else {
                aitcam_ipc_osd_register_drawer(streamid,fmt, (ipc_osd_drawer *)0,AIT_OSD_TYPE_FDFRAME ) ;
            }
        }
        #endif
      #if SUPPORT_AEC
      MMPF_AEC_Pause(MMP_FALSE) ;
      #endif

        break;
    #endif
    #if (V4L2_GRAY)
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_I420:
    case V4L2_PIX_FMT_NV12:
        width  = ipc_info->img_info.P1.img_width;
        height = ipc_info->img_info.P2.img_height;
        if (Gray_Open(id, width, height,fmt) == 0) {
            Gray_On(id, streamid);
            success = MMP_TRUE;
        }
        else {
            gray_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
            gray_stream_tbl[id].used     = 0;
        }
        #if SUPPORT_OSD_FUNC==1
        if ( success ){
            int i ;
            for(i=0;i<MMP_IBC_PIPE_MAX;i++){
                desc_osd[i].ulSrcWidth  = width ;
                desc_osd[i].ulSrcHeight = height ;
            }
            draw_fdfr_osd = MMP_TRUE;            
        }
        else {
           draw_fdfr_osd = MMP_FALSE ;

        }
        #endif
        break;
    #endif
    default:
        printc("unknown stream fmt x%x\r\n", fmt);
        return -2;
    }

    if (success)
        return 0;

    // failure case, change state to skip off & release actions
    v4l2_stream[streamid].state = IPC_V4L2_NOT_SUPPORT;
    return -2;
}

static int aitcam_ipc_streamoff_handle(int streamid)
{
    MMP_ULONG id;
    unsigned long fmt = aitcam_ipc_get_format(streamid);

    if (v4l2_stream[streamid].state != IPC_V4L2_STREAMOFF)
        return -1;

    id = aitcam_ipc_stream2obj(streamid, fmt);
    if (id == V4L2_INVALID_OBJ_ID)
        return -1;

    switch(fmt) {
    #if (V4L2_JPG)
    case V4L2_PIX_FMT_MJPEG:
        Jpeg_Off(id);
        break;
    #endif
    #if (V4L2_AAC)
    case V4L2_PIX_FMT_MPEG:
        Audio_Off(id);
        break;
    #endif
    #if (V4L2_H264)
    case V4L2_PIX_FMT_H264:
        Video_Off(id);
        break;
    #endif
    #if (V4L2_GRAY)
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_I420:
    case V4L2_PIX_FMT_NV12:
        Gray_Off(id);
        break;
    #endif
    default:
        printc("unknown stream fmt x%x\r\n", fmt);
        return -2;
    }

    return 0;
}

static int aitcam_ipc_release_handle(int streamid)
{
    MMP_ULONG id;
    unsigned long fmt = aitcam_ipc_get_format(streamid);

    if (v4l2_stream[streamid].state != IPC_V4L2_RELEASE)
        return -1;

    id = aitcam_ipc_stream2obj(streamid, fmt);
    if (id == V4L2_INVALID_OBJ_ID)
        return -1;

    switch(fmt) {
    #if (V4L2_JPG)
    case V4L2_PIX_FMT_MJPEG:
        Jpeg_Close(id);
        jpg_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
        jpg_stream_tbl[id].used     = 0;
        break;
    #endif
    #if (V4L2_AAC)
    case V4L2_PIX_FMT_MPEG:
        Audio_Close(id);
        aud_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
        aud_stream_tbl[id].used     = 0;
        break;
    #endif
    #if (V4L2_H264)
    case V4L2_PIX_FMT_H264:
        Video_Close(id);
        vid_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
        vid_stream_tbl[id].used     = 0;
        break;
    #endif
    #if (V4L2_GRAY)
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_I420:
    case V4L2_PIX_FMT_NV12:
        Gray_Close(id);
        gray_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
        gray_stream_tbl[id].used     = 0;
        break;
    #endif
    default:
        printc("unknown stream fmt x%x\r\n", fmt);
        return -1;
    }

    return 0;
}

static void V4L2Task_init(void)
{
    int s;

    /* Reset all V4L2 streams initial states */
    for(s = 0; s < AITCAM_NUM_CONTEXTS; s++) {
        v4l2_stream[s].state = IPC_V4L2_NOT_SUPPORT;
        v4l2_stream[s].frame_seq = 0;
    }

    #if (V4L2_H264)
    for(s = 0; s < MAX_H264_STREAM_NUM; s++) {
        vid_stream_tbl[s].streamid  = AITCAM_NUM_CONTEXTS;
        vid_stream_tbl[s].used      = 0;
    }
    #endif

    #if (V4L2_AAC)
    for(s = 0; s < MAX_AUD_STREAM_NUM; s++) {
        aud_stream_tbl[s].streamid  = AITCAM_NUM_CONTEXTS;
        aud_stream_tbl[s].used      = 0;
    }
    #endif

    #if (V4L2_JPG)
    for(s = 0; s < MAX_JPG_STREAM_NUM; s++) {
        jpg_stream_tbl[s].streamid  = AITCAM_NUM_CONTEXTS;
        jpg_stream_tbl[s].used      = 0;
    }
    #endif

    #if (V4L2_GRAY)
    for(s = 0; s < MAX_GRAY_STREAM_NUM; s++) {
        gray_stream_tbl[s].streamid = AITCAM_NUM_CONTEXTS;
        gray_stream_tbl[s].used     = 0;
    }
    #endif
}

void DualCpu_V4L2Task(void* pData)
{
    MMP_ULONG swap_info_dma_addr = DRAM_NONCACHE_VA((MMP_ULONG)V4L2_PAYLOAD_INFO);
    MMP_ULONG cmd;
    MMP_ULONG SocketRet = 0;
    int streamid;
    int (*cmd_handle)(int) = NULL;
    int cmd_argu = 0;
    cpu_comm_transfer_data *V4L2_CPU_Reg = &_V4L2_Proc; 
    cpu_comm_transfer_data *V4L2_Return  = &_V4L2_Return;

    V4L2Task_init();

    //printc("DualCpu_V4L2 Start\r\n");
    RTNA_DBG_Str0("[V4L2]\r\n");

    if (0) { //(v4l2_debug) {
        printc("sizeof(AITCAM_VIDBUF_QUEUE): %d\r\n", sizeof(AITCAM_VIDBUF_QUEUE));
        printc("sizeof(AITCAM_VIDBUF_INFO) : %d\r\n", sizeof(AITCAM_VIDBUF_INFO));
        printc("sizeof(AITCAM_VIDENC_QUEUE): %d\r\n", sizeof(AITCAM_VIDENC_QUEUE));
    }
    
    CpuComm_RegisterEntry(CPU_COMM_ID_V4L2A2B, CPU_COMM_TYPE_SOCKET);
    CpuComm_RegisterISRService(CPU_COMM_ID_V4L2A2B, 0);

    // Enable sender no ack cmd
    while(1) {

        // recv & ack 
        SocketRet = CpuComm_SocketReceive(  CPU_COMM_ID_V4L2A2B,
                                            (MMP_UBYTE *)V4L2_CPU_Reg,
                                            sizeof(cpu_comm_transfer_data),
                                            MMP_TRUE);

        if (SocketRet  == CPU_COMM_ERR_NONE)
        {
            int ret = 0;
            int post_ret = 1; // return result after cmd done
            aitcam_img_info *img_info;
            aitcam_ipc_info *ipc_info;
            aitcam_ipc_ctrl *ipc_ctrl;

            V4L2_CPU_Reg->phy_addr = DRAM_NONCACHE_VA(V4L2_CPU_Reg->phy_addr);
            V4L2_Return->phy_addr  = swap_info_dma_addr;

            cmd = (V4L2_CPU_Reg->command & 0x7FFFFFFF);
            
            cmd_handle = NULL;

            switch(cmd) {
            case IPC_V4L2_GET_JPG:
            {
                MMP_ULONG jpg_addr,jpg_size ;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                V4L2_Return->command = cmd;
                if(MMPF_JStream_GetFrame(0,&jpg_addr,&jpg_size)==MMP_ERR_NONE ) {                  
                  V4L2_Return->result     = 0 ;
                  V4L2_Return->phy_addr   = jpg_addr;
                  V4L2_Return->size       = jpg_size;
                }  
                else {
                  V4L2_Return->result     = 1 ;
                }
                break;  
            }
            case IPC_IQ_TUNING:
                ret = aitcam_ipc_set_iq( (void *)V4L2_CPU_Reg->phy_addr );
                V4L2_Return->command = cmd;
                V4L2_Return->result     = ret;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                break;    
            case IPC_V4L2_OPEN:
                img_info = (aitcam_img_info *)V4L2_CPU_Reg->phy_addr;
                streamid = img_info->streamid;

                ret = aitcam_ipc_open(img_info);
                V4L2_Return->command = cmd;
                V4L2_Return->result     = ret;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                if (ret == 0) {
                    cmd_argu    = streamid;
                    cmd_handle  = aitcam_ipc_open_handle;
                }
                break;

            case IPC_V4L2_STREAMON:
                ipc_info = (aitcam_ipc_info *)V4L2_CPU_Reg->phy_addr;
                streamid = ipc_info->img_info.streamid;

                ret = aitcam_ipc_streamon(ipc_info);
                V4L2_Return->command = cmd;
                V4L2_Return->result     = ret;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                if (ret == 0) {
                    cmd_argu    = streamid;
                    cmd_handle  = aitcam_ipc_streamon_handle;
                }
                break;

            case IPC_V4L2_STREAMOFF:
                streamid =(int)DRAM_NONCACHE_VA(V4L2_CPU_Reg->phy_addr);

                ret = aitcam_ipc_streamoff(streamid);
                V4L2_Return->command = cmd;
                V4L2_Return->result     = ret;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                if (ret == 0) {
                    cmd_argu    = streamid;
                    cmd_handle  = aitcam_ipc_streamoff_handle;
                }
                break;

            case IPC_V4L2_RELEASE:
                streamid = (int)DRAM_NONCACHE_VA(V4L2_CPU_Reg->phy_addr);

                ret = aitcam_ipc_release(streamid);
                V4L2_Return->command = cmd;
                V4L2_Return->result     = ret;
                V4L2_Return->phy_addr   = 0;
                V4L2_Return->size       = 0;
                if (ret == 0) {
                    cmd_argu    = streamid;
                    cmd_handle  = aitcam_ipc_release_handle;
                }
                break;

            case IPC_V4L2_CTRL:
            {
                aitcm_ipc_ctrl_dir dir = ( V4L2_CPU_Reg->command >> 31) & 1 ;
                
                ipc_ctrl = (aitcam_ipc_ctrl *)V4L2_CPU_Reg->phy_addr;
                if( dir == IPC_V4L2_SET ) { 
                  ret = aitcam_ipc_set_ctrl(ipc_ctrl);
                  V4L2_Return->phy_addr   = 0;
                }
                else {
                  ret = aitcam_ipc_get_ctrl(ipc_ctrl);
                  V4L2_Return->phy_addr   = ipc_ctrl->val;
                }
                V4L2_Return->command = cmd;
                V4L2_Return->result     = (unsigned long) ret;
                
                V4L2_Return->size       = 0;
                break;
            }
            default:
                printc("no such command\r\n");
                V4L2_Return->command = IPC_V4L2_NOT_SUPPORT;
                break;
            }
            if (post_ret) {
                if (V4L2_Return->result < 0) {
                    printc("CMD %d err: 0x%x\r\n", cmd, V4L2_Return->result);
                }

                V4L2_Return->flag = CPUCOMM_FLAG_CMDSND;
                // cpu_a didnot know the cpub's cachable address
                // so go back to non-cachable address
                V4L2_Return->phy_addr = DRAM_NONCACHE_VA(V4L2_Return->phy_addr);
                CpuComm_SocketSend( CPU_COMM_ID_V4L2B2A,
                                    (MMP_UBYTE *)V4L2_Return,
                                    sizeof(cpu_comm_transfer_data));
            }

            // Execute the command handler
            if (cmd_handle)
                cmd_handle(cmd_argu);
        }
        else {
            printc("SocketRet: %x\r\n", SocketRet);
        }
    }

    printc("Task Stop\r\n");

    // Kill task by itself
    MMPF_OS_DeleteTask(OS_PRIO_SELF);
}

void EstablishV4L2(void)
{
	MMPF_TASK_CFG   task_cfg;

	task_cfg.ubPriority = TASK_B_V4L2_PRIO;
	task_cfg.pbos = (MMP_ULONG)&s_V4L2TaskStk[0];
	task_cfg.ptos = (MMP_ULONG)&s_V4L2TaskStk[TASK_B_V4L2_STK_SIZE-1];
	MMPF_OS_CreateTask(DualCpu_V4L2Task, &task_cfg, (void *)0);
}

void aitcam_ipc_debug(int dbg_on)
{
    v4l2_debug = dbg_on;
}

void aitcam_ipc_debug_ts(int dbg_on)
{
    v4l2_debug_ts = dbg_on;
}

int aitcam_ipc_is_debug_ts(void)
{
    return v4l2_debug_ts ; 
}

void aitcam_ipc_debug_osd(int dbg_on)
{
    v4l2_debug_osd =  dbg_on;
}

int aitcam_ipc_is_debug_osd(void)
{
    return v4l2_debug_osd ; 
}


int aitcam_ipc_state(int streamid)
{
    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }

    return v4l2_stream[streamid].state;
}

int aitcam_ipc_open(aitcam_img_info *img_info)
{
    int streamid = img_info->streamid;
    MMP_ULONG id;
    MMP_BOOL  success = MMP_FALSE, seamless;
    char      *fmt = "?";
#if MIPI_TEST_PARAM_EN==1
    mipi_test_ng=0;
#endif
    printc("vipc_open(%d)\r\n", streamid);

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }
    else if ((v4l2_stream[streamid].state != IPC_V4L2_NOT_SUPPORT) &&
            (v4l2_stream[streamid].state != IPC_V4L2_RELEASE)) {
        return -2;
    }

    switch(img_info->img_fmt) {
    #if (V4L2_H264)
    case V4L2_PIX_FMT_H264:
        seamless =  img_info->streamtype & V4L2_CAMERA_AIT_LOOP_RECORDING ?
                    MMP_TRUE : MMP_FALSE;
        id = Video_Init(MMP_TRUE, seamless);
        if (id < MAX_H264_STREAM_NUM) {
            vid_stream_tbl[id].streamid = streamid;
            vid_stream_tbl[id].used     = 1;
            aitcam_ipc_init_vidctrl(streamid);
            success = MMP_TRUE;
            fmt = "H264";
        }
        break;
    #endif

    #if (V4L2_JPG)
    case V4L2_PIX_FMT_MJPEG:
        id = Jpeg_Init(MMP_TRUE);
        if (id < MAX_JPG_STREAM_NUM) {
            jpg_stream_tbl[id].streamid = streamid;
            jpg_stream_tbl[id].used     = 1;
            success = MMP_TRUE;
            fmt = "JPEG";
        }
        break;
    #endif

    #if (V4L2_GRAY)
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_I420:
    case V4L2_PIX_FMT_NV12:
        id = Gray_Init(MMP_TRUE);
        if (id < MAX_GRAY_STREAM_NUM) {
            gray_stream_tbl[id].streamid = streamid;
            gray_stream_tbl[id].used     = 1;
            success = MMP_TRUE;
            fmt = "GRAY";
        }
        break;
    #endif

    #if (V4L2_AAC)
    case V4L2_PIX_FMT_MPEG:
        seamless =  img_info->streamtype & V4L2_CAMERA_AIT_LOOP_RECORDING ?
                    MMP_TRUE : MMP_FALSE;
        id = Audio_Init(MMP_TRUE, seamless);
        if (id < MAX_AUD_STREAM_NUM) {
            aud_stream_tbl[id].streamid = streamid;
            aud_stream_tbl[id].used     = 1;
            success = MMP_TRUE;
            fmt = "MPEG";
        }
        break;
    }
    #endif

    if (success) {
        v4l2_stream[streamid].info.img_info = *img_info;
        v4l2_stream[streamid].state = IPC_V4L2_OPEN;
        if (v4l2_debug) {
            printc(" -obj_id           : %d\r\n",    id);
            printc(" -width/bitrate    : %d\r\n",    img_info->P1.img_width);
            printc(" -height/sampelrate: %d\r\n",    img_info->P2.img_height);
            printc(" -format           : %s\r\n",    fmt);
            printc(" -maxframesize     : %d\r\n",    img_info->max_framesize);
            printc(" -streamtype       : x%04x\r\n", img_info->streamtype);
        }
        return 0;
    }

    // failure case
    v4l2_stream[streamid].state = IPC_V4L2_NOT_SUPPORT;
    return -1;
}

int aitcam_ipc_streamon(aitcam_ipc_info *ipc_info) 
{
    int streamid;
    aitcam_ipc_info *stream_info;

    streamid = ipc_info->img_info.streamid;
    stream_info = &v4l2_stream[streamid].info;

    printc("vipc_on(%d)-ticks:%d\r\n", streamid , OSTime );

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }
    else if ((v4l2_stream[streamid].state != IPC_V4L2_OPEN) &&
            (v4l2_stream[streamid].state != IPC_V4L2_STREAMOFF)) {
        return -2;
    }

    if (v4l2_debug) {
        printc(" -streamtype: %d\r\n",      stream_info->img_info.streamtype);
        printc(" -ctrl_flag: %d\r\n",      stream_info->img_info.ctrl_flag);
        printc(" -que.phy   : 0x%08x\r\n",  ipc_info->vbq_phy_addr);
        printc(" -que.virt  : 0x%08x\r\n",  ipc_info->vbq_virt_addr);
        printc(" -que.size  : 0x%08x\r\n",  ipc_info->remote_q_size);
    }

    if (ipc_info->remote_q_size != sizeof(AITCAM_VIDBUF_QUEUE)) {
        printc("#Error : queue struct is not consistence(r=%d,l=%d)\r\n",
                ipc_info->remote_q_size, sizeof(AITCAM_VIDENC_QUEUE));
        return -1;
    }

    v4l2_stream[streamid].frame_seq = 0;

    // don't overwrite img_info setting at open    
    stream_info->vbq_phy_addr   = ipc_info->vbq_phy_addr;
    stream_info->vbq_virt_addr  = ipc_info->vbq_virt_addr;
    stream_info->remote_q_size  = ipc_info->remote_q_size;

    switch(ipc_info->img_info.img_fmt) {
    case V4L2_PIX_FMT_H264:
        // TBD
        break;
    case V4L2_PIX_FMT_MJPEG:
        // TBD
        break;
    case V4L2_PIX_FMT_GREY:
        // TBD
        break;
    case V4L2_PIX_FMT_MPEG:
        // TBD
        break;
    }

    v4l2_stream[streamid].state = IPC_V4L2_STREAMON;

    return 0;
}

int aitcam_ipc_streamoff(int streamid)
{
    printc("vipc_soff(%d)\r\n", streamid);

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }
    else if (v4l2_stream[streamid].state != IPC_V4L2_STREAMON) {
        return -2;
    }

    v4l2_stream[streamid].state = IPC_V4L2_STREAMOFF;

    switch(aitcam_ipc_get_format(streamid)) {
    case V4L2_PIX_FMT_H264:
    #if AIT_FDFRAME_EN
        aitcam_ipc_osd_register_drawer(streamid,V4L2_PIX_FMT_H264, (ipc_osd_drawer *)0,AIT_OSD_TYPE_FDFRAME ) ;
    #endif    
        break;

    case V4L2_PIX_FMT_MJPEG:
        break;

    case V4L2_PIX_FMT_GREY:
    #if AIT_FDFRAME_EN
        draw_fdfr_osd = MMP_FALSE ;
    #endif
        break;

    case V4L2_PIX_FMT_MPEG:
        break;
    }

    return 0;
}

int aitcam_ipc_release(int streamid) 
{
    printc("vipc_rel(%d)\r\n", streamid);

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return -1;
    }
    else if (v4l2_stream[streamid].state == IPC_V4L2_OPEN) {
        // do nothing in open stage, so ignore the following release action
        v4l2_stream[streamid].state = IPC_V4L2_RELEASE;
        return -2;
    }
    else if (v4l2_stream[streamid].state != IPC_V4L2_STREAMOFF) {
        return -3;
    }

    switch(aitcam_ipc_get_format(streamid)) {
    case V4L2_PIX_FMT_H264:
        // TBD
        break;
    case V4L2_PIX_FMT_MJPEG:
        // TBD
        break;
    case V4L2_PIX_FMT_GREY:
        break;
    case V4L2_PIX_FMT_MPEG:
        // AAC only
        break;
    }

    v4l2_stream[streamid].state = IPC_V4L2_RELEASE;
#if MIPI_TEST_PARAM_EN==1
    if(mipi_test_ng) {
      printc("----------[MIPI] : P frame check is too BIG\r\nsystem halt\r\n");
      //while(1);
    }
#endif
    return 0;
}

int aitcam_is_streamer_alive(void)
{
  return v4l2_streamer_status ;
}

static void aitcam_ipc_init_vidctrl(int streamid)
{
    VIDENC_CTL_SET *ctrl = &v4l2_stream[streamid].ctrl_set;

    ctrl->bitrate               = 8000000;
    ctrl->gop                   = 30;
    ctrl->lb_size               = 1000;
    ctrl->enc_fps.ulIncr        = 1000;
    ctrl->enc_fps.ulIncrx1000   = 1000000;
    ctrl->enc_fps.ulResol       = 30000;
    ctrl->profile               = H264ENC_BASELINE_PROFILE;
    ctrl->level                 = 40;
    ctrl->entropy               = H264ENC_ENTROPY_CAVLC;
    ctrl->rc_mode               = VIDENC_RC_MODE_VBR;
    ctrl->rc_skip               = MMP_FALSE;
    ctrl->rc_skiptype           = VIDENC_RC_SKIP_SMOOTH;
    ctrl->force_idr             = MMP_FALSE;
    ctrl->tnr                   = 0;
    ctrl->qp_init[0]            =
    ctrl->qp_init[1]            = 36;
    ctrl->qp_bound[0]           = 10; // low bound
    ctrl->qp_bound[1]           = 46; // high bound
}


int aitcam_ipc_camera_ctrl(unsigned long ctrl_id, int val)
{
    //printc("ctrl id : %08x\r\n",ctrl_id );
    
    /*
    skip to set if sensor & isp is not initialized
    */
    if (MMPS_Sensor_IsInitialized(PRM_SENSOR) == MMP_FALSE) {
        return  0 ;
    }
    
    switch(ctrl_id) {
    case V4L2_CID_BRIGHTNESS:
        //printc("--brightness: %d\r\n",val);
        ISP_IF_F_SetBrightness( val );
        break;
    case V4L2_CID_CONTRAST:
        //printc("--contrast: %d\r\n",val);
        ISP_IF_F_SetContrast( val );
        break;
    case V4L2_CID_GAMMA:
        //printc("--gamma: %d\r\n",val);
        ISP_IF_F_SetGamma( val );
        break;
    case V4L2_CID_SHARPNESS:
        //printc("--sharpness: %d\r\n",val);
        ISP_IF_F_SetSharpness( val );
        break;
    case V4L2_CID_BACKLIGHT_COMPENSATION:
        //printc("--wdr : %d\r\n",val);
        ISP_IF_F_SetWDR((val) ? 255 : 0);
        break ;
    case V4L2_CID_HUE:
        //printc("--hue: %d\r\n",val);
        ISP_IF_F_SetHue( val );
        break;
    case V4L2_CID_SATURATION:
        //printc("--saturation: %d\r\n",val);
        ISP_IF_F_SetSaturation( val );
        break;
    case V4L2_CID_POWER_LINE_FREQUENCY:
    {
        ISP_AE_FLICKER fkr = 0;
        switch(val) {
        case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
            fkr = ISP_AE_FLICKER_OFF ;
            break;
        case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
            fkr = ISP_AE_FLICKER_50HZ ;
            break ;
        case V4L2_CID_POWER_LINE_FREQUENCY_60HZ:
        case V4L2_CID_POWER_LINE_FREQUENCY_AUTO:
        default:
            fkr = ISP_AE_FLICKER_60HZ ;
            break;
        }
        if (fkr != ISP_IF_AE_GetFlicker()) {
            ISP_IF_AE_SetFlicker(fkr);
        }
        //printc("--flicker : %d\r\n",fkr );
        break ;
    }    
    case V4L2_CID_EXPOSURE:
        //printc("--exposure : %d\r\n",val );
        ISP_IF_AE_SetEV(val);
        break ;
    case V4L2_CID_CAMERA_AIT_ORIENTATION:
        switch (val)
        {
            case V4L2_CAMERA_AIT_ORTN_FLIP:
                MMPS_Sensor_SetFlip(PRM_SENSOR, MMPF_SENSOR_COLUMN_FLIP);
                break;
            case V4L2_CAMERA_AIT_ORTN_MIRROR:
                MMPS_Sensor_SetFlip(PRM_SENSOR, MMPF_SENSOR_ROW_FLIP);
                break;
            case V4L2_CAMERA_AIT_ORTN_FLIP_MIRROR :
                MMPS_Sensor_SetFlip(PRM_SENSOR, MMPF_SENSOR_COLROW_FLIP);
                break;
            case V4L2_CAMERA_AIT_ORTN_NORMAL:
            default:
                MMPS_Sensor_SetFlip(PRM_SENSOR, MMPF_SENSOR_NO_FLIP);
                break;
        }
        break ;
    case V4L2_CID_CAMERA_AIT_NIGHT_VISION:
        //printc("--nightvision : %d\r\n",val );
        SetNightVision(val) ;
        break ;
    case V4L2_CID_CAMERA_AIT_IR_LED:
    break ;
    case V4L2_CID_CAMERA_AIT_IR_SHUTTER:
    break;
    case V4L2_CID_CAMERA_AIT_STREAMER_ALIVE:
      printc("--streamer alive : %d\r\n",val );
      v4l2_streamer_status = val ;
      break;  
    default:
    break;
    }
    return 0;
}

#if (V4L2_H264)
int aitcam_ipc_vid_ctrl(int streamid, unsigned long ctrl_id, int val)
{
#define FPSINC2RTOS(x) (x*100)
    int             id;
    IPC_VIDEO_CTL   ctrl = IPC_VIDCTL_NUM;
    VIDENC_CTL_SET  *param = &v4l2_stream[streamid].ctrl_set;
    static MMP_BOOL fps_inc_set = MMP_FALSE, fps_res_set = MMP_FALSE;
    static MMP_BOOL qp_i_set = MMP_FALSE, qp_p_set = MMP_FALSE;
    static MMP_BOOL qp_low_set = MMP_FALSE, qp_high_set = MMP_FALSE;

    /* Apply video control only when streaming opened or started */
    if ((v4l2_stream[streamid].state != IPC_V4L2_OPEN) &&
        (v4l2_stream[streamid].state != IPC_V4L2_STREAMON))
        return -1;

    id = aitcam_ipc_stream2obj(streamid, V4L2_PIX_FMT_H264);
    if (id == V4L2_INVALID_OBJ_ID)
        return -1;

    switch(ctrl_id) {
    case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
        if (val)
            param->rc_mode = VIDENC_RC_MODE_VBR;
        else
            param->rc_mode = VIDENC_RC_MODE_LOWBR;
        ctrl = IPC_VIDCTL_RC_MODE;
        break;

    case V4L2_CID_MPEG_AIT_VIDEO_RC_FRAME_SKIP:
        if (val)
            param->rc_skip = MMP_TRUE;
        else
            param->rc_skip = MMP_FALSE;
        ctrl = IPC_VIDCTL_RC_SKIP;
        break;

    case V4L2_CID_MPEG_AIT_RC_SKIP_TYPE:
        if (val)
            param->rc_skiptype = VIDENC_RC_SKIP_SMOOTH;
        else
            param->rc_skiptype = VIDENC_RC_SKIP_DIRECT;
        ctrl = IPC_VIDCTL_RC_SKIP_TYPE;
        break;

    case V4L2_CID_MPEG_AIT_VIDEO_FPS_RESOL:
    case V4L2_CID_MPEG_AIT_VIDEO_FPS_INCR:
        if (ctrl_id == V4L2_CID_MPEG_AIT_VIDEO_FPS_RESOL) {
            param->enc_fps.ulResol = FPSINC2RTOS(val);
            fps_res_set = MMP_TRUE;
        }
        else {
            param->enc_fps.ulIncr      = FPSINC2RTOS(val);
            param->enc_fps.ulIncrx1000 = param->enc_fps.ulIncr * 1000;
            fps_inc_set = MMP_TRUE;
        }
        /* Update encode fps only if both FPS_RESOL & FPS_INCR are set */
        if (fps_res_set && fps_inc_set) {
            fps_res_set = fps_inc_set = MMP_FALSE;
            ctrl = IPC_VIDCTL_ENC_FPS;
        }
        break;

    case V4L2_CID_MPEG_VIDEO_H264_I_PERIOD:
        param->gop = val;
        ctrl = IPC_VIDCTL_GOP;
        break;

    case V4L2_CID_MPEG_VIDEO_BITRATE:
        param->bitrate = val;
        ctrl = IPC_VIDCTL_BITRATE;
        break;

    case V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE:
    #if V4L2_CPB_AS_LBS==0
        val = val << 10;
        param->lb_size = CPB_SIZE_2_LB_SIZE(val, param->bitrate >> 10);
    #else
        param->lb_size = val ;
    #endif    
        ctrl = IPC_VIDCTL_LB_SIZE;
        break;
//
// not allowed to switch profile when untra-low power mode
//        
#if PLL_FOR_ULTRA_LOWPOWER==1
    case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
        switch(val) {
        case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
            param->profile = H264ENC_MAIN_PROFILE;
            break;
        case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
            param->profile = H264ENC_HIGH_PROFILE;
            break;
        case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
        default:
            param->profile = H264ENC_BASELINE_PROFILE;
            break;
        }
        ctrl = IPC_VIDCTL_PROFILE;
        break;

    case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
        param->level = h264_level[val];
        ctrl = IPC_VIDCTL_LEVEL;
        break;

    case V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE:
        if (val == V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC)
            param->entropy = H264ENC_ENTROPY_CABAC;
        else
            param->entropy = H264ENC_ENTROPY_CAVLC;
        ctrl = IPC_VIDCTL_ENTROPY;
        break;
#endif
    case V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP:
    case V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP:
        if (ctrl_id == V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP) {
            param->qp_init[0] = val;
            qp_i_set = MMP_TRUE;
        }
        else {
            param->qp_init[1] = val;
            qp_p_set = MMP_TRUE;
        }
        if (qp_i_set && qp_p_set) {
            ctrl = IPC_VIDCTL_QP_INIT;
            qp_i_set = qp_p_set = MMP_FALSE;
        }
        break;

    case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
    case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
        if (ctrl_id == V4L2_CID_MPEG_VIDEO_H264_MIN_QP) {
            param->qp_bound[0] = val;
            qp_low_set = MMP_TRUE;
        }
        else {
            param->qp_bound[1] = val;
            qp_high_set = MMP_TRUE;
        }
        if (qp_low_set && qp_high_set) {
            ctrl = IPC_VIDCTL_QP_BOUND;
            qp_low_set = qp_high_set = MMP_FALSE;
        }
        break;

    case V4L2_CID_MPEG_AIT_VIDEO_FORCE_FRAME:
        switch (val) {
        case V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_IDR_RESYNC:
        case V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_IDR:
        case V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_I_RESYNC:
        case V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_I:
            param->force_idr = MMP_TRUE;
            break;
        default:
            param->force_idr = MMP_FALSE;
            break;
        }
        ctrl = IPC_VIDCTL_FORCE_I;
        break;

    case V4L2_CID_MPEG_AIT_TNR:
        param->tnr = 0;
        if (val & V4L2_H264_TNR_ZERO_MV_EN)
            param->tnr |= TNR_ZERO_MV_EN;
        if (val & V4L2_H264_TNR_LOW_MV_EN)
            param->tnr |= TNR_LOW_MV_EN;
        if (val & V4L2_H264_TNR_HIGH_MV_EN)
            param->tnr |= TNR_HIGH_MV_EN;

        ctrl = IPC_VIDCTL_TNR;
        break;
    }

    /* if streaming is not turned on yet, only keep the video control param.
     * and only need to apply the control run-time by Video_Control while
     * streaming already.
     */
    if ((v4l2_stream[streamid].state == IPC_V4L2_STREAMON) &&
        (ctrl < IPC_VIDCTL_NUM))
        Video_Control(id, ctrl, param);

    return 0;
}
#endif

int aitcam_ipc_get_ctrl(aitcam_ipc_ctrl *ctrl)
{
extern unsigned int TempSensorVal(void) ;
    aitcam_img_info *img_info = &v4l2_stream[ctrl->streamid].info.img_info;
    int streamid = ctrl->streamid, id;
    switch(ctrl->id) {
    case V4L2_CID_CAMERA_AIT_GET_TEMPERATURE:
      ctrl->val = TempSensorVal();
      break ;    
    }
    return 0 ;
}

int aitcam_ipc_set_ctrl(aitcam_ipc_ctrl *ctrl)
{
    aitcam_img_info *img_info = &v4l2_stream[ctrl->streamid].info.img_info;
    int streamid = ctrl->streamid, id;
    unsigned long fmt = img_info->img_fmt;
    MMPS_VSTREAM_PROPT *vpropt;
    MMPS_ASTREAM_PROPT *apropt;

    #if (V4L2_VIDCTL_DBG)
    printc("ctrl(0x%08x).name: %s, .val: %d\r\n",  ctrl->id,
                                                    ctrl->name, ctrl->val);
    #endif

    // format independent
    switch(ctrl->id) {
    case V4L2_CID_BRIGHTNESS:
    case V4L2_CID_CONTRAST:
    case V4L2_CID_GAMMA:
    case V4L2_CID_SHARPNESS:
    case V4L2_CID_BACKLIGHT_COMPENSATION:
    case V4L2_CID_HUE:
    case V4L2_CID_SATURATION:
    case V4L2_CID_POWER_LINE_FREQUENCY:
    case V4L2_CID_EXPOSURE:
    case V4L2_CID_CAMERA_AIT_ORIENTATION:
    case V4L2_CID_CAMERA_AIT_NIGHT_VISION:
    case V4L2_CID_CAMERA_AIT_IR_LED:
    case V4L2_CID_CAMERA_AIT_IR_SHUTTER:
    case V4L2_CID_CAMERA_AIT_STREAMER_ALIVE:
        // TBD
        aitcam_ipc_camera_ctrl(ctrl->id,ctrl->val);
        break;
    }

    // format dependent
    switch (fmt) {
    #if (V4L2_H264)||(V4L2_AAC)
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_MPEG:
        if (ctrl->id == V4L2_CID_CAMERA_AIT_STREAM_TYPE) {
            MMP_BOOL seamless = MMP_FALSE;

            img_info->streamtype = 0;

            if (ctrl->val & V4L2_CAMERA_AIT_LOOP_RECORDING) {
                seamless = MMP_TRUE;
                img_info->streamtype |= V4L2_CAMERA_AIT_LOOP_RECORDING;
            }
            if (ctrl->val & V4L2_CAMERA_AIT_REALTIME_TYPE) {
                img_info->streamtype |= V4L2_CAMERA_AIT_REALTIME_TYPE;
            }

            if (v4l2_stream[streamid].state == IPC_V4L2_STREAMON) {
                id = aitcam_ipc_stream2obj(streamid, fmt);
                if (id == V4L2_INVALID_OBJ_ID)
                    return 0;

                if (fmt == V4L2_PIX_FMT_H264) {
                    if (Video_GetAttribute(id, &vpropt) == 0)
                        vpropt->seamless = seamless;
                }
                else {
                    if (Audio_GetAttribute(id, &apropt) == 0)
                        apropt->seamless = seamless;
                }
            }
            printc("vipc_ctrl(%d) s-type: %d\r\n", streamid,
                                                     img_info->streamtype);
        }
        #if (V4L2_H264)
        else if (fmt == V4L2_PIX_FMT_H264) {
            aitcam_ipc_vid_ctrl(streamid, ctrl->id, ctrl->val);
        }
        #endif
        break;
    #endif

    case V4L2_PIX_FMT_MJPEG:
        break;

    default:
        break;
    }
    return 0;
}

int push_q(AITCAM_VIDENC_QUEUE *queue, MMP_UBYTE buffer)
{
    if (queue->size >= AITCAM_MAX_QUEUE_SIZE) {
        printc(0, "Queue overflow\r\n");
        return 0;
    }
    queue->buffers[(queue->head + queue->size++) % AITCAM_MAX_QUEUE_SIZE] = buffer;

    return 0;
}

MMP_UBYTE pop_q(AITCAM_VIDENC_QUEUE *queue, MMP_UBYTE offset)
{
    MMP_UBYTE buffer, target, source;

    if (queue->size == 0) {
        printc( "Queue underflow\n");
        return AITCAM_MAX_QUEUE_SIZE;
    }

    target = (queue->head + offset) % AITCAM_MAX_QUEUE_SIZE; //position for pop data
    buffer = queue->buffers[target];
    while (target != queue->head) { //shift elements before offset
        source = (target)? (target-1): (AITCAM_MAX_QUEUE_SIZE-1);
        queue->buffers[target] = queue->buffers[source];
        target = source;
    }
    queue->size--;
    queue->head = (queue->head + 1) % AITCAM_MAX_QUEUE_SIZE;

    return buffer;
}

void aitcam_ipc_push_vbq(AITCAM_VIDBUF_QUEUE *pVbq, MMP_UBYTE q_select,
                         MMP_UBYTE buf_id)
{
    if (buf_id >= pVbq->buf_num)
        return;

    if (q_select == VIDBUF_FREE_QUEUE) {
        push_q(&pVbq->free_vbq, buf_id);
    }
    else {
        push_q(&pVbq->ready_vbq, buf_id);
    }

    return;
}

MMP_UBYTE aitcam_ipc_pop_vbq(AITCAM_VIDBUF_QUEUE *pVbq, MMP_UBYTE q_select)
{
    if (q_select == VIDBUF_FREE_QUEUE) {
        return pop_q(&pVbq->free_vbq, 0);
    }
    else {
        return pop_q(&pVbq->ready_vbq, 0);
    }
}

void *aitcam_ipc_get_q(MMP_UBYTE streamid)
{
    AITCAM_VIDBUF_QUEUE *queue;

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return NULL;
    }
    queue = (AITCAM_VIDBUF_QUEUE *)v4l2_stream[streamid].info.vbq_phy_addr;

    return (void *)DRAM_NONCACHE_VA((MMP_ULONG)queue);
}

/*
push a video frame into ready queue
*/
void aitcam_ipc_push_frame(MMP_UBYTE streamid)
{
    AITCAM_VIDBUF_QUEUE *queue;
    CPU_LOCK_INIT();

    if (streamid >= AITCAM_NUM_CONTEXTS)
        return;

    queue = (AITCAM_VIDBUF_QUEUE *)aitcam_ipc_get_q(streamid);
    CPU_LOCK();
    aitcam_ipc_push_vbq(queue, VIDBUF_READY_QUEUE, aitcam_ipc_pop_vbq(queue, VIDBUF_FREE_QUEUE) );
    CPU_UNLOCK();

    return;
}

/*
get current free frame for write
*/
MMP_ULONG aitcam_ipc_cur_wr(MMP_UBYTE streamid)
{
    MMP_UBYTE head;
    MMP_ULONG cur_wr;
    AITCAM_VIDBUF_QUEUE *queue;
    CPU_LOCK_INIT();

    if (streamid >= AITCAM_NUM_CONTEXTS)
        return 0;

    queue = aitcam_ipc_get_q(streamid);

    CPU_LOCK();
    head = queue->free_vbq.buffers[queue->free_vbq.head];
    cur_wr = queue->buffers[head].buf_start;
    CPU_UNLOCK();

    //printc("cur head: %d, ptr:0x%X\r\n",head, cur_wr);
    return DRAM_NONCACHE_VA(cur_wr);
}

/*
check if free size is enough or not
*/
MMP_ULONG aitcam_ipc_free_size(MMP_UBYTE streamid)
{
    MMP_ULONG free_size;
    AITCAM_VIDBUF_QUEUE *queue;
    CPU_LOCK_INIT();

    if (streamid >= AITCAM_NUM_CONTEXTS)
        return 0;

    queue = aitcam_ipc_get_q(streamid);

    CPU_LOCK();
    if (queue->free_vbq.size) {
        free_size = queue->buf_size;
    }
    else {
        free_size = 0;
    }
    CPU_UNLOCK();
   
    return free_size;
}

/*
fill frame into into queue 
*/
void aitcam_ipc_fill_frame_info(MMP_UBYTE   streamid,
                                MMP_ULONG   framelength,
                                MMP_ULONG64 timestamp)
{
    MMP_UBYTE head;
    AITCAM_VIDBUF_QUEUE *queue;
    CPU_LOCK_INIT();

    if (streamid >= AITCAM_NUM_CONTEXTS) {
        return;
    }

    queue = aitcam_ipc_get_q(streamid);

    CPU_LOCK();
    head = queue->free_vbq.buffers[queue->free_vbq.head];
    queue->buffers[head].used_size = framelength;
    queue->buffers[head].timestamp = timestamp;
    CPU_UNLOCK();

    return;

}

MMP_ULONG aitcam_ipc_get_slot(MMP_ULONG streamid, MMP_ULONG *slot)
{
    MMP_ULONG free_size;
    AITCAM_VIDBUF_QUEUE *queue;

    queue = (AITCAM_VIDBUF_QUEUE *)aitcam_ipc_get_q(streamid);
    free_size = aitcam_ipc_free_size(streamid);

    if (free_size) {
        *slot = aitcam_ipc_cur_wr(streamid);
        return free_size;
    }
    else {
        *slot = 0;
        return free_size;
    }
}

MMP_BOOL aitcam_ipc_send_frame(MMP_ULONG streamid, MMP_ULONG size, MMP_ULONG ts)
{
static int _1st = 3 ;
    int channel = get_noack_channel(CPU_COMM_ID_V4L2B2A, 1);
    struct cpu_share_mem_slot *S_slot;

    aitcam_ipc_fill_frame_info(streamid, size, ts);
    aitcam_ipc_push_frame(streamid);
    
    while(1) {
		if (get_slot(channel, &S_slot) == 0) {
			S_slot->dev_id  = CPU_COMM_ID_V4L2B2A;
			S_slot->command = IPC_V4L2_ISR;
			S_slot->data_phy_addr = 0;
			S_slot->size = 0;
			S_slot->send_parm[0] = (unsigned int)streamid;
			S_slot->send_parm[1] = 0;
			S_slot->send_parm[2] = 0;
			S_slot->send_parm[3] = 0;
			S_slot->ack_func = 0;
			send_slot(channel, S_slot);

			break;
		}
		else { 
			printc("%s can't get slot\r\n", __func__);
		}
	}
    if((streamid==0) && (_1st > 0 )) {
      printc("[sz:%05d,ts:%08d]\r\n",size,ts );
      _1st-- ;
    }

    return MMP_TRUE;
}

int aitcam_ipc_set_iq( void *iq_ptr )
{
  ISP_CMD_STATUS isp_ret ;
  EZ_MODE_IQ_DATA_BUF *iq = (EZ_MODE_IQ_DATA_BUF *)iq_ptr;
  #if 0
  printc(" cmd_type     : %d\r\n",iq->cmd_type    );
	printc(" data_length  : %d\r\n",iq->data_length );
	//printc(" sysmode      : %d\r\n",iq->sysmode     );
	printc(" page         : %d\r\n",iq->page        );
	//printc(" type         : %d\r\n",iq->type        );
	//printc(" index        : %d\r\n",iq->index       );
	//printc(" buf_ptr_size : %d\r\n",iq->buf_ptr_size);
  #endif
  isp_ret = ISP_IF_APTUNING_Para(iq) ;
  if(isp_ret) {
    printc("set_iq ret:%d\r\n",isp_ret );
    return -1 ;
  }
  return 0;
}

#if 0
void _____FDFR_OSD_PARTS_____(){}
#endif

#if SUPPORT_OSD_FUNC==1
extern void         OSD_SetAttribute(MMPS_VSTREAM_PROPT *propt);
MMP_ULONG sync2linux = 0;
ait_fdframe_config *aitcam_ipc_mdosd_base(void)
{
static unsigned long mdosd_base=0 ;  
    MMP_SYS_MEM_MAP *mem_map ;
    mem_map = (MMP_SYS_MEM_MAP *)&AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG[48] ;
    if(! mdosd_base) {
      mdosd_base = DRAM_NONCACHE_VA( mem_map->ulOSDPhyAddr + MD_OSD_MEM_OFF );
    }
    return (ait_fdframe_config *)mdosd_base ;
}
ait_fdframe_config *aitcam_ipc_osd_base(void)
{
    static unsigned long ipc_osd_base, ipc_osd_base_cpua = 0 ;
    MMP_SYS_MEM_MAP *mem_map ;
    ait_fdframe_config *osd_base_fdfr ;
    ait_rtc_config *rtc_base;
    ait_osd_config *osd_base_rtc, *osd_base_str;
    AUTL_DATETIME *dt;
    AUTL_DATETIME m_RtcBaseTime = {
      1970,   ///< Year
      1,      ///< Month: 1 ~ 12
      1,      ///< Day of month: 1 ~ 28/29/30/31
      0,      ///< Sunday ~ Saturday
      0,      ///< 0 ~ 11 for 12-hour, 0 ~ 23 for 24-hour
      0,      ///< 0 ~ 59
      0,      ///< 0 ~ 59
      0,      ///< AM: 0; PM: 1 (for 12-hour only)
      0       ///< 24-hour: 0; 12-hour: 1	
    };
    dt = &m_RtcBaseTime ;
    mem_map = (MMP_SYS_MEM_MAP *)&AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG[48] ;
    //Get OSD addribute by UI setting & set in default
    if(!ipc_osd_base) {
        MMPS_VSTREAM_PROPT propt;
        VIDENC_CTL_SET  ctrl;   
        OSD_SetAttribute(&propt);
        ctrl = propt.ctl;
        osd_base_fdfr = (ait_fdframe_config *)DRAM_NONCACHE_VA( mem_map->ulOSDPhyAddr );
        ait_rtc_config *rtc_base;
        rtc_base = (ait_rtc_config *) (osd_base_fdfr + MAX_OSD_INDEX);

        ait_osd_config *osd_base_rtc, *osd_base_str;
        osd_base_rtc = (ait_osd_config *) (rtc_base + 1);
        osd_base_str = (ait_osd_config *) (osd_base_rtc + 1);

        ait_rtc_config *osdRTC2rtos ;
        ait_osd_config *osd2rtos_rtc, *osd2rtos_str ;

        osdRTC2rtos = rtc_base;
        //if(MMPF_RTC_GetTime(dt) == -1) {
        #if SUPPORT_RTC
        if( RTC_GetTime(dt) == -1 ) {
          osdRTC2rtos->usYear = ctrl.rtc_config.usYear;
          osdRTC2rtos->usMonth  = ctrl.rtc_config.usMonth;
          osdRTC2rtos->usDay= ctrl.rtc_config.usDay;
          osdRTC2rtos->usHour= ctrl.rtc_config.usHour;
          osdRTC2rtos->usMinute = ctrl.rtc_config.usMinute;
          osdRTC2rtos->usSecond = ctrl.rtc_config.usSecond;
        }
        else 
        #endif
        {
          osdRTC2rtos->usYear = dt->usYear;
          osdRTC2rtos->usMonth  = dt->usMonth;
          osdRTC2rtos->usDay= dt->usDay;
          osdRTC2rtos->usHour= dt->usHour;
          osdRTC2rtos->usMinute = dt->usMinute;
          osdRTC2rtos->usSecond = dt->usSecond;
        }

        osdRTC2rtos->bOn = ctrl.rtc_config.bOn;

        osd2rtos_rtc = osd_base_rtc;
        osd2rtos_rtc->index = ctrl.osd_config_rtc.index;
        osd2rtos_rtc->type  = ctrl.osd_config_rtc.type;
        osd2rtos_rtc->pos[0]= ctrl.osd_config_rtc.pos[0]; 
        osd2rtos_rtc->pos[1]= ctrl.osd_config_rtc.pos[1]; 
        strcpy(osd2rtos_rtc->str,ctrl.osd_config_rtc.str );
        osd2rtos_rtc->TextColorY = ctrl.osd_config_rtc.TextColorY  ;
        osd2rtos_rtc->TextColorU = ctrl.osd_config_rtc.TextColorU  ;
        osd2rtos_rtc->TextColorV = ctrl.osd_config_rtc.TextColorV  ;

        osd2rtos_str = osd_base_str;
        osd2rtos_str->index = ctrl.osd_config_str.index;
        osd2rtos_str->type  = ctrl.osd_config_str.type;
        osd2rtos_str->pos[0]= ctrl.osd_config_str.pos[0]; 
        osd2rtos_str->pos[1]= ctrl.osd_config_str.pos[1]; 
        strcpy(osd2rtos_str->str,ctrl.osd_config_str.str );
        osd2rtos_str->TextColorY = ctrl.osd_config_str.TextColorY  ;
        osd2rtos_str->TextColorU = ctrl.osd_config_str.TextColorU  ;
        osd2rtos_str->TextColorV = ctrl.osd_config_str.TextColorV  ;
        ipc_osd_base = (unsigned long)osd_base_fdfr;

    }

    return (ait_fdframe_config *)ipc_osd_base;
}


int aitcam_ipc_osd_register_drawer(int streamid , unsigned long fmt, ipc_osd_drawer *drawer,int osd_type)
{
    unsigned int argc = 0 ;
    
    MMP_IBC_PIPEID pid = 0 ;
    aitcam_img_info *img_info;
     
    int obj_id = aitcam_ipc_stream2obj(streamid , fmt );
    
    if(!drawer) {
        MMPS_VStream_RegisterDrawer( Video_Obj(obj_id) , 0, (void *)0) ;
        return 0 ;
    }
    
    if(obj_id!=V4L2_INVALID_OBJ_ID) {
        pid = Get_Obj_Pipe(obj_id,fmt) ;
        argc = (streamid << 16) | pid ;
        if(pid) {
            switch(osd_type) {
            case AIT_OSD_TYPE_FDFRAME:
            #if AIT_FDFRAME_EN
                img_info = &v4l2_stream[streamid].info.img_info ;
                ait_osd_Init( &h_osd[pid] );
                desc_osd[pid].ul2N = 4 ;
                desc_osd[pid].ulBufWidth  = img_info->P1.img_width ;
                desc_osd[pid].ulBufHeight = img_info->P2.img_height ;
                if(draw_fdfr_osd) {                
                    desc_osd[pid].ulRatioX = (desc_osd[pid].ulBufWidth << desc_osd[pid].ul2N) / desc_osd[pid].ulSrcWidth ;
                    desc_osd[pid].ulRatioY = (desc_osd[pid].ulBufHeight<< desc_osd[pid].ul2N) / desc_osd[pid].ulSrcHeight ;
                }
                else {
                    desc_osd[pid].ulRatioX = 0 ;
                    desc_osd[pid].ulRatioY = 0 ;
                }
                if(v4l2_debug_osd) {
                    printc("Draw Image (W,H) : (%d,%d) - %d,%d , ratio(%d,%d)\r\n", desc_osd[pid].ulSrcWidth , desc_osd[pid].ulSrcHeight , desc_osd[pid].ulBufWidth,desc_osd[pid].ulBufHeight ,  desc_osd[pid].ulRatioX, desc_osd[pid].ulRatioY);
                }
            #endif
            #if AIT_MDFRAME_EN
                img_info = &v4l2_stream[streamid].info.img_info ;
                ait_osd_Init( &h_osd[pid] );
                desc_md_osd.ul2N = 4 ;
                desc_md_osd.ulBufWidth  = img_info->P1.img_width ;
                desc_md_osd.ulBufHeight = img_info->P2.img_height ;
                if(draw_md_osd) {                
                    desc_md_osd.ulRatioX = (desc_md_osd.ulBufWidth << desc_md_osd.ul2N) / desc_md_osd.ulSrcWidth ;
                    desc_md_osd.ulRatioY = (desc_md_osd.ulBufHeight<< desc_md_osd.ul2N) / desc_md_osd.ulSrcHeight ;
                }
                else {
                    desc_md_osd.ulRatioX = 0 ;
                    desc_md_osd.ulRatioY = 0 ;
                }
                if(1/*v4l2_debug_osd*/) {
                    printc("Draw Image (W,H) : (%d,%d) - %d,%d , ratio(%d,%d)\r\n", 
                          desc_md_osd.ulSrcWidth , 
                          desc_md_osd.ulSrcHeight, 
                          desc_md_osd.ulBufWidth,
                          desc_md_osd.ulBufHeight,
                          desc_md_osd.ulRatioX,
                          desc_md_osd.ulRatioY);
                }
            
            #endif
                break;
            }
            MMPS_VStream_RegisterDrawer( Video_Obj(obj_id) , drawer, (void *)argc) ;
        }
    }
    return 0;
}
#endif

//------------------------------------------------------------------------------
//  Function    : CpuB_V4L2Init
//  Description : CPU_B V4L2 module initialization routine
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuB_V4L2Init(void)
{
    //RTNA_DBG_Str(0, "[V4L2]\r\n");

    s_ulV4L2RcvSemId = MMPF_OS_CreateSem(1);
    EstablishV4L2();

    return CPU_COMM_ERR_NONE;
}
CPUCOMM_MODULE_INIT(CpuB_V4L2Init)

#endif //#if (SUPPORT_V4L2)
