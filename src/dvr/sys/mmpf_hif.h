//==============================================================================
//
//  File        : mmpf_hif.h
//  Description : INCLUDE File for the Firmware Host Interface Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_HIF_H_
#define _MMPF_HIF_H_

/** @addtogroup MMPF_HIF
@{
*/
//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_hif_cmd.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

/* Flag for HIF_CMD (SYS_Flag_Hif) */
#define SYS_FLAG_SYS                        		(0x00000001)
#define SYS_FLAG_FS                         		(0x00000002)
#define	SYS_FLAG_USBOP								(0x00000004)
#define SYS_FLAG_MEMDEV_ACK                 		(0x00000008)
#define SYS_FLAG_LTASK	                    		(0x00000010)
#define SYS_FLAG_AGC	                    		(0x00000020)

/* Flag for Sensor/ISP (SENSOR_Flag) */
#define SENSOR_FLAG_VIF_GRAB_END         			(0x00000001)
#define SENSOR_FLAG_VIF_FRM_END          			(0x00000002)
#define SENSOR_FLAG_VIF_FRM_ST           			(0x00000004)
#define SENSOR_FLAG_VIF_LINE_NO          			(0x00000008)

#define SENSOR_FLAG_ISP_FRM_ST              		(0x00000010)
#define SENSOR_FLAG_ISP_FRM_END             		(0x00000020)

#define SENSOR_FLAG_SENSOR_CMD                  	(0x00000200)
#define SENSOR_FLAG_ROTDMA                  		(0x00000400) //For Front Cam only
#define SENSOR_FLAG_FDTC_DRAWRECT          	 		(0x00000800)
#define SENSOR_FLAG_PCAMOP							(0x00001000)
#define SENSOR_FLAG_CHANGE_SENSOR_MODE	    		(0x00002000)
#define SENSOR_FLAG_TRIGGER_DEINTERLACE	    		(0x00004000)
#define SENSOR_FLAG_TRIGGER_GRA_TO_PIPE	    		(0x00008000)
#define SENSOR_FLAG_ROTDMA_REAR                 	(0x00010000) //For Rear Cam only

/* Flag for DSC (DSC_Flag) */
#define DSC_FLAG_DSC_CMD							(0x00000001)
#define DSC_FLAG_TRIGGER_STILL                      (0x00000002)

/* Flag for Video Record (VID_REC_Flag) */
#define CMD_FLAG_VSTREAM                            (0x00000001)
#define CMD_FLAG_VIDRECD							(0x00000002)
#define SYS_FLAG_VIDENC                             (0x00000004)

/* Flag for Audio Record (AUD_REC_Flag) */
#define AUD_FLAG_AUDIOREC                           (0x00000001)
#define AUD_FLAG_GAPLESS_CB                         (0x00000002)
#define AUD_FLAG_IN_READY                           (0x00000004)
#define AUD_FLAG_OUT_READY                          (0x00000008)
#define AUD_FLAG_ENC_ST                             (0x00000100)
    #define AUD_FLAG_ENC_SHIFT      (8)
    #define AUD_FLAG_ENC(id)        (1 << (AUD_FLAG_ENC_SHIFT + (id)))
#define AUD_FLAG_ENC_END                            (0x00008000) // id < 8

/* Flag for Audio Streaming (AUD_STREAM_Flag) */
#define AUD_FLAG_AUDIOWRITE_FILE                    (0x00000001)
#define AUD_FLAG_AUDIOREAD_FILE                     (0x00000002)
#define AUD_FLAG_MP3_GET_INFO                       (0x00000004)
#define AUD_FLAG_AUI_READ_FILE                      (0x00000008)

/* Flag for streamer operation (STREAMER_Flag) */
#define SYS_FLAG_ASTREAM_ST                         (0x00000001)
    #define SYS_FLAG_ASTREAM_SHIFT  (0)
    #define SYS_FLAG_ASTREAM(id)    (1 << (SYS_FLAG_ASTREAM_SHIFT + (id)))
#define SYS_FLAG_ASTREAM_END                        (0x00000008) // id < 4
#define SYS_FLAG_VSTREAM_ST                         (0x00000010)
    #define SYS_FLAG_VSTREAM_SHIFT  (4)
    #define SYS_FLAG_VSTREAM(id)    (1 << (SYS_FLAG_VSTREAM_SHIFT + (id)))
#define SYS_FLAG_VSTREAM_END                        (0x00000080) // id < 4
#define SYS_FLAG_JSTREAM_ST                         (0x00000100)
    #define SYS_FLAG_JSTREAM_SHIFT  (8)
    #define SYS_FLAG_JSTREAM(id)    (1 << (SYS_FLAG_JSTREAM_SHIFT + (id)))
