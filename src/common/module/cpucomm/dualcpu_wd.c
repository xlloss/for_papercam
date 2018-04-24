//==============================================================================
//
//  File        : dualcpu_uart.c
//  Description : UART wrapper for CPU B
//  Author      : Chiket Lin
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
#include "mmp_ipc_inc.h"
#include "mmpf_wd.h"
#include "mmp_reg_wd.h"
#include "os_wrap.h"

#define AUTO_START_WD   (1)
#define CONFIG_AIT_WATCHDOG_DEFAULT_TIME	(10) 

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
static void EstablishWD(void);
//==============================================================================
//
//                              Macro & Define
//
//==============================================================================


//==============================================================================
//
//                        Static or Global Variables
//
//==============================================================================

static OS_STK  s_WDTaskStk[TASK_B_WD_STK_SIZE];   // Stack for service task

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           Init & Exit dual MD                            //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
//  Function    : CpuB_UartInit
//  Description : Construct callback which will be call when event flag of comm
//                is created
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuB_WDInit(void)
{
	EstablishWD();
	return CPU_COMM_ERR_NONE;
}

CPUCOMM_MODULE_INIT(CpuB_WDInit)


static cpu_comm_transfer_data _WD_Proc ;
static cpu_comm_transfer_data _WD_Return;

static int wd_debug = 0;
extern void CpuB_SysReboot(int ms);

static unsigned int wd_start_time ,wd_start_time_base;
static int wd_start ,wd_timeout = CONFIG_AIT_WATCHDOG_DEFAULT_TIME;
static MMPF_OS_TMRID wd_timer = (MMPF_OS_TMRID)-1;

int cpub_wd_ping_noack(void)
{
    int channel = 1;
    struct cpu_share_mem_slot *S_slot;
    while(1) {
      if (get_slot(channel, &S_slot) == 0) {
      	S_slot->dev_id  = CPU_COMM_ID_WD;
      	S_slot->command = WD_PING;
      	S_slot->data_phy_addr = 0;
      	S_slot->size = 0;
      	S_slot->send_parm[0] = 0;
      	S_slot->send_parm[1] = 0;
      	S_slot->send_parm[2] = 0;
      	S_slot->send_parm[3] = 0;
      	S_slot->ack_func = 0;
      	send_slot(channel, S_slot);
      	break;
      }
      else { 
      	printc("%s can't get slot\r\n", __func__);
      }
    }
    return 0;
}

static void wd_timer_cb(void *ptmr, void *parg)
{
  //
  if(!wd_start) {
    return ;
  }
  wd_start_time+=1000;
  if( (wd_start_time - wd_start_time_base) >= 1000*wd_timeout ) {
    printc("wd reboot\r\n");
    CpuB_SysReboot(500);
  }
}

static void WD_Start(void)
{
  AITPS_GBL   pGBL 	= AITC_BASE_GBL;
  printc("WD_Start\r\n");
  wd_start_time_base = OSTime ;
  wd_start_time = wd_start_time_base ;
  wd_start = 1 ;
  if(wd_timer == (MMPF_OS_TMRID)-1) {
    wd_timer = MMPF_OS_StartTimer(1000,MMPF_OS_TMR_OPT_PERIODIC,wd_timer_cb,0); 
  }
}

static void WD_Stop(void)
{
  printc("WD_Stop\r\n");
  wd_start = 0 ;
  MMPF_OS_StopTimer(wd_timer,MMPF_OS_TMR_OPT_PERIODIC) ;
  wd_timer = (MMPF_OS_TMRID)-1;
}

static void WD_Ping(void)
{
  //printc("[WD PING]\r\n");
  wd_start_time = wd_start_time_base ;
  //cpub_wd_ping_noack();
}

static int test_wd = 0;
static void WD_Kick(void)
{
  wd_start_time = wd_start_time_base ;
  cpub_wd_ping_noack();
}

static void WD_SetTimeout(unsigned int c)
{
  printc("WD_SetT %d\r\n",c);
  wd_timeout = c ;
}

