//==============================================================================
//
//  File        : sensor_OV5653.h
//  Description : Firmware Sensor Control File
//  Author      : Andy Liu
//  Revision    : 1.0
//
//=============================================================================

#ifndef	_SENSOR_OV5653_H_
#define	_SENSOR_OV5653_H_

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define SENSOR_IF				(SENSOR_IF_PARALLEL) // OV5653 can support digital video port (DVP) parallel output interface and 1-lane MIPI interface (up to 800Mbps)

#define RES_IDX_1920x1080_30P 	(0) 	// mode 0, 1920 * 1080 30P, Video  (16:9)
#define RES_IDX_1280x720_30P 	(1) 	// mode 1, 1280 *  720 30P, Video  (16:9)
#define RES_IDX_1280x720_60P 	(2) 	// mode 2, 1280 *  720 60P, Video  (16:9)
#define RES_IDX_1440x1080_30P 	(3) 	// mode 3, 1440 * 1080 30P, Camera ( 4:3)
#define RES_IDX_2560x1920_14P 	(4) 	// mode 4, 2560 * 1920 15P, Camera ( 4:3)

#ifndef SENSOR_ROTATE_180
#define SENSOR_ROTATE_180		(0)
#endif

#define MAX_SENSOR_GAIN			(16)

//==============================================================================
//
//                              MACRO DEFINE (Resolution For UI)
//
//==============================================================================

#ifndef SENSOR_DRIVER_MODE_NOT_SUUPORT
#define SENSOR_DRIVER_MODE_NOT_SUUPORT 				(0xFFFF)
#endif

#define SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_16TO9_RESOLUTION (RES_IDX_1920x1080_30P)
#define SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION  (RES_IDX_1440x1080_30P)

// Index 0
#define SENSOR_DRIVER_MODE_VGA_30P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 30P
#define SENSOR_DRIVER_MODE_VGA_50P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 50P
#define SENSOR_DRIVER_MODE_VGA_60P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 60P
#define SENSOR_DRIVER_MODE_VGA_100P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 100P
#define SENSOR_DRIVER_MODE_VGA_120P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 120P

// Index 5
#define SENSOR_DRIVER_MODE_HD_24P_RESOLUTION        (SENSOR_DRIVER_MODE_NOT_SUUPORT)    // 1280*720 24P
#define SENSOR_DRIVER_MODE_HD_30P_RESOLUTION 		(RES_IDX_1920x1080_30P)             // 1280*720 30P
#define SENSOR_DRIVER_MODE_HD_50P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1280*720 50P
#define SENSOR_DRIVER_MODE_HD_60P_RESOLUTION 		(RES_IDX_1280x720_60P) 	            // 1280*720 60P
#define SENSOR_DRIVER_MODE_HD_100P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1280*720 100P

// Index 10
#define SENSOR_DRIVER_MODE_HD_120P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1280*720 120P
#define SENSOR_DRIVER_MODE_FULL_HD_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1920*1080 15P
#define SENSOR_DRIVER_MODE_FULL_HD_24P_RESOLUTION   (SENSOR_DRIVER_MODE_NOT_SUUPORT)    // 1920*1080 24P
#define SENSOR_DRIVER_MODE_FULL_HD_25P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT)    // 1920*1080 25P
#define SENSOR_DRIVER_MODE_FULL_HD_30P_RESOLUTION 	(RES_IDX_1920x1080_30P) 			// 1920*1080 30P

// Index 15
#define SENSOR_DRIVER_MODE_FULL_HD_50P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1920*1080 50P
#define SENSOR_DRIVER_MODE_FULL_HD_60P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1920*1080 60P
#define SENSOR_DRIVER_MODE_SUPER_HD_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT)    // 2304x1296 30P
#define SENSOR_DRIVER_MODE_1440_30P_RESOLUTION		(SENSOR_DRIVER_MODE_NOT_SUUPORT)	// 2560*1440 30P
#define SENSOR_DRIVER_MODE_2D7K_15P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2704*1524 15P

// Index 20
#define SENSOR_DRIVER_MODE_2D7K_30P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2704*1524 30P
#define SENSOR_DRIVER_MODE_4K2K_15P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3840*2160 15P
#define SENSOR_DRIVER_MODE_4K2K_30P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3840*2160 30P
#define SENSOR_DRIVER_MODE_4TO3_VGA_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION)            // 640*480   30P
#define SENSOR_DRIVER_MODE_4TO3_1D2M_30P_RESOLUTION (SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 1280*960  30P

// Index 25
#define SENSOR_DRIVER_MODE_4TO3_1D5M_30P_RESOLUTION (SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 1440*1080 30P
#define SENSOR_DRIVER_MODE_4TO3_3M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 2048*1536 15P
#define SENSOR_DRIVER_MODE_4TO3_3M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 2048*1536 30P
#define SENSOR_DRIVER_MODE_4TO3_5M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 2560*1920 15P
#define SENSOR_DRIVER_MODE_4TO3_5M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 2560*1920 30P

// Index 30
#define SENSOR_DRIVER_MODE_4TO3_8M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 3264*2448 15P
#define SENSOR_DRIVER_MODE_4TO3_8M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 3264*2448 30P
#define SENSOR_DRIVER_MODE_4TO3_10M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 3648*2736 15P
#define SENSOR_DRIVER_MODE_4TO3_10M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 3648*2736 30P
#define SENSOR_DRIVER_MODE_4TO3_12M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 4032*3024 15P

// Index 35
#define SENSOR_DRIVER_MODE_4TO3_12M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 4032*3024 30P
#define SENSOR_DRIVER_MODE_4TO3_14M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 4352*3264 15P
#define SENSOR_DRIVER_MODE_4TO3_14M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION) 	        // 4352*3264 30P

// For Camera Preview
#if (LCD_MODEL_RATIO_X == 16) && (LCD_MODEL_RATIO_Y == 9)
#define SENSOR_DRIVER_MODE_BEST_CAMERA_PREVIEW_RESOLUTION       (RES_IDX_1920x1080_30P)
#else
#define SENSOR_DRIVER_MODE_BEST_CAMERA_PREVIEW_RESOLUTION       (SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION)
#endif

#define SENSOR_DRIVER_MODE_FULL_HD_30P_RESOLUTION_HDR           (SENSOR_DRIVER_MODE_NOT_SUUPORT)

#endif // _SENSOR_OV5653_H_

