//==============================================================================
//
//  File        : dualcpu_alsa.c
//  Description : ALSA interface for CPU B
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "mmu.h"
#include "cpucomm_bus.h"
#include "cpu_sharemem.h"
#include "dualcpu_alsa.h"
#include "mmpf_alsa.h"
#include "mmp_ipc_inc.h"
#if (SUPPORT_AEC)
#include "mmpf_aec.h"
#endif

#if (SUPPORT_ALSA)

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local variables
 */
static MMPF_OS_SEMID s_ulALSARcvSemId;  // Semaphore for critical section

static OS_STK s_ALSATaskStk[TASK_B_ALSA_STK_SIZE]; // Stack for service task

static alsa_ipc_info alsa_stream_info[ALSA_MAX_STREAMS];

static int alsa_ipc_afe_owner[ALSA_MAX_STREAMS] = {AFE_OWNER_IS_RTOS, };
static alsa_ipc_state alsa_state[ALSA_MAX_STREAMS] = {ALSA_IPC_STATE_CLOSE, };

static MMPF_ALSA_PROPT alsa_propt[ALSA_MAX_STREAMS];

static cpu_comm_transfer_data _ALSA_Proc;
static cpu_comm_transfer_data _ALSA_Return;
static MMP_LONG ALSA_PAYLOAD_INFO[4];

static int g_aec_work_status = 0 ;
static int g_audio_mode = AUDIO_ENH_NONE;
 
//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define GET_ALSA_ID(path) (path == ALSA_MIC) ? ALSA_ID_MIC_IN : ALSA_ID_SPK_OUT

//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================
#if (ALSA_RINGBUF_DBG)
static void alsa_ipc_dbg_bufinfo(ring_bufctl_t *pcmb)
{
	if (!pcmb)
        return;

    printc("dma_inbuf_virt_addr : 0x%08x\r\n", pcmb->dma_inbuf_virt_addr);
    printc("dma_outbuf_virt_addr: 0x%08x\r\n", pcmb->dma_outbuf_virt_addr);
    printc("dma_inbuf_phy_addr  : 0x%08x\r\n", pcmb->dma_inbuf_phy_addr);
    printc("dma_outbuf_phy_addr : 0x%08x\r\n", pcmb->dma_outbuf_phy_addr);
    printc("in_buf_h            : 0x%08x\r\n", &pcmb->in_buf_h);
    printc("out_buf_h           : 0x%08x\r\n", &pcmb->out_buf_h);
}

static void alsa_ipc_dbg_ring(AUTL_RINGBUF *ring)
{
	if (!ring)
        return;
    printc("ring        : 0x%08x\r\n", (unsigned long)ring);

    printc("buf        : 0x%08x\r\n", ring->buf);
    printc("buf_phy    : 0x%08x\r\n", ring->buf_phy);
    printc("size       : %d\r\n", ring->size);
    printc("ptr.rd     : %d\r\n", ring->ptr.rd);
    printc("ptr.wr     : %d\r\n", ring->ptr.wr);
    printc("ptr.rd_wrap: %d\r\n", ring->ptr.rd_wrap);
    printc("ptr.wr_wrap: %d\r\n", ring->ptr.wr_wrap);
}

static void alsa_ipc_dbg_dump(AUTL_RINGBUF *ring,int samples,int fill)
{
  int i;
  unsigned short *p ,*p1;
	if (!ring)
        return;
  p = (unsigned short *)DRAM_CACHE_VA( (unsigned long)(ring->buf_phy) );
  p1 = (unsigned short *)DRAM_NONCACHE_VA( (unsigned long)(ring->buf_phy) );
  
  for(i=0;i<samples;i++) {
    if(fill) {
      p1[i] = 0xffff;
    } else {
      printc("[%d] = %04x\r\n",i, p[i] );
    }  
  }         
}

#endif

/*
 * Switch AFE control to linux or rtos.
 * rtos firmware need to decide if can switch or not
 */
static int alsa_ipc_setowner(int aud_path, int owner)
{
    printc("alsa_setowner[%d]: %d\r\n", aud_path, owner);

    if (aud_path >= ALSA_SRC_END) {
        printc("Invalid alsa path\r\n");
        return -1;
    }

    alsa_ipc_afe_owner[aud_path] = owner;
    return 0;
}

