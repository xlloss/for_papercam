//==============================================================================
//
//  File        : mmpf_uart.c
//  Description : Firmware UART Control Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "mmpf_uart.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_uart.h"
#include "lib_retina.h"

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static AITPS_US             m_pUS[MMP_UART_MAX_COUNT];
#if !defined(MINIBOOT_FW)
static MMPF_OS_SEMID        m_UartSemID[MMP_UART_MAX_COUNT];
#endif
static MMP_BOOL	            m_UartInDmaMode[MMP_UART_MAX_COUNT];

#if (UART_RXINT_MODE_EN == 1)
static MMP_BOOL	            m_UartRxEnable[MMP_UART_MAX_COUNT];
static UartCallBackFunc     *m_UartRx_CallBack[MMP_UART_MAX_COUNT];
#define RX_ENTER_SIGNAL  	(13)
#define RX_SENSITIVE  		(100)
#define RX_QUEUE_SIZE		(128)
#endif

#if (UART_DMA_MODE_EN == 1)
#if !defined(MINIBOOT_FW)
static UartCallBackFunc     *m_UartDma_CallBack[MMP_UART_MAX_COUNT];
#endif
#endif

#if (USER_STRING)
extern size_t strlen (const char *str); 
#endif
//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
#if 0
void ____Uart_DMA_Function____(){ruturn;} //dummy
#endif

#if !defined(MINIBOOT_FW)
//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_SwitchToDmaMode
//  Description :
//  Note        : This is the 1st step of UART using DMA mode
//------------------------------------------------------------------------------
/** @brief This function set the UART device from normal mode to DMA mode.

This function set the UART device from normal mode to DMA mode.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] bEnable stands for enable switch to DMA mode or back from DMA mode.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_Uart_SwitchToDmaMode(MMP_UART_ID uart_id, MMP_BOOL bEnable, MMP_UBYTE* ubUartCtl)
{
	#if (UART_DMA_MODE_EN == 1)

    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id != MMP_UART_ID_0) {
        RTNA_DBG_Str(0, "No DMA mode on Uart1,2,3\r\n");
        return MMP_ERR_NONE;    
    }
     
	if (bEnable == MMP_TRUE) 
	{
		// To use DMA interrupt mode normanlly, we should turn off the interrupt we used in UART normal mode
		#if (UART_RXINT_MODE_EN == 1)
		pUS->US_IER &= (~US_RX_FIFO_OVER_THRES);
		#endif
		
		*ubUartCtl = (MMP_UBYTE)(pUS->US_CR & (~US_CLEAN_CTL0));

		pUS->US_CR |= US_TXEN;      // Clean FIFO first. //EROY CHECK
		#if !defined(MINIBOOT_FW)
		MMPF_OS_Sleep(10);
		#endif
		pUS->US_CR &= US_CLEAN_CTL0;// Switch to dma mode,
		m_UartInDmaMode[uart_id] = MMP_TRUE;
	}
	else {

    	pUS->US_CR &= US_CLEAN_CTL0; //Switch to Normal mode, //EROY CHECK
    	pUS->US_CR |= *ubUartCtl;
    	#if (UART_RXINT_MODE_EN == 1)
		pUS->US_IER = US_RX_FIFO_OVER_THRES;
		#endif
    	m_UartInDmaMode[uart_id] = MMP_FALSE;
	}
	#else
	RTNA_DBG_Str(0, "No DMA mode: UART_DMA_MODE_EN = 0 !!\r\n");
	#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_SetTxDmaMode
//  Description :
//  Note        : This is the 2nd step of UART using DMA TX mode
//------------------------------------------------------------------------------
/** @brief This function set the parameters using by UART DMA TX mode.

This function set the parameters using by UART DMA Tx mode.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] uart_dma_mode indicate which DMA mode to be used, please refer the data structure MMP_UART_DMAMODE.
@param[in] ulTxStartAddr indicate the Tx DMA start address.
@param[in] usTxTotalByte indicate number of bytes would be sent (start from start address).
@return It reports the status of the operation.
*/
MMP_ERR MMPF_Uart_SetTxDmaMode(MMP_UART_ID uart_id, MMP_UART_DMAMODE uart_dma_mode, MMP_ULONG ulTxStartAddr, MMP_USHORT usTxTotalByte)
{
	#if (UART_DMA_MODE_EN == 1)
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id != MMP_UART_ID_0) {
        RTNA_DBG_Str(0, "No DMA mode on Uart1,2,3\r\n");
        return MMP_ERR_NONE;
    }
	
	if (uart_dma_mode == MMP_UART_TXDMA) {
		pUS->US_TXDMA_START_ADDR = ulTxStartAddr;
		pUS->US_TXDMA_TOTAL_BYTE = usTxTotalByte;
	}
	else {
		return MMP_UART_ERR_PARAMETER;
	}
	#else
	RTNA_DBG_Str(0, "No DMA mode: UART_DMA_MODE_EN = 0 !!\r\n");
	#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_SetRxDmaMode
