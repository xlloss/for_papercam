/**
 @file mmpf_audio_ctl.c
 @brief Control functions of Audio Driver
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"

#include "mmp_reg_audio.h"

#include "mmpf_pll.h"
#include "mmpf_system.h"
#include "mmpf_audio_ctl.h"
#include "mmpf_alsa.h"
#if (SUPPORT_AEC)
#include "mmpf_aec.h"
#endif

#include "mmu.h"
#include "ipc_cfg.h"
#include "aitu_ringbuf.h"

/** @addtogroup MMPF_AUDIO
@{
*/
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
/* ADC module related */
static MMP_AUD_INOUT_PATH   m_ulAdcInPath;  ///< Input ADC path
static MMP_AUD_LINEIN_CH    m_AdcInCh;      ///< Input channel(s)

static MMP_UBYTE            m_bAdcDigGain = DEFAULT_ADC_DIGITAL_GAIN;
static MMP_UBYTE            m_bAdcAnaGain = DEFAULT_ADC_ANALOG_GAIN;
static MMP_BOOL             m_bAdcMute = MMP_FALSE;

static MMP_BOOL             m_bAdcOn = MMP_FALSE;
static MMP_BOOL             m_bAdcFastPowerOn = MMP_TRUE;

static MMPF_ADC_CLASS       m_AdcModule[MAX_ADC_MODULE_NUM];

#if (ADC_INITIAL_SILENCE != 0)
static MMP_ULONG            m_ulAdcInitSilence = ADC_INITIAL_SILENCE * 48;
#endif

///< Ring buf for PCM input
static AUTL_RINGBUF         m_AdcInRingBuf;

/* #pragma arm section zidata = "audiofifobuf" */
static MMP_SHORT            m_AdcInBuf[ADC_FIFO_BUF_SIZE] __attribute__((section(".audiofifobuf"))) ;
/* #pragma arm section zidata */

/* DAC module related */
static MMP_AUD_INOUT_PATH   m_ulDacOutPath; ///< DAC output path

static MMP_UBYTE            m_bDacDigGain = DEFAULT_DAC_DIGITAL_GAIN;
static MMP_UBYTE            m_bDacAnaGain = DEFAULT_DAC_ANALOG_GAIN;
static MMP_BOOL             m_bDacMute = MMP_FALSE;

static MMP_BOOL             m_bDacOn = MMP_FALSE;
static MMP_BOOL             m_bDacFastPowerOn = MMP_TRUE;

static MMPF_DAC_CLASS       m_DacModule[MAX_DAC_MODULE_NUM];

static AUTL_RINGBUF         *m_DacOutRingBuf = NULL;

/*
 * External Variables
 */
extern MMPF_OS_FLAGID       AUD_REC_Flag;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPF_Audio_SetDACFreq(MMP_ULONG samplerate);

static MMP_ERR MMPF_Audio_OpenCapture(MMP_USHORT threshold);
static MMP_ERR MMPF_Audio_EnableCapture(void);
static MMP_ERR MMPF_Audio_DisableCapture(void);
static MMP_ERR MMPF_Audio_CloseCapture(void);

static MMP_ERR MMPF_Audio_OpenRender(MMP_USHORT threshold);
static MMP_ERR MMPF_Audio_EnableRender(void);
static MMP_ERR MMPF_Audio_DisableRender(void);
static MMP_ERR MMPF_Audio_CloseRender(void);

static MMP_ERR MMPF_Audio_EnableMux(MMPF_AUDIO_DATA_FLOW path, MMP_BOOL enable);
static MMP_ERR MMPF_Audio_PowerOnADC(void);
static MMP_ERR MMPF_Audio_PowerOffADC(void);
static MMP_ERR MMPF_Audio_PowerOnDAC(void);
static MMP_ERR MMPF_Audio_PowerOffDAC(void);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize the ADC modules. Once after power on.

 @retval None.
*/
void MMPF_ADC_Init(void)
{
    MMP_ULONG i;
    AITPS_AIC pAIC = AITC_BASE_AIC;

    /* Now only internal ADC supported */
    m_AdcModule[0].state    = ADC_STATE_IDLE;
    m_AdcModule[0].refcnt   = 0;
    m_AdcModule[0].fs       = 0;
    m_AdcModule[0].in_ch    = gADCInCh;
    m_AdcModule[0].path     = gADCInPath;

    for(i = 1; i < MAX_ADC_MODULE_NUM; i++) {
        m_AdcModule[i].state    = ADC_STATE_IDLE;
        m_AdcModule[i].refcnt   = 0;
        m_AdcModule[i].fs       = 0;
        m_AdcModule[i].in_ch    = MMP_AUD_LINEIN_DUAL;
        m_AdcModule[i].path     = MMP_AUD_IN_PATH_NONE;
    }

    RTNA_AIC_Open(  pAIC, AIC_SRC_AFE_FIFO, afe_isr_a,
                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_AFE_FIFO);

    /* Workaround: Linux will access audio OPR before power on it */
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ADC, MMP_TRUE);
}

void MMPF_ADC_SetInitCh(MMP_AUD_LINEIN_CH ch)
{
  gADCInCh = ch ;
  m_AdcModule[0].in_ch = gADCInCh ;
}