static int alsa_ipc_poweron(alsa_ipc_info *ipc_info)
{
    ring_bufctl_t   *pcmb = (ring_bufctl_t *)
                            DRAM_NONCACHE_VA((MMP_ULONG)(ipc_info->pcm_buffer));
    alsa_audio_info *info = &ipc_info->aud_info;
    int             path  = info->aud_path;
    MMP_ULONG       buf_hdl;

    printc("alsa_poweron : %d,%d,%d\r\n", path,info->channels,info->sample_rate);
    #if 0
    printc("channels     : %d\r\n", info->channels);
    printc("sample_rate  : %d\r\n", info->sample_rate);
    printc("irq_threshold: %d\r\n", info->irq_threshold);
    printc("ipc_info     : x%x\r\n", ipc_info);
    printc("pcm_buffer   : x%x\r\n", pcmb);
    #endif
    
    #if (ALSA_RINGBUF_DBG)
    alsa_ipc_dbg_bufinfo(pcmb);
    #endif

    if (path >= ALSA_SRC_END) {
        printc("Invalid alsa path\r\n");
        return -1;
    }

    /* switch to non-cache address */
    ipc_info->pcm_buffer = pcmb;
    alsa_stream_info[path] = *ipc_info;

    buf_hdl = (MMP_ULONG)(&pcmb->in_buf_h);
    buf_hdl = DRAM_NONCACHE_VA(buf_hdl);

    /* ALSA properties */
    alsa_propt[path].fs     = info->sample_rate;
    alsa_propt[path].ch     = info->channels;
    alsa_propt[path].thr    = info->irq_threshold;
    alsa_propt[path].ipc    = MMP_TRUE;
    alsa_propt[path].buf    = (AUTL_RINGBUF *)buf_hdl;

    #if (ALSA_RINGBUF_DBG)
    
    alsa_ipc_dbg_ring(alsa_propt[path].buf);
    alsa_ipc_dbg_dump(alsa_propt[path].buf,16,1) ;
    #endif

    return 0;
}

static int alsa_ipc_poweroff(int aud_path)
{
    printc("alsa_poweroff: %d\r\n", aud_path);

    return 0;
}

static int alsa_ipc_trigger_start(int aud_path, int period)
{
    printc("alsa_start: %d,%d\r\n", aud_path, period);
    #if ALSA_RINGBUF_DBG
    alsa_ipc_dbg_ring(alsa_propt[aud_path].buf);
    alsa_ipc_dbg_dump(alsa_propt[aud_path].buf,2,0);
    #endif
    alsa_propt[aud_path].period = period;
    if(aud_path==ALSA_MIC) {           
    	//printc("ALSA MIC trigger start\r\n");
    }
    else {
      //printc("ALSA SPK trigger start\r\n");
      if(((g_audio_mode == AUDIO_ENH_AEC) || (g_audio_mode == AUDIO_ENH_AEC_DBG)) 
      && g_aec_work_status == 0 )
      {
          printc("ALSA SPK AEC Enable \r\n");
          #if SUPPORT_AEC
          MMPF_AEC_Enable();
          #endif		
          g_aec_work_status = 1;
      }
    }
    return 0;
}

static int alsa_ipc_trigger_stop(int aud_path)
{
    printc("alsa_stop: %d\r\n", aud_path);
    if(aud_path==ALSA_MIC) {        
        //printc("ALSA MIC trigger stop\r\n");        
    }
    else {
      //printc("ALSA SPK trigger stop\r\n");
      if(((g_audio_mode == AUDIO_ENH_AEC) || (g_audio_mode == AUDIO_ENH_AEC_DBG)) && g_aec_work_status == 1 ) 
      {
        //printc("ALSA SPK AEC Disable \r\n");
        #if SUPPORT_AEC
        MMPF_AEC_Disable();
        #endif		
        g_aec_work_status = 0;
      }        
    }
    return 0;
}

static int alsa_ipc_mute(int aud_path, int mute)
{
    MMP_ERR err = MMP_ERR_NONE;

    printc("alsa_mute[%d]: %d\r\n", aud_path, mute);

    err = MMPF_Alsa_Control(GET_ALSA_ID(aud_path), IPC_ALSA_CTL_MUTE, &mute);
    if (err) {
        printc("alsa mute failed\r\n");
        return -1;
    }

    return 0;
}

