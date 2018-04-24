//==============================================================================
//
//  File        : mmpf_cpucomm.c
//  Description : CPU communication between dual CPU
//  Author      : Chiket Lin
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
#include "cpucomm_core.h"
#include "cpu_sharemem.h"
struct cpu_comm_transfer_data{
	unsigned long  command;
	unsigned long  phy_addr;
	unsigned long  size;
	unsigned long  seq;	
	unsigned long  result;
#define CPUCOMM_FLAG_RESULT_OK 		(1 << 0)	/* Receiver need response */	
	unsigned long  flag;
#define CPUCOMM_FLAG_WAIT_FOR_RESP (1 << 0) /* Receiver need response */
#define CPUCOMM_FLAG_ACK    (1 << 1) /* Receiver need response */
#define CPUCOMM_FLAG_CMDSND   (1 << 2) /* Receiver need response */
	
	unsigned long  reserved;	

};
//==============================================================================
//
//                              Define
//
//==============================================================================

//------------------------------------------------------------------------------
//  Variable    : g_psCpuCommTable
//  Description : Store pointer of date entry sent from client modules.
//                Value is assigned because the first ID is 0, the same as init
//                vlaue
//------------------------------------------------------------------------------
/* static __align(CACHE_LINE_SIZE) CPU_COMM_ENTRY           g_psCpuCommTable[CPUCOMM_ID_MAX_NUM] = {{CPU_COMM_ID_ILLEGAL}}; */

static CPU_COMM_ENTRY	g_psCpuCommTable[CPUCOMM_ID_MAX_NUM] __attribute__((aligned(CACHE_LINE_SIZE))) =
{
	{ CPU_COMM_ID_ILLEGAL }
};
static volatile _CPU_ID         s_ulCpuId;
CpuComm_CriticalSectionDeclare()

char* strCommID[CPUCOMM_ID_MAX_NUM] = {

  "SYSFLAG",
  "UART",
  "A2B",
  "MD",
};

#if (CPU_SHAREMEM == 0)
static void CpuComm_DumpData(struct cpu_comm_transfer_data *p)
{
	printc("--cmd 	  : 0x%08x\r\n",p->command );
	printc("--payload : 0x%08x\r\n",p->phy_addr);
	printc("--size    : 0x%08x\r\n",p->size    );
	printc("--seq     : 0x%08x\r\n",p->seq     );
	printc("--result  : 0x%08x\r\n",p->result  );
	printc("--flag    : 0x%08x\r\n",p->flag    ); 
}
#endif

//------------------------------------------------------------------------------
//  Function    : CpuComm_VolatileCopy
//  Description : Copy volatile data
//------------------------------------------------------------------------------
static __inline void CpuComm_VolatileCopy( volatile void *pbyDst, volatile void *pbySrc, MMP_ULONG ulSize )
{
    MMP_ULONG i;

	//printc( "Copy: 0x%x -> 0x%x:\r\n",pbySrc,pbyDst);

    for( i = 0; i<ulSize; i++ )
    {
        ((volatile MMP_UBYTE*)pbyDst)[i] = ((volatile MMP_UBYTE*)pbySrc)[i];
		//printc( " 0x%x ", ((volatile MMP_UBYTE*)pbyDst)[i] );
		//if(i % 8 == 0)
		//	printc( "\r\n");		
    }
	if(ulSize == sizeof(CPU_COMM_SWAP_DATA))
	{
#if 0
		CPU_COMM_SWAP_DATA	*p;
		unsigned long		*pData;

		p = pbyDst;

		//printc("ulCommId = %d (%s)\r\n",p->ulCommId,strCommID[p->ulCommId]);	
        {
		pData =(unsigned long*) p->pbyBuffer;
		printc("command 	= %08x\r\n",pData[0]);
		printc("phy_addr 	= %08x\r\n",pData[1]);
		printc("size 		= %08x\r\n",pData[2]);
		printc("seq 		= %08x\r\n",pData[3]);
		printc("result 		= %08x\r\n",pData[4]);	
		printc("flag 	= %08x\r\n",pData[5]);	
		printc("reserved1 	= %08x\r\n",pData[6]);
		//printc("data0 	= %08x\r\n",(*(unsigned long*)(pData[1])));
		//printc("data1 	= %08x\r\n",(*((unsigned long*)(pData[1])+1)));
		//printc("data2 	= %08x\r\n",(*((unsigned long*)(pData[1])+2)));
		}
#endif
	}
	else
	{
		for( i = 0; i < ulSize; i++ )
		{
		    #if 0
			printc( " 0x%02x ", ((volatile MMP_UBYTE*)pbyDst)[i] );
			if(i % 8 == 0)
				printc( "\r\n");	
			#endif
		}
	}
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_GetSwapData
//  Description : Get global swap data
//                psSwapData in function consists with two swap data for two CPU
//                psSwapData[0]: send CPU_A data to CPU_B
//                psSwapData[1]: send CPU_B data to CPU_A
//                This table is implemented by the CPU share register
//                If we need a bigger size of swap buffer, we have to move
//                the global swap buffer to SRAM. But it means we need to
//                another fixed address in SRAM or CPU A must pass the
//                the address to CPU B during init.
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] ulId : CPU ID
 @retval: a swap data pointer
