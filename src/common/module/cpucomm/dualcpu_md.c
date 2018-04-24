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

#if (USER_LOG)
extern unsigned int log10(unsigned int v);
#endif

#if (SUPPORT_MDTC)
#include "md.h"
#include "mmps_mdtc.h"

#if 1
#define MS_FUNC(who,statement)     statement 
#else
#define MS_FUNC(who,statement)				\
do {										\
	MMP_ULONG t1 = MMPF_BSP_GetTime();		\
	statement ;								\
	printc("%s time : %d\r\n",who,MMPF_BSP_GetTime() - t1 );\
} while (0)
#endif
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
extern void Sensor_Init(void) ;
extern void Mdtc_Close(void);

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

static MMPF_OS_SEMID s_ulMDRcvSemId;        // Semaphore for critical section
static OS_STK  s_MDTaskStk[TASK_B_MD_STK_SIZE];   // Stack for service task

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
CPU_COMM_ERR CpuB_MDInit(void)
{
	//printc("CpuB_MDInit \r\n");
	s_ulMDRcvSemId = MMPF_OS_CreateSem(1);
	EstablishMD();
	return CPU_COMM_ERR_NONE;
}

CPUCOMM_MODULE_INIT(CpuB_MDInit)


static cpu_comm_transfer_data _MD_Proc ;
static cpu_comm_transfer_data _MD_Return;

static int md_debug = 0;
static int md_enable_event = 0 ;

