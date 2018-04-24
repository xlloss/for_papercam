//==============================================================================
//
//  File        : JobDispatch.c
//  Description : ce job dispatch function
//  Author      : Robin Feng
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
#include "ait_config.h"
#include "version.h"

#include "ex_vidrec.h"
#include "dualcpu_v4l2.h"
#include "cfgparser.h"

#include "mmpf_system.h"

#include "ipc_cfg.h"
#if (SUPPORT_MDTC)
#include <stdio.h>
#include "mmps_mdtc.h" 
#endif
#include "ait_osd.h"
#include "ex_misc.h"


#if (USER_STRING)
extern void small_sprintf(char* s,char *fmt, ...);
#define sprintf small_sprintf
#endif
//==============================================================================
//
//                              OPTIONS
//
//==============================================================================

#define BOOT_MENU_DBG       (0)

//==============================================================================
//
//                              ENUMERATIONS
//
//==============================================================================

typedef enum _MENU_BOOT_MODE {
    BOOT_MODE_VIDEO = 0,
    BOOT_MODE_CAMERA,
    BOOT_MODE_NUM
} MENU_BOOT_MODE;

typedef enum _MENU_BOOT_EVENT {
    PWR_KEY      = (1 << 3), // normal power on, no action
    PIR_KEY      = (1 << 0), // PIR trigger
    DOORBELL_KEY = (1 << 2), // Doorbell trigger
    WIFI_KEY     = (1 << 4), // wifi wakeup,old
    WIFI_KEY_NEW = (0 << 0), // new wifi key
    DISABLE_KEY  = (1 << 1)  // ignore hw key even, use actioncam.cfg default boot action
} MENU_BOOT_EVENT;

typedef enum _MENU_MD_MODE {
  MD_MODE_DISABLE,
  MD_MODE_INTERNAL,
  MD_MODE_EXTERNAL  
} MENU_MD_MODE;
//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef struct _MENU_SETTING {
// section [boot]
    MMP_BOOL    bAutoShot;
    MMP_UBYTE   ubBootMode;
 // section [camera]
    MMP_UBYTE   ubPowerLine ; // camera power line
    MMP_UBYTE   ubOrientation; // sensor orientation    
    MMP_UBYTE   ubNightVision; // 0 : off , 1 : on , 2 : auto
    MMP_UBYTE   ubLightSensorH,ubLightSensorL;
    MMP_UBYTE   ubTempSensorH,ubTempSensorL ;
    MMP_UBYTE   ubIRLedLevel ;  // 0~255 // value meaning is depend on custmer
#if MIPI_TEST_PARAM_EN
    MMP_UBYTE   ubSOT  ;
    MMP_UBYTE   ubDelay;
    MMP_UBYTE   ubLatchEdge;
    MMP_UBYTE   ubPFrameTH ;
#endif    
// section [capture]    
    MMP_USHORT  usBootShots;
    MMP_USHORT  usCaptureWidth ;
    MMP_USHORT  usCaptureHeight;
    //HW RTC setting
    MMP_USHORT  usRTCAlarmOn;
    MMP_USHORT  usRTCAlarmPeriod;
    MMP_USHORT  usFps;
    MMP_ULONG   usJpeg2VideoShots;

// section [stream]    
    /*demo for boot setting*/
    MMP_ULONG   ulBootFormat; // V4L2 four-cc
    MMP_USHORT  usBootWidth;
    MMP_USHORT  usBootHeight;
    MMP_USHORT  usBootFps;
    MMP_USHORT  usBootGop;
    MMP_USHORT  usBootProfile; // V4L2 : enum v4l2_mpeg_video_h264_profile
    MMP_USHORT  usH264InitICnt ; // only for real-time stream mode
    MMP_UBYTE   ubInitQP,ubMinQP,ubMaxQP ;
    
    MMP_ULONG   ulBootDuration;
    MMP_ULONG   ulBootBitrate;
    MMP_ULONG   ulBootBitrateAAC;
    MMP_ULONG   ulBootBitrateStart ;
    MMP_BOOL    bBootSeamless;
    MMP_BOOL    bOffAfterCap;
    MMP_BOOL    bRcLBR;
    MMP_BOOL    bRcSkip;
    MMP_BOOL    bSnapShot ; // capture jpeg or not in video mode
    
    /*audio sample rate*/
    MMP_ULONG   ulBootSampleRate;
    /*MD property*/
    MENU_MD_MODE    bMDEnable ;
    MMP_USHORT  usMDWidth ;
    MMP_USHORT  usMDHeight;
    MMP_UBYTE  ubMDDivX;
    MMP_UBYTE  ubMDDivY;
    MMP_UBYTE  ubPerctThdMin;
    MMP_UBYTE  ubPerctThdMax;
    MMP_UBYTE  ubSensitivity;
    MMP_USHORT  usLearnRate;
    #if MD_USE_ROI_TYPE==1
    MMP_UBYTE   ubMDRoiNum;
    MD_params_in_t  roi_p[MDTC_MAX_ROI]  ;
    MD_block_info_t roi[MDTC_MAX_ROI] ;
    #endif
    #if SUPPORT_OSD_FUNC
    MMP_BOOL OsdStrEnable[MAX_H264_STREAM_NUM]; // for 3 H264
    MMP_BOOL OsdRtcEnable[MAX_H264_STREAM_NUM];
    MMP_BOOL OsdBootStream;
    MMP_ULONG str_pos_startX;
    MMP_ULONG str_pos_startY;
    MMP_ULONG rtc_pos_startX;
    MMP_ULONG rtc_pos_startY;
    MMP_ULONG str_pos_x;
    MMP_ULONG str_pos_y;
    MMP_ULONG rtc_pos_x;
    MMP_ULONG rtc_pos_y;
    char OsdStr[MAX_OSD_STR_LEN];
    char RtcStr[2];
    #endif

} MENU_SETTING;

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static MENU_SETTING     menu;

