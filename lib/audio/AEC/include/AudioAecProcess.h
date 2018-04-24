
//==============================================================================
//
//  File        : AudioAecProcess.h
//  Description : Aduio AEC Process function source code
//  Author      : ChenChu Hsu
//  Revision    : 1.0
//
//==============================================================================
#ifndef _AUDIOAECPROCESS_H_
#define _AUDIOAECPROCESS_H_

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================


//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
typedef struct
{
	//unsigned int noise_floor;
	unsigned int point_number; // can only be 256
	unsigned int nearend_channel;
	unsigned int farend_channel;
}
AudioAecProcessStruct;


//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
unsigned int IaaAec_GetBufferSize(void);
int IaaAec_Init(char * workingBufferAddress, AudioAecProcessStruct * audio_process_struct);
int IaaAec_Run(short* pssAduioNearEndIn, short* pssAduioFarEndIn);
int IaaAec_Free(void);
int IaaAec_SetGenDRInputBufEn(int gen_en);
int IaaAec_SetDRInputBufAddress(int* address_in, int size_in);
int IaaAec_GetLibVersion(unsigned short *ver_year,
						 unsigned short *ver_date,
						 unsigned short *ver_time);
int IaaAec_SetMode(int mode); //0~3

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
int FuncAudioAecProcess_Init(char * workingBufferAddress, AudioAecProcessStruct * audio_process_struct);
int FuncAudioAecProcess_DoOneFrame(short* pssAduioNearEndIn, short* pssAduioFarEndIn);
int FuncAudioAecProcess_Free(void);
int FuncAudioAecProcess_SetGenDRInputBufEn(int gen_en);
int FuncAudioAecProcess_SetDRInputBufAddress(int* address_in, int size_in);

int FuncAudioAecProcess_GetLibVersion(unsigned short* ver_year, unsigned short* ver_date, unsigned short* ver_time);


#endif // #ifndef _AUDIOAECPROCESS_H_
