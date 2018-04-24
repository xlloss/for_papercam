//==============================================================================
//
//  File        : sensor_ov4689.h
//  Description : Firmware Sensor Control File
//  Author      : Andy Liu
//  Revision    : 1.0
//
//==============================================================================

#ifndef _SENSOR_OV4689_H_
#define _SENSOR_OV4689_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#ifndef SENSOR_IF
#define SENSOR_IF                   (SENSOR_IF_MIPI_4_LANE)
#endif

#if (HDR_FOV_ENLARGE)
#define RES_IDX_2432x1368_60P_HDR           (0)     // mode 0,  2432*1368 60P
#else
#define RES_IDX_2304x1296_60P_HDR	        (0)		// mode 0,  2304*1296 60P
#endif
#define RES_IDX_2560x1440_60P		        (1)		// mode 1,  2560*1440 60P       // Video (16:9)
#define RES_IDX_2688x1520_30P               (2)     // mode 2,  2688*1520 30P    	// Video (16:9)
#define RES_IDX_1280x960_30P                (3)     // mode 3,  1280*960  30P
#define RES_IDX_1920x1080_30P               (4)     // mode 4,  1920*1080 30P     	// Video (16:9)
#define RES_IDX_1920x1080_50P               (5)     // mode 5,  1920*1080 50P
#define RES_IDX_1920x1080_60P               (6)     // mode 6,  1920*1080 60P
#define RES_IDX_2016x1512_30P               (7)     // mode 7,  2016*1512 30P   	// Camera (4:3)
#define RES_IDX_1920x1080_15P               (8)     // mode 8,  1920*1080 15P
#define RES_IDX_1280x720_30P                (9)     // mode 9,  1280*720  30P   	// Video (16:9)
#define RES_IDX_1280x720_60P                (10)    // mode 10, 1280*720  60P    	// Video (16:9)
#define RES_IDX_1280x720_100P               (11)	// mode 11, 1280*720  100P 
#define RES_IDX_1280x720_120P               (12)    // mode 12, 1280*720  120P
#define RES_IDX_640x480_30P                 (13)    // mode 13, 640*480   30P		// Video (4:3)
#define RES_IDX_2016x1512_30P_SCALEUP       (14)    // mode 14, 2016*1512 30P       // Camera (4:3)
#define RES_IDX_1920x1440_25P               (15)    // mode 15, 1920*1440 25P       // Video (16:9)
#define RES_IDX_1920x1440_30P               (16)    // mode 16, 1920*1440 30P       // Video (16:9)
#define RES_IDX_2688x1520_25P               (17)    // mode 17, 2688*1520 25P    	// Video (16:9)
#define RES_IDX_2688x1520_50P               (18)    // mode 18, 2688*1520 50P    	// Video (16:9)

#define RES_IDX_2560x1440_30P               (19)    // mode 19, 2560*1440 30P    	// Video (16:9)
#define RES_IDX_2688x1512_30P               (20)    // mode 20, 2688*1512 30P    	// Video (16:9)


#ifndef SNR_OV4689_SID_PIN
#define SNR_OV4689_SID_PIN          (0) // SID pin should be pulled high for device address 0x20 and pulled low for device address 0x6C.
                                        // The device slave addresses are 0x6C for write and 0x6D for read (when SID=1, 0x20 for write and 0x21 for read).
#endif

#define TEST_PATTERN_EN 			(0)

//==============================================================================
//
//                              MACRO DEFINE (Resolution For UI)
//
//==============================================================================

#ifndef SENSOR_DRIVER_MODE_NOT_SUUPORT
#define SENSOR_DRIVER_MODE_NOT_SUUPORT 				(0xFFFF)
#endif

// Index 0
#define SENSOR_DRIVER_MODE_VGA_30P_RESOLUTION 		(RES_IDX_2016x1512_30P) 			// 640*360 30P
#define SENSOR_DRIVER_MODE_VGA_50P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 50P
#define SENSOR_DRIVER_MODE_VGA_60P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 60P
#define SENSOR_DRIVER_MODE_VGA_100P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 100P
#define SENSOR_DRIVER_MODE_VGA_120P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 640*360 120P

// Index 5
#define SENSOR_DRIVER_MODE_HD_24P_RESOLUTION        (RES_IDX_2688x1520_25P)             // 1280*720 24P
#define SENSOR_DRIVER_MODE_HD_30P_RESOLUTION 		(RES_IDX_2688x1520_30P) 			// 1280*720 30P
#define SENSOR_DRIVER_MODE_HD_50P_RESOLUTION 		(RES_IDX_2688x1520_50P) 	        // 1280*720 50P
#define SENSOR_DRIVER_MODE_HD_60P_RESOLUTION 		(RES_IDX_2560x1440_60P)             // (RES_IDX_1280x720_60P) 				// 1280*720 60P
#define SENSOR_DRIVER_MODE_HD_100P_RESOLUTION 		(RES_IDX_1280x720_100P) 	        // 1280*720 100P

