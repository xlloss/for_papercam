#ifndef _ALL_FW_H_
#define _ALL_FW_H_

#include "lib_retina.h"

/* Task priority & stack size*/
#define TASK_AUD_CRITICAL_PRIO      (10)
    #define AUD_CRITICAL_STK_SIZE       (768)
#define TASK_MP4VENC_PRIO           (6)
    #define MP4VENC_STK_SIZE            (384)
#define TASK_SENSOR_PRIO            (11)
    #define TASK_SENSOR_STK_SIZE        (512)
#define TASK_VSTREAM_PRIO           (13)
    #define VSTREAM_STK_SIZE            (512)
#define TASK_JSTREAM_PRIO           (14)
    #define JSTREAM_STK_SIZE            (256)
#define TASK_AUD_ENCODE_PRIO        (15)
    #define AUD_ENCODE_STK_SIZE         (1024)
#define TASK_STREAMER_PRIO          (16)
    #define STREAMER_STK_SIZE           (512)
#define TASK_ALSA_PRIO              (17)
    #define ALSA_STK_SIZE               (256)

#define TASK_OSD_PRIO              (27)
    #define OSD_STK_SIZE               (512)

#define TASK_MDTC_PRIO              (25)
    #define MDTC_STK_SIZE               (512)

#define TASK_IVA_PRIO              (26)
    #define IVA_STK_SIZE               (1024)


#define TASK_JOB_DISPATCH_PRIO      (50)
    #define TASK_JOB_DISPATCH_STK_SIZE  (1024)

/* For CPU IPC */
#define TASK_CPUB_START_PRIO        (51)
    #define TASK_B_SYS_STK_SIZE         (256)
#define TASK_B_V4L2_PRIO            (TASK_CPUB_START_PRIO + 1)
    #define TASK_B_V4L2_STK_SIZE        (512)
#define TASK_B_ALSA_PRIO            (TASK_CPUB_START_PRIO + 2)
    #define TASK_B_ALSA_STK_SIZE        (512)
#define TASK_B_MD_PRIO              (TASK_CPUB_START_PRIO + 3)
    #define TASK_B_MD_STK_SIZE        (512)

#define TASK_B_IVA_PRIO             (TASK_CPUB_START_PRIO + 4)
    #define TASK_B_IVA_STK_SIZE        (512)
#define TASK_B_I2C_PRIO             (TASK_CPUB_START_PRIO + 5)
    #define TASK_B_I2C_STK_SIZE        (512)


/*
TBD : 5,6,7
*/
#define TASK_B_MISC_IO_PRIO            (TASK_CPUB_START_PRIO + 8)
    #define TASK_B_MISC_IO_SIZE        (512)


#define TASK_B_WD_PRIO              (62)
    #define TASK_B_WD_STK_SIZE      (128)

#endif //#ifndef _ALL_FW_H_

