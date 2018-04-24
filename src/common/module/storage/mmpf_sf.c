//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmpf_sf.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_pad.h"
#include "mmp_reg_sif.h"
#include "mmpf_pll.h"
#include "mmpf_system.h"

//==============================================================================
//
//                              CONSTANT
//
//==============================================================================

#define SIF_TIMEOUT_EN              (0)
#define SIF_TIMEOUT_MS              (1000)      //unit in ms
#define SIF_WTRIE_TIMEOUT           (1000000)   //unit in us
#define SIF_ERASE_SECTOR_TIMEOUT    (2000)      //unit in ms
#define SIF_ERASE_BLOCK_TIMEOUT     (5000)      //unit in ms
#define SIF_ERASE_CHIP_TIMEOUT      (12000)     //unit in ms
#if defined(MINIBOOT_FW)
#define OS_EN                       (0)
#else 
#define OS_EN                       (1)
#endif

//==============================================================================
//
//                              Global VARIABLES
//
//==============================================================================

static MMP_ULONG        m_ulSFIdBuf;
static MMP_ULONG        m_ulSFDmaAddr;
static MMPF_SIF_INFO    m_SFInfo;
static MMP_ULONG        m_ulSFDevNum = 0;   //number of registered SF modules

#if (USING_SF_IF)
SPINorProfile           *SPINORindex[SPINORFLASHMAXNUM];  
#endif
SPINorProfile           *m_SFDev;

//==============================================================================
//
//                              EXTERNAL FUNCTIONS
//
//==============================================================================

