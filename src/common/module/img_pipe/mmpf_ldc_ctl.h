//==============================================================================
//
//  File        : mmpf_ldc_ctl.h
//  Description : INCLUDE File for the LDC Driver Function
//  Author      : Eroy Yang
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_LDC_CTL_H_
#define _MMPF_LDC_CTL_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define MMPF_LDC_MAX_QUEUE_SIZE   	(50)

#define LDC_DONE_SEM_TIMEOUT   		(90)

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

typedef struct _MMPF_LDC_CTL_ATTR 
{
    MMP_GRAPHICS_BUF_ATTR   sGraBufAttr;            ///< Graphics Buffer Attribute

    MMP_UBYTE               ubSensorId;       		///< Sensor ID
    MMP_UBYTE				ubSourcePipe;			///< Source Pipe ID
	MMP_UBYTE               ubPreviewPipe;      	///< Preview Pipe ID
	MMP_UBYTE               ubEncodePipe;       	///< Encode Pipe ID
	MMP_UBYTE               ubCapturePipe;       	///< Capture Pipe ID
} MMPF_LDC_CTL_ATTR;

typedef struct _MMPF_LDC_CTL_QUEUE 
{
    MMPF_LDC_CTL_ATTR       attr[MMPF_LDC_MAX_QUEUE_SIZE];  	///< Queue for ready to encode/decode
    MMP_ULONG               weight[MMPF_LDC_MAX_QUEUE_SIZE];   	///< The times to encode the same frame
    MMP_ULONG               head;                               ///< Queue head index
    MMP_ULONG               size;                               ///< Queue size
    MMP_ULONG               weighted_size;
} MMPF_LDC_CTL_QUEUE;

//===============================================================================
//
//                               EXTERN VARIABLES
//
//===============================================================================

extern MMPF_LDC_CTL_QUEUE m_sLdcCtlQueue;

//===============================================================================
//
//                               FUNCTION PROTOTYPES
//
//===============================================================================

void        MMPF_LDCCTL_ResetQueue(MMPF_LDC_CTL_QUEUE *queue);
MMP_ULONG   MMPF_LDCCTL_GetQueueDepth(MMPF_LDC_CTL_QUEUE *queue, MMP_BOOL weighted);
MMP_ERR     MMPF_LDCCTL_PushQueue(MMPF_LDC_CTL_QUEUE *queue, MMPF_LDC_CTL_ATTR attr, MMP_BOOL weighted);
MMP_ERR     MMPF_LDCCTL_PopQueue(MMPF_LDC_CTL_QUEUE *queue, MMP_ULONG offset, MMPF_LDC_CTL_ATTR *pData, MMP_BOOL weighted);

#endif // _MMPF_LDC_CTL_H_