*/
static __inline CPU_COMM_SWAP_DATA* CpuComm_GetGlobalSwapData( _CPU_ID ulId )
{
    CPU_COMM_SWAP_DATA  *psSwapData = (CPU_COMM_SWAP_DATA *)MMP_CPUCOMM_SHARE_REG_GET();

    return ulId==_CPU_ID_A ? &psSwapData[0] : &psSwapData[1];
}


//------------------------------------------------------------------------------
//  Function    : CpuComm_GetDataEntry
//  Description : Get the comm data entry
//                There is still a problem, we do not check that
//                two comm tables have same entries. Decalrations file
//                may not include in MCP file, and LX is more complicate.
//                Maybe we also need to introduce the table check before init.
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] ulCommId : communication ID, or the index value of communication data table.
 @retval: a communication data entry
*/
static __inline CPU_COMM_ENTRY* CpuComm_GetDataEntry( CPU_COMM_ID ulCommId )
{
    if( ulCommId >= CPUCOMM_ID_MAX_NUM )
        return NULL;

    return &g_psCpuCommTable[ulCommId];
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_InitData
//  Description : Reset and init communication data
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] psData : pointer of communication data
 @retval: TRUE:  init successfully
          FALSE: init fail
*/
static __inline CPU_COMM_ERR CpuComm_InitData( CPU_COMM_ENTRY* psData )
{
    MMPF_OS_SEMID   ulFlagId;
    MMPF_OS_SEMID   ulSemId;

    CpuComm_CriticalSectionInit();

    // Reset comm data
    memset( &psData->sCommData, 0, sizeof(CPU_COMM_DATA) );

    // Create flag and set the initial value as 0
    if( (ulFlagId = MMPF_OS_CreateSem( 0 )) >= 0xFE )
    {
        return CPU_COMM_ERR_INIT_FAIL;
    }

    // Create semaphore for data critical protection
    if( (ulSemId = MMPF_OS_CreateSem( 1 )) >= 0xFE )
    {
        MMPF_OS_DeleteSem( ulFlagId );
        return CPU_COMM_ERR_INIT_FAIL;
    }

    // Enter critial section to ensure operation will not be interrupt by other thread.
    CpuComm_CriticalSectionEnter();

    // Assing flag and semaphore to comm data
    psData->sCommData.ulFlagId = ulFlagId;
    psData->sCommData.ulSemId = ulSemId;

    // Leave critical section
    CpuComm_CriticalSectionLeave();


    return CPU_COMM_ERR_NONE;
}


//------------------------------------------------------------------------------
//  Function    : CpuComm_DestroyData
//  Description : Destroy comm data
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] psCommEntry : pointer of comm data
*/
static __inline void CpuComm_DestroyData( CPU_COMM_ENTRY* psCommEntry )
{
    volatile MMPF_OS_SEMID   ulSemId, ulFlagId;
    CpuComm_CriticalSectionInit();

    // Enter critial section to ensure operation will not be interrupt by other thread.
    CpuComm_CriticalSectionEnter();

    // copy flag and semaphore
    ulFlagId = psCommEntry->sCommData.ulFlagId;
    ulSemId = psCommEntry->sCommData.ulSemId;

    // Reset data
    psCommEntry->ulCommId = CPU_COMM_ID_ILLEGAL;
    psCommEntry->sCommData.ulFlagId = 0xFF;
    psCommEntry->sCommData.ulSemId  = 0xFF;

    // Leave critical section
    CpuComm_CriticalSectionLeave();

    // Delete flag
    MMPF_OS_DeleteSem( ulFlagId );

    // Delete semaphore
    MMPF_OS_DeleteSem( ulSemId );
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_DataCriticalEnter
//  Description : Critical section for data protection.
//------------------------------------------------------------------------------
static __inline MMP_BOOL CpuComm_DataCriticalEnter( CPU_COMM_ENTRY* psCommEntry )
{
    if( MMPF_OS_AcquireSem( psCommEntry->sCommData.ulSemId, 0 ) )
    {
        return MMP_FALSE;
    }

    if( psCommEntry->ulCommId == CPU_COMM_ID_ILLEGAL )
    {
        return MMP_FALSE;
    }

    return MMP_TRUE;
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_DataCriticalLeave
//  Description : Critical section for data protection.
//------------------------------------------------------------------------------
static __inline void CpuComm_DataCriticalLeave( CPU_COMM_ENTRY* psCommEntry )
{
    if( psCommEntry->ulCommId != CPU_COMM_ID_ILLEGAL )
    {
        MMPF_OS_ReleaseSem( psCommEntry->sCommData.ulSemId );
    }
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_CPUISR
//  Description : ISR for the communication between both CPU
//------------------------------------------------------------------------------
void CpuComm_CPUISR( void )
{   
#if (CPU_SHAREMEM == 0)
    CpuComm_SwapISR();
    MMP_CPUCOMM_IRQ_CLEAR( s_ulCpuId );
#else
    MMP_CPUCOMM_IRQ_CLEAR( s_ulCpuId );
    Cpu_sharemem_ISR();
#endif
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_SwapISR
//  Description : ISR for the communication between both CPU
//------------------------------------------------------------------------------
void CpuComm_SwapISR( void )
{
    CPU_COMM_SWAP_DATA*    psGlobalSwapData = CpuComm_GetGlobalSwapData(s_ulCpuId==_CPU_ID_A?_CPU_ID_B:_CPU_ID_A);
    CPU_COMM_ENTRY*        psCommEntry;

	/* struct cpu_comm_transfer_data *p;// =(struct cpu_comm_transfer_data*)psCommEntry->sCommData.sData.pbyBuffer; */
	
    //__UartWrite( "ISR\r\n" );
	
    // If ulFalgData or ulCommandId is illegal, there is nothing to do.
    if( psGlobalSwapData->ulCommId == CPU_COMM_ID_ILLEGAL )
    {
    	printc( "psGlobalSwapData->ulCommId = %d is illegal.\r\n",psGlobalSwapData->ulCommId );
    
        goto ret;
    }

    // Get Comm entry
    psCommEntry = CpuComm_GetDataEntry( psGlobalSwapData->ulCommId );
    if( psCommEntry == NULL )
    {
    	printc( "psCommEntry is NULL\r\n" );
        goto ret;
    }

    // Copy global data to comm data entry
     CpuComm_VolatileCopy( &psCommEntry->sCommData.sData, psGlobalSwapData, sizeof(CPU_COMM_SWAP_DATA) );

	/* p =(struct cpu_comm_transfer_data*)psCommEntry->sCommData.sData.pbyBuffer; */
	//printc("A2B IRQ CommID:%d %s\r\n",psCommEntry->sCommData.sData.ulCommId , p ->flag&CPUCOMM_FLAG_ACK?"ACK":"SND");

    // Set flags to notify data is arrived
    if( MMPF_OS_ReleaseSem( psCommEntry->sCommData.ulFlagId ) )
    {
        // todo ?
    }

ret:
//printc( "Clear IRQ flag \r\n");
    // Clear IRQ flag
    //MMP_CPUCOMM_IRQ_CLEAR( s_ulCpuId );
    // Clear ulFlags of global data to notify another CPU for next swap
   // psGlobalSwapData->ulCommId = CPU_COMM_ID_ILLEGAL;
//printc( "psGlobalSwapData->ulCommId = %d is CPU_COMM_ID_ILLEGAL.\r\n",psGlobalSwapData->ulCommId );
return;
    
}

void CpuComm_SwapISREx( void )
{
    CPU_COMM_SWAP_DATA*    psGlobalSwapData = CpuComm_GetGlobalSwapData(s_ulCpuId==_CPU_ID_A?_CPU_ID_B:_CPU_ID_A);
    CPU_COMM_ENTRY*        psCommEntry;
	
    //__UartWrite( "ISR\r\n" );
//	printc("CpuComm_SwapISREx +\n");
    // If ulFalgData or ulCommandId is illegal, there is nothing to do.
    if( psGlobalSwapData->ulCommId == CPU_COMM_ID_ILLEGAL )
    {
   // 	printc( "psGlobalSwapData->ulCommId = %d is illegal.\r\n",psGlobalSwapData->ulCommId );
    
        goto ret;
    }

//printc( "psGlobalSwapData->ulCommId = %d is Handled.\r\n",psGlobalSwapData->ulCommId );
    // Get Comm entry
    psCommEntry = CpuComm_GetDataEntry( psGlobalSwapData->ulCommId );
    if( psCommEntry == NULL )
    {
    	printc( "psCommEntry is NULL\r\n" );
        goto ret;
    }

    // Copy global data to comm data entry
     CpuComm_VolatileCopy( &psCommEntry->sCommData.sData, psGlobalSwapData, sizeof(CPU_COMM_SWAP_DATA) );
//printc( "MMPF_OS_ReleaseSem = %d \r\n",psCommEntry->sCommData.ulFlagId);
    // Set flags to notify data is arrived
    if( MMPF_OS_ReleaseSem( psCommEntry->sCommData.ulFlagId ) )
    {
        // todo ?
    }

ret:
//printc( "Clear IRQ flag \r\n");
    // Clear IRQ flag
   // MMP_CPUCOMM_IRQ_CLEAR( s_ulCpuId );
    // Clear ulFlags of global data to notify another CPU for next swap
    //psGlobalSwapData->ulCommId = CPU_COMM_ID_ILLEGAL;
//printc( "psGlobalSwapData->ulCommId = %d is CPU_COMM_ID_ILLEGAL.\r\n",psGlobalSwapData->ulCommId );
	//printc("CpuComm_SwapISREx -\n");
	return;
    
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_Swap
//  Description : Swap data to another CPU
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] psSwapData:  pointer of swap data
 @retval: TRUE: swap successful
          FALSE swap timeout
*/
CPU_COMM_ERR CpuComm_Swap(CPU_COMM_SWAP_DATA* psSwapData)
{
    #define TIMEOUT_COUNT   (1000)

    CPU_COMM_SWAP_DATA* psGlobalSwapData = CpuComm_GetGlobalSwapData(s_ulCpuId);
    MMP_ULONG           iTimeoutCount = TIMEOUT_COUNT;
    CPU_COMM_ERR        ulRet = CPU_COMM_ERR_NONE;

    CpuComm_CriticalSectionInit();

    CpuComm_CriticalSectionEnter();
	//printc( "Swap: %d\r\n",psGlobalSwapData->ulCommId);

    // Busy wait for another CPU,
    while( (psGlobalSwapData->ulCommId != CPU_COMM_ID_ILLEGAL))// && (--iTimeoutCount > 0) )
    {
        // To avoid dead lock of dual disabled IRQ, both side should
        CpuComm_SwapISREx();
        
    }

    // Swap timeout?
    if( iTimeoutCount == 0 )
    {
        ulRet =  CPU_COMM_ERR_TIMEOUT;
        goto ERROR;
    }

    // copy data
     CpuComm_VolatileCopy( psGlobalSwapData, psSwapData, sizeof(CPU_COMM_SWAP_DATA) );
    // Trigger IRQ to antoher CPU
    //printc("IRQ:B2A\r\n");
    MMP_CPUCOMM_IRQ_SET( s_ulCpuId==_CPU_ID_A?_CPU_ID_B:_CPU_ID_A );
ERROR:
    CpuComm_CriticalSectionLeave();
    return ulRet;
}

#if (CPU_SHAREMEM == 0)
static CPU_COMM_ERR CpuComm_CommWaitAck( CPU_COMM_ENTRY* psCommEntry,
                                        MMP_UBYTE *pbyData,
                                        MMP_ULONG ulDataSize,
                                        MMP_ULONG ulTimeout )
{
	struct cpu_comm_transfer_data *p =(struct cpu_comm_transfer_data*)psCommEntry->sCommData.sData.pbyBuffer;
    CPU_COMM_SWAP_DATA*    psGlobalSwapData = CpuComm_GetGlobalSwapData(s_ulCpuId==_CPU_ID_A?_CPU_ID_B:_CPU_ID_A);
	//printc("CPUCOMM: Start CommWait %d\r\n",psCommEntry->sCommData.ulFlagId);

	
    // Wait flag
    if( MMPF_OS_AcquireSem( psCommEntry->sCommData.ulFlagId, ulTimeout ) )
    {
        printc("CPUCOMM: %s CPU COMM error event\n",__func__);    
        return CPU_COMM_ERR_EVENT;
    }

	//pr_debug("CPUCOMM: Exit CommWait %d\n",psCommEntry->sCommData.ulFlagId);		
	
    if( psCommEntry->ulCommId == CPU_COMM_ID_ILLEGAL )
    {
        printc("CPUCOMM: %s CPU COMM ID illegal\r\n",__func__);		    
        return CPU_COMM_ERR_DESTROY;
    }

    if( !(p->flag & CPUCOMM_FLAG_ACK ))
    {
        printc("CPUCOMM: %s Ack(%x) format is wrong.\r\n",__func__,p->flag);	
        CpuComm_DumpData(p) ;
                while(1);	    
        return CPU_COMM_ERR_UNSUPPORT;
    }

    if( (p->result!= CPUCOMM_FLAG_RESULT_OK ))
    {
        printc("CPUCOMM:%s result = %d\r\n",__func__,p->result);
        while(1);
        return p->result;
    }

    // Copy Respose Data
    if( pbyData != NULL )
    {
        // memcpy( pbyData, psCommEntry->sCommData.sData.pbyBuffer, ulDataSize );
        CpuComm_VolatileCopy( pbyData, psCommEntry->sCommData.sData.pbyBuffer, ulDataSize );
    }
    psGlobalSwapData->ulCommId = CPU_COMM_ID_ILLEGAL;
    return CPU_COMM_ERR_NONE;
}
#endif

//------------------------------------------------------------------------------
//  Function    : CpuComm_CommWait
//  Description : Check or wait the response from another CPU
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] psCommEntry:  comm data entry
 @param[in] pbyData:      pointer to store the return data
 @param[in] ulDataSize:   data size
 @param[in] ulTimeout:    timeout count
 @retval: CPU_COMM_ERR
*/
static CPU_COMM_ERR CpuComm_CommWait( CPU_COMM_ENTRY* psCommEntry,
                                        MMP_UBYTE *pbyData,
                                        MMP_ULONG ulDataSize,
                                        MMP_ULONG ulTimeout )
{
	//printc( "Wait\r\n");
    CPU_COMM_SWAP_DATA*    psGlobalSwapData = CpuComm_GetGlobalSwapData(s_ulCpuId==_CPU_ID_A?_CPU_ID_B:_CPU_ID_A);
    // Wait flag
    if( MMPF_OS_AcquireSem( psCommEntry->sCommData.ulFlagId, ulTimeout ) )
    {
        return CPU_COMM_ERR_EVENT;
    }

    if( psCommEntry->ulCommId == CPU_COMM_ID_ILLEGAL )
    {
        return CPU_COMM_ERR_DESTROY;
    }

	//printc( "Rcv: ID:%d (%s)\r\n", psCommEntry->ulCommId,strCommID[psCommEntry->ulCommId] );

    // Copy Respose Data
    if( pbyData != NULL )
    {
        // memcpy( pbyData, psCommEntry->sCommData.sData.pbyBuffer, ulDataSize );
        CpuComm_VolatileCopy( pbyData, psCommEntry->sCommData.sData.pbyBuffer, ulDataSize );
    }
    psGlobalSwapData->ulCommId = CPU_COMM_ID_ILLEGAL;
    return CPU_COMM_ERR_NONE;
}



static CPU_COMM_ERR CpuComm_CommAck( CPU_COMM_ENTRY* psCommEntry,
                                        MMP_UBYTE *pbyData,
                                        MMP_ULONG ulDataSize )
{
    // use local data to send data
    CPU_COMM_SWAP_DATA sData;

    // Prepare swap buffer
    sData.ulCommId    = psCommEntry->ulCommId;
	//printc( "Ack: ID: %d\r\n", psCommEntry->ulCommId );
    // memset( sData.pbyBuffer, 0, SWAP_BUFFER_SIZE );

    if( pbyData != NULL )
    {
        CpuComm_VolatileCopy( sData.pbyBuffer, pbyData, ulDataSize );
    }
    // Swap Data
    return CpuComm_Swap( &sData );
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_CommPost
//  Description : prepare and send data to another CPU
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] psCommEntry:  pointer of comm data entry
 @param[in] pbyData:      data which will be sent to another CPU
 @param[in] ulDataSize:   data size
 @retval: CPU_COMM_ERR
*/
static CPU_COMM_ERR CpuComm_CommPost( CPU_COMM_ENTRY* psCommEntry,
                                        MMP_UBYTE *pbyData,
                                        MMP_ULONG ulDataSize )
{
    // use local data to send data
    CPU_COMM_SWAP_DATA sData;

    // Prepare swap buffer
    sData.ulCommId    = psCommEntry->ulCommId;
	//printc( "Post: ID: %d %x\r\n", psCommEntry->ulCommId,  pbyData);
    // memset( sData.pbyBuffer, 0, SWAP_BUFFER_SIZE );
    if( pbyData != NULL )
    {
        CpuComm_VolatileCopy( sData.pbyBuffer, pbyData, ulDataSize );
    }
    // Swap Data
    return CpuComm_Swap( &sData );
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_FlagSet
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in]  ulCommId:  comm ID
 @retval: CPU_COMM_ERR
*/
CPU_COMM_ERR CpuComm_FlagSet( CPU_COMM_ID ulCommId )
{
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);
	printc( "Set Flag\r\n");
	
    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Only flag can call this API
    if( psEntry->ulCommType != CPU_COMM_TYPE_FLAG )
        return CPU_COMM_ERR_UNSUPPORT;

    return CpuComm_CommPost( psEntry, NULL, 0 );
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_FlagGet
//  Description : event flag for dual cpu communication
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in]  ulCommId:    comm ID
 @param[in]  ulTimeout:   timeout count
 @retval: CPU_COMM_ERR
*/
CPU_COMM_ERR CpuComm_FlagGet( CPU_COMM_ID ulCommId,
                              MMP_ULONG ulTimeout )
{
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);
	printc( "Get Flag\r\n");
	
    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Only flag can call this API
    if( psEntry->ulCommType != CPU_COMM_TYPE_FLAG )
        return CPU_COMM_ERR_UNSUPPORT;

    return CpuComm_CommWait( psEntry, NULL, 0, ulTimeout );
}
#if (CPU_SHAREMEM ==1)
#define CPUCOMM_CHN		0 // with ack
#define CPUCOMM_CHN_ISR 1 // without ack
MMPF_OS_SEMID send_semaphore[MAX_DEV_ID];

unsigned int cpucomm_send_ack(void *slot)
{
	struct cpu_share_mem_slot *S_slot = (struct cpu_share_mem_slot *)slot;
	struct cpu_comm_transfer_data *data = (struct cpu_comm_transfer_data *)S_slot->send_parm[3];
	data->result = S_slot->recv_parm[0];
	MMPF_OS_ReleaseSem(send_semaphore[S_slot->dev_id]);
	return 0;
}

CPU_COMM_ERR CpuComm_SocketSend( CPU_COMM_ID ulCommId,
                                  MMP_UBYTE *pbyData,
                                  MMP_ULONG ulDataSize )
{
	struct cpu_share_mem_slot *S_slot;
	struct cpu_comm_transfer_data *data = (struct cpu_comm_transfer_data *)pbyData;

	while(1)
	{
		if(get_slot(CPUCOMM_CHN, &S_slot) == 0)
		{
			S_slot->dev_id = ulCommId;
			S_slot->command = data->command;
			S_slot->data_phy_addr = data->phy_addr;
			S_slot->size = data->size;
			S_slot->send_parm[0] = data->seq;
			S_slot->send_parm[1] = data->flag;
			S_slot->send_parm[2] = data->result;
			S_slot->send_parm[3] = (unsigned int)pbyData;
			S_slot->ack_func = cpucomm_send_ack;
			send_slot(CPUCOMM_CHN, S_slot);
			MMPF_OS_AcquireSem(send_semaphore[S_slot->dev_id], 0);
			break;
		}else{
			printc("%s can't get slot\r\n", __func__);
		}
	}

	if(data->result != CPUCOMM_FLAG_RESULT_OK)
	{
		printc("%s result = 0x%x\n", __func__, data->result);
		return data->result;
	}

	return CPU_COMM_ERR_NONE;
}

CPU_COMM_ERR CpuComm_SocketReceive( CPU_COMM_ID ulCommId,
                                    MMP_UBYTE *pbyData,
                                    MMP_ULONG ulDataSize,
                                    MMP_BOOL bPost )
{
	struct cpu_share_mem_slot *R_slot;
	struct cpu_comm_transfer_data *data = (struct cpu_comm_transfer_data *)pbyData;

	while(1)
	{
		if(recv_slot(CPUCOMM_CHN, ulCommId, &R_slot, 0) == 0)
		{
			data->command = R_slot->command;
			data->phy_addr = R_slot->data_phy_addr;
			data->size = R_slot->size;
			data->seq = R_slot->send_parm[0];
			data->flag = R_slot->send_parm[1];
			data->result = R_slot->send_parm[2];
			R_slot->recv_parm[0] = CPUCOMM_FLAG_RESULT_OK;
			ack_slot(CPUCOMM_CHN, R_slot);
			break;
		}else{
			printc("%s queue is empty after release\n", __func__);
		}
	}

	return CPU_COMM_ERR_NONE;
}

#else
//------------------------------------------------------------------------------
//  Function    : CpuComm_SocketSend
//  Description : Send data to another CPU
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in] ulCommId:   comm ID
 @param[in] pbyData:    data for the service routine
 @param[in] ulDataSize: data size
 @retval: CPU_COMM_ERR
