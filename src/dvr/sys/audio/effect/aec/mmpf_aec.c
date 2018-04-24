/**
 @file mmpf_aec.c
 @brief Control functions of Acoustic Echo Cancellation (AEC)
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmpf_system.h"
#include "mmpf_audio_ctl.h"
#include "mmpf_aec.h"
#include "mmu.h"
#include "AudioAecProcess.h"
#include "AudioProcess.h"

#if (USER_STRING)
extern int sprintf(char *out, const char *format, ...);
#endif

#if (SUPPORT_AEC)
#define RUN_AEC (1)
#define RUN_NR  (0)
#define RUN_AGC (1)
#define DB_BOUND (-30)

//could not be chaned smaller
#define WORKING_BUFER_SIZE_APC	    (1024*100)
// AEC need bigger working buffer size
#define WORKING_BUFER_SIZE_AEC	    (1024*500)


/** @addtogroup MMPF_AEC
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
static MMPF_AEC_CLASS       m_AecObj = {
    MMP_FALSE,      /* mod_init */
    MMP_FALSE,      /* enable */
};

/* #pragma arm section zidata = "audiofifobuf" */
static MMP_SHORT  m_AecSpkBuf[AEC_SPK_BUF_SIZE] __attribute__((section(".audiofifobuf")));
/* #pragma arm section zidata */

#if 1// (AEC_FUNC_DBG)
static MMP_SHORT  m_AecDbgBuf[AEC_BLOCK_SIZE]  __attribute__((section(".audiofifobuf")));
#endif
static MMP_SHORT  *m_AecMonoBuf = m_AecDbgBuf ;
static MMP_SHORT  m_AecSpkProcBuf[AEC_BLOCK_SIZE]  __attribute__((section(".audiofifobuf")));

static MMP_SHORT m_AecProcCh = 2 ;


//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================


//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================

#if 0
void _____AEC_INTERNAL_INTERFACE_____(){}
#endif


/* log functions for AEC library, please porting for it */
void IaaAec_Printf(int x, char* y)
{
       printc("%s \n", y);
}

/* log functions for NR library, please porting for it */
void IaaApc_Printf(int x, char* y)
{
       printc("%s \n", y);
}

