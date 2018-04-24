/**
 @file mmpf_mdtc.h
 @brief Header function of Motion Detection
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_IVA_H_
#define _MMPF_IVA_H_

#include "includes_fw.h"
//#include "ipc_cfg.h"
#include "mmps_ystream.h"

/** @addtogroup MMPF_IVA
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
#define IVA_STREAM_ID      (100) // virtual stream id , not overlap streamid from v4l2 
#define IVA_IMG_FMT         V4L2_PIX_FMT_I420
#define IVA_IMG_W          (320)
#define IVA_IMG_H          (180)

#define IVA_LUMA_SLOTS     (GRAY_FRM_SLOTS)


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
// IVA API put here


/*
 * Buffer queue
 */
typedef struct {
    MMP_ULONG   head;
    MMP_ULONG   size;
    MMP_UBYTE   data[IVA_LUMA_SLOTS];
} MMPF_IVA_QUEUE;

/*
 * Class
 */
typedef struct {
    MMPS_YSTREAM_CLASS     *obj;   ///< MD object
    MMPF_IVA_QUEUE     free_q; ///< Queue for free luma buffers
    MMPF_IVA_QUEUE     rdy_q;  ///< Queue for ready luma frames
    volatile MMP_BOOL   busy;   ///< MD in operation
    
    void            *iva_priv ; /// < IVA private data
} MMPF_IVA_HANDLE;
/*
 * IVA handler
 */

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ULONG   MMPF_IVA_BufSize(MMP_ULONG w,MMP_ULONG h);
MMP_ERR     MMPF_IVA_Init(void);
MMP_ERR     MMPF_IVA_Open(MMPS_YSTREAM_CLASS *iva);
MMP_ERR     MMPF_IVA_Close(void);

/// @}

#endif  // _MMPF_MDTC_H_
