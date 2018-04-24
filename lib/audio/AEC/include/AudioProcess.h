//==============================================================================
//
//  File        : AudioProcess.h
//  Description : Audio Process function source code
//  Author      : ChenChu Hsu
//  Revision    : 1.0
//
//==============================================================================
#ifndef AUDIOPROCESS_H_
#define AUDIOPROCESS_H_
//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
#define CHECK_AUDIO_TIME_USAGE_INLIB_EN (0)
#define eqCOSTDOWN_EN					(1)
#define COSTDOWN_EN						(0)

#if CHECK_AUDIO_TIME_USAGE_INLIB_EN
#include "mmpf_typedef.h"
extern volatile unsigned int ulTimer[20];
extern int a;
#endif

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================
typedef struct {
	//unsigned int noise_floor;
	unsigned int point_number; // can only be 256
	unsigned int channel;
	int beamforming_en;  // only useful when channel == 2
} AudioProcessStruct;

//==============================================================================
//
//                              ENUMERATIONS
//
//==============================================================================
typedef enum {
	AP_DR_IN_FROM_INSIDE,
	AP_DR_IN_FROM_OUTSIDE
} AP_DR_IN_MODE;

typedef enum {
	NO_VAD,
	VAD_STAGE_1,
	VAD_STAGE_2,
	VAD_STAGE_3
} VAD_MODE;

typedef enum {
	AEC_LEVEL_0,
	AEC_LEVEL_1,
	AEC_LEVEL_2,
} AEC_MODE;

typedef enum {
	NR_LEVEL_0,
	NR_LEVEL_1,
	NR_LEVEL_2,
} NR_MODE;

typedef enum {
	EQ_OFF		   = 0,
	EQ_DEFAULT	   = -1,
	EQ_USER_DEFINE = 1
} EQ_MODE;

typedef enum {
	NR_SPEED_LOW,
	NR_SPEED_MID,
	NR_SPEED_HIGH
} NrConvergeSpeed;

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
unsigned int IaaApc_GetBufferSize(void);
int IaaApc_Init(char *workingBufferAddress,
				AudioProcessStruct *audio_process_struct);
int IaaApc_Run(short* pssAudioIn);
void IaaApc_Free(void);


int IaaApc_SetEqEnable(int enable, short *weight_x256);
int IaaApc_SetNrEnable(int enable);
int IaaApc_SetAgcEnable(int enable);
int IaaApc_SetNrSmoothLevel(unsigned int level);
int IaaApc_SetNrCostDownLevel(unsigned int level);
int IaaApc_SetNrConvergeSpeed(NrConvergeSpeed speed);
int IaaApc_SetNrMode(int mode);
int IaaApc_SetDereverbEnable(int enable);
int IaaApc_SetDereverbMode(AP_DR_IN_MODE in_mode);
void *IaaApc_GetDereverbBuffer(void);
int IaaApc_GetDereverbHalfBufferSize(void);
int IaaApc_GetSpeechProb(void);
int IaaApc_GetVadOut(void); // there are 2 stages
void IaaApc_SetVadMode(VAD_MODE mode);   // NO_VAD: no VAD
										 // VAD_STAGE_$: VAD stage number $
void IaaApc_SetVadThreshold(int level);
int IaaApc_BypassRightChannel(int enable);
int IaaApc_GetLibVersion(unsigned short *ver_year,
						 unsigned short *ver_date,
						 unsigned short *ver_time);
//For AGC
int IaaApc_AgcRun(short*);
int IaaApc_SetAgcTargetPowerIndB(int value); //dBFS, only input integer between -80 and 0
int IaaApc_GetAgcGain(void);
void IaaApc_SetAgcNoiseGateEnable(int enable, int dB);

#endif // #ifndef _AUDIOPROCESS_H_
