#ifndef __CPU_SHAREMEM_H__
#define __CPU_SHAREMEM_H__

#include "cpucomm.h"
#include "mmp_cpucomm.h"

#define	DEVICE_NAME_CPUCOMM	"cpucomm"
#define MAX_CPU_QUEUE		2
#define	MAX_SEND_QUEUE		MAX_CPU_QUEUE
#define	MAX_RECV_QUEUE		MAX_CPU_QUEUE
#define MAX_SLOT		32
#define MAX_DEV_ID		64

enum Slot_block
{
	SEND_SLOT_BLOCK_MODE	= 0,
	SEND_SLOT_NONBLOCK_MODE	= 1
};

enum Slot_direct
{
	L_TO_R_SLOT	= (1 << 0),	//Linux to RTOS
	R_TO_L_SLOT	= (1 << 1),	//RTOS to Linux
	A_TO_B_SLOT	= (1 << 2),	//CPUA tO CPUB 
	B_TO_A_SLOT	= (1 << 3)	//CPUB to CPUA
};

enum Slot_status
{
	SLOT_FREE	= 0,
	SLOT_GET	= 1,
	SLOT_SEND	= 2,
	SLOT_QUEUE	= 3,
	SLOT_PROC	= 4,
	SLOT_ACK	= 5,

	SLOT_ERROR	= 0xFF
};

enum Slot_ack_status
{
 	ACK_CMD_OK		= 0,
	ACK_CMD_ERROR		= 1,
	ACK_ID_DISABLE		= 2
};

struct _queue_info {
	unsigned int slot_v_st_addr;
	unsigned int slot_p_st_addr;
	unsigned int total_slot;
	unsigned int free_slot;		//free slot bitmap 1:free 0:used 
	unsigned int used_slot;		//used slot bitmap 0:free 1:used
	unsigned int wait_ack_slot;	//wait ack slot bitmap 0:not wait 1:wait
	unsigned int recv_q_slot[MAX_DEV_ID];	//the bitmap indicate the slot need to process for each device ID.
    unsigned int recv_q_proc[MAX_DEV_ID];	//the bitmap indicate the slot need to process for each device ID.
	unsigned long long dev_id_map;	//active device id bitmap,
	unsigned long long dev_id_no_ack;       //enable slot to work like UDP
	MMPF_OS_SEMID dev_sem[MAX_DEV_ID];
	unsigned int (*no_ack_func[MAX_DEV_ID])(void *slot);
	unsigned short init_done;
	unsigned short seq;
};

struct cpu_share_mem_slot {
	unsigned int dev_id;
	unsigned int command;
	unsigned int data_phy_addr;
	unsigned int size;
	unsigned int hdr_phy_addr;
	unsigned int (*ack_func)(void *slot);
	unsigned short seq;
	unsigned short slot_direct;
	unsigned short slot_num;
	unsigned short slot_status;
//32bytes
	unsigned int send_parm[4];
	unsigned int recv_parm[4];
//32bytes
};

struct _cpu_share_mem_info {
	struct _queue_info send_queue_info[MAX_SEND_QUEUE];
	struct _queue_info recv_queue_info[MAX_RECV_QUEUE];
	unsigned int *baseaddr_hint;
	unsigned int *baseaddr_share;
};

int initial_cpu_share_mem(int max_channel);
void notify_cpux(int max_channel);
unsigned int get_share_register(void);
int Cpu_sharemem_ISR(void);
//send function
int init_send_queue(unsigned int channel, unsigned int slot_v_st_addr, unsigned int slot_p_st_addr, unsigned int total_slot);
int init_send_dev_id(unsigned int channel, unsigned int dev_id);
int enable_send_dev_id_noack(unsigned int channel, unsigned int dev_id);
int close_send_dev_id(unsigned int channel, unsigned int dev_id);
int get_slot(unsigned int channel, struct cpu_share_mem_slot **slot);
int send_slot(unsigned int channel, struct cpu_share_mem_slot *slot); 

//recv function
int init_recv_queue(unsigned int channel, unsigned int slot_v_st_addr, unsigned int slot_p_st_addr, unsigned int total_slot);
int init_recv_dev_id(unsigned int channel, unsigned int dev_id);
int enable_recv_dev_id_noack(unsigned int channel, unsigned int dev_id, unsigned int (*no_ack_func)(void *slot));
int close_recv_dev_id(unsigned int channel, unsigned int dev_id);
int recv_slot(unsigned int channel, unsigned int dev_id, struct cpu_share_mem_slot **slot, unsigned int timeout);
int ack_slot(unsigned channel, struct cpu_share_mem_slot *slot);
int get_noack_channel(unsigned int dev_id,int is_send);
#endif