//  Description :
//  Note        : This is the 2nd step of UART using DMA RX mode
//------------------------------------------------------------------------------
/** @brief This function set the parameters using by UART DMA RX mode.

This function set the parameters using by UART DMA Tx mode.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] uart_dma_mode indicate which DMA mode to be used, please refer the data structure MMP_UART_DMAMODE.
@param[in] ulRxStartAddr indicate the RX DMA start address.
@param[in] ulRxEndAddr indicate the RX DMA End address.
@param[in] ulRxLowBoundAddr indicate the RX lower bound address (Using by RX DMA Ring Mode). 
@return It reports the status of the operation.
*/
MMP_ERR MMPF_Uart_SetRxDmaMode(MMP_UART_ID uart_id, MMP_UART_DMAMODE uart_dma_mode, MMP_ULONG ulRxStartAddr, MMP_ULONG ulRxEndAddr, MMP_ULONG ulRxLowBoundAddr)
{
	#if (UART_DMA_MODE_EN == 1)
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id != MMP_UART_ID_0) {
        RTNA_DBG_Str(0, "No DMA mode on Uart1,2,3\r\n");
        return MMP_ERR_NONE;
    }
	
	if ((uart_dma_mode == MMP_UART_RXDMA) || (uart_dma_mode == MMP_UART_RXDMA_RING)) {

		pUS->US_RXDMA_START_ADDR = ulRxStartAddr;
		
		if(uart_dma_mode == MMP_UART_RXDMA_RING) {
			// Note: for ring mode, RX_DMA_END_ADDR must be 8 byte alignment
			pUS->US_RXDMA_END_ADDR = ulRxEndAddr;
			pUS->US_RXDMA_LB_ADDR  = ulRxLowBoundAddr;
		}	
	}
	else {
		return MMP_UART_ERR_PARAMETER;
	}
	#else
	RTNA_DBG_Str(0, "No DMA mode: UART_DMA_MODE_EN = 0 !!\r\n");
	#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_SetDmaInterruptMode
