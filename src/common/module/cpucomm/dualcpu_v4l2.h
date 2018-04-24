#ifndef DUALCPU_V4L2_H
#define DUALCPU_V4L2_H

#include "mmp_lib.h"
#include "mmpf_vidcmn.h"
#include "md.h"
//==============================================================================
//
//                              MACRO FUNCTION
//
//==============================================================================

#define CPB_SIZE_2_LB_SIZE(cpb, kbps)   (((cpb) << 4) / (kbps))

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

/*  Values for ctrl_class field */
#define V4L2_CTRL_CLASS_USER    0x00980000	/* Old-style 'user' controls */
#define V4L2_CTRL_CLASS_MPEG    0x00990000	/* MPEG-compression controls */
#define V4L2_CTRL_CLASS_CAMERA  0x009a0000	/* Camera class controls */
#define V4L2_CTRL_CLASS_FM_TX   0x009b0000	/* FM Modulator control class */
#define V4L2_CTRL_CLASS_FLASH   0x009c0000	/* Camera flash controls */
#define V4L2_CTRL_CLASS_JPEG    0x009d0000  /* JPEG-compression controls */
#define V4L2_CTRL_CLASS_DETECT  0x00a30000  /* Detection controls */

#define V4L2_CID_BASE			(V4L2_CTRL_CLASS_USER | 0x900)
#define V4L2_CID_USER_BASE 		(V4L2_CID_BASE)
/*  IDs reserved for driver specific controls */
#define V4L2_CID_PRIVATE_BASE   (0x08000000)
#define V4L2_CID_USER_CLASS     (V4L2_CTRL_CLASS_USER | 1)
#define V4L2_CID_BRIGHTNESS     (V4L2_CID_BASE+0)
#define V4L2_CID_CONTRAST       (V4L2_CID_BASE+1)
#define V4L2_CID_SATURATION     (V4L2_CID_BASE+2)
#define V4L2_CID_HUE            (V4L2_CID_BASE+3)
#define V4L2_CID_AUDIO_VOLUME   (V4L2_CID_BASE+5)
#define V4L2_CID_AUDIO_BALANCE  (V4L2_CID_BASE+6)
#define V4L2_CID_AUDIO_BASS		(V4L2_CID_BASE+7)
#define V4L2_CID_AUDIO_TREBLE   (V4L2_CID_BASE+8)
#define V4L2_CID_AUDIO_MUTE     (V4L2_CID_BASE+9)
#define V4L2_CID_AUDIO_LOUDNESS (V4L2_CID_BASE+10)
#define V4L2_CID_BLACK_LEVEL    (V4L2_CID_BASE+11) /* Deprecated */
#define V4L2_CID_AUTO_WHITE_BALANCE (V4L2_CID_BASE+12)
#define V4L2_CID_DO_WHITE_BALANCE   (V4L2_CID_BASE+13)
#define V4L2_CID_RED_BALANCE    (V4L2_CID_BASE+14)
#define V4L2_CID_BLUE_BALANCE   (V4L2_CID_BASE+15)
#define V4L2_CID_GAMMA          (V4L2_CID_BASE+16)
#define V4L2_CID_WHITENESS      (V4L2_CID_GAMMA) /* Deprecated */
#define V4L2_CID_EXPOSURE       (V4L2_CID_BASE+17)
#define V4L2_CID_AUTOGAIN       (V4L2_CID_BASE+18)
#define V4L2_CID_GAIN           (V4L2_CID_BASE+19)
#define V4L2_CID_HFLIP          (V4L2_CID_BASE+20)
#define V4L2_CID_VFLIP          (V4L2_CID_BASE+21)

/* Deprecated; use V4L2_CID_PAN_RESET and V4L2_CID_TILT_RESET */
#define V4L2_CID_HCENTER        (V4L2_CID_BASE+22)
#define V4L2_CID_VCENTER        (V4L2_CID_BASE+23)

#define V4L2_CID_POWER_LINE_FREQUENCY   (V4L2_CID_BASE+24)
enum v4l2_power_line_frequency {
	V4L2_CID_POWER_LINE_FREQUENCY_DISABLED	= 0,
	V4L2_CID_POWER_LINE_FREQUENCY_50HZ	= 1,
	V4L2_CID_POWER_LINE_FREQUENCY_60HZ	= 2,
	V4L2_CID_POWER_LINE_FREQUENCY_AUTO	= 3
};

#define V4L2_CID_HUE_AUTO       (V4L2_CID_BASE+25)
#define V4L2_CID_WHITE_BALANCE_TEMPERATURE  (V4L2_CID_BASE+26)
#define V4L2_CID_SHARPNESS      (V4L2_CID_BASE+27)
#define V4L2_CID_BACKLIGHT_COMPENSATION     (V4L2_CID_BASE+28)
#define V4L2_CID_CHROMA_AGC     (V4L2_CID_BASE+29)
#define V4L2_CID_COLOR_KILLER   (V4L2_CID_BASE+30)
#define V4L2_CID_COLORFX        (V4L2_CID_BASE+31)

#define V4L2_CID_AUTOBRIGHTNESS (V4L2_CID_BASE+32)
#define V4L2_CID_BAND_STOP_FILTER   (V4L2_CID_BASE+33)

#define V4L2_CID_ROTATE         (V4L2_CID_BASE+34)
#define V4L2_CID_BG_COLOR       (V4L2_CID_BASE+35)

#define V4L2_CID_CHROMA_GAIN    (V4L2_CID_BASE+36)

#define V4L2_CID_ILLUMINATORS_1 (V4L2_CID_BASE+37)
#define V4L2_CID_ILLUMINATORS_2 (V4L2_CID_BASE+38)

#define V4L2_CID_MIN_BUFFERS_FOR_CAPTURE    (V4L2_CID_BASE+39)
#define V4L2_CID_MIN_BUFFERS_FOR_OUTPUT     (V4L2_CID_BASE+40)

/* last CID + 1 */
#define V4L2_CID_LASTP1         (V4L2_CID_BASE+41)

/* Minimum number of buffer needed by the device */

/*  MPEG-class control IDs defined by V4L2 */
#define V4L2_CID_MPEG_BASE      (V4L2_CTRL_CLASS_MPEG | 0x900)
#define V4L2_CID_MPEG_CLASS     (V4L2_CTRL_CLASS_MPEG | 1)

