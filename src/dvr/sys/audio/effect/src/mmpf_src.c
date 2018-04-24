/**
 @file mmpf_src.c
 @brief Control functions of Sample rate converter (SRC)
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmpf_system.h"
#include "mmpf_audio_ctl.h"
#include "mmpf_src.h"
#include "mmu.h"

#if (SRC_SUPPORT)

/** @addtogroup MMPF_AEC
@{
*/

#define SRC_MAX_INSTANCE      (2)
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
static MMPF_SRC_CLASS       m_SrcObj[SRC_MAX_INSTANCE];

//static MMP_SHORT  m_SrcOutBuf[2048] __attribute__((section(".audiofifobuf")));
static MMP_SHORT  *m_SrcOutBuf[SRC_MAX_INSTANCE] ;
/*
32KB only for 8KHz upsample to 16KB
*/
//static MMP_UBYTE  m_SrcWorkingBuf[SRC_WORKING_BUF_SIZE] __attribute__((section(".audiofifobuf")));

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
void _____SRC_INTERNAL_INTERFACE_____(){}
#endif


//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for ARC.
*/
static MMP_ERR MMPF_SRC_ReserveBuf(int id)
{
    MMPF_SRC_CLASS *obj ;
    obj = &m_SrcObj[id] ;
    obj->workbuf.size = IaaSrc_GetBufferSize( SRC_8k_to_16k );
    obj->workbuf.base = MMPF_SYS_HeapMalloc(SYS_HEAP_SRAM,obj->workbuf.size, 16);
    m_SrcOutBuf[id]   = (MMP_SHORT *)MMPF_SYS_HeapMalloc(SYS_HEAP_SRAM,4096,16);
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize SRC module.
 
 @retval It reports the status of the operation.
*/
int MMPF_SRC_ModInit(void)
{
    int i ;
    for(i=0;i < SRC_MAX_INSTANCE ;i++) {
      if (m_SrcObj[i].mod_init == MMP_FALSE) {
          MMP_ERR err = MMP_ERR_NONE;
          MEMSET(&m_SrcObj[i], 0, sizeof(MMPF_SRC_CLASS));
          err = MMPF_SRC_ReserveBuf(i); // Reserve heap buffer for AEC engine
          if (err)
              return err;
          m_SrcObj[i].mod_init = MMP_TRUE;
      }
    }
    return 0;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_LibInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize SRC library.
 
 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_SRC_LibInit(int id,int channels)
{
    int ret;
    char *workingbuf;
    if( id < SRC_MAX_INSTANCE) {
      //printc("[SRC] : id=%d,t1:%d\r\n", id ,OSTime );  
      m_SrcObj[id].ap_src.WaveIn_srate = SRATE_8K;
      m_SrcObj[id].ap_src.point_number = SRC_BLOCK_SIZE; 
      m_SrcObj[id].ap_src.mode = SRC_8k_to_16k ;
      m_SrcObj[id].ap_src.channel = channels; 
      workingbuf = (char *)m_SrcObj[id].workbuf.base;
      m_SrcObj[id].handle = IaaSrc_Init(workingbuf, &m_SrcObj[id].ap_src);
      if (!m_SrcObj[id].handle)
          return MMP_AUDIO_ERR_INSUFFICIENT_BUF;
      //printc("[SRC] : id=%d,t2:%d\r\n", id , OSTime);  
    }
    return MMP_ERR_NONE;
}

#if 0
void _____SRC_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize SRC engine.

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_SRC_Init(int id ,int channels)
{
    MMP_ERR     err;
    MMP_ULONG   buf;

    err = MMPF_SRC_ModInit();
    if (err) {
        RTNA_DBG_Str0("#SRC mod init err\r\n");
        return err;
    }
    if(id < SRC_MAX_INSTANCE) {
      err = MMPF_SRC_LibInit(id,channels);
      if (err) {
          RTNA_DBG_Str0("#SRC lib init err\r\n");
          return err;
      }
      m_SrcObj[id].enable = MMP_TRUE ;
      //printc("[SRC] : Init id : %d\r\n",id);
    }
    return MMP_ERR_NONE;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_Disable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set SRC engine function disabled.

 @retval None.
*/
void MMPF_SRC_Disable(int id)
{
    if(id < SRC_MAX_INSTANCE) {
        m_SrcObj[id].enable = MMP_FALSE;
        if( m_SrcObj[id].handle ) {
            IaaSrc_Release(m_SrcObj[id].handle);
            m_SrcObj[id].handle = 0;
            printc("[SRC] : Free id : %d\r\n",id);
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_IsEnable
//  Description :
//------------------------------------------------------------------------------
/**
 @brief The function returns whether SRC engine function is enabled or not.

 @retval MMP_TRUE is SRC is enable, otherwise, returns MMP_FALSE.
*/
MMP_BOOL MMPF_SRC_IsEnable(int id)
{
    return m_SrcObj[id].enable;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SRC_Process
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start SRC processing.
 @retval None.
*/
MMP_SHORT *MMPF_SRC_Process(int id,MMP_SHORT *in, int samples,int *out_samples)
{
//static MMP_SHORT src_out[2048] ;
static MMP_ULONG max_t ;    
    MMP_ULONG t1,t2 ;

  int i,loop = samples / SRC_BLOCK_SIZE , done_samples,frame_size ;
  MMP_SHORT *ptr_out = m_SrcOutBuf[id] ;
  MMP_SHORT *ptr_in  = in ,l,r;
  *out_samples = 0 ;
  frame_size = SRC_BLOCK_SIZE / m_SrcObj[id].ap_src.channel ;

#if 1
  t1 = OSTime ;
  
  for(i=0;i<loop;i++) {
    done_samples = IaaSrc_Run(m_SrcObj[id].handle, ptr_in,ptr_out,frame_size);  
    *out_samples += done_samples ;
    ptr_in += SRC_BLOCK_SIZE ;
    ptr_out += done_samples ;
  }
	t2 = OSTime - t1 ;
	if( t2 > max_t) {
	  max_t = t2 ;
  	printc("-src.t : %d\r\n",max_t );
	}
#else
  // duplicate only
  for ( i= 0 ; i < (samples >> 1)  ;i++) {
    l = ptr_in[i*2 + 0] ;
    r = ptr_in[i*2 + 1] ;
    ptr_out[ i* 4 + 0 ] = l ;
    ptr_out[ i* 4 + 1 ] = r ;
    ptr_out[ i* 4 + 2 ] = l ;
    ptr_out[ i* 4 + 3 ] = r ;
    
  }
  *out_samples = samples << 1 ;
#endif  
  return m_SrcOutBuf[id];
}


ait_module_init(MMPF_SRC_ModInit);

/// @}

#endif // (SUPPORT_AEC)