//  Description :
//  Note        : This is the step of UART using DMA interrupt mode settings. (This step can be done betwee step2 and step3)
//------------------------------------------------------------------------------
/** @brief This function sets the UART DMA interrupt mode.

This function sets the UART DMA interrupt mode.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] uart_int_mode indicate which DMA interrupt mode to be used, please refer the data structure MMP_UART_DMA_INT_MODE.
@param[in] bEnable stands for "enable the related interrupt mode or not".
@param[in] callBackFunc is used as interrupt handler.
@param[in] usRxThreshold is used by RX DMA mode, when dma count reaches the Threshold and the related interrupt occurs. 
@param[in] ubRxTimeOut is used in RX interrupt mode (MMP_UART_RXDMA_WRMEM_EN & MMP_UART_RXDMA_DROPDATA_EN)
@return It reports the status of the operation.
*/
MMP_ERR MMPF_Uart_SetDmaInterruptMode(MMP_UART_ID uart_id, MMP_UART_DMA_INT_MODE uart_int_mode, MMP_BOOL bEnable, UartCallBackFunc* callBackFunc, MMP_USHORT usRxThreshold, MMP_UBYTE ubRxTimeOut)
{
	#if (UART_DMA_MODE_EN == 1)
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id != MMP_UART_ID_0) {
        RTNA_DBG_Str(0, "No DMA mode on Uart1,2,3\r\n");
        return MMP_ERR_NONE;
    }

	if (bEnable == MMP_TRUE) 
	{
		switch (uart_int_mode) {
			case MMP_UART_TXDMA_END_EN:
				pUS->US_IER |= US_TXDMA_END_EN;
				break;
			case MMP_UART_RXDMA_TH_EN:
				pUS->US_RXDMA_TOTAL_THR = usRxThreshold;
				pUS->US_IER |= US_RXDMA_TH_EN;
				break;
			case MMP_UART_RXDMA_WRMEM_EN:
				pUS->US_IER |= US_RXDMA_WRMEM_EN;
				pUS->US_BRGR |= (ubRxTimeOut << US_RX_TIMEOUT_SHIFTER);
				pUS->US_CR |= US_RX_TIMEOUT_EN;
				break;
			case MMP_UART_RXDMA_DROPDATA_EN:
				pUS->US_IER |= US_RXDMA_DROPDATA_EN;
				pUS->US_BRGR |= (ubRxTimeOut << US_RX_TIMEOUT_SHIFTER);
				pUS->US_CR |= US_RX_TIMEOUT_EN;
				break;
			default:
				return MMP_UART_ERR_PARAMETER;
				break;
		}
		
		if (callBackFunc != NULL) {	
			m_UartDma_CallBack[uart_id] = callBackFunc;
		}
	}
	else 
	{
		switch (uart_int_mode) {
			case MMP_UART_TXDMA_END_EN:
				pUS->US_IER &= (~US_TXDMA_END_EN);
				break;
			case MMP_UART_RXDMA_TH_EN:
				pUS->US_RXDMA_TOTAL_THR = 0;
				pUS->US_IER &= (~US_RXDMA_TH_EN);
				break;
			case MMP_UART_RXDMA_WRMEM_EN:
				pUS->US_IER &= (~US_RXDMA_WRMEM_EN);
				pUS->US_CR &= (~US_RX_TIMEOUT_EN);
				pUS->US_BRGR &= (~US_RX_TIMEOUT_MASK);
				break;
			case MMP_UART_RXDMA_DROPDATA_EN:
				pUS->US_IER &= (~US_RXDMA_DROPDATA_EN);
				pUS->US_CR &= (~US_RX_TIMEOUT_EN);
				pUS->US_BRGR &= (~US_RX_TIMEOUT_MASK);
				break;
			default:
				return MMP_UART_ERR_PARAMETER;
				break;
		}	
		m_UartDma_CallBack[uart_id] = NULL;
	}
	
	#else 
	RTNA_DBG_Str(0, "No DMA mode: UART_DMA_MODE_EN = 0 !!\r\n");
	#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_EnableDmaMode
