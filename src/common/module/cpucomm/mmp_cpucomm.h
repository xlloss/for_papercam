#ifndef __MMP_CPU_COMM_H__
#define __MMP_CPU_COMM_H__

#include "mmp_reg_int.h"
#include "mmp_reg_gbl.h"
#include "mmp_register.h"

typedef enum
{
    MD_INIT,//0
    MD_GET_VERSION,	
    MD_RUN,
    MD_SET_WINDOW,
    MD_GET_WINDOW_SIZE,
    MD_SET_WINDOW_PARA_IN,//5
    MD_GET_WINDOW_PARA_IN,
    MD_GET_WINDOW_PARA_OUT,
    MD_GET_BUFFER_INFO,
    MD_SUSPEND,
    MD_HISTGRAM,
    MD_SET_ROI_PARA_IN =90,
    // Event to CPU-A definition
    MD_EVENT_CHANGES = 100 ,
    /*
    HW sensor control
    */
    VENDER_CMD=110,
    
    MD_NOT_SUPPORT = -1
} MD_COMMAND;

//#pragma pack(4)


typedef enum
{
    AEC_INIT,//0
    AEC_GET_VERSION,	
    AEC_RUN,
    AEC_NOT_SUPPORT = -1,
    AEC_DBG = (int)0x80000000
} AEC_COMMAND;

typedef enum
{
	AAC_INIT,
	AAC_ENCODE,
	AAC_NOT_SUPPORT = -1 
} AAC_COMMAND ;

typedef enum
{
    AES_INIT,
    AES_ENCODE,
    AES_NOT_SUPPORT
} AES_COMMAND;


typedef enum
{
    IPC_V4L2_OPEN,
    IPC_V4L2_RELEASE,
    IPC_V4L2_STREAMON,
    IPC_V4L2_STREAMOFF,
    IPC_V4L2_CTRL,
    IPC_V4L2_ISR,
    IPC_IQ_TUNING,
    IPC_V4L2_GET_JPG,
    IPC_V4L2_NOT_SUPPORT
} V4L2_COMMAND;

typedef enum
{
    IPC_ALSA_SETOWNER,
    IPC_ALSA_POWERON,
    IPC_ALSA_POWEROFF,
    IPC_ALSA_MUTE,
    IPC_ALSA_GAIN,
    IPC_ALSA_RW,
    IPC_ALSA_ISR,
    IPC_ALSA_TRIGGER_START,
    IPC_ALSA_TRIGGER_STOP,
    IPC_ALSA_SET_AEC,
    IPC_ALSA_NOT_SUPPORT
} ALSA_COMMAND;

typedef enum
{
    WD_START,
    WD_STOP ,	
    WD_PING ,
    WD_SET_TIMEOUT ,
    WD_NOT_SUPPORT   
} WD_COMMAND ;

typedef enum
{
    I2C_QUERY,
    I2C_READ ,	
    I2C_WRITE,
    I2C_NOT_SUPPORT    
} I2C_COMMAND ;

typedef struct _cpu_comm_transfer_data
{
	unsigned int	command;
	unsigned long	phy_addr;
	unsigned long	size;
	unsigned long	seq;
	signed long		result;
#define CPUCOMM_FLAG_RESULT_OK	(1 << 0)		/* Receiver need response */
	unsigned long	flag;
#define CPUCOMM_FLAG_WAIT_FOR_RESP	(1 << 0)	/* Receiver need response */
#define CPUCOMM_FLAG_ACK			(1 << 1)	/* Receiver need response */
#define CPUCOMM_FLAG_CMDSND			(1 << 2)	/* Receiver need response */
	unsigned long	reserved;
} cpu_comm_transfer_data;

typedef enum __CPU_ID
{
    _CPU_ID_A = 0,
    _CPU_ID_B = 1
} _CPU_ID;
extern void printc( char* szFormat, ... );
static __inline void MMP_CPUCOMM_IRQ_SET( _CPU_ID ulId )
{
    AITC_BASE_HINT->HINT_SET_CPU_INT = (ulId==_CPU_ID_A)? HINT_CPU2CPUA_INT : HINT_CPU2CPUB_INT;
}

static __inline void MMP_CPUCOMM_IRQ_CLEAR( _CPU_ID ulId )
{
    AITC_BASE_HINT->HINT_CLR_CPU_INT = (ulId==_CPU_ID_A)? HINT_CPU2CPUA_INT : HINT_CPU2CPUB_INT;
}

static __inline AIT_REG_B * MMP_CPUCOMM_SHARE_REG_GET(void)
{
    return AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG;
}
#endif //__MMP_CPU_COMM_H__
