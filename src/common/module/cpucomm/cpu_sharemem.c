#include "includes_fw.h"
#include "config_fw.h"
#include "includes_fw.h"
#include "lib_retina.h"
#include "os_cpu.h"
#include "mmpf_pll.h"
#include "cpucomm.h"
#include "mmu.h"
#include "cpucomm_core.h"
#include "cpucomm_bus.h"
#include "cpu_sharemem.h"
#include "mmp_cpucomm.h"
#include "mmp_reg_int.h"
#include "mmp_reg_gbl.h"
#include "mmp_register.h"

#define EBUSY		16	/* Device or resource busy */
#define EINVAL		22	/* Invalid argument */
#define EIO			5	/* I/O error */

#if (CPU_SHAREMEM == 1)
static struct _cpu_share_mem_info csm_info_alloc;
static struct _cpu_share_mem_info *csm_info = &csm_info_alloc;

static void set_slot_free(struct _queue_info *qi, unsigned int slot_num)
{
	qi->free_slot = qi->free_slot | (1 << slot_num);
	qi->used_slot = qi->used_slot & ~(1 << slot_num);
	qi->wait_ack_slot = qi->wait_ack_slot & ~(1 << slot_num);
}

static void set_slot_used(struct _queue_info *qi, unsigned int slot_num)
{
	qi->free_slot = qi->free_slot & ~(1 << slot_num);
	qi->used_slot = qi->used_slot | (1 << slot_num);
}

static void set_slot_wait(struct _queue_info *qi, unsigned int slot_num)
{
	qi->wait_ack_slot = qi->wait_ack_slot | (1 << slot_num);
}

int initial_cpu_share_mem(int max_channel)
{
	int i;
	AIT_REG_D *SHARE_REG = (AIT_REG_D *)MMP_CPUCOMM_SHARE_REG_GET();
	unsigned int sq_addr, rq_addr;
	
	memset(csm_info, 0, sizeof(struct _cpu_share_mem_info));
	csm_info->baseaddr_share = (unsigned int *)AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG;
	
	for(i=0;i<max_channel;i++)
	{
		sq_addr = DRAM_NONCACHE_VA(*(SHARE_REG+(i*2)+1));
		rq_addr = DRAM_NONCACHE_VA(*(SHARE_REG+(i*2)));
		
		init_send_queue(i, sq_addr, sq_addr, MAX_SLOT);
		init_recv_queue(i, rq_addr, rq_addr, MAX_SLOT);
    }
    return 0;
}

void notify_cpux(int max_channel)
{
	int i;
	AIT_REG_D *SHARE_REG = (AIT_REG_D *)MMP_CPUCOMM_SHARE_REG_GET();

	for(i=0;i<max_channel;i++)
    {
		*(SHARE_REG+i*2) = 0x0;
		*(SHARE_REG+(i*2)+1) = 0x0;
	}
}

unsigned int get_share_register(void)
{
	return (unsigned int)csm_info->baseaddr_share; 
}