#define SYS_FLAG_JSTREAM_END                        (0x00000800) // id < 4
#define SYS_FLAG_YSTREAM_ST                         (0x00001000)
    #define SYS_FLAG_YSTREAM_SHIFT  (12)
    #define SYS_FLAG_YSTREAM(id)    (1 << (SYS_FLAG_YSTREAM_SHIFT + (id)))
#define SYS_FLAG_YSTREAM_END                        (0x00008000) // id < 4

/* Flag for Motion Detection (MDTC_Flag) */
#define MD_FLAG_RUN                                 (0x00000001)

/* Flag for OSD Start (OSD_Flag) */
#define OSD_FLAG_RUN                                 (0x00000001)
#define OSD_FLAG_STOP                                (0x00000002)
#define OSD_FLAG_OSDRES                              (0x00000004)


/* Flag for IVA (IVA_Flag) */
#define IVA_FLAG_RUN                                 (0x00000001)

/* Flag for IPC UI operation (IPC_UI_Flag) */
#define UI_FLAG_STREAM_ON                           (0x00000001)
#define UI_FLAG_STREAM_OFF                          (0x00000002)

//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================

extern MMP_ULONG    m_ulHifCmd[GRP_IDX_NUM];
extern MMP_ULONG    m_ulHifParam[GRP_IDX_NUM][MAX_HIF_ARRAY_SIZE];

typedef MMP_ULONG   MMPF_OS_FLAGID; //Ref:os_wrap.h

extern MMPF_OS_FLAGID	SYS_Flag_Hif;
extern MMPF_OS_FLAGID	SENSOR_Flag;
extern MMPF_OS_FLAGID	DSC_Flag;
extern MMPF_OS_FLAGID 	AUD_REC_Flag;
extern MMPF_OS_FLAGID	VID_REC_Flag;
extern MMPF_OS_FLAGID	STREAMER_Flag;
extern MMPF_OS_FLAGID	MDTC_Flag;
extern MMPF_OS_FLAGID	DSC_UI_Flag;
extern MMPF_OS_FLAGID   IPC_UI_Flag;
extern MMPF_OS_FLAGID	IVA_Flag;

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum _MMPF_HIF_INT_FLAG
{
    MMPF_HIF_INT_VIDEOR_END     = 0x00000001,
    MMPF_HIF_INT_VIDEOP_END     = 0x00000002,
    MMPF_HIF_INT_AUDIOR_END     = 0x00000004,
    MMPF_HIF_INT_AUDIOP_END     = 0x00000008,
    MMPF_HIF_INT_AUDIOR_GETDATA = 0x00000010,
    MMPF_HIF_INT_AUDIOP_FILLBUF = 0x00000020,
    MMPF_HIF_INT_VIDEOR_MOV     = 0x00000040,
    MMPF_HIF_INT_VIDEOP_MOV     = 0x00000080,
    MMPF_HIF_INT_FDTC_END       = 0x00000100,
    MMPF_HIF_INT_SDCARD_REMOVE  = 0x00000200,
    MMPF_HIF_INT_MDTV_ESG_DONE  = 0x00000400,
    MMPF_HIF_INT_MDTV_BUF_OK    = 0x00000800,
    MMPF_HIF_INT_SBC_EMPTY_BUF  = 0x00001000,
    MMPF_HIF_INT_WAV_EMPTY_BUF  = 0x00002000,
    MMPF_HIF_INT_USB_SUSPEND    = 0x00004000,
    MMPF_HIF_INT_USB_HOST_DEV   = 0x00008000,
    MMPF_HIF_INT_SMILE_SHOT     = 0x00010000
} MMPF_HIF_INT_FLAG;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void MMPF_HIF_ResetCmdStatus(void);
void MMPF_HIF_SetCmdStatus(MMP_ULONG status);
void MMPF_HIF_ClearCmdStatus(MMP_ULONG status);
MMP_ULONG MMPF_HIF_GetCmdStatus(MMP_ULONG status);

void MMPF_HIF_SetCpu2HostInt(MMPF_HIF_INT_FLAG status);

void MMPF_HIF_FeedbackParamL(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_ULONG ulParamdata);
void MMPF_HIF_FeedbackParamW(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_USHORT usParamdata);
void MMPF_HIF_FeedbackParamB(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_UBYTE ubParamdata);

#endif // _MMPF_HIF_H_
