//==============================================================================
//
//  File        : mmpf_hif.c
//  Description : Firmware Host Interface Control Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"

/** @addtogroup MMPF_HIF
@{
*/

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static MMP_ULONG   m_ulStatus = 0;

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_ResetCmdStatus
//  Description : Set the status of command
//------------------------------------------------------------------------------
void MMPF_HIF_ResetCmdStatus(void)
{
	m_ulStatus = 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_SetCmdStatus
//  Description : Set the status of command, handshake between host and firmware
//------------------------------------------------------------------------------
void MMPF_HIF_SetCmdStatus(MMP_ULONG status)
{
	m_ulStatus |= status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_ClearCmdStatus
//  Description : Clear the status of command, handshake between host and firmware
//------------------------------------------------------------------------------
void MMPF_HIF_ClearCmdStatus(MMP_ULONG status)
{
	m_ulStatus &= ~status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_GetCmdStatus
//  Description : Get the status of command, handshake between host and firmware
//------------------------------------------------------------------------------
MMP_ULONG MMPF_HIF_GetCmdStatus(MMP_ULONG status)
{
	return (m_ulStatus & status);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Int_SetCpu2HostInt
//  Description : 
//------------------------------------------------------------------------------
/** @brief The function set the cpu interrupt to the host.

The function set the cpu interrupt to the host.

  @return It reports the status of the operation.
*/
void MMPF_HIF_SetCpu2HostInt(MMPF_HIF_INT_FLAG status) 
{
	// NOP
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_FeedbackParamL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void MMPF_HIF_FeedbackParamL(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_ULONG ulParamdata)
{
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);

    ParaBufPtr[(ubParamnum >> 2)] = ulParamdata;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_FeedbackParamW
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void MMPF_HIF_FeedbackParamW(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_USHORT usParamdata)
{
 	MMP_UBYTE index = (ubParamnum /4);
 	MMP_UBYTE shifter = (ubParamnum%4)*8;
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
   	ParaBufPtr[index] &= ~((MMP_ULONG)0xFFFF << shifter);     	//Clean the related position
	ParaBufPtr[index] |= ((MMP_ULONG)usParamdata << shifter); 	//Set the value to the related position  
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_FeedbackParamB
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void MMPF_HIF_FeedbackParamB(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_UBYTE ubParamdata)
{
	MMP_UBYTE index = (ubParamnum /4);
	MMP_UBYTE shifter = (ubParamnum%4)*8;
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
	ParaBufPtr[index] &= ~((MMP_ULONG)0xFF << shifter);     	//Clean the related position
	ParaBufPtr[index] |= ((MMP_ULONG)ubParamdata << shifter); 	//Set the value to the related position  
}

/** @} */ //end of MMPF_HIF