*/
CPU_COMM_ERR CpuComm_SocketSend( CPU_COMM_ID ulCommId,
                                  MMP_UBYTE *pbyData,
                                  MMP_ULONG ulDataSize )
{
    CPU_COMM_ERR   ulErr;
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);

	//printc( "Socket Send\r\n");

    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Only client can call this API
    if( psEntry->ulCommType != CPU_COMM_TYPE_SOCKET )
        return CPU_COMM_ERR_UNSUPPORT;

    if( ulDataSize > SWAP_BUFFER_SIZE )
        return CPU_COMM_ERR_OVERFLOW;

    // Enter data critical section
    if( !CpuComm_DataCriticalEnter( psEntry ) )
        return CPU_COMM_ERR_DESTROY;
        
    // Send data to antoher CPU
    if( ( ulErr = CpuComm_CommPost( psEntry, pbyData, ulDataSize ) ) != CPU_COMM_ERR_NONE )
    {
        CpuComm_DataCriticalLeave( psEntry );
        return ulErr;
    }
    
    // Wait for acknoledge
    ulErr = CpuComm_CommWaitAck( psEntry, NULL, 0, 0 );

    
    // Leave data critical section
    CpuComm_DataCriticalLeave( psEntry );

    return ulErr;
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_SocketReceive
//  Description : Receive data from another CPU
//                This API would block the thread until data is received
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in]  ulCommId:   comm ID
 @param[out] pbyData:    data for the service side
 @param[in]  ulDataSize: data size
 @retval: CPU_COMM_ERR
