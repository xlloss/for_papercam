/**
 @file mmpf_src.h
 @brief Header of Sampling rate converter
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_SRC_H_
#define _MMPF_SRC_H_

#include "includes_fw.h"
#include "AudioSRCProcess.h"

/** @addtogroup MMPF_AEC
@{
*/

//==============================================================================
//
//                              OPTIONS
//
//==============================================================================


//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define SRC_BLOCK_SIZE  (256)

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================


//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/*
 * AEC Buffer block
 */
typedef struct {
    MMP_ULONG   base;       ///< Base address of memory block
    MMP_ULONG   size;       ///< Size of memory block
} ST_SRC_MEMBLK;

/*
 * AEC Class
 */
typedef struct {
    MMP_BOOL                mod_init;   ///< Is AEC module initialized?
    MMP_BOOL                enable;     ///< Is AEC function enabled?
    ST_SRC_MEMBLK           workbuf;    ///< Working buffer address
    SRCStructProcess        ap_src;     ///< Process structure
    //AUTL_RINGBUF            spk_ring;   ///< Ring buffer for speak out data
    SRC_HANDLE              handle ;
} MMPF_SRC_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ERR     MMPF_SRC_Init(int id,int channels);
void        MMPF_SRC_Enable(int id);
void        MMPF_SRC_Disable(int id);
MMP_BOOL    MMPF_SRC_IsEnable(int id);
MMP_SHORT  *MMPF_SRC_Process(int id,MMP_SHORT *in, int samples,int *out_samples);

/// @}

#endif  // _MMPF_AEC_H_
