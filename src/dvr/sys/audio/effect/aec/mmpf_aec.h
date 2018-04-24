/**
 @file mmpf_aec.h
 @brief Header of Acoustic Echo Cancellation Function
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_AEC_H_
#define _MMPF_AEC_H_

#include "includes_fw.h"
#include "AEC.h"
#include "AudioProcess.h"
#include "AudioAecProcess.h"

/** @addtogroup MMPF_AEC
@{
*/

//==============================================================================
//
//                              OPTIONS
//
//==============================================================================

/*
 * Option to support AEC function debugging.
 * Keep L channel from AEC effecting for comparing with R channel
 */
#define AEC_FUNC_DBG            (1)

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define AEC_BLOCK_SIZE      (512) // Block size in unit of sample for all ch
#define APC_BLOCK_SIZE      (512)
#define AEC_SPK_BUF_SIZE    (AEC_BLOCK_SIZE << 2)

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
} ST_AEC_MEMBLK;

/*
 * AEC Class
 */
typedef struct {
    MMP_BOOL                mod_init;   ///< Is AEC module initialized?
    MMP_BOOL                enable;     ///< Is AEC function enabled?
    MMP_BOOL                pause ;
    
    MMP_BOOL                dbg_mode;   ///< Debug mode, apply R channel only
    ST_AEC_MEMBLK           workbuf;    ///< Working buffer address
    AudioAecProcessStruct   ap_aec;     ///< Process structure
    AudioProcessStruct      ap_nragc;
    AUTL_RINGBUF            spk_ring;   ///< Ring buffer for speak out data
} MMPF_AEC_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ERR     MMPF_AEC_Init(void);
void        MMPF_AEC_Enable(void);
void        MMPF_AEC_Disable(void);
MMP_BOOL    MMPF_AEC_IsEnable(void);
void        MMPF_AEC_Process(void);
void        MMPF_AEC_TxReady(MMP_SHORT *buf, MMP_ULONG amount, MMP_ULONG ch);
void        MMPF_AEC_DebugEnable(MMP_BOOL en);

/// @}

#endif  // _MMPF_AEC_H_