*/
CPU_COMM_ERR CpuComm_SocketReceive( CPU_COMM_ID ulCommId,
                                    MMP_UBYTE *pbyData,
                                    MMP_ULONG ulDataSize,
                                    MMP_BOOL bPost )
{
    CPU_COMM_ERR   ulErr;
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);

	//printc( "Wait Data Rcv\r\n");

    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Only client can call this API
    if( psEntry->ulCommType != CPU_COMM_TYPE_SOCKET )
        return CPU_COMM_ERR_UNSUPPORT;

    if( ulDataSize > SWAP_BUFFER_SIZE )
        return CPU_COMM_ERR_OVERFLOW;

    // Enter data critical section
    if( !CpuComm_DataCriticalEnter( psEntry ) )
        return CPU_COMM_ERR_DESTROY;

    // Wait for receiving data
    ulErr = CpuComm_CommWait( psEntry, pbyData, ulDataSize, 0 );	
           
    // Response an acknoledge
    if( ulErr == CPU_COMM_ERR_NONE && bPost == MMP_TRUE)
    {
//printc("Post IRQ:     CpuComm_SocketReceive\n");    
//        ulErr = CpuComm_CommPost( psEntry, NULL, 0 );
#if 1 //20150907 return ack
		struct cpu_comm_transfer_data ack,*get_data = (struct cpu_comm_transfer_data *)pbyData ;
		ack.command = get_data->command ;
	    ack.result = CPUCOMM_FLAG_RESULT_OK; 
	    ack.flag = CPUCOMM_FLAG_ACK;
	    ack.phy_addr = 0;
	    ack.size = 0; 
	    //printc("recv.ack : %d\r\n",ulCommId);
#endif

        ulErr = CpuComm_CommAck( psEntry,(MMP_UBYTE *) &ack, sizeof(struct cpu_comm_transfer_data) );
    }
    // Leave data critical section
    CpuComm_DataCriticalLeave( psEntry );
    
    return ulErr;
}
#endif
//------------------------------------------------------------------------------
//  Function    : CpuComm_SocketPost
//  Description : Post to another CPU
//                This API would block the thread until data is received
//------------------------------------------------------------------------------
/**
 Parameters:
 @param[in]  ulCommId:   comm ID
 @retval: CPU_COMM_ERR
*/
CPU_COMM_ERR CpuComm_SocketPost( CPU_COMM_ID ulCommId, unsigned long subCommandID )
{
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);
    CPU_COMM_ERR   ulErr;
	//printc( "Socket Rcv\r\n");


	struct cpu_comm_transfer_data data;

	data.command = subCommandID;
    data.result = CPUCOMM_FLAG_RESULT_OK; 
    data.flag = CPUCOMM_FLAG_ACK;
    data.phy_addr = 0;
    data.size = 0; 
    
    
    
    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Only client can call this API
    if( psEntry->ulCommType != CPU_COMM_TYPE_SOCKET )
        return CPU_COMM_ERR_UNSUPPORT;

    // Enter data critical section
    if( !CpuComm_DataCriticalEnter( psEntry ) )
        return CPU_COMM_ERR_DESTROY;

