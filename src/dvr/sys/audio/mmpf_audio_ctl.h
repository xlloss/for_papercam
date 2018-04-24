/**
 @file mmpf_audio_ctl.h
 @brief Header function of Audio Driver
 @author Alterman
 @version 1.0
*/
#ifndef MMPF_AUDIO_CTL_H
#define MMPF_AUDIO_CTL_H

#include "config_fw.h"
#include "mmp_aud_inc.h"
#include "aitu_ringbuf.h"

/** @addtogroup MMPF_AUDIO
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define MAX_ADC_MODULE_NUM      (1)     ///< Only internal ADC supported
#define MAX_DAC_MODULE_NUM      (1)     ///< Only internal DAC supported

#define ADC_FIFO_DEPTH          (512)
// Size of ring buffer for ADC FIFO input
#define ADC_FIFO_BUF_SIZE       (3072)  // LCM of 192 & 256
// Silent the beginning samples in ADC power on to avoid noise
#define ADC_INITIAL_SILENCE     (550)   // Set initial 550ms samples to silence

#define DAC_FIFO_DEPTH          (512)
// Size of ring buffer for DAC FIFO input
#define DAC_FIFO_BUF_SIZE       (3072)  // LCM of 192 & 256

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================
/*
 * Audio data route
 */
typedef enum _MMPF_AUDIO_DATA_FLOW
{
    ADC_TO_AFE_RX_FIFO = 0, ///< ADC input to Rx FIFO
    AFE_TX_FIFO_TO_DAC,     ///< DAC output to Tx FIFO
    MMPF_AUDIO_MAX_FLOW
} MMPF_AUDIO_DATA_FLOW;

/*
 * ADC Module Status
 */
typedef enum {
    ADC_STATE_IDLE = 0,
    ADC_STATE_OPEN,
    ADC_STATE_ON,
    ADC_STATE_OFF,
    ADC_STATE_INVALID
} MMPF_ADC_STATE;

/*
 * DAC Module Status
 */
typedef enum {
    DAC_STATE_IDLE = 0,
    DAC_STATE_OPEN,
    DAC_STATE_ON,
    DAC_STATE_OFF,
    DAC_STATE_INVALID
} MMPF_DAC_STATE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

/*
 * AStream ADC Module
 */
typedef struct {
    MMPF_ADC_STATE          state;      ///< ADC state
    MMP_AUD_INOUT_PATH      path;       ///< ADC path
    MMP_AUD_LINEIN_CH       in_ch;      ///< Input channel (L/R/LR)
    MMP_ULONG               fs;         ///< Sample rate
    MMP_ULONG               refcnt;     ///< Reference count
} MMPF_ADC_CLASS;

/*
 * AStream DAC Module
 */
typedef struct {
    MMPF_DAC_STATE          state;      ///< DAC state
    MMP_AUD_INOUT_PATH      path;       ///< DAC path
    MMP_ULONG               fs;         ///< Sample rate
    MMP_ULONG               refcnt;     ///< Reference count
    MMP_ULONG               period;     ///< Period buffer size
    AUTL_RINGBUF            *buf;       ///< Output buffer handler
    MMP_BOOL                ready;      ///< Data ready
} MMPF_DAC_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void        MMPF_ADC_Init(void);
MMPF_ADC_CLASS *MMPF_ADC_InitModule(MMP_AUD_INOUT_PATH path, MMP_ULONG fs);
MMP_ERR     MMPF_ADC_UninitModule(MMPF_ADC_CLASS *mod);
MMP_ERR     MMPF_ADC_OpenModule(MMPF_ADC_CLASS *mod);
MMP_ERR     MMPF_ADC_StartModule(MMPF_ADC_CLASS *mod);
MMP_ERR     MMPF_ADC_StopModule(MMPF_ADC_CLASS *mod);
MMP_ERR     MMPF_ADC_CloseModule(MMPF_ADC_CLASS *mod);
void        MMPF_ADC_SetInitCh(MMP_AUD_LINEIN_CH ch);
MMP_AUD_LINEIN_CH        MMPF_ADC_GetInitCh(void );


void        MMPF_DAC_Init(void);
MMPF_DAC_CLASS *MMPF_DAC_InitModule(MMP_AUD_INOUT_PATH path, MMP_ULONG fs);
MMP_ERR     MMPF_DAC_UninitModule(MMPF_DAC_CLASS *mod);
MMP_ERR     MMPF_DAC_UninitModule(MMPF_DAC_CLASS *mod);
MMP_ERR     MMPF_DAC_OpenModule(MMPF_DAC_CLASS *mod);
MMP_ERR     MMPF_DAC_StartModule(MMPF_DAC_CLASS *mod, MMP_ULONG period);
MMP_ERR     MMPF_DAC_StopModule(MMPF_DAC_CLASS *mod);
MMP_ERR     MMPF_DAC_CloseModule(MMPF_DAC_CLASS *mod);

void        MMPF_Audio_CtrlClock(MMP_AUD_CLK clktype, MMP_BOOL enable);
MMP_ULONG   MMPF_Audio_FifoIn(void);
MMP_ULONG   MMPF_Audio_FifoInSamples(void);
MMP_ULONG   MMPF_Audio_FifoInRead(  MMP_SHORT   *buf_addr,
                                    MMP_ULONG   samples,
                                    MMP_ULONG   ch,
                                    MMP_ULONG   ofst);
MMP_ERR     MMPF_Audio_FifoInCommitRead(MMP_ULONG samples);

MMP_ERR     MMPF_Audio_SetADCMute(MMP_BOOL bMute, MMP_BOOL bSaveSettingOnly);
MMP_ERR     MMPF_Audio_SetADCDigitalGain(MMP_UBYTE  gain,
                                        MMP_BOOL    bSaveSettingOnly);
MMP_ERR     MMPF_Audio_SetADCAnalogGain(MMP_UBYTE   gain, 
                                        MMP_BOOL    bSaveSettingOnly);

MMP_ERR     MMPF_Audio_SetDACMute(MMP_BOOL bMute, MMP_BOOL bSaveSettingOnly);
MMP_ERR     MMPF_Audio_SetDACDigitalGain(MMP_UBYTE  gain,
                                        MMP_BOOL    bSaveSettingOnly);
MMP_ERR     MMPF_Audio_SetDACAnalogGain(MMP_UBYTE   gain,
                                        MMP_BOOL    bSaveSettingOnly);

void        MMPF_Audio_SetInputCh(MMP_AUD_LINEIN_CH ch);
MMP_ERR     MMPF_Audio_SetInputPath(MMP_AUD_INOUT_PATH path);
MMP_ERR     MMPF_Audio_SetOutputPath(MMP_AUD_INOUT_PATH path);
MMP_ERR     MMPF_Audio_SetPLL(MMP_AUD_INOUT_PATH path, MMP_ULONG samplerate);

/// @}

#endif // MMPF_AUDIO_CTL_H
