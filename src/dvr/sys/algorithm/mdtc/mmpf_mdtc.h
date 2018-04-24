/**
 @file mmpf_mdtc.h
 @brief Header function of Motion Detection
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_MDTC_H_
#define _MMPF_MDTC_H_

#include "includes_fw.h"
#include "ipc_cfg.h"
#include "md.h"

/** @addtogroup MMPF_MDTC
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

/* Color format of source image */
#define MD_COLOR_Y          (1)

/* Max. number of luma frame buffers */
#define MDTC_LUMA_SLOTS     (2)

/* Max. window divisions */
#define MDTC_MAX_DIV_X      (16)
#define MDTC_MAX_DIV_Y      (9)


/* ROI ver : Max regions*/
#define MDTC_MAX_ROI        (64)

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
 * Buffer block
 */
typedef struct {
    MMP_ULONG   base;       ///< Base address of memory block
    MMP_ULONG   size;       ///< Size of memory block
} ST_MDTC_MEMBLK;

/*
 * Detect Window
 */
typedef struct {
    MMP_SHORT   x_start;    ///< Left-top point in X-axis
    MMP_SHORT   y_start;    ///< Left-top point in Y-axis
    MMP_SHORT   x_end;      ///< Right-bottom point in X-axis
    MMP_SHORT   y_end;      ///< Right-bottom point in Y-axis
    MMP_USHORT  div_x;      ///< Window divisions in horizontal
    MMP_USHORT  div_y;      ///< Window divisions in vertical
    #if MD_USE_ROI_TYPE==1
    MMP_USHORT  roi_num ;
    #endif
} ST_MDTC_WIN;

/*
 * Buffer queue
 */
typedef struct {
    MMP_ULONG   head;
    MMP_ULONG   size;
    MMP_UBYTE   data[MDTC_LUMA_SLOTS];
} MMPF_MDTC_QUEUE;

/*
 * Class
 */
typedef struct {
    /* private */
    MMP_ULONG       w;          ///< Luma frame width
    MMP_ULONG       h;          ///< Luma frame height
    MMP_ULONG       luma_slots; ///< Number of luma frame slots
    ST_MDTC_MEMBLK  luma[MDTC_LUMA_SLOTS];
    ST_MDTC_MEMBLK  workbuf;    ///< Working buffers
    ST_MDTC_WIN     window;     ///< Detection window
    #if MD_USE_ROI_TYPE==0
    MD_params_in_t  param[MDTC_MAX_DIV_X][MDTC_MAX_DIV_Y];  ///< Windows param
    MD_params_out_t result[MDTC_MAX_DIV_X][MDTC_MAX_DIV_Y]; ///< Resoults
    #else
    MD_params_in_t  param[MDTC_MAX_ROI];  ///< Windows param
    MD_params_out_t result[MDTC_MAX_ROI]; ///< Resoults
    MD_block_info_t roi[MDTC_MAX_ROI] ;
    #endif
    MMP_ULONG       tick;       ///< For calculating work time per frame.
    /* md histgram */
    MD_motion_info_t md_hist;
    
    void (*md_change)(int new_val,int old_val);
    
} MMPF_MDTC_CLASS;

/*
 * MD handler
 */
typedef struct {
    MMPF_MDTC_CLASS     *obj;   ///< MD object
    MMPF_MDTC_QUEUE     free_q; ///< Queue for free luma buffers
    MMPF_MDTC_QUEUE     rdy_q;  ///< Queue for ready luma frames
    volatile MMP_BOOL   busy;   ///< MD in operation
} MMPF_MDTC_HANDLE;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ULONG   MMPF_MD_BufSize(MMP_ULONG w,
                            MMP_ULONG h,
                            MMP_UBYTE div_x,
                            MMP_UBYTE div_y);
MMP_ERR     MMPF_MD_Init(MMPF_MDTC_CLASS *md);
MMP_ERR     MMPF_MD_Open(MMPF_MDTC_CLASS *md);
MMP_ERR     MMPF_MD_Close(MMPF_MDTC_CLASS *md);
MMP_ERR     MMPF_MD_Update( MD_params_in_t *param);
/// @}

#endif  // _MMPF_MDTC_H_