/*  MPEG streams, specific to multiplexed streams */
#define V4L2_CID_MPEG_STREAM_TYPE   (V4L2_CID_MPEG_BASE+0)
enum v4l2_mpeg_stream_type {
	V4L2_MPEG_STREAM_TYPE_MPEG2_PS   = 0, /* MPEG-2 program stream */
	V4L2_MPEG_STREAM_TYPE_MPEG2_TS   = 1, /* MPEG-2 transport stream */
	V4L2_MPEG_STREAM_TYPE_MPEG1_SS   = 2, /* MPEG-1 system stream */
	V4L2_MPEG_STREAM_TYPE_MPEG2_DVD  = 3, /* MPEG-2 DVD-compatible stream */
	V4L2_MPEG_STREAM_TYPE_MPEG1_VCD  = 4, /* MPEG-1 VCD-compatible stream */
	V4L2_MPEG_STREAM_TYPE_MPEG2_SVCD = 5  /* MPEG-2 SVCD-compatible stream */
};

#define V4L2_CID_MPEG_STREAM_PID_PMT        (V4L2_CID_MPEG_BASE+1)
#define V4L2_CID_MPEG_STREAM_PID_AUDIO      (V4L2_CID_MPEG_BASE+2)
#define V4L2_CID_MPEG_STREAM_PID_VIDEO      (V4L2_CID_MPEG_BASE+3)
#define V4L2_CID_MPEG_STREAM_PID_PCR        (V4L2_CID_MPEG_BASE+4)
#define V4L2_CID_MPEG_STREAM_PES_ID_AUDIO   (V4L2_CID_MPEG_BASE+5)
#define V4L2_CID_MPEG_STREAM_PES_ID_VIDEO   (V4L2_CID_MPEG_BASE+6)
#define V4L2_CID_MPEG_STREAM_VBI_FMT        (V4L2_CID_MPEG_BASE+7)
enum v4l2_mpeg_stream_vbi_fmt {
	V4L2_MPEG_STREAM_VBI_FMT_NONE = 0,  /* No VBI in the MPEG stream */
	V4L2_MPEG_STREAM_VBI_FMT_IVTV = 1   /* VBI in private packets, IVTV format */
};

/*  MPEG audio controls specific to multiplexed streams  */
#define V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ   (V4L2_CID_MPEG_BASE+100)
#define V4L2_CID_MPEG_AUDIO_MODE            (V4L2_CID_MPEG_BASE+105)
enum v4l2_mpeg_audio_mode {
	V4L2_MPEG_AUDIO_MODE_STEREO       = 0,
	V4L2_MPEG_AUDIO_MODE_JOINT_STEREO = 1,
	V4L2_MPEG_AUDIO_MODE_DUAL         = 2,
	V4L2_MPEG_AUDIO_MODE_MONO         = 3
};

#define V4L2_CID_MPEG_AUDIO_MUTE            (V4L2_CID_MPEG_BASE+109)
#define V4L2_CID_MPEG_AUDIO_AAC_BITRATE     (V4L2_CID_MPEG_BASE+110)
#define V4L2_CID_MPEG_AUDIO_AC3_BITRATE     (V4L2_CID_MPEG_BASE+111)
enum v4l2_mpeg_audio_ac3_bitrate {
	V4L2_MPEG_AUDIO_AC3_BITRATE_32K  = 0,
	V4L2_MPEG_AUDIO_AC3_BITRATE_40K  = 1,
	V4L2_MPEG_AUDIO_AC3_BITRATE_48K  = 2,
	V4L2_MPEG_AUDIO_AC3_BITRATE_56K  = 3,
	V4L2_MPEG_AUDIO_AC3_BITRATE_64K  = 4,
	V4L2_MPEG_AUDIO_AC3_BITRATE_80K  = 5,
	V4L2_MPEG_AUDIO_AC3_BITRATE_96K  = 6,
	V4L2_MPEG_AUDIO_AC3_BITRATE_112K = 7,
	V4L2_MPEG_AUDIO_AC3_BITRATE_128K = 8,
	V4L2_MPEG_AUDIO_AC3_BITRATE_160K = 9,
	V4L2_MPEG_AUDIO_AC3_BITRATE_192K = 10,
	V4L2_MPEG_AUDIO_AC3_BITRATE_224K = 11,
	V4L2_MPEG_AUDIO_AC3_BITRATE_256K = 12,
	V4L2_MPEG_AUDIO_AC3_BITRATE_320K = 13,
	V4L2_MPEG_AUDIO_AC3_BITRATE_384K = 14,
	V4L2_MPEG_AUDIO_AC3_BITRATE_448K = 15,
	V4L2_MPEG_AUDIO_AC3_BITRATE_512K = 16,
	V4L2_MPEG_AUDIO_AC3_BITRATE_576K = 17,
	V4L2_MPEG_AUDIO_AC3_BITRATE_640K = 18
};

#define V4L2_CID_MPEG_VIDEO_B_FRAMES        (V4L2_CID_MPEG_BASE+202)
#define V4L2_CID_MPEG_VIDEO_GOP_SIZE        (V4L2_CID_MPEG_BASE+203)
#define V4L2_CID_MPEG_VIDEO_GOP_CLOSURE     (V4L2_CID_MPEG_BASE+204)
#define V4L2_CID_MPEG_VIDEO_PULLDOWN        (V4L2_CID_MPEG_BASE+205)
#define V4L2_CID_MPEG_VIDEO_BITRATE_MODE    (V4L2_CID_MPEG_BASE+206)
enum v4l2_mpeg_video_bitrate_mode {
	V4L2_MPEG_VIDEO_BITRATE_MODE_VBR = 0,
	V4L2_MPEG_VIDEO_BITRATE_MODE_CBR = 1
};

#define V4L2_CID_MPEG_VIDEO_BITRATE         (V4L2_CID_MPEG_BASE+207)
#define V4L2_CID_MPEG_VIDEO_BITRATE_PEAK    (V4L2_CID_MPEG_BASE+208)
#define V4L2_CID_MPEG_VIDEO_TEMPORAL_DECIMATION (V4L2_CID_MPEG_BASE+209)
#define V4L2_CID_MPEG_VIDEO_MUTE            (V4L2_CID_MPEG_BASE+210)
#define V4L2_CID_MPEG_VIDEO_MUTE_YUV        (V4L2_CID_MPEG_BASE+211)
#define V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE     (V4L2_CID_MPEG_BASE+212)
#define V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER (V4L2_CID_MPEG_BASE+213)
#define V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB		(V4L2_CID_MPEG_BASE+214)
#define V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE (V4L2_CID_MPEG_BASE+215)
#define V4L2_CID_MPEG_VIDEO_HEADER_MODE	    (V4L2_CID_MPEG_BASE+216)
enum v4l2_mpeg_video_header_mode {
	V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE = 0,
	V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME = 1
};

