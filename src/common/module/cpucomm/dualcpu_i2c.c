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
#include "os_wrap.h"
#include "dualcpu_i2c.h"

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
static void EstablishI2C(void);
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

static OS_STK  s_I2CTaskStk[TASK_B_I2C_STK_SIZE];   // Stack for service task

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
CPU_COMM_ERR CpuB_I2CInit(void)
{
	EstablishI2C();
	return CPU_COMM_ERR_NONE;
}

CPUCOMM_MODULE_INIT(CpuB_I2CInit)


static cpu_comm_transfer_data _I2C_Proc ;
static cpu_comm_transfer_data _I2C_Return;
static i2c_device_t *i2c_devices[I2C_MAX_DEVICES] ;
static int i2c_register_id = 0 ;

static i2c_device_t *get_device_by_id(int id)
{
  int i;
  i2c_device_t *dev;
  for ( i = 0 ;i<I2C_MAX_DEVICES;i++) {
    dev = i2c_devices[i] ;
    if(dev->id != -1) {
      if(dev->id == id) {
        return dev ;
      }  
    }  
  }
  return 0 ;
}

int I2C_Register(i2c_device_t *dev)
{
  int ret ;
  if(!dev) {
    return -1 ;
  }  
  
  if( i2c_register_id >= I2C_MAX_DEVICES ) {
    return -1 ;
  }
  
  ret = dev->probe(dev->driver_data);
  if(ret < 0) {
    return ret ;
  }
  
  if(dev->id < 0 ) {
    dev->id = i2c_register_id ;
  }
  
  i2c_devices[i2c_register_id] = dev ;
  i2c_register_id++ ;
  printc("IPC I2C register : %s,id :%d\r\n",dev->name,dev->id);
  return 0 ;
}


int I2C_Query( i2c_ipc_info_t *ipc_info )
{
  int i,ret ;
  i2c_device_t *dev ;
  i2c_info_t *info;
  ipc_info->devices = 0 ;
  printc("I2C_Query start\r\n");
  if(!ipc_info) {
    return -1 ;
  }
  for(i=0; i < I2C_MAX_DEVICES ;i++) {
    dev = i2c_devices[i] ;
    if(dev) {
      if(dev->id != -1 ) {
        info = &ipc_info->info[ipc_info->devices] ;
        info->id = dev->id ;
        strcpy(info->name,dev->name);
        info->slave_addr = dev->slave_addr ;
        info->addr_len = dev->addr_len ;
        info->data_len = dev->data_len ;
        ipc_info->devices++;
        printc("i2c-dev[%d] : %s,addr_len : %d, data_len :%d\r\n",info->id,info->name,info->addr_len,info->data_len);  
      }
    }
  } 
  
  return 0;
}

int I2C_Read( i2c_ipc_rw_t *rw )
{
  int ret ;
  i2c_device_t *dev ;
  //printc("I2C_Read\r\n");
  if( !rw ) {
    return -1 ;
  }
  dev = get_device_by_id(rw->id) ;
  if(!dev) {
    return -1 ;
  }
  ret = dev->read(dev->driver_data,rw->addr,&rw->data);
  printc("I2C_Read[%d] : 0x%x,0x%x\r\n",dev->id,rw->addr,rw->data); 
  return ret ;
}

int I2C_Write( i2c_ipc_rw_t *rw )
{
  i2c_device_t *dev ;
  //printc("I2C_Write\r\n");
  if( !rw ) {
    return -1 ;
  }
  dev = get_device_by_id(rw->id) ;
  if(!dev) {
    return -1 ;
  }
  printc("I2C_Write[%d] : 0x%x,0x%x\r\n",dev->id,rw->addr,rw->data); 
  return dev->write(dev->driver_data,rw->addr,rw->data);  
}

void DualCpu_I2CTask(void* pData)
{
	cpu_comm_transfer_data *I2C_CPU_Reg = &_I2C_Proc; 
	cpu_comm_transfer_data *I2C_Return  = &_I2C_Return;
	RTNA_DBG_Str0("[I2C]\r\n");
	CpuComm_RegisterEntry(CPU_COMM_ID_I2C, CPU_COMM_TYPE_SOCKET);
	CpuComm_RegisterISRService( CPU_COMM_ID_I2C ,0 );
	
	while(1)
	{
	    MMP_ULONG SocketRet = 0;
     // recv & ack 
	    SocketRet = CpuComm_SocketReceive(  CPU_COMM_ID_I2C,
	                                        (MMP_UBYTE*)I2C_CPU_Reg,
	                                        sizeof(cpu_comm_transfer_data),
	                                        MMP_TRUE);
	    
	    if (SocketRet  == CPU_COMM_ERR_NONE) {
	        int ret = 0;
	        
	        I2C_CPU_Reg->phy_addr = DRAM_NONCACHE_VA(I2C_CPU_Reg->phy_addr);
          I2C_Return->phy_addr  = I2C_CPU_Reg->phy_addr ;

          switch(I2C_CPU_Reg->command) {
            
            case I2C_QUERY:
            {
                i2c_ipc_info_t *i2c_info = (i2c_ipc_info_t *)I2C_CPU_Reg->phy_addr ;
                I2C_Return->result = I2C_Query(i2c_info) ;
                I2C_Return->command = I2C_CPU_Reg->command;
                I2C_Return->size     = sizeof(i2c_ipc_info_t) ;
            }
            break ;    
            case I2C_READ:
            {  
                i2c_ipc_rw_t *rw = (i2c_ipc_rw_t *)I2C_CPU_Reg->phy_addr ;
                I2C_Return->result = I2C_Read(rw);
                I2C_Return->command = I2C_CPU_Reg->command;
                I2C_Return->size     = sizeof(i2c_ipc_rw_t);            
            }
            break;
            case I2C_WRITE:
            {     
                i2c_ipc_rw_t *rw = (i2c_ipc_rw_t *)I2C_CPU_Reg->phy_addr ;
                I2C_Return->result = I2C_Write(rw);         
                I2C_Return->command = I2C_CPU_Reg->command; 
                I2C_Return->size     = sizeof(i2c_ipc_rw_t);
            }
            break;
          }
    			if(I2C_Return->result < 0) {
    			    printc("CMD %d err:0x%x\r\n", I2C_CPU_Reg->command, I2C_Return->result);
    			}
    			I2C_Return->flag = CPUCOMM_FLAG_CMDSND;
   				CpuComm_SocketSend(CPU_COMM_ID_I2C, (MMP_UBYTE*)I2C_Return, sizeof(cpu_comm_transfer_data));
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
void EstablishI2C(void)
{
	MMPF_TASK_CFG	task_cfg;

	task_cfg.ubPriority = TASK_B_I2C_PRIO;
	task_cfg.pbos = (MMP_ULONG) & s_I2CTaskStk[0];
	task_cfg.ptos = (MMP_ULONG) & s_I2CTaskStk[TASK_B_I2C_STK_SIZE - 1];
	MMPF_OS_CreateTask(DualCpu_I2CTask, &task_cfg, (void *) 0);
}

