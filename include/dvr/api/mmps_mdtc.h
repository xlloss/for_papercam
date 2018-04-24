/**
 @file mmps_mdtc.h
 @brief Header File for Modtion Detection Control
 @author Alterman
 @version 1.0
*/

#ifndef _MMPS_MDTC_H_
#define _MMPS_MDTC_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmpd_fctl.h"

#include "ipc_cfg.h"
#include "mmpf_mdtc.h"

/** @addtogroup MMPS_MDTC
@{
*/
#if (SUPPORT_MDTC)
//==============================================================================
//
//                               OPTIONS
//
//==============================================================================

#define MDTC_MEM_MAP_DBG        (0)

//==============================================================================
//
//                               ENUMERATION
//
//==============================================================================
/*
 * State
 *              open            start
 *   [ IDLE ] ------> [ OPEN ] ------> [ START ]
 *       ^              | ^                |
 *       |_____________ | |________________|
 *            close              stop
 */
typedef enum {
    MDTC_STATE_IDLE = 0,
    MDTC_STATE_OPEN,
    MDTC_STATE_START,
    MDTC_STATE_UNKNOWN
} E_MDTC_STATE;

//==============================================================================
//
//                               STRUCTURES
//
//==============================================================================

/*
 * Heap buffer
 */
typedef struct {
    MMP_ULONG   base;       ///< Base address of memory block
    MMP_ULONG   end;        ///< End address of memory block
    MMP_ULONG   size;       ///< Size of memory block
} ST_MDTC_HEAP;

/* Properties */
typedef struct {

    MMP_ULONG       luma_w;     ///< Luma frame width
    MMP_ULONG       luma_h;     ///< Luma frame height
    ST_MDTC_WIN     window;     ///< Detection window
    #if MD_USE_ROI_TYPE==0
    MD_params_in_t  param[MDTC_MAX_DIV_X][MDTC_MAX_DIV_Y];  ///< window param
    #else
    MD_params_in_t  param[MDTC_MAX_ROI];  ///< window param
    MD_block_info_t roi[MDTC_MAX_ROI] ;
    #endif
    MMP_UBYTE       snrID;      ///< Input sensor ID
} MMPS_MDTC_PROPT;

/*
 * Class
 */
typedef struct {
    /*
     * Public members
     */
    // state
    E_MDTC_STATE    state;      ///< state

    /*
     * Private members
     */
    // capability
    ST_MDTC_CAP     *cap;       ///< capability by ipc_cfg.c
    // sensor ID
    MMP_UBYTE       snrID;      ///< input sensor ID
    // pipe
    MMP_BOOL        pipe_en;    ///< is pipe activated
    MMPD_FCTL_LINK  pipe;       ///< pipe path used
    // heap buffer
    ST_MDTC_HEAP    heap;       ///< heap to allocate buffers
    // handshaking buffer Linux <-> cpu-b 
    ST_MDTC_HEAP    ipc_buf;
    // parnet class
    MMPF_MDTC_CLASS md;         ///< motion detection parnet class
} MMPS_MDTC_CLASS;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================

int    MMPS_MDTC_ModInit(void);
MMP_ERR MMPS_MDTC_Open(MMPS_MDTC_PROPT *feat, void (*md_change)(int new_val,int old_val) );
MMP_ERR MMPS_MDTC_Start(void);
MMP_ERR MMPS_MDTC_Stop(void);
MMP_ERR MMPS_MDTC_Close(void);
MMPS_MDTC_CLASS *MMPS_MDTC_Obj(void);

#endif //(SUPPORT_MDTC)
/// @}
#endif //_MMPS_MDTC_H_