#define V4L2_CID_MPEG_VIDEO_MAX_REF_PIC	    (V4L2_CID_MPEG_BASE+217)
#define V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE    (V4L2_CID_MPEG_BASE+218)
#define V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES   (V4L2_CID_MPEG_BASE+219)
#define V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB      (V4L2_CID_MPEG_BASE+220)
#define V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE        (V4L2_CID_MPEG_BASE+221)
enum v4l2_mpeg_video_multi_slice_mode {
	V4L2_MPEG_VIDEO_MULTI_SLICE_MODE_SINGLE		= 0,
	V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_MB		= 1,
	V4L2_MPEG_VIDEO_MULTI_SICE_MODE_MAX_BYTES	= 2
};

#define V4L2_CID_MPEG_VIDEO_VBV_SIZE        (V4L2_CID_MPEG_BASE+222)
#define V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP (V4L2_CID_MPEG_BASE+300)
#define V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP (V4L2_CID_MPEG_BASE+301)
#define V4L2_CID_MPEG_VIDEO_H263_B_FRAME_QP (V4L2_CID_MPEG_BASE+302)
#define V4L2_CID_MPEG_VIDEO_H263_MIN_QP     (V4L2_CID_MPEG_BASE+303)
#define V4L2_CID_MPEG_VIDEO_H263_MAX_QP     (V4L2_CID_MPEG_BASE+304)
#define V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP (V4L2_CID_MPEG_BASE+350)
#define V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP (V4L2_CID_MPEG_BASE+351)
#define V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP (V4L2_CID_MPEG_BASE+352)
#define V4L2_CID_MPEG_VIDEO_H264_MIN_QP     (V4L2_CID_MPEG_BASE+353)
#define V4L2_CID_MPEG_VIDEO_H264_MAX_QP     (V4L2_CID_MPEG_BASE+354)
#define V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM  (V4L2_CID_MPEG_BASE+355)
#define V4L2_CID_MPEG_VIDEO_H264_CPB_SIZE       (V4L2_CID_MPEG_BASE+356)
#define V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE   (V4L2_CID_MPEG_BASE+357)
enum v4l2_mpeg_video_h264_entropy_mode {
	V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CAVLC	= 0,
	V4L2_MPEG_VIDEO_H264_ENTROPY_MODE_CABAC	= 1
};

#define V4L2_CID_MPEG_VIDEO_H264_I_PERIOD   (V4L2_CID_MPEG_BASE+358)
#define V4L2_CID_MPEG_VIDEO_H264_LEVEL      (V4L2_CID_MPEG_BASE+359)
enum v4l2_mpeg_video_h264_level {
	V4L2_MPEG_VIDEO_H264_LEVEL_1_0	= 0,
	V4L2_MPEG_VIDEO_H264_LEVEL_1B	= 1,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_1	= 2,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_2	= 3,
	V4L2_MPEG_VIDEO_H264_LEVEL_1_3	= 4,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_0	= 5,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_1	= 6,
	V4L2_MPEG_VIDEO_H264_LEVEL_2_2	= 7,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_0	= 8,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_1	= 9,
	V4L2_MPEG_VIDEO_H264_LEVEL_3_2	= 10,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_0	= 11,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_1	= 12,
	V4L2_MPEG_VIDEO_H264_LEVEL_4_2	= 13,
	V4L2_MPEG_VIDEO_H264_LEVEL_5_0	= 14,
	V4L2_MPEG_VIDEO_H264_LEVEL_5_1	= 15
};

#define V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA  (V4L2_CID_MPEG_BASE+360)
#define V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA   (V4L2_CID_MPEG_BASE+361)
#define V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE   (V4L2_CID_MPEG_BASE+362)
enum v4l2_mpeg_video_h264_loop_filter_mode {
	V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_ENABLED   = 0,
	V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED  = 1,
	V4L2_MPEG_VIDEO_H264_LOOP_FILTER_MODE_DISABLED_AT_SLICE_BOUNDARY = 2
};

#define V4L2_CID_MPEG_VIDEO_H264_PROFILE    (V4L2_CID_MPEG_BASE+363)
enum v4l2_mpeg_video_h264_profile {
	V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE               = 0,
	V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE   = 1,
	V4L2_MPEG_VIDEO_H264_PROFILE_MAIN                   = 2,
	V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED               = 3,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH                   = 4,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10                = 5,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422               = 6,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE    = 7,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10_INTRA          = 8,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422_INTRA         = 9,
	V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_INTRA         = 10,
	V4L2_MPEG_VIDEO_H264_PROFILE_CAVLC_444_INTRA        = 11,
	V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_BASELINE      = 12,
	V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH	        = 13,
	V4L2_MPEG_VIDEO_H264_PROFILE_SCALABLE_HIGH_INTRA    = 14,
	V4L2_MPEG_VIDEO_H264_PROFILE_STEREO_HIGH            = 15,
	V4L2_MPEG_VIDEO_H264_PROFILE_MULTIVIEW_HIGH	        = 16
};

#define V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT (V4L2_CID_MPEG_BASE+364)
#define V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH  (V4L2_CID_MPEG_BASE+365)
#define V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE     (V4L2_CID_MPEG_BASE+366)
#define V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC        (V4L2_CID_MPEG_BASE+367)
enum v4l2_mpeg_video_h264_vui_sar_idc {
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED    = 0,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_1x1		= 1,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_12x11		= 2,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_10x11		= 3,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_16x11		= 4,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_40x33		= 5,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_24x11		= 6,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_20x11		= 7,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_32x11		= 8,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_80x33		= 9,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_18x11		= 10,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_15x11		= 11,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_64x33		= 12,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_160x99		= 13,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_4x3		= 14,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_3x2		= 15,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_2x1		= 16,
	V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_EXTENDED	= 17
};

#define V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP    (V4L2_CID_MPEG_BASE+400)
#define V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP    (V4L2_CID_MPEG_BASE+401)
#define V4L2_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP    (V4L2_CID_MPEG_BASE+402)
#define V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP    (V4L2_CID_MPEG_BASE+403)
#define V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP    (V4L2_CID_MPEG_BASE+404)
#define V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL     (V4L2_CID_MPEG_BASE+405)
enum v4l2_mpeg_video_mpeg4_level {
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_0	= 0,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_0B	= 1,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_1	= 2,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_2	= 3,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_3	= 4,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_3B	= 5,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_4	= 6,
	V4L2_MPEG_VIDEO_MPEG4_LEVEL_5	= 7
};