static int alsa_ipc_setgain(int aud_path, unsigned short dgain, unsigned short again)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMPF_ALSA_ID id;

    printc("alsa_setgain[%d]: dgain: x%x, again: x%x\r\n",
                                    aud_path, dgain, again);

    id = GET_ALSA_ID(aud_path);

    err = MMPF_Alsa_Control(id, IPC_ALSA_CTL_D_GAIN, &dgain);
    if (err) {
        printc("alsa d_gain failed\r\n");
    }
    err = MMPF_Alsa_Control(id, IPC_ALSA_CTL_A_GAIN, &again);
    if (err) {
        printc("alsa a_gain failed\r\n");
    }

    return 0;
}

static int alsa_ipc_set_aec(AUDIO_ENH_MODE mode)
{
    #if (SUPPORT_AEC)
    printc("set aec: %d\r\n", mode);

    MMPF_AEC_DebugEnable(0);

    if (mode == AUDIO_ENH_AEC) { 
        MMPF_AEC_Enable();
        g_audio_mode = AUDIO_ENH_AEC;
        g_aec_work_status = 1;
    }
    else if (mode == AUDIO_ENH_NONE) {
        MMPF_AEC_Disable();
        g_audio_mode = AUDIO_ENH_NONE;
        g_aec_work_status = 0;
    }
    else if (mode == AUDIO_ENH_AEC_DBG) {
        //TBD: more flag to set debug mode
        MMPF_AEC_DebugEnable(1);
        MMPF_AEC_Enable();
        g_audio_mode = AUDIO_ENH_AEC_DBG;
        g_aec_work_status = 1;
    }

    #else

    if (mode != AUDIO_ENH_NONE) {
        printc("aec not support %d\r\n", mode);
        return -1 ;
    }
    #endif

    return 0;
}

static void alsa_ipc_frame_done(int aud_path, int samples)
{
    struct cpu_share_mem_slot *slot;
    int channel = get_noack_channel(CPU_COMM_ID_ALSAB2A, 1);

    while(1) {
        if (get_slot(channel, &slot) == 0) {
            slot->dev_id        = CPU_COMM_ID_ALSAB2A;
            slot->command       = IPC_ALSA_ISR;
            slot->data_phy_addr = 0;
            slot->size          = 0;
            slot->send_parm[0]  = aud_path;
            slot->send_parm[1]  = samples;
            slot->send_parm[2]  = 0;
            slot->send_parm[3]  = 0;
            slot->ack_func      = 0;
            send_slot(channel, slot);
            break;
        }
        else {
        	printc("%s can't get slot\r\n", __func__);
        }
    }
}


static  unsigned int alsa_ipc_noack_cmd(void *slot)
{ 
  struct cpu_share_mem_slot *r_slot = (struct cpu_share_mem_slot *)slot ;
  int aud_path ;
  MMP_ERR err = MMP_ERR_NONE;
  if(! r_slot ) {
    printc("#unknow slot isr\n");
    return 0;  
  }
  aud_path = r_slot->send_parm[0] ;
  
  if(r_slot->dev_id==CPU_COMM_ID_ALSAA2B) {
    switch (r_slot->command) 
    {
        case IPC_ALSA_TRIGGER_STOP:
            if (alsa_state[aud_path] != ALSA_IPC_STATE_ON)
                break;

            if(aud_path==ALSA_MIC) {
                printc("ALSA MIC.stop\r\n");
            }
            else {
                if(((g_audio_mode == AUDIO_ENH_AEC) || (g_audio_mode == AUDIO_ENH_AEC_DBG)) && g_aec_work_status == 1 ) 
                {
                  printc("ALSA SPK AEC.Disable\r\n");
                  #if SUPPORT_AEC
                  MMPF_AEC_Disable();
                  #endif		
                  g_aec_work_status = 0;
                }        
                printc("ALSA SPK.stop\r\n");
            }
      

            err = MMPF_Alsa_TriggerOP(  GET_ALSA_ID(aud_path),ALSA_OP_TRIGGER_OFF,NULL);
            if (err)
                printc("alsa trigger off failed\r\n");
            else
                alsa_state[aud_path] = ALSA_IPC_STATE_OFF;
        
        break ;
    }
  }
  return 0;
}



