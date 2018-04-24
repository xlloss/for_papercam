//==============================================================================
//
//  File        : AudioProcess.h
//  Description : Audio Process function source code
//  Author      : ChenChu Hsu
//  Revision    : 1.0
//
//==============================================================================
#ifndef _AEC_H_
#define _AEC_H_

typedef struct AEC_run_s
{
	short* MIC_in_buffer;
	short* SPK_out_buffer;
	short  samples;
	short  AEC_Process_Samples;
} AEC_run_t;
#endif // #ifndef _AEC_H_
