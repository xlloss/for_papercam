/// @ait_only
//==============================================================================
//
//  File        : mmpd_fctl.h
//  Description : INCLUDE File for the Host Flow Control Driver.
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

/**
 *  @file mmpd_fctl.h
 *  @brief The header File for the Host Flow Control Driver
 *  @author Penguin Torng
 *  @version 1.0
 */

#ifndef _MMPD_FCTL_H_
#define _MMPD_FCTL_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"

#include "mmpd_ibc.h"
#include "mmpd_icon.h"
#include "mmpd_graphics.h"

#include "mmp_display_inc.h"

/** @addtogroup MMPD_FCtl
 *  @{
 */

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define MAX_PIPELINE_NUM    	(5)

#define MAX_FCTL_BUF_NUM		(4)
#define MAX_FCTL_ROT_BUF_NUM	(2)

#define FCTL_PIPE_TO_LINK(p, flink)	{	flink.scalerpath = (MMP_SCAL_PIPEID) p; \
										flink.icopipeID	 = (MMP_ICO_PIPEID)p; \
										flink.ibcpipeID  = (MMP_IBC_PIPEID)p; \
									}

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
/*
 * Pipe Link Destination
 */
typedef enum {
    PIPE_LINK_H264  = 0x01, ///< Pipe link to H.264 engine
    PIPE_LINK_JPG   = 0x02, ///< Pipe link to JPG engine
    PIPE_LINK_FB    = 0x04, ///< Pipe link to frame buffer
    PIPE_LINK_FB_Y  = 0x08, ///< Pipe link to Y frame buffer
    PIPE_LINK_ALL   = 0x0F
} MMPD_FCTL_LINK_TO;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef struct  _MMPD_FCTL_LINK
{
    MMP_SCAL_PIPEID    		scalerpath;
   	MMP_ICO_PIPEID     		icopipeID;
    MMP_IBC_PIPEID     		ibcpipeID;
} MMPD_FCTL_LINK;

typedef struct _MMPD_FCTL_ATTR 
{
    MMP_DISPLAY_COLORMODE	colormode;
    MMPD_FCTL_LINK			fctllink;
	MMP_SCAL_FIT_RANGE 		fitrange;
	MMP_SCAL_GRAB_CTRL 		grabctl;
	MMP_SCAL_SOURCE			scalsrc;
	MMP_BOOL				bSetScalerSrc;
	MMP_UBYTE				ubPipeLinkedSnr;
	
    MMP_USHORT          	usBufCnt;
    MMP_ULONG           	ulBaseAddr[MAX_FCTL_BUF_NUM];
    MMP_ULONG           	ulBaseUAddr[MAX_FCTL_BUF_NUM];
    MMP_ULONG           	ulBaseVAddr[MAX_FCTL_BUF_NUM];
        
    MMP_BOOL            	bUseRotateDMA;      					// Use rotate DMA to rotate or not
    MMP_ULONG           	ulRotateAddr[MAX_FCTL_ROT_BUF_NUM];    	// Dest Y buffer address for rotate DMA
    MMP_ULONG           	ulRotateUAddr[MAX_FCTL_ROT_BUF_NUM];   	// Dest U buffer address for rotate DMA
    MMP_ULONG           	ulRotateVAddr[MAX_FCTL_ROT_BUF_NUM];   	// Dest V buffer address for rotate DMA
    MMP_USHORT          	usRotateBufCnt;     					// Dest buffer count for rotate DMA
} MMPD_FCTL_ATTR;

/*
 * Pipe Class
 */
typedef struct {
    /* Class public members */
    MMP_UBYTE           id;     ///< Pipe ID, mapping to physical HW pipe num
    MMP_BOOL            used;   ///< Is pipe used?
    MMP_USHORT          max_w;  ///< The max. supported width by the pipe
    MMPD_FCTL_LINK_TO   link2;  ///< Supported link destinations by the pipe
} MMPD_FCTL_PIPE;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ERR MMPD_Fctl_SetPipeAttrForIbcFB(MMPD_FCTL_ATTR *pAttr);
MMP_ERR MMPD_Fctl_SetPipeAttrForH264FB(MMPD_FCTL_ATTR *pAttr);
MMP_ERR MMPD_Fctl_SetPipeAttrForH264Rt( MMPD_FCTL_ATTR *pAttr,
                                        MMP_ULONG       ulEncWidth);
MMP_ERR MMPD_Fctl_SetPipeAttrForJpeg(   MMPD_FCTL_ATTR  *pAttr,
                                        MMP_BOOL        bSetScalerGrab,
                                        MMP_BOOL        bSrcIsBT601);
MMP_ERR MMPD_Fctl_SetSubPipeAttr(MMPD_FCTL_ATTR *pAttr);

MMP_ERR MMPD_Fctl_GetAttributes(MMP_IBC_PIPEID pipeID, MMPD_FCTL_ATTR *pAttr);
MMP_ERR MMPD_Fctl_EnablePreview(MMP_UBYTE       ubSnrSel, 
                                MMP_IBC_PIPEID  pipeID,
                                MMP_BOOL        bEnable,
                                MMP_BOOL        bCheckFrameEnd);
MMP_ERR MMPD_Fctl_EnablePipe(   MMP_UBYTE       ubSnrSel,
                                MMP_IBC_PIPEID  pipeID,
                                MMP_BOOL        bEnable);

/* IBC Link Type */
MMP_ERR MMPD_Fctl_GetIBCLinkAttr(MMP_IBC_PIPEID 	        pipeID, 
								 MMP_IBC_LINK_TYPE 		    *IBCLinkType,
								 MMP_DISPLAY_DEV_TYPE	    *previewdev,
								 MMP_DISPLAY_WIN_ID		    *winID,
								 MMP_DISPLAY_ROTATE_TYPE    *rotateDir);
MMP_ERR MMPD_Fctl_RestoreIBCLinkAttr(MMP_IBC_PIPEID 		    pipeID, 
									 MMP_IBC_LINK_TYPE 		    IBCLinkType,
									 MMP_DISPLAY_DEV_TYPE	    previewdev,
									 MMP_DISPLAY_WIN_ID		    winID,
									 MMP_DISPLAY_ROTATE_TYPE    rotateDir);

MMP_ERR MMPD_Fctl_ResetIBCLinkType(MMP_IBC_PIPEID pipeID);
MMP_ERR MMPD_Fctl_LinkPipeToGraphic(MMP_IBC_PIPEID pipeID);
MMP_ERR MMPD_Fctl_LinkPipeToVideo(MMP_IBC_PIPEID pipeID, MMP_USHORT ubEncId);
MMP_ERR MMPD_Fctl_LinkPipeToWithId(MMP_IBC_PIPEID pipeID, MMP_ULONG id,MMP_ULONG module);
MMP_ERR MMPD_Fctl_LinkPipeTo(MMP_IBC_PIPEID pipeID, MMP_ULONG module);

MMP_IBC_PIPEID MMPD_Fctl_AllocatePipe(MMP_ULONG width, MMPD_FCTL_LINK_TO dst);
MMP_ERR MMPD_Fctl_ReleasePipe(MMP_IBC_PIPEID id);

/// @}

#endif // _MMPD_FCTL_H_

/// @end_ait_only