#define V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE   (V4L2_CID_MPEG_BASE+406)
enum v4l2_mpeg_video_mpeg4_profile {
	V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE            = 0,
	V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_SIMPLE   = 1,
	V4L2_MPEG_VIDEO_MPEG4_PROFILE_CORE              = 2,
	V4L2_MPEG_VIDEO_MPEG4_PROFILE_SIMPLE_SCALABLE   = 3,
	V4L2_MPEG_VIDEO_MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY = 4
};

#define V4L2_CID_MPEG_VIDEO_MPEG4_QPEL      (V4L2_CID_MPEG_BASE+407)

/*  MPEG-class control IDs specific to the CX2341x driver as defined by V4L2 */
#define V4L2_CID_MPEG_CX2341X_BASE          (V4L2_CTRL_CLASS_MPEG | 0x1000)
#define V4L2_CID_MPEG_CX2341X_VIDEO_SPATIAL_FILTER_MODE (V4L2_CID_MPEG_CX2341X_BASE+0)
enum v4l2_mpeg_cx2341x_video_spatial_filter_mode {
	V4L2_MPEG_CX2341X_VIDEO_SPATIAL_FILTER_MODE_MANUAL = 0,
	V4L2_MPEG_CX2341X_VIDEO_SPATIAL_FILTER_MODE_AUTO   = 1
};

#define V4L2_CID_MPEG_CX2341X_VIDEO_SPATIAL_FILTER              (V4L2_CID_MPEG_CX2341X_BASE+1)
#define V4L2_CID_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE    (V4L2_CID_MPEG_CX2341X_BASE+2)
enum v4l2_mpeg_cx2341x_video_luma_spatial_filter_type {
	V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_OFF                  = 0,
	V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_1D_HOR               = 1,
	V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_1D_VERT              = 2,
	V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_2D_HV_SEPARABLE      = 3,
	V4L2_MPEG_CX2341X_VIDEO_LUMA_SPATIAL_FILTER_TYPE_2D_SYM_NON_SEPARABLE = 4
};

#define V4L2_CID_MPEG_CX2341X_VIDEO_CHROMA_SPATIAL_FILTER_TYPE  (V4L2_CID_MPEG_CX2341X_BASE+3)
enum v4l2_mpeg_cx2341x_video_chroma_spatial_filter_type {
	V4L2_MPEG_CX2341X_VIDEO_CHROMA_SPATIAL_FILTER_TYPE_OFF    = 0,
	V4L2_MPEG_CX2341X_VIDEO_CHROMA_SPATIAL_FILTER_TYPE_1D_HOR = 1
};

#define V4L2_CID_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER_MODE    (V4L2_CID_MPEG_CX2341X_BASE+4)
enum v4l2_mpeg_cx2341x_video_temporal_filter_mode {
	V4L2_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER_MODE_MANUAL = 0,
	V4L2_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER_MODE_AUTO   = 1
};

#define V4L2_CID_MPEG_CX2341X_VIDEO_TEMPORAL_FILTER         (V4L2_CID_MPEG_CX2341X_BASE+5)
#define V4L2_CID_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE      (V4L2_CID_MPEG_CX2341X_BASE+6)
enum v4l2_mpeg_cx2341x_video_median_filter_type {
	V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_OFF      = 0,
	V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_HOR      = 1,
	V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_VERT     = 2,
	V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_HOR_VERT = 3,
	V4L2_MPEG_CX2341X_VIDEO_MEDIAN_FILTER_TYPE_DIAG     = 4
};

#define V4L2_CID_MPEG_CX2341X_VIDEO_LUMA_MEDIAN_FILTER_BOTTOM   (V4L2_CID_MPEG_CX2341X_BASE+7)
#define V4L2_CID_MPEG_CX2341X_VIDEO_LUMA_MEDIAN_FILTER_TOP      (V4L2_CID_MPEG_CX2341X_BASE+8)
#define V4L2_CID_MPEG_CX2341X_VIDEO_CHROMA_MEDIAN_FILTER_BOTTOM (V4L2_CID_MPEG_CX2341X_BASE+9)
#define V4L2_CID_MPEG_CX2341X_VIDEO_CHROMA_MEDIAN_FILTER_TOP    (V4L2_CID_MPEG_CX2341X_BASE+10)
#define V4L2_CID_MPEG_CX2341X_STREAM_INSERT_NAV_PACKETS         (V4L2_CID_MPEG_CX2341X_BASE+11)

/*  MPEG-class control IDs specific to the Samsung MFC 5.1 driver as defined by V4L2 */
#define V4L2_CID_MPEG_MFC51_BASE    (V4L2_CTRL_CLASS_MPEG | 0x1100)

#define V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY        (V4L2_CID_MPEG_MFC51_BASE+0)
#define V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY_ENABLE (V4L2_CID_MPEG_MFC51_BASE+1)
#define V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE   (V4L2_CID_MPEG_MFC51_BASE+2)
enum v4l2_mpeg_mfc51_video_frame_skip_mode {
	V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED		= 0,
	V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT	= 1,
	V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT		= 2
};

#define V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE  (V4L2_CID_MPEG_MFC51_BASE+3)
enum v4l2_mpeg_mfc51_video_force_frame_type {
	V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_DISABLED		= 0,
	V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_I_FRAME		= 1,
	V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_NOT_CODED	= 2
};

#define V4L2_CID_MPEG_MFC51_VIDEO_PADDING                   (V4L2_CID_MPEG_MFC51_BASE+4)
#define V4L2_CID_MPEG_MFC51_VIDEO_PADDING_YUV               (V4L2_CID_MPEG_MFC51_BASE+5)
#define V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT       (V4L2_CID_MPEG_MFC51_BASE+6)
#define V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF         (V4L2_CID_MPEG_MFC51_BASE+7)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_ACTIVITY (V4L2_CID_MPEG_MFC51_BASE+50)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_DARK     (V4L2_CID_MPEG_MFC51_BASE+51)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_SMOOTH   (V4L2_CID_MPEG_MFC51_BASE+52)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_STATIC   (V4L2_CID_MPEG_MFC51_BASE+53)
#define V4L2_CID_MPEG_MFC51_VIDEO_H264_NUM_REF_PIC_FOR_P    (V4L2_CID_MPEG_MFC51_BASE+54)

/*  Camera class control IDs */
#define V4L2_CID_CAMERA_CLASS_BASE  (V4L2_CTRL_CLASS_CAMERA | 0x900)
#define V4L2_CID_CAMERA_CLASS       (V4L2_CTRL_CLASS_CAMERA | 1)

#define V4L2_CID_EXPOSURE_AUTO      (V4L2_CID_CAMERA_CLASS_BASE+1)
enum v4l2_exposure_auto_type {
	V4L2_EXPOSURE_AUTO              = 0,
	V4L2_EXPOSURE_MANUAL            = 1,
	V4L2_EXPOSURE_SHUTTER_PRIORITY  = 2,
	V4L2_EXPOSURE_APERTURE_PRIORITY = 3
};

