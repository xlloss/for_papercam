//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

/**
 @file ex_vidrec.c
 @brief Video record related sample code
 @author Alterman
 @version 1.0
*/

#include "lib_retina.h"
#include "ait_utility.h"
#include "ex_vidrec.h"
#include "mmps_sensor.h"
#include "mmpf_sensor.h"

#include "ipc_cfg.h"
#include "isp_if.h"
#if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
#include "mmpf_alsa.h"
#include "camera_tone.h"
#endif
#if (SUPPORT_MDTC)
#include "mmps_mdtc.h"
#endif
#if (SUPPORT_V4L2)
#include "dualcpu_v4l2.h"
#endif
#if (SUPPORT_IVA)
#include "mmpf_iva.h"
#endif

#include "ait_osd.h"
#include "ex_misc.h"
#include "isp_if.h"
#include "misc.h"
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local variables
 */
static MMP_UBYTE        m_ubMainSensor = PRM_SENSOR;

#if (V4L2_H264)
/* stream object instance */
/*static*/ VSTREAM_RESOURCE m_VidStream[MAX_H264_STREAM_NUM] = {
    {NULL, MMP_FALSE, MMP_FALSE},
#if (MAX_H264_STREAM_NUM > 1)
    {NULL, MMP_FALSE, MMP_FALSE},
#endif
#if (MAX_H264_STREAM_NUM > 2)
    {NULL, MMP_FALSE, MMP_FALSE},
#endif
};
#endif

#if (V4L2_AAC)
static ASTREAM_RESOURCE m_AudStream[MAX_AUD_STREAM_NUM] = {
    {NULL, MMP_FALSE, MMP_FALSE},
#if (MAX_AUD_STREAM_NUM > 1)
    {NULL, MMP_FALSE, MMP_FALSE},
#endif
#if (MAX_AUD_STREAM_NUM > 2)
    {NULL, MMP_FALSE, MMP_FALSE},
#endif
};
#endif

#if (V4L2_JPG)
static JSTREAM_RESOURCE m_JpgStream[MAX_JPG_STREAM_NUM] = {
    {NULL, MMP_FALSE},
#if (MAX_JPG_STREAM_NUM > 1)
    {NULL, MMP_FALSE},
#endif
#if (MAX_JPG_STREAM_NUM > 2)
    {NULL, MMP_FALSE},
#endif
};
#endif

#if (V4L2_GRAY)
static YSTREAM_RESOURCE m_GrayStream[MAX_GRAY_STREAM_NUM] = {
    {NULL, MMP_FALSE},
#if (MAX_GRAY_STREAM_NUM > 1)
    {NULL, MMP_FALSE},
#endif
#if (MAX_GRAY_STREAM_NUM > 2)
    {NULL, MMP_FALSE},
#endif
};
#endif

/* Support 1 seamless mode stream once for video & audio each */
#if (V4L2_H264)
static MMP_ULONG        m_NumSeamlessVidStream = 0;
#endif
#if (V4L2_AAC)
static MMP_ULONG        m_NumSeamlessAudStream = 0;
#endif
#if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
static AUTL_RINGBUF     m_AlsaRingBuf;
#endif

static PIPE_RESOURCE PIPE_USAGE_MAP[MMP_IBC_PIPE_MAX] ;
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
/* Local functions */
static void         Video_SetAttribute(MMPS_VSTREAM_PROPT *propt);
#if SUPPORT_OSD_FUNC
void         OSD_SetAttribute(MMPS_VSTREAM_PROPT *propt);
extern void aitcam_osd_drawer(void *argc);
#endif

/* External functions */
extern MMP_ULONG        UI_Get_Resolution(MMP_USHORT *w,MMP_USHORT *h);
extern MMP_ULONG        UI_Get_Bitrate(MMP_BOOL video);
extern MMP_ULONG        UI_Get_GopSize(void);
extern MMP_ULONG        UI_Get_SampleRate(void);
extern MMP_BOOL         UI_Get_Seamless(void);
extern MMP_BOOL         UI_Get_RcSkip(void);
extern VIDENC_RC_MODE   UI_Get_RcMode(void);
extern MMP_USHORT       UI_Get_StartIFrames(void);
extern MMP_UBYTE        UI_Get_InitQP(void);
extern MMP_UBYTE        UI_Get_MinQP(void);
extern MMP_UBYTE        UI_Get_MaxQP(void);
extern MMP_UBYTE        UI_Get_PowerLine(void) ;
extern MMP_UBYTE        UI_Get_Orientation(void) ;
extern MMP_UBYTE        UI_Get_MdEnable(void);
extern MMP_USHORT       UI_Get_Fps(void);
extern MMP_UBYTE        UI_Get_NightVision(void);
#if (SUPPORT_MDTC)
extern void             UI_Set_MdProperties(MMPS_MDTC_PROPT *prot, MMP_BOOL en);
extern void MdEventToCpuA(int new_val,int old_val);
#endif
extern int aitcam_ipc_stream2obj(int streamid, unsigned long fmt);
extern unsigned long aitcam_ipc_get_format(int streamid);
extern MMP_BOOL                UI_Get_OsdStrEnable(MMP_ULONG streamset);
extern MMP_BOOL                UI_Get_OsdRtcEnable(MMP_ULONG streamset);
extern MMP_BOOL                UI_Get_Rtc_Pos_startX(void);
extern MMP_BOOL                UI_Get_Rtc_Pos_startY(void);
extern MMP_BOOL                UI_Get_Str_Pos_startX(void);
extern MMP_BOOL                UI_Get_Str_Pos_startY(void);
extern char*                   UI_Get_OsdStr(void);
extern char*                   UI_Get_RtcStr(void);
extern MMP_ULONG               UI_Get_Str_Pos_X(void);
extern MMP_ULONG               UI_Get_Str_Pos_Y(void);
extern MMP_ULONG               UI_Get_Rtc_Pos_X(void);
extern MMP_ULONG               UI_Get_Rtc_Pos_Y(void);
extern void UI_Get_CaptureRes(MMP_USHORT *w, MMP_USHORT *h);
//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================
#if (SUPPORT_MDTC)||(V4L2_H264)||(V4L2_JPG)||(V4L2_GRAY)
static int m_NumOpenSensor = 0 ;