//==============================================================================
//
//                              FUNCTION PROTOTYPE
//
//==============================================================================

extern void aitcam_ipc_debug(int dbg_on);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
static void UI_Init_MenuSetting(MENU_SETTING *menu)
{
    /* section [boot] */
    menu->ubBootMode        = BOOT_MODE_VIDEO;
    menu->bAutoShot         = MMP_TRUE;
    menu->ulBootSampleRate  = 16000;
    
    /* section [camera] */
    menu->ubPowerLine       = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
    menu->ubOrientation     = V4L2_CAMERA_AIT_ORTN_NORMAL ;
    menu->ubNightVision     = 2 ; // 0 : off , 1 : on , 2 : auto
    menu->ubLightSensorH    = 14 ;
    menu->ubLightSensorL    = 10 ;
    menu->ubTempSensorH     = 50 ;
    menu->ubTempSensorL     = 45 ;
    menu->ubIRLedLevel      = 0  ; // 0~255 , 0 is led off
    

    /* Boot as camera mode */
    menu->usBootShots       = 1;
    menu->usCaptureWidth    = 1920 ;
    menu->usCaptureHeight   = 1080 ;
    /* HW RTC Alarm mode */
    menu->usRTCAlarmOn      = 0;
    menu->usRTCAlarmPeriod  = 1;
    menu->usFps = 30;
    menu->usJpeg2VideoShots = 5;

    /* Boot as video mode */
    menu->usBootGop         = 30;
    menu->ulBootDuration    = (MMP_ULONG)-1;
    menu->bBootSeamless     = 0;
    menu->usBootFps         = 30;
    menu->ulBootBitrate     = 800 * 1000;
    menu->ulBootBitrateAAC  = 16 * 1000;
    menu->ulBootBitrateStart= 0 ;
    menu->usBootProfile     = V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE;
    menu->bRcLBR            = TRUE;
    menu->bRcSkip           = FALSE;
    menu->bSnapShot         = FALSE;
    menu->ulBootFormat      = V4L2_PIX_FMT_H264;
    menu->usH264InitICnt    = 1;
    menu->ubInitQP          = 36 ;
    menu->ubMinQP           = 10 ;
    menu->ubMaxQP           = 46 ;
    
    /* MD init */
    menu->bMDEnable         = MD_MODE_DISABLE;
    menu->usMDWidth         = 160 ;
    menu->usMDHeight        =  90 ;
    menu->ubMDDivX          = 1 ;
    menu->ubMDDivY          = 1 ;
    menu->ubPerctThdMin     = 4 ;
    menu->ubPerctThdMax     = 100;
    menu->ubSensitivity     = 57 ;
    menu->usLearnRate       = 500 ;
    #if MIPI_TEST_PARAM_EN
    menu->ubSOT             = 31 ;
    menu->ubDelay           = 8 ;
    menu->ubLatchEdge       = 1 ;
    menu->ubPFrameTH        = 10;
    #endif
}

static MMP_UBYTE UI_Get_BootEvent(void)
{
    volatile MMP_UBYTE *ptr;

    ptr = (volatile MMP_UBYTE *)(&AITC_BASE_CPU_IPC->CPU_IPC[4]);
    return ptr[0];
}