#define V4L2_CID_EXPOSURE_ABSOLUTE  (V4L2_CID_CAMERA_CLASS_BASE+2)
#define V4L2_CID_EXPOSURE_AUTO_PRIORITY (V4L2_CID_CAMERA_CLASS_BASE+3)

#define V4L2_CID_PAN_RELATIVE       (V4L2_CID_CAMERA_CLASS_BASE+4)
#define V4L2_CID_TILT_RELATIVE      (V4L2_CID_CAMERA_CLASS_BASE+5)
#define V4L2_CID_PAN_RESET          (V4L2_CID_CAMERA_CLASS_BASE+6)
#define V4L2_CID_TILT_RESET         (V4L2_CID_CAMERA_CLASS_BASE+7)

#define V4L2_CID_PAN_ABSOLUTE       (V4L2_CID_CAMERA_CLASS_BASE+8)
#define V4L2_CID_TILT_ABSOLUTE      (V4L2_CID_CAMERA_CLASS_BASE+9)

#define V4L2_CID_FOCUS_ABSOLUTE     (V4L2_CID_CAMERA_CLASS_BASE+10)
#define V4L2_CID_FOCUS_RELATIVE	    (V4L2_CID_CAMERA_CLASS_BASE+11)
#define V4L2_CID_FOCUS_AUTO	        (V4L2_CID_CAMERA_CLASS_BASE+12)

#define V4L2_CID_ZOOM_ABSOLUTE      (V4L2_CID_CAMERA_CLASS_BASE+13)
#define V4L2_CID_ZOOM_RELATIVE      (V4L2_CID_CAMERA_CLASS_BASE+14)
#define V4L2_CID_ZOOM_CONTINUOUS    (V4L2_CID_CAMERA_CLASS_BASE+15)

#define V4L2_CID_PRIVACY            (V4L2_CID_CAMERA_CLASS_BASE+16)

#define V4L2_CID_IRIS_ABSOLUTE      (V4L2_CID_CAMERA_CLASS_BASE+17)
#define V4L2_CID_IRIS_RELATIVE      (V4L2_CID_CAMERA_CLASS_BASE+18)

/* FM Modulator class control IDs */
#define V4L2_CID_FM_TX_CLASS_BASE   (V4L2_CTRL_CLASS_FM_TX | 0x900)
#define V4L2_CID_FM_TX_CLASS        (V4L2_CTRL_CLASS_FM_TX | 1)

#define V4L2_CID_RDS_TX_DEVIATION   (V4L2_CID_FM_TX_CLASS_BASE + 1)
#define V4L2_CID_RDS_TX_PI          (V4L2_CID_FM_TX_CLASS_BASE + 2)
#define V4L2_CID_RDS_TX_PTY         (V4L2_CID_FM_TX_CLASS_BASE + 3)
#define V4L2_CID_RDS_TX_PS_NAME     (V4L2_CID_FM_TX_CLASS_BASE + 5)
#define V4L2_CID_RDS_TX_RADIO_TEXT  (V4L2_CID_FM_TX_CLASS_BASE + 6)

#define V4L2_CID_AUDIO_LIMITER_ENABLED      (V4L2_CID_FM_TX_CLASS_BASE + 64)
#define V4L2_CID_AUDIO_LIMITER_RELEASE_TIME (V4L2_CID_FM_TX_CLASS_BASE + 65)
#define V4L2_CID_AUDIO_LIMITER_DEVIATION    (V4L2_CID_FM_TX_CLASS_BASE + 66)

#define V4L2_CID_AUDIO_COMPRESSION_ENABLED  (V4L2_CID_FM_TX_CLASS_BASE + 80)
#define V4L2_CID_AUDIO_COMPRESSION_GAIN     (V4L2_CID_FM_TX_CLASS_BASE + 81)
#define V4L2_CID_AUDIO_COMPRESSION_THRESHOLD    (V4L2_CID_FM_TX_CLASS_BASE + 82)
#define V4L2_CID_AUDIO_COMPRESSION_ATTACK_TIME  (V4L2_CID_FM_TX_CLASS_BASE + 83)
#define V4L2_CID_AUDIO_COMPRESSION_RELEASE_TIME (V4L2_CID_FM_TX_CLASS_BASE + 84)

#define V4L2_CID_PILOT_TONE_ENABLED     (V4L2_CID_FM_TX_CLASS_BASE + 96)
#define V4L2_CID_PILOT_TONE_DEVIATION   (V4L2_CID_FM_TX_CLASS_BASE + 97)
#define V4L2_CID_PILOT_TONE_FREQUENCY   (V4L2_CID_FM_TX_CLASS_BASE + 98)

#define V4L2_CID_TUNE_PREEMPHASIS       (V4L2_CID_FM_TX_CLASS_BASE + 112)
enum v4l2_preemphasis {
	V4L2_PREEMPHASIS_DISABLED	= 0,
	V4L2_PREEMPHASIS_50_uS		= 1,
	V4L2_PREEMPHASIS_75_uS		= 2
};

#define V4L2_CID_TUNE_POWER_LEVEL       (V4L2_CID_FM_TX_CLASS_BASE + 113)
#define V4L2_CID_TUNE_ANTENNA_CAPACITOR (V4L2_CID_FM_TX_CLASS_BASE + 114)

/* Flash and privacy (indicator) light controls */
#define V4L2_CID_FLASH_CLASS_BASE       (V4L2_CTRL_CLASS_FLASH | 0x900)
#define V4L2_CID_FLASH_CLASS            (V4L2_CTRL_CLASS_FLASH | 1)

#define V4L2_CID_FLASH_LED_MODE         (V4L2_CID_FLASH_CLASS_BASE + 1)
enum v4l2_flash_led_mode {
	V4L2_FLASH_LED_MODE_NONE,
	V4L2_FLASH_LED_MODE_FLASH,
	V4L2_FLASH_LED_MODE_TORCH
};

#define V4L2_CID_FLASH_STROBE_SOURCE    (V4L2_CID_FLASH_CLASS_BASE + 2)
enum v4l2_flash_strobe_source {
	V4L2_FLASH_STROBE_SOURCE_SOFTWARE,
	V4L2_FLASH_STROBE_SOURCE_EXTERNAL
};

#define V4L2_CID_FLASH_STROBE           (V4L2_CID_FLASH_CLASS_BASE + 3)
#define V4L2_CID_FLASH_STROBE_STOP	    (V4L2_CID_FLASH_CLASS_BASE + 4)
#define V4L2_CID_FLASH_STROBE_STATUS    (V4L2_CID_FLASH_CLASS_BASE + 5)

