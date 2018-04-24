/**
 @file mmpf_alsa.h
 @brief Header function of Audio ALSA Driver
 @author Alterman
 @version 1.0
*/

#ifndef _MMPF_ALSA_H_
#define _MMPF_ALSA_H_

#include "includes_fw.h"
#include "mmpf_audio_ctl.h"
#include "mmp_aud_inc.h"
#include "mmp_ipc_inc.h"
#include "aitu_ringbuf.h"

/** @addtogroup MMPF_ALSA
@{
*/

//==============================================================================
//
//                              OPTIONS
//
//==============================================================================

#define SPK_CTRL            (1)
    #define SPK_CTRL_PIN    MMP_GPIO97
    #define SPK_CTRL_ON_LVL GPIO_HIGH

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

/*
 * Audio ALSA ID
 */
typedef enum {
    ALSA_ID_MIC_IN = 0,
    ALSA_ID_SPK_OUT,
    ALSA_ID_NUM
} MMPF_ALSA_ID;

/*
 * Audio ALSA State
 */
typedef enum {
    ALSA_STATE_IDLE = 0,
    ALSA_STATE_OPEN,
    ALSA_STATE_ON,
    ALSA_STATE_OFF,
    ALSA_STATE_NUM
} MMPF_ALSA_STATE;

/*
 * Audio ALSA Operation
 */
typedef enum {
    ALSA_OP_OPEN = 0,
    ALSA_OP_TRIGGER_ON,
    ALSA_OP_TRIGGER_OFF,
    ALSA_OP_CLOSE,
    ALSA_OP_NUM
} MMPF_ALSA_OP;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

/*
 * ALSA Messages
 */
typedef struct {
    MMPF_ALSA_ID    id;     ///< ALSA ID
    MMPF_ALSA_OP    op;     ///< Message operation
    void            *param; ///< Associated parameter
} MMPF_ALSA_MSG;

/*
 * ALSA Properties
 */
typedef struct {
    MMP_ULONG       fs;     ///< Sample rate
    MMP_ULONG       ch;     ///< Channel number
    MMP_ULONG       thr;    ///< Interrupt threshold
    MMP_ULONG       period; ///< ALSA period buffer size
    MMP_ULONG       ipc;    ///< IPC mode
    AUTL_RINGBUF    *buf;   ///< Ring buffer for PCM raw data
} MMPF_ALSA_PROPT;

/*
 * ALSA Class
 */
typedef struct {
    MMPF_ALSA_STATE state;  ///< status
    /* Properties */
    MMPF_ALSA_PROPT propt;  ///< properties
    /* Reference count */
    MMP_ULONG       refcnt; ///< Reference count
    /* ADC module */
    MMPF_ADC_CLASS  *adc;   ///< Associated ADC module
    /* DAC module */
    MMPF_DAC_CLASS  *dac;   ///< Associated DAC module
} MMPF_ALSA_CLASS;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void        MMPF_Alsa_Init(void);
MMP_ERR     MMPF_Alsa_TriggerOP(MMPF_ALSA_ID id, MMPF_ALSA_OP op, void *param);
MMP_ERR     MMPF_Alsa_Control(MMPF_ALSA_ID id, IPC_ALSA_CTL ctrl, void *param);
MMP_ULONG   MMPF_Alsa_Read(MMPF_ALSA_ID id, MMP_ULONG samples);
MMP_ERR     MMPF_Alsa_Write(MMPF_ALSA_ID id);

MMPF_ALSA_CLASS *MMPF_Alsa_Object(MMPF_ALSA_ID id);

/// @}

#endif  // _MMPF_ALSA_H_