static void UI_Get_BootCapture(MENU_SETTING *menu)
{
extern void MMPF_ADC_SetInitCh(MMP_AUD_LINEIN_CH ch);
#if SUPPORT_AEC
extern void MMPF_AEC_SetProcCh(MMP_SHORT ch) ;
#endif

    static int  _init_user_cfg = 0;
    char        str_val[32];
    int         int_val;
    MMP_UBYTE   boot_event;

    if (!_init_user_cfg) {
        _init_user_cfg = 1;
        if (init_cfg_parser(0, 0) < 0) {
            RTNA_DBG_Str0("Unable to load menu setting\r\n");
        }
    }

    boot_event = UI_Get_BootEvent();
    // no action when pwr key pressed
    if (boot_event == PWR_KEY)
        menu->bAutoShot = MMP_FALSE;
    else
        menu->bAutoShot = MMP_TRUE;

    if (!switch_cfg_section("[boot]")) {
        
        if (!get_cfg_data(CFG_INTEGER, "samplerate", &int_val)) {
            menu->ulBootSampleRate = int_val;
        }
        /*set ADC input channel */
        if (!get_cfg_data(CFG_INTEGER, "audio_in_ch", &int_val)) {
            MMPF_ADC_SetInitCh(int_val) ;
        }
        if (!get_cfg_data(CFG_INTEGER, "aec_proc_ch", &int_val)) {
          #if SUPPORT_AEC
            MMPF_AEC_SetProcCh(int_val );
          #endif  
        }
        
        if (!get_cfg_data(CFG_INTEGER, "debug", &int_val)) {
            aitcam_ipc_debug(int_val);
        }
        if (!get_cfg_data(CFG_INTEGER, "debug_ts", &int_val)) {
            aitcam_ipc_debug_ts(int_val);
        }
        if (!get_cfg_data(CFG_INTEGER, "debug_osd", &int_val)) {
            aitcam_ipc_debug_osd(int_val);
        }

        if (!get_cfg_data(CFG_STRING, "keymap", str_val)) {
            if (!strcmp(str_val, "disable")) {
                boot_event = DISABLE_KEY; 
            }    
        }    

        if (!get_cfg_data(CFG_STRING, "stream", str_val)) {

            // boot mode set to stream0, video record
            if (!strcmp(str_val, "[stream0]")) {
                // video record is VBR
                menu->bRcLBR = MMP_FALSE;
                menu->bRcSkip = MMP_FALSE;
                #if SUPPORT_OSD_FUNC
                menu->OsdBootStream = STREAM_SELECT_0;
                #endif
                // but key event is wifi mode
                if ((boot_event == DOORBELL_KEY) ||
                    (boot_event == WIFI_KEY) ||
                    (boot_event == WIFI_KEY_NEW))
                {
                    strcpy(str_val, "[wifi]");
                }
            }
            if (!strcmp(str_val, "[wifi]")) {
            // streaming is LBR
                strcpy(str_val, "[stream1]");
                menu->bRcLBR = MMP_TRUE;
                menu->bRcSkip = MMP_FALSE;
                #if SUPPORT_OSD_FUNC
                menu->OsdBootStream = STREAM_SELECT_1;
                #endif
            }

            #if (BOOT_MENU_DBG)
            printc("Boot: %s, event: 0x%02x\r\n", str_val, boot_event);
            #endif

            if (!switch_cfg_section(str_val)) {
                if (!strcmp(str_val, "[capture]")) {
                    #if (BOOT_MENU_DBG)
                    printc("->Capture\r\n");
                    #endif

                    menu->ubBootMode = BOOT_MODE_CAMERA;
                    if (!get_cfg_data(CFG_INTEGER, "shots", &int_val)) {
                        menu->usBootShots = int_val;
                    }
                    if (!get_cfg_data(CFG_INTEGER, "width", &int_val)) {
                        menu->usCaptureWidth = int_val;
                    }
                    if (!get_cfg_data(CFG_INTEGER, "height", &int_val)) {
                        menu->usCaptureHeight = int_val;
                    }

                    if (!get_cfg_data(CFG_INTEGER, "alarm", &int_val)) {
                        menu->usRTCAlarmOn = int_val;
                    }

                    if (!get_cfg_data(CFG_INTEGER, "alarmperiod", &int_val)) {
                        menu->usRTCAlarmPeriod = int_val;
                    }

                    if (!get_cfg_data(CFG_INTEGER, "fps", &int_val)) {
                        menu->usFps = int_val;
                    }

                    if (!get_cfg_data(CFG_INTEGER, "jpeg2videoshots", &int_val)) {
                        menu->usJpeg2VideoShots = int_val;
                    }
                    
                    
                }
                else {
                    #if (BOOT_MENU_DBG)
                    printc("->Record\r\n");
                    #endif

                    if (!get_cfg_data(CFG_INTEGER, "gopsize", &int_val)) {
                        menu->usBootGop = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("gopsize: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "duration", &int_val)) {
                        menu->ulBootDuration = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("duration: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "seamless", &int_val)) {
                        menu->bBootSeamless = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("seamless: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "fps", &int_val)) {
                        menu->usBootFps = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("fps: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "bitrate", &int_val)) {
                        menu->ulBootBitrate = int_val * 1000;
                        #if (BOOT_MENU_DBG)
                        printc("bitrate: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "bitrate_start", &int_val)) {
                        menu->ulBootBitrateStart = int_val * 1000;
                        #if (BOOT_MENU_DBG)
                        printc("bitrate_start: %d\r\n", int_val);
                        #endif
                    }

                    if (!get_cfg_data(CFG_INTEGER, "bitrate_aac", &int_val)) {
                        menu->ulBootBitrateAAC = int_val * 1000;
                        #if (BOOT_MENU_DBG)
                        printc("bitrateAAC: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "profile", &int_val)) {
                        menu->usBootProfile = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("profile: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "snapshot", &int_val)) {
                        menu->bSnapShot = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("snapshot: %d\r\n", int_val);
                        #endif
                    }
                    
                    if (!get_cfg_data(CFG_INTEGER, "start_i_frames", &int_val)) {
                        menu->usH264InitICnt = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("start_i_frames: %d\r\n", int_val);
                        #endif
                    }
                    
  
                     if (!get_cfg_data(CFG_INTEGER, "initqp", &int_val)) {
                        menu->ubInitQP = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("initqp: %d\r\n", int_val);
                        #endif
                    }
                  
                     if (!get_cfg_data(CFG_INTEGER, "minqp", &int_val)) {
                        menu->ubMinQP = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("minqp: %d\r\n", int_val);
                        #endif
                    }

                     if (!get_cfg_data(CFG_INTEGER, "maxqp", &int_val)) {
                        menu->ubMaxQP = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("maxqp: %d\r\n", int_val);
                        #endif
                    }
                    if (!get_cfg_data(CFG_INTEGER, "rcskip", &int_val)) {
                        menu->bRcSkip = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("rcskip: %d\r\n", int_val);
                        #endif
                    }
                    
                    if (!get_cfg_data(CFG_INTEGER, "lbr", &int_val)) {
                        menu->bRcLBR = int_val;
                        #if (BOOT_MENU_DBG)
                        printc("rcLBR: %d\r\n", int_val);
                        #endif
                    }
                    
                                            
                }
                if (!get_cfg_data(CFG_STRING, "format", str_val)) {
                    #if (BOOT_MENU_DBG)
                    printc("format: %s\r\n", str_val);
                    #endif

                    if (!strcmp(str_val, "jpeg"))
                        menu->ulBootFormat = V4L2_PIX_FMT_MJPEG;
                    else if (!strcmp(str_val, "h264"))
                        menu->ulBootFormat = V4L2_PIX_FMT_H264;
                }
                if (!get_cfg_data(CFG_INTEGER, "width", &int_val)) {
                    #if (BOOT_MENU_DBG)
                    printc("width: %d\r\n", int_val);
                    #endif
                    menu->usBootWidth = int_val;
                }
                if (!get_cfg_data(CFG_INTEGER, "height", &int_val)) {
                    #if (BOOT_MENU_DBG)
                    printc("height: %d\r\n", int_val);
                    #endif
                    menu->usBootHeight = int_val;
                }
                if (!get_cfg_data(CFG_INTEGER, "poweroff", &int_val)) {
                    #if (BOOT_MENU_DBG)
                    printc("poweroff: %d\r\n", int_val);
                    #endif
                    menu->bOffAfterCap = int_val;
                }
                if (!get_cfg_data(CFG_INTEGER, "start", &int_val)) {
                    menu->bAutoShot = int_val;
                }
            }
            else {
                menu->bAutoShot = MMP_FALSE;
            }
        }
        else {
            menu->bAutoShot = MMP_FALSE;
        }
    }
    else {
        menu->bAutoShot = MMP_FALSE;
    }

    // parsing [camera] section
    if (!switch_cfg_section("[camera]")) {
        if (!get_cfg_data(CFG_INTEGER, "powerline", &int_val)) {
            menu->ubPowerLine = int_val ;
        }    
        if (!get_cfg_data(CFG_INTEGER, "orientation", &int_val)) {
            menu->ubOrientation = int_val ;
        }    

        if (!get_cfg_data(CFG_INTEGER, "nightvision", &int_val)) {
            menu->ubNightVision = int_val ;
        }
            
        if (!get_cfg_data(CFG_INTEGER, "lightsensor_h", &int_val)) {
            menu->ubLightSensorH = int_val ;
        }    

        if (!get_cfg_data(CFG_INTEGER, "lightsensor_l", &int_val)) {
            menu->ubLightSensorL = int_val ;
        }    
        
        if (!get_cfg_data(CFG_INTEGER, "tempsensor_h", &int_val)) {
            menu->ubTempSensorH = int_val ;
        }    

        if (!get_cfg_data(CFG_INTEGER, "tempsensor_l", &int_val)) {
            menu->ubTempSensorL = int_val ;
        }    
        
        if (!get_cfg_data(CFG_INTEGER, "irled_level", &int_val)) {
            menu->ubIRLedLevel = int_val ;
        }    

        if (!get_cfg_data(CFG_INTEGER, "orientation", &int_val)) {
            menu->ubOrientation = int_val ;
        }    
        #if MIPI_TEST_PARAM_EN
        if(!menu->bAutoShot) {
          if (!get_cfg_data(CFG_INTEGER, "mipi_sot", &int_val)) {
              menu->ubSOT = int_val ;
          }    
          
          if (!get_cfg_data(CFG_INTEGER, "mipi_delay", &int_val)) {
              menu->ubDelay = int_val ;
          }    
  
          if (!get_cfg_data(CFG_INTEGER, "mipi_edge", &int_val)) {
              menu->ubLatchEdge = int_val ;
          }    

          if (!get_cfg_data(CFG_INTEGER, "mipi_pframe_th", &int_val)) {
              menu->ubPFrameTH = int_val ;
          }    
        
        }
        #endif
        
    }
    // if need snapshot in any of stream action, get parameter from [capture]
    if( menu->bSnapShot ) {
      if (!switch_cfg_section("[capture]")) {
          if (!get_cfg_data(CFG_INTEGER, "shots", &int_val)) {
              menu->usBootShots = int_val;
          }
          if (!get_cfg_data(CFG_INTEGER, "width", &int_val)) {
              menu->usCaptureWidth = int_val;
          }
          if (!get_cfg_data(CFG_INTEGER, "height", &int_val)) {
              menu->usCaptureHeight = int_val;
          }
       }           
    }
    
    // add MD configuration setting
    if (!switch_cfg_section("[md]")) {
            if (!get_cfg_data(CFG_INTEGER, "enable", &int_val)) {
                menu->bMDEnable = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "width", &int_val)) {
                menu->usMDWidth = int_val ;
                if(menu->usMDWidth < 160) menu->usMDWidth = 160 ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "height", &int_val)) {
                menu->usMDHeight = int_val ;
                if(menu->usMDHeight < 90) menu->usMDHeight = 90 ;
            }    
        #if MD_USE_ROI_TYPE==0
          if( menu->bMDEnable & MD_MODE_INTERNAL ){
            if (!get_cfg_data(CFG_INTEGER, "div_x", &int_val)) {
                menu->ubMDDivX = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "div_y", &int_val)) {
                menu->ubMDDivY = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "perct_thd_min", &int_val)) {
                menu->ubPerctThdMin = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "perct_thd_max", &int_val)) {
                menu->ubPerctThdMax = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "sensitivity", &int_val)) {
                menu->ubSensitivity = int_val ;
            }    
            if (!get_cfg_data(CFG_INTEGER, "learn_rate", &int_val)) {
                menu->usLearnRate = int_val ;
            }
          }
        #else
        if( menu->bMDEnable & MD_MODE_INTERNAL ){
            char roi[16],roi_p[16] ;
            char roi_val[32],roi_p_val[32] ;
            int i, md_osd_en=0;
            menu->ubMDDivX = 1 ;
            menu->ubMDDivY = 1 ;
            menu->ubMDRoiNum = 0 ;
            
#if SUPPORT_OSD_FUNC && AIT_MDFRAME_EN

            if (!get_cfg_data(CFG_INTEGER, "osd", &md_osd_en)) {
                aitcam_enable_md_osd(md_osd_en,menu->usMDWidth,menu->usMDHeight) ;
            }    
#endif
            
            for(i=0;i<MDTC_MAX_ROI;i++) {
                MD_block_info_t *roi_ptr  = (MD_block_info_t *)&menu->roi[i] ;
                MD_params_in_t  *roi_p_tr = (MD_params_in_t  *)&menu->roi_p[i] ;
                #if (USER_STRING)
                int p[4]={0,0,0,0};
                sprintf(roi  ,"roi_%d" ,i );
                sprintf(roi_p,"roi_p%d",i );
                if (!get_cfg_data(CFG_STRING, roi , roi_val)) {
                    char *end = &roi_val[0];
                    int i=0;
                    while(*end) 
                    {
                        p[i] = strtol(end, &end, 10);
                        while(*end == ',')
                        {
                            end++;
                            i++;
                        }
                    }
                    roi_ptr->st_x = p[0] ;
                    roi_ptr->st_y = p[1] ;
                    roi_ptr->end_x = p[2] ;
                    roi_ptr->end_y = p[3] ;
                    menu->ubMDRoiNum++;
                }
                #else
                int p0,p1,p2,p3 ; 
                sprintf(roi  ,"roi_%d" ,i );
                sprintf(roi_p,"roi_p%d",i );
                if (!get_cfg_data(CFG_STRING, roi , roi_val)) {
                    sscanf(roi_val,"%d,%d,%d,%d", &p0,&p1,&p2,&p3 );
                    roi_ptr->st_x = p0 ;
                    roi_ptr->st_y = p1 ;
                    roi_ptr->end_x = p2 ;
                    roi_ptr->end_y = p3 ;
                    menu->ubMDRoiNum++;
                }
                #endif
                else {
                    roi_ptr->st_x  = 0 ;
                    roi_ptr->st_y  = 0 ;
                    roi_ptr->end_x = 0 ;
                    roi_ptr->end_y = 0 ;
                }
                if (!get_cfg_data(CFG_STRING, roi_p , roi_p_val)) {
                #if (USER_STRING)
                    char *end = &roi_p_val[0];
                    int i=0;
                    while(*end) 
                    {
                        p[i] = strtol(end, &end, 10);
                        while(*end == ',')
                        {
                            i++;
                            end++;
                        }
                    }
                    roi_p_tr->size_perct_thd_min = p[0] ;
                    roi_p_tr->size_perct_thd_max = p[1] ;
                    roi_p_tr->sensitivity = p[2] ;
                    roi_p_tr->learn_rate  = p[3] ;
                    roi_p_tr->enable  = 1;
                #else
                    sscanf(roi_p_val,"%d,%d,%d,%d",     &p0,&p1,&p2,&p3);
                    roi_p_tr->size_perct_thd_min = p0 ;
                    roi_p_tr->size_perct_thd_max = p1 ;
                    roi_p_tr->sensitivity = p2 ;
                    roi_p_tr->learn_rate  = p3 ;
                    roi_p_tr->enable  = 1;
                #endif
                }
                else {
                    roi_p_tr->enable  = 0 ;
                }
            }
            //in case no ROIs found at config setting
            // set defaut 5x5 ROIs
            if( !menu->ubMDRoiNum ) {
                int div_x = 5;
                int div_y = 5;
                int w = menu->usMDWidth  / div_x ;
                int h = menu->usMDHeight / div_y ;
                
                menu->ubMDRoiNum = div_x * div_y ;
                
                for(i=0;i<menu->ubMDRoiNum;i++) {
                    MD_block_info_t *roi_ptr  = (MD_block_info_t *)&menu->roi[i] ;
                    MD_params_in_t  *roi_p_tr = (MD_params_in_t  *)&menu->roi_p[i] ;
                    roi_p_tr->size_perct_thd_min = 5 ;
                    roi_p_tr->size_perct_thd_max = 100 ;
                    roi_p_tr->sensitivity = 70 ;
                    roi_p_tr->learn_rate  = 2000 ;
                    roi_p_tr->enable  = 1;
                    
                    roi_ptr->st_x  = 0 + (i%div_x) * ( w );	
                    roi_ptr->end_x = roi_ptr->st_x + w - 1 ;				
                    roi_ptr->st_y  = 0 + (i/div_y) * (h  );			
                    roi_ptr->end_y = roi_ptr->st_y + (h-1 );
                }
            }
            
#if SUPPORT_OSD_FUNC && AIT_MDFRAME_EN
            
            if(md_osd_en) {
              MD_block_info_t *roi_ptr ;
              for(i=0;i<menu->ubMDRoiNum;i++) {
                roi_ptr = (MD_block_info_t *)&menu->roi[i] ;
                aitcam_set_md_osd(i,menu->ubMDRoiNum, roi_ptr);
              }
            }
#endif                    
            
        }
        #endif
    }
    #if SUPPORT_OSD_FUNC
    if (!switch_cfg_section("[osd]")) {
                    // parsing osd section
                    if (!get_cfg_data(CFG_STRING, "osd_str_text", str_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream text: %s\r\n", str_val);
                        #endif
                        if ( strlen(str_val) ) {
                            strcpy(menu->OsdStr,str_val);
                        }

                    }
                    if (!get_cfg_data(CFG_INTEGER, "osd_str_stream0", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 0 osd str enable: %d\r\n", int_val);
                        #endif
                        if (int_val == 1 ) {
                            menu->OsdStrEnable[0] = TRUE;
                        }
                        else {
                            menu->OsdStrEnable[0] = FALSE;
                        }

                    }
                    if (!get_cfg_data(CFG_INTEGER, "osd_rtc_stream0", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 0 osd rtc enable: %d\r\n", int_val);
                        #endif
                        if(int_val == 1)
                        menu->OsdRtcEnable[0] = TRUE;
                        menu->RtcStr[0] = 1;
                        menu->RtcStr[1] = 1;
                    }

                    if (!get_cfg_data(CFG_INTEGER, "osd_str_stream1", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 1 osd str enable: %d\r\n", int_val);
                        #endif
                        if (int_val == 1) {
                            menu->OsdStrEnable[1] = TRUE;
                        }
                        else {
                            menu->OsdStrEnable[1] = FALSE;
                        }

                    }

                    if (!get_cfg_data(CFG_INTEGER, "osd_rtc_stream1", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 1 osd rtc enable: %d\r\n", int_val);
                        #endif
                        if(int_val == 1)
                        menu->OsdRtcEnable[1] = TRUE;
                        menu->RtcStr[0] = 1;
                        menu->RtcStr[1] = 1;
                    }
                    #if CUSTOMER_H264_STREAMS > 2                    
                    if (!get_cfg_data(CFG_INTEGER, "osd_str_stream2", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 2 osd str enable: %d\r\n", int_val);
                        #endif
                        if (int_val == 1) {
                            menu->OsdStrEnable[2] = TRUE;
                        }
                        else {
                            menu->OsdStrEnable[2] = FALSE;
                        }

                    }

                    if (!get_cfg_data(CFG_INTEGER, "osd_rtc_stream2", &int_val)) {
                        #if (BOOT_MENU_DBG)
                        printc("stream 2 osd rtc enable: %d\r\n", int_val);
                        #endif
                        if(int_val == 1)
                        menu->OsdRtcEnable[2] = TRUE;
                        menu->RtcStr[0] = 1;
                        menu->RtcStr[1] = 1;
                    }
                    
                    #endif
                      if (!get_cfg_data(CFG_INTEGER, "rtc_pos_startX", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("rtc_pos_startX: %d\r\n", int_val);
                          #endif
                          menu->rtc_pos_startX = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "rtc_pos_startY", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("rtc_pos_startY: %d\r\n", int_val);
                          #endif
                          menu->rtc_pos_startY = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "rtc_pos_x", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("rtc_pos_x: %d\r\n", int_val);
                          #endif
                          menu->rtc_pos_x = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "rtc_pos_y", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("rtc_pos_y: %d\r\n", int_val);
                          #endif
                          menu->rtc_pos_y = int_val;
                      }

                      if (!get_cfg_data(CFG_INTEGER, "str_pos_startX", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("str_pos_startX: %d\r\n", int_val);
                          #endif
                          menu->str_pos_startX = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "str_pos_startY", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("str_pos_startY: %d\r\n", int_val);
                          #endif
                          menu->str_pos_startY = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "str_pos_x", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("str_pos_x: %d\r\n", int_val);
                          #endif
                          menu->str_pos_x = int_val;
                      }
  
                      if (!get_cfg_data(CFG_INTEGER, "str_pos_y", &int_val)) {
                          #if (BOOT_MENU_DBG)
                          printc("str_pos_y: %d\r\n", int_val);
                          #endif
                          menu->str_pos_y = int_val;
                      }
                }
#endif
    

    if (!menu->bAutoShot) 
        printc("No Action\r\n");
}


void UI_Get_CaptureRes(MMP_USHORT *w, MMP_USHORT *h)
{
    *w = menu.usCaptureWidth;
    *h = menu.usCaptureHeight;
    if (*w == 0)
        *w = 1920;
    if (*h == 0)
        *h = 1080;
}

void UI_Get_AlarmAttr(MMP_USHORT *alarmOn, MMP_USHORT *alarmperiod)
{
    *alarmOn = menu.usRTCAlarmOn;
    *alarmperiod = menu.usRTCAlarmPeriod;
}



MMP_ULONG UI_Get_VideoTargetBitrate(void)
{
    return menu.ulBootBitrate ;    
}

MMP_ULONG UI_Get_Bitrate(MMP_BOOL video)
{
    if (video) {
        printc("(H264.BR):%d\r\n", menu.ulBootBitrate);
        if (!menu.ulBootBitrate) {
            menu.ulBootBitrate = 4 * 1000 * 1000;
        }

        // FIXME
        if (menu.ulBootBitrate > (4 * 1000 * 1000))
            menu.ulBootBitrate = 4 * 1000 * 1000;

        if( menu.ulBootBitrateStart) {
            return menu.ulBootBitrateStart ;
        }
        return menu.ulBootBitrate;
    }
    else {
        printc("(AAC.BR):%d\r\n", menu.ulBootBitrateAAC);
        if (!menu.ulBootBitrateAAC) {
            menu.ulBootBitrateAAC = 16000;
        }
        return menu.ulBootBitrateAAC;
    }
}

MMP_ULONG UI_Get_Resolution(MMP_USHORT *w, MMP_USHORT *h)
{
    *w = menu.usBootWidth;
    *h = menu.usBootHeight;
    if (*w == 0)
        *w = 1920;
    if (*h == 0)
        *h = 1080;

    return 0;
}

MMP_ULONG UI_Get_GopSize(void)
{
    if (menu.usBootGop == 0) {
        menu.usBootGop = 30;
    }
    return menu.usBootGop;
}

MMP_ULONG UI_Get_SampleRate(void)
{
    if (menu.ulBootSampleRate == 0) {
        menu.ulBootSampleRate = 16000;
    }
    return menu.ulBootSampleRate;    
}

MMP_BOOL UI_Get_Seamless(void)
{
    return menu.bBootSeamless;
}

VIDENC_RC_MODE UI_Get_RcMode(void)
{
    if (menu.bRcLBR)
        return VIDENC_RC_MODE_LOWBR;

    return VIDENC_RC_MODE_VBR;
}

MMP_BOOL UI_Get_RcSkip(void)
{
    return menu.bRcSkip;
}

MMP_USHORT UI_Get_Profile(void)
{
    switch( menu.usBootProfile ) {
    case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
        return H264ENC_MAIN_PROFILE ;
    case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
        return H264ENC_HIGH_PROFILE ;    
    }
    return H264ENC_BASELINE_PROFILE ;
}

MMP_USHORT UI_Get_StartIFrames(void)
{
    return menu.usH264InitICnt ;
}


MMP_UBYTE UI_Get_InitQP(void)
{
    return menu.ubInitQP ;
}

MMP_UBYTE UI_Get_MinQP(void)
{
    return menu.ubMinQP ;
}

MMP_UBYTE UI_Get_MaxQP(void)
{
    return menu.ubMaxQP ;
}


MMP_UBYTE UI_Get_MdEnable(void)
{
    return menu.bMDEnable;
}

MMP_USHORT UI_Get_Fps(void)
{
  return menu.usBootFps ;
}

#if (SUPPORT_MDTC)

void UI_Set_MdProperties(MMPS_MDTC_PROPT *propt,MMP_BOOL en)
{
    MMP_BOOL roi_from_menu = MMP_FALSE ;
    MMP_ULONG x/*,y*/;
    if(!propt->luma_w) {
        propt->luma_w = menu.usMDWidth ;
    }
    
    if(!propt->luma_h) {
        propt->luma_h = menu.usMDHeight;
    }
    
    if( propt->luma_w > gstMdtcCap.luma_w ) {
        propt->luma_w = gstMdtcCap.luma_w ;    
    }

    if( propt->luma_h > gstMdtcCap.luma_h ) {
        propt->luma_h = gstMdtcCap.luma_h ;    
    }
    
    if( propt->luma_w < 160 ) {
        propt->luma_w = 160 ;
    }
    if( propt->luma_h <  90 ) {
        propt->luma_h =  90 ;
    }

    propt->window.x_start   = 0 ;
    propt->window.x_end     = propt->luma_w - 1;
    propt->window.y_start   = 0 ;
    propt->window.y_end     = propt->luma_h - 1;
    #if MD_USE_ROI_TYPE==0
    propt->window.div_x     = menu.ubMDDivX;
    propt->window.div_y     = menu.ubMDDivY;
    #else
    propt->window.div_x     = 1;
    propt->window.div_y     = 1;
    if(!propt->window.roi_num) {
        propt->window.roi_num   = menu.ubMDRoiNum;
        roi_from_menu = MMP_TRUE ;
    }
    #endif
 
    #if MD_USE_ROI_TYPE==0
    if( propt->window.div_x > gstMdtcCap.div_x ) {
        propt->window.div_x = gstMdtcCap.div_x ;    
    }

    if( !propt->window.div_x ) {
        propt->window.div_x = 1 ;
    }

    if( !propt->window.div_y  ) {
        propt->window.div_y = 1 ;    
    }
    #else
    if( propt->window.roi_num > MDTC_MAX_ROI ) {    
        propt->window.roi_num = MDTC_MAX_ROI ;
    }
    #endif
    #if MD_USE_ROI_TYPE==0
    for(x = 0; x < propt->window.div_x; x++) {
        for(y = 0; y < propt->window.div_y; y++) {
            propt->param[x][y].enable               = en ;
            propt->param[x][y].size_perct_thd_min   = menu.ubPerctThdMin ; 
            propt->param[x][y].size_perct_thd_max   = menu.ubPerctThdMax ;
            propt->param[x][y].sensitivity          = menu.ubSensitivity ;
            propt->param[x][y].learn_rate           = menu.usLearnRate ;
        
        }    
    }
    #else
    if(roi_from_menu) {
        for(x=0;x<propt->window.roi_num;x++) {
            propt->roi[x]   = menu.roi[x]   ; 
            propt->param[x] = menu.roi_p[x] ;
        }
    }
    #endif
    #if 0
    printc("[md].enable:%d,w x h = (%d,%d)\r\n",menu.bMDEnable,propt->luma_w ,propt->luma_h );
    printc("[md].div_x,div_y : %d,%d\r\n",propt->window.div_x,propt->window.div_y );
    printc("[md].thd min,max : %d,%d\r\n",propt->param[0][0].size_perct_thd_min,propt->param[0][0].size_perct_thd_max);
    printc("[md].sensitivity,learn_rate : %d,%d\r\n",propt->param[0][0].sensitivity,propt->param[0][0].learn_rate);
    #endif
 }
#endif

MMP_BOOL UI_Get_DualCPU(void)
{
#if 1 //TBD
    return MMP_TRUE ;
#else
    return menu.bDualCPU ;
#endif    
}

MMP_UBYTE UI_Get_PowerLine(void)
{

    if( menu.ubPowerLine >= V4L2_CID_POWER_LINE_FREQUENCY_AUTO )
        menu.ubPowerLine = V4L2_CID_POWER_LINE_FREQUENCY_60HZ ;    
    return menu.ubPowerLine ;        
}


MMP_UBYTE UI_Get_Orientation(void)
{
    if( menu.ubOrientation > V4L2_CAMERA_AIT_ORTN_FLIP_MIRROR ) {
        menu.ubOrientation = V4L2_CAMERA_AIT_ORTN_NORMAL ;        
    }
    return menu.ubOrientation ;
}

MMP_UBYTE UI_Get_NightVision(void)
{
    if( menu.ubNightVision > V4L2_CAMERA_AIT_NV_AUTO ) {
        menu.ubNightVision = V4L2_CAMERA_AIT_NV_AUTO ;
    }  
    return menu.ubNightVision ;
}

MMP_UBYTE UI_Get_LightSensorTH_High(void)
{
    return menu.ubLightSensorH ;
}

MMP_UBYTE UI_Get_LightSensorTH_Low(void)
{
    return menu.ubLightSensorL ;
}

MMP_UBYTE UI_Get_TempSensorTH_High(void)
{
    return menu.ubTempSensorH ;
}

MMP_UBYTE UI_Get_TempSensorTH_Low(void)
{
    return menu.ubTempSensorL ;
}

// here return 0~127 , 128 level
MMP_UBYTE UI_Get_IRLED_Level(void)
{
    return menu.ubIRLedLevel ;
}

#if SUPPORT_OSD_FUNC
MMP_BOOL UI_Get_OsdStrEnable(MMP_ULONG streamset)
{
    return menu.OsdStrEnable[streamset];
}

MMP_BOOL UI_Get_OsdRtcEnable(MMP_ULONG streamset)
{
    return menu.OsdRtcEnable[streamset];
}

MMP_BOOL UI_Get_OsdBootStream(void)
{
    return menu.OsdBootStream;
}


char* UI_Get_OsdStr(void)
{
    return menu.OsdStr ;
}

char* UI_Get_RtcStr(void)
{
    return menu.RtcStr ;
}

MMP_ULONG UI_Get_Str_Pos_startX(void)
{
    return menu.str_pos_startX ;
}

MMP_ULONG UI_Get_Str_Pos_startY(void)
{
    return menu.str_pos_startY ;
}

MMP_ULONG UI_Get_Str_Pos_X(void)
{
    return menu.str_pos_x ;
}

MMP_ULONG UI_Get_Str_Pos_Y(void)
{
    return menu.str_pos_y ;
}

MMP_ULONG UI_Get_Rtc_Pos_startX(void)
{
    return menu.rtc_pos_startX ;
}

MMP_ULONG UI_Get_Rtc_Pos_startY(void)
{
    return menu.rtc_pos_startY ;
}

MMP_ULONG UI_Get_Rtc_Pos_X(void)
{
    return menu.rtc_pos_x ;
}

MMP_ULONG UI_Get_Rtc_Pos_Y(void)
{
    return menu.rtc_pos_y ;
}
#endif

#if MIPI_TEST_PARAM_EN
void UI_GetMIPIParamters(MMP_UBYTE *sot,MMP_UBYTE *delay,MMP_UBYTE *edge)
{
  *sot   = menu.ubSOT   ;
  *delay = menu.ubDelay ;
  *edge  = menu.ubLatchEdge ;
}

MMP_ULONG UI_GetPFrameTH(void)
{
  return ((MMP_ULONG)menu.ubPFrameTH) * 1000  ;
}


#endif

#if CONFIG_HW_FOR_DRAM_TIMIMG_TEST==1
void dump_data(volatile char *data,int size)
{
    MMP_ULONG i;
    for(i = 0; i < size; i++) {
        if( i && !( i & 15 )) {
            RTNA_DBG_Str(0, "\r\n");    
        }
        RTNA_DBG_Byte(0, data[i]);
    }
    RTNA_DBG_Str(0, "\r\n");
}

void dump_reg()
{
    printc("--DRAM register--\r\n");
    dump_data((volatile char *)0x80006E00,256);

    printc("--DDR3 register--\r\n");
    dump_data((volatile char *)0x80003000,256);
    
/*
    printc("--GBL1 register--\r\n");
    dump_data((volatile char *)0x80005d00,256);

    printc("--GBL2 register--\r\n");
    dump_data((volatile char *)0x80006900,256);

    printc("--VIF register--\r\n");
    dump_data((volatile char *)0x80006100,16);
    
    printc("--BAYER register--\r\n");
    dump_data((volatile char *)0x80007100,256);
    printc("--SCALER register--\r\n");    
    dump_data((volatile char *)0x80004500,512);
*/    
}
#endif

void UI_Main_Task(void *p_arg)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;
    MMP_USHORT alarmPeriod,alarmOn;
	unsigned char gpio_value;

#if CONFIG_HW_FOR_DRAM_TIMIMG_TEST
    dump_reg();
#endif
    RTNA_DBG_Str3("UI_Task()\r\n");
    UI_Init_MenuSetting(&menu);
    UI_Get_BootCapture(&menu);
#if SUPPORT_OSD_FUNC
    OSD_Run();
#endif
    // initial IO after get boot setting
    IO_Init();

    if (menu.bAutoShot) {
        #if (V4L2_JPG)
        if (menu.ubBootMode == BOOT_MODE_CAMERA) {
            #if (V4L2_JPG)
            Jpeg_Stream();
            #endif
        }
        else
        #endif
        {
            if(menu.bSnapShot) {
                #if (V4L2_JPG)
                Jpeg_Stream();
                #endif
            }
            #if (V4L2_AAC)
            Audio_Stream();
            #endif
            #if (V4L2_H264)
            Video_Stream();
            #endif
        }
    }

    #if (SUPPORT_ALSA) && (ALSA_PWRON_PLAY)
    /* for debugging/testing only */
	MMPF_PIO_GetData(MMP_GPIO113, &gpio_value);
	printc("==========gpio113 %d\r\n", gpio_value);
	if (gpio_value) {
		Alsa_Open();
	}
    #endif

    wait_flags = UI_FLAG_STREAM_ON | UI_FLAG_STREAM_OFF;
    #if CONFIG_HW_FOR_DRAM_TIMIMG_TEST==0
    MMPF_SYS_PowerSaving();
    #endif
    
    #if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
    MMPF_OS_SetFlags(IPC_UI_Flag, UI_FLAG_STREAM_OFF, MMPF_OS_FLAG_SET);
    #endif

    while(1) {
        MMPF_OS_WaitFlags(IPC_UI_Flag, wait_flags,
                          MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                          0, &flags);

        if (flags & UI_FLAG_STREAM_ON) {
        }
        if (flags & UI_FLAG_STREAM_OFF) {
            #if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
            /* for debugging/testing only */
			if (gpio_value) {
				MMPF_OS_Sleep(5000);
				MMPF_OS_Sleep(4000);
				Alsa_Close();
				printc("==========%s %d %d\r\n", __func__, __LINE__, OSTime);
			}
            #endif
        }
    }
}