extern void MMPF_MMU_FlushDCacheMVA(MMP_ULONG ulRegion, MMP_ULONG ulSize);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//Register the module parameter information in the structure pointer array SPINORindex[]
MMP_ERR MMPF_SF_Register(MMP_UBYTE ubSFIndex, SPINorProfile *pArrayAdd) ITCMFUNC;
MMP_ERR MMPF_SF_Register(MMP_UBYTE ubSFIndex, SPINorProfile *pArrayAdd) 
{
    SPINORindex[ubSFIndex] = pArrayAdd;
	m_ulSFDevNum++;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_CheckReady(void) ITCMFUNC;
MMP_ERR MMPF_SF_CheckReady(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ULONG   ulUScount = 0;

	while(1) {
		if (pSIF->SIF_INT_CPU_SR & SIF_CMD_DONE) {
			break;
        }
		else {
			MMPF_SYS_WaitUs(1);
			ulUScount++;
		    if ((ulUScount/1000) > SIF_TIMEOUT_MS) {
				MMP_PRINT_RET_ERROR(SIF_DBG_LEVEL, 0, "SF CheckReady Timeout\r\n");
				return MMP_SIF_ERR_TIMEOUT;
		    }
		}
	}
    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ReadStatus(MMP_UBYTE SRnum, MMP_UBYTE *ubStatus) ITCMFUNC;
MMP_ERR MMPF_SF_ReadStatus(MMP_UBYTE SRnum, MMP_UBYTE *ubStatus)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;

    if (SRnum == 1)
        pSIF->SIF_CMD = READ_STATUS1;
    else if (SRnum == 2)
        pSIF->SIF_CMD = READ_STATUS2;
    else
        pSIF->SIF_CMD = READ_STATUS3;

    pSIF->SIF_CTL = SIF_START | SIF_R | SIF_DATA_EN;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    *ubStatus = pSIF->SIF_DATA_RD;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_WriteStatus(MMP_UBYTE SRnum, MMP_UBYTE ubData) ITCMFUNC;
MMP_ERR MMPF_SF_WriteStatus(MMP_UBYTE SRnum, MMP_UBYTE ubData)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubStatus;
	MMP_ERR     err = 0;
    #if (SIF_TIMEOUT_EN)
    MMP_ULONG   ulUScount = 0;
    #endif

    err = MMPF_SF_EnableWriteSR(); //instead of MMPF_SF_EnableWrite API      
	ERRCHECK_RETURN(err);

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;

    if (SRnum == 1)
        pSIF->SIF_CMD = WRITE_STATUS1;
    else if (SRnum == 2)
        pSIF->SIF_CMD = WRITE_STATUS2;
    else
        pSIF->SIF_CMD = WRITE_STATUS3;

    pSIF->SIF_DATA_WR = ubData;
    pSIF->SIF_CTL = SIF_START | SIF_W | SIF_DATA_EN;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    #if (SIF_TIMEOUT_EN)
    do {
        MMPF_SYS_WaitUs(1, NULL, NULL);
        ulUScount++;

        if (ulUScount >= SIF_WTRIE_TIMEOUT) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #else
    do {
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #endif

    return MMP_ERR_NONE;
}

//every time you want to write, you must enable write again

MMP_ERR MMPF_SF_EnableWrite(void) ITCMFUNC;
MMP_ERR MMPF_SF_EnableWrite(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;

	pSIF->SIF_CMD = WRITE_ENABLE;
	
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_EnableWriteSR(void) ITCMFUNC;
MMP_ERR MMPF_SF_EnableWriteSR(void) //CMD<50H> is only used for Enable Write to "Status Register"
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
	
	if (m_SFDev->EnableVolatileWrite)  
	    pSIF->SIF_CMD = WRITE_ENABLEFORVOLATILE;
	else    
	    pSIF->SIF_CMD = WRITE_ENABLE;
	    
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_DisableWrite(void) ITCMFUNC;
MMP_ERR MMPF_SF_DisableWrite(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     bERR = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = WRITE_DISABLE;
    pSIF->SIF_CTL = SIF_START;

    bERR = MMPF_SF_CheckReady();
 	if (bERR)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_EnableResetCommand(void) ITCMFUNC;
MMP_ERR MMPF_SF_EnableResetCommand(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     bERR = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = ENABLERESETCOMMAND; 
    pSIF->SIF_CTL = SIF_START;

    bERR = MMPF_SF_CheckReady();
 	if (bERR)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ResetCommand(void) ITCMFUNC;
MMP_ERR MMPF_SF_ResetCommand(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     bERR = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = RESETCOMMAND; 
    pSIF->SIF_CTL = SIF_START;

    bERR = MMPF_SF_CheckReady();
 	if (bERR)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ClearCommand(void) ITCMFUNC;
MMP_ERR MMPF_SF_ClearCommand(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     bERR = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = CLEARSR;   
    pSIF->SIF_CTL = SIF_START;

    bERR = MMPF_SF_CheckReady();
 	if (bERR)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ENEX4B(MMP_UBYTE ENEXStatus) ITCMFUNC;
MMP_ERR MMPF_SF_ENEX4B(MMP_UBYTE ENEXStatus)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     bERR = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
	if (ENEXStatus)
    	pSIF->SIF_CMD = EN4B; //Enter 4B,EN4B
	else 
		pSIF->SIF_CMD = EX4B; //Exit 4B,EX4B
    pSIF->SIF_CTL = SIF_START;

    bERR = MMPF_SF_CheckReady();
 	if (bERR)
		return MMP_SIF_ERR_TIMEOUT;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_SetIdBufAddr(MMP_ULONG ulAddr) ITCMFUNC;
MMP_ERR MMPF_SF_SetIdBufAddr(MMP_ULONG ulAddr)
{
    /* buffer for device ID info */
    m_ulSFIdBuf = ulAddr;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_SetTmpAddr(MMP_ULONG ulStartAddr) ITCMFUNC;
MMP_ERR MMPF_SF_SetTmpAddr(MMP_ULONG ulStartAddr)
{
    /* buffer for file system */
    m_ulSFDmaAddr = ulStartAddr;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_GetTmpAddr(MMP_ULONG *ulStartAddr) ITCMFUNC;
MMP_ERR MMPF_SF_GetTmpAddr(MMP_ULONG *ulStartAddr)
{
    *ulStartAddr = m_ulSFDmaAddr;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_InitialInterface(MMPF_SIF_PAD PadId) ITCMFUNC;
MMP_ERR MMPF_SF_InitialInterface(MMPF_SIF_PAD PadId)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    AITPS_GBL   pGBL = AITC_BASE_GBL;
    AITPS_PAD   pPAD = AITC_BASE_PAD;
    MMP_ULONG   ulG0Freq, ulSifClkDiv;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_BS_SPI, MMP_TRUE);
    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulG0Freq);

    /* #if(USE_DIV_CONST) */
    /* MMP_UBYTE id = pGBL->GBL_CHIP_VER; */
    /* #if (DRAM_ID == DRAM_DDR) */
    /* ulG0Freq = 200000; //400/2 */

    /* if ((id & 0x03) == 0x00)  */
    /* { */
        /* ulG0Freq=180000; //360/2; */
    /* } */
    /* #endif */

    /* #if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2) */
    /* ulG0Freq = 400000; // CLK_GRP_DRAM   400MHz */
    /* #endif */
    /* #endif */

    #if (CHIP == MCR_V2) // pad enable
    pGBL->GBL_GPIO_CFG[0] &= ~(0xFC00); // disable PBGPIO0~5 GPIO mode
    pGBL->GBL_MISC_IO_CFG |= GBL_SIF_PAD_EN;
    #endif

    // TM suggest to decrease the driving strength of SIF clock pad
    pPAD->PAD_IO_CFG_PBGPIO[0] &= ~(PAD_OUT_DRIVING(3));
    pPAD->PAD_IO_CFG_PBGPIO[0] |= PAD_OUT_DRIVING(1);

    // And also, latch data in failing edge can have better setup time
    pSIF->SIF_DATA_IN_LATCH = SIF_LATCH_BY_FALLING;

    //SIF max freq is 33 Mhz
    //Target clock rate = source clock rate / 2(SIF_CLK_DIV + 1)
    ulSifClkDiv = (ulG0Freq+32999)/33000;

    if (ulSifClkDiv & 0x01)
        ulSifClkDiv++;

    pSIF->SIF_CLK_DIV = SIF_CLK_DIV(ulSifClkDiv);

    return  MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_Reset(void) ITCMFUNC;
MMP_ERR MMPF_SF_Reset(void) //This is New MMPF_SF_Reset
{   
    MMP_UBYTE       ubSRData;
	MMP_ULONG       i;
	MMP_ULONG       ulG0Freq, ulSifClkDiv;
	AITPS_SIF       pSIF = AITC_BASE_SIF;
	MMP_ERR         err = 0;


	err = MMPF_SF_ReadDevId(&m_SFInfo.ulDevId);
	ERRCHECK_RETURN(err);
	#if !defined(MBOOT_FW)
    RTNA_DBG_PrintLong(0, m_SFInfo.ulDevId);
    #endif

	if ((m_SFInfo.ulDevId == 0) || (m_SFInfo.ulDevId == 0xFFFFFF))
        return MMP_USER_ERR_INIT;

    // Find the SPI flash profile which has the identical ID
    m_SFDev = NULL;
	for (i = 1; i < m_ulSFDevNum; i++) {
		if (SPINORindex[i]->DeviceID == m_SFInfo.ulDevId) {
		    m_SFDev = SPINORindex[i];
			break;
		}
	}

    if (m_SFDev == NULL) {
        #if !defined(MBOOT_FW)
        RTNA_DBG_Str(0, "Unknow SPI Flash\r\n");
        #endif
        m_SFDev = SPINORindex[0]; //use deafult setting
    }

	//Check SIFCLKDividor
	if ((m_SFDev->SIFCLKDividor) != 33000) {
	    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulG0Freq);
	    //SIF max freq is 33 Mhz
        //Target clock rate = source clock rate / 2(SIF_CLK_DIV + 1)
        //some bug need fixed

        /* #if(USE_DIV_CONST) */
        /* AITPS_GBL   pGBL = AITC_BASE_GBL; */
        /* MMP_UBYTE id = pGBL->GBL_CHIP_VER; */
        /* #if (DRAM_ID == DRAM_DDR) */
        /* ulG0Freq = 200000; //400/2 */

        /* if ((id & 0x03) == 0x00)  */
        /* { */
        /* ulG0Freq=180000; //360/2; */
        /* } */
        /* #endif */

        /* #if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2) */
        /* ulG0Freq = 400000; // CLK_GRP_DRAM   400MHz */
        /* #endif */

        /* ulSifClkDiv = (ulG0Freq + 66000 - 1)/66000; */
        /* #else */
        // SIF freqn only set be 33 MHz or 66 Mhz, replace for dividing to constant */
        ulSifClkDiv = (ulG0Freq + m_SFDev->SIFCLKDividor - 1)/
                      (m_SFDev->SIFCLKDividor);
	    if (ulSifClkDiv & 0x01)
            ulSifClkDiv++;

	    pSIF->SIF_CLK_DIV = SIF_CLK_DIV(ulSifClkDiv);
    }
    //Initial Parameter for m_SFInfo
    m_SFInfo.ulSFSectorSize = m_SFDev->SectorSIZE;        
    m_SFInfo.ulSFBlockSize  = m_SFDev->BlockSIZE;
    m_SFInfo.ulSFTotalSize  = m_SFDev->TotalSize; 

    //if module is match,Set 4ByteAddress and Quad SPI mode for initialization
    if (m_SFDev) { 
        if (m_SFDev->FourByteAddress) { //Enable 4 byte address mode
			if (m_SFDev->ADPMode) { //Execute ADP setting 
			    err = MMPF_SF_ReadStatus(m_SFDev->SR4ByteAdd , &ubSRData);
				ERRCHECK_RETURN(err);
				err = MMPF_SF_WriteStatus(m_SFDev->SR4ByteAdd , (ubSRData | (m_SFDev->SRBit4ByteAdd)));
				ERRCHECK_RETURN(err);
			}				
			err = MMPF_SF_ENEX4B(1); 
			ERRCHECK_RETURN(err);
        }    
        else { //Disable 4 byte address mode 
			if (m_SFDev->ADPMode) { //Execute ADP setting  
			    err = MMPF_SF_ReadStatus(m_SFDev->SR4ByteAdd , &ubSRData);
				ERRCHECK_RETURN(err);
				err = MMPF_SF_WriteStatus(m_SFDev->SR4ByteAdd , (ubSRData & (~(m_SFDev->SRBit4ByteAdd))));
				ERRCHECK_RETURN(err);
			}
			//To avoid the Module under <32MB/256Mb> do the Disable ADP process
			if ((m_SFDev->TotalSize) >= 0x2000000) {
			    err = MMPF_SF_ENEX4B(0); 	
				ERRCHECK_RETURN(err);
			}
		}
    }

    #if defined(ALL_FW)
    RTNA_DBG_Str(0, "SPI-NOR-Flash Reset OK\r\n");
    #endif

    err = MMPF_SF_WriteStatus(1,0);
	ERRCHECK_RETURN(err);
	if (m_SFDev->SupportClearSR) {
		err = MMPF_SF_ClearCommand();
		ERRCHECK_RETURN(err);
	}

    return MMP_ERR_NONE;
}

MMPF_SIF_INFO *MMPF_SF_GetSFInfo(void) ITCMFUNC;
MMPF_SIF_INFO *MMPF_SF_GetSFInfo(void)
{
    return &m_SFInfo;
}

MMP_ERR MMPF_SF_ReadDevId(MMP_ULONG *id) ITCMFUNC;
MMP_ERR MMPF_SF_ReadDevId(MMP_ULONG *id)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;
    MMP_ULONG   *ulIdBuf = (MMP_ULONG *)m_ulSFIdBuf;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = READ_DEVICE_ID;
    pSIF->SIF_DMA_ST = (MMP_ULONG)m_ulSFIdBuf;

    if (pSIF->SIF_DMA_ST == 0) {
        RTNA_DBG_Str(0, "SF Dma Addr is 0\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }
    pSIF->SIF_DMA_CNT = 3 - 1; // real read 3 byte

    *ulIdBuf = 0;

    pSIF->SIF_CTL = (SIF_START | SIF_DATA_EN | SIF_R | SIF_DMA_EN);
    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    *id =  ((*ulIdBuf<<16) & 0x00FF0000) +
            (*ulIdBuf & 0x0000FF00) +
            ((*ulIdBuf>>16) & 0x000000FF);

    pSIF->SIF_DMA_ST = 0;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ReadData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount) ITCMFUNC;
MMP_ERR MMPF_SF_ReadData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount)
{
    AITPS_SIF pSIF = AITC_BASE_SIF;
    MMP_UBYTE ubAddrLenBit;
	MMP_ERR err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    if (m_SFDev->FourByteAddress) {
        pSIF->SIF_CMD = (m_SFDev->ReadDataCMD4Byte);        
    }    
    else {
        pSIF->SIF_CMD = (m_SFDev->ReadDataCMD3Byte);
    }
    pSIF->SIF_FLASH_ADDR = ulSFAddr;
    pSIF->SIF_DMA_ST = ulDmaAddr;

    if (pSIF->SIF_DMA_ST == 0) {
        RTNA_DBG_Str(0, "SF Dma Addr is 0\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }

    pSIF->SIF_DMA_CNT = ulByteCount - 1;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = (SIF_START | SIF_DATA_EN | SIF_R |
                    SIF_DMA_EN | SIF_ADDR_EN | ubAddrLenBit);

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    pSIF->SIF_FLASH_ADDR = 0;
    pSIF->SIF_DMA_ST = 0;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_FastReadData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount) ITCMFUNC;
MMP_ERR MMPF_SF_FastReadData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount)
{
    AITPS_SIF pSIF = AITC_BASE_SIF;
    MMP_UBYTE ubAddrLenBit;
	MMP_ERR err = 0;
    MMP_UBYTE ubStatus = 0; 

    if (m_SFDev->ReadQuadSPI) { //setting to enter Quad SPI
        MMPF_SF_ReadStatus(m_SFDev->SRQuadEnable, &ubStatus);
    	MMPF_SF_WriteStatus(m_SFDev->SRQuadEnable, (ubStatus | (m_SFDev->SRBitQuadEnable)));
    }  

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;

	if (m_SFDev->ReadQuadSPI) {
	    pSIF->SIF_CTL2 |= SIF_QUAD_MODE;
	    
	    if (m_SFDev->FourByteAddress)
	        pSIF->SIF_CMD = (m_SFDev->QuadReadDataCMD4Byte);
	    else
	        pSIF->SIF_CMD = (m_SFDev->QuadReadDataCMD3Byte);
	}
	else if (m_SFDev->ReadDualSPI) {
	    pSIF->SIF_CTL2 |= SIF_DUAL_MODE;
	    
	    if (m_SFDev->FourByteAddress)
	        pSIF->SIF_CMD = (m_SFDev->DualReadDataCMD4Byte);
	    else
	        pSIF->SIF_CMD = (m_SFDev->DualReadDataCMD3Byte);
	}
    else {
        if (m_SFDev->FourByteAddress)
	        pSIF->SIF_CMD = (m_SFDev->FastReadDataCMD4Byte);
	    else
	        pSIF->SIF_CMD = (m_SFDev->FastReadDataCMD3Byte);	    
    }

    pSIF->SIF_FLASH_ADDR = ulSFAddr;
    pSIF->SIF_DMA_ST = ulDmaAddr;
    if (pSIF->SIF_DMA_ST == 0) {
        RTNA_DBG_Str(0, "SF Dma Addr is 0\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }
    pSIF->SIF_DMA_CNT = ulByteCount - 1;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = (SIF_START | SIF_FAST_READ | SIF_DATA_EN | SIF_R |
                    SIF_DMA_EN | SIF_ADDR_EN | ubAddrLenBit);

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    pSIF->SIF_FLASH_ADDR = 0;
    pSIF->SIF_DMA_ST = 0;

	if (m_SFDev->ReadQuadSPI) {
	    pSIF->SIF_CTL2 &= ~(SIF_QUAD_MODE);
	    MMPF_SF_WriteStatus(m_SFDev->SRQuadEnable, (ubStatus & ~(m_SFDev->SRBitQuadEnable)));
	}
	else if (m_SFDev->ReadDualSPI) {
    	pSIF->SIF_CTL2 &= (~SIF_DUAL_MODE);
	}

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_WriteData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount) ITCMFUNC;
MMP_ERR MMPF_SF_WriteData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubStatus;     
	MMP_ERR     err = 0;
    MMP_UBYTE   ubStatus2;
    #if (SIF_TIMEOUT_EN)
    MMP_ULONG   ulUScount = 0;
    MMP_BOOL    ubCmdTimeOut = MMP_FALSE;
    #endif
    MMP_UBYTE   ubAddrLenBit;   

    if (m_SFDev->WriteQuadSPI){ //setting to enter Quad SPI
        MMPF_SF_ReadStatus(m_SFDev->SRQuadEnable, &ubStatus2);
    	MMPF_SF_WriteStatus(m_SFDev->SRQuadEnable, (ubStatus2 | (m_SFDev->SRBitQuadEnable)));    
	}
    
    err = MMPF_SF_EnableWrite();
	ERRCHECK_RETURN(err);
    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;

	if (m_SFDev->WriteQuadSPI) {
    	pSIF->SIF_CTL2 |= SIF_QUAD_MODE;

        if (m_SFDev->FourByteAddress)
	        pSIF->SIF_CMD = (m_SFDev->QuadWriteDataCMD4Byte);
	    else
	        pSIF->SIF_CMD = (m_SFDev->QuadWriteDataCMD3Byte);
	}
	else {
	    if (m_SFDev->FourByteAddress)
	        pSIF->SIF_CMD = (m_SFDev->WriteDataCMD4Byte);
	    else
	        pSIF->SIF_CMD = (m_SFDev->WriteDataCMD3Byte);
    }

    pSIF->SIF_FLASH_ADDR = ulSFAddr;
    pSIF->SIF_DMA_ST = ulDmaAddr;
    if (pSIF->SIF_DMA_ST == 0) {
        RTNA_DBG_Str(0, "SF Dma Addr is 0\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }
    pSIF->SIF_DMA_CNT = ulByteCount - 1;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = (SIF_START | SIF_DATA_EN | SIF_W |
                    SIF_DMA_EN | SIF_ADDR_EN | ubAddrLenBit);
    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    #if (SIF_TIMEOUT_EN)
    do {
        MMPF_SYS_WaitUs(1, NULL, NULL);
        ulUScount++;
        if (ulUScount >= SIF_WTRIE_TIMEOUT) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            ubCmdTimeOut = MMP_TRUE;
            break;
        }
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #else
    do {
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #endif

    pSIF->SIF_FLASH_ADDR = 0;
    pSIF->SIF_DMA_ST = 0;

	if (m_SFDev->WriteQuadSPI){
    	pSIF->SIF_CTL2 &= ~(SIF_QUAD_MODE);
    	MMPF_SF_WriteStatus(m_SFDev->SRQuadEnable, (ubStatus2 & ~(m_SFDev->SRBitQuadEnable)));
	}
    
	#if (SIF_TIMEOUT_EN)
    if (ubCmdTimeOut)
    	return MMP_SYSTEM_ERR_CMDTIMEOUT;
    else
    	return MMP_ERR_NONE;
    #else
    return MMP_ERR_NONE;
    #endif
}

MMP_ERR MMPF_SF_EraseSector(MMP_ULONG ulSFAddr) ITCMFUNC;
MMP_ERR MMPF_SF_EraseSector(MMP_ULONG ulSFAddr)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubStatus;
    #if (SIF_TIMEOUT_EN)
    MMP_ULONG   ulStartTime, ulEndTime, ulUScount = 0;
    #endif
    MMP_UBYTE   ubAddrLenBit;
	MMP_ERR     err = 0;

    err = MMPF_SF_EnableWrite();
	ERRCHECK_RETURN(err);

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
	if (m_SFDev->FourByteAddress) 
		pSIF->SIF_CMD = (m_SFDev->EraseSectorCMD4Byte);	
	else 
		pSIF->SIF_CMD = (m_SFDev->EraseSectorCMD3Byte);

    pSIF->SIF_FLASH_ADDR = ulSFAddr;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = (SIF_START | SIF_R | SIF_ADDR_EN | ubAddrLenBit);
    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    #if (SIF_TIMEOUT_EN)
    #if (OS_EN == 1)
    MMPF_OS_GetTime(&ulStartTime);
    #endif
    do {
    	#if (OS_EN == 1)
        MMPF_OS_GetTime(&ulEndTime);
        if ((ulEndTime - ulStartTime) >= SIF_ERASE_SECTOR_TIMEOUT){
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }else{
            MMPF_OS_Sleep(1);
        }
        #else
        MMPF_SYS_WaitUs(1, NULL, NULL);
        ulUScount++;
        if (ulUScount >= SIF_ERASE_SECTOR_TIMEOUT*1000) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }
        #endif
        
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #else
    do {
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #endif

    pSIF->SIF_FLASH_ADDR = 0;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_EraseBlock(MMP_ULONG ulSFAddr) ITCMFUNC;
MMP_ERR MMPF_SF_EraseBlock(MMP_ULONG ulSFAddr)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubStatus;
    #if (SIF_TIMEOUT_EN)
    MMP_ULONG   ulStartTime, ulEndTime, ulUScount = 0;
    #endif
    MMP_UBYTE   ubAddrLenBit;
	MMP_ERR     err = 0;

    err = MMPF_SF_EnableWrite();
	ERRCHECK_RETURN(err);

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = BLOCK_ERASE;
    pSIF->SIF_FLASH_ADDR = ulSFAddr;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = (SIF_START | SIF_R | SIF_ADDR_EN | ubAddrLenBit);
    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    #if (SIF_TIMEOUT_EN)
    #if (OS_EN == 1)
    MMPF_OS_GetTime(&ulStartTime);
    #endif
    do {
    	#if (OS_EN == 1)
        MMPF_OS_GetTime(&ulEndTime);
        if ((ulEndTime - ulStartTime) >= SIF_ERASE_BLOCK_TIMEOUT) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }
        else {
            MMPF_OS_Sleep(1);
        }
        #else
        MMPF_SYS_WaitUs(1, NULL, NULL);
        ulUScount++;
        if (ulUScount >= SIF_ERASE_BLOCK_TIMEOUT*1000) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }
        #endif
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #else
    do {
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #endif

    pSIF->SIF_FLASH_ADDR = 0;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_EraseChip() ITCMFUNC;
MMP_ERR MMPF_SF_EraseChip()
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubStatus;
    #if (SIF_TIMEOUT_EN)
    MMP_ULONG   ulStartTime, ulEndTime, ulUScount = 0;
    #endif
	MMP_ERR     err = 0;

    err = MMPF_SF_EnableWrite();
	ERRCHECK_RETURN(err);
    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = CHIP_ERASE;
    pSIF->SIF_CTL = SIF_START;
    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    #if (SIF_TIMEOUT_EN)
    #if (OS_EN == 1)
    MMPF_OS_GetTime(&ulStartTime);
    #endif
    do {
    	#if (OS_EN == 1)
        MMPF_OS_GetTime(&ulEndTime);
        if ((ulEndTime - ulStartTime) >= SIF_ERASE_CHIP_TIMEOUT){
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }else{
            MMPF_OS_Sleep(1);
        }
        #else
        MMPF_SYS_WaitUs(1, NULL, NULL);
        ulUScount++;
        if (ulUScount >= SIF_ERASE_CHIP_TIMEOUT*1000) {
            RTNA_DBG_Str(0, "Serial flash time out\r\n");
            break;
        }
        #endif
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #else
    do {
        err = MMPF_SF_ReadStatus(1, &ubStatus);
		ERRCHECK_RETURN(err);
    } while (ubStatus & WRITE_IN_PROGRESS);
    #endif

    return MMP_ERR_NONE;
}

//AAI Write CMD must write in word
MMP_ERR MMPF_SF_AaiWriteData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount) ITCMFUNC;
MMP_ERR MMPF_SF_AaiWriteData(MMP_ULONG ulSFAddr, MMP_ULONG ulDmaAddr, MMP_ULONG ulByteCount)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
    MMP_UBYTE   ubAddrLenBit;
	MMP_ULONG   ulUScount = 0;
	MMP_ERR     err = 0;

    err = MMPF_SF_EnableWrite();
	ERRCHECK_RETURN(err);
    err = MMPF_SF_EBSY();
	ERRCHECK_RETURN(err); 
    pSIF->SIF_INT_CPU_SR = SIF_CLR_AAI_CMD_STATUS|SIF_CLR_CMD_STATUS;

    pSIF->SIF_CMD = ADDR_AUTO_INC_WRITE;
    pSIF->SIF_FLASH_ADDR = ulSFAddr;
    pSIF->SIF_DMA_CNT = ulByteCount - 1;
    pSIF->SIF_DMA_ST = ulDmaAddr;
    if ( (pSIF->SIF_DMA_ST) == 0 ){
        RTNA_DBG_Str(0, "Error :: SIF_DMA_ST = 0, Dma Address is abnormal\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }
    pSIF->SIF_CTL2 |= SIF_AAI_MODE_EN;

    ubAddrLenBit = SIF_ADDR_LEN_3;
    #if (CHIP == MCR_V2)
    if (m_SFDev->FourByteAddress)
        ubAddrLenBit |= SIF_ADDR_LEN_2;
    #endif
    pSIF->SIF_CTL = SIF_START | SIF_DMA_EN | SIF_DATA_EN |
                    SIF_ADDR_EN | ubAddrLenBit;

	while(1) {
		if (pSIF->SIF_INT_CPU_SR & SIF_AAI_CMD_DONE)
			break;
		else {
			MMPF_SYS_WaitUs(1);
			ulUScount++;
		    if ((ulUScount/1000) > SIF_TIMEOUT_MS) {
				MMP_PRINT_RET_ERROR(SIF_DBG_LEVEL, 0, "SF AaiWriteData Timeout\r\n");
				return MMP_SIF_ERR_TIMEOUT;
		    }
		}	
	}	

    err = MMPF_SF_DBSY();
	ERRCHECK_RETURN(err);
    err = MMPF_SF_DisableWrite();
	ERRCHECK_RETURN(err);
    pSIF->SIF_FLASH_ADDR = 0;
    pSIF->SIF_DMA_ST = 0;

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_EBSY(void) ITCMFUNC;
MMP_ERR MMPF_SF_EBSY(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = EBSY;
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    return  MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_DBSY(void) ITCMFUNC;
MMP_ERR MMPF_SF_DBSY(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = DBSY;
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    return  MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_ReleasePowerDown(void) ITCMFUNC;
MMP_ERR MMPF_SF_ReleasePowerDown(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = RELEASE_DEEP_POWER_DOWN;
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    return  MMP_ERR_NONE;
}

MMP_ERR MMPF_SF_PowerDown(void) ITCMFUNC;
MMP_ERR MMPF_SF_PowerDown(void)
{
    AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_ERR     err = 0;

    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = DEEP_POWER_DOWN;
    pSIF->SIF_CTL = SIF_START;

    err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

    MMPF_SYS_EnableClock(MMPF_SYS_CLK_BS_SPI, MMP_FALSE);

    return  MMP_ERR_NONE;
}

#if (CHIP == MCR_V2)
MMP_ERR MMPF_SF_EnableCrcCheck(MMP_BOOL bEnable) ITCMFUNC;
MMP_ERR MMPF_SF_EnableCrcCheck(MMP_BOOL bEnable)
{
    AITPS_SIF pSIF = AITC_BASE_SIF;

    pSIF->SIF_BIST_EN |= SIF_CRC_AUTO_CLEAN_EN;
    if (bEnable)
        pSIF->SIF_BIST_EN |= SIF_BIST_ENABLE;
    else
        pSIF->SIF_BIST_EN &= ~(SIF_BIST_ENABLE);

    return MMP_ERR_NONE;
}

MMP_USHORT MMPF_SF_GetCrc(void) ITCMFUNC;
MMP_USHORT MMPF_SF_GetCrc(void)
{
    AITPS_SIF pSIF = AITC_BASE_SIF;

    return pSIF->SIF_CRC_OUT;
}
#endif

MMP_ERR MMPF_SF_ReadUniqueId(MMP_ULONG64 *ulDmaAddr) ITCMFUNC;
MMP_ERR MMPF_SF_ReadUniqueId(MMP_ULONG64 *ulDmaAddr)
{
#if defined(ALL_FW)
	AITPS_SIF   pSIF = AITC_BASE_SIF;
	MMP_UBYTE   uniqueIdData[12];  // 4 dummy + 64bits data
	MMP_ERR     err = 0;

	pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
	pSIF->SIF_CMD = READ_UNIQUE_ID;
	pSIF->SIF_DMA_ST = (MMP_ULONG) &uniqueIdData;

	if (pSIF->SIF_DMA_ST == 0) {
        RTNA_DBG_Str(0, "SF Dma Addr is 0\r\n");
        return MMP_SIF_ERR_DMAADDRESS;
    }
	pSIF->SIF_DMA_CNT = 12 - 1; // 4 dummy + 64bits data = 12
	pSIF->SIF_CTL = (SIF_START | SIF_DATA_EN | SIF_R | SIF_DMA_EN);
	err = MMPF_SF_CheckReady();
 	if (err)
		return MMP_SIF_ERR_TIMEOUT;

	pSIF->SIF_FLASH_ADDR = 0;
	pSIF->SIF_DMA_ST = 0;

	if (ulDmaAddr) {
		MMP_UBYTE index = 0;

		*ulDmaAddr = 0;
		for (index = 0; index < 8; index++)
			*ulDmaAddr |= ((MMP_ULONG64)uniqueIdData[4 + index]) << (56 - index * 8);
	}
#endif
	return MMP_ERR_NONE;
}
