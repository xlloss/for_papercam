//==============================================================================
//
//  File        : includes_fw.h
//  Description : Top level global definition and configuration.
//  Author      : Jerry Tsao
//  Revision    : 1.0
//
//==============================================================================

#ifndef _INCLUDES_FW_H_
#define _INCLUDES_FW_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "AIT_Init.h"
#include "ucos_ii.h"
#include "mmp_err.h"
#include "config_fw.h"
#include "mmpf_task_cfg.h"
#include "mmpf_typedef.h"
#include "os_wrap.h"
#include "bsp.h"
#include "mmpf_hif.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

extern MMP_UBYTE    gbSystemCoreID;
extern MMP_UBYTE    gbEcoVer;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

/** @brief Memory set functions exactly like memset in standard c library.

The uFS uses FS_MEMSET marco to memset too. So we just use it here.
And this file has included string.h already.
There is a deprecated version function in sys.c.
@note Please try to use this function for faster memset and reduce the code size.
*/
#include <stdlib.h>
#ifndef	MEMSET
#define MEMSET(s,c,n)   memset(s,c,n)
#endif

/// Set this variable to 0
#ifndef	MEMSET0
#define MEMSET0(s)      memset(s,0,sizeof(*s))
#endif
#ifndef	MEMCPY
#define MEMCPY(d, s, c) memcpy (d, s, c)
#endif

void MMPF_MP4VENC_TaskHandler(void *p_arg);
void MMPF_VStream_TaskHandler(void *p_arg);
void MMPF_JStream_TaskHandler(void *p_arg);
void MMPF_Audio_CriticalTaskHandler(void *p_arg);
void MMPF_Audio_EncodeTaskHandler(void *p_arg);
void MMPF_Streamer_TaskHandler(void *p_arg);
void MMPF_OSD_TaskHandler(void *p_arg);
#if (SUPPORT_ALSA)
void MMPF_Alsa_TaskHandler(void *p_arg);
#endif
#if (SUPPORT_MDTC)
void MMPF_MDTC_TaskHandler(void *p_arg);
#endif
#if (SUPPORT_IVA)
void MMPF_IVA_TaskHandler(void *p_arg);
#endif

void MMPF_SENSOR_TaskHandler(void *p_arg);

void MISC_IO_TaskHandler(void *p_arg);
//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define MMPF_OS_Sleep_MS(_ms)   MMPF_OS_Sleep((MMP_USHORT)(OS_TICKS_PER_SEC * ((MMP_ULONG)_ms + 500L / OS_TICKS_PER_SEC) / 1000L));
#define ALIGN32(_a)     (((_a) + 31) >> 5 << 5)

#endif // _INCLUDES_FW_H_