MMP_UBYTE Preset_SensorMode(void)
{
    MMP_UBYTE ubSnrMode = DEFAULT_SNR_MODE;
    #if (PLL_FOR_ULTRA_LOWPOWER==1 ) 
    {
      
        MMP_USHORT w, h;
        UI_Get_Resolution(&w, &h);
        if(w <= 1280 ) {
            int fps = UI_Get_Fps();
            if(fps > 30) {
              ubSnrMode = 2;
              printc("#720p 60fps mode\r\n");
            }
            else {
              ubSnrMode = 1;
              //printc("#720p low power mode\r\n");
            }
        }
    }
    #endif
    MMPS_Sensor_InitPresetMode(m_NumOpenSensor, ubSnrMode) ;
    return ubSnrMode ;
}

/*static*/ void Sensor_Init(void)
{
    MMP_ERR ret = MMP_ERR_NONE ;
    ISP_AE_FLICKER fkr ;
    MMP_UBYTE ubSnrMode = DEFAULT_SNR_MODE;   
    ubSnrMode = Preset_SensorMode();
    
    /* Initial Sensor */
    if (MMPS_Sensor_IsInitialized(m_ubMainSensor) == MMP_FALSE) {
        int i ;
        ret = MMPS_Sensor_Initialize(m_ubMainSensor, ubSnrMode, MMP_SNR_VID_PRW_MODE);
        for(i=0;i<MMP_IBC_PIPE_MAX;i++) {
            Set_Obj_Pipe(i,-1,0);
        }
        
    }    
    if(ret == MMP_ERR_NONE) {
        m_NumOpenSensor++ ;    
    }

    /*No need to set again when sensor is intialized*/
    if( m_NumOpenSensor > 1 ) {
      return ;
    }

    ISP_IF_AE_SetMaxSensorFPSx10( UI_Get_Fps() * 10 ) ;
    
    //printc("Sensor_Init:%d\r\n",m_NumOpenSensor);
    /*set sensor parameter here*/
    switch( UI_Get_PowerLine() ) {
    case V4L2_CID_POWER_LINE_FREQUENCY_DISABLED:
        fkr = ISP_AE_FLICKER_OFF ;
        break ;
    case V4L2_CID_POWER_LINE_FREQUENCY_50HZ:
        fkr = ISP_AE_FLICKER_50HZ ;
        break ;
    default:
        fkr = ISP_AE_FLICKER_60HZ ;
        break ;          
    }
    ISP_IF_AE_SetFlicker(fkr);
    
    switch ( UI_Get_Orientation() ) {
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
    #if SUPPORT_IRLED
    printc("#Menu.NV : %d\r\n",UI_Get_NightVision() );
    SetNightVision( UI_Get_NightVision() );
    #endif

}
/*
power down sensor if no used, for power saving....
*/
/*static*/ void Sensor_DeInit(void)
{
    if ( m_NumOpenSensor > 0 ) {
        m_NumOpenSensor-- ;
    }
    //printc("Sensor_DeInit:%d\r\n",m_NumOpenSensor);

    if(!m_NumOpenSensor) {
        printc("Shut-down sensor\r\n");
        MMPS_Sensor_PowerDown(m_ubMainSensor);
    }
}
#endif

#if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
//------------------------------------------------------------------------------
//  Function    : Alsa_Open
//  Description :
//------------------------------------------------------------------------------
void Alsa_Open(void)
{
    MMP_ERR err;
    static MMPF_ALSA_PROPT propt;
		
    printc("%s\r\n", __func__);
    AUTL_RingBuf_Init(&m_AlsaRingBuf, (MMP_ULONG)m_camera_tone,
                                        sizeof(m_camera_tone));
    // all data are ready in buffer
    AUTL_RingBuf_CommitWrite(&m_AlsaRingBuf, sizeof(m_camera_tone));

    propt.fs    = 8000;
    propt.ch    = 1;
    propt.thr   = 2 * 8 * 8 * 10; // 8ms
    propt.ipc   = MMP_FALSE;
    propt.buf   = &m_AlsaRingBuf;

    err = MMPF_Alsa_TriggerOP(ALSA_ID_SPK_OUT, ALSA_OP_OPEN, &propt);
    if (err) {
        printc("alsa open failed\r\n");
        return;
    }

	propt.period = propt.thr << 3;
	err = MMPF_Alsa_TriggerOP(ALSA_ID_SPK_OUT, ALSA_OP_TRIGGER_ON, &propt.period);
	if (err) {
		printc("alsa trigger on failed\r\n");
		return;
	}
}

void Alsa_Close(void)
{
    MMP_ERR err;

	MMPF_OS_Sleep((sizeof(m_camera_tone)/16));
    err = MMPF_Alsa_TriggerOP(ALSA_ID_SPK_OUT, ALSA_OP_TRIGGER_OFF, NULL);
    if (err) {
        printc("alsa trigger off failed\r\n");
        return;
    }
    err = MMPF_Alsa_TriggerOP(ALSA_ID_SPK_OUT, ALSA_OP_CLOSE, NULL);
    if (err) {
        printc("alsa close failed\r\n");
        return;
    }
}
#endif

#if (SUPPORT_MDTC)
//------------------------------------------------------------------------------
//  Function    : Mdtc_Open
//  Description :
//------------------------------------------------------------------------------
void Mdtc_Open(void)
{
    MMPS_MDTC_PROPT propt;
    if (UI_Get_MdEnable()&0x01) {
        /* Initial Sensor */
        Sensor_Init();
    }
    printc("Mdtc_Open\r\n");
    MMPS_MDTC_ModInit();

#if 0
    propt.luma_w = 160;
    propt.luma_h = 120;

    propt.window.x_start    = 0;
    propt.window.x_end      = propt.luma_w - 1;
    propt.window.y_start    = 0;
    propt.window.y_end      = propt.luma_h - 1;
    propt.window.div_x      = 1;
    propt.window.div_y      = 1;

    propt.param[0][0].enable             = 1;
    propt.param[0][0].size_perct_thd_min = 4;
    propt.param[0][0].size_perct_thd_max = 100;
    propt.param[0][0].sensitivity        = 57;
    propt.param[0][0].learn_rate         = 500;
#else
    propt.luma_w = 0;
    propt.luma_h = 0;
    #if MD_USE_ROI_TYPE==1
    propt.window.roi_num = 0 ;
    #endif
    UI_Set_MdProperties(&propt,MMP_TRUE);
#endif
    propt.snrID = m_ubMainSensor;

    if (UI_Get_MdEnable()&0x01) {
        MMPS_MDTC_Open(&propt,MdEventToCpuA);
        MMPS_MDTC_Start();
    }
}

//------------------------------------------------------------------------------
//  Function    : Mdtc_Close
//  Description :
//------------------------------------------------------------------------------
void Mdtc_Close(void)
{
    MMPS_MDTC_Stop();
    MMPS_MDTC_Close();
    Sensor_DeInit();
}
#endif //(SUPPORT_MDTC)


#if (SUPPORT_IVA)
static MMP_ULONG iva_id = MAX_GRAY_STREAM_NUM ;

extern aitcam_streamtbl gray_stream_tbl[MAX_GRAY_STREAM_NUM];

//------------------------------------------------------------------------------
//  Function    : Iva_Open
//  Description :
//------------------------------------------------------------------------------
void IVA_Open(void)
{
    MMP_ULONG id = Gray_Init(MMP_TRUE);
    if(id < MAX_GRAY_STREAM_NUM) {
        gray_stream_tbl[id].streamid = IVA_STREAM_ID ;
        gray_stream_tbl[id].used     = 1 ;
        if (Gray_Open(id, IVA_IMG_W, IVA_IMG_H ,IVA_IMG_FMT) == 0) {
            MMPF_IVA_Init();
            MMPF_IVA_Open( m_GrayStream[id].obj );
            Gray_On(id,IVA_STREAM_ID);
        }
        else {
            gray_stream_tbl[id].streamid = AITCAM_NUM_CONTEXTS;
            gray_stream_tbl[id].used     = 0;            
        }
        
    }
    iva_id = id ;
}

//------------------------------------------------------------------------------
//  Function    : Mdtc_Close
//  Description :
//------------------------------------------------------------------------------
void IVA_Close(void)
{
    if( iva_id < MAX_GRAY_STREAM_NUM) {
        MMPF_IVA_Close( );
        Gray_Off(iva_id );
        Gray_Close(iva_id);
        gray_stream_tbl[iva_id].streamid = AITCAM_NUM_CONTEXTS;
        gray_stream_tbl[iva_id].used     = 0;
    }
}
#endif //(SUPPORT_IVA)

#if (SUPPORT_V4L2)
#if (V4L2_H264)
//------------------------------------------------------------------------------
//  Function    : Video_Stream
//  Description :
//------------------------------------------------------------------------------
extern MMP_BOOL UI_Get_OsdBootStream(void);
void Video_Stream(void)
{
    MMP_ULONG id;
    MMPS_VSTREAM_PROPT propt;
/*
Fixed bug if def snr mode != 0
*/
    Preset_SensorMode();
    
    Video_SetAttribute(&propt);
    /* Start video encoding */
    // Get stream id resource, but not occupy it
    id = Video_Init(MMP_FALSE, propt.seamless);
    if (id < MAX_H264_STREAM_NUM) {
        if (propt.seamless && m_NumSeamlessVidStream)
            m_NumSeamlessVidStream--;
#if SUPPORT_OSD_FUNC
        if ( UI_Get_OsdBootStream() == STREAM_SELECT_1 )
        {
            printc("BOOT select stream 1 \r\n");
            Video_Open(id, &propt,STREAM_SELECT_1);
        }
        else if ( UI_Get_OsdBootStream() == STREAM_SELECT_0 )
        {
            printc("BOOT select stream 0 \r\n");
            Video_Open(id, &propt,STREAM_SELECT_0);
        }
#else
        Video_Open(id,&propt,STREAM_SELECT_0);
#endif

    }
}
#endif

#if (V4L2_AAC)
//------------------------------------------------------------------------------
//  Function    : Audio_Stream
//  Description :
//------------------------------------------------------------------------------
void Audio_Stream(void)
{
    MMP_ULONG id;
    MMPS_ASTREAM_PROPT propt;

    propt.feat.src      = MMP_AUD_AFE_IN_DIFF;
    propt.feat.fmt      = MMP_ASTREAM_AAC;
    propt.feat.fs       = UI_Get_SampleRate();
    propt.feat.bitrate  = UI_Get_Bitrate(MMP_FALSE);//128000;
    propt.feat.buftime  = 8000;
    propt.seamless      = UI_Get_Seamless();

    /* Start audio encoding */
    // Get stream id resource, but not occupy it
    id = Audio_Init(MMP_FALSE, propt.seamless);
    if (id < MAX_AUD_STREAM_NUM) {
        if (propt.seamless && m_NumSeamlessAudStream)
            m_NumSeamlessAudStream--;
        Audio_Open(id, &propt);
    }
}
#endif

#if (V4L2_JPG)
//------------------------------------------------------------------------------
//  Function    : Jpeg_Stream
//  Description :
//------------------------------------------------------------------------------
void Jpeg_Stream(void)
{
    MMP_ULONG id;
    MMPS_JSTREAM_PROPT propt;
    MMP_USHORT w,h ;
    UI_Get_CaptureRes(&w,&h) ;
    propt.snrID = m_ubMainSensor;
    propt.w = w ;
    propt.h = h ;
    propt.size = 1500;       // 500KB	propt.size * 5/4 * 95/100 < propt.bufsize
    propt.bufsize = 2000*1024;   // 900KB

    id = Jpeg_Init(MMP_FALSE);  // Get stream id resource, but not occupy it
    if (id < MAX_JPG_STREAM_NUM)
        Jpeg_Open(id, &propt);
}
#endif

#if (V4L2_H264)
//------------------------------------------------------------------------------
//  Function    : Video_Init
//  Description :
//------------------------------------------------------------------------------
MMP_ULONG Video_Init(MMP_BOOL occupy, MMP_BOOL seamless)
{
    MMP_ULONG id;

    /* For seamless stream, use the existed one */
    if (seamless) {
        if (m_NumSeamlessVidStream) {
            /* Support up to 1 activate seamless stream once.
             * To open 2 seamless streams at the same time is not allowed.
             */
            printc("Loop video stream occupied\r\n");
            return MAX_H264_STREAM_NUM;
        }
        for(id = 0; id < MAX_H264_STREAM_NUM; id++) {
            /* attach to an opened seamless video stream */
            if (m_VidStream[id].obj && m_VidStream[id].obj->propt.seamless) {
                if ((m_VidStream[id].obj->state == IPC_STREAMER_OPEN) &&
                    !m_VidStream[id].inuse)
                {
                    if (occupy)
                        m_VidStream[id].inuse = MMP_TRUE;
                    m_VidStream[id].seamless = MMP_TRUE;
                    m_NumSeamlessVidStream++;
                    return id;
                }
            }
        }
    }

    for(id = 0; id < MAX_H264_STREAM_NUM; id++) {
        if ((m_VidStream[id].obj == NULL) && !m_VidStream[id].inuse) {
            if (occupy)
                m_VidStream[id].inuse = MMP_TRUE;
            if (seamless) {
                m_VidStream[id].seamless = MMP_TRUE;
                m_NumSeamlessVidStream++;
            }
            return id;
        }
    }

    printc("No video stream resource\r\n");
    return MAX_H264_STREAM_NUM;
}

//------------------------------------------------------------------------------
//  Function    : Video_Open
//  Description :
//------------------------------------------------------------------------------
#if SUPPORT_OSD_FUNC
extern AIT_OSD_BUFDESC desc_osd[MMP_IBC_PIPE_MAX];
#endif

int Video_Open(MMP_ULONG id, MMPS_VSTREAM_PROPT *propt, MMP_ULONG streamset)
{
    MMP_ULONG argc;
    MMP_IBC_PIPEID pid;

    if (id >= MAX_H264_STREAM_NUM)
        return -1;

    /* Seamless stream existed, no need to open again */
    if (propt->seamless && m_VidStream[id].obj)
        return 0;

    /* Initialize VStream Module */
    MMPS_VStream_ModInit();

    /* Initial Sensor */
    Sensor_Init();


    printc("Video%d_Open:%d\r\n", id,OSTime);
    argc = (streamset << 8);
#if SUPPORT_OSD_FUNC
    propt->CallBackFunc = (ipc_osd_drawer*)aitcam_osd_drawer; 
    propt->CallBackArgc = argc;
#endif
    m_VidStream[id].obj = MMPS_VStream_Open(propt);
    
    if (!m_VidStream[id].obj) {
        m_VidStream[id].inuse = MMP_FALSE;
        printc("\t-> failed\r\n");
        return -2;
    }
    Set_Obj_Pipe(m_VidStream[id].obj->pipe.ibcpipeID,id,V4L2_PIX_FMT_H264);

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Video_On
//  Description :
//------------------------------------------------------------------------------
int Video_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!m_VidStream[id].obj || !m_VidStream[id].inuse)
        return -2;

    printc("Video%d_On:%d\r\n", id,OSTime);

#if SUPPORT_OSD_FUNC
    OSD_Run();
#endif
    err = MMPS_VStream_Start(m_VidStream[id].obj, v4l2_id, opt);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Video_Off
//  Description :
//------------------------------------------------------------------------------
int Video_Off(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_VidStream[id].obj || !m_VidStream[id].inuse)
        return -2;

    printc("Video%d_Off\r\n", id);
    #if SUPPORT_OSD_FUNC
    OSD_Stop();
    #endif
    err = MMPS_VStream_Stop(m_VidStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Video_Close
//  Description :
//------------------------------------------------------------------------------
int Video_Close(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_VidStream[id].obj || !m_VidStream[id].inuse)
        return -2;

    /* Never close the seamless stream */
    if (m_VidStream[id].obj->propt.seamless) {
        m_VidStream[id].inuse = MMP_FALSE;
        m_NumSeamlessVidStream--;
        return 0;
    }

    printc("Video%d_Close\r\n", id);
    err = MMPS_VStream_Close(m_VidStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    if (m_VidStream[id].seamless)
        m_NumSeamlessVidStream--;
        
    Set_Obj_Pipe( m_VidStream[id].obj->pipe.ibcpipeID , -1 , 0);

    m_VidStream[id].obj = NULL;
    m_VidStream[id].inuse = MMP_FALSE;
    m_VidStream[id].seamless = MMP_FALSE;
    Sensor_DeInit();
    return 0;
}

MMPS_VSTREAM_CLASS *Video_Obj(MMP_ULONG id)
{
    return m_VidStream[id].obj ;
}
//------------------------------------------------------------------------------
//  Function    : Video_Control
//  Description :
//------------------------------------------------------------------------------
int Video_Control(MMP_ULONG id, IPC_VIDEO_CTL ctrl, VIDENC_CTL_SET *param)
{
    MMP_ERR err;

    if (!m_VidStream[id].obj || !m_VidStream[id].inuse)
        return -2;

    printc("Video%d_Control(%d)\r\n", id, ctrl);
    err = MMPS_VStream_Control(m_VidStream[id].obj, ctrl, param);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Video_GetAttribute
//  Description :
//------------------------------------------------------------------------------
int Video_GetAttribute(MMP_ULONG id, MMPS_VSTREAM_PROPT **propt)
{
    if (!m_VidStream[id].obj || !m_VidStream[id].inuse) {
        *propt = NULL;
        return -2;
    }

    *propt = &m_VidStream[id].obj->propt;
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Video_SetAttribute
//  Description :
//------------------------------------------------------------------------------
extern MMP_USHORT UI_Get_Profile(void);
extern MMP_ULONG UI_Get_VideoTargetBitrate(void);
#if SUPPORT_OSD_FUNC
extern MMPF_OS_FLAGID	OSD_Flag;
#endif
static void Video_SetAttribute(MMPS_VSTREAM_PROPT *propt)
{
    MMP_USHORT w, h;
    VIDENC_CTL_SET *ctl = &propt->ctl;

    UI_Get_Resolution(&w, &h);

    /* stream properties */
    propt->snrID        = m_ubMainSensor;
    propt->seamless     = UI_Get_Seamless();
    propt->w            = w;
    propt->h            = h;
    propt->infrmslots   = 2;
    propt->outbuf_ms    = 8000;

    MMPS_Sensor_GetCurFpsx10(m_ubMainSensor, &propt->snr_fps.ulResol);
    propt->snr_fps.ulResol     *= 100;
    propt->snr_fps.ulIncr       = 1000;
    propt->snr_fps.ulIncrx1000  = 1000000;

    /* video control properties */
    ctl->enc_fps     = propt->snr_fps;
    ctl->gop         = UI_Get_GopSize();
    ctl->profile     = UI_Get_Profile();
    ctl->level       = 40;
    if(ctl->profile==H264ENC_BASELINE_PROFILE) {
        ctl->entropy     = H264ENC_ENTROPY_CAVLC;
    }
    else{
        ctl->entropy     = H264ENC_ENTROPY_CABAC;
    }
    ctl->bitrate     = UI_Get_Bitrate(MMP_TRUE);
    ctl->rampup_bitrate  = ctl->bitrate ;
    ctl->target_bitrate = UI_Get_VideoTargetBitrate();
    ctl->rc_mode     = UI_Get_RcMode();
    ctl->rc_skip     = UI_Get_RcSkip();
    ctl->start_i_frames = UI_Get_StartIFrames();
    
    ctl->rc_skiptype = VIDENC_RC_SKIP_SMOOTH;
    ctl->lb_size     = 1000; // 1 sec
    ctl->tnr         = 0;

    ctl->qp_init[0]  =
    ctl->qp_init[1]  = UI_Get_InitQP();
    ctl->qp_bound[0] = UI_Get_MinQP(); // QP low bound
    ctl->qp_bound[1] = UI_Get_MaxQP(); // QP high bound

    printc("- video resol,br,seamless,gop,profile: %d x %d,%d,%d,%d,%d\r\n", w, h,propt->ctl.bitrate,propt->seamless,propt->ctl.gop,propt->ctl.profile);
	#if SUPPORT_OSD_FUNC
	MMPF_OS_SetFlags(OSD_Flag, OSD_FLAG_OSDRES, MMPF_OS_FLAG_SET);
	#endif
}

#if SUPPORT_OSD_FUNC
ait_fdframe_config *osd_base_fdfr;
void OSD_Run()
{
    //MMPF_OS_SetFlags(OSD_Flag, OSD_FLAG_RUN, MMPF_OS_FLAG_SET);
}

void OSD_Stop()
{
    //MMPF_OS_SetFlags(OSD_Flag, OSD_FLAG_STOP, MMPF_OS_FLAG_SET);
}

void OSD_SetAttribute(MMPS_VSTREAM_PROPT *propt)
{
    VIDENC_CTL_SET *ctl = &propt->ctl;
    char *rtc_str, *str_str;

    propt->OsdRtcEnable = 0 ;
    propt->OsdStrEnable = 0 ;
    ctl->osd_config_str.type = 0;
    ctl->osd_config_rtc.type = 0;

    ctl->osd_config_rtc.type = AIT_OSD_TYPE_INACTIVATE;

    ctl->osd_config_str.type = AIT_OSD_TYPE_INACTIVATE;

    propt->rtc_pos_startX = UI_Get_Rtc_Pos_startX();
    propt->rtc_pos_startY = UI_Get_Rtc_Pos_startY();
    propt->str_pos_startX = UI_Get_Str_Pos_startX();
    propt->str_pos_startY = UI_Get_Str_Pos_startY();
    


    /* video rtc properties */
    ctl->rtc_config.usYear     = 1970;
    ctl->rtc_config.usMonth    = 0;
    ctl->rtc_config.usDay      = 0;
    ctl->rtc_config.usHour     = 0;
    ctl->rtc_config.usMinute   = 0;
    ctl->rtc_config.usSecond   = 0;
    ctl->rtc_config.bOn        = 1;

   
    /* video OSD properties */
    ctl->osd_config_rtc.index  = 0;
    ctl->osd_config_rtc.TextColorY = 0x00FF;
    ctl->osd_config_rtc.TextColorU = 0x0080;
    ctl->osd_config_rtc.TextColorV = 0x0080;

    ctl->osd_config_rtc.pos[AXIS_X] = UI_Get_Rtc_Pos_X();
    ctl->osd_config_rtc.pos[AXIS_Y] = UI_Get_Rtc_Pos_Y();



    ctl->osd_config_str.index  = 0;
    ctl->osd_config_str.TextColorY = 0x00FF;
    ctl->osd_config_str.TextColorU = 0x0080;
    ctl->osd_config_str.TextColorV = 0x0080;

    ctl->osd_config_str.pos[AXIS_X] = UI_Get_Str_Pos_X();
    ctl->osd_config_str.pos[AXIS_Y] = UI_Get_Str_Pos_Y();


    str_str =  UI_Get_OsdStr();
    strcpy(ctl->osd_config_str.str,str_str);
    rtc_str =  UI_Get_RtcStr();
    strcpy(ctl->osd_config_rtc.str,rtc_str);

#if SUPPORT_OSD_FUNC
    printc("- osd 0/1/2 rtc enable: %d/%d/%d\r\n", UI_Get_OsdRtcEnable(0),UI_Get_OsdRtcEnable(1),UI_Get_OsdRtcEnable(2));
    printc("- osd 0/1/2 str enable: %d/%d/%d\r\n", UI_Get_OsdStrEnable(0),UI_Get_OsdStrEnable(1),UI_Get_OsdStrEnable(2));
    if(UI_Get_OsdRtcEnable(0)||UI_Get_OsdRtcEnable(1)||UI_Get_OsdRtcEnable(2)) {
      printc("- osd string pos[AXIS_X] : %d\r\n", ctl->osd_config_str.pos[0]);
      printc("- osd string pos[AXIS_Y] : %d\r\n", ctl->osd_config_str.pos[1]);
      printc("- osd str : %s\r\n", ctl->osd_config_str.str);
    }
    if(UI_Get_OsdStrEnable(0)||UI_Get_OsdStrEnable(1)||UI_Get_OsdStrEnable(2)) {
      printc("- osd TimeFormatSetting:(%d:%d)\r\n", ctl->osd_config_rtc.str[0], ctl->osd_config_rtc.str[1]);
      printc("- osd rtc pos[AXIS_X] : %d\r\n", ctl->osd_config_rtc.pos[0]);
      printc("- osd rtc pos[AXIS_Y] : %d\r\n", ctl->osd_config_rtc.pos[1]);
    }
#endif

}
#endif

#endif //(V4L2_H264)

#if (V4L2_AAC)
//------------------------------------------------------------------------------
//  Function    : Audio_Init
//  Description :
//------------------------------------------------------------------------------
MMP_ULONG Audio_Init(MMP_BOOL occupy, MMP_BOOL seamless)
{
    MMP_ULONG id;

    /* For seamless stream, use the existed one */
    if (seamless) {
        if (m_NumSeamlessAudStream) {
            /* Support up to 1 activate seamless stream once.
             * To open 2 seamless streams at the same time is not allowed.
             */
            printc("Loop audio stream occupied\r\n");
            return MAX_AUD_STREAM_NUM;
        }
        for(id = 0; id < MAX_AUD_STREAM_NUM; id++) {
            if (m_AudStream[id].obj && m_AudStream[id].obj->propt.seamless) {
                if ((m_AudStream[id].obj->state == IPC_STREAMER_OPEN) &&
                    !m_AudStream[id].inuse)
                {
                    if (occupy)
                        m_AudStream[id].inuse = MMP_TRUE;
                    m_AudStream[id].seamless = MMP_TRUE;
                    m_NumSeamlessAudStream++;
                    return id;
                }
            }
        }
    }

    for(id = 0; id < MAX_AUD_STREAM_NUM; id++) {
        if ((m_AudStream[id].obj == NULL) && !m_AudStream[id].inuse) {
            if (occupy)
                m_AudStream[id].inuse = MMP_TRUE;
            if (seamless) {
                m_NumSeamlessAudStream++;
                m_AudStream[id].seamless = MMP_TRUE;
            }
            return id;
        }
    }

    printc("No audio stream resource\r\n");
    return MAX_AUD_STREAM_NUM;
}

//------------------------------------------------------------------------------
//  Function    : Audio_Open
//  Description :
//------------------------------------------------------------------------------
int Audio_Open(MMP_ULONG id, MMPS_ASTREAM_PROPT *propt)
{
    if (id >= MAX_AUD_STREAM_NUM)
        return -1;

    /* Seamless stream existed, no need to open again */
    if (propt->seamless && m_AudStream[id].obj)
        return 0;

    /* Initialize AStream Module */
    MMPS_AStream_ModInit();

    printc("Audio%d_Open:%d\r\n", id,OSTime);
    m_AudStream[id].obj = MMPS_AStream_Open(propt);
    if (!m_AudStream[id].obj) {
        printc("\t-> failed\r\n");
        m_AudStream[id].inuse = MMP_FALSE;
        return -2;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Audio_On
//  Description :
//------------------------------------------------------------------------------
int Audio_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!m_AudStream[id].obj || !m_AudStream[id].inuse)
        return -2;

    printc("Audio%d_On:%d\r\n", id , OSTime);
    err = MMPS_AStream_Start(m_AudStream[id].obj, v4l2_id, opt);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Audio_Off
//  Description :
//------------------------------------------------------------------------------
int Audio_Off(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_AudStream[id].obj || !m_AudStream[id].inuse)
        return -2;

    printc("Audio%d_Off\r\n", id);
    err = MMPS_AStream_Stop(m_AudStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Audio_Close
//  Description :
//------------------------------------------------------------------------------
int Audio_Close(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_AudStream[id].obj || !m_AudStream[id].inuse)
        return -2;

    /* Never close the seamless stream */
    if (m_AudStream[id].obj->propt.seamless) {
        m_AudStream[id].inuse = MMP_FALSE;
        m_NumSeamlessAudStream--;
        return 0;
    }

    printc("Audio%d_Close\r\n", id);
    err = MMPS_AStream_Close(m_AudStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    if (m_AudStream[id].seamless)
        m_NumSeamlessAudStream--;

    m_AudStream[id].obj = NULL;
    m_AudStream[id].seamless = MMP_FALSE;
    m_AudStream[id].inuse = MMP_FALSE;
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Audio_GetAttribute
//  Description :
//------------------------------------------------------------------------------
int Audio_GetAttribute(MMP_ULONG id, MMPS_ASTREAM_PROPT **propt)
{
    if (!m_AudStream[id].obj || !m_AudStream[id].inuse) {
        *propt = NULL;
        return -2;
    }

    *propt = &m_AudStream[id].obj->propt;
    return 0;
}
#endif //(V4L2_AAC)

#if (V4L2_JPG)
//------------------------------------------------------------------------------
//  Function    : Jpeg_Init
//  Description :
//------------------------------------------------------------------------------
MMP_ULONG Jpeg_Init(MMP_BOOL occupy)
{
    MMP_ULONG id;

    /* If any JPEG stream is opened, skip open */
    for(id = 0; id < MAX_JPG_STREAM_NUM; id++) {
        if (m_JpgStream[id].obj && !m_JpgStream[id].inuse) {
            if (occupy)
                m_JpgStream[id].inuse = MMP_TRUE;
            return id;
        }
    }

    /* No JPEG stream opened, open a new one */
    for(id = 0; id < MAX_JPG_STREAM_NUM; id++) {
        if ((m_JpgStream[id].obj == NULL) && !m_JpgStream[id].inuse) {
            if (occupy)
                m_JpgStream[id].inuse = MMP_TRUE;
            return id;
        }
    }

    printc("No jpeg stream resource\r\n");
    return MAX_JPG_STREAM_NUM;
}

//------------------------------------------------------------------------------
//  Function    : Jpeg_Open
//  Description :
//------------------------------------------------------------------------------
int Jpeg_Open(MMP_ULONG id, MMPS_JSTREAM_PROPT *propt)
{
    if (id >= MAX_JPG_STREAM_NUM)
        return -1;

    /* Stream opened, no need to open again */
    if (m_JpgStream[id].obj)
        return 0;

    /* Initialize JStream Module */
    MMPS_JStream_ModInit();

    /* Initial Sensor */
    Sensor_Init();
	
    printc("Jpeg%d_Open\r\n", id);
    //printc("key_map 0x%x\r\n", key_map);

	//change_effect_mode(ADC_Get_Effect());
    m_JpgStream[id].obj = MMPS_JStream_Open(propt);
    if (!m_JpgStream[id].obj) {
        printc("\t-> failed\r\n");
        m_JpgStream[id].inuse = MMP_FALSE;
        return -2;
    }
    Set_Obj_Pipe(m_JpgStream[id].obj->pipe.ibcpipeID , id, V4L2_PIX_FMT_MJPEG );
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Jpeg_On
//  Description :
//------------------------------------------------------------------------------
int Jpeg_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!m_JpgStream[id].obj || !m_JpgStream[id].inuse)
        return -2;

    printc("Jpeg%d_On\r\n", id);
    err = MMPS_JStream_Start(m_JpgStream[id].obj, v4l2_id, opt);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Jpeg_Off
//  Description :
//------------------------------------------------------------------------------
int Jpeg_Off(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_JpgStream[id].obj || !m_JpgStream[id].inuse)
        return -2;

    printc("Jpeg%d_Off\r\n", id);
    err = MMPS_JStream_Stop(m_JpgStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Jpeg_Close
//  Description :
//------------------------------------------------------------------------------
int Jpeg_Close(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_JpgStream[id].obj || !m_JpgStream[id].inuse)
        return -2;

    printc("Jpeg%d_Close\r\n", id);
    err = MMPS_JStream_Close(m_JpgStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return - 1;
    }

    Set_Obj_Pipe(m_JpgStream[id].obj->pipe.ibcpipeID , -1, 0 );
    m_JpgStream[id].obj = NULL;
    m_JpgStream[id].inuse = MMP_FALSE;
    Sensor_DeInit();
    return 0;
}
#endif //(V4L2_JPG)

#if (V4L2_GRAY)
//------------------------------------------------------------------------------
//  Function    : Gray_Init
//  Description :
//------------------------------------------------------------------------------
MMP_ULONG Gray_Init(MMP_BOOL occupy)
{
    MMP_ULONG id;

    /* Try to find one free gray stream */
    for(id = 0; id < MAX_GRAY_STREAM_NUM; id++) {
        if ((m_GrayStream[id].obj == NULL) && !m_GrayStream[id].inuse) {
            if (occupy)
                m_GrayStream[id].inuse = MMP_TRUE;
            return id;
        }
    }

    printc("No Gray stream resource\r\n");
    return MAX_GRAY_STREAM_NUM;
}

//------------------------------------------------------------------------------
//  Function    : Gray_Open
//  Description :
//------------------------------------------------------------------------------
int Gray_Open(MMP_ULONG id, MMP_ULONG width, MMP_ULONG height,MMP_ULONG fmt)
{
    MMPS_YSTREAM_PROPT propt;

    if (id >= MAX_GRAY_STREAM_NUM)
        return -1;

    /* Initialize YStream Module */
    MMPS_YStream_ModInit();

    /* Initial Sensor */
    Sensor_Init();

    propt.snrID = m_ubMainSensor;
    propt.w     = width;
    propt.h     = height;
    propt.fmt   = fmt ;

    printc("Gray%d_Open\r\n", id);
    m_GrayStream[id].obj = MMPS_YStream_Open(&propt);
    if (!m_GrayStream[id].obj) {
        printc("\t-> failed\r\n");
        m_GrayStream[id].inuse = MMP_FALSE;
        return -2;
    }
    Set_Obj_Pipe(m_GrayStream[id].obj->pipe.ibcpipeID , id, fmt );

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Gray_On
//  Description :
//------------------------------------------------------------------------------
int Gray_On(MMP_ULONG id, MMP_ULONG v4l2_id)
{
    MMP_ERR err;

    if (!m_GrayStream[id].obj || !m_GrayStream[id].inuse)
        return -2;

    printc("Gray%d_On\r\n", id);
    err = MMPS_YStream_Start(m_GrayStream[id].obj, v4l2_id);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Gray_Off
//  Description :
//------------------------------------------------------------------------------
int Gray_Off(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_GrayStream[id].obj || !m_GrayStream[id].inuse)
        return -2;

    printc("Gray%d_Off\r\n", id);
    err = MMPS_YStream_Stop(m_GrayStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : Gray_Close
//  Description :
//------------------------------------------------------------------------------
int Gray_Close(MMP_ULONG id)
{
    MMP_ERR err;

    if (!m_GrayStream[id].obj || !m_GrayStream[id].inuse)
        return -2;

    printc("Gray%d_Close\r\n", id);
    err = MMPS_YStream_Close(m_GrayStream[id].obj);
    if (err) {
        printc("\t-> failed\r\n");
        return - 1;
    }

    Set_Obj_Pipe(m_GrayStream[id].obj->pipe.ibcpipeID , -1 , 0 );
    m_GrayStream[id].obj = NULL;
    m_GrayStream[id].inuse = MMP_FALSE;
    Sensor_DeInit();
    return 0;
}
#endif //(V4L2_GRAY)

int Get_Obj_Pipe(int obj_id,unsigned long fmt)
{
    int pipe_id ;
    for(pipe_id = 0 ; pipe_id < MMP_IBC_PIPE_MAX ; pipe_id++ ) {
        if(PIPE_USAGE_MAP[pipe_id].obj_id!=-1) {
            if ( (PIPE_USAGE_MAP[pipe_id].obj_id == obj_id) && (PIPE_USAGE_MAP[pipe_id].fmt == fmt) ) {
                return pipe_id ;
            }
        }
    } 
    return -1 ;
}

int Set_Obj_Pipe(int pipe_id,int obj_id,unsigned long fmt)
{
    if(pipe_id < MMP_IBC_PIPE_MAX ) {
        PIPE_USAGE_MAP[pipe_id].obj_id = obj_id ;
        PIPE_USAGE_MAP[pipe_id].fmt = fmt ;
    }
    return 0;
}


#endif //(SUPPORT_V4L2)

