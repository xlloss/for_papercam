//==============================================================================
//
//  File        : mmpf_display.h
//  Description : INCLUDE File for the Firmware Display Control driver function, including LCD/TV/Win
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_DISPLAY_H_
#define _MMPF_DISPLAY_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_display_inc.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define DISP_START_PREVIEW_SEM_TIMEOUT  (3000)
#define DISP_STOP_PREVIEW_SEM_TIMEOUT   (3000)

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

// Basic Function
MMP_UBYTE MMPF_Display_GetActivePipeNum(MMP_UBYTE ubSnrSel);
MMP_ERR MMPF_Display_GetActivePipeId(MMP_UBYTE ubSnrSel, MMP_UBYTE *pPipe, MMP_UBYTE ubSearchNum);

MMP_ERR MMPF_Display_StartSoftwareRefresh(MMP_USHORT usIBCPipe);
MMP_ERR MMPF_Display_StartPreview(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe, MMP_BOOL bCheckFrameEnd);
MMP_ERR MMPF_Display_StopPreview(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe, MMP_BOOL bCheckFrameEnd);
MMP_ERR MMPF_Display_GetPreviewSts(MMP_UBYTE *pubStatus);
MMP_ERR MMPF_Display_FrameDoneTrigger(MMP_USHORT usIBCPipe);
MMP_ERR MMPF_Display_GetExposedFrameBuffer(MMP_USHORT usIBCPipe, MMP_ULONG *pulYAddr, MMP_ULONG *pulUAddr, MMP_ULONG *pulVAddr);
MMP_ERR MMPF_Display_GetCurIbcFrameBuffer(MMP_USHORT usIBCPipe, MMP_ULONG *pulYAddr, MMP_ULONG *pulUAddr, MMP_ULONG *pulVAddr);

MMP_ERR MMPF_Display_EnablePipe(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe);
MMP_ERR MMPF_Display_DisablePipe(MMP_UBYTE ubSnrSel, MMP_USHORT usIBCPipe);

#endif // _MMPF_DISPLAY_H_
