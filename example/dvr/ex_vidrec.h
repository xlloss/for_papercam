/**
 @file ex_vidrec.h
 @brief Header file for video record related sample code
 @author Alterman
 @version 1.0
*/
#ifndef _EX_VIDRECD_H_
#define _EX_VIDRECD_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "mmps_astream.h"
#include "mmps_vstream.h"
#include "mmps_jstream.h"
#include "mmps_ystream.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================
#define max_width 1920
#define max_height 1080
#define Y_BLACK 0
#define U_BLACK 128
#define Y_GREEN 76
#define U_GREEN 84
#define RTC_str_len 19
#define String_str_len 30
#define STREAM_SELECT_0 0
#define STREAM_SELECT_1 1
//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
#if (SUPPORT_ALSA)&&(ALSA_PWRON_PLAY)
void        Alsa_Open(void);
void        Alsa_Close(void);
#endif
#if (SUPPORT_MDTC)
void        Mdtc_Open(void);
void        Mdtc_Close(void);
#endif
#if (SUPPORT_IVA)
void        IVA_Open(void);
void        IVA_Close(void);
#endif

void OSD_Run(void);
void OSD_Stop(void);

#if (V4L2_H264)
void        Video_Stream(void);
MMP_ULONG   Video_Init(MMP_BOOL occupy, MMP_BOOL seamless);
int         Video_Open(MMP_ULONG id, MMPS_VSTREAM_PROPT *propt, MMP_ULONG streamset);
int         Video_Close(MMP_ULONG id);
int         Video_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt);
int         Video_Off(MMP_ULONG id);
int         Video_Control(  MMP_ULONG       id,
                            IPC_VIDEO_CTL   ctrl,
                            VIDENC_CTL_SET  *param);
int         Video_GetAttribute(MMP_ULONG id, MMPS_VSTREAM_PROPT **propt);
MMPS_VSTREAM_CLASS *Video_Obj(MMP_ULONG id) ;

#endif
#if (V4L2_AAC)
void        Audio_Stream(void);
MMP_ULONG   Audio_Init(MMP_BOOL occupy, MMP_BOOL seamless);
int         Audio_Open(MMP_ULONG id, MMPS_ASTREAM_PROPT *cap);
int         Audio_Close(MMP_ULONG id);
int         Audio_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt);
int         Audio_Off(MMP_ULONG id);
int         Audio_GetAttribute(MMP_ULONG id, MMPS_ASTREAM_PROPT **propt);
#endif
#if (V4L2_JPG)
void        Jpeg_Stream(void);
MMP_ULONG   Jpeg_Init(MMP_BOOL occupy);
int         Jpeg_Open(MMP_ULONG id, MMPS_JSTREAM_PROPT *propt);
int         Jpeg_Close(MMP_ULONG id);
int         Jpeg_On(MMP_ULONG id, MMP_ULONG v4l2_id, IPC_STREAMER_OPTION opt);
int         Jpeg_Off(MMP_ULONG id);
#endif
#if (V4L2_GRAY)
MMP_ULONG   Gray_Init(MMP_BOOL occupy);
int         Gray_Open(MMP_ULONG id, MMP_ULONG width, MMP_ULONG height,MMP_ULONG fmt);
int         Gray_Close(MMP_ULONG id);
int         Gray_On(MMP_ULONG id, MMP_ULONG v4l2_id);
int         Gray_Off(MMP_ULONG id);
#endif
int          Get_Obj_Pipe(int obj_id,unsigned long fmt);
int          Set_Obj_Pipe(int pipe_id,int obj_id,unsigned long fmt);

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/*
 * Video Stream Resource
 */
typedef struct {
    MMPS_VSTREAM_CLASS  *obj;
    MMP_BOOL            inuse;
    MMP_BOOL            seamless;
} VSTREAM_RESOURCE;

/*
 * Audio Stream Resource
 */
typedef struct {
    MMPS_ASTREAM_CLASS  *obj;
    MMP_BOOL            inuse;
    MMP_BOOL            seamless;
} ASTREAM_RESOURCE;

/*
 * Jpeg Stream Resource
 */
typedef struct {
    MMPS_JSTREAM_CLASS  *obj;
    MMP_BOOL            inuse;
} JSTREAM_RESOURCE;

/*
 * Gray Stream Resource
 */
typedef struct {
    MMPS_YSTREAM_CLASS  *obj;
    MMP_BOOL            inuse;
} YSTREAM_RESOURCE;

typedef struct {
    int obj_id ;
    unsigned long fmt;
} PIPE_RESOURCE;

#endif //_EX_VIDRECD_H_