void alsa_ipc_inframe_done(int samples)
{
    if (alsa_state[ALSA_MIC] == ALSA_IPC_STATE_ON)
        alsa_ipc_frame_done(ALSA_MIC, samples);
}

void alsa_ipc_outframe_request(int samples)
{
    if (alsa_state[ALSA_SPK] == ALSA_IPC_STATE_ON)
        alsa_ipc_frame_done(ALSA_SPK, samples);
}

/*
 * Main routine of ALSA IPC task
 */
void DualCpu_ALSATask(void* pData)
{
    MMP_ULONG swap_info_addr = DRAM_NONCACHE_VA((MMP_ULONG)ALSA_PAYLOAD_INFO);
    ALSA_COMMAND cmd;
    cpu_comm_transfer_data *ALSA_CPU_Reg = &_ALSA_Proc; 
    cpu_comm_transfer_data *ALSA_Return  = &_ALSA_Return;

   // printc("DualCpu_ALSA Start\r\n");
    RTNA_DBG_Str0("[ALSA]\r\n");
    
    CpuComm_RegisterEntry(CPU_COMM_ID_ALSAA2B, CPU_COMM_TYPE_SOCKET);
    //CpuComm_RegisterISRService(CPU_COMM_ID_ALSAA2B, 0);
    CpuComm_RegisterISRService(CPU_COMM_ID_ALSAA2B,alsa_ipc_noack_cmd );

    // Enable sender no ack cmd
    while(1)
    {
	    MMP_ULONG SocketRet = 0;
        MMP_ERR err;
        int ret = 0;
        int aud_path = 0, owner = 0, mute = 0;
        int period;
        int post_ret = 1; // return result after cmd done
        alsa_ipc_info *ipc_info;

        SocketRet = CpuComm_SocketReceive(  CPU_COMM_ID_ALSAA2B,
                                            (MMP_UBYTE *)ALSA_CPU_Reg,
                                            sizeof(cpu_comm_transfer_data),
                                            MMP_TRUE);

        if (SocketRet != CPU_COMM_ERR_NONE) {
            printc("SocketRet: %x\r\n", SocketRet);
            continue;
        }

        ALSA_CPU_Reg->phy_addr = DRAM_NONCACHE_VA(ALSA_CPU_Reg->phy_addr);
        ALSA_Return->phy_addr  = swap_info_addr;

        cmd = ALSA_CPU_Reg->command;
        ALSA_Return->command = cmd;

        switch(cmd) {
        case IPC_ALSA_SETOWNER:
            aud_path = ALSA_CPU_Reg->phy_addr; 
            owner = ALSA_CPU_Reg->size;

            ret = alsa_ipc_setowner(aud_path, owner);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_POWERON:
            ipc_info = (alsa_ipc_info *)ALSA_CPU_Reg->phy_addr;
            aud_path = ipc_info->aud_info.aud_path; 

            ret = alsa_ipc_poweron(ipc_info);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_POWEROFF:
            aud_path = ALSA_CPU_Reg->phy_addr;

            ret = alsa_ipc_poweroff(aud_path);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_TRIGGER_START:
            aud_path = ALSA_CPU_Reg->phy_addr; 
            period = ALSA_CPU_Reg->size;

            ret = alsa_ipc_trigger_start(aud_path, period);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_TRIGGER_STOP:
            aud_path = ALSA_CPU_Reg->phy_addr; 

            ret = alsa_ipc_trigger_stop(aud_path);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_MUTE:
            aud_path = ALSA_CPU_Reg->phy_addr; 
            mute = ALSA_CPU_Reg->size;

            ret = alsa_ipc_mute(aud_path, mute);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_GAIN:
            aud_path = ALSA_CPU_Reg->phy_addr;

            ret = alsa_ipc_setgain( aud_path,
                                    ALSA_DGAIN(ALSA_CPU_Reg->size),
                                    ALSA_AGAIN(ALSA_CPU_Reg->size));
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_RW:
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        case IPC_ALSA_SET_AEC:
            ret = alsa_ipc_set_aec((AUDIO_ENH_MODE)ALSA_CPU_Reg->phy_addr);
            ALSA_Return->result     = ret;
            ALSA_Return->phy_addr   = 0;
            ALSA_Return->size       = 0;
            break;

        default:
            printc("no such command\r\n");
            ALSA_Return->command = IPC_ALSA_NOT_SUPPORT;
            break;
        }

        if (post_ret) {
            if (ALSA_Return->result < 0) {
                printc("CMD %d, err: 0x%x\r\n",
                                        ALSA_CPU_Reg->command,
                                        ALSA_Return->result);
            }

            ALSA_Return->flag = CPUCOMM_FLAG_CMDSND;

            // cpu_a didnot know the cpub's noncachable address
            // so go back to cachable address 
            ALSA_Return->phy_addr = DRAM_CACHE_VA(ALSA_Return->phy_addr);
            CpuComm_SocketSend( CPU_COMM_ID_ALSAB2A,
                                (MMP_UBYTE *)ALSA_Return,
                                sizeof(cpu_comm_transfer_data));
        }

        switch(cmd) {
        case IPC_ALSA_POWERON:
            if (alsa_state[aud_path] != ALSA_IPC_STATE_CLOSE)
                break;

            err = MMPF_Alsa_TriggerOP(  GET_ALSA_ID(aud_path),
                                        ALSA_OP_OPEN,
                                        &alsa_propt[aud_path]);
            if (err)
                printc("alsa power on failed\r\n");
            else
                alsa_state[aud_path] = ALSA_IPC_STATE_OPEN;
            break;

        case IPC_ALSA_POWEROFF:
            if ((alsa_state[aud_path] != ALSA_IPC_STATE_OFF) &&
                (alsa_state[aud_path] != ALSA_IPC_STATE_OPEN))
                break;

            err = MMPF_Alsa_TriggerOP(  GET_ALSA_ID(aud_path),
                                        ALSA_OP_CLOSE,
                                        NULL);
            if (err)
                printc("alsa power on failed\r\n");
            else
                alsa_state[aud_path] = ALSA_IPC_STATE_CLOSE;
            break;

        case IPC_ALSA_TRIGGER_START:
            if ( (alsa_state[aud_path] != ALSA_IPC_STATE_OPEN ) &&
                 (alsa_state[aud_path] != ALSA_IPC_STATE_OFF))
                break;

            err = MMPF_Alsa_TriggerOP(  GET_ALSA_ID(aud_path),
                                        ALSA_OP_TRIGGER_ON,
                                        &alsa_propt[aud_path].period);
            if (err)
                printc("alsa trigger on failed\r\n");
            else
                alsa_state[aud_path] = ALSA_IPC_STATE_ON;
            break;

        case IPC_ALSA_TRIGGER_STOP:
            if ( (alsa_state[aud_path] != ALSA_IPC_STATE_OPEN) &&
                 (alsa_state[aud_path] != ALSA_IPC_STATE_ON) )
                break;

            err = MMPF_Alsa_TriggerOP(  GET_ALSA_ID(aud_path),
                                        ALSA_OP_TRIGGER_OFF,
                                        NULL);
            if (err)
                printc("alsa trigger off failed\r\n");
            else
                alsa_state[aud_path] = ALSA_IPC_STATE_OFF;
            break;
        default:
            //
            break;
        }
    }

    printc("Task Stop\r\n");
    // Kill task by itself
    MMPF_OS_DeleteTask(OS_PRIO_SELF);
}

/*
 * Create the task for handle ALSA IPC
 */
static void EstablishALSA(void)
{
	MMPF_TASK_CFG	task_cfg;

	task_cfg.ubPriority = TASK_B_ALSA_PRIO;
	task_cfg.pbos = (MMP_ULONG)&s_ALSATaskStk[0];
	task_cfg.ptos = (MMP_ULONG)&s_ALSATaskStk[TASK_B_ALSA_STK_SIZE-1];
	MMPF_OS_CreateTask(DualCpu_ALSATask, &task_cfg, (void *)0);
}

//------------------------------------------------------------------------------
//  Function    : CpuB_ALSAInit
//  Description : CPU_B Alsa module initialization routine
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuB_ALSAInit(void)
{
	  //RTNA_DBG_Str(0, "[ALSA]\r\n");

    s_ulALSARcvSemId = MMPF_OS_CreateSem(1);
    EstablishALSA();

    return CPU_COMM_ERR_NONE;
}
CPUCOMM_MODULE_INIT(CpuB_ALSAInit)

#endif //(SUPPORT_ALSA)