unsigned int cpub_wd_handler(void *slot) 
{
  struct cpu_share_mem_slot *r_slot = (struct cpu_share_mem_slot *)slot ;
  
  if(! r_slot ) {
    printc("#unknow slot isr\r\n");
    return 0;  
  }
  if(r_slot->dev_id==(unsigned int)CPU_COMM_ID_WD) {
    switch (r_slot->command) 
    {
      case WD_PING:
      WD_Kick();
      break ;
    }
  }
  return 0;
      
}

void DualCpu_WDTask(void* pData)
{
	cpu_comm_transfer_data *WD_CPU_Reg = &_WD_Proc; 
	cpu_comm_transfer_data *WD_Return  = &_WD_Return;
	RTNA_DBG_Str0("[WD]\r\n");
	CpuComm_RegisterEntry(CPU_COMM_ID_WD, CPU_COMM_TYPE_SOCKET);
	CpuComm_RegisterISRService( CPU_COMM_ID_WD ,cpub_wd_handler );
	#if AUTO_START_WD
	WD_SetTimeout(CONFIG_AIT_WATCHDOG_DEFAULT_TIME);
	WD_Start();
	#endif
	
	while(1)
	{
	    MMP_ULONG SocketRet = 0;
     // recv & ack 
	    SocketRet = CpuComm_SocketReceive(  CPU_COMM_ID_WD,
	                                        (MMP_UBYTE*)WD_CPU_Reg,
	                                        sizeof(cpu_comm_transfer_data),
	                                        MMP_TRUE);
	    
	    if (SocketRet  == CPU_COMM_ERR_NONE) {
	        int ret = 0;
	        
	        WD_CPU_Reg->phy_addr =  DRAM_NONCACHE_VA(WD_CPU_Reg->phy_addr);
          switch(WD_CPU_Reg->command) {
            case WD_START:
            {
                WD_Start() ;
                WD_Return->command = WD_CPU_Reg->command;
                WD_Return->result   = 0 ;
                WD_Return->phy_addr = 0;
                WD_Return->size     = 0;
              
            }
            break ;    
            case WD_STOP:
            {  
                WD_Stop();
                WD_Return->command = WD_CPU_Reg->command;
                WD_Return->result   = 0 ;                
                WD_Return->phy_addr = 0;
                WD_Return->size     = 0;                
                break ;
            }
            case WD_PING:
            {     
              WD_Ping();         
              WD_Return->command = WD_CPU_Reg->command; 
              WD_Return->result   = 0;
              WD_Return->phy_addr = 0;
              WD_Return->size     = 0;
            }
            break;
            case WD_SET_TIMEOUT:
            {  
              WD_SetTimeout(WD_CPU_Reg->size );
              WD_Return->command = WD_CPU_Reg->command;
              WD_Return->result   = 0 ;
              WD_Return->phy_addr = 0;
              WD_Return->size     = 0;
              break;
            }
          }
    			if(WD_Return->result < 0) {
    			    printc("CMD %d err:0x%x\r\n", WD_CPU_Reg->command, WD_Return->result);
    			}
    			WD_Return->flag = CPUCOMM_FLAG_CMDSND;
   				CpuComm_SocketSend(CPU_COMM_ID_WD, (MMP_UBYTE*)WD_Return, sizeof(cpu_comm_transfer_data));
  		}
  		else
  		{
  		    printc("SocketRet:%x\r\n", SocketRet);
  		}
	}

	printc( "Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );

}

/* */
void EstablishWD(void)
{
	MMPF_TASK_CFG	task_cfg;

	task_cfg.ubPriority = TASK_B_WD_PRIO;
	task_cfg.pbos = (MMP_ULONG) & s_WDTaskStk[0];
	task_cfg.ptos = (MMP_ULONG) & s_WDTaskStk[TASK_B_WD_STK_SIZE - 1];
	MMPF_OS_CreateTask(DualCpu_WDTask, &task_cfg, (void *) 0);
}