#define V4L2_CID_FLASH_TIMEOUT          (V4L2_CID_FLASH_CLASS_BASE + 6)
#define V4L2_CID_FLASH_INTENSITY        (V4L2_CID_FLASH_CLASS_BASE + 7)
#define V4L2_CID_FLASH_TORCH_INTENSITY  (V4L2_CID_FLASH_CLASS_BASE + 8)
#define V4L2_CID_FLASH_INDICATOR_INTENSITY  (V4L2_CID_FLASH_CLASS_BASE + 9)

#define V4L2_CID_FLASH_FAULT            (V4L2_CID_FLASH_CLASS_BASE + 10)
#define V4L2_FLASH_FAULT_OVER_VOLTAGE       (1 << 0)
#define V4L2_FLASH_FAULT_TIMEOUT            (1 << 1)
#define V4L2_FLASH_FAULT_OVER_TEMPERATURE   (1 << 2)
#define V4L2_FLASH_FAULT_SHORT_CIRCUIT      (1 << 3)

#define V4L2_CID_FLASH_CHARGE           (V4L2_CID_FLASH_CLASS_BASE + 11)
#define V4L2_CID_FLASH_READY            (V4L2_CID_FLASH_CLASS_BASE + 12)

/* JPEG-class control IDs */
 
#define V4L2_CID_JPEG_CLASS_BASE        (V4L2_CTRL_CLASS_JPEG | 0x900)
#define V4L2_CID_JPEG_CLASS             (V4L2_CTRL_CLASS_JPEG | 1)

#define V4L2_CID_JPEG_CHROMA_SUBSAMPLING    (V4L2_CID_JPEG_CLASS_BASE + 1)
enum v4l2_jpeg_chroma_subsampling {
        V4L2_JPEG_CHROMA_SUBSAMPLING_444    = 0,
        V4L2_JPEG_CHROMA_SUBSAMPLING_422    = 1,
        V4L2_JPEG_CHROMA_SUBSAMPLING_420    = 2,
        V4L2_JPEG_CHROMA_SUBSAMPLING_411    = 3,
        V4L2_JPEG_CHROMA_SUBSAMPLING_410    = 4,
        V4L2_JPEG_CHROMA_SUBSAMPLING_GRAY   = 5
};

#define V4L2_CID_JPEG_RESTART_INTERVAL      (V4L2_CID_JPEG_CLASS_BASE + 2)
#define V4L2_CID_JPEG_COMPRESSION_QUALITY   (V4L2_CID_JPEG_CLASS_BASE + 3)

#define V4L2_CID_JPEG_ACTIVE_MARKER     (V4L2_CID_JPEG_CLASS_BASE + 4)
#define V4L2_JPEG_ACTIVE_MARKER_APP0    (1 << 0)
#define V4L2_JPEG_ACTIVE_MARKER_APP1    (1 << 1)
#define V4L2_JPEG_ACTIVE_MARKER_COM     (1 << 16)
#define V4L2_JPEG_ACTIVE_MARKER_DQT     (1 << 17)
#define V4L2_JPEG_ACTIVE_MARKER_DHT     (1 << 18)


/*  Detection-class control IDs defined by V4L2 */
#define V4L2_CID_DETECT_CLASS_BASE      (V4L2_CTRL_CLASS_DETECT | 0x900)
#define V4L2_CID_DETECT_CLASS           (V4L2_CTRL_CLASS_DETECT | 1)

#define V4L2_CID_DETECT_MD_MODE         (V4L2_CID_DETECT_CLASS_BASE + 1)
enum v4l2_detect_md_mode {
        V4L2_DETECT_MD_MODE_DISABLED        = 0,
        V4L2_DETECT_MD_MODE_GLOBAL          = 1,
        V4L2_DETECT_MD_MODE_THRESHOLD_GRID  = 2,
        V4L2_DETECT_MD_MODE_REGION_GRID     = 3
};

#define V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD (V4L2_CID_DETECT_CLASS_BASE + 2)
#define V4L2_CID_DETECT_MD_THRESHOLD_GRID   (V4L2_CID_DETECT_CLASS_BASE + 3)
#define V4L2_CID_DETECT_MD_REGION_GRID      (V4L2_CID_DETECT_CLASS_BASE + 4)

#define v4l2_fourcc(a, b, c, d)\
          ((unsigned long)(a) | ((unsigned long)(b) << 8) |\
          ((unsigned long)(c) << 16) | ((unsigned long)(d) << 24))

/* H264 with start codes */
#define V4L2_PIX_FMT_H264     v4l2_fourcc('H', '2', '6', '4')
/* Motion-JPEG */
#define V4L2_PIX_FMT_MJPEG    v4l2_fourcc('M', 'J', 'P', 'G')
/* Greyscale */
#define V4L2_PIX_FMT_GREY     v4l2_fourcc('G', 'R', 'E', 'Y')
/* Y/CbCr 4:2:0 */
#define V4L2_PIX_FMT_NV12     v4l2_fourcc('N', 'V', '1', '2')
/* MPEG-1/2/4 Multiplexed */
#define V4L2_PIX_FMT_MPEG     v4l2_fourcc('M', 'P', 'E', 'G')
/* Y/Cb/Cr planar 4:2:0 */
#define V4L2_PIX_FMT_I420     v4l2_fourcc('I', '4', '2', '0')

// need same as linux video driver 
#define AITCAM_MAX_QUEUE_SIZE   (16)
#define AITCAM_NUM_CONTEXTS     (6)

#define VIDBUF_READY_QUEUE      (0)
#define VIDBUF_FREE_QUEUE       (1)

// copy from ait_cam_v4l2.h
/*  MPEG-class control IDs specific to the AIT-CAM driver as defined by V4L2 */
#define V4L2_CID_MPEG_AIT_BASE                  (V4L2_CTRL_CLASS_MPEG | 0x1800)

#define V4L2_CID_MPEG_AIT_VIDEO_FORCE_FRAME     (V4L2_CID_MPEG_AIT_BASE+0)
enum v4l2_mpeg_ait_video_force_frame_mode{
    V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_NONE        = 0,
    V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_IDR_RESYNC  = 1,
    V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_IDR         = 2,
    V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_I_RESYNC    = 3,
    V4L2_MPEG_AIT_VIDEO_FORCE_FRAME_I           = 4
};