//  Description :
//  Note        : This function is the 3rd step of UART DMA settings.
//------------------------------------------------------------------------------
/** @brief This function is used to enable or disable UART DMA mode.

This function is used to enable or disable UART DMA mode.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] uart_dma_mode indicate which DMA mode to be used, please refer the data structure MMP_UART_DMAMODE.
@param[in] bEnable stands for "enable the related mode or not".
@return It reports the status of the operation.
*/
MMP_ERR MMPF_Uart_EnableDmaMode(MMP_UART_ID uart_id, MMP_UART_DMAMODE uart_dma_mode, MMP_BOOL bEnable)
{
	#if (UART_DMA_MODE_EN == 1)
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id != MMP_UART_ID_0) {
        RTNA_DBG_Str(0, "No DMA mode on Uart1,2,3\r\n");
        return MMP_ERR_NONE;
    }

	if (bEnable == MMP_TRUE) 
	{
		switch (uart_dma_mode) {
			case MMP_UART_RXDMA_RING:
				pUS->US_CR |= (US_RXDMA_RING_EN | US_RXDMA_EN) ;
				pUS->US_CR |= US_RXDMA_START_FLAG | (US_RXDMA_RING_EN | US_RXDMA_EN);
				pUS->US_CR |= US_RSTRX | (US_RXDMA_RING_EN | US_RXDMA_EN);  //Add RXDMA_EN again, this is because of 0x6403 can not read issue
				pUS->US_CR |= US_RXEN | (US_RXDMA_RING_EN | US_RXDMA_EN);	//Add RXDMA_EN again, this is because of 0x6403 can not read issue
				break;
    		case MMP_UART_TXDMA:
    			pUS->US_CR |= US_TXDMA_EN;
    			pUS->US_CR |= US_RSTTX | US_TXDMA_EN; //Reset TX, for the TXDMA can not read bug, we add TX_DMA_EN again
    			pUS->US_CR |= US_TXEN | US_TXDMA_EN;  //Enable TX, for the TXDMA can not read bug, we add TX_DMA_EN again
    			break;
    		case MMP_UART_RXDMA:
    			pUS->US_CR |= US_RXDMA_EN;
    			pUS->US_CR |= US_RXDMA_START_FLAG | US_RXDMA_EN;//Add RXDMA_EN again, this is because of 0x6403 can not read issue
    			pUS->US_CR |= US_RSTRX | US_RXDMA_EN;           //Add RXDMA_EN again, this is because of 0x6403 can not read issue
				pUS->US_CR |= US_RXEN | US_RXDMA_EN;	        //Add RXDMA_EN again, this is because of 0x6403 can not read issue
    			break;
    		default:
    			return MMP_UART_ERR_PARAMETER;
    			break;
		}
	}
	else {
		switch (uart_dma_mode) {
			case MMP_UART_RXDMA_RING:
				pUS->US_CR &= (~(US_RXDMA_RING_EN | US_RXDMA_EN | US_RXDMA_START_FLAG));
				break;
    		case MMP_UART_TXDMA:
    			pUS->US_CR &= (~US_TXDMA_EN);
    			break;
    		case MMP_UART_RXDMA:
    			pUS->US_CR &= (~(US_RXDMA_EN | US_RXDMA_START_FLAG));
    			break;
    		default:
    			return MMP_UART_ERR_PARAMETER;
    			break;
		}
	}
	
	#else 
	RTNA_DBG_Str(0, "No DMA mode: UART_DMA_MODE_EN = 0 !!\r\n");
	#endif
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_UART_UseTxDMA
//  Description :
//  Note        : UART tx_dma mode API, the only one API for upper layer.
//------------------------------------------------------------------------------
/** @brief UART tx_dma mode API, the only one API for upper layer.

UART tx_dma mode API, the only one API for upper layer.
@param[in] uart_id indicate which UART device, please refer the data structure of MMP_UART_ID
@param[in] ulTxStartAddr indicate the Tx DMA start address.
@param[in] usTxTotalByte indicate number of bytes would be sent (start from start address).
@param[in] uart_int_mode indicate which DMA interrupt mode to be used, please refer the data structure MMP_UART_DMA_INT_MODE.
@param[in] bEnableInt indicate enable interrup mode or not
@param[in] callBackFunc is the callback function when interrupt occur.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_UART_UseTxDMA(MMP_UART_ID uart_id, MMP_ULONG ulTxStartAddr, MMP_USHORT usTxTotalByte,  
                           MMP_UART_DMA_INT_MODE uart_int_mode, MMP_BOOL bEnableInt, UartCallBackFunc* callBackFunc)
{
	MMP_ERR     status = MMP_ERR_NONE;
	MMP_UBYTE   ubUartCtl;
	
	#if !defined(MINIBOOT_FW)
	MMP_ERR     SemStatus = 0xFF;
	
	SemStatus = MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT);
	
	if (SemStatus == 0) {
	#endif
		status |= MMPF_Uart_SwitchToDmaMode(uart_id, MMP_TRUE, &ubUartCtl);
		status |= MMPF_Uart_SetTxDmaMode(uart_id, MMP_UART_TXDMA, ulTxStartAddr, usTxTotalByte);
		status |= MMPF_Uart_SetDmaInterruptMode(uart_id, uart_int_mode, bEnableInt, callBackFunc, 0, 0); 
	    status |= MMPF_Uart_EnableDmaMode(uart_id, MMP_UART_TXDMA, MMP_TRUE);
	    
	    if(bEnableInt == MMP_FALSE) {
	    	status |= MMPF_Uart_SwitchToDmaMode(uart_id, MMP_FALSE, &ubUartCtl);
	    }
	    
	#if !defined(MINIBOOT_FW)
	    MMPF_OS_ReleaseSem(m_UartSemID[uart_id]);
    }
    else {
    	RTNA_DBG_PrintLong(0, (MMP_ULONG)SemStatus);
    	return MMP_UART_SYSTEM_ERR;
    }
    #endif
    return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_UART_UseRxDMA
//  Description :
//  Note        : UART rx_dma mode API, the only one RX API for upper layer.
//------------------------------------------------------------------------------
/** @brief UART rx_dma mode API, the only one RX API for upper layer.

UART rx_dma mode API, the only one RX API for upper layer.
@param[in] uart_id indicates which UART device, please refer the data structure of MMP_UART_ID
@param[in] bRingModeEn indicates to use ring mode or not.
@param[in] ulRxStartAddr indicate the RX DMA start address.
@param[in] ulRxEndAddr indicate the RX DMA End address.
@param[in] ulRxLowBoundAddr indicate the RX lower bound address (Using by RX DMA Ring Mode). 
@param[in] uart_int_mode indicate which DMA interrupt mode to be used, please refer the data structure MMP_UART_DMA_INT_MODE.
@param[in] bEnableInt indicate enable interrup mode or not
@param[in] callBackFunc is the callback function when interrupt occur.
@param[in] usRxThreshold which is used in ring mode.
@param[in] ubRxTimeOut is used in RX interrupt mode (MMP_UART_RXDMA_WRMEM_EN & MMP_UART_RXDMA_DROPDATA_EN)
@return It reports the status of the operation.
*/
MMP_ERR MMPF_UART_UseRxDMA(MMP_UART_ID uart_id, MMP_BOOL bRingModeEn, MMP_ULONG ulRxStartAddr, MMP_ULONG ulRxEndAddr, MMP_ULONG ulRxLowBoundAddr,  MMP_UART_DMA_INT_MODE uart_int_mode, 
							MMP_BOOL bEnableInt, UartCallBackFunc* callBackFunc, MMP_USHORT usRxThreshold, MMP_UBYTE ubRxTimeOut)
{
	MMP_ERR     status = MMP_ERR_NONE;
	MMP_UART_DMAMODE rx_dma_mode = MMP_UART_RXDMA;
	MMP_UBYTE   ubUartCtl;
	#if !defined(MINIBOOT_FW)
	MMP_ERR     SemStatus = 0x0;
	#endif
	
	if(bRingModeEn) {
		rx_dma_mode = MMP_UART_RXDMA_RING;
	}
	#if !defined(MINIBOOT_FW)
	SemStatus = MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT);
	
	if(SemStatus == 0) {
	#endif
	
		status |= MMPF_Uart_SwitchToDmaMode(uart_id, MMP_TRUE, &ubUartCtl);
		status |= MMPF_Uart_SetRxDmaMode(uart_id, rx_dma_mode, ulRxStartAddr, ulRxEndAddr, ulRxLowBoundAddr);
		status |= MMPF_Uart_SetDmaInterruptMode(uart_id, uart_int_mode, bEnableInt, callBackFunc, usRxThreshold, ubRxTimeOut); 
		status |= MMPF_Uart_EnableDmaMode(uart_id, rx_dma_mode, MMP_TRUE);
		
		if(bEnableInt == MMP_FALSE) {
	    	status |= MMPF_Uart_SwitchToDmaMode(uart_id, MMP_FALSE, &ubUartCtl);
	    }
	#if !defined(MINIBOOT_FW)    
	}
	else {
    	RTNA_DBG_PrintLong(0, (MMP_ULONG)SemStatus);
    	return MMP_UART_SYSTEM_ERR;
    }
    #endif
	
	return status;
}
#endif

