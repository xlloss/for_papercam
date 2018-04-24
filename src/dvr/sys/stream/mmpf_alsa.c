/**
 @file mmpf_alsa.c
 @brief Control functions of Audio ALSA Driver
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmu.h"
#include "ipc_cfg.h"
#include "mmpf_alsa.h"
#if (SPK_CTRL)
#include "mmpf_pio.h"
#endif

/** @addtogroup MMPF_ALSA
@{
*/

#if (SUPPORT_ALSA)
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
static MMPF_ALSA_CLASS  m_AlsaObj[ALSA_ID_NUM];
static MMP_ULONG        m_AlsaWrCnt = 0;
static MMPF_OS_MQID     m_AlsaMsgID = 0;
static void             *m_AlsaMsgQ[ALSA_ID_NUM][ALSA_OP_NUM];
static MMPF_ALSA_MSG    m_AlsaMsg[ALSA_ID_NUM][ALSA_OP_NUM];

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define ALSA_OBJ(id)    (&m_AlsaObj[id])
#define ALSA_ID(obj)    (obj - &m_AlsaObj[0])

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern void alsa_ipc_inframe_done(int samples);
extern void alsa_ipc_outframe_request(int samples);

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____ALSA_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize ALSA module

 @retval None.
*/
void MMPF_Alsa_Init(void)
{
    MMP_ULONG i;

    MMPF_ADC_Init();
    MMPF_DAC_Init();

    for(i = 0; i < ALSA_ID_NUM; i++) {
        m_AlsaObj[i].state      = ALSA_STATE_IDLE;
        m_AlsaObj[i].refcnt     = 0;
        m_AlsaObj[i].propt.fs   = 0;
        m_AlsaObj[i].propt.ch   = 2;
        m_AlsaObj[i].propt.thr  = 0;
        m_AlsaObj[i].propt.buf  = NULL;
        m_AlsaObj[i].adc        = NULL;
        m_AlsaObj[i].dac        = NULL;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Object
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the ALSA object with the spcified ID

 @param[in] id      ID of the associated interface

 @retval It return the ALSA object instance.
*/
MMPF_ALSA_CLASS *MMPF_Alsa_Object(MMPF_ALSA_ID id)
{
    if (id >= ALSA_ID_NUM) {
        RTNA_DBG_Str0("Invalid ALSA ID\r\n");
        return NULL;
    }

    return &m_AlsaObj[id];
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_TriggerOP
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Send a operation message to a ALSA interface with the spcified ID

 @param[in] id      ID of the associated interface
 @param[in] op      Operation
 @param[in] param   The associated parameter for the message

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Alsa_TriggerOP(MMPF_ALSA_ID id, MMPF_ALSA_OP op, void *param)
{
    MMPF_ALSA_MSG *msg = NULL;

    if ((id >= ALSA_ID_NUM) || (op >= ALSA_OP_NUM)) {
        RTNA_DBG_Str0("Invalid ALSA msg\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

    msg = &m_AlsaMsg[id][op];
    msg->id     = id;
    msg->op     = op;
    msg->param  = param;

    return MMPF_OS_PutMessage(m_AlsaMsgID, msg);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Control
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Control the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface
 @param[in] ctrl    ALSA control type
 @param[in] param   ALSA control parameters

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Alsa_Control(MMPF_ALSA_ID id, IPC_ALSA_CTL ctrl, void *param)
{
    MMP_ERR     err = MMP_ERR_NONE;
    MMP_BOOL    bKeepOnly = MMP_FALSE;
    MMP_ULONG   value = *(MMP_ULONG *)param;

    if (id >= ALSA_ID_NUM) {
        RTNA_DBG_Str(0, "Invalid ALSA param\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

#if 0
    /* Just keep settings, and apply it during enabling the interface */
    if ((m_AlsaObj[id].state != ALSA_STATE_OPEN) &&
        (m_AlsaObj[id].state != ALSA_STATE_ON)) {
        bKeepOnly = MMP_TRUE;
    }
#endif

    MMPF_Audio_CtrlClock(MMP_AUD_CLK_CORE, MMP_TRUE);

    switch(ctrl) {
    case IPC_ALSA_CTL_MUTE:
        if (id == ALSA_ID_MIC_IN)
            err = MMPF_Audio_SetADCMute(value, bKeepOnly);
        else
            err = MMPF_Audio_SetDACMute(value, bKeepOnly);
        break;
    case IPC_ALSA_CTL_A_GAIN:
        if (id == ALSA_ID_MIC_IN)
            err = MMPF_Audio_SetADCAnalogGain(value, bKeepOnly);
        else
            err = MMPF_Audio_SetDACAnalogGain(value, bKeepOnly);
        break;
    case IPC_ALSA_CTL_D_GAIN:
        if (id == ALSA_ID_MIC_IN)
            err = MMPF_Audio_SetADCDigitalGain(value, bKeepOnly);
        else
            err = MMPF_Audio_SetDACDigitalGain(value, bKeepOnly);
        break;
    default:
        return MMP_ALSA_ERR_UNSUPPORTED;
    }

    MMPF_Audio_CtrlClock(MMP_AUD_CLK_CORE, MMP_FALSE);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Read
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Read samples from the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface
 @param[in] samples Available samples in FIFO buffer

 @retval It reports the actual numb er of samples read.
*/
MMP_ULONG MMPF_Alsa_Read(MMPF_ALSA_ID id, MMP_ULONG samples)
{
    MMP_ULONG       free, ofst2end, thr, ch, readcnt = 0;
    MMP_SHORT       *buf;
    AUTL_RINGBUF    *alsa_buf;

    if (samples == 0)
        return 0;
    else if (id != ALSA_ID_MIC_IN)
        return 0;
    else if (m_AlsaObj[id].state != ALSA_STATE_ON)
        return samples;

    thr      = m_AlsaObj[id].propt.thr;
    ch       = m_AlsaObj[id].propt.ch;
    alsa_buf = m_AlsaObj[id].propt.buf;

    if (ch == 1) {
        samples = samples >> 1;
        thr     = thr >> 1;
    }

    /* align the samples in multiple of threshold */
    // smaples , and thr could also be variables, I can't adjust to constant.
    /* #if USE_DIV_LIB */
    /* struct libdivide_u64_t fast_d = libdivide_u64_gen(thr); */
    /* samples = libdivide_u64_do(samples,&fast_d) * thr; */
    /* #else */
    samples = (samples / thr) * thr;
    /* #endif */

    AUTL_RingBuf_SpaceAvailable(alsa_buf, &free);
    free = free >> 1;   // in unit of sample (16-bit)

    if (free >= samples) {
        ofst2end = (alsa_buf->size - alsa_buf->ptr.wr) >> 1;
        buf = (MMP_SHORT *)((MMP_ULONG)alsa_buf->buf_phy + alsa_buf->ptr.wr);

        if (ofst2end >= samples) {
            MMPF_Audio_FifoInRead(buf, samples, ch, 0);
        }
        else {
            MMPF_Audio_FifoInRead(buf, ofst2end, ch, 0);
            buf = (MMP_SHORT *)alsa_buf->buf_phy;
            MMPF_Audio_FifoInRead(buf, samples - ofst2end, ch, ofst2end);
        }
        AUTL_RingBuf_CommitWrite(alsa_buf, samples << 1);

        while(readcnt < samples) {

//            #if (ALSA_PWRON_PLAY == 0)
            /* Inform alsa driver via IPC interface */
            if (m_AlsaObj[id].propt.ipc)
                alsa_ipc_inframe_done(thr << 1);
//            #else
            else
                AUTL_RingBuf_CommitRead(alsa_buf, thr << 1);
//            #endif

            readcnt += thr;
        }

        if (ch == 1)
            readcnt = readcnt << 1;
    }
    else {
        RTNA_DBG_Str0("alsaOV\r\n");
    }

    return readcnt;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Write
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Write samples to the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface
 @param[in] space   Available space in unit of sample in FIFO buffer

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_Alsa_Write(MMPF_ALSA_ID id)
{
    MMPF_ALSA_CLASS *alsa = &m_AlsaObj[id];

    if (alsa->propt.ipc) {
        MMP_ULONG thr = alsa->propt.thr;

        alsa_ipc_outframe_request(thr << 1);
    }

    RTNA_DBG_Byte3(alsa->dac->buf->ptr.wr_wrap);
    RTNA_DBG_Byte3(alsa->dac->buf->ptr.rd_wrap);
    RTNA_DBG_Long3(alsa->dac->buf->ptr.wr);
    RTNA_DBG_Long3(alsa->dac->buf->ptr.rd);
    RTNA_DBG_Str3("\r\n");

    return MMP_ERR_NONE;
}

/*
 * AMP power on/down control
 */
#if (SPK_CTRL)
static void MMPF_Alsa_SpkCtrl(MMP_BOOL bPwrOn)
{
    if (bPwrOn) {
        MMPF_PIO_EnableOutputMode(SPK_CTRL_PIN, MMP_TRUE, MMP_FALSE);
        MMPF_PIO_SetData(SPK_CTRL_PIN, SPK_CTRL_ON_LVL, MMP_FALSE);
    }
    else {
        MMPF_PIO_SetData(SPK_CTRL_PIN, !SPK_CTRL_ON_LVL, MMP_FALSE);
        MMPF_PIO_EnableOutputMode(SPK_CTRL_PIN, MMP_FALSE, MMP_FALSE);
    }
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a ALSA interface with the specified ID & properties

 @param[in] id      ID of the associated interface
 @param[in] propt   Request properties

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Alsa_Open(MMPF_ALSA_ID id, MMPF_ALSA_PROPT *propt)
{
    MMP_ERR ret = MMP_ERR_NONE;
    MMPF_ALSA_CLASS *alsa = NULL;

    /* Parameters check */
    if ((id >= ALSA_ID_NUM) || !propt) {
        RTNA_DBG_Str(0, "Invalid ALSA param\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

    alsa = &m_AlsaObj[id];
    if (alsa->state == ALSA_STATE_ON) {
        /* The ALSA interface already open, all properties requested should
         * match to original one.
         */
        if ((propt->fs != alsa->propt.fs) ||
            (propt->ch != alsa->propt.ch) ||
            (propt->thr != alsa->propt.thr)) {
            return MMP_ALSA_ERR_STATE;
        }

        alsa->refcnt++;
        return MMP_ERR_NONE;
    }

    /* Open the ALSA interface with the specified properties */
    alsa->propt = *propt;

    if (id == ALSA_ID_MIC_IN) {
        /* Initialize ADC module, and turn on it */
        alsa->adc = MMPF_ADC_InitModule(gADCInPath, alsa->propt.fs);
        if (!alsa->adc) {
            RTNA_DBG_Str(0, "ADC_InitMod err\r\n");
            return MMP_ALSA_ERR_RESOURCE;
        }

        /* We don't pass the "thr" properties to ADC module.
         * Because we use the same rule as Linux to decide the threhsold:
         * For sample rate >= 32K, set FIFO interrupt interval to 4ms;
         * Otherwise, set FIFO interrupt interval to 8ms.
         */
        ret = MMPF_ADC_OpenModule(alsa->adc);
        if (ret) {
            RTNA_DBG_Str(0, "ADC_ModOpen:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            MMPF_ADC_UninitModule(alsa->adc);
            return ret;
        }
    }
    else if (id == ALSA_ID_SPK_OUT) {
        #if (SPK_CTRL)
        MMPF_Alsa_SpkCtrl(MMP_TRUE);
        #endif

        /* Initialize DAC module, and turn on it */
        alsa->dac = MMPF_DAC_InitModule(gDACOutPath, alsa->propt.fs);
        if (!alsa->dac) {
            RTNA_DBG_Str(0, "DAC_InitMod err\r\n");
            return MMP_ALSA_ERR_RESOURCE;
        }

        m_AlsaWrCnt = 0;

        /* We don't pass the "thr" properties to DAC module.
         * Because we use the same rule as Linux to decide the threhsold:
         * For sample rate >= 32K, set FIFO interrupt interval to 4ms;
         * Otherwise, set FIFO interrupt interval to 8ms.
         */
        alsa->dac->buf = alsa->propt.buf; // assign the buffer handle to DAC
        ret = MMPF_DAC_OpenModule(alsa->dac);
        if (ret) {
            RTNA_DBG_Str(0, "DAC_ModOpen:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            MMPF_DAC_UninitModule(alsa->dac);
            return ret;
        }
    }

    alsa->refcnt++;
    alsa->state = ALSA_STATE_OPEN;

    return ret;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Alsa_Close(MMPF_ALSA_ID id)
{
    MMP_ERR ret = MMP_ERR_NONE;
    MMPF_ALSA_CLASS *alsa = NULL;

    /* Parameters check */
    if (id >= ALSA_ID_NUM) {
        RTNA_DBG_Str(0, "Invalid ALSA param\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

    alsa = &m_AlsaObj[id];
    if ((alsa->state != ALSA_STATE_OPEN) && (alsa->state != ALSA_STATE_OFF)) {
        RTNA_DBG_Str(0, "Invalid ALSA state\r\n");
        return MMP_ALSA_ERR_STATE;
    }

    alsa->refcnt--;
    if (alsa->refcnt)
        return MMP_ERR_NONE;

    if (id == ALSA_ID_MIC_IN) {
        ret = MMPF_ADC_CloseModule(alsa->adc);
        if (ret) {
            RTNA_DBG_Str(0, "ADC_ModClose:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
        }
        ret = MMPF_ADC_UninitModule(alsa->adc);
        if (ret) {
            RTNA_DBG_Str(0, "ADC_UninitMod:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
        }
        alsa->adc = NULL;
    }
    else if (id == ALSA_ID_SPK_OUT) {
        ret = MMPF_DAC_CloseModule(alsa->dac);
        if (ret) {
            RTNA_DBG_Str(0, "DAC_ModClose:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
        }
        ret = MMPF_DAC_UninitModule(alsa->dac);
        if (ret) {
            RTNA_DBG_Str(0, "DAC_UninitMod:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
        }
        alsa->dac = NULL;

        #if (SPK_CTRL)
        MMPF_Alsa_SpkCtrl(MMP_FALSE);
        #endif
    }

    alsa->state = ALSA_STATE_IDLE;
    alsa->propt.buf = NULL;

    return ret;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_On
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start the FIFO interrupt of the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Alsa_On(MMPF_ALSA_ID id, MMP_ULONG period)
{
    MMP_ERR ret = MMP_ERR_NONE;
    MMPF_ALSA_CLASS *alsa = NULL;

    /* Parameters check */
    if (id >= ALSA_ID_NUM) {
        RTNA_DBG_Str(0, "Invalid ALSA param\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

    alsa = &m_AlsaObj[id];
    // so many check, hard to control
    // let it simple
    /*
    if (alsa->state != ALSA_STATE_OPEN) {
        RTNA_DBG_Str(0, "Invalid ALSA state\r\n");
        return MMP_ALSA_ERR_STATE;
    }
    */
    if (id == ALSA_ID_MIC_IN) {
        ret = MMPF_ADC_StartModule(alsa->adc);
        if (ret) {
            RTNA_DBG_Str(0, "ADC_ModOn:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            return ret;
        }
    }
    else if (id == ALSA_ID_SPK_OUT) {

        alsa->propt.period = period;
        ret = MMPF_DAC_StartModule(alsa->dac, period);
        if (ret) {
            RTNA_DBG_Str(0, "DAC_ModOn:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            return ret;
        }
        //2017_03_22 Power on amp
        #if (SPK_CTRL)
        MMPF_Alsa_SpkCtrl(MMP_TRUE);
        #endif
        
    }

    alsa->state = ALSA_STATE_ON;

    return ret;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Off
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the FIFO interrupt of the ALSA interface with the specified ID

 @param[in] id      ID of the associated interface

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_Alsa_Off(MMPF_ALSA_ID id)
{
    MMP_ERR ret = MMP_ERR_NONE;
    MMPF_ALSA_CLASS *alsa = NULL;

    /* Parameters check */
    if (id >= ALSA_ID_NUM) {
        RTNA_DBG_Str(0, "Invalid ALSA param\r\n");
        return MMP_ALSA_ERR_PARAMETER;
    }

    alsa = &m_AlsaObj[id];
    if (alsa->state != ALSA_STATE_ON) {
        RTNA_DBG_Str(0, "Invalid ALSA state\r\n");
        return MMP_ALSA_ERR_STATE;
    }

    if (id == ALSA_ID_MIC_IN) {
        ret = MMPF_ADC_StopModule(alsa->adc);
        if (ret) {
            RTNA_DBG_Str(0, "ADC_ModOff:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            return ret;
        }
    }
    else if (id == ALSA_ID_SPK_OUT) {
        //2017_03_22 Power off amp
        #if (SPK_CTRL)
        MMPF_Alsa_SpkCtrl(MMP_FALSE);
        #endif
    
        ret = MMPF_DAC_StopModule(alsa->dac);
        if (ret) {
            RTNA_DBG_Str(0, "DAC_ModOff:");
            RTNA_DBG_Long(0, ret);
            RTNA_DBG_Str(0, "\r\n");
            return ret;
        }
    }

    alsa->state = ALSA_STATE_OFF;

    return ret;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_TaksInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize routine in ALSA task startup.

 @retval None.
*/
static void MMPF_Alsa_TaksInit(void)
{
    MMPF_Alsa_Init();

    m_AlsaMsgID = MMPF_OS_CreateMQueue(m_AlsaMsgQ[0], ALSA_ID_NUM * ALSA_OP_NUM);
    if (m_AlsaMsgID >= MMPF_OS_MQID_MAX) {
        RTNA_DBG_Str0("Fail to create ALSA msgQ\r\n");
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Alsa_Task
//  Description :
//------------------------------------------------------------------------------
/**
 @brief ALSA task main routine

 @retval None.
*/
void MMPF_Alsa_Task(void)
{
    MMPF_ALSA_MSG *msg;

    RTNA_DBG_Str3("Alsa_Task()\r\n");

    MMPF_Alsa_TaksInit();

    while(1) {
        if (MMPF_OS_GetMessage(m_AlsaMsgID, (void **)&msg, 0) == 0) {
            switch(msg->op) {
            case ALSA_OP_OPEN:
                MMPF_Alsa_Open(msg->id, (MMPF_ALSA_PROPT *)msg->param);
                break;

            case ALSA_OP_TRIGGER_ON:
                MMPF_Alsa_On(msg->id, *(MMP_ULONG *)msg->param);
                break;

            case ALSA_OP_TRIGGER_OFF:
                MMPF_Alsa_Off(msg->id);
                break;

            case ALSA_OP_CLOSE:
                MMPF_Alsa_Close(msg->id);
                break;
            default:
                //
                break;
            }
        }
    }
}

#endif // (SUPPORT_ALSA)

/// @}