#define V4L2_CID_MPEG_AIT_VIDEO_FPS_RESOL       (V4L2_CID_MPEG_AIT_BASE+1)
#define V4L2_CID_MPEG_AIT_VIDEO_FPS_INCR        (V4L2_CID_MPEG_AIT_BASE+2)
#define V4L2_CID_MPEG_AIT_VIDEO_RC_FRAME_SKIP   (V4L2_CID_MPEG_AIT_BASE+3)
#define V4L2_CID_MPEG_AIT_GOP_LENGTH            (V4L2_CID_MPEG_AIT_BASE+4) // gop length in 100ms
// TNR
#define V4L2_CID_MPEG_AIT_TNR                   (V4L2_CID_MPEG_AIT_BASE+5) // en tnr
enum v4l2_h264_tnr_mode {
    V4L2_H264_TNR_LOW_MV_EN     = 1,
    V4L2_H264_TNR_ZERO_MV_EN    = 2,
    V4L2_H264_TNR_HIGH_MV_EN    = 4   
} ;
#define V4L2_CID_MPEG_AIT_TNR_DUMP              (V4L2_CID_MPEG_AIT_BASE+6) // dump tnr opr
#define V4L2_CID_MPEG_AIT_TNR_LOW_MV_THR        (V4L2_CID_MPEG_AIT_BASE+7) // low motion mv thr
#define V4L2_CID_MPEG_AIT_TNR_ZERO_MV_THR       (V4L2_CID_MPEG_AIT_BASE+8) // zero motion mv thr
#define V4L2_CID_MPEG_AIT_TNR_ZERO_LUMA_PXL_DIFF_THR    (V4L2_CID_MPEG_AIT_BASE+9) 
#define V4L2_CID_MPEG_AIT_TNR_ZERO_CHROMA_PXL_DIFF_THR  (V4L2_CID_MPEG_AIT_BASE+10) 
#define V4L2_CID_MPEG_AIT_TNR_ZERO_MV_4x4_CNT_THR       (V4L2_CID_MPEG_AIT_BASE+11)
/*
mv_filter : 4 bytes definition
mv_filter[0] = luma_4x4 
mv_filter[1] = chroma_4x4
mv_filter[2~3] = 8x8 thr (mv_filter[2] | mv_filter[3] << 8)
*/
#define V4L2_CID_MPEG_AIT_TNR_LOW_MV_FILTER     (V4L2_CID_MPEG_AIT_BASE+12)
#define V4L2_CID_MPEG_AIT_TNR_ZERO_MV_FILTER    (V4L2_CID_MPEG_AIT_BASE+13)
#define V4L2_CID_MPEG_AIT_TNR_HIGH_MV_FILTER    (V4L2_CID_MPEG_AIT_BASE+14)
/*
H264 RDO enable/disable
*/
#define V4L2_CID_MPEG_AIT_RDO                   (V4L2_CID_MPEG_AIT_BASE+15)
/*
complicate scene <---> smooth scene
qstep = { 0 ,1,2,3,4,5,6,7,8,9 } ;
each 0~9 use nibble byte ( 4 bits)
complicate scene :increase qstep
smooth scene : decrease qstep

increase qstep : ex: 1,2,3,4...
decrease qstep : ex: (16-1):-1,(16-2):-2,(16-3):-3....

*/
#define V4L2_CID_MPEG_AIT_QSTEP3_P1             (V4L2_CID_MPEG_AIT_BASE+16)        
#define V4L2_CID_MPEG_AIT_QSTEP3_P2             (V4L2_CID_MPEG_AIT_BASE+17)
#define V4L2_CID_MPEG_AIT_RC_SKIP_TYPE          (V4L2_CID_MPEG_AIT_BASE+18)

/*  Camera class control IDs specific to the AIT-CAM driver as defined by V4L2 */
#define V4L2_CID_CAMERA_AIT_BASE                (V4L2_CTRL_CLASS_CAMERA | 0x1800)
#define V4L2_CID_CAMERA_AIT_ORIENTATION         (V4L2_CID_CAMERA_AIT_BASE+0)
enum v4l2_camera_ait_orientation {
    V4L2_CAMERA_AIT_ORTN_NORMAL = 0,
    V4L2_CAMERA_AIT_ORTN_FLIP,
    V4L2_CAMERA_AIT_ORTN_MIRROR,
    V4L2_CAMERA_AIT_ORTN_FLIP_MIRROR
};

#define V4L2_CID_CAMERA_AIT_EXPOSURE_STATE      (V4L2_CID_CAMERA_AIT_BASE+1)
#define V4L2_CID_CAMERA_AIT_MANUAL_R_GAIN       (V4L2_CID_CAMERA_AIT_BASE+2)
#define V4L2_CID_CAMERA_AIT_MANUAL_G_GAIN       (V4L2_CID_CAMERA_AIT_BASE+3)
#define V4L2_CID_CAMERA_AIT_MANUAL_B_GAIN       (V4L2_CID_CAMERA_AIT_BASE+4)
#define V4L2_CID_CAMERA_AIT_NIGHT_VISION        (V4L2_CID_CAMERA_AIT_BASE+5) // off/on/auto
#define V4L2_CID_CAMERA_AIT_IR_LED              (V4L2_CID_CAMERA_AIT_BASE+6) // off/on
#define V4L2_CID_CAMERA_AIT_IR_SHUTTER          (V4L2_CID_CAMERA_AIT_BASE+7) // off/on
// Get 
#define V4L2_CID_CAMERA_AIT_LIGHT_MODE_STATE    (V4L2_CID_CAMERA_AIT_BASE+8) // day/night
#define V4L2_CID_CAMERA_AIT_LIGHT_CONDITION     (V4L2_CID_CAMERA_AIT_BASE+9) // 32bits light value for advance control
enum v4l2_camera_ait_night_vision {
    V4L2_CAMERA_AIT_NV_OFF  = 0,
    V4L2_CAMERA_AIT_NV_ON   = 1,
    V4L2_CAMERA_AIT_NV_AUTO = 2   
};

#define V4L2_CID_CAMERA_AIT_IMAGE_EFFECT        (V4L2_CID_CAMERA_AIT_BASE+10)  // ISP_IMAGE_EFFECT
enum v4l2_camera_ait_image_effect {
    V4L2_CAMERA_AIT_EFFECT_NORMAL   = 0,
    V4L2_CAMERA_AIT_EFFECT_GREY     = 1,
    V4L2_CAMERA_AIT_EFFECT_SEPIA    = 2,
    V4L2_CAMERA_AIT_EFFECT_NEGATIVE = 3,
    V4L2_CAMERA_AIT_EFFECT_RED      = 4,
    V4L2_CAMERA_AIT_EFFECT_GREEN    = 5,
    V4L2_CAMERA_AIT_EFFECT_BLUE     = 6,
    V4L2_CAMERA_AIT_EFFECT_YELLOW   = 7,
    V4L2_CAMERA_AIT_EFFECT_BW       = 8
};