// Index 10
#define SENSOR_DRIVER_MODE_HD_120P_RESOLUTION 		(RES_IDX_1280x720_120P)             // (SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1280*720 120P
#define SENSOR_DRIVER_MODE_FULL_HD_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1920*1080 15P
#define SENSOR_DRIVER_MODE_FULL_HD_24P_RESOLUTION   (SENSOR_DRIVER_MODE_NOT_SUUPORT)    // 1920*1080 24P
#define SENSOR_DRIVER_MODE_FULL_HD_25P_RESOLUTION 	(RES_IDX_2688x1520_25P)             // (SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 1920*1080 25P
#define SENSOR_DRIVER_MODE_FULL_HD_30P_RESOLUTION 	(RES_IDX_2688x1520_30P)				// 1920*1080 30P

// Index 15
#define SENSOR_DRIVER_MODE_FULL_HD_50P_RESOLUTION 	(RES_IDX_2688x1520_50P) 	        // 1920*1080 50P
#define SENSOR_DRIVER_MODE_FULL_HD_60P_RESOLUTION 	(RES_IDX_2560x1440_60P)             // (RES_IDX_1920x1080_60P) 			// 1920*1080 60P
#define SENSOR_DRIVER_MODE_SUPER_HD_30P_RESOLUTION 	(RES_IDX_2688x1520_30P) 	        // 2304*1296 30P
#define SENSOR_DRIVER_MODE_2D7K_15P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2704*1524 15P
#define SENSOR_DRIVER_MODE_2D7K_30P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2704*1524 30P

// Index 20
#define SENSOR_DRIVER_MODE_4K2K_15P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3840*2160 15P
#define SENSOR_DRIVER_MODE_4K2K_30P_RESOLUTION 		(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3840*2160 30P
#define SENSOR_DRIVER_MODE_4TO3_VGA_30P_RESOLUTION 	(RES_IDX_2016x1512_30P) 			// 640*480   30P
#define SENSOR_DRIVER_MODE_4TO3_1D2M_30P_RESOLUTION (RES_IDX_2016x1512_30P) 			// 1280*960  30P
#define SENSOR_DRIVER_MODE_4TO3_1D5M_30P_RESOLUTION (RES_IDX_2016x1512_30P) 			// 1440*1080 30P

// Index 25
#define SENSOR_DRIVER_MODE_4TO3_3M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2016*1512 15P
#define SENSOR_DRIVER_MODE_4TO3_3M_30P_RESOLUTION 	(RES_IDX_2688x1512_30P)             // 2016*1512 30P
#define SENSOR_DRIVER_MODE_4TO3_5M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2560*1920 15P
#define SENSOR_DRIVER_MODE_4TO3_5M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 2560*1920 30P
#define SENSOR_DRIVER_MODE_4TO3_8M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3264*2448 15P

// Index 30
#define SENSOR_DRIVER_MODE_4TO3_8M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3264*2448 30P
#define SENSOR_DRIVER_MODE_4TO3_10M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3648*2736 15P
#define SENSOR_DRIVER_MODE_4TO3_10M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 3648*2736 30P
#define SENSOR_DRIVER_MODE_4TO3_12M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 4032*3024 15P
#define SENSOR_DRIVER_MODE_4TO3_12M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 4032*3024 30P

// Index 35
#define SENSOR_DRIVER_MODE_4TO3_14M_15P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 4352*3264 15P
#define SENSOR_DRIVER_MODE_4TO3_14M_30P_RESOLUTION 	(SENSOR_DRIVER_MODE_NOT_SUUPORT) 	// 4352*3264 30P

#define SENSOR_DRIVER_MODE_8M_SCALEUP_RESOLUTION    (RES_IDX_2016x1512_30P_SCALEUP)     // 
#define SENSOR_DRIVER_MODE_6M_SCALEUP_RESOLUTION    (RES_IDX_2016x1512_30P_SCALEUP)     // 
#define SENSOR_DRIVER_MODE_1440P_25P_RESOLUTION     (RES_IDX_1920x1440_25P)             // (RES_IDX_2016x1512_30P_SCALEUP)     // 2016*1512 30p
#define SENSOR_DRIVER_MODE_1440P_30P_RESOLUTION     (RES_IDX_1920x1440_30P)             // (RES_IDX_2016x1512_30P_SCALEUP)     // 2016*1512 30p

#define SENSOR_DRIVER_MODE_1440P_W_30P_RESOLUTION   (RES_IDX_2560x1440_30P)             // 2560*1440 30p
#define SENSOR_DRIVER_MODE_1512P_30P_RESOLUTION     (RES_IDX_2688x1512_30P)             // 2688*1512 30p

// For Camera Preview
#define SENSOR_DRIVER_MODE_BEST_CAMERA_PREVIEW_RESOLUTION   	(RES_IDX_2688x1512_30P)

#define SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_16TO9_RESOLUTION (RES_IDX_2688x1520_30P)
#define SENSOR_DRIVER_MODE_BEST_CAMERA_CAPTURE_4TO3_RESOLUTION  (RES_IDX_2016x1512_30P)

#if (HDR_FOV_ENLARGE)
#define SENSOR_DRIVER_MODE_FULL_HD_30P_RESOLUTION_HDR           (RES_IDX_2432x1368_60P_HDR)
#else
#define SENSOR_DRIVER_MODE_FULL_HD_30P_RESOLUTION_HDR           (RES_IDX_2304x1296_60P_HDR)
#endif

#endif // _SENSOR_OV4689_H_