//printc("Post IRQ:     CpuComm_SocketPost\r\n");
    ulErr = CpuComm_CommAck( psEntry,(MMP_UBYTE*) &data, sizeof(data) );
    // Leave data critical section
    CpuComm_DataCriticalLeave( psEntry );
    
    return ulErr;
}
#if (CPU_SHAREMEM ==1)
CPU_COMM_ERR CpuComm_RegisterEntry( CPU_COMM_ID ulCommId, CPU_COMM_TYPE ulCommType )
{
	if(init_send_dev_id(CPUCOMM_CHN, ulCommId) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}

	if(init_recv_dev_id(CPUCOMM_CHN, ulCommId) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}
	#if MAX_CPU_QUEUE > 1
	if(init_send_dev_id(CPUCOMM_CHN_ISR, ulCommId) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}

	if(init_recv_dev_id(CPUCOMM_CHN_ISR, ulCommId) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}
    #endif

	if((send_semaphore[ulCommId] = MMPF_OS_CreateSem( 0 )) >= 0xFE )
	{
		printc("Can't create Send_semaphore\r\n");
		return CPU_COMM_ERR_INIT_FAIL;
	}

	return CPU_COMM_ERR_NONE;
}

CPU_COMM_ERR CpuComm_UnregisterEntry( CPU_COMM_ID ulCommId )
{
        if(close_send_dev_id(CPUCOMM_CHN, ulCommId) !=0)
        {
                return CPU_COMM_ERR_INIT_FAIL;
        }

        if(close_recv_dev_id(CPUCOMM_CHN, ulCommId) !=0)
        {
                return CPU_COMM_ERR_INIT_FAIL;
        }
    #if MAX_CPU_QUEUE > 1
    if(close_send_dev_id(CPUCOMM_CHN_ISR, ulCommId) !=0)
    {
            return CPU_COMM_ERR_INIT_FAIL;
    }

    if(close_recv_dev_id(CPUCOMM_CHN_ISR, ulCommId) !=0)
    {
            return CPU_COMM_ERR_INIT_FAIL;
    }
    #endif
        return CPU_COMM_ERR_NONE;
}