MMP_AUD_LINEIN_CH        MMPF_ADC_GetInitCh(void )
{
  return gADCInCh ;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_InitModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize the ADC module

 @param[in] path    ADC path
 @param[in] fs      Sample rate

 @retval It reports the module of ADC
*/
MMPF_ADC_CLASS *MMPF_ADC_InitModule(MMP_AUD_INOUT_PATH path, MMP_ULONG fs)
{
    MMP_ULONG m = 0;
    MMPF_ADC_CLASS *mod = NULL;

    for(m = 0; m < MAX_ADC_MODULE_NUM; m++) {
        if (m_AdcModule[m].path == path) {
            if (m_AdcModule[m].refcnt == 0)
                m_AdcModule[m].fs = fs;
#if SRC_SUPPORT
            if(( mod->refcnt > 0) && (m_AdcModule[m].fs==8000 ) ) {
                mod = &m_AdcModule[m];
                mod->refcnt++;                
            }
#endif
            if (m_AdcModule[m].fs == fs) {
                mod = &m_AdcModule[m];
                mod->refcnt++;
            }
        }
    }

    if (!mod) {
        RTNA_DBG_Str(0, "Can't find ADC module\r\n");
        return NULL;
    }

    return mod;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_UninitModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release the ADC module

 @param[in] mod     ADC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_ADC_UninitModule(MMPF_ADC_CLASS *mod)
{
    if (!mod)
        return MMP_ASTREAM_ERR_PARAMETER;
    if (mod->refcnt == 0)
        return MMP_ASTREAM_ERR_OPERATION;

    mod->refcnt--;

    if (!mod) {
        RTNA_DBG_Str(0, "Can't find ADC module\r\n");
        return 0;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_OpenModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Power on the ADC module

 @param[in] mod     ADC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_ADC_OpenModule(MMPF_ADC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_USHORT thr; // FIFO interrupt threshold

    if (mod->state != ADC_STATE_IDLE)
        return MMP_ERR_NONE;

    if (mod->path & MMP_AUD_AFE_IN_MASK) {
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_TRUE);
   	    MMPF_SYS_EnableClock(MMPF_SYS_CLK_ADC, MMP_TRUE);
        err = MMPF_Audio_SetPLL(MMP_AUD_AFE_IN, mod->fs);
        if (err) return err;

        MMPF_Audio_SetInputPath(mod->path);
        MMPF_Audio_SetInputCh(mod->in_ch);

        if (mod->fs >= 32000)
            thr = (2 * mod->fs) / 250; // interrupt of 4ms interval
        else
            thr = (2 * mod->fs) / 125; // interrupt of 8ms interval

        #if (ADC_INITIAL_SILENCE != 0)
        if (m_ulAdcInitSilence != 0)
            m_ulAdcInitSilence = (ADC_INITIAL_SILENCE * mod->fs * mod->in_ch) /
                                                                        1000;
        #endif
        err = MMPF_Audio_OpenCapture(thr);
        if (err) return err;
    }
    else {
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    mod->state = ADC_STATE_OPEN;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_StartModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Enable the ADC module interrupt

 @param[in] mod     ADC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_ADC_StartModule(MMPF_ADC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;

    if (mod->state != ADC_STATE_OPEN)
        return MMP_ERR_NONE;

    if (mod->path & MMP_AUD_AFE_IN_MASK) {
        err = MMPF_Audio_EnableCapture();
        if (err) return err;
    }
    else {
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    mod->state = ADC_STATE_ON;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_StopModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Disable the ADC module interrupt

 @param[in] mod     ADC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_ADC_StopModule(MMPF_ADC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;

    if (mod->state != ADC_STATE_ON)
        return MMP_ERR_NONE;

    if (mod->refcnt == 1) {
        if (mod->path & MMP_AUD_AFE_IN_MASK) {
            err = MMPF_Audio_DisableCapture();
            if (err) return err;
        }
        else {
            return MMP_ASTREAM_ERR_UNSUPPORTED;
        }

        mod->state = ADC_STATE_OFF;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_ADC_CloseModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Power down the ADC module

 @param[in] mod     ADC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_ADC_CloseModule(MMPF_ADC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;

    if ((mod->state != ADC_STATE_OFF) && (mod->state != ADC_STATE_OPEN))
        return MMP_ERR_NONE;

    if (mod->refcnt == 1) {
        if (mod->path & MMP_AUD_AFE_IN_MASK) {
            err = MMPF_Audio_CloseCapture();
            if (err) return err;

            MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_FALSE);
   	        MMPF_SYS_EnableClock(MMPF_SYS_CLK_ADC, MMP_FALSE);
        }
        else {
            return MMP_ASTREAM_ERR_UNSUPPORTED;
        }

        mod->state = ADC_STATE_IDLE;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize the DAC modules. Once after power on.

 @retval None.
*/
void MMPF_DAC_Init(void)
{
    MMP_ULONG i;
    AITPS_AIC pAIC = AITC_BASE_AIC;

    /* Now only internal DAC supported */
    m_DacModule[0].state    = DAC_STATE_IDLE;
    m_DacModule[0].refcnt   = 0;
    m_DacModule[0].fs       = 0;
    m_DacModule[0].path     = gDACOutPath;
    m_DacModule[0].ready    = MMP_FALSE;

    for(i = 1; i < MAX_DAC_MODULE_NUM; i++) {
        m_DacModule[i].state    = DAC_STATE_IDLE;
        m_DacModule[i].refcnt   = 0;
        m_DacModule[i].fs       = 0;
        m_DacModule[i].path     = MMP_AUD_OUT_PATH_NONE;
        m_DacModule[i].ready    = MMP_FALSE;
    }

    RTNA_AIC_Open(  pAIC, AIC_SRC_AFE_FIFO, afe_isr_a,
                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_AFE_FIFO);

    /* Workaround: Linux will access audio OPR before power on it */
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_TRUE);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_DAC, MMP_TRUE);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_InitModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize the DAC module

 @param[in] path    DAC path
 @param[in] fs      Sample rate

 @retval It reports the module of DAC
*/
MMPF_DAC_CLASS *MMPF_DAC_InitModule(MMP_AUD_INOUT_PATH path, MMP_ULONG fs)
{
    MMP_ULONG m = 0;
    MMPF_DAC_CLASS *mod = NULL;

    for(m = 0; m < MAX_DAC_MODULE_NUM; m++) {
        if (m_DacModule[m].path == path) {
            if (m_DacModule[m].refcnt == 0) {
                m_DacModule[m].fs = fs;
                m_DacModule[m].ready = MMP_FALSE;
            }

            if (m_DacModule[m].fs == fs) {
                mod = &m_DacModule[m];
                mod->refcnt++;
            }
        }
    }

    if (!mod) {
        RTNA_DBG_Str(0, "Can't find DAC module\r\n");
        return NULL;
    }

    return mod;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_UninitModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release the DAC module

 @param[in] mod     DAC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_DAC_UninitModule(MMPF_DAC_CLASS *mod)
{
    if (!mod)
        return MMP_ASTREAM_ERR_PARAMETER;
    if (mod->refcnt == 0)
        return MMP_ASTREAM_ERR_OPERATION;

    mod->refcnt--;

    if (!mod) {
        RTNA_DBG_Str(0, "Can't find DAC module\r\n");
        return 0;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_OpenModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Power on the DAC module

 @param[in] mod     DAC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_DAC_OpenModule(MMPF_DAC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_USHORT thr; // FIFO interrupt threshold

    if (mod->state != DAC_STATE_IDLE)
        return MMP_ERR_NONE;

    if (mod->path & MMP_AUD_AFE_OUT_MASK) {
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_TRUE);
   	    MMPF_SYS_EnableClock(MMPF_SYS_CLK_DAC, MMP_TRUE);

        err = MMPF_Audio_SetPLL(MMP_AUD_AFE_OUT, mod->fs);
        if (err) return err;

        m_DacOutRingBuf = mod->buf;
        MMPF_Audio_SetOutputPath(mod->path);
        MMPF_Audio_SetDACFreq(mod->fs);

        if (mod->fs >= 32000)
            thr = (2 * mod->fs) / 250; // interrupt of 4ms interval
        else
            thr = (2 * mod->fs) / 125; // interrupt of 8ms interval

        err = MMPF_Audio_OpenRender(thr);
        if (err) return err;
    }
    else {
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    mod->state = DAC_STATE_OPEN;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_StartModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Enable the DAC module interrupt

 @param[in] mod     DAC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_DAC_StartModule(MMPF_DAC_CLASS *mod, MMP_ULONG period)
{
    MMP_ERR err = MMP_ERR_NONE;

    if ((mod->state != DAC_STATE_OPEN) && ( mod->state != DAC_STATE_OFF) )
        return MMP_ERR_NONE;

    if (mod->path & MMP_AUD_AFE_OUT_MASK) {
        mod->period = period; // in unit of sample
        err = MMPF_Audio_EnableRender();
        if (err) return err;
    }
    else {
        return MMP_ASTREAM_ERR_UNSUPPORTED;
    }

    mod->state = DAC_STATE_ON;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_StopModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Disable the DAC module interrupt

 @param[in] mod     DAC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_DAC_StopModule(MMPF_DAC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;

    if (mod->state != DAC_STATE_ON)
        return MMP_ERR_NONE;

    if (mod->refcnt == 1) {
        if (mod->path & MMP_AUD_AFE_OUT_MASK) {
            err = MMPF_Audio_DisableRender();
            if (err) return err;
        }
        else {
            return MMP_ASTREAM_ERR_UNSUPPORTED;
        }

        mod->state = DAC_STATE_OFF;
        mod->ready = MMP_FALSE;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_DAC_CloseModule
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Power down the DAC module

 @param[in] mod     DAC module

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_DAC_CloseModule(MMPF_DAC_CLASS *mod)
{
    MMP_ERR err = MMP_ERR_NONE;

    if ((mod->state != DAC_STATE_OFF) && (mod->state != DAC_STATE_OPEN))
        return MMP_ERR_NONE;

    if (mod->refcnt == 1) {
        if (mod->path & MMP_AUD_AFE_OUT_MASK) {
            err = MMPF_Audio_CloseRender();
            if (err) return err;

            MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, MMP_FALSE);
   	        MMPF_SYS_EnableClock(MMPF_SYS_CLK_DAC, MMP_FALSE);
        }
        else {
            return MMP_ASTREAM_ERR_UNSUPPORTED;
        }

        mod->state = DAC_STATE_IDLE;
        m_DacOutRingBuf = NULL;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_ReadFifo
//  Description : Read PCM data from audio Rx FIFO
//------------------------------------------------------------------------------
/**
 @brief Read raw PCM data from audio engine Rx FIFO

 @param[in] fifo    FIFO port of audio HW engine
 @param[in] buf     Buffer to store the PCM data
 @param[in] amount  Sample count to read from Rx FIFO
 @param[in] stereo  Save PCM as stereo or mono

 @retval None.
*/
static void MMPF_Audio_ReadFifo(AIT_REG_D   *fifo, 
                                MMP_SHORT   *buf,
                                MMP_ULONG   amount,
                                MMP_BOOL    stereo)
{
    MMP_SHORT   temp;
    MMP_USHORT	i, loop_cnt = 0;

    loop_cnt = amount >> 2;

    if (!buf) {
        for(i = 0; i < loop_cnt; i++) {
            temp = *fifo;
            temp = *fifo;
            temp = *fifo;
            temp = *fifo;
        }
        #if (ADC_INITIAL_SILENCE != 0)
        if (m_ulAdcInitSilence) {
            m_ulAdcInitSilence =   (m_ulAdcInitSilence > amount) ?
                                    m_ulAdcInitSilence - amount :
                                    0;
        }
        #endif
        return;
    }

    #if (ADC_INITIAL_SILENCE != 0)
    if (m_ulAdcInitSilence) {
        /* Fill buffer with zero samples as silence */
        for(i = 0; i < loop_cnt; i++) {
            temp = *fifo;
            temp = *fifo;
            temp = *fifo;
            temp = *fifo;
        }
        if (stereo) {
            MEMSET(buf, 0, amount << 1);
        }
        else {
            MEMSET(buf, 0, amount);
        }
        if (m_ulAdcInitSilence >= amount)
            m_ulAdcInitSilence -= amount;
        else
            m_ulAdcInitSilence = 0;

        return;
    }
    #endif

    if (stereo) {
        switch (m_AdcInCh) {
        case MMP_AUD_LINEIN_DUAL:
            for(i = 0; i < loop_cnt; i++) {
                *buf++ = *fifo;
                *buf++ = *fifo;
                *buf++ = *fifo;
                *buf++ = *fifo;
            }
            break;
        case MMP_AUD_LINEIN_L:
            for(i = 0; i < loop_cnt; i++) {
                temp   = *fifo;    // L
                *buf++ = temp;
                *buf++ = temp;
                temp   = *fifo;    // R
                temp   = *fifo;    // L
                *buf++ = temp;
                *buf++ = temp;
                temp   = *fifo;    // R
            }
            break;
        case MMP_AUD_LINEIN_R:
            for(i = 0; i < loop_cnt; i++) {
                temp   = *fifo;    // L
                temp   = *fifo;    // R
                *buf++ = temp;
                *buf++ = temp;
                temp   = *fifo;    // L
                temp   = *fifo;    // R
                *buf++ = temp;
                *buf++ = temp;
            }
            break;
        case MMP_AUD_LINEIN_SWAP:
            for(i = 0; i < loop_cnt; i++) {
                temp   = *fifo;    // L
                *buf++ = *fifo;    // R
                *buf++ = temp;
                temp   = *fifo;    // L
                *buf++ = *fifo;    // R
                *buf++ = temp;
            }
            break;
        }
    }
    else {
        switch (m_AdcInCh) {
        case MMP_AUD_LINEIN_DUAL:
        case MMP_AUD_LINEIN_R:
            for(i = 0; i < loop_cnt; i++) {
                temp   = *fifo;    // L
                *buf++ = *fifo;    // R
                temp   = *fifo;    // L
                *buf++ = *fifo;    // R
            }
            break;
        case MMP_AUD_LINEIN_L:
        case MMP_AUD_LINEIN_SWAP:
            for(i = 0; i < loop_cnt; i++) {
                *buf++ = *fifo;    // L
                temp   = *fifo;    // R
                *buf++ = *fifo;    // L
                temp   = *fifo;    // R
            }
            break;
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_WriteFifo
//  Description : Write PCM data to audio Tx FIFO
//------------------------------------------------------------------------------
/**
 @brief Write raw PCM data from audio engine Tx FIFO

 @param[in] fifo    FIFO port of audio HW engine
 @param[in] buf     Buffer to store the PCM data
 @param[in] amount  Sample data available for all channel(s) to write to FIFO
 @param[in] ch      Number of data output channel

 @retval None.
*/
static void MMPF_Audio_WriteFifo(   AIT_REG_D   *fifo,
                                    MMP_SHORT   *buf,
                                    MMP_ULONG   amount,
                                    MMP_ULONG   ch)
{
    MMP_ULONG   i, loop_cnt = 0;

    if (ch == 2)
        loop_cnt = amount >> 2;
    else if (ch == 1)
        loop_cnt = amount >> 1;

    if (!buf) {
        /* Fill 0 if no PCM data available */
        for(i = 0; i < loop_cnt; i++) {
            *fifo = 0;
            *fifo = 0;
            *fifo = 0;
            *fifo = 0;
        }
        return;
    }

    if (ch == 2) {
        for(i = 0; i < loop_cnt; i++) {
            *fifo = *buf++;
            *fifo = *buf++;
            *fifo = *buf++;
            *fifo = *buf++;
        }
    }
    else if (ch == 1) {
        for(i = 0; i < loop_cnt; i++) {
            *fifo = *buf;
            *fifo = *buf++;
            *fifo = *buf;
            *fifo = *buf++;
        }
    }
}
//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoInHandler
//  Description : Interrupt handler for FIFO IN interrupt
//------------------------------------------------------------------------------
/**
 @brief Interrupt handler for reading raw PCM from Rx FIFO.

 @param[in] fifo    FIFO port of audio HW engine

 @retval None.
*/
static void MMPF_Audio_FifoInHandler(AIT_REG_D *fifo)
{
    MMP_ULONG free = 0, thr = 0, end_ofst;
    MMP_SHORT *in_buf;
    AITPS_AUD pAUD = AITC_BASE_AUD;

    thr  = pAUD->ADC_FIFO.MP.RD_TH;

    #if (ADC_INITIAL_SILENCE != 0)
    if (m_ulAdcInitSilence > 0) {
        if (m_ulAdcInitSilence < (thr << 2) && !m_bAdcMute &&
            (pAUD->AFE_DIG_GAIN_SETTING_CTL & AFE_LR_MUTE_ENA))
        {
            MMPF_Audio_SetADCMute(m_bAdcMute, MMP_FALSE);
        }
    }
    #endif

    AUTL_RingBuf_SpaceAvailable(&m_AdcInRingBuf, &free);
    free = free >> 1;   // in unit of sample (16-bit)
    if (free >= thr) {
        in_buf = (MMP_SHORT *)(m_AdcInRingBuf.buf + m_AdcInRingBuf.ptr.wr);
        end_ofst = (m_AdcInRingBuf.size - m_AdcInRingBuf.ptr.wr) >> 1;

        if (end_ofst >= thr) {
            MMPF_Audio_ReadFifo(fifo, in_buf, thr, MMP_TRUE);
        }
        else {
            MMPF_Audio_ReadFifo(fifo, in_buf, end_ofst, MMP_TRUE);
            in_buf = (MMP_SHORT *)m_AdcInRingBuf.buf;
            MMPF_Audio_ReadFifo(fifo, in_buf, thr - end_ofst, MMP_TRUE);
        }
        AUTL_RingBuf_CommitWrite(&m_AdcInRingBuf, thr << 1);
        MMPF_OS_SetFlags(AUD_REC_Flag, AUD_FLAG_IN_READY, MMPF_OS_FLAG_SET);
    }
    else {
        MMPF_Audio_ReadFifo(fifo, NULL, thr, MMP_TRUE);
        RTNA_DBG_Str0("f_ov\r\n");
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoOutHandler
//  Description : Interrupt handler for FIFO OUT interrupt
//------------------------------------------------------------------------------
/**
 @brief Interrupt handler for sending raw PCM to Tx FIFO.

 @param[in] fifo    FIFO port of audio HW engine

 @retval None.
*/
static void MMPF_Audio_FifoOutHandler(AIT_REG_D *fifo)
{
    MMP_ULONG available = 0, thr = 0, end_ofst, ch;
    MMP_ULONG buf_addr;
    MMP_SHORT *buf;
    AITPS_AUD pAUD = AITC_BASE_AUD;
    static MMP_ULONG preiod_remain = 0;

    thr = pAUD->AFE_FIFO.DAC_MP.WR_TH;
    ch  = MMPF_Alsa_Object(ALSA_ID_SPK_OUT)->propt.ch;

    if (m_DacOutRingBuf) {
        /* Data has been filled into the whole buffer by Linux */
        #if 0 // comment out this black-code
        if ((m_DacOutRingBuf->ptr.rd == 0) &&
            (m_DacOutRingBuf->ptr.rd_wrap == 0)) {
            m_DacOutRingBuf->ptr.wr_wrap = 1;
            preiod_remain = 0;
            RTNA_DBG_Str(0,"[SEAN]:FILLED FULL\r\n");
        }
        #endif

        AUTL_RingBuf_DataAvailable(m_DacOutRingBuf, &available);
        available = available >> 1;   // in unit of sample (16-bit)

        //RTNA_DBG_Dec(0,available);
        //RTNA_DBG_Str(0,"\r\n");
        
        if (ch == 1)
            thr = thr >> 1;

        available = (available >= thr) ? thr : available;

        /* Fill FIFO according to the buffer period */
        if (preiod_remain == 0)
            preiod_remain = m_DacModule[0].period;

        available = (preiod_remain > available) ? available : preiod_remain;
        preiod_remain -= available;
    }

    if (available) {
        buf_addr = (MMP_ULONG)m_DacOutRingBuf->buf_phy + m_DacOutRingBuf->ptr.rd;
        buf_addr = DRAM_CACHE_VA(buf_addr);
        buf      = (MMP_SHORT *)buf_addr;
        end_ofst = (m_DacOutRingBuf->size - m_DacOutRingBuf->ptr.rd) >> 1;

        if (end_ofst >= available) {
            MMPF_Audio_WriteFifo(fifo, buf, available, ch);
            #if (SUPPORT_AEC)
            MMPF_AEC_TxReady(buf, available, ch);
            #endif
        }
        else {
            MMPF_Audio_WriteFifo(fifo, buf, end_ofst, ch);
            #if (SUPPORT_AEC)
            MMPF_AEC_TxReady(buf, end_ofst, ch);
            #endif
            buf = (MMP_SHORT *)m_DacOutRingBuf->buf_phy;
            MMPF_Audio_WriteFifo(fifo, buf, available - end_ofst, ch);
            #if (SUPPORT_AEC)
            MMPF_AEC_TxReady(buf, available - end_ofst, ch);
            #endif
        }
        AUTL_RingBuf_CommitRead(m_DacOutRingBuf, available << 1);
        m_DacModule[0].ready = MMP_TRUE;

        //MMPF_Alsa_Write(ALSA_ID_SPK_OUT);
    }
    else {
        MMPF_Audio_WriteFifo(fifo, NULL, thr, 2);
        if (m_DacModule[0].ready)
            RTNA_DBG_Str0("#");
    }
    // Always request data till stop
    MMPF_Alsa_Write(ALSA_ID_SPK_OUT);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_InitializeRxFIFO
//  Description : Reset audio Rx FIFO
//------------------------------------------------------------------------------
/**
 @brief Reset audio Rx FIFO.

 @param[in] usPath      Audio Rx path
 @param[in] usThreshold Threshold to generate Rx FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_InitializeRxFIFO( MMP_USHORT  usPath,
                                            MMP_USHORT  usThreshold)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;
    MMP_ULONG inbuf;

    switch (usPath) {
   	case ADC_TO_AFE_RX_FIFO:
        inbuf = (MMP_ULONG)m_AdcInBuf;
        AUTL_RingBuf_Init(&m_AdcInRingBuf, inbuf, sizeof(m_AdcInBuf));

        pAUD->ADC_FIFO.MP.RST = AUD_FIFO_RST_EN;
        pAUD->ADC_FIFO.MP.RST = 0;
        pAUD->ADC_FIFO.MP.RD_TH = usThreshold;
        break;
    default:
        return MMP_AUDIO_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_InitializeTxFIFO
//  Description : Reset audio Tx FIFO
//------------------------------------------------------------------------------
/**
 @brief Reset audio Tx FIFO.

 @param[in] usPath      Audio Tx path
 @param[in] usThreshold Threshold to generate Tx FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_InitializeTxFIFO( MMP_USHORT  usPath,
                                            MMP_USHORT  usThreshold)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;
    MMP_ULONG i;

    switch (usPath) {
   	case AFE_TX_FIFO_TO_DAC:
        pAUD->AFE_FIFO.DAC_MP.RST = AUD_FIFO_RST_EN;
        pAUD->AFE_FIFO.DAC_MP.RST = 0;
        pAUD->AFE_FIFO.DAC_MP.WR_TH = usThreshold;

        // initialized FIFO with zero samples
        for (i = 0; i < DAC_FIFO_DEPTH; i += 4) {
            pAUD->AFE_FIFO.DAC_MP.DATA = 0;
            pAUD->AFE_FIFO.DAC_MP.DATA = 0;
            pAUD->AFE_FIFO.DAC_MP.DATA = 0;
            pAUD->AFE_FIFO.DAC_MP.DATA = 0;
        }
        break;
    default:
        return MMP_AUDIO_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoIn
//  Description : Get the address of input samples to read
//------------------------------------------------------------------------------
/**
 @brief Get the address of input samples to read from.

 @retval The address of samples located.
*/
MMP_ULONG MMPF_Audio_FifoIn(void)
{
    return (m_AdcInRingBuf.buf + m_AdcInRingBuf.ptr.rd);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoInSamples
//  Description : Get the amount of samples available in input buffer
//------------------------------------------------------------------------------
/**
 @brief Get the amount of data samples avaiable within the FIFO in buffer.

 @retval The samples count available in input buffer.
*/
MMP_ULONG MMPF_Audio_FifoInSamples(void)
{
    MMP_ULONG size = 0;

    if (AUTL_RingBuf_DataAvailable(&m_AdcInRingBuf, &size)) {
        RTNA_DBG_Str0("AudInBuf err\r\n");
        return 0;
    }

    return (size >> 1);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoInRead
//  Description : Read input data from the FIFO input buffer
//------------------------------------------------------------------------------
/**
 @brief Read the input data from the input buffer, but doesn't update the read
        pointer.

 @param[in] buf_addr    The target buffer address to copy data into
 @param[in] samples     The number of samples to read (for all channels)
 @param[in] ch          Input channel number
 @param[in] ofst        Sample offset to read data from FIFO buffer

 @retval The number of samples read from the input buffer.
*/
MMP_ULONG MMPF_Audio_FifoInRead(MMP_SHORT   *buf_addr,
                                MMP_ULONG   samples,
                                MMP_ULONG   ch,
                                MMP_ULONG   ofst)
{
    MMP_SHORT *data;
    MMP_ULONG data_size = 0, end_ofst, i;
    AUTL_RINGBUF ring; 

    AUTL_RingBuf_Fork(&m_AdcInRingBuf, &ring);

    if (ofst) {
        ring.ptr.rd += (ofst << 1);
        if (ring.ptr.rd >= ring.size)
            ring.ptr.rd -= ring.size;
    }

    data = (MMP_SHORT *)(ring.buf + ring.ptr.rd);
    end_ofst = (ring.size - ring.ptr.rd) >> 1;

    AUTL_RingBuf_DataAvailable(&ring, &data_size);

    if (ch == 2)
        data_size = data_size >> 1; // in unit of sample (16-bit)
    else if (ch == 1)
        data_size = data_size >> 2; // in unit of sample (16-bit), one channel
    data_size = (data_size > samples) ? samples : data_size;

    if (data_size == 0)
        return 0;

    if (end_ofst >= data_size) {
        if (ch == 2) {
            MEMCPY(buf_addr, data, data_size << 1);
        }
        else if (ch == 1) {
            i = data_size; // for one channel
            while(i) {
                *buf_addr++ = *data;
                data += 2; // skip one channel
                *buf_addr++ = *data;
                data += 2; // skip one channel
                i -= 2;
            }
        }
    }
    else {
        // Will wrap to the head of ring buffer
        if (ch == 2) {
            MEMCPY(buf_addr, data, end_ofst << 1);
            buf_addr += end_ofst;
            data = (MMP_SHORT *)ring.buf;
            MEMCPY(buf_addr, data, (data_size - end_ofst) << 1);
        }
        else if (ch == 1) {
            i = end_ofst >> 1; // for one channel
            while(i) {
                *buf_addr++ = *data;
                data += 2; // skip one channel
                *buf_addr++ = *data;
                data += 2; // skip one channel
                i -= 2;
            }
            data = (MMP_SHORT *)ring.buf;
            i = (data_size - end_ofst) >> 1; // for one channel
            while(i) {
                *buf_addr++ = *data;
                data += 2; // skip one channel
                *buf_addr++ = *data;
                data += 2; // skip one channel
                i -= 2;
            }
        }
    }

    return data_size;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_FifoInCommitRead
//  Description : Advance the read pointer of the FIFO input buffer
//------------------------------------------------------------------------------
/**
 @brief Advance the read pointer of the input buffer.

 @param[in] samples     The number of samples have been read out

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_FifoInCommitRead(MMP_ULONG samples)
{
    int err;

    err = AUTL_RingBuf_StrictCommitRead(&m_AdcInRingBuf, samples << 1);
    if (err) {
        RTNA_DBG_Str0("AudInBuf upd rd err\r\n");
        return MMP_AUDIO_ERR_STREAM_POINTER;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_OpenCapture
//  Description : Power on ADC and reset Rx FIFO
//------------------------------------------------------------------------------
/**
 @brief Power on ADC and reset Rx FIFO

 @param[in] threshold   Threshold to generate FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_OpenCapture(MMP_USHORT threshold)
{
    if (m_ulAdcInPath & MMP_AUD_AFE_IN_MASK) {
        MMPF_Audio_InitializeRxFIFO(ADC_TO_AFE_RX_FIFO, threshold);
        MMPF_Audio_PowerOnADC();
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_EnableCapture
//  Description : Enable FIFO interrupt
//------------------------------------------------------------------------------
/**
 @brief Enable ADC to receive PCM data via FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_EnableCapture(void)
{
    if (m_ulAdcInPath & MMP_AUD_AFE_IN_MASK) {
        #if (ADC_INITIAL_SILENCE != 0)
        if (!m_bAdcMute && m_ulAdcInitSilence) {
            AITPS_AUD pAUD = AITC_BASE_AUD;

            pAUD->AFE_ADC_ANA_LPGA_CTL |= LPGA_MUTE_CTL;
            pAUD->AFE_ADC_ANA_RPGA_CTL |= RPGA_MUTE_CTL;
            pAUD->AFE_DIG_GAIN_SETTING_CTL |= AFE_LR_MUTE_ENA;
        }
        #endif

        // Enable AFE FIFO interrupt
        MMPF_Audio_EnableMux(ADC_TO_AFE_RX_FIFO, MMP_TRUE);
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_DisableCapture
//  Description : Disable FIFO interrupt
//------------------------------------------------------------------------------
/**
 @brief Disable FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_DisableCapture(void)
{
    MMP_ERR err = MMP_AUDIO_ERR_INVALID_FLOW;

    if (m_ulAdcInPath & MMP_AUD_AFE_IN_MASK) {
        err = MMPF_Audio_EnableMux(ADC_TO_AFE_RX_FIFO, MMP_FALSE);
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }
    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_CloseCapture
//  Description : Power off ADC
//------------------------------------------------------------------------------
/**
 @brief Power down ADC

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_CloseCapture(void)
{
    MMP_ERR err = MMP_AUDIO_ERR_INVALID_FLOW;

    if (m_ulAdcInPath & MMP_AUD_AFE_IN_MASK) {
        err = MMPF_Audio_PowerOffADC();
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }
    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_OpenRender
//  Description : Power on DAC and reset Tx FIFO
//------------------------------------------------------------------------------
/**
 @brief Power on DAC and reset Tx FIFO

 @param[in] threshold   Threshold to generate FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_OpenRender(MMP_USHORT threshold)
{
    if (m_ulDacOutPath & MMP_AUD_AFE_OUT_MASK) {
        MMPF_Audio_InitializeTxFIFO(AFE_TX_FIFO_TO_DAC, threshold);
        MMPF_Audio_PowerOnDAC();
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_EnableRender
//  Description : Enable FIFO interrupt
//------------------------------------------------------------------------------
/**
 @brief Enable FIFO interrupt

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_EnableRender(void)
{
    if (m_ulDacOutPath & MMP_AUD_AFE_OUT_MASK) {
        // Enable AFE FIFO interrupt
        MMPF_Audio_EnableMux(AFE_TX_FIFO_TO_DAC, MMP_TRUE);
    }
    else {
        return MMP_AUDIO_ERR_INVALID_FLOW;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_DisableRender
//  Description : Disable FIFO interrupt and power off DAC
//------------------------------------------------------------------------------
/**
 @brief Disable FIFO interrupt and power down DAC

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_DisableRender(void)
{
    MMP_ERR err = MMP_AUDIO_ERR_INVALID_FLOW;

    if (m_ulDacOutPath & MMP_AUD_AFE_OUT_MASK)
        err = MMPF_Audio_EnableMux(AFE_TX_FIFO_TO_DAC, MMP_FALSE);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_CloseRender
//  Description : Power off DAC
//------------------------------------------------------------------------------
/**
 @brief Power down DAC

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_CloseRender(void)
{
    MMP_ERR err = MMP_AUDIO_ERR_INVALID_FLOW;

    if (m_ulDacOutPath & MMP_AUD_AFE_OUT_MASK)
        err = MMPF_Audio_PowerOffDAC();

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_CtrlClock
//  Description : Control audio engine clocks
//------------------------------------------------------------------------------
/**
 @brief Control audio engine related clocks

 @param[in] clktype Audio clock types, bit-mapping
 @param[in] enable  Enable or disable clocks

 @retval None.
*/
void MMPF_Audio_CtrlClock(MMP_AUD_CLK clktype, MMP_BOOL enable)
{
    if (clktype & MMP_AUD_CLK_CORE)
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_AUD, enable);
    if (clktype & MMP_AUD_CLK_ADC)
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_ADC, enable);
    if (clktype & MMP_AUD_CLK_DAC)
        MMPF_SYS_EnableClock(MMPF_SYS_CLK_DAC, enable);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetInputCh
//  Description : Set input channel according to the schematic
//------------------------------------------------------------------------------
/**
 @brief Select the input channel(s) according to the HW schematic

 @param[in] ch      Input channel (L/R/LR)

 @retval It reports the status of the operation.
*/
void MMPF_Audio_SetInputCh(MMP_AUD_LINEIN_CH ch)
{
    m_AdcInCh = ch;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetInputPath
//  Description : Set HW input path
//------------------------------------------------------------------------------
/**
 @brief Select the HW input path

 @param[in] path    Input path of audio raw PCM data

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetInputPath(MMP_AUD_INOUT_PATH path)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;
    AITPS_GBL pGBL = AITC_BASE_GBL;

    m_ulAdcInPath = path;

    pAUD->AFE_ADC_ANA_LPGA_CTL &= ~(LPGA_SRC_IN_MASK);
    pAUD->AFE_ADC_ANA_RPGA_CTL &= ~(RPGA_SRC_IN_MASK);

    if (m_ulAdcInPath == MMP_AUD_AFE_IN_SING) {
        pAUD->AFE_ADC_ANA_LPGA_CTL |= LPGA_SRC_IN_AUXL;
        pAUD->AFE_ADC_ANA_RPGA_CTL |= RPGA_SRC_IN_AUXR;
    }
    else if (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF) {
        pAUD->AFE_ADC_ANA_LPGA_CTL |= LPGA_SRC_IN_MICLIP_LIN;
        pAUD->AFE_ADC_ANA_RPGA_CTL |= RPGA_SRC_IN_MICRIP_RIN;            
    }
    else if (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF2SING) {
        pAUD->AFE_ADC_ANA_LPGA_CTL |= LPGA_SRC_IN_MICLIP;
        pAUD->AFE_ADC_ANA_RPGA_CTL |= RPGA_SRC_IN_MICRIP;       
    }
    else if (m_ulAdcInPath == MMP_AUD_DMIC_PAD0) {
        pGBL->GBL_I2S_DMIC_CFG  &= ~(GBL_DMIC_PAD_MASK);
        pGBL->GBL_I2S_DMIC_CFG  |=GBL_DMIC_PAD(0);
    }
    else if (m_ulAdcInPath == MMP_AUD_DMIC_PAD1) {
        pGBL->GBL_I2S_DMIC_CFG  &= ~(GBL_DMIC_PAD_MASK);
        pGBL->GBL_I2S_DMIC_CFG  |=GBL_DMIC_PAD(1);
    }
    else if (m_ulAdcInPath == MMP_AUD_DMIC_PAD2) {
        pGBL->GBL_I2S_DMIC_CFG  &= ~(GBL_DMIC_PAD_MASK);
        pGBL->GBL_I2S_DMIC_CFG  |=GBL_DMIC_PAD(2);
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetOutputPath
//  Description : Set HW audio output path
//------------------------------------------------------------------------------
/**
 @brief Select the HW audio output path

 @param[in] path    Output path of audio PCM data

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetOutputPath(MMP_AUD_INOUT_PATH path)
{
    if (path & MMP_AUD_AFE_OUT_MASK)
        m_ulDacOutPath = path;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetADCDigitalGain
//  Description : Set ADC digital gain
//------------------------------------------------------------------------------
/**
 @brief Set the digital gain of ADC

 @param[in] gain        ADC digital gain, refer to register guide
 @param[in] keepSetting Keep the gain setting or not

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetADCDigitalGain(MMP_UBYTE gain, MMP_BOOL bSaveSettingOnly)
{
	AITPS_AUD pAUD = AITC_BASE_AUD;

	m_bAdcDigGain = gain;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    pAUD->AFE_ADC_DIG_GAIN = (gain << 8) | gain;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetDACDigitalGain
//  Description : Set DAC digital gain
//------------------------------------------------------------------------------
/**
 @brief Set the digital gain of DAC

 @param[in] gain        DAC digital gain, refer to register guide
 @param[in] keepSetting Keep the gain setting or not

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetDACDigitalGain(MMP_UBYTE gain, MMP_BOOL bSaveSettingOnly)
{
	AITPS_AUD pAUD = AITC_BASE_AUD;

    gain &= DAC_DIG_GAIN_MASK;

	m_bDacDigGain = gain;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    pAUD->AFE_DAC_DIG_GAIN = (gain << 8) | gain;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetADCAnalogGain
//  Description : Set ADC analog gain
//------------------------------------------------------------------------------
/**
 @brief Set the analog gain of ADC

 @param[in] gain        ADC analog gain, refer to register guide
 @param[in] keepSetting Keep the gain setting or not

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetADCAnalogGain(MMP_UBYTE gain, MMP_BOOL bSaveSettingOnly)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    m_bAdcAnaGain = gain;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    pAUD->AFE_ADC_ANA_LPGA_GAIN = gain;
    pAUD->AFE_ADC_ANA_RPGA_GAIN = gain;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetDACAnalogGain
//  Description : Set DAC analog gain
//------------------------------------------------------------------------------
/**
 @brief Set the analog gain of DAC

 @param[in] gain        DAC analog gain, refer to register guide
 @param[in] keepSetting Keep the gain setting or not

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetDACAnalogGain(MMP_UBYTE gain, MMP_BOOL bSaveSettingOnly)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    m_bDacAnaGain = ((gain << 1) & LOUT_ANA_GAIN_MASK) >> 1;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    pAUD->AFE_DAC_LOUT_VOL = (gain << 1) & LOUT_ANA_GAIN_MASK;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetADCMute
//  Description : Mute or unmute ADC
//------------------------------------------------------------------------------
/**
 @brief Mute control of ADC

 @param[in] bMute   MMP_TRUE to mute ADC, and MMP_FALSE to unmute

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetADCMute(MMP_BOOL bMute, MMP_BOOL bSaveSettingOnly)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    m_bAdcMute = bMute;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    if (bMute) {
        pAUD->AFE_ADC_ANA_LPGA_CTL |= LPGA_MUTE_CTL;
        pAUD->AFE_ADC_ANA_RPGA_CTL |= RPGA_MUTE_CTL;
        pAUD->AFE_DIG_GAIN_SETTING_CTL |= AFE_LR_MUTE_ENA;
    }
    else {
        pAUD->AFE_ADC_ANA_LPGA_CTL &= ~(LPGA_MUTE_CTL);
        pAUD->AFE_ADC_ANA_RPGA_CTL &= ~(RPGA_MUTE_CTL);
        pAUD->AFE_DIG_GAIN_SETTING_CTL &= ~(AFE_LR_MUTE_ENA);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetDACMute
//  Description : Mute or unmute DAC
//------------------------------------------------------------------------------
/**
 @brief Mute control of DAC

 @param[in] bMute   MMP_TRUE to mute DAC, and MMP_FALSE to unmute

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetDACMute(MMP_BOOL bMute, MMP_BOOL bSaveSettingOnly)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    m_bDacMute = bMute;

    if (bSaveSettingOnly)
        return MMP_ERR_NONE;

    if (bMute) {
        pAUD->AFE_DAC_LOUT_VOL |= LOUT_ANA_GAIN_MUTE;
        pAUD->AFE_DAC_DIG_GAIN_CTL |= DAC_R_SOFT_MUTE | DAC_L_SOFT_MUTE;
    }
    else {
        pAUD->AFE_DAC_LOUT_VOL &= ~(LOUT_ANA_GAIN_MUTE);
        pAUD->AFE_DAC_DIG_GAIN_CTL &= ~(DAC_R_SOFT_MUTE | DAC_L_SOFT_MUTE);
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetDACFreq
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_Audio_SetDACFreq(MMP_ULONG samplerate)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;
    AITPS_I2S pI2S = AITC_BASE_I2S0;

    if (samplerate <= 48000) {
        #if (HIGH_SRATE_MODE == DOWN_SAMPLE_TIMES)
        pI2S->AFE_DOWN_SAMPLE_SEL = DOWN_SAMPLE_OFF;
        #elif (HIGH_SRATE_MODE == BYPASS_FILTER_STAGE)
        pAUD->AFE_SAMPLE_RATE_SEL = SRATE_48000Hz_UNDER;
        #endif
    }

    switch(samplerate) {
	case 48000:
        pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_48000HZ);
		break;	
	case 44100:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_44100HZ);
		break;	
	case 32000:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_32000HZ);
		break;	
	case 24000:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_24000HZ);
		break;	
	case 22050:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_22050HZ);
		break;	
	case 16000:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_16000HZ);
		break;	
	case 12000:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_12000HZ);
		break;	
	case 11025:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_11025HZ);
		break;
	case 8000:
		pAUD->AFE_SAMPLE_RATE_CTL = DAC_SRATE(SRATE_8000HZ);
		break;	
	}

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetHPFMode
//  Description : Set the audio high pass filter mode
//------------------------------------------------------------------------------
/**
 @brief Set the audio high pass filter mode

 @param[in] HPFMode HPF mode, set the cut-off frequency

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_SetHPFMode(MMP_AUD_HPF_MODE HPFMode) 
{
    AITPS_DADC_EXT pDADC_EXT = AITC_BASE_DADC_EXT;

    switch(HPFMode) {
    case MMP_AUD_HPF_AUD_2HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_AUD;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_AUD_FC_2HZ;
        break;
    case MMP_AUD_HPF_AUD_4HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_AUD;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_AUD_FC_4HZ;
        break;        
    case MMP_AUD_HPF_AUD_8HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_AUD;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_AUD_FC_8HZ;
        break;
    case MMP_AUD_HPF_AUD_16HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_AUD;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_AUD_FC_16HZ;
        break;
        
    case MMP_AUD_HPF_VOC_2D5HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_2D5HZ;
        break;
    case MMP_AUD_HPF_VOC_25HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_25HZ;
        break;        
    case MMP_AUD_HPF_VOC_50HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_50HZ;
        break;
    case MMP_AUD_HPF_VOC_100HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_100HZ;
        break;
    case MMP_AUD_HPF_VOC_200HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_200HZ;
        break;
    case MMP_AUD_HPF_VOC_300HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_300HZ;
        break;
    case MMP_AUD_HPF_VOC_400HZ:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_VOC;
	    pDADC_EXT->DADC_HPF_VOC_MODE_COEF   =  HPF_VOC_FC_400HZ;
        break;
    case MMP_AUD_HPF_BYPASS:
        pDADC_EXT->DADC_HPF_MODE_SEL        =  ADC_HPF_BYPASS;
        break;
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_EnableMux
//  Description : Enable or disable MUX in the audio data route
//------------------------------------------------------------------------------
/**
 @brief Enable or disable MUX in the audio data route

 @param[in] path    Audio route path
 @param[in] bEnable Eable or disable date to pass the route

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_EnableMux(MMPF_AUDIO_DATA_FLOW path, MMP_BOOL enable)
{
    MMP_ULONG i = 0x10;
    AITPS_AUD pAUD = AITC_BASE_AUD;

	switch (path) {
    case AFE_TX_FIFO_TO_DAC:
        if (enable) {
            m_bDacOn = MMP_TRUE;

            pAUD->AFE_FIFO.DAC_MP.RST = AUD_FIFO_RST_EN;
            while(i--); // wait for one cycle of audio moudle clock
            pAUD->AFE_FIFO.DAC_MP.RST = 0;

            pAUD->AFE_FIFO.DAC_MP.CPU_INT_EN |= AUD_INT_FIFO_REACH_UNWR_TH;
            pAUD->AFE_CPU_INT_EN |= AUD_DAC_INT_EN;

            pAUD->AFE_L_CHNL_DATA = 0;
            pAUD->AFE_R_CHNL_DATA = 0;	               

			pAUD->AFE_MUX_MODE_CTL |= AFE_MUX_AUTO_MODE;
        }
        else {
            m_bDacOn = MMP_FALSE;

            pAUD->AFE_L_CHNL_DATA = 0;
            pAUD->AFE_R_CHNL_DATA = 0;
            if (!m_bAdcOn && !m_bDacOn)
                pAUD->AFE_MUX_MODE_CTL &= ~(AFE_MUX_AUTO_MODE);
            pAUD->AFE_FIFO.DAC_MP.CPU_INT_EN &= ~(AUD_INT_FIFO_REACH_UNWR_TH);
            pAUD->AFE_CPU_INT_EN &= ~(AUD_DAC_INT_EN);
        }
        break;
    case ADC_TO_AFE_RX_FIFO:
        if (enable) {
            m_bAdcOn = MMP_TRUE;

            pAUD->ADC_FIFO.MP.RST = AUD_FIFO_RST_EN;
            while(i--); // wait for one cycle of audio moudle clock
            pAUD->ADC_FIFO.MP.RST = 0;

            pAUD->ADC_FIFO.MP.CPU_INT_EN |= AUD_INT_FIFO_REACH_UNRD_TH;
            pAUD->AFE_CPU_INT_EN |= AUD_ADC_INT_EN;                                              

            pAUD->AFE_MUX_MODE_CTL |= AFE_MUX_AUTO_MODE;
        }
        else {
            m_bAdcOn = MMP_FALSE;

            if (!m_bAdcOn && !m_bDacOn)
                pAUD->AFE_MUX_MODE_CTL &= ~(AFE_MUX_AUTO_MODE);
            pAUD->ADC_FIFO.MP.CPU_INT_EN &= ~(AUD_INT_FIFO_REACH_UNRD_TH);
            pAUD->AFE_CPU_INT_EN &= ~(AUD_ADC_INT_EN);
        }
        break;
    default:
        return MMP_AUDIO_ERR_PARAMETER;
	}

    return  MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_PowerOnADC
//  Description : Power on ADC
//------------------------------------------------------------------------------
/**
 @brief Power on ADC digital/analog part, set the specified gain and unmute.

 @param[in] samplerate  Specified the ADC sample rate

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_PowerOnADC(void)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;
    MMP_ULONG ulStepTime = 0;

    pAUD->AFE_ADC_LOOP_CTL = 0;

    pAUD->AFE_DIG_GAIN_SETTING_CTL = AFE_DIG_GAIN_SMOOTH_METHOD |
                                     AFE_LR_MUTE_ENA;
    pAUD->AFE_ANA_GAIN_SETTING_CTL = AFE_ANA_GAIN_SMOOTH_METHOD |
                                     TIME_OUT_PULSE_ENA;

    MMPF_Audio_SetHPFMode(MMP_AUD_HPF_VOC_300HZ);

    if ((pAUD->AFE_POWER_CTL & ANALOG_POWER_EN) == 0) {
        pAUD->AFE_POWER_CTL |= ANALOG_POWER_EN;
        if (!m_bAdcFastPowerOn)
            MMPF_OS_Sleep(170); //at least wait 170ms
    }

    if ((m_ulAdcInPath & MMP_AUD_DMIC_MASK) == 0) {
        if ((pAUD->AFE_POWER_CTL & VREF_POWER_EN) == 0) {
            pAUD->AFE_POWER_CTL |= VREF_POWER_EN;
            if (!m_bAdcFastPowerOn)
                MMPF_OS_Sleep(20);
        }            
    }
    else {
        pAUD->AFE_CLK_CTL |= DMIC_EN;
    }

    if ((pAUD->AFE_POWER_CTL & ADC_DF_POWER_EN) == 0) {
        pAUD->AFE_POWER_CTL |= ADC_DF_POWER_EN;
        if (!m_bAdcFastPowerOn)
            MMPF_OS_Sleep(10);
    }

    //Control ADC mic L/R boost up, mic bias
    if ((m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF) ||
        (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF2SING))
    {
		#if (DEFAULT_ADC_MIC_BIAS_MP == ADC_MIC_BIAS_NONE)
	    pAUD->AFE_MICBIAS_DUAL_CHAN = 0; // power down mic bias
		#elif (DEFAULT_ADC_MIC_BIAS_MP == ADC_MIC_BIAS_0d65AVDD_MP)
	    pAUD->AFE_MICBIAS_DUAL_CHAN = ( pAUD->AFE_MICBIAS_DUAL_CHAN &
	                                    ADC_MIC_BIAS_VOLT_MASK) |
	    		                        ADC_MIC_BIAS_R_VOLT_0D65_MP |
	    		                        ADC_MIC_BIAS_L_VOLT_0D65_MP;
		#elif (DEFAULT_ADC_MIC_BIAS_MP == ADC_MIC_BIAS_0d75AVDD_MP)
		pAUD->AFE_MICBIAS_DUAL_CHAN = ( pAUD->AFE_MICBIAS_DUAL_CHAN &
		                                ADC_MIC_BIAS_VOLT_MASK) |
	    		                        ADC_MIC_BIAS_R_VOLT_0D75_MP |
	    		                        ADC_MIC_BIAS_L_VOLT_0D75_MP;
		#elif (DEFAULT_ADC_MIC_BIAS_MP == ADC_MIC_BIAS_0d85AVDD_MP)
	    pAUD->AFE_MICBIAS_DUAL_CHAN = ( pAUD->AFE_MICBIAS_DUAL_CHAN &
	                                    ADC_MIC_BIAS_VOLT_MASK) |
	    		                        ADC_MIC_BIAS_R_VOLT_0D85_MP |
	    		                        ADC_MIC_BIAS_L_VOLT_0D85_MP;
		#elif (DEFAULT_ADC_MIC_BIAS_MP == ADC_MIC_BIAS_0d95AVDD_MP)
	    pAUD->AFE_MICBIAS_DUAL_CHAN = ( pAUD->AFE_MICBIAS_DUAL_CHAN &
	                                    ADC_MIC_BIAS_VOLT_MASK) |
	                             		ADC_MIC_BIAS_R_VOLT_0D95_MP|
	                             		ADC_MIC_BIAS_L_VOLT_0D85_MP;
		#endif
        pAUD->AFE_MICBIAS_DUAL_CHAN |=  ADC_MIC_BIAS_R_POWER_UP |
                                        ADC_MIC_BIAS_L_POWER_UP;
        
        if (((pAUD->AFE_ANA_ADC_POWER_CTL & ADC_PGA_L_POWER_EN) == 0) ||
            ((pAUD->AFE_ANA_ADC_POWER_CTL & ADC_PGA_R_POWER_EN) == 0))
        {
            pAUD->AFE_ANA_ADC_POWER_CTL |= (ADC_PGA_L_POWER_EN |
                                            ADC_PGA_R_POWER_EN);
            if (!m_bAdcFastPowerOn)
                MMPF_OS_Sleep(20);
            pAUD->AFE_ANA_ADC_POWER_CTL |= (ADC_R_POWER_UP | ADC_L_POWER_UP);
        }
    }

    //Control ADC in path
    if (m_ulAdcInPath == MMP_AUD_AFE_IN_SING) {
        pAUD->AFE_ADC_ANA_LPGA_CTL = LPGA_SRC_IN_AUXL | IN_LPGA_ZC_EN;
        pAUD->AFE_ADC_ANA_RPGA_CTL = RPGA_SRC_IN_AUXR | IN_RPGA_ZC_EN;
    }
    else if (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF) {
        pAUD->AFE_ADC_ANA_LPGA_CTL = LPGA_SRC_IN_MICLIP_LIN | IN_LPGA_ZC_EN;
        pAUD->AFE_ADC_ANA_RPGA_CTL = RPGA_SRC_IN_MICRIP_RIN | IN_RPGA_ZC_EN;
    }
    else if (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF2SING) {
        pAUD->AFE_ADC_ANA_LPGA_CTL = LPGA_SRC_IN_MICLIP | IN_LPGA_ZC_EN;
        pAUD->AFE_ADC_ANA_RPGA_CTL = RPGA_SRC_IN_MICRIP | IN_RPGA_ZC_EN;
    }
    else if ((m_ulAdcInPath & MMP_AUD_DMIC_MASK)) {
        pAUD->AFE_DMIC_CLK_DATA_CTL |= AFE_DMIC_CLK_EN;
    }

    if (!m_bAdcMute)
        MMPF_Audio_SetADCDigitalGain(m_bAdcDigGain, MMP_FALSE);
    /* Digital gain default is mute, disable mute */
    pAUD->AFE_DIG_GAIN_SETTING_CTL &= ~(AFE_LR_MUTE_ENA);
    pAUD->AFE_ANA_GAIN_SETTING_CTL &= ~(AFE_ANA_GAIN_SMOOTH_METHOD);
    pAUD->AFE_ANA_GAIN_SETTING_CTL |= AFE_ANA_GAIN_DIRECT_METHOD;

    if (!m_bAdcMute) {
        MMPF_Audio_SetADCAnalogGain(m_bAdcAnaGain, MMP_FALSE);
        MMPF_Audio_SetADCMute(MMP_FALSE, MMP_FALSE);
    }
    else {
        MMPF_Audio_SetADCMute(MMP_TRUE, MMP_FALSE);
    }
    pAUD->AFE_DIG_GAIN_MUTE_STEP   &= ~(ADC_PGA_DIRECT_METHOD);
    pAUD->AFE_DIG_GAIN_MUTE_STEP   |= ADC_PGA_SMOOTH_METHOD;
    pAUD->AFE_DIG_GAIN_SETTING_CTL &= ~(AFE_DIG_GAIN_DIRECT_METHOD);
    pAUD->AFE_DIG_GAIN_SETTING_CTL &= ~(AFE_NORMAL_STATE_DIG_GAIN_DIRECT_METHOD);
    pAUD->AFE_DIG_GAIN_SETTING_CTL |= AFE_DIG_GAIN_SMOOTH_METHOD;
    ulStepTime = (256 * m_AdcModule[0].fs) / 1000;
    pAUD->AFE_ANA_L_STEP_TIME[0]    =
    pAUD->AFE_ANA_R_STEP_TIME[0]    = (ulStepTime & 0xFF);
    pAUD->AFE_ANA_L_STEP_TIME[1]    =
    pAUD->AFE_ANA_R_STEP_TIME[1]    = (ulStepTime >> 8) & 0xFF;
    pAUD->AFE_ANA_L_STEP_TIME[2]    =
    pAUD->AFE_ANA_R_STEP_TIME[2]    = (ulStepTime >> 16) & 0xFF;
    pAUD->AFE_LADC_DIG_STEP_TIME[0] =
    pAUD->AFE_RADC_DIG_STEP_TIME[0] = (ulStepTime & 0xFF);
    pAUD->AFE_LADC_DIG_STEP_TIME[1] =
    pAUD->AFE_LADC_DIG_STEP_TIME[1] = (ulStepTime >> 8) & 0xFF;
    pAUD->AFE_LADC_DIG_STEP_TIME[2] =
    pAUD->AFE_LADC_DIG_STEP_TIME[2] = (ulStepTime >> 16) & 0xFF;

    if (!m_bAdcFastPowerOn)
        MMPF_OS_Sleep(250);
#if 0 // always fast power on....
    m_bAdcFastPowerOn = MMP_FALSE;
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_PowerOnDAC
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Audio_PowerOnDAC(void)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    pAUD->AFE_OVF_BUGFIX |= DAC_DIG_VOL_OVF_FIX | DAC_SDM_OVF_FIX;

    pAUD->AFE_CLK_CTL |= DAC_CLK_INV_EN;
    pAUD->AFE_CLK_CTL |= DAC_CLK_256FS_MODE;

    pAUD->AFE_DAC_FILTER_CTL = DAC_128_FS;

    pAUD->AFE_DAC_DIG_GAIN_CTL = DAC_R_SOFT_MUTE | DAC_L_SOFT_MUTE;
    pAUD->AFE_DAC_LOUT_VOL &= ~(LOUT_ANA_GAIN_MASK);

    /* VREF setting time depend on resistor capacitor circuit (RC) */
    if ((pAUD->AFE_POWER_CTL & ANALOG_POWER_EN) == 0) {
        pAUD->AFE_POWER_CTL |= ANALOG_POWER_EN;
        if (!m_bDacFastPowerOn)
            MMPF_OS_Sleep(170);
    }

    if ((pAUD->AFE_POWER_CTL & VREF_POWER_EN) == 0) {
        pAUD->AFE_POWER_CTL |= VREF_POWER_EN;
        if (!m_bDacFastPowerOn)
            MMPF_OS_Sleep(20);
    }

    if ((pAUD->AFE_POWER_CTL & DAC_DF_POWER_EN) == 0) {
        pAUD->AFE_POWER_CTL |= DAC_DF_POWER_EN;
        if (!m_bDacFastPowerOn)
            MMPF_OS_Sleep(10);
    }

    pAUD->AFE_ANA_DAC_POWER_CTL |= DAC_POWER_UP;

    // TBD
    if (!m_bDacFastPowerOn)
        MMPF_OS_Sleep(1); // original 10

    if (m_ulDacOutPath & MMP_AUD_AFE_OUT_LINE)
        pAUD->AFE_ANA_DAC_POWER_CTL |= LINEOUT_POWER_UP;
    else
        pAUD->AFE_ANA_DAC_POWER_CTL &= ~(LINEOUT_POWER_UP);

    if (!m_bDacFastPowerOn)
        MMPF_OS_Sleep(100); //TBD

    if (m_bDacMute) {
        MMPF_Audio_SetDACMute(MMP_TRUE, MMP_FALSE);
    }
    else {
        MMPF_Audio_SetDACAnalogGain(m_bDacAnaGain, MMP_FALSE);
        MMPF_Audio_SetDACDigitalGain(m_bDacDigGain, MMP_FALSE);
        MMPF_Audio_SetDACMute(MMP_FALSE, MMP_FALSE);
    }

    m_bDacFastPowerOn = MMP_FALSE;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_PowerOffADC
//  Description : Power off ADC
//------------------------------------------------------------------------------
/**
 @brief Power off ADC, including the digital/analog part.

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Audio_PowerOffADC(void)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    /* Never close ADC for noise issue */
    return  MMP_ERR_NONE;

    if ((m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF) ||
        (m_ulAdcInPath == MMP_AUD_AFE_IN_DIFF2SING))
        pAUD->AFE_MICBIAS_DUAL_CHAN = 0;

    pAUD->AFE_POWER_CTL         &= ~(ADC_DF_POWER_EN);
    pAUD->AFE_ANA_ADC_POWER_CTL &= ~(ADC_R_POWER_UP |
                                     ADC_L_POWER_UP |
                                     ADC_PGA_L_POWER_EN |
                                     ADC_PGA_R_POWER_EN);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_PowerDownDAC
//  Parameter   : bPowerDownNow -- Truly power down DAC
//  Return Value : None
//  Description : Power down DAC only when bPowerDownNow is set to true
//------------------------------------------------------------------------------
MMP_ERR MMPF_Audio_PowerOffDAC(void)
{
    AITPS_AUD pAUD = AITC_BASE_AUD;

    pAUD->AFE_POWER_CTL &= ~(DAC_DF_POWER_EN);
    pAUD->AFE_ANA_DAC_POWER_CTL &= ~(DAC_POWER_UP | LINEOUT_POWER_UP);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Audio_SetPLL
//  Description : Configure PLL for audio
//------------------------------------------------------------------------------
/**
 @brief Configure PLL for audio according to the specified sample rate.

 @param[in] path        Audio data path
 @param[in] samplerate  Specified the target sample rate

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Audio_SetPLL(MMP_AUD_INOUT_PATH path, MMP_ULONG samplerate)
{
    MMP_ERR             err;
    MMPF_PLL_MODE       pll_mode;
    GRP_CLK_SRC         pll_src;
    MMP_UBYTE           ubPllDiv;
    /* static MMP_ULONG    _ulSamplerate; */

    /* Application might adjust audio PLL fraction manually,
     * so we always re-config audio PLL regardless of whether sample rate
     * is idential to original one or not.
     */
    switch(samplerate) {
    case 192000:
        pll_mode = MMPF_PLL_AUDIO_192K;
        break;
    case 128000:
        pll_mode = MMPF_PLL_AUDIO_128K;
        break;
    case 96000:
        pll_mode = MMPF_PLL_AUDIO_96K;
        break;
    case 64000:
        pll_mode = MMPF_PLL_AUDIO_64K;
        break;
    case 48000:
        pll_mode = MMPF_PLL_AUDIO_48K;
        break;
    case 44100:
        pll_mode = MMPF_PLL_AUDIO_44d1K;
        break;
    case 32000:
        pll_mode = MMPF_PLL_AUDIO_32K;
        break;
    case 24000:
        pll_mode = MMPF_PLL_AUDIO_24K;
        break;
    case 22050:
        pll_mode = MMPF_PLL_AUDIO_22d05K;
        break;
    case 16000:
        pll_mode = MMPF_PLL_AUDIO_16K;
        break;
    case 12000:
        pll_mode = MMPF_PLL_AUDIO_12K;
        break;
    case 11025:
        pll_mode = MMPF_PLL_AUDIO_11d025K;
        break;
    case 8000:
        pll_mode = MMPF_PLL_AUDIO_8K;
        break;
    default:
        RTNA_DBG_Str0("Unsupported audio sample rate!\r\n");
        return MMP_AUDIO_ERR_PARAMETER;
        break;
    }

    if (path & MMP_AUD_AFE_IN_MASK)
        path = MMP_AUD_AFE_IN;
    else if (path & MMP_AUD_AFE_OUT_MASK)
        path = MMP_AUD_AFE_OUT;

    err = MMPF_PLL_GetGroupSrcAndDiv(CLK_GRP_AUD, &pll_src, &ubPllDiv);
    if (err != MMP_ERR_NONE) {
        RTNA_DBG_Str0("Get Audio group source failed!\r\n");
        return err;
    }

    err = MMPF_PLL_SetAudioPLL(MMPF_AUDSRC_MCLK, pll_mode, path);
    if (err != MMP_ERR_NONE) {
        RTNA_DBG_Str0("Set Audio PLL frequency failed!\r\n");
        return err;
    }
    /* _ulSamplerate = samplerate; */

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AFE_ISR
//  Description : AFE FIFO interrupt service routine
//------------------------------------------------------------------------------
/**
 @brief The interrupt service routine for the AFE FIFO interrupts

 @retval None.
*/
void MMPF_AFE_ISR(void)
{
    AITPS_AUD   pAUD = AITC_BASE_AUD;
    MMP_ULONG 	irq_src;
    AIT_REG_D   *fifo;
    #if (AUDIO_MIXER_EN == 0)
    MMP_USHORT  path;
    #endif

    /* DAC output */
    irq_src = pAUD->AFE_FIFO.DAC_MP.CPU_INT_EN & pAUD->AFE_FIFO.DAC_MP.SR;
    if (irq_src & AUD_INT_FIFO_REACH_UNWR_TH) {
        fifo = (AIT_REG_D *)&pAUD->AFE_FIFO.DAC_MP.DATA;
        MMPF_Audio_FifoOutHandler(fifo);
    }

    /* ADC input */
    irq_src = pAUD->ADC_FIFO.MP.CPU_INT_EN & pAUD->ADC_FIFO.MP.SR;
    if (irq_src & AUD_INT_FIFO_REACH_UNRD_TH) {
        fifo = (AIT_REG_D *)&pAUD->ADC_FIFO.MP.DATA;
        MMPF_Audio_FifoInHandler(fifo);
    }
    return;
}

/** @} */ // end of MMPF_AUDIO