int Cpu_sharemem_ISR(void)
{
	int channel, i;
	struct _queue_info *rqi, *sqi;
	struct cpu_share_mem_slot *temp_slot;


	for(channel=0;channel<MAX_SEND_QUEUE;channel++)
	{
		if(csm_info->send_queue_info[channel].init_done != 1)
		{
			continue;
		}else{
			sqi = &(csm_info->send_queue_info[channel]);
			//printc("Check send channel %d\r\n", channel);
		}

		//check wait ack slot
		for(i=0;(i<sqi->total_slot)&&(sqi->wait_ack_slot!=0);i++)
		{
			//check wait ack slot
			if((sqi->wait_ack_slot & (1 << i)) == (1 << i))
			{
				temp_slot = (struct cpu_share_mem_slot *)(sqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
				if(temp_slot->slot_status == SLOT_ACK)
				{
					//check if need to do callback function
					if((temp_slot->ack_func != NULL) && (sqi->dev_id_map & (1 << temp_slot->dev_id)) == (1 << temp_slot->dev_id))
					{
						//printc("Callback function\r\n");
						temp_slot->ack_func((void *)temp_slot);
					}

					//update queue statsu
					temp_slot->slot_status = SLOT_FREE;
					set_slot_free(sqi, i);
					//printc("Finish slot %d send\r\n", i);
				}
			}
		}
	}

	//check recv queue
	for(channel=0;channel<MAX_RECV_QUEUE;channel++)
	{
		if(csm_info->recv_queue_info[channel].init_done != 1)
		{
			continue;
		}else{
			rqi = &(csm_info->recv_queue_info[channel]);
			//printc("Check recv channel %d\r\n", channel);
		}

		for(i=0;i<rqi->total_slot;i++)
		{
			temp_slot = (struct cpu_share_mem_slot *)(rqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
			if(temp_slot->slot_status == SLOT_SEND)
			{
				if(rqi->dev_id_map & (1 << temp_slot->dev_id) & rqi->dev_id_no_ack) //device id don't need ack
				{
					if(rqi->no_ack_func[temp_slot->dev_id] != NULL)
					{
						rqi->no_ack_func[temp_slot->dev_id]((void *)temp_slot);
					}
					temp_slot->slot_status = SLOT_FREE; //just free
				}
				else if(rqi->dev_id_map & (1 << temp_slot->dev_id)) //device id need ack
				{
					if(rqi->recv_q_slot[temp_slot->dev_id] == 0)
					{
						//queue is empty and need to do release
						temp_slot->slot_status = SLOT_QUEUE;
						rqi->recv_q_slot[temp_slot->dev_id] = rqi->recv_q_slot[temp_slot->dev_id] | (1 << i);
						MMPF_OS_ReleaseSem(rqi->dev_sem[temp_slot->dev_id]);
					}else{
						//queue is not empty
						temp_slot->slot_status = SLOT_QUEUE;
						rqi->recv_q_slot[temp_slot->dev_id] = rqi->recv_q_slot[temp_slot->dev_id] | (1 << i);
					}
				}else{ //Error
					printc("Recv one slot and dev_id (0x%x) is not enable. Ack with error\r\n", temp_slot->dev_id);
					temp_slot->recv_parm[0] = ACK_ID_DISABLE;
					temp_slot->slot_status = SLOT_ACK;
					MMP_CPUCOMM_IRQ_SET(_CPU_ID_A);
				}
			}
		}
	}
	return 0;
}

int init_send_queue(unsigned int channel, unsigned int slot_v_st_addr, unsigned int slot_p_st_addr, unsigned int total_slot)
{
	struct cpu_share_mem_slot *temp_slot;
	int i;

	//check channel and slot number
	if((channel >= MAX_SEND_QUEUE) || (total_slot > MAX_SLOT))
	{
		printc("ERROR: channel or total_slot are invalid. channel = 0x%x total_slot = 0x%x\r\n", channel, total_slot);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->send_queue_info[channel].init_done == 1)
	{
		printc("ERROR: send queue is not initial\r\n");
		return -EINVAL;
	}

	//initial queue info
	csm_info->send_queue_info[channel].init_done = 1;
	csm_info->send_queue_info[channel].seq = 0;
	csm_info->send_queue_info[channel].slot_v_st_addr = slot_v_st_addr;
	csm_info->send_queue_info[channel].slot_p_st_addr = slot_p_st_addr;
	csm_info->send_queue_info[channel].total_slot = total_slot;
	csm_info->send_queue_info[channel].free_slot = (unsigned int)((1 << total_slot) - 1);
	csm_info->send_queue_info[channel].used_slot = 0;
	csm_info->send_queue_info[channel].wait_ack_slot = 0;
	csm_info->send_queue_info[channel].dev_id_map = 0;
	csm_info->send_queue_info[channel].dev_id_no_ack = 0;

	//initial slot info
	for(i=0;i<total_slot;i++)
	{
		temp_slot = (struct cpu_share_mem_slot *)(slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
		memset(temp_slot, 0, sizeof(struct cpu_share_mem_slot));
		temp_slot->hdr_phy_addr = (unsigned int)(slot_p_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
		temp_slot->slot_direct = R_TO_L_SLOT;
		temp_slot->slot_num = i;
		temp_slot->slot_status = SLOT_FREE;
	}

	return 0;
}

int init_send_dev_id(unsigned int channel, unsigned int dev_id)
{
	int ret=0;
	struct _queue_info *sqi;
	CpuComm_CriticalSectionInit();	

	//check channel
	if((channel >= MAX_SEND_QUEUE) || (dev_id > MAX_DEV_ID))
	{
		printc("ERROR: channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\r\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->send_queue_info[channel].init_done != 1)
	{
		printc("ERROR: send queue is not initial\r\n");
		return -EINVAL;
	}else{
		sqi = &(csm_info->send_queue_info[channel]);
	}


	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((sqi->dev_id_map & (1 << dev_id)) == (1 << dev_id))
	{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: dev_id have enabled\n");
		return -EBUSY;
	}else{
		sqi->dev_id_map = sqi->dev_id_map | (1 << dev_id);
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int enable_send_dev_id_noack(unsigned int channel, unsigned int dev_id)
{
	int ret=0;
	struct _queue_info *sqi;
	CpuComm_CriticalSectionInit();

	//check channel
	if((channel >= MAX_SEND_QUEUE) || (dev_id > MAX_DEV_ID))
	{
		printc("ERROR: send noack channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\r\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->send_queue_info[channel].init_done != 1)
	{
		printc("ERROR: send queue is not initial\n");
		return -EINVAL;
	}else{
		sqi = &(csm_info->send_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((sqi->dev_id_no_ack & (1 << dev_id)) == (1 << dev_id))
	{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: No ack options have enabled\n");
		return -EBUSY;
	}else{
		sqi->dev_id_no_ack = sqi->dev_id_no_ack | (1 << dev_id);
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int close_send_dev_id(unsigned int channel, unsigned int dev_id)
{
	int ret=0, i;
	struct _queue_info *sqi;
	struct cpu_share_mem_slot *temp_slot;
	CpuComm_CriticalSectionInit();	

	//check channel
	if((channel >= MAX_SEND_QUEUE) || (dev_id > MAX_DEV_ID))
	{
		printc("ERROR: channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\r\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->send_queue_info[channel].init_done != 1)
	{
		printc("ERROR: send queue is not initial\n");
		return -EINVAL;
	}else{
		sqi = &(csm_info->send_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((sqi->dev_id_map & (1 << dev_id)) == (1 << dev_id))
	{
		sqi->dev_id_map = sqi->dev_id_map & ~(1 << dev_id);
	}else{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: dev_id is not enabled\n");
		return -EINVAL;
	}
	// check active no ack bitmap
	if((sqi->dev_id_no_ack & (1 << dev_id)) == (1 << dev_id)) {
	    sqi->dev_id_no_ack = sqi->dev_id_no_ack & ~(1 << dev_id);
	}
	

	for(i=0;i<sqi->total_slot;i++)
	{
		if(((sqi->wait_ack_slot & (1 << i)) == (1 << i)) || ((sqi->used_slot & (1 << i)) == (1 << i)))
		{
			temp_slot = (struct cpu_share_mem_slot *)(sqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
			temp_slot->slot_status = SLOT_FREE;
			set_slot_free(sqi, i);
		}
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int get_slot(unsigned int channel, struct cpu_share_mem_slot **slot)
{
	int ret=0, i;
	struct _queue_info *sqi;
	CpuComm_CriticalSectionInit();	

	//check channel
	if(channel >= MAX_SEND_QUEUE)
	{
		printc("ERROR: channel is invalid. channel = 0x%x\r\n", channel);
		return -EINVAL;
	}

	//check queue info
	if(csm_info->send_queue_info[channel].init_done != 1)
	{
		printc("ERROR: send queue is not initial\r\n");
		return -EINVAL;
	}else{
		sqi = &(csm_info->send_queue_info[channel]);
	}

	//check if slots are full
	if(sqi->free_slot == 0)
	{
		printc("ERROR: Slot is full\r\n");
		return -EBUSY;
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//find free slot
	for(i=0;i<sqi->total_slot;i++)
	{
		if((sqi->free_slot & (1 << i)) == (1 << i))
		{
			//update slot status
			*slot = (struct cpu_share_mem_slot *)(sqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
			if((*slot)->slot_status != SLOT_FREE) //No ack slot
			{
				continue;
			}else{ //Normal slot
			(*slot)->slot_status = SLOT_GET;
			(*slot)->slot_direct = L_TO_R_SLOT;
			(*slot)->seq = sqi->seq;
			if((*slot)->slot_num != i)
			{
				printc("ERROR: slot num is not match bitmap!!!!\r\n");
			}

			//update queue status
			sqi->seq = (sqi->seq + 1) % 0xFFFF;
			set_slot_used(sqi, i);
			}
			break;
		}
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

 	// check if all slot are used
 	if(i == sqi->total_slot)
 	{
 		printc("ERROR: All slot are used\r\n");
 		return -EBUSY;
 	}

	return ret;
}

int send_slot(unsigned int channel, struct cpu_share_mem_slot *slot)
{
	int ret=0;
	struct _queue_info *sqi;
	CpuComm_CriticalSectionInit();	

	//check channel
	if(channel >= MAX_SEND_QUEUE)
	{
		printc("ERROR: channel is invalid. channel = 0x%x\r\n", channel);
		return -EINVAL;
	}

	//check queue info
	if(csm_info->send_queue_info[channel].init_done != 1)
	{
		printc("ERROR: send queue is not initial\n");
		return -EINVAL;
	}else{
		sqi = &(csm_info->send_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check slot status and slot bitmap
	if(((sqi->used_slot & (1 << slot->slot_num)) != (1 << slot->slot_num)) || (slot->slot_status != SLOT_GET))
	{
		printc("ERROR: this slot(%d) is not in the list of used.(bitmap = 0x%x) status = %d.\r\n", slot->slot_num, sqi->used_slot, slot->slot_status);
		CpuComm_CriticalSectionLeave();
		return -EINVAL;
	}

	//change queue and slot status
	if((sqi->dev_id_no_ack & (1 << slot->dev_id)) != (1 << slot->dev_id))
	{
		set_slot_wait(sqi, slot->slot_num); //set slot to be wait.
	}else{
		set_slot_free(sqi, slot->slot_num); //For no ack slot we should free the slot but should not change the status.
	}
	slot->slot_status = SLOT_SEND;

	//triger interrupt
	MMP_CPUCOMM_IRQ_SET(_CPU_ID_A);

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int init_recv_queue(unsigned int channel, unsigned int slot_v_st_addr, unsigned int slot_p_st_addr, unsigned int total_slot)
{
	int i;

	//check channel and slot
	if((channel >= MAX_RECV_QUEUE) || (total_slot > MAX_SLOT))
	{
		printc("ERROR: channel or total_slot are invalid. channel = 0x%x total_slot = 0x%x\r\n", channel, total_slot);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done == 1)
	{
		printc("ERROR: recv queue is not initial\r\n");
		return -EINVAL;
	}

	//initial queue info
	csm_info->recv_queue_info[channel].init_done = 1;
	csm_info->recv_queue_info[channel].seq = 0;
	csm_info->recv_queue_info[channel].slot_v_st_addr = slot_v_st_addr;
	csm_info->recv_queue_info[channel].slot_p_st_addr = slot_p_st_addr;
	csm_info->recv_queue_info[channel].total_slot = total_slot;
	csm_info->recv_queue_info[channel].free_slot = (unsigned int)((1 << total_slot) - 1);
	csm_info->recv_queue_info[channel].used_slot = 0;
	csm_info->recv_queue_info[channel].wait_ack_slot = 0;
	csm_info->send_queue_info[channel].dev_id_map = 0;
	csm_info->send_queue_info[channel].dev_id_no_ack = 0;
	
	for(i=0;i<MAX_DEV_ID;i++)
	{
		 csm_info->recv_queue_info[channel].no_ack_func[i] = NULL;
	}

	return 0;
}

int init_recv_dev_id(unsigned int channel, unsigned int dev_id)
{
	struct _queue_info *rqi;
	CpuComm_CriticalSectionInit();	

	//check channel
	if((channel > MAX_RECV_QUEUE) || (dev_id > MAX_DEV_ID))
	{
		printc("ERROR: channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\r\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done != 1)
	{
		printc("ERROR: recv queue is not initial\n");
		return -EBUSY;
	}else{
		rqi = &(csm_info->recv_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((rqi->dev_id_map & (1 << dev_id)) == (1 << dev_id))
	{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: dev_id have enabled\r\n");
		return -EBUSY;
	}else{
		rqi->dev_id_map = rqi->dev_id_map | (1 << dev_id);
	}

	//init semaphore
	if((rqi->dev_sem[dev_id] = MMPF_OS_CreateSem( 0 )) >= 0xFE )
	{
		CpuComm_CriticalSectionLeave();
		return -EBUSY;
	}

    //set bitmap to be zero
    rqi->recv_q_slot[dev_id] = 0;

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return 0;
}

int enable_recv_dev_id_noack(unsigned int channel, unsigned int dev_id, unsigned int (*no_ack_func)(void *slot))
{
	int ret=0;
	struct _queue_info *rqi;
	CpuComm_CriticalSectionInit();

	//check channel
	if((channel > MAX_RECV_QUEUE) || (dev_id > MAX_DEV_ID) || (no_ack_func == NULL))
	{
		//printc("ERROR: recv noack channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\r\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done != 1)
	{
		printc("ERROR: recv queue is not initial\n");
		return -EBUSY;
	}else{
		rqi = &(csm_info->recv_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((rqi->dev_id_no_ack & (1 << dev_id)) == (1 << dev_id))
	{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: No ack options have enabled\n");
		return -EBUSY;
	}else{
		rqi->dev_id_no_ack = rqi->dev_id_no_ack | (1 << dev_id);
		rqi->no_ack_func[dev_id] = no_ack_func;
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int close_recv_dev_id(unsigned int channel, unsigned int dev_id)
{
	int ret=0, i;
	struct _queue_info *rqi;
	struct cpu_share_mem_slot *temp_slot;
	CpuComm_CriticalSectionInit();	

	//check channel
	if((channel > MAX_RECV_QUEUE) || (dev_id > MAX_DEV_ID))
	{
		printc("ERROR: channel or dev_id are invalid channel = 0x%x dev_id = 0x%x\n", channel, dev_id);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done != 1)
	{
		printc("ERROR: recv queue is not initial\n");
		return -EBUSY;
	}else{
		rqi = &(csm_info->recv_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check active bitmap
	if((rqi->dev_id_map & (1 << dev_id)) == (1 << dev_id))
	{
		rqi->dev_id_map = rqi->dev_id_map & ~(1 << dev_id);
	}else{
		CpuComm_CriticalSectionLeave();
		printc("ERROR: dev_id is not enabled\n");
		return -EINVAL;
	}

	// check active no ack bitmap
	if((rqi->dev_id_no_ack & (1 << dev_id)) == (1 << dev_id)) {
	    rqi->dev_id_no_ack = rqi->dev_id_no_ack & ~(1 << dev_id);
	}


	//scan slot and ack error
	for(i=0;i<rqi->total_slot;i++)
	{
		//check queue status
		if((rqi->recv_q_slot[dev_id] & (1 << i)) == (1 << i))
		{
			temp_slot = (struct cpu_share_mem_slot *)(rqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
			temp_slot->recv_parm[0] = ACK_ID_DISABLE;
			temp_slot->slot_status = SLOT_ACK;
			MMP_CPUCOMM_IRQ_SET(_CPU_ID_A);
		}
	}

 	// Leave critical section
 	CpuComm_CriticalSectionLeave();

	return ret;
}

int recv_slot(unsigned int channel, unsigned int dev_id, struct cpu_share_mem_slot **slot, unsigned int timeout)
{
	int i;
	struct _queue_info *rqi;
	CpuComm_CriticalSectionInit();

	//check channel and slot
	if(channel >= MAX_RECV_QUEUE)
	{
		printc("ERROR: channel is invalid. channel = 0x%x\n", channel);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done != 1)
	{
		printc("ERROR: recv queue is not initial\n");
		return -EBUSY;
	}else{
		rqi = &(csm_info->recv_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check if queue is empty or all slot are in process
	while((rqi->recv_q_slot[dev_id] == 0) || (rqi->recv_q_slot[dev_id] == rqi->recv_q_proc[dev_id]))
	{
 		// Leave critical section
 		CpuComm_CriticalSectionLeave();

		MMPF_OS_AcquireSem(rqi->dev_sem[dev_id], 0);

		// Enter critial section to ensure operation will not be interrupt by other thread.
		CpuComm_CriticalSectionEnter();

		if(rqi->recv_q_slot[dev_id] == 0)
		{
			//Somethings are wrong. we can still wait until queue is not empty
			//printc("WARNING:Queue is still empty after release\n");
 			//CpuComm_CriticalSectionLeave();
			//return -EIO;
		}
	}

	//scan slot
	for(i=0;i<rqi->total_slot;i++)
	{
		//check queue status
		if((rqi->recv_q_slot[dev_id] & (1 << i)) == (1 << i))
		{
			*slot = (struct cpu_share_mem_slot *)(rqi->slot_v_st_addr + (i * sizeof(struct cpu_share_mem_slot)));
			if((*slot)->slot_status == SLOT_QUEUE)
			{
				//change the status. we will clean the slot status when we ack the slot. It can avoid two many up(release semaphore).
				//rqi->recv_q_slot[dev_id] = rqi->recv_q_slot[dev_id] & ~(1 << i);
				rqi->recv_q_proc[dev_id] = rqi->recv_q_proc[dev_id] | (1 << i);
				(*slot)->slot_status = SLOT_PROC;
				if((*slot)->slot_num != i)
				{
					printc("ERROR: slot num is not match bitmap!!!!\n");
				}
				break;
			}else if((*slot)->slot_status == SLOT_PROC){
				printc("This slot is already in proccess status\n");
			}else{
				// Leave critical section
 				CpuComm_CriticalSectionLeave();
 				printc("ERROR: Slot status is not SLOT_QUEUE when recv slot\n");
				return -EIO;
			}
		}
	}

	// Leave critical section
	CpuComm_CriticalSectionLeave();

	return 0;
}

int ack_slot(unsigned channel, struct cpu_share_mem_slot* slot)
{
	struct _queue_info *rqi;
	CpuComm_CriticalSectionInit();

	//check channel and slot
	if(channel >= MAX_RECV_QUEUE)
	{
		printc("ERROR: channel is invalid. channel = 0x%x\n", channel);
		return -EINVAL;
	}

	//check queue status
	if(csm_info->recv_queue_info[channel].init_done != 1)
	{
		printc("ERROR: recv queue is not initial\n");
		return -EBUSY;
	}else{
		rqi = &(csm_info->recv_queue_info[channel]);
	}

	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();

	//check slot status and change the status to be SLOT_ACK. We need to dequeuq the slot before ack
	if((slot->slot_status != SLOT_PROC))
	{
		// Leave critical section
		CpuComm_CriticalSectionLeave();
		return -EINVAL;
	}else{
		rqi->recv_q_slot[slot->dev_id] = rqi->recv_q_slot[slot->dev_id] & ~(1 << slot->slot_num);
		rqi->recv_q_proc[slot->dev_id] = rqi->recv_q_proc[slot->dev_id] & ~(1 << slot->slot_num);
		slot->slot_status = SLOT_ACK;
		MMP_CPUCOMM_IRQ_SET(_CPU_ID_A);
	}

	// Leave critical section
	CpuComm_CriticalSectionLeave();

	return 0;
}
/*
return the channel which support no ack service
*/
int get_noack_channel(unsigned int dev_id,int is_send)
{
	int i,channel = -EINVAL  ;
	struct _queue_info *qi;
	CpuComm_CriticalSectionInit();

	//check channel
	if( dev_id > MAX_DEV_ID )
	{
		printc("ERROR:get_noack_channel dev_id = 0x%x\r\n", dev_id);
		return -EINVAL;
	}
	// Enter critial section to ensure operation will not be interrupt by other thread.
	CpuComm_CriticalSectionEnter();
    for(i=0;i<MAX_CPU_QUEUE;i++) {
        qi = 0 ;
        if(is_send) {
        	//check queue status
        	if(csm_info->send_queue_info[i].init_done )
        	{
        		qi = &(csm_info->send_queue_info[i]);
        	}
        
        }
        else {
        	//check queue status
        	if(csm_info->recv_queue_info[i].init_done )
        	{
        		qi = &(csm_info->recv_queue_info[i]);
        	}
        }
    	if((qi->dev_id_no_ack & (1 << dev_id)) == (1 << dev_id))
    	{
    	   channel = i ;
    	}
    }
 	CpuComm_CriticalSectionLeave();

	return channel;    
}


//#define CPU_SHAREMEM_TEST
#ifdef CPU_SHAREMEM_TEST
//#define TEST_NO_ACK
#define SEND_CHN        0
#define SEND1_CHN       0
#define SEND_DEV_ID     2
#define SEND1_DEV_ID     11
#define RECV_DEV_ID     1
#define RECV1_DEV_ID     10

static OS_STK  s_CPU_SHAREMEM_RTaskStk[TASK_B_CPU_SHAREMEM_R_STK_SIZE];   // Stack for service task
static OS_STK  s_CPU_SHAREMEM_STaskStk[TASK_B_CPU_SHAREMEM_S_STK_SIZE];   // Stack for service task
static OS_STK  s_CPU_SHAREMEM_S1TaskStk[TASK_B_CPU_SHAREMEM_S1_STK_SIZE];   // Stack for service task
static OS_STK  s_CPU_SHAREMEM_R1TaskStk[TASK_B_CPU_SHAREMEM_R1_STK_SIZE];   // Stack for service task
MMPF_OS_SEMID Send_semaphore;
MMPF_OS_SEMID Send1_semaphore;

unsigned int send1_ack_up(void *slot)
{
        MMPF_OS_ReleaseSem(Send1_semaphore);
        return 0;
}

unsigned int send_ack_up(void *slot)
{
        MMPF_OS_ReleaseSem(Send_semaphore);
        return 0;
}

void DualCpu_CPU_SHARE_TEST_R_Task(void* pData)
{
	int i, j=0;
	struct cpu_share_mem_slot *R_slot, *S_slot;
	printc( "DualCpu_CPU_SHARE_R_TEST Start \r\n" );

	init_recv_dev_id(SEND_CHN, RECV_DEV_ID);

	while(1)
	{
		printc("start recv\r\n");
		while(i<4096)
		{
			if(recv_slot(SEND_CHN, RECV_DEV_ID, &R_slot, 0) == 0)
			{
				R_slot->recv_parm[0] = ACK_CMD_OK;
				ack_slot(SEND_CHN, R_slot);
				i++;
			}else{
				printc("recv_slot error, i = %d\n", i);
			}
		}

		printc("R_Finish j = %d\r\n", j);

		printc("wait ...\r\n");
		j++;
		MMPF_OS_Sleep(1);
		i=0;
	}
	printc( "CPU_SHARE_R_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );		
}

#ifdef TEST_NO_ACK
unsigned int test_no_ack(void *slot)
{
	struct cpu_share_mem_slot *R_slot = (struct cpu_share_mem_slot *)slot;
	printc("slot->slot_num = %d slot->dev_id = 0x%x slot->slot_status = 0x%x\r\n", R_slot->slot_num, R_slot->dev_id, R_slot->slot_status);
	return 0;
}

void DualCpu_CPU_SHARE_TEST_R1_Task(void* pData)
{
	printc( "DualCpu_CPU_SHARE_R1_TEST Start with no ack\r\n" );

	init_recv_dev_id(SEND1_CHN, RECV1_DEV_ID);
	enable_recv_dev_id_noack(SEND1_CHN, RECV1_DEV_ID, test_no_ack);

	while(1)
	{
		MMPF_OS_Sleep(5);

	}
	printc( "CPU_SHARE_R1_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );		
}
#else
void DualCpu_CPU_SHARE_TEST_R1_Task(void* pData)
{
	int i, j=0;
	struct cpu_share_mem_slot *R_slot, *S_slot;
	printc( "DualCpu_CPU_SHARE_R1_TEST Start \r\n" );

	init_recv_dev_id(SEND1_CHN, RECV1_DEV_ID);

	while(1)
	{
		printc("start recv\r\n");
		while(i<4096)
		{
			if(recv_slot(SEND1_CHN, RECV1_DEV_ID, &R_slot, 0) == 0)
			{
				R_slot->recv_parm[0] = ACK_CMD_OK;
				ack_slot(SEND1_CHN, R_slot);
				i++;
			}else{
				printc("recv_slot error, i = %d\n", i);
			}
		}

		printc("R1_Finish j = %d\r\n", j);

		printc("wait ...\r\n");
		j++;
		MMPF_OS_Sleep(1);
		i=0;
	}
	printc( "CPU_SHARE_R1_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );		
}
#endif

void DualCpu_CPU_SHARE_TEST_S_Task(void* pData)
{
	struct cpu_share_mem_slot *R_slot, *S_slot;
	printc( "DualCpu_CPU_SHARE_R_TEST Start \r\n" );

	init_recv_dev_id(SEND_CHN, SEND_DEV_ID);
	init_send_dev_id(SEND_CHN, SEND_DEV_ID);

	if((Send_semaphore = MMPF_OS_CreateSem( 0 )) >= 0xFE )
	{
		printc("Can't create Send_semaphore\r\n");
		MMPF_OS_DeleteTask( OS_PRIO_SELF );
	}

	recv_slot(SEND_CHN, SEND_DEV_ID, &R_slot, 0);
	//printc("Recv one slot slot_num = 0x%x slot_status = 0x%x hdr_phy_addr = 0x%x\r\n", slot->slot_num, slot->slot_status, slot->hdr_phy_addr);
	R_slot->recv_parm[0] = ACK_CMD_OK;
	ack_slot(SEND_CHN, R_slot);

	//wait recv init
	MMPF_OS_Sleep(1);
	printc("Start to Send slot \r\n" );
	
	while(1)
	{
		if(get_slot(SEND_CHN, &S_slot) == 0)
		{
			S_slot->dev_id =SEND_DEV_ID;
			S_slot->command = 24;
			S_slot->ack_func = send_ack_up;
			send_slot(SEND_CHN, S_slot);
			MMPF_OS_AcquireSem(Send_semaphore, 0);
		}else{
			printc("%s get slot fail\n", __func__);
		}
	}

	printc( "CPU_SHARE_S_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );	
}

#ifdef TEST_NO_ACK
void DualCpu_CPU_SHARE_TEST_S1_Task(void* pData)
{
	struct cpu_share_mem_slot *R_slot, *S_slot;
	printc( "DualCpu_CPU_SHARE_R_TEST Start \r\n" );

	init_recv_dev_id(SEND1_CHN, SEND1_DEV_ID);
	init_send_dev_id(SEND1_CHN, SEND1_DEV_ID);
	enable_send_dev_id_noack(SEND1_CHN, SEND1_DEV_ID);

	recv_slot(SEND1_CHN, SEND1_DEV_ID, &R_slot, 0);
	//printc("Recv one slot slot_num = 0x%x slot_status = 0x%x hdr_phy_addr = 0x%x\r\n", slot->slot_num, slot->slot_status, slot->hdr_phy_addr);
	R_slot->recv_parm[0] = ACK_CMD_OK;
	ack_slot(SEND1_CHN, R_slot);

	//wait recv init
	MMPF_OS_Sleep(1);
	printc("Start to Send slot \r\n" );
	
	while(1)
	{
		if(get_slot(SEND1_CHN, &S_slot) == 0)
		{
			S_slot->dev_id =SEND1_DEV_ID;
			S_slot->command = 24;
			send_slot(SEND1_CHN, S_slot);
			MMPF_OS_SleepMs(30);
		}else{
			printc("%s get slot fail\n", __func__);
		}
	}

	printc( "CPU_SHARE_S_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );	
}
#else
void DualCpu_CPU_SHARE_TEST_S1_Task(void* pData)
{
	struct cpu_share_mem_slot *R_slot, *S_slot;
	printc( "DualCpu_CPU_SHARE_R_TEST Start \r\n" );

	init_recv_dev_id(SEND1_CHN, SEND1_DEV_ID);
	init_send_dev_id(SEND1_CHN, SEND1_DEV_ID);

	if((Send1_semaphore = MMPF_OS_CreateSem( 0 )) >= 0xFE )
	{
		printc("Can't create Send_semaphore\r\n");
		MMPF_OS_DeleteTask( OS_PRIO_SELF );
	}

	recv_slot(SEND1_CHN, SEND1_DEV_ID, &R_slot, 0);
	//printc("Recv one slot slot_num = 0x%x slot_status = 0x%x hdr_phy_addr = 0x%x\r\n", slot->slot_num, slot->slot_status, slot->hdr_phy_addr);
	R_slot->recv_parm[0] = ACK_CMD_OK;
	ack_slot(SEND1_CHN, R_slot);

	//wait recv init
	MMPF_OS_Sleep(1);
	printc("Start to Send slot \r\n" );
	
	while(1)
	{
		if(get_slot(SEND1_CHN, &S_slot) == 0)
		{
			S_slot->dev_id =SEND1_DEV_ID;
			S_slot->command = 24;
			S_slot->ack_func = send1_ack_up;
			send_slot(SEND1_CHN, S_slot);
			MMPF_OS_AcquireSem(Send1_semaphore, 0);
		}else{
			printc("%s get slot fail\n", __func__);
		}
	}

	printc( "CPU_SHARE_S_TEST Task Stop\r\n" );

	// Kill task by itself
	MMPF_OS_DeleteTask( OS_PRIO_SELF );	
}
#endif

void EstablishCPU_SHARE_TEST(void)
{

	MMPF_TASK_CFG	task_cfg_R, task_cfg_S;
	MMPF_OS_TASKID	ulTaskId_R, ulTaskId_S;

	task_cfg_R.ubPriority = TASK_B_CPU_SHAREMEM_R_PRIO;
	task_cfg_R.pbos = (MMP_ULONG)&s_CPU_SHAREMEM_RTaskStk[0];
	task_cfg_R.ptos = (MMP_ULONG)&s_CPU_SHAREMEM_RTaskStk[TASK_B_CPU_SHAREMEM_R_STK_SIZE-1];
	ulTaskId_R = MMPF_OS_CreateTask(DualCpu_CPU_SHARE_TEST_R_Task, &task_cfg_R, (void *)2 );
	printc("Create CPU_SHARE_R_TEST Task,id:%d\r\n",ulTaskId_R);

	task_cfg_S.ubPriority = TASK_B_CPU_SHAREMEM_S_PRIO;
	task_cfg_S.pbos = (MMP_ULONG)&s_CPU_SHAREMEM_STaskStk[0];
	task_cfg_S.ptos = (MMP_ULONG)&s_CPU_SHAREMEM_STaskStk[TASK_B_CPU_SHAREMEM_S_STK_SIZE-1];
	ulTaskId_S = MMPF_OS_CreateTask(DualCpu_CPU_SHARE_TEST_S_Task, &task_cfg_S, (void *)2 );
	printc("Create CPU_SHARE_S_TEST Task,id:%d\r\n",ulTaskId_S);	

	task_cfg_R.ubPriority = TASK_B_CPU_SHAREMEM_R1_PRIO;
	task_cfg_R.pbos = (MMP_ULONG)&s_CPU_SHAREMEM_R1TaskStk[0];
	task_cfg_R.ptos = (MMP_ULONG)&s_CPU_SHAREMEM_R1TaskStk[TASK_B_CPU_SHAREMEM_R1_STK_SIZE-1];
	ulTaskId_R = MMPF_OS_CreateTask(DualCpu_CPU_SHARE_TEST_R1_Task, &task_cfg_R, (void *)2 );
	printc("Create CPU_SHARE_R_TEST Task,id:%d\r\n",ulTaskId_R);	

	task_cfg_S.ubPriority = TASK_B_CPU_SHAREMEM_S1_PRIO;
	task_cfg_S.pbos = (MMP_ULONG)&s_CPU_SHAREMEM_S1TaskStk[0];
	task_cfg_S.ptos = (MMP_ULONG)&s_CPU_SHAREMEM_S1TaskStk[TASK_B_CPU_SHAREMEM_S1_STK_SIZE-1];
	ulTaskId_S = MMPF_OS_CreateTask(DualCpu_CPU_SHARE_TEST_S1_Task, &task_cfg_S, (void *)2 );
	printc("Create CPU_SHARE_S_TEST Task,id:%d\r\n",ulTaskId_S);		
}

CPU_COMM_ERR Cpu_sharemem_test_Init(void)
{
	printc( "CpuB_Sharemem_test \r\n" );
	EstablishCPU_SHARE_TEST();
	return CPU_COMM_ERR_NONE;
}

CPUCOMM_MODULE_INIT(Cpu_sharemem_test_Init)

#endif

#endif