CPU_COMM_ERR CpuComm_RegisterISRService(CPU_COMM_ID ulCommId ,unsigned int (*recv_cb)(void *slot))
{

    #if MAX_CPU_QUEUE > 1
	if(enable_send_dev_id_noack(CPUCOMM_CHN_ISR, ulCommId) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}
	if(enable_recv_dev_id_noack(CPUCOMM_CHN_ISR, ulCommId, recv_cb) != 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}
	#endif
    return CPU_COMM_ERR_NONE ;
}

#else
//------------------------------------------------------------------------------
//  Function    : CpuComm_RegisterEntry
//  Description : Register entry
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuComm_RegisterEntry( CPU_COMM_ID ulCommId, CPU_COMM_TYPE ulCommType )
{
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);

    if( psEntry == NULL || psEntry->ulCommId == ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    // Set ID & Type
    psEntry->ulCommId = ulCommId;
    psEntry->ulCommType = ulCommType;

    // init data
    CpuComm_InitData( psEntry );

    return CPU_COMM_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_UnregisterEntry
//  Description : unregister entry
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuComm_UnregisterEntry( CPU_COMM_ID ulCommId )
{
    CPU_COMM_ENTRY *psEntry = CpuComm_GetDataEntry(ulCommId);

    if( psEntry == NULL || psEntry->ulCommId != ulCommId )
        return CPU_COMM_ERR_INIT_FAIL;

    CpuComm_DestroyData( psEntry );

    // memset( psEntry, 0, sizeof(CPU_COMM_ENTRY) );
    psEntry->ulCommId = CPU_COMM_ID_ILLEGAL;

    return CPU_COMM_ERR_NONE;
}
#endif
//------------------------------------------------------------------------------
//  Function    : CpuComm_HwInit
//  Description : Init cpucomm HW
//                Current Implementation hard-code the register access
//                We should modify the CpuComm_HwInit() to recive the
//                register settings from ouside world.
//------------------------------------------------------------------------------
void CpuComm_HwInit( _CPU_ID ulCpuId )
{
    CPU_COMM_ID     i;
    CPU_COMM_ENTRY  *psEntry;

    // assign CPU ID
    s_ulCpuId = ulCpuId;

    // Reset data entry
    for( i = 0; i < CPUCOMM_ID_MAX_NUM; i++ )
    {
        psEntry = CpuComm_GetDataEntry(i);

        if( psEntry != NULL )
        {
            // memset( psEntry, 0, sizeof(CPU_COMM_ENTRY) );
            psEntry->ulCommId = CPU_COMM_ID_ILLEGAL;
        }
    }

    // Reset globabl swap buffer
    CpuComm_GetGlobalSwapData(ulCpuId)->ulCommId = CPU_COMM_ID_ILLEGAL;
}

#if 1
/*
 * Peterson's 2-process Mutual Exclusion Algorithm
 * for SMP (Dual-Core) to protect single kernel.
 */
//char _q_[2] = {0, 0};
//char _turn_ = 0;
static int _core_id_  = CPU_B;
//extern unsigned char OSCoreNesting;
static CPU_LOCKCORE *cpu_lock = (CPU_LOCKCORE *)( AITC_BASE_CPU_IPC->CPU_IPC ) ;

void OS_LockCore(void)
{
	volatile char     *p, *q;
	cpu_lock->_corenesting++;
	if (cpu_lock->_corenesting != 1) {
	    //printc("Nesting %d\r\n", cpu_lock->_corenesting);
	}
	cpu_lock->_q_[_core_id_] = 1;
	cpu_lock->_turn_ = _core_id_;
	q  = &cpu_lock->_q_[_core_id_ ^ 1];
	p  = &cpu_lock->_turn_;
	while (*q == 1 && *p == _core_id_) ; 
}

void OS_UnlockCore(void/*int cpu_sr*/)
{
	cpu_lock->_corenesting--;
	cpu_lock->_q_[_core_id_] = 0;
}

void OS_CPULockInfo(void) 
{
	printc("nesting : %d\r\n",cpu_lock->_corenesting);
	printc("_q_ : [%d,%d]\r\n",cpu_lock->_q_[0],cpu_lock->_q_[1]);
	printc("_turn_ : [%d]\r\n",cpu_lock->_turn_);
	
}

#endif