#if 0
void ____Uart_General_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_UART_ISR
//  Description :
//  Note        : for uart debug or uart dma interrupt mode, the uart resource protect is done by
//				  MMPF_UART_UseTxDMA and MMPF_UART_UseRxDMA
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)&&(DEBUG_RX_EN == 1)
void MMPF_UART_ISR(void)
{
    MMP_ULONG   intsrc;
    MMP_ULONG   i;
    AITPS_US    pUS;

    for (i = 0; i < MMP_UART_MAX_COUNT; i++) 
    {
        pUS = m_pUS[i];
        intsrc = pUS->US_ISR & pUS->US_IER;
        pUS->US_ISR = intsrc;

        #if (UART_RXINT_MODE_EN == 1)
        MMP_ULONG len;
        if ((m_UartInDmaMode[i] == MMP_FALSE) && (intsrc & US_RX_FIFO_OVER_THRES)) {

            len = pUS->US_FSR & US_RX_FIFO_UNRD_MASK;
                       
            if ((m_UartRx_CallBack[i] != NULL) && len)
                (*m_UartRx_CallBack[i])((MMP_UBYTE)len, &(pUS->US_RXPR));
            break;
        }
        else if (intsrc & US_RX_PARITY_ERR) {
            RTNA_DBG_Str(0, "Error: US_RX_PARITY_ERR\r\n");
            break;
        }
        #endif

        #if (UART_DMA_MODE_EN == 1)
        if (intsrc & US_TXDMA_END_EN) {
            if ((m_UartDma_CallBack[i] != NULL) && (m_UartInDmaMode[i] == MMP_TRUE)) {
                (*m_UartDma_CallBack[i]) (MMP_UART_TXDMA_END_EN);
            }	
        }
        else if (intsrc & US_RXDMA_TH_EN) {
            if ((m_UartDma_CallBack[i] != NULL) && (m_UartInDmaMode[i] == MMP_TRUE)) {
                (*m_UartDma_CallBack[i]) (MMP_UART_RXDMA_TH_EN);
            }
        }
        else if (intsrc & US_RXDMA_WRMEM_EN) {
            if ((m_UartDma_CallBack[i] != NULL) && (m_UartInDmaMode[i] == MMP_TRUE)){
                (*m_UartDma_CallBack[i]) (MMP_UART_RXDMA_WRMEM_EN);
            }
        }
        else if (intsrc & US_RXDMA_DROPDATA_EN) {
            if ((m_UartDma_CallBack[i] != NULL) && (m_UartInDmaMode[i] == MMP_TRUE)){
                (*m_UartDma_CallBack[i]) (MMP_UART_RXDMA_DROPDATA_EN);
            }
        }
        #endif
    }
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_Init
//  Description : Initial the semaphore and call-back functions.
//  Note        :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Uart_Init(void)
{
	static MMP_BOOL bUartInitFlag = MMP_FALSE;
	#if !defined(MINIBOOT_FW)&&(UART_RXINT_MODE_EN)
	AITPS_AIC   pAIC = AITC_BASE_AIC;
    #endif
    AITPS_UARTB pUART = AITC_BASE_UARTB;
	MMP_USHORT 	i = 0;

    if (!bUartInitFlag) 
    {
        #if !defined(MINIBOOT_FW)&&(UART_RXINT_MODE_EN)
        #error "cpub : don't enable it"

        RTNA_AIC_Open(pAIC, AIC_SRC_UART,   uart_isr_a,
	                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
	    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_UART);
	    #endif

		for (i = 0; i < MMP_UART_MAX_COUNT; i++) 
		{
			#if !defined(MINIBOOT_FW)
			m_UartSemID[i]          = MMPF_OS_CreateSem(1);
			#endif
			m_UartInDmaMode[i]      = MMP_FALSE;
			#if (UART_RXINT_MODE_EN == 1)
			m_UartRxEnable[i]       = MMP_FALSE;
			m_UartRx_CallBack[i]    = NULL;
			#endif
			#if (UART_DMA_MODE_EN == 1)
			#if !defined(MINIBOOT_FW)
			m_UartDma_CallBack[i]   = NULL;
			#endif
			#endif
		}

        m_pUS[0] = &pUART->UART_0;
        m_pUS[1] = &pUART->UART_1;
        m_pUS[2] = &pUART->UART_2;
        m_pUS[3] = &pUART->UART_3;

		bUartInitFlag = MMP_TRUE;
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_Open
//  Description :
//  Note        :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Uart_Open(MMP_UART_ID uart_id, MMP_UART_ATTR *uartattribute)
{
	AITPS_GBL   pGBL = AITC_BASE_GBL;
	MMP_ERR     status = MMP_ERR_NONE;
    AITPS_US    pUS;

	status |= MMPF_Uart_Init();
	pUS = m_pUS[uart_id];

	#if !defined(MINIBOOT_FW)
    if (MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT) == 0) 
    {
    #endif

        switch(uart_id) {
        case 0:
            pGBL->GBL_UART_PAD_CFG &= ~(GBL_UART0_PAD_MASK);
            pGBL->GBL_UART_PAD_CFG |= GBL_UART0_PAD(uartattribute->padset);
            break;
        case 1:
            pGBL->GBL_UART_PAD_CFG &= ~(GBL_UART1_PAD_MASK);
            pGBL->GBL_UART_PAD_CFG |= GBL_UART1_PAD(uartattribute->padset);
            break;
        case 2:
            pGBL->GBL_UART_PAD_CFG &= ~(GBL_UART2_PAD_MASK);
            pGBL->GBL_UART_PAD_CFG |= GBL_UART2_PAD(uartattribute->padset);
            break;
        case 3:
            pGBL->GBL_UART_PAD_CFG &= ~(GBL_UART3_PAD_MASK);
            pGBL->GBL_UART_PAD_CFG |= GBL_UART3_PAD(uartattribute->padset);
            break;
        case 4:
            //
            break;
        }

	    // Disable interrupts
	    pUS->US_IER = 0;
	    
	    // Reset receiver and transmitter
	    pUS->US_CR = US_RSTRX | US_RSTTX | US_RXDIS | US_TXDIS;
	    
        //something  failed
        /* #if(USE_DIV_CONST) */
        /* // Define the baud rate divisor register,baud use the constant:115200 */
        /* #if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168) */
        /* pUS->US_BRGR = ((( (168000 >> 1)*1000 << 1) / 115200) + 1) >> 1;		//	CLK_GRP_GBL    168MHz					 */
        /* #endif */

        /* #if (PLL_CONFIG==PLL_FOR_POWER)||(PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192) || (PLL_CONFIG==PLL_FOR_PERFORMANCE) || (PLL_CONFIG==PLL_FOR_BURNING) */
        /* pUS->US_BRGR = ((( (264000 >> 1)*1000 << 1) / 115200) + 1) >> 1;		//	CLK_GRP_GBL    264MHz					 */
        /* #endif */
        /* pUS->US_BRGR = (((uartattribute->ulMasterclk << 1)  */
                                        /* / 1152000) + 1) >> 1;								 */
	    // Define the baud rate divisor register
		pUS->US_BRGR = (((uartattribute->ulMasterclk << 1) 
										/ uartattribute->ulBaudrate) + 1) >> 1;								
	    
	    // Define the UART mode 8-N-1
	    pUS->US_CR = US_ASYNC_MODE | US_TXEN;

		#if (UART_RXINT_MODE_EN == 1)
	    pUS->US_FTHR &= ~US_RX_FIFO_THRES_MASK;
	    pUS->US_FTHR |= US_RX_FIFO_THRES(1);
	    pUS->US_IER = US_RX_FIFO_OVER_THRES;

		#endif
		
		if(uartattribute->bParityEn == MMP_TRUE) 
		{
			pUS->US_CR &= (~US_PAR_DIS);
			pUS->US_CR &= (~US_PAR_MASK);
			
			switch(uartattribute->parity) {
				case MMP_UART_PARITY_EVEN:
					pUS->US_CR |= US_PAR_EVEN;
					break;
				case MMP_UART_PARITY_ODD:
					pUS->US_CR |= US_PAR_ODD;
					break;
				case MMP_UART_PARITY_FORCE0:
					pUS->US_CR |= US_PAR_0;
					break;
				case MMP_UART_PARITY_FORCE1:
					pUS->US_CR |= US_PAR_1;
					break;
				default:
					break;
			} 
		}
		else {
			pUS->US_CR |= US_PAR_DIS;
		}
		
        if (uartattribute->bFlowCtlEn == MMP_TRUE) {
		    if (uartattribute->ubFlowCtlSelect) {
		    	pUS->US_CR |= US_RTSCTS_ACTIVE_H;
		    }
		    else {
		    	pUS->US_CR &= (~US_RTSCTS_ACTIVE_H); 
		    }

			pUS->US_CR |= US_RTSCTS_EN;
		}
		else {
			pUS->US_CR &= (~US_RTSCTS_EN);
		}
	#if !defined(MINIBOOT_FW)	
		MMPF_OS_ReleaseSem(m_UartSemID[uart_id]);
	}
	else {
		return MMP_UART_SYSTEM_ERR;
	}
	#endif
	return	status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_Write
//  Description : Debug output a string
//  Note : uart are often used in CPU ISR mode (ex: for debug), so we comment out the resource protect code.
//------------------------------------------------------------------------------
#if (uart_printc ==0)
MMP_ERR MMPF_Uart_Write(MMP_UART_ID uart_id, const char *pWrite_Str, MMP_ULONG ulLength)
{
    MMP_ULONG   i = 0, txcnt = 0;
    MMP_ERR	    status = MMP_ERR_NONE;
    AITPS_US    pUS = m_pUS[uart_id];
    /* unsigned long ulLength = strlen(pWrite_Str); */

    #if (SUPPORT_MSDC_SCSI_AIT_CMD_MODE) 
    {  
        //extern void MMPF_AIT_DBG_WriteLog(MMP_UBYTE *pbuf, MMP_ULONG buf_len); //CarDV...
        //MMPF_AIT_DBG_WriteLog((MMP_UBYTE *)pWrite_Str, ulLength);
    }
    #endif

    while (ulLength) 
    {
        txcnt = (pUS->US_FSR & US_TX_FIFO_UNWR_MASK) >> 8;

        if (txcnt) {
            if (txcnt > ulLength) {
                txcnt = ulLength;
            }
            for (i = 0; i < txcnt; i++) {
                pUS->US_TXPR = *pWrite_Str++;
            }
            ulLength -= txcnt;
        }
    }

    return	status;
}

#else
MMP_ERR MMPF_Uart_Write(MMP_UART_ID uart_id, const char *pWrite_Str) ITCMFUNC;
MMP_ERR MMPF_Uart_Write(MMP_UART_ID uart_id, const char *pWrite_Str)
{
    MMP_ULONG   txcnt = 0;
    MMP_ERR     status = MMP_ERR_NONE;
    AITPS_US    pUS = m_pUS[uart_id];
    unsigned long ulLength = strlen(pWrite_Str);

    while (ulLength)
    {
        txcnt = (pUS->US_FSR & US_TX_FIFO_UNWR_MASK) >> 8;

        if (txcnt)
        {
            pUS->US_TXPR = *pWrite_Str++;
            ulLength--;
        }
    }

    return  status;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_Close
//  Description :
//  Note        :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Uart_Close(MMP_UART_ID uart_id)
{
    AITPS_US    pUS = m_pUS[uart_id];
	
	#if !defined(MINIBOOT_FW)
	if (MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT) == 0) {
	#endif
	    
	    // Disable interrupts
	    pUS->US_IER = 0;

	    // Reset receiver and transmitter
	    pUS->US_CR = US_RSTRX | US_RSTTX | US_RXDIS | US_TXDIS;
	#if !defined(MINIBOOT_FW)    
	    MMPF_OS_ReleaseSem(m_UartSemID[uart_id]);
    }
    else {
    	return MMP_UART_SYSTEM_ERR;
    }
    #endif

    return	MMP_ERR_NONE;
}

#if (UART_RXINT_MODE_EN == 1)
//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_Close
//  Description : Return if RX is enabled with the specified UART ID
//  Note        :
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)
MMP_BOOL MMPF_Uart_IsRxEnable(MMP_UART_ID uart_id)
{
    if (uart_id < MMP_UART_MAX_COUNT)
        return m_UartRxEnable[uart_id];
    else
        return MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_EnableRx
//  Description : Enable RX with the specified UART ID, set interrupt threshold
//  Note        :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Uart_EnableRx(MMP_UART_ID uart_id, MMP_ULONG threshold, UartCallBackFunc *callBackFunc)
{ 
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id >= MMP_UART_MAX_COUNT)
        return MMP_UART_ERR_PARAMETER;
    else if (threshold > 32)
        return MMP_UART_ERR_PARAMETER;

    if (m_UartRxEnable[uart_id] == MMP_FALSE) 
    {
    	#if !defined(MINIBOOT_FW)
        if (MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT) == 0) {
        #endif
        
            m_UartRx_CallBack[uart_id] = callBackFunc;
            
            // Disable FIFO interrupt first before reset FIFO threshold
            pUS->US_IER &= ~(US_RX_FIFO_OVER_THRES);
            
            pUS->US_FTHR &= ~(US_RX_FIFO_THRES_MASK);
            pUS->US_FTHR |= US_RX_FIFO_THRES(threshold);
            
            // Enable receiver and Rx FIFO interrupt
            pUS->US_CR |= US_RXEN;
            pUS->US_IER |= US_RX_FIFO_OVER_THRES;
        #if !defined(MINIBOOT_FW)    
            MMPF_OS_ReleaseSem(m_UartSemID[uart_id]);
        }
        else {
            return MMP_UART_SYSTEM_ERR;
        }
        #endif

        m_UartRxEnable[uart_id] = MMP_TRUE;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Uart_DisableRx
//  Description : Disable RX with the specified UART ID
//  Note        :
//------------------------------------------------------------------------------
MMP_ERR MMPF_Uart_DisableRx(MMP_UART_ID uart_id)
{
    AITPS_US    pUS = m_pUS[uart_id];

    if (uart_id >= MMP_UART_MAX_COUNT)
        return MMP_UART_ERR_PARAMETER;

    if (m_UartRxEnable[uart_id]) 
    {
    	#if !defined(MINIBOOT_FW)
        if (MMPF_OS_AcquireSem(m_UartSemID[uart_id], UART_RXINT_SEM_TIMEOUT) == 0) {
        #endif
            pUS->US_IER &= ~(US_RX_FIFO_OVER_THRES);
            pUS->US_CR &= ~(US_RXEN);
            
            m_UartRx_CallBack[uart_id] = NULL;
        #if !defined(MINIBOOT_FW)    
            MMPF_OS_ReleaseSem(m_UartSemID[uart_id]);
        }
        else {
            return MMP_UART_SYSTEM_ERR;
        }
        #endif
        
        // Reset receiver
	    pUS->US_CR |= US_RSTRX;

        m_UartRxEnable[uart_id] = MMP_FALSE;

        pUS->US_CR &= ~(US_RSTRX);
    }

    return MMP_ERR_NONE;
}
#endif // !defined(MINIBOOT_FW)
#endif