void IaaAgc_Printf(int x, char* y)
{
       printc("%s \n", y);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for AEC.
*/
static MMP_ERR MMPF_AEC_ReserveBuf(void)
{
    MMPF_AEC_CLASS *obj = &m_AecObj;

    obj->workbuf.size = WORKING_BUFER_SIZE_AEC + WORKING_BUFER_SIZE_APC;
    obj->workbuf.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                         obj->workbuf.size, 16);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize AEC module.
 
 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_AEC_ModInit(void)
{
    if (m_AecObj.mod_init == MMP_FALSE) {
        MMP_ERR err = MMP_ERR_NONE;

        MEMSET(&m_AecObj, 0, sizeof(MMPF_AEC_CLASS));

        err = MMPF_AEC_ReserveBuf(); // Reserve heap buffer for AEC engine
        if (err)
            return err;

        m_AecObj.mod_init = MMP_TRUE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_LibInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize AEC library.
 
 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_AEC_LibInit(void)
{
    int ret;
    char *buf_aec,*buf_apc;
    printc("AEC.Proc : %d,t1 :%d\r\n",m_AecProcCh ,OSTime );
    
//aec, nr algorithm. we only can run one of them in the same time.
#if  (RUN_AEC == 1)
    m_AecObj.ap_aec.point_number = (AEC_BLOCK_SIZE >> 1); // per channel
    m_AecObj.ap_aec.nearend_channel = m_AecProcCh;
    m_AecObj.ap_aec.farend_channel  = m_AecProcCh;

    buf_aec = (char *)(DRAM_CACHE_VA(m_AecObj.workbuf.base));
    buf_apc = buf_aec + WORKING_BUFER_SIZE_AEC ;
    ret = IaaAec_Init(buf_aec, &m_AecObj.ap_aec);
    if (ret)
        return MMP_AUDIO_ERR_INSUFFICIENT_BUF;

    IaaAec_SetGenDRInputBufEn(0);
    IaaAec_SetDRInputBufAddress(0,0);
    //only can set 0-2, higher number higher NR effect, but could induce to side effect
    IaaAec_SetMode(AEC_LEVEL_2);

#endif

#if RUN_NR || RUN_AGC
    m_AecObj.ap_nragc.point_number = APC_BLOCK_SIZE >> 1 ;
    m_AecObj.ap_nragc.channel = m_AecProcCh ;
    m_AecObj.ap_nragc.beamforming_en = 0 ;
    IaaApc_Init((char *)buf_apc, &m_AecObj.ap_nragc);
#if RUN_NR    
    //set 0 will disable the APC
    IaaApc_SetNrEnable(1);
#else
    IaaApc_SetNrEnable(0);
#endif    
    ////only can set 0-2, higher number higher NR effect, but could induce to side effect
    IaaApc_SetNrMode(NR_LEVEL_2);
    ////set 0 will disble the dereverbberation
    IaaApc_SetDereverbEnable(1);
    IaaApc_BypassRightChannel(0);

    ////for AGC
    IaaApc_SetAgcTargetPowerIndB(-9); //-9
    ////set 0 will disable the AGC
#if RUN_AGC
    IaaApc_SetAgcEnable(1);
    ////if the voice db under -30 db, it will not be done AGC process
    //IaaApc_SetAgcNoiseGateEnable(1,DB_BOUND);
#else
    IaaApc_SetAgcEnable(0);

#endif
    
#endif

    printc("AEC.End t2 :%d\r\n",OSTime );

    return MMP_ERR_NONE;
}

#if 0
void _____AEC_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize AEC engine.

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_AEC_Init(void)
{
    MMP_ERR     err;
    MMP_ULONG   buf;

    err = MMPF_AEC_ModInit();
    if (err) {
        RTNA_DBG_Str0("#AEC mod init err\r\n");
        return err;
    }
    err = MMPF_AEC_LibInit();
    if (err) {
        RTNA_DBG_Str0("#AEC lib init err\r\n");
        return err;
    }

    buf = (MMP_ULONG)m_AecSpkBuf;
    AUTL_RingBuf_Init(&m_AecObj.spk_ring, buf, sizeof(m_AecSpkBuf));

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_Enable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set AEC engine function enabled.

 @retval None.
*/
void MMPF_AEC_Enable(void)
{
    if (!m_AecObj.enable) {
        MMPF_AEC_Init();
        m_AecObj.enable = MMP_TRUE;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_Disable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set AEC engine function disabled.

 @retval None.
*/
void MMPF_AEC_Disable(void)
{
    if (m_AecObj.enable)
        m_AecObj.enable = MMP_FALSE;
}

void MMPF_AEC_Pause(MMP_BOOL pause)
{
  m_AecObj.pause = pause ;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_IsEnable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief The function returns whether AEC engine function is enabled or not.

 @retval MMP_TRUE is AEC is enable, otherwise, returns MMP_FALSE.
*/
MMP_BOOL MMPF_AEC_IsEnable(void)
{
    return m_AecObj.enable;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_DebugEnable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief The function assigns debug mode to apply AEC to R channel only.

 @retval None.
*/
void MMPF_AEC_DebugEnable(MMP_BOOL en)
{
    m_AecObj.dbg_mode = en;
}

void MMPF_AEC_SetProcCh(MMP_SHORT ch)
{
  if(MMPF_ADC_GetInitCh() == MMP_AUD_LINEIN_DUAL ) {
    m_AecProcCh = 2 ;
  }
  else {
    if( (ch <=2  ) && (ch >= 1) ) {
      m_AecProcCh = ch ;
    }
  }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_Process
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start AEC processing.

 @retval None.
*/
void MMPF_AEC_Process(void)
{
    int ret;
    MMP_SHORT *mic_in, *spk_out ,*mic_in_proc ,*spk_out_proc;
    MMP_ULONG available;
    #if (AEC_FUNC_DBG)
    MMP_ULONG i;
    #endif

    MMP_ULONG t1,t2 ;
static MMP_ULONG max_t ;    
    if (!m_AecObj.enable)
        return;

    AUTL_RingBuf_DataAvailable(&m_AecObj.spk_ring, &available);
    if ((available >> 1) < AEC_BLOCK_SIZE)
        return;

    mic_in  = (MMP_SHORT *)(MMPF_Audio_FifoIn());
    spk_out = (MMP_SHORT *)(m_AecObj.spk_ring.buf + m_AecObj.spk_ring.ptr.rd);
    
    mic_in_proc = mic_in;
    spk_out_proc = spk_out ;
    
    #if 1// (AEC_FUNC_DBG)
    if(m_AecProcCh==2) {
      #if (AEC_FUNC_DBG)
      if (m_AecObj.dbg_mode  ) {
          for(i = 0; i < AEC_BLOCK_SIZE; i += 2) {
              m_AecDbgBuf[i] = mic_in[i];
          }
      }
      #endif      
    }
    else {
      for(i = 0; i < AEC_BLOCK_SIZE; i += 2) {
        m_AecMonoBuf[i>>1]  = mic_in[i];
        m_AecSpkProcBuf [i>>1]  = spk_out[i] ;
      }
      mic_in_proc  = m_AecMonoBuf;
      spk_out_proc = m_AecSpkProcBuf ;
    }
    #endif
   
  	#if  (RUN_AGC == 1)
  	// do one frame automatic gain control, sustain voice level
  	ret = IaaApc_AgcRun(mic_in_proc);
    if (ret) {
          RTNA_DBG_Str0("#AEC agc err");
    }
    
    #endif
  
    #if  (RUN_AEC == 1)
    //printc("-AEC\r\n");
  	t1 = OSTime ;
  	if(m_AecObj.pause ) {
  	  ret = 0 ;
  	}
  	else {
      ret = IaaAec_Run(mic_in_proc, spk_out_proc);
    }
  	t2 = OSTime - t1 ;
  	if( t2 > max_t) {
  	  max_t = t2 ;
    	printc("-aec.t : %d\r\n",max_t );
  	}
    if (ret) {
        RTNA_DBG_Str0("#AEC proc err");
    }
    #endif

    #if (RUN_NR == 1)
    ret = IaaApc_Run(mic_in_proc);
    if (ret) {
        RTNA_DBG_Str0("#AEC nr err");
    }
    #endif

    #if 1//(AEC_FUNC_DBG)
    if(m_AecProcCh==2) {
      #if (AEC_FUNC_DBG)
      if (m_AecObj.dbg_mode) {
          for(i = 0; i < AEC_BLOCK_SIZE; i += 2) {
              mic_in[i] = m_AecDbgBuf[i];
          }
      }
      #endif
    }
    else {
      #if AEC_FUNC_DBG
      if ( m_AecObj.dbg_mode ) {
        for(i = 0; i < AEC_BLOCK_SIZE; i += 2) {
          mic_in[i] = m_AecMonoBuf[ i >> 1 ];
        }  
      }
      else 
      #endif
      {
        MMP_SHORT *ptr = mic_in ;
        for(i = 0; i < AEC_BLOCK_SIZE >> 1; i++ ) {
          *ptr++ = m_AecMonoBuf[ i ]; 
          *ptr++ = m_AecMonoBuf[ i ]; 
        } 
      }
    }
    
    #endif

    AUTL_RingBuf_CommitRead(&m_AecObj.spk_ring, AEC_BLOCK_SIZE << 1);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_AEC_TxReady
//  Description : Tx direction data to send
//------------------------------------------------------------------------------
/**
 @brief Keep the samples going to be sent out.

 @param[in] buf     PCM data samples
 @param[in] amount  Sample data available for all channel(s) to send out
 @param[in] ch      Number of data output channel

 @retval None.
*/
void MMPF_AEC_TxReady(MMP_SHORT *buf, MMP_ULONG amount, MMP_ULONG ch)
{
    MMP_ULONG       i, loop_cnt = 0, wrap_size;
    MMP_SHORT       *spk_out;
    AUTL_RINGBUF    *ring;

    if (!m_AecObj.enable)
        return;

    ring = &m_AecObj.spk_ring;

    while(amount) {
        MMP_ULONG tx_size;

        spk_out = (MMP_SHORT *)(ring->buf + ring->ptr.wr);
        wrap_size = (ring->size - ring->ptr.wr) >> 1;

        tx_size = (amount > wrap_size) ? wrap_size : amount;

        if (ch == 2)
            loop_cnt = tx_size >> 2;
        else if (ch == 1)
            loop_cnt = tx_size >> 1;

        if (!buf) {
            MEMSET(spk_out, 0, tx_size << 1);
            AUTL_RingBuf_CommitWrite(ring, tx_size << 1);
        }
        else {
            if (ch == 2) {
                for(i = 0; i < loop_cnt; i++) {
                    *spk_out++ = *buf++;
                    *spk_out++ = *buf++;
                    *spk_out++ = *buf++;
                    *spk_out++ = *buf++;
                }
            }
            else if (ch == 1) {
                for(i = 0; i < loop_cnt; i++) {
                    *spk_out++ = *buf;
                    *spk_out++ = *buf++;
                    *spk_out++ = *buf;
                    *spk_out++ = *buf++;
                }
            }
            AUTL_RingBuf_CommitWrite(ring, loop_cnt << 3);
        }

        amount -= tx_size;
    }
}
/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall"  */
/* #pragma O0 */
ait_module_init(MMPF_AEC_ModInit);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

/// @}

#endif // (SUPPORT_AEC)