void MdEventToCpuA(int new_val,int old_val)
{

    int channel ;
    struct cpu_share_mem_slot *S_slot;
    if(md_enable_event) {
        if(! new_val || ! old_val) { // one of them is zero , then signal to cpuA
            //printc("md.ev (%d,%d)\r\n",new_val,old_val);
            channel = get_noack_channel(CPU_COMM_ID_MDA2B, 1);
            while(1) {
        		if (get_slot(channel, &S_slot) == 0) {
        			S_slot->dev_id  = CPU_COMM_ID_MDA2B;
        			S_slot->command = MD_EVENT_CHANGES;
        			S_slot->data_phy_addr = 0;
        			S_slot->size = 0;
        			S_slot->send_parm[0] = (unsigned int)new_val;
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
        }    
    }    
}

void MD_Run_VendorCmd(int subcmd)
{
#if SUPPORT_PAS7671==1
extern void PAS7671_Ack(void);
extern void PAS7671_Init(void);  
#endif
  // CMD 1 : PAS7671 init
  // CMD 2 : PAS7671 event clr
  switch(subcmd) {
#if SUPPORT_PAS7671==1
  case 1:
    PAS7671_Init();
    break;  
  case 2:
    PAS7671_Ack();
    break;
#endif
  }  
} 

void DualCpu_MDTask(void* pData)
{
extern void UI_Set_MdProperties(MMPS_MDTC_PROPT *propt,MMP_BOOL en);
    static int MD_inited = 0 ;
    MMPS_MDTC_CLASS *obj = (MMPS_MDTC_CLASS *)MMPS_MDTC_Obj();
    static MMPS_MDTC_PROPT propt;
	//MMP_ULONG swap_info_dma_addr = DRAM_NONCACHE_VA( (MMP_ULONG)MD_PAYLOAD_DATA ) ;
	cpu_comm_transfer_data *MD_CPU_Reg = &_MD_Proc; 
	cpu_comm_transfer_data *MD_Return  = &_MD_Return;

	//printc( "DualCpu_MD Start\r\n");
	RTNA_DBG_Str0("[MD]\r\n");

	CpuComm_RegisterEntry(CPU_COMM_ID_MDA2B, CPU_COMM_TYPE_SOCKET);
	//	Create event signal for MD changes
    CpuComm_RegisterISRService(CPU_COMM_ID_MDA2B, 0);
	while(1)
	{
	    MMP_ULONG SocketRet = 0;

	    // recv & ack 
	    SocketRet = CpuComm_SocketReceive(  CPU_COMM_ID_MDA2B,
	                                        (MMP_UBYTE*)MD_CPU_Reg,
	                                        sizeof(cpu_comm_transfer_data),
	                                        MMP_TRUE);
	    
	    if (SocketRet  == CPU_COMM_ERR_NONE) {
	        int ret = 0;
	        
	        MD_CPU_Reg->phy_addr =  DRAM_NONCACHE_VA(MD_CPU_Reg->phy_addr);
	        MD_Return->phy_addr = DRAM_NONCACHE_VA(obj->ipc_buf.base);

            switch(MD_CPU_Reg->command) {
            case VENDER_CMD:
            {
                MD_Run_VendorCmd(MD_CPU_Reg->phy_addr) ;
                MD_Return->command = MD_CPU_Reg->command;
                MD_Return->result   = 0 ;
                MD_Return->phy_addr = 0;
                MD_Return->size     = 0;
              
            }
            break ;    
            case MD_EVENT_CHANGES:
                if( md_enable_event != MD_CPU_Reg->phy_addr) {
                    printc("Enable Md event :%d\r\n", MD_CPU_Reg->phy_addr);
                }
                md_enable_event = MD_CPU_Reg->phy_addr ;
                MD_Return->command = MD_CPU_Reg->command;
                MD_Return->result   = 0 ;
                MD_Return->phy_addr = 0;
                MD_Return->size     = 0;
                
                break ;
            case MD_INIT:
                {
                    MD_init_t *MD_Init = (MD_init_t*)MD_CPU_Reg->phy_addr;

                    propt.luma_w = MD_Init->width;
                    propt.luma_h = MD_Init->height;
                    #if MD_USE_ROI_TYPE==1
                    propt.window.roi_num = MD_Init->mux.roi_num ;
                    #endif
                    UI_Set_MdProperties(&propt,MMP_FALSE);
                    propt.snrID  = 0;
                    MD_inited = 0;

                    MD_Return->command = MD_CPU_Reg->command;
                    
                    MD_Return->result   = MD_USE_ROI_TYPE ;
                    MD_Return->phy_addr = 0;
                    MD_Return->size     = 0;

                }
                break;
            case MD_GET_VERSION:
                ret = MD_GetLibVersion((MMP_ULONG *)MD_Return->phy_addr);
                MD_Return->command = MD_CPU_Reg->command;
                MD_Return->result   = ret;
                MD_Return->size     = sizeof(MMP_ULONG);
                printc("MD_Ver: 0x%x\r\n", *(MMP_ULONG *)MD_Return->phy_addr);
                break;

            case MD_SET_WINDOW:
                {
                    MD_detect_window_t *MD_Window_Info =
                                    (MD_detect_window_t *)MD_CPU_Reg->phy_addr;
                    
                    #if MD_USE_ROI_TYPE==1
                    short roi_id = MD_Window_Info->win_roi.roi_id ;
                    if ( (roi_id >=0 ) && (roi_id < MDTC_MAX_ROI) ){
                        propt.roi[roi_id].st_x = MD_Window_Info->lt_x ;
                        propt.roi[roi_id].st_y = MD_Window_Info->lt_y ;
                        propt.roi[roi_id].end_x = MD_Window_Info->rb_x ;
                        propt.roi[roi_id].end_y = MD_Window_Info->rb_y ;
                        
                    }
                    #else
                    propt.window.x_start = MD_Window_Info->lt_x;
                    propt.window.y_start = MD_Window_Info->lt_y;
                    propt.window.x_end   = MD_Window_Info->rb_x;
                    propt.window.y_end   = MD_Window_Info->rb_y;
                    propt.window.div_x   = MD_Window_Info->win_roi.win[0];
                    propt.window.div_y   = MD_Window_Info->win_roi.win[1];
                    #endif
                    
                    
                    if (md_debug) {
                    	printc("SET_WIN(0x%08x): (%d,%d,%d,%d), (%d,%d)\r\n",
                    			 MD_CPU_Reg->phy_addr,
                    			 MD_Window_Info->lt_x,
                    			 MD_Window_Info->lt_y,
                    			 MD_Window_Info->rb_x,
                    			 MD_Window_Info->rb_y,
                    			 MD_Window_Info->win_roi.win[0],
                    			 MD_Window_Info->win_roi.win[1]);
                    			 
                    }
                    MD_Return->command = MD_CPU_Reg->command;
                    MD_Return->result   = ret;
                    MD_Return->phy_addr = 0;
                    MD_Return->size     = 0;   
                }
                break;

            case MD_GET_WINDOW_SIZE:
                {
                    MMP_USHORT st_x, st_y, div_w, div_h;
                    
                    //CpuComm_SocketPost(CPU_COMM_ID_MDA2B, MD_CPU_Reg->command);
                    if( obj->state != MDTC_STATE_IDLE ) {
                        MD_get_detect_window_size(&st_x, &st_y, &div_w, &div_h);
                        *(((MMP_SHORT*)MD_Return->phy_addr) + 0) = st_x;
                        *(((MMP_SHORT*)MD_Return->phy_addr) + 1) = st_y;
                        *(((MMP_SHORT*)MD_Return->phy_addr) + 2) = div_w;
                        *(((MMP_SHORT*)MD_Return->phy_addr) + 3) = div_h;
                    }
                    else {
                        ret = 1 ;
                    }
                    MD_Return->command    = MD_CPU_Reg->command;
                    MD_Return->result     = ret ;
                    MD_Return->size       = sizeof(MMP_SHORT) * 4;
                }
                break;
            #if MD_USE_ROI_TYPE==1    
            case MD_SET_ROI_PARA_IN:
            {
              //printc("sizeof(MD_params_in_t) = %d\r\n",sizeof(MD_params_in_t) );
              MMPF_MD_Update( (MD_params_in_t *)MD_CPU_Reg->phy_addr ); 
            }
            break;
            #endif
            case MD_SET_WINDOW_PARA_IN:
            {
              MD_window_parameter_in_t *MD_Window_Parameter = (MD_window_parameter_in_t *)MD_CPU_Reg->phy_addr;
              if(md_debug) {
              
                  printc("SET_WIN_PARA(0x%08x):\r\n",MD_CPU_Reg->phy_addr);
                  printc("--w_num : %d\r\n",MD_Window_Parameter->win_roi.win[0] );
                  printc("--h_num : %d\r\n",MD_Window_Parameter->win_roi.win[1] );
                                    
                  printc("--p.enable : %d\r\n",MD_Window_Parameter->param.enable );
                  printc("--p.size_perct_thd_min : %d\r\n",MD_Window_Parameter->param.size_perct_thd_min );
                  printc("--p.size_perct_thd_max : %d\r\n",MD_Window_Parameter->param.size_perct_thd_max );
                  printc("--p.sensitivity : %d\r\n",MD_Window_Parameter->param.sensitivity );
                  printc("--p.learn_rate : %d\r\n",MD_Window_Parameter->param.learn_rate );
              }
              #if MD_USE_ROI_TYPE==0
                    propt.param[MD_Window_Parameter->win_roi.win[0]][MD_Window_Parameter->win_roi.win[1]].enable             = MD_Window_Parameter->param.enable ;  
                    propt.param[MD_Window_Parameter->win_roi.win[0]][MD_Window_Parameter->win_roi.win[1]].size_perct_thd_min = MD_Window_Parameter->param.size_perct_thd_min ;  
                    propt.param[MD_Window_Parameter->win_roi.win[0]][MD_Window_Parameter->win_roi.win[1]].size_perct_thd_max = MD_Window_Parameter->param.size_perct_thd_max ;  
                    propt.param[MD_Window_Parameter->win_roi.win[0]][MD_Window_Parameter->win_roi.win[1]].sensitivity        = MD_Window_Parameter->param.sensitivity ;  
                    propt.param[MD_Window_Parameter->win_roi.win[0]][MD_Window_Parameter->win_roi.win[1]].learn_rate         = MD_Window_Parameter->param.learn_rate ;
              #else
                    propt.param[MD_Window_Parameter->win_roi.roi_id].enable             = MD_Window_Parameter->param.enable ;  
                    propt.param[MD_Window_Parameter->win_roi.roi_id].size_perct_thd_min = MD_Window_Parameter->param.size_perct_thd_min ;  
                    propt.param[MD_Window_Parameter->win_roi.roi_id].size_perct_thd_max = MD_Window_Parameter->param.size_perct_thd_max ;  
                    propt.param[MD_Window_Parameter->win_roi.roi_id].sensitivity        = MD_Window_Parameter->param.sensitivity ;  
                    propt.param[MD_Window_Parameter->win_roi.roi_id].learn_rate         = MD_Window_Parameter->param.learn_rate ;
                    
                #endif  
                MD_Return->command    = MD_CPU_Reg->command;
                MD_Return->result     = 0;
                MD_Return->size       = 0;
            }
            break;
                case MD_GET_WINDOW_PARA_IN:
                {
                    MD_window_parameter_in_t MD_Win, *md_win_p = &MD_Win ;
                    MD_params_in_t *param = (MD_params_in_t*) MD_Return->phy_addr ;
                    memcpy((void*)md_win_p, (void*)MD_CPU_Reg->phy_addr, sizeof(MD_window_parameter_in_t));
                    #if MD_USE_ROI_TYPE==0
                    *param = propt.param[md_win_p->win_roi.win[0]][md_win_p->win_roi.win[1]] ;
                    #else
                    *param = propt.param[md_win_p->win_roi.roi_id] ;
                    #endif
                    
                    MD_Return->command = MD_CPU_Reg->command;
                    MD_Return->result = ret;
                    MD_Return->size   = sizeof(MD_params_in_t);
                }
                break;
                case MD_GET_WINDOW_PARA_OUT:
                {
                    MD_window_parameter_out_t MD_Win , *md_win_p = &MD_Win ;
                    #if MD_USE_ROI_TYPE==1
                    MD_params_out_t param_out[ MDTC_MAX_ROI ] ;
                    #endif
                    memcpy((void*)md_win_p, (void*)MD_CPU_Reg->phy_addr, sizeof(MD_window_parameter_out_t));
                    #if MD_USE_ROI_TYPE==0
                    ret = MD_get_window_params_out(md_win_p->win_roi.win[0], md_win_p->win_roi.win[1],  (MD_params_out_t*) MD_Return->phy_addr);
                    #else
                    ret = MD_get_window_params_out(0, 0, param_out);
                    *(MD_params_out_t*) MD_Return->phy_addr = param_out[md_win_p->win_roi.roi_id] ;
                    #endif
                    MD_Return->command = MD_CPU_Reg->command;
                    MD_Return->result = ret;
                    MD_Return->size   = sizeof(MD_params_out_t);
                    
                    if(md_debug) {
                        printc("GET_WIN_PARA_OUT[0x%08x]:%x,%x",MD_CPU_Reg->phy_addr, md_win_p->win_roi.win[0], md_win_p->win_roi.win[1] );
                    }
                }
                break;
                case MD_GET_BUFFER_INFO:
                {
                    MD_buffer_info_t *MD_Buffer_Info = (MD_buffer_info_t*)(MD_CPU_Reg->phy_addr);
                    
                    MD_Return->command = MD_CPU_Reg->command;
                    // cpu-b decided
                    MD_Return->result = 0;
                    // this is a tricky, pyh_addr is for return buffer size
                    MD_Return->phy_addr = MMPF_MD_BufSize(MD_Buffer_Info->width,MD_Buffer_Info->height,MD_Buffer_Info->w_div,MD_Buffer_Info->h_div ) ;
                    MD_Return->size       = sizeof(MMP_ULONG);
                    if(md_debug) {
                    	printc("GET_INFO(0x%08x) : buf_size : %d\r\n" ,MD_CPU_Reg->phy_addr, MD_Return->phy_addr );
                    }
                }
                break;
                case MD_SUSPEND:
                    //MD_suspend(unsigned char md_suspend_enable, unsigned char md_suspend_duration, unsigned char md_suspend_threshold);
                break;
                case MD_RUN:
                if(1){
                    MMPF_MDTC_CLASS *md = &obj->md ;
                	MD_motion_info_t *motion_info = (MD_motion_info_t *)MD_Return->phy_addr ;
                    MD_Return->size = sizeof(MD_motion_info_t);

                    if(!MD_inited) {
                        if(obj->state != MDTC_STATE_IDLE ) {
                            printc("Restart MD\r\n...");
                            /*
                            MMPS_MDTC_Stop();
                            MMPS_MDTC_Close(); 
                            */
                            // for power saving
                            Mdtc_Close();
                            
                        }
                        // for power saving
                        // start sensor
                        Sensor_Init();
                        ret = MMPS_MDTC_Open( &propt ,MdEventToCpuA );
                        if(ret) {
                            printc("MMPS_MDTC_Open ret:%d\r\n",ret);
                        }
                        MMPS_MDTC_Start();
                        MD_inited = 1 ;
                    }
                    motion_info->obj_cnt = 0 ;

                    if(!ret) {
                       	int x/*,y*/ ,md_win ,cnt = 0 ;
                    	MD_params_out_t md_outinfo ;
                    	
                    	//printc("MD_Win[");
                    	#if MD_USE_ROI_TYPE==0
                    	for(x=0;x<md->window.div_x;x++) {
                    		for(y=0;y<md->window.div_y;y++) {
                    			//MD_get_window_params_out(i,j, &md_outinfo);
                    			md_outinfo = md->result[x][y] ;
                    			if(md_outinfo.md_result) {
                    				md_win =  x * md->window.div_y + y ;
                    				motion_info->obj_axis[cnt] = md_win ;
                    				cnt++ ;
                    				//printc("%d ",md_win);
                    			}
                    		}
                    	}
                    	#else
                    	for(x=0;x<md->window.roi_num;x++) {
                    	    md_outinfo = md->result[x] ;
                    	    if(md_outinfo.md_result) {
                    	        md_win =  x  ;
                    	        motion_info->obj_axis[cnt] = x ;
                    	        cnt++ ;
                    	    }
                    	}
                    	#endif
                    	motion_info->obj_cnt =cnt;

                    	//printc("]->%d\r\n",motion_info->obj_cnt);
                    }
                    MD_Return->result =  motion_info->obj_cnt;
                }
                break;
                case MD_HISTGRAM:
                {
                    //int i;
                    MMPF_MDTC_CLASS *md = &obj->md ;
                	MD_motion_info_t *motion_info = (MD_motion_info_t *)MD_Return->phy_addr ;
                    MD_Return->size = sizeof(MD_motion_info_t);
                    *motion_info = md->md_hist ;
                    MD_Return->result = motion_info->obj_cnt ;
                    MEMSET( &md->md_hist,0, sizeof(md->md_hist) );
                   // printc("MD motion cnt : %d\r\n",motion_info->obj_cnt );
                   // printc("MD motion cnt by win id:\r\n");   
                    // pass div_x & div_y
                    #if MD_USE_ROI_TYPE==0
                    motion_info->obj_cnt = (md->window.div_x << 8 ) | md->window.div_y ;
                    #else
                    motion_info->obj_cnt = (md->window.roi_num );
                    #endif
                    if(md_debug) { 
                        printc("MD histgram : 0x%04x 0x%04x, addr : 0x%08x\r\n",motion_info->obj_cnt,motion_info->obj_cnt_sum ,(MMP_ULONG)motion_info);
                    }
                    //for(i=0;i<md->window.div_x  * md->window.div_y ;i++) {
                    //  printc("win[%d] : %d\r\n",i,motion_info->obj_axis[i] );
                    //}
                    
                 }    
                break;
                
                default:
                    printc("no such command\r\n");
                    MD_Return->command = MD_NOT_SUPPORT;
                break;
            }

			if(MD_Return->result < 0)
			{
			    printc("CMD %d err:0x%x\r\n", MD_CPU_Reg->command, MD_Return->result);
			}
			MD_Return->flag = CPUCOMM_FLAG_CMDSND;
			MS_FUNC("send",
				CpuComm_SocketSend(CPU_COMM_ID_MDB2A, (MMP_UBYTE*)MD_Return, sizeof(cpu_comm_transfer_data));
			);
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
void EstablishMD(void)
{
	MMPF_TASK_CFG	task_cfg;

	task_cfg.ubPriority = TASK_B_MD_PRIO;
	task_cfg.pbos = (MMP_ULONG) & s_MDTaskStk[0];
	task_cfg.ptos = (MMP_ULONG) & s_MDTaskStk[TASK_B_MD_STK_SIZE - 1];
	MMPF_OS_CreateTask(DualCpu_MDTask, &task_cfg, (void *) 2);
}
#endif