#define V4L2_CID_CAMERA_AIT_NR_GAIN             (V4L2_CID_CAMERA_AIT_BASE+11) // 0 ~ 0x0A(low lux) 
#define V4L2_CID_CAMERA_AIT_STREAM_TYPE         (V4L2_CID_CAMERA_AIT_BASE+12) 
enum v4l2_camera_ait_stream_type {
    V4L2_CAMERA_AIT_STORAGE_TYPE      = (0 << 0),
    V4L2_CAMERA_AIT_REALTIME_TYPE     = (1 << 0),
    V4L2_CAMERA_AIT_LOOP_RECORDING    = (1 << 1)
};


#define V4L2_CID_CAMERA_AIT_STREAMER_ALIVE (V4L2_CID_CAMERA_AIT_BASE+19) 
enum v4l2_camera_ait_streamer_alive {
  V4L2_CAMERA_AIT_STREAMER_DOWN  =0 ,
  V4L2_CAMERA_AIT_STREAMER_UP    =1 ,
} ;

#define V4L2_CID_CAMERA_AIT_GET_TEMPERATURE (V4L2_CID_CAMERA_AIT_BASE+20)

/*
 *  to indicate no stream object ID matched
 */
#define V4L2_INVALID_OBJ_ID     (-1)

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
typedef struct __attribute__((__packed__)) { 
    MMP_ULONG   used_size;
    MMP_ULONG   buf_start;
    MMP_ULONG64 timestamp;
    MMP_ULONG   flags;
} AITCAM_VIDBUF_INFO;

typedef struct __attribute__((__packed__)) { 
    ///< queue for buffer ready to encode, in display order
    MMP_ULONG   buffers[AITCAM_MAX_QUEUE_SIZE];
    MMP_UBYTE   head;
    MMP_UBYTE   size;
} AITCAM_VIDENC_QUEUE;

typedef struct __attribute__((__packed__)) { 
    MMP_ULONG           buf_num;
    MMP_ULONG           buf_size;
    MMP_ULONG           buf_addr;
    AITCAM_VIDBUF_INFO  buffers[AITCAM_MAX_QUEUE_SIZE];
    AITCAM_VIDENC_QUEUE free_vbq, ready_vbq;
    MMP_UBYTE           ready_waitq[8];
    // mutex lock for blocking mode
	MMP_UBYTE           vbq_lock[12]; // same as linux size
    int                 non_blocking;
} AITCAM_VIDBUF_QUEUE;

/*
streamtype
*/
#define CTRL_FLAG_OSD_EN    (1<<0)

typedef struct __attribute__((__packed__)) { 
    int             streamid;
    unsigned int    ctrl_flag;
    __packed union {
    int             img_width;
    int             bitrate;
    } P1;
    __packed union {
    int             img_height;
    int             samplerate;
    } P2;
    unsigned long   img_fmt; 
    unsigned long   max_framesize;   
    int             streamtype;     // enum v4l2_camera_ait_stream_type
} aitcam_img_info;
 
typedef struct __attribute__((__packed__)) { 
    aitcam_img_info img_info;
    unsigned long   vbq_virt_addr;
    unsigned long   vbq_phy_addr; 
    int             remote_q_size; 
} aitcam_ipc_info;

typedef struct __attribute__((__packed__)) { 
    int             streamid;
    unsigned long   id;
    int             val;  
    char            name[32];
} aitcam_ipc_ctrl;

typedef struct {
    int             state;
    aitcam_ipc_info info;
    VIDENC_CTL_SET  ctrl_set;
    unsigned int    frame_seq;
    unsigned long   id;
} aitcam_stream;

typedef struct {
    int             streamid;
    int             obj_id;
    int             used;
} aitcam_streamtbl;

// OSD type define
#define MAX_OSD_INDEX           (10)

enum ait_osd_type {
    AIT_OSD_TYPE_INACTIVATE = 0,
    AIT_OSD_TYPE_RTC,
    AIT_OSD_TYPE_CUST_STRING,
    AIT_OSD_TYPE_WATERMARK,
    AIT_OSD_TYPE_FDFRAME
};



typedef struct _ait_fdframe_config {
    unsigned long   index;
    unsigned long   type;

    unsigned long   pos[AXIS_MAX];
    unsigned long   width;
    unsigned long   height;
    unsigned long   TextColorY;
    unsigned long   TextColorU;
    unsigned long   TextColorV;

    char            str[MAX_OSD_STR_LEN];
} ait_fdframe_config;

typedef enum 
{
  IPC_V4L2_SET = 0 ,
  IPC_V4L2_GET
} aitcm_ipc_ctrl_dir ;
//==============================================================================
//
//                              DATA TYPES
//
//==============================================================================

//typedef void (*ipc_signal_ready)(int streamid);
typedef void ipc_osd_drawer(void *argu);

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

int aitcam_ipc_state(int streamid);
int aitcam_ipc_open(aitcam_img_info *out_img_info);
int aitcam_ipc_streamon(aitcam_ipc_info *ipc_info);
int aitcam_ipc_streamoff(int streamid);
int aitcam_ipc_release(int streamid);
int aitcam_ipc_set_ctrl(aitcam_ipc_ctrl *ctrl);
int aitcam_ipc_get_ctrl(aitcam_ipc_ctrl *ctrl);
int aitcam_ipc_set_ctrl_val(int stream_id, unsigned long id, int val);
void aitcam_ipc_push_frame(MMP_UBYTE streamid);
MMP_ULONG aitcam_ipc_cur_wr(MMP_UBYTE streamid);
MMP_ULONG aitcam_ipc_free_size(MMP_UBYTE streamid);
void aitcam_ipc_fill_frame_info(MMP_UBYTE streamid,
                                MMP_ULONG framelength,
                                MMP_ULONG64 timestamp);
int aitcam_ipc_osd_register_drawer(int streamid , unsigned long fmt, ipc_osd_drawer *drawer,int osd_type);
ait_fdframe_config *aitcam_ipc_osd_base(void);  
ait_fdframe_config *aitcam_ipc_mdosd_base(void);  
int aitcam_ipc_is_debug_ts(void);
void aitcam_ipc_debug_ts(int dbg_on);
void aitcam_ipc_debug(int dbg_on);
void aitcam_ipc_debug_osd(int dbg_on);
int  aitcam_enable_md_osd(MMP_BOOL en,int md_w,int md_h);
int  aitcam_set_md_osd(int at,int md_n,MD_block_info_t *area);
int  aitcam_ipc_set_iq(void *) ;
                            
#endif

