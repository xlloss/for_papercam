#include "config_fw.h"
#if (CHIP == MCR_V2)
#include "includes_fw.h"
#include "lib_retina.h"
#include "mmpf_sfnand.h"
#include "lib_retina.h"
#include "mmp_reg_gpio.h"
#include "mmp_reg_sif.h"
#include "mmpf_system.h"

#if (USING_SN_IF)

/** @addtogroup MMPF_SFNAND
@{
*/
#pragma O0
//==============================================================================
//
//                              Define
//
//==============================================================================
#define _LOGADDRESS         1
#define _PHYADDRESS         0

#define FULL_ZONE           1
#define TWO_ZONE            0
#define ONE_ZONE            0

#pragma O2
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
MMPF_OS_SEMID sfnandBusySemID = 0;
MMP_ULONG   SFSMC_MAP_DATA_BUF;         // size = 2048 bytes
MMP_ULONG   SFSMC_BLK_ADDR_BUF;         // size = 2048 bytes
MMP_ULONG   SFNAND_IO_DATA_ADDR;        // size = 8 bytes
MMP_ULONG   SF_A8_RA_BUF_ADDR;           // size = 64 bytes
MMP_ULONG   SF_A8_COPY_BACK_ADDR;        // size = 512 bytes
MMP_ULONG   A8_TEMP_DMA_ADDR;         // size = 2048+64 bytes
static MMP_ULONG   A8_INFO_ADDR = 0;             // size = 16 bytes = 3Byte page address (4 byte alignment) or 2Byte Sector address. // Need non cacheable.
volatile    SFNAND_SMCMAPWRITEINFO SFWriteInfo;
MMP_UBYTE   SFZone0BuildCnt = 0;
MMP_USHORT  SFSMCTotalZoneCnt = 0;
MMP_USHORT  SFSMMAPInfoBlkNum;
MMP_USHORT  SFCurrMapTabNum = 0;
MMP_USHORT  *SFSMCMapTabBuf;
MMP_USHORT  *SFSMCBlkAddrBuf;
volatile    MMP_USHORT  SFSMCZoneNum = 0xFFFF;
volatile    MMP_ULONG   SFWriteLogPageAddr = 0xFFFFFFFF;
volatile    MMP_UBYTE   SFPreparePostCopy = 0;
MMP_USHORT  SFPwrOnFirst = 1;
MMP_USHORT  SFSetMapLabel = 0;
MMP_ULONG   SFSMTmpAddr = 0;
MMP_ULONG   m_ulSFMDmaAddr;
//MMP_UBYTE   m_gbSFMShortPage = 0;

MMPF_OS_SEMID  SFSMDMASemID;

MMP_BOOL m_gbSFMShortPage = MMP_FALSE;

MMP_ULONG  glSFTimeoutCnt = 0x0;
MMP_ULONG glSFTimeOutLimit = 0x50;  //Define 5ms as while polling time-out
MMP_BOOL   glSFNandFail = MMP_FALSE;

MMP_USHORT gsSPINandType = 0; // 0 for old type, 1 for new type
//==============================================================================
//
//                              External
//
//==============================================================================
extern  MMP_ULONG SNDMAAddr;

//==============================================================================
//
//                              FUNCTION Body
//
//==============================================================================
/**
*   @brief  MMPF_SFNAND_CheckReady
*
*
*/
MMP_ERR MMPF_SFNAND_CheckReady(void)
{
    MMP_ULONG   stampstart = 0; 
    MMP_ULONG   stampstop  = 0; 
    AITPS_SIF pSIF = AITC_BASE_SIF; 
    
    
    stampstart = OSTimeGet();
    //while((pSIF->SIF_INT_CPU_SR & SIF_CMD_DONE) == 0) ;
    while(1){
        if (pSIF->SIF_INT_CPU_SR & SIF_CMD_DONE)
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 200){
                RTNA_DBG_Str(0, "SFNAND CheckReady Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }
        }   
    }

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_WaitInterruptDone
*
*
*/
MMP_ERR MMPF_SFNAND_WaitInterruptDone(void)
{
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_WaitCount
*
*
*/
void MMPF_SFNAND_WaitCount(MMP_ULONG count)
{
    MMP_ULONG i;

    for (i = 0; i < count; i++);
}
/**
*   @brief  MMPF_SFNAND_ISR
*
*
*/
void    MMPF_SFNAND_ISR(void)
{


}
/**
*   @brief  MMPF_SFNAND_SetMemory
*
*
*/
MMP_ERR  MMPF_SFNAND_SetMemory(MMP_ULONG ulStartAddr)
{
    #if 0
    RTNA_DBG_Str1("SFNAND Logical->Physical Use Address:");
    RTNA_DBG_Long1(ulStartAddr);
    RTNA_DBG_Str1("\r\n");
    #endif

    if (ulStartAddr == 0) {
        return MMP_NAND_ERR_PARAMETER;
    }

#if(FULL_ZONE)
    A8_INFO_ADDR = ulStartAddr;
    SF_A8_COPY_BACK_ADDR = A8_INFO_ADDR + 0x10;
    SF_A8_RA_BUF_ADDR = SF_A8_COPY_BACK_ADDR + 0x200;
    SFSMC_BLK_ADDR_BUF = SF_A8_RA_BUF_ADDR + 0x40;
    SFSMC_MAP_DATA_BUF = SFSMC_BLK_ADDR_BUF + 0x800;
    
#endif
	MMPF_SFNAND_SetTmpAddr(SFSMC_MAP_DATA_BUF + 0x800, 2112);
	
    SFPreparePostCopy = 0;
    SFWriteLogPageAddr = 0xFFFFFFFF;
    SFSMCZoneNum = 0xFFFF;

    SFCurrMapTabNum = 0;
    SFZone0BuildCnt = 0;
    SFSMCMapTabBuf = (MMP_USHORT *)SFSMC_MAP_DATA_BUF;
    SFSMCBlkAddrBuf = (MMP_USHORT *)SFSMC_BLK_ADDR_BUF;

    if (ulStartAddr != SFSMTmpAddr && SFSMTmpAddr != 0) {
        MMPF_SFNAND_BuildAllMapTable();
    }
    SFSMTmpAddr = ulStartAddr;
    //RTNA_DBG_PrintLong(0, SFSMC_MAP_DATA_BUF);

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_FinishWrite
*
*
*/
MMP_ERR    MMPF_SFNAND_FinishWrite(void)
{
    MMP_ERR   ERRbyte = 0;   

    RTNA_DBG_Str1("API OF <MMPF_SFNAND_FinishWrite> is executing now\r\n\r\n\r\n\r\n");
    if(SFWriteLogPageAddr != 0xFFFFFFFF) {
        ERRbyte = MMPF_SFNAND_FinishWritePage(SFWriteLogPageAddr-1);   
        if (ERRbyte)
            return ERRbyte;        
    }
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_MemoryAlloc
*
*
*/
MMP_USHORT MMPF_SFNAND_MemoryAlloc(void)
{

    MMP_USHORT  size = 0;
#if (FULL_ZONE)
    size = 2*2048+64+512+2112 + 0x10; // Last 2112 is for temp buffer;
#endif
    return size;
}
/**
*   @brief  MMPF_SFNAND_Enable
*
*
*/
MMP_ERR    MMPF_SFNAND_Enable(MMP_BOOL bEnable, MMP_BOOL bWProtect)
{
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_GetPageAddr
*
*
*/
void    MMPF_SFNAND_GetPageAddr(MMP_ULONG address)
{
    MMP_USHORT tmp_addr;    
      
    tmp_addr = (address >> 2);
    /*RTNA_DBG_Str(0, "address:");
    RTNA_DBG_Long(0, address);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, tmp_addr);
    RTNA_DBG_Str(0, "\r\n");
*/
    *(MMP_ULONG *)A8_INFO_ADDR = tmp_addr;
/*  
    RTNA_DBG_Str(0, "address:");
    RTNA_DBG_Long(0, address);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, spare);
    RTNA_DBG_Str(0, "\r\n");
    
    //if (spare == 0 && address == 0) {
	    RTNA_DBG_Str(0, "SMAddr:");
	    RTNA_DBG_Byte(0, SMAddr[0]);
	    RTNA_DBG_Str(0, ":");
	    RTNA_DBG_Byte(0, SMAddr[1]);
	    RTNA_DBG_Str(0, ":");
	    RTNA_DBG_Byte(0, SMAddr[2]);
	    RTNA_DBG_Str(0, ":");
	    RTNA_DBG_Byte(0, SMAddr[3]);
	    RTNA_DBG_Str(0, ":");
	    RTNA_DBG_Byte(0, SMAddr[4]);
	    RTNA_DBG_Str(0, "\r\n");
	//}
*/	
}

/**
*   @brief  MMPF_SFNAND_GetSectorAddr
*
*
*/
void    MMPF_SFNAND_GetSectorAddr(MMP_ULONG address, MMP_BOOL bRA, MMP_BOOL bRW)
{
    MMP_USHORT tmp_addr;    
    MMP_UBYTE  ubShift = 0;
    if (bRW == MMP_TRUE) { // Read Case
        if (gsSPINandType == 1)
            ubShift = 0;
        else 
            ubShift = 8;
    }
    else { 
        ubShift = 0;
    }    
    if (gsSPINandType == 1) {
        if (bRA) {
            *(MMP_ULONG *)A8_INFO_ADDR = (2048) << ubShift; // Read RA area.
        }
        else {
            tmp_addr = (address & 3) * 512;
            *(MMP_ULONG *)A8_INFO_ADDR = (tmp_addr) << ubShift;
        }
    }
    else {
        if (bRA) {
            *(MMP_ULONG *)A8_INFO_ADDR = (NAD_WRAPVALUE(NAD_WRAP64) | 2048) << ubShift; // Read RA area.
        }
        else {
            tmp_addr = (address & 3) * 512;
            *(MMP_ULONG *)A8_INFO_ADDR = (NAD_WRAPVALUE(NAD_WRAP2048) | tmp_addr) << ubShift;
        }        
    }
    /*
    RTNA_DBG_Str(0, "GS:");
    RTNA_DBG_Short(0, gsSPINandType);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Short(0, bRA);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Short(0, ubShift);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, *(MMP_ULONG *)A8_INFO_ADDR);
    RTNA_DBG_Str(0, "\r\n");
    */
}

/**
*   @brief  MMPF_SFNAND_SendCommand
*
*
*/
MMP_ERR MMPF_SFNAND_SendCommand(SFNANDCommand *SFNandCommand)
{
	AITPS_SIF pSIF = AITC_BASE_SIF;
    MMP_UBYTE ubControl = 0;
    MMP_ERR   ERRbyte = 0; 
    // Modify to interrupt.
    pSIF->SIF_INT_CPU_SR = SIF_CLR_CMD_STATUS;
    pSIF->SIF_CMD = SFNandCommand->ubCommand;
    pSIF->SIF_FLASH_ADDR = SFNandCommand->ulNandAddr;
    if (SFNandCommand->RW == NAND_WRITE) {
        ubControl |= SIF_W;
    }    
    else  {
        ubControl |= SIF_R;
    }

    if (SFNandCommand->usDMASize != 0) {
        pSIF->SIF_DMA_ST = SFNandCommand->ulDMAAddr;
        pSIF->SIF_DMA_CNT = SFNandCommand->usDMASize - 1;
        ubControl |= SIF_DMA_EN | SIF_DATA_EN;
    }
    if (SFNandCommand->ubNandAddrCnt > 4) {
        RTNA_DBG_Str(0, "Err SF Nand address count\r\n");
    }
    else {
        if (SFNandCommand->ubNandAddrCnt != 0) {
            ubControl |= SIF_ADDR_EN;
            switch (SFNandCommand->ubNandAddrCnt) {
                case 1:
                    ubControl |= SIF_ADDR_LEN_1;
                    break;
                case 2:
                    ubControl |= SIF_ADDR_LEN_2;
                    break;
                case 3:
                    ubControl |= (SIF_ADDR_LEN_3);
                    break;
            }

        }
    }
    pSIF->SIF_CTL =  (SIF_START | ubControl);
    // Modify to interrupt.    
    ERRbyte = MMPF_SFNAND_CheckReady(); 
    if (ERRbyte)
        return ERRbyte;
        
    
    return MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_MMPF_SFNAND_GetStatus
*
*
*/
MMP_ERR  MMPF_SFNAND_GetStatus(MMP_UBYTE *status)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_STATUS, 0, 1, NAD_GETSTATUS, 1, NAND_READ};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD); 
    if (ERRbyte)
        return ERRbyte;
    
    *status = *(MMP_ULONG *)A8_INFO_ADDR;
    return MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_Reset
*
*
*/
MMP_ERR MMPF_SFNAND_Reset(void)
{
    #if 1
    MMP_UBYTE status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;    
    SFNANDCommand SFCMD = {0, 0, 0, NAD_RESET, 0, NAND_NONE_ACCESS};
    
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
        
    MMPF_SFNAND_WaitCount(40);    
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);     
        if (ERRbyte)
            return ERRbyte;  
        
        RTNA_DBG_Str(0, "status:");
        RTNA_DBG_Byte(0, status);
        RTNA_DBG_Str(0, "\r\n");
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 50){
                RTNA_DBG_Str(0, "MMPF_SFNAND_Reset Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }        
    }    
    #endif
    MMPF_SFNAND_SetProtection(0);
    return  MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_WriteEn
*
*
*/
MMP_ERR MMPF_SFNAND_WriteEn(MMP_BOOL bEnable)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {0, 0, 0, NAD_WRITE_EN, 0, NAND_NONE_ACCESS};
    if (bEnable) {
        SFCMD.ubCommand = NAD_WRITE_EN;
    }
    else {
        SFCMD.ubCommand = NAD_WRITE_DIS;
    }
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;    
    
    return  MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_GetID
*
*
*/
MMP_ERR MMPF_SFNAND_GetID(MMP_UBYTE *MID, MMP_UBYTE *DID)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_READID, 0, 2, NAD_READID, 1, NAND_READ};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
    
    *MID = (((*(MMP_ULONG *)A8_INFO_ADDR) & 0xFF));
    *DID = (((*(MMP_ULONG *)A8_INFO_ADDR) & 0xFF00) >> 8);
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_GetID
*
*
*/
MMP_ERR MMPF_SFNAND_GetID1(MMP_UBYTE *MID, MMP_UBYTE *DID1, MMP_UBYTE *DID2)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_READID, 0, 3, NAD_READID, 0, NAND_READ};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
    
    *MID = (((*(MMP_ULONG *)A8_INFO_ADDR) & 0xFF));
    *DID1 = (((*(MMP_ULONG *)A8_INFO_ADDR) & 0xFF00) >> 8);
    *DID2 = (((*(MMP_ULONG *)A8_INFO_ADDR) & 0xFF0000) >> 16);    
    return  MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNANDGetFeature
*
*
*/
MMP_ERR  MMPF_SFNAND_GetFeature(MMP_UBYTE *status)
{  
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_FEATURE, 0, 1, NAD_GETSTATUS, 1, NAND_READ};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
    
    *status = *(MMP_ULONG *)A8_INFO_ADDR;
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_SetFeature
*
*
*/
MMP_ERR  MMPF_SFNAND_SetFeature(MMP_UBYTE bFeature)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_FEATURE, 0, 1, NAD_SETFEATURE, 1, NAND_WRITE};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    *(volatile MMP_UBYTE *)A8_INFO_ADDR = bFeature;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
    
    return MMP_ERR_NONE;
}
/*   @brief  MMPF_SFNAND_GetProtection
*
*
*/
MMP_ERR  MMPF_SFNAND_GetProtection(MMP_UBYTE *status)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_PROTECT, 0, 1, NAD_GETSTATUS, 1, NAND_READ};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);   
    if (ERRbyte)
        return ERRbyte;
    
    *status = *(MMP_ULONG *)A8_INFO_ADDR;
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_SetProtection
*
*
*/
MMP_ERR  MMPF_SFNAND_SetProtection(MMP_UBYTE bProtection)
{
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {NAD_ADDRESS_PROTECT, 0, 1, NAD_SETFEATURE, 1, NAND_WRITE};
    SFCMD.ulDMAAddr = A8_INFO_ADDR;
    *(volatile MMP_UBYTE *)A8_INFO_ADDR = bProtection;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);    
    if (ERRbyte)
        return ERRbyte;
    
    return MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_SetType
*
*
*/
MMP_ERR MMPF_SFNAND_SetType(MMP_UBYTE ubType)
{
    // ubType = 0; // Micron, GIGADEVICE old version, ATO use this.
    // ubType = 1; // GIGADEVICE new version use this.
    gsSPINandType = ubType;
    SFSMCTotalZoneCnt = 1;
    m_gbSFMShortPage = MMP_FALSE;
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SM_InitialInterface
*
*
*/
MMP_ERR MMPF_SFNAND_InitialInterface(void)
{
    AITPS_SIF pSIF = AITC_BASE_SIF;
    AITPS_GBL pGBL = AITC_BASE_GBL;
	#if (CHIP == MCR_V2)
	pGBL->GBL_CLK_EN[1] = GBL_CLK_BS_SPI;
	pGBL->GBL_MISC_IO_CFG |= GBL_SIF_PAD_EN;
	#endif
    #if FPGA_PLATFOMR
    pSIF->SIF_CLK_DIV = 3;//div 8
    #else
    pSIF->SIF_CLK_DIV = SIF_CLK_DIV_4; // div 4
    #endif
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_SendAddress
*/
MMP_ERR MMPF_SFNAND_SendAddress(MMP_UBYTE *ubAddr, MMP_USHORT usPhase)
{
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_GetSize
*
*
*/

MMP_ERR MMPF_SFNAND_GetSize(MMP_ULONG *pSize)
{
#if 1
    MMP_UBYTE bMID= 0;
    MMP_UBYTE bDID= 0;
    MMP_UBYTE bDID1= 0;
    MMPF_SFNAND_GetID(&bMID, &bDID);
    RTNA_DBG_Str(0, "ID:");
    RTNA_DBG_Byte(0, bDID);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Byte(0, bMID);    
    RTNA_DBG_Str(0, "\r\n"); 
    switch (bMID) {   
        case 0xC8:
            gsSPINandType = 0;
            break;
        case 0x9B:
            gsSPINandType = 0;
            break;
        default:
            RTNA_DBG_Str(0, "May use new SPI\r\n");
            MMPF_SFNAND_GetID1(&bMID, &bDID, &bDID1);
            RTNA_DBG_Str(0, "ID:");
            RTNA_DBG_Byte(0, bMID);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Byte(0, bDID);    
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Byte(0, bDID1);        
            RTNA_DBG_Str(0, "\r\n");      
            if (bMID != 0xC8) {
    	        RTNA_DBG_Str(0, "Unknown MID\r\n");
    	        *pSize = 0;
    	        SFSMCTotalZoneCnt = 0;
                return  MMP_NAND_ERR_RESET;
            }
            else {
                gsSPINandType = 1; 
            }
            break;
    }
    switch(bDID) {
	    case 0xF1:
	    case 0xB1:
	    case 0xA1:
	    case 0x12:
	    case 0xD1:     // add for GD NAND 1G Rev:B
	    case 0x21:     // add for ESMT NAND 1G
	        *pSize = (1*1000*128);
	        SFSMCTotalZoneCnt = 1;
	        m_gbSFMShortPage = MMP_FALSE;
	        break;
	    case 0xB2:
	    case 0xA2:
	        *pSize = (2*1000*128);
	        SFSMCTotalZoneCnt = 1;
	        m_gbSFMShortPage = MMP_FALSE;
	        break;
        default:  
	        RTNA_DBG_Str(0, "Unknown Device ID\r\n");
	        *pSize = 0;
	        SFSMCTotalZoneCnt = 0;
            break;
    }
#endif    
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_ECCDecode
*
*
*/
MMP_ERR MMPF_SFNAND_ECCDecode(MMP_UBYTE* p_buf, MMP_UBYTE* p_ecc, MMP_UBYTE* ecc_data)
{
    #if 0
    MMP_UBYTE offset, corrected;
    MMP_ERR   status;

    if ((p_ecc[0] != ecc_data[0]) || (p_ecc[1] != ecc_data[1]) || (p_ecc[2] != ecc_data[2])) {
        status = MMPF_SFNAND_ECCCorrect(p_buf, p_ecc, ecc_data, &offset, &corrected);
        if (status == MMP_NAND_ERR_ECC_CORRECTABLE) {
            *((MMP_UBYTE*)p_buf + offset) = corrected;
            return MMP_NAND_ERR_ECC_CORRECTABLE;
        }
        else if (status == MMP_NAND_ERR_ECC)
            return MMP_NAND_ERR_ECC;
    }
    #endif
    return MMP_ERR_NONE;
}

/**
*   @brief  MMPF_SFNAND_ECCCorrect
*
*
*/
MMP_ERR MMPF_SFNAND_ECCCorrect(MMP_UBYTE *p_data, MMP_UBYTE *ecc1, MMP_UBYTE *ecc2, MMP_UBYTE *p_offset, MMP_UBYTE *p_corrected)
{
// SPI NAND may no need to support this. Device have support.
#if 0
  int i;
  MMP_UBYTE tmp0_bit[8],tmp1_bit[8],tmp2_bit[8], tmp0, tmp1, tmp2;
  MMP_UBYTE comp0_bit[8],comp1_bit[8],comp2_bit[8];
  MMP_UBYTE ecc_bit[22];
  MMP_UBYTE ecc_gen[3];
  MMP_UBYTE ecc_sum=0;
  MMP_UBYTE ecc_value,find_byte,find_bit;

  tmp0 = ~ecc1[0];
  tmp1 = ~ecc1[1];
  tmp2 = ~ecc1[2];


  for (i = 0; i <= 2; ++i)
  {
      ecc_gen[i] = ~ecc2[i];
  }

  for (i = 0; i < 7; i++) {
      tmp0_bit[i]= tmp0 & 0x01;
      tmp0 >>= 1;
  }
  tmp0_bit[7] = tmp0 & 0x01;

  for (i = 0; i < 7; i++) {
      tmp1_bit[i]= tmp1 & 0x01;
      tmp1 >>= 1;
  }
  tmp1_bit[7] = tmp1 & 0x01;

  for (i = 0; i < 7; i++) {
      tmp2_bit[i]= tmp2 & 0x01;
      tmp2 >>= 1;
  }
  tmp2_bit[7]= tmp2 & 0x01;

  for (i = 0; i < 7; i++) {
      comp0_bit[i]= ecc_gen[0] & 0x01;
      ecc_gen[0] >>= 1;
  }
  comp0_bit[7]= ecc_gen[0] & 0x01;

  for (i = 0; i < 7; i++) {
      comp1_bit[i]= ecc_gen[1] & 0x01;
      ecc_gen[1] >>= 1;
  }
  comp1_bit[7]= ecc_gen[1] & 0x01;

  for (i = 0; i < 7; i++) {
      comp2_bit[i]= ecc_gen[2] & 0x01;
      ecc_gen[2] >>= 1;
  }
  comp2_bit[7]= ecc_gen[2] & 0x01;

  for (i = 0; i <= 5; ++i)
  {
      ecc_bit[i] = tmp2_bit[i + 2] ^ comp2_bit[i + 2];
  }

  for (i = 0; i <= 7; ++i)
  {
      ecc_bit[i + 6] = tmp0_bit[i] ^ comp0_bit[i];
  }

  for (i = 0; i <= 7; ++i)
  {
      ecc_bit[i + 14] = tmp1_bit[i] ^ comp1_bit[i];
  }

  for (i = 0; i <= 21; ++i)
  {
      ecc_sum += ecc_bit[i];
  }

  if (ecc_sum == 11)
  {
      find_byte = (ecc_bit[21] << 7) + (ecc_bit[19] << 6) + (ecc_bit[17] << 5) + (ecc_bit[15] << 4) + (ecc_bit[13] << 3) + (ecc_bit[11] << 2) + (ecc_bit[9] << 1) + ecc_bit[7];
      find_bit = (ecc_bit[5] << 2) + (ecc_bit[3] << 1) + ecc_bit[1];
      ecc_value = (p_data[find_byte] >> find_bit) & 0x01;
      if (ecc_value == 0)
          ecc_value = 1;
      else
          ecc_value = 0;

      *p_offset = find_byte;
      *p_corrected = p_data[find_byte];

      for (i = 0; i < 7; i++) {
          tmp0_bit[i] = *p_corrected & 0x01;
          *p_corrected >>= 1;
      }
      tmp0_bit[7] = *p_corrected & 0x01;

      tmp0_bit[find_bit] = ecc_value;

      *p_corrected = (tmp0_bit[7] << 7) + (tmp0_bit[6] << 6) + (tmp0_bit[5] << 5) + (tmp0_bit[4] << 4) + (tmp0_bit[3] << 3) + (tmp0_bit[2] << 2) + (tmp0_bit[1] << 1) + tmp0_bit[0];

      return MMP_NAND_ERR_ECC_CORRECTABLE;
  }
  else if ((ecc_sum == 0) || (ecc_sum == 1)) {
      return MMP_ERR_NONE;
  }
  else
  {
      return MMP_NAND_ERR_ECC;
  }
#endif
  return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_GetMapAddress
*
*
*/
MMP_ULONG MMPF_SFNAND_GetMapAddress(MMP_ULONG ulLogicalAddr)
{
    MMP_USHORT page_offset;
    MMP_USHORT zone_num =0;
    MMP_USHORT blk_offset = 0;
    //short blk_num;
    MMP_USHORT blk_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT usBlockSize = 0;
	MMP_USHORT ubPagePerBlockMask = 0;
	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
		usBlockSize = 256;
		ubPagePerBlockMask = 0xFF;
	}
	else {
		ubBlockSizeShift = 5;
		usBlockSize = 32;
		ubPagePerBlockMask = 0x1F;
	}

    blk_num = (MMP_USHORT)(ulLogicalAddr >> ubBlockSizeShift);
    page_offset = (MMP_USHORT)(ulLogicalAddr & ubPagePerBlockMask);

    zone_num = blk_num / MAX_BLOCK_NUM;
    blk_offset = blk_num - zone_num * MAX_BLOCK_NUM;

    if((zone_num != SFSMCZoneNum)) {
#if (FULL_ZONE)
        MMPF_SFNAND_BuildMapTab(zone_num, SFSMCBlkAddrBuf, SFSMCMapTabBuf);
#endif
    }
    
  return  ((*(MMP_USHORT *)(SFSMCMapTabBuf + blk_offset) + (SFSMCZoneNum << 10)) << ubBlockSizeShift) + page_offset;
  //return  ((2 + (SFSMCZoneNum << 10)) << ubBlockSizeShift) + page_offset;
}

/**
*   @brief  MMPF_SFNAND_ReadSector
*
*
*/
MMP_ERR MMPF_SFNAND_ReadSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount)
{
    MMP_ULONG phy_page_addr;
    MMP_USHORT tmp_zone_num = 0xFFFF;
    MMP_USHORT *tmp_sm_map_tab_buf;
    MMP_UBYTE  ERRbyte = 0; 

    #if 0
    RTNA_DBG_Str0("Read: startsect : sectcount: ");
    RTNA_DBG_Long0(dmastartaddr);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(startsect);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(sectcount);
    RTNA_DBG_Str0("\r\n");
    #endif

    if(SFWriteLogPageAddr != 0xFFFFFFFF) {
        tmp_zone_num = SFSMCZoneNum;
        tmp_sm_map_tab_buf = SFSMCMapTabBuf;
    }

    /// @todo need to add async write state in read operation
    while (sectcount) {
        if(((startsect & 0xFFFFFF00) == (SFWriteLogPageAddr & 0xFFFFFF00))
                && (startsect >= SFWriteLogPageAddr)) {

            if(SFWriteLogPageAddr != 0xFFFFFFFF) {
                #if(_LOGADDRESS)
                RTNA_DBG_Str3("readsector finish:");
                RTNA_DBG_Str3("need to add async write state in read operation");
                RTNA_DBG_Str3("\r\n");
                #endif
                MMPF_SFNAND_FinishWritePage(SFWriteLogPageAddr - 1);
            }
        }
        phy_page_addr = MMPF_SFNAND_GetMapAddress(startsect);
        /*
        RTNA_DBG_Str(0, "GMA:");
        RTNA_DBG_Short(0, startsect);
        RTNA_DBG_Str(0, ":");
        RTNA_DBG_Short(0, phy_page_addr);
        RTNA_DBG_Str(0, "\r\n");
*/
        ERRbyte = MMPF_SFNAND_ReadPhysicalSector(dmastartaddr, phy_page_addr);
        if(ERRbyte)
            return ERRbyte;
        
        sectcount--;
        dmastartaddr += 512;
        startsect++;
    }

    if(tmp_zone_num != 0xFFFF) {
        SFSMCZoneNum = tmp_zone_num;
        SFSMCMapTabBuf = tmp_sm_map_tab_buf;
    }
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_BuildOneMapTable
*
*
*/
void    MMPF_SFNAND_BuildOneMapTable(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf, MMP_USHORT *smcmaptabbuf)
{
    MMP_USHORT *smcblkaddrbuf1 = (MMP_USHORT *)SFSMC_BLK_ADDR_BUF;

    MMPF_SFNAND_BuildMapTab(zone_num, smcblkaddrbuf1, smcmaptabbuf);
    // smcblkaddrbuf1 physcial->logical map table
//    MMPF_SFNAND_GetAllBlkAddr(zone_num, smcblkaddrbuf1);
    // smcmaptabbuf logical to physical map table
//    MMPF_SFNAND_GetMapTab(smcblkaddrbuf1, smcmaptabbuf);
}
/**
*   @brief  MMPF_SFNAND_BuildAllMapTable
*
*
*/
MMP_USHORT MMPF_SFNAND_BuildAllMapTable(void)
{
    MMP_ULONG   size;
    MMP_LONG    i;

#if(FULL_ZONE)
    MMPF_SFNAND_GetSize(&size);
    for(i = (SFSMCTotalZoneCnt - 1); i >= 0; i--) {
        MMPF_SFNAND_BuildOneMapTable(i, SFSMCBlkAddrBuf, SFSMCMapTabBuf);
        if(glSFNandFail == MMP_TRUE) {
            return 1;
        }
    }
#endif
    return MMP_ERR_NONE;
}


/**
*   @brief  MMPF_SFNAND_GetAllBlkAddr
*
*
*/
MMP_UBYTE MMPF_SFNAND_GetAllBlkAddr(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf)
{
    MMP_USHORT  map_blk_num;
    MMP_USHORT  free_blk_num, bad_blk_num;
    MMP_UBYTE  *read_ra_buf2;
    MMP_ULONG   start_zone_addr;
    MMP_USHORT  i;
    MMP_USHORT  blk_status;
    MMP_USHORT  *i3;
//	MMP_UBYTE kk = 0;
    SFSMCZoneNum = zone_num;
	if (!m_gbSFMShortPage) {
    	start_zone_addr = zone_num << 18;
	}
	else {
		start_zone_addr = zone_num << 15;
	}
    map_blk_num = 0;
    free_blk_num = 0;
    bad_blk_num = 0;

    i3 = smcblkaddrbuf;
    for(i = 0; i < 1024; i++) {
        MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, start_zone_addr);
        if(glSFNandFail == MMP_TRUE) {
            return 1;
        } 
        read_ra_buf2 = (MMP_UBYTE *)SF_A8_RA_BUF_ADDR;

        /* Check MAP! label */
        if((read_ra_buf2[1] == 0x4D) && (read_ra_buf2[5] == 0x41)) {
            MMPF_SFNAND_EraseBlock(start_zone_addr);
            RTNA_DBG_Str(1, "erase reserve blk\r\n");
        }

        if(read_ra_buf2[0] == 0xFF) {
            if( ( read_ra_buf2[2] == 0xFF) && ( read_ra_buf2[3] == 0xFF)) {
                blk_status = FREE_BLOCK;
		       	//RTNA_DBG_Str(0, "Free ??\r\n"); 				
            }
            else{
               blk_status = (MMP_USHORT)read_ra_buf2[2] + ((MMP_USHORT)read_ra_buf2[3] <<8) ;// Log address
	        	
	        	#if 0
	        	RTNA_DBG_Str(0, "blk_status:");
	        	RTNA_DBG_Long(0, blk_status);
	        	RTNA_DBG_Str(0, "\r\n");
	        	#endif
				
            }
        }

        /* map block bad block or reserved block */
        else{
        	RTNA_DBG_Str(0, "Bad:");
        	RTNA_DBG_Short(0, i);
        	RTNA_DBG_Str(0, " ");
            blk_status = REV_BAD_BLOCK;
            bad_blk_num++;
        }

        *i3++ = blk_status;

        if( (blk_status - FREE_BLOCK) == 0 ){
            free_blk_num++;
        }
        if (!m_gbSFMShortPage) {
       		start_zone_addr += 256;
    	}
    	else {
    		start_zone_addr += 32;
    	}
    }

    if(bad_blk_num == 1024) {
        glSFNandFail = MMP_TRUE;
    }
    RTNA_DBG_PrintShort(0, free_blk_num);
    RTNA_DBG_PrintShort(0, bad_blk_num);

    return 0;

}

/**
*   @brief  MMPF_SFNAND_BuildMapTab
*/
void    MMPF_SFNAND_BuildMapTab(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf, MMP_USHORT *smcmaptabbuf)
{
	RTNA_DBG_Str(0, "BuildMap\r\n");
#if (FULL_ZONE)
    if(zone_num == 0) {
        SFSMCMapTabBuf = (MMP_USHORT *)(SFSMC_MAP_DATA_BUF + (zone_num<<11));
        SFSMCZoneNum = 0;
    }
#endif
	MMPF_SFNAND_GetAllBlkAddr(zone_num, smcblkaddrbuf);
	
    if(glSFNandFail == MMP_TRUE) {
        return;
    }
	MMPF_SFNAND_GetMapTab(smcblkaddrbuf, smcmaptabbuf);
    SFWriteInfo.free_blk_idx = SFSMCMapTabBuf + (MAX_BLOCK_NUM - 1);
}

/**
*   @brief  MMPF_SFNAND_GetLogBlk
*
*
*/
MMP_UBYTE  MMPF_SFNAND_GetLogBlk(MMP_ULONG ulPageAddr)
{
    MMP_USHORT blk_num, zone_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
	}
	else {
		ubBlockSizeShift = 5;
	}
    blk_num = ulPageAddr >> ubBlockSizeShift;

    zone_num = blk_num / MAX_BLOCK_NUM;
    SFWriteInfo.log_blk_offset = blk_num - zone_num*MAX_BLOCK_NUM;

    return(zone_num);
}

/**
*   @brief  MMPF_SFNAND_GetMapTab
*
*
*/
void MMPF_SFNAND_GetMapTab(MMP_USHORT * smcblkaddrbuf, MMP_USHORT * smcmaptabbuf)
{
    MMP_USHORT  i;
    MMP_USHORT  *i1, *i2, *i3;
    MMP_USHORT  *pend_addr_free_blk;
    MMP_USHORT  blk_idx = 0;
    MMP_USHORT  log_addr_state;
    MMP_ULONG   phy_block_addr = 0;
    MMP_UBYTE   *read_ra_buf;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_UBYTE  ubBlockSize = 0;
	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
		ubBlockSize = 0xFF;
	}
	else {
		ubBlockSizeShift = 5;
		ubBlockSize = 0x1F;
	}
    i1 = SFSMCMapTabBuf;
    for(i = 0;i < 1024; i++){
        *i1++ = NULL_BLK_ENTRY;
    }

    i1 = smcblkaddrbuf;
    i2 = SFSMCMapTabBuf + MAX_BLOCK_NUM;

    pend_addr_free_blk = SFSMCMapTabBuf + ZONE_SIZE;
    for(i = 0; i<ZONE_SIZE; i++){
        log_addr_state = *i1++;
        // 判斷是FREE 或是ERROR BLOCK
        if( ( log_addr_state - FREE_BLOCK) >= 0){
            if( (log_addr_state - (MMP_USHORT)FREE_BLOCK) == 0){
                // 如果他是FREE BLOCK. 在MAP TAB + 1000 的位置上先記下來.
                if( (i2 - pend_addr_free_blk) < 0){ // Address 相減小於0 -> 代表只會填1024-1000 個進SMMAPTABBUFFER的最後.             
                    *i2++ = blk_idx;
                    blk_idx++;
                    continue;
                }
                else{
                    blk_idx++;
                    continue;
                }
            }
            else{
                blk_idx++;
                continue;
            }
        }
        else{
            // 若是已有值.
            i3 = log_addr_state + SFSMCMapTabBuf; // 可以找到PHY block.
#if 0
                *i3++ = blk_idx;
#else
            if( *i3 == NULL_BLK_ENTRY){ 
                *i3++ = blk_idx;
            }
            else{
                phy_block_addr =  ( ( (SFSMCZoneNum <<10) + *i3)<<ubBlockSizeShift) + ubBlockSize;
                MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, phy_block_addr);

                read_ra_buf = (MMP_UBYTE *)SF_A8_RA_BUF_ADDR;
                if( ( (MMP_USHORT)read_ra_buf[2] + ( (MMP_USHORT)read_ra_buf[3] <<8)) == log_addr_state){
                    RTNA_DBG_Str0("=========duplicated phy addr=========\r\n");
                    phy_block_addr =  ( ( (SFSMCZoneNum <<10) + blk_idx)<<ubBlockSizeShift) + ubBlockSize;
                    MMPF_SFNAND_EraseBlock(phy_block_addr);
                }
                else{
                    RTNA_DBG_Str0("*********duplicated phy addr**********\r\n");
                    phy_block_addr =  ( ( (SFSMCZoneNum <<10) + *i3)<<ubBlockSizeShift) + ubBlockSize;
                    MMPF_SFNAND_EraseBlock(phy_block_addr);
                    *i3++ = blk_idx;
                }
            }

            blk_idx++;
#endif
       }
    }



}
/**
*   @brief  MMPF_SFNAND_GetWriteAddr
*
*
*/
void MMPF_SFNAND_GetWriteAddr(void)
{
    SFWriteInfo.old_phy_blk_addr += 1;
    SFWriteInfo.new_phy_blk_addr += 1;
}
/**
*   @brief  MMPF_SFNAND_CopyBack
*
*
*/
MMP_ERR MMPF_SFNAND_CopyBack(MMP_ULONG  ulSrcPage, MMP_ULONG ulDstPage)
{
    MMP_UBYTE   status;
    MMP_USHORT  *p_ra_buf;
    MMP_USHORT  i;
    MMP_ULONG   stampstart = 0; 
    MMP_ULONG   stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMDPage = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMDPE = {0, 0, 0, NAD_PROGRAMEXECUTE, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMDPL = {0, 0, 0, NAD_RANDOMPROGRAMLOAD, 2, NAND_NONE_ACCESS};
    // 1. Page Read to Cache command 13.
    MMPF_SFNAND_GetPageAddr(ulSrcPage);
    SFCMDPage.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPage);     
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_PLL_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);    
        if (ERRbyte)
            return ERRbyte;
        
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 50){
                RTNA_DBG_Str(0, "MMPF_SFNAND_CopyBack Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }      
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Read Sector Error\r\n");
        
        //P_FAIL flag do not affect the read process,so don't return errer at this situation
        //return MMP_NAND_ERR_PROGRAM;
    }
    // Update new RA.
    p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
    MEMSET(p_ra_buf, 0xFF, 64);
    //MMPF_SFNAND_GetLogBlk(ulDstPage);
    if(SFSetMapLabel == 0){
        for (i = 0 ; i <= 3; i++) {
            p_ra_buf = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR+(i << 4)+0x2);
            *p_ra_buf = SFWriteInfo.log_blk_offset;
        }
    }
    MMPF_SFNAND_GetSectorAddr(ulDstPage, MMP_TRUE, MMP_FALSE);
    SFCMDPL.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
/*
    RTNA_DBG_Str(0, "PLNAND:");
    RTNA_DBG_Long(0, SFCMDPL.ulNandAddr);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, SFSetMapLabel);    
    RTNA_DBG_Str(0, "\r\n");
*/
    SFCMDPL.ulDMAAddr = SF_A8_RA_BUF_ADDR;
    SFCMDPL.usDMASize = 64;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPL);       
    if (ERRbyte)
        return ERRbyte;
        
    // 2. Write Enable 
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_TRUE);    
    if (ERRbyte)
        return ERRbyte;
    
    // 3. Program execute.
    MMPF_SFNAND_GetPageAddr(ulDstPage);
    SFCMDPE.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
    //MMPF_SFNAND_SendCommand(&SFCMDPage); 
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPE);  
    if (ERRbyte)
        return ERRbyte;
     
    MMPF_SFNAND_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);      
        if (ERRbyte)
            return ERRbyte;
        
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_SFNAND_CopyBack Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }      
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Write Sector Error\r\n");
        return MMP_NAND_ERR_PROGRAM;
    }
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_FALSE);    
    if (ERRbyte)
        return ERRbyte;
     
    /*
    RTNA_DBG_Str(0, "CB:");
    RTNA_DBG_Long(0, ulSrcPage);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, ulDstPage);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, SFCMDPage.ulNandAddr);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, SFCMDPE.ulNandAddr);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, SFWriteInfo.log_blk_offset);    
    RTNA_DBG_Str(0, "\r\n");
    p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
    MEMSET(p_ra_buf, 0x33, 64);
    MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, ulDstPage);
    RTNA_DBG_Str(0, "RA0:");
    for(i = 0 ; i < 32; i++) {
        RTNA_DBG_Short(0, *p_ra_buf++);
        RTNA_DBG_Str(0, ":");
    }
    RTNA_DBG_Str(0, "\r\n");
*/
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_PreCopyBack
*
*
*/
MMP_ERR MMPF_SFNAND_PreCopyBack(MMP_ULONG ulPageAddr)
{
    MMP_USHORT tmp_zone_num;
    MMP_USHORT *i1;
    MMP_USHORT tmp_old_phy_block;
    MMP_USHORT tmp_new_phy_block;
    MMP_USHORT i;
    MMP_USHORT  rebuild_free_cntr = 3;
    MMP_USHORT result;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT usBlockSize = 0;
	MMP_ERR    ERRbyte = 0;

	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
		usBlockSize = 256;
	}
	else {
		ubBlockSizeShift = 5;
		usBlockSize = 32;
	}
    SFWriteInfo.loop_cntr = ulPageAddr & (usBlockSize - 1);
	
    tmp_zone_num = MMPF_SFNAND_GetLogBlk(ulPageAddr);
    if((SFSMCZoneNum - tmp_zone_num) != 0){
        RTNA_DBG_Str(1, "wtmp_zone_num:");
        RTNA_DBG_Long(1, tmp_zone_num);
        RTNA_DBG_Str(1, ":");
        RTNA_DBG_Long(1, SFSMCZoneNum);
        RTNA_DBG_Str(1, "\r\n");
        MMPF_SFNAND_BuildMapTab(tmp_zone_num, SFSMCBlkAddrBuf, SFSMCMapTabBuf);
    }


    while(rebuild_free_cntr){
        i1 = SFSMCMapTabBuf + MAX_BLOCK_NUM;
        for(i = 0; i < MAX_FREE_BLK_NUM; i++){
            SFWriteInfo.free_blk_idx = i1;
            tmp_new_phy_block = *i1++;
            if((tmp_new_phy_block-NULL_BLK_ENTRY) != 0) {
                result = 0;
                break;
            }
        }
        if((tmp_new_phy_block - NULL_BLK_ENTRY) == 0) {
            RTNA_DBG_Str(1, "NULL_BLK_ENTRY\r\n");
            MMPF_SFNAND_BuildMapTab(tmp_zone_num , SFSMCBlkAddrBuf, SFSMCMapTabBuf);
            result = 1;
        }
        if(result == 0){
            break;
        }
        rebuild_free_cntr -= 1;
    }
    SFWriteInfo.new_phy_blk_offset = tmp_new_phy_block;
    SFWriteInfo.new_phy_blk_addr = ((SFSMCZoneNum << 10) + SFWriteInfo.new_phy_blk_offset)<<ubBlockSizeShift;

    i1 = SFWriteInfo.log_blk_offset + SFSMCMapTabBuf;
    tmp_old_phy_block = *i1;
    SFWriteInfo.old_phy_blk_offset = tmp_old_phy_block;

    *i1 = SFWriteInfo.new_phy_blk_offset;

    i1 = SFWriteInfo.free_blk_idx;
    if((tmp_old_phy_block - NULL_BLK_ENTRY) != 0){
        // 若是現在要寫的BLOCK 有被寫過. 把BLOCK 的部份PAGE 搬走.
        *i1 = tmp_old_phy_block;
        SFWriteInfo.old_phy_blk_addr = ((SFSMCZoneNum <<10) + SFWriteInfo.old_phy_blk_offset) << ubBlockSizeShift;
        SFWriteInfo.cur_blk_state = OLD_BLOCK;
    }
    else{
        // 若是現在要寫的BLOCK 沒被寫過. 只改RA
        *i1 = NULL_BLK_ENTRY;
        SFWriteInfo.cur_blk_state = NEW_BLOCK;
    }
/*
    RTNA_DBG_Str(0, "PRCP:");
    RTNA_DBG_Long(0,  SFWriteInfo.old_phy_blk_addr);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0,  SFWriteInfo.new_phy_blk_addr);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0,  SFWriteInfo.loop_cntr);
    RTNA_DBG_Str(0, "\r\n");
*/
    if(SFWriteInfo.loop_cntr == 0){
        return MMP_ERR_NONE;
    }
	if (!m_gbSFMShortPage) {
	    //while( MMPF_SFNAND_SwapPages1() );
        while(1){
            ERRbyte = MMPF_SFNAND_SwapPages1();
            if(ERRbyte)
                return ERRbyte;
            else
                break;
        } 
    }
    else {
    	MMPF_SFNAND_SwapPagesShort();
    }

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_PostCopyBack
*
*
*/
MMP_ERR MMPF_SFNAND_PostCopyBack(void)
{
    MMP_USHORT tmp_loop_cntr;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT usBlockSize = 0;
	MMP_ERR    ERRbyte = 0;

	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
		usBlockSize = 256;
	}
	else {
		ubBlockSizeShift = 5;
		usBlockSize = 32;
	}

    tmp_loop_cntr = (SFWriteInfo.new_phy_blk_addr & (usBlockSize - 1) );

    if((SFWriteInfo.loop_cntr = (usBlockSize-1) - tmp_loop_cntr) != 0){
        RTNA_DBG_Str(0, "POCP:");
        RTNA_DBG_Long(0,  SFWriteInfo.old_phy_blk_addr);
        RTNA_DBG_Str(0, ":");
        RTNA_DBG_Long(0,  SFWriteInfo.new_phy_blk_addr);
        RTNA_DBG_Str(0, ":");
        RTNA_DBG_Long(0,  SFWriteInfo.loop_cntr);
        RTNA_DBG_Str(0, "\r\n");
        MMPF_SFNAND_GetWriteAddr();
    	if (!m_gbSFMShortPage) {
            //while(MMPF_SFNAND_SwapPages2());
        	while(1){
        	    ERRbyte = MMPF_SFNAND_SwapPages2();
        	    if(ERRbyte)
        	        return ERRbyte;
        	    else
        	        break;
        	}
        }
        else {
        	MMPF_SFNAND_SwapPagesShort();
        }
    }

    if(SFWriteInfo.cur_blk_state == OLD_BLOCK){
        MMPF_SFNAND_EraseBlock((MMP_ULONG)(((SFSMCZoneNum << 10) + SFWriteInfo.old_phy_blk_offset) << ubBlockSizeShift));
    }

    SFWriteInfo.cur_blk_state = 0;

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_SwapBigPages
*
*
*/
MMP_ERR MMPF_SFNAND_SwapBigPages(void)
{
    //MMP_USHORT  i;
    MMP_USHORT  *p_ra_buf;
    MMP_ERR ERRbyte = 0;

    if( !( (SFWriteInfo.loop_cntr>>2) & 0x3F) ){
        return MMP_ERR_NONE;
    }

    if(SFWriteInfo.cur_blk_state == OLD_BLOCK){
#if 0   // Has problem SUNNY_050816
        MMPF_SFNAND_CopyBack(SFWriteInfo.old_phy_blk_addr, SFWriteInfo.new_phy_blk_addr);
#else
        //for(i = 0; i < 4; i++){
        //    MMPF_SFNAND_ReadPhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.old_phy_blk_addr+i);
        //    MMPF_SFNAND_WritePhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.new_phy_blk_addr+i);
        ERRbyte = MMPF_SFNAND_ReadPhysicalPage(m_ulSFMDmaAddr, SFWriteInfo.old_phy_blk_addr);
        if (ERRbyte)
            return ERRbyte;
        
        ERRbyte = MMPF_SFNAND_WritePhysicalPage(m_ulSFMDmaAddr, SFWriteInfo.new_phy_blk_addr);
        if (ERRbyte)
            return ERRbyte;

            //if(status)
            //    return status;
        //}
#endif
    }
    else {
#ifdef  LOG_READ_WRITE_TEST
        RTNA_DBG_Str(0, "New block only writeRAbig:\r\n");
#endif
        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *p_ra_buf++ = 0xFFFF;
        //}
        MEMSET(p_ra_buf, 0xFF, 64);
        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
        *(p_ra_buf + ((SFWriteInfo.new_phy_blk_addr & 3)<<3)+ 1) = SFWriteInfo.log_blk_offset;

        ERRbyte = MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, SFWriteInfo.new_phy_blk_addr,((SFWriteInfo.new_phy_blk_addr & 3)<<3)+ 1, 2);
        if (ERRbyte)
            return ERRbyte;
    
    }
    // Not necessary SUNNY_050812
//    phy_page_addr = SFWriteInfo.new_phy_blk_addr;

    /* move to here for non-proper irq */
    SFWriteInfo.old_phy_blk_addr += 4;
    SFWriteInfo.new_phy_blk_addr += 4;
    SFWriteInfo.loop_cntr -= 4;

    // replac by writera and copyback SUNNY_050812
//    if(MMPF_SFNAND_WritePhysicalSector(SF_A8_COPY_BACK_ADDR, phy_page_addr)){
//      return SM_NOT_COMPLETE;
//    }

    return MMP_NAND_ERR_NOT_COMPLETE;
}
/**
*   @brief  MMPF_SFNAND_SwapSmallPages
*
*
*/
MMP_ERR  MMPF_SFNAND_SwapSmallPages(void)
{
    //MMP_USHORT i;
    MMP_USHORT * p_ra_buf;
    MMP_ERR status;
    MMP_ERR ERRbyte = 0;

    if(!(SFWriteInfo.loop_cntr & 3) ){
        return MMP_ERR_NONE;
    }

    if(SFWriteInfo.cur_blk_state == OLD_BLOCK){
        ERRbyte = MMPF_SFNAND_ReadPhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.old_phy_blk_addr);
        if (ERRbyte)
            return ERRbyte;
        
        status = MMPF_SFNAND_WritePhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.new_phy_blk_addr);
        if (status)
            return status;

    }
    else {
#ifdef  LOG_READ_WRITE_TEST
ait81x_uart_write("New block only writeRAsmall:\r\n");
#endif

#if 1   // Look like no meaning. And SPI NAND do not support write RA only, remove first.
        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *p_ra_buf++ = 0xFFFF;
        //}
        MEMSET(p_ra_buf, 0xFF, 64);

        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
        
        // For 2K SFNAND

       	*(p_ra_buf + ((SFWriteInfo.new_phy_blk_addr & 3)<<3) + 1) = SFWriteInfo.log_blk_offset;

        ERRbyte = MMPF_SFNAND_WriteRA( SF_A8_RA_BUF_ADDR, SFWriteInfo.new_phy_blk_addr, ((SFWriteInfo.new_phy_blk_addr & 3)<<3) + 1, 2);
        if (ERRbyte)
            return ERRbyte;
        
#endif
    }

    // Not necessary SUNNY_050812
//    phy_page_addr = SFWriteInfo.new_phy_blk_addr;

    /* move to here for non-proper irq */
    MMPF_SFNAND_GetWriteAddr();
    SFWriteInfo.loop_cntr -= 1;

    // Not necessary SUNNY_050812
//    if(MMPF_SFNAND_WritePhysicalSector(SF_A8_COPY_BACK_ADDR, phy_page_addr)){
//        return SM_NOT_COMPLETE;
//    }

    return MMP_NAND_ERR_NOT_COMPLETE;
}
/**
*   @brief  MMPF_SFNAND_SwapPages1
*
*
*/
MMP_ERR MMPF_SFNAND_SwapPages1(void)
{
    MMP_ERR err;

    do{
        err = MMPF_SFNAND_SwapBigPages();
        if(err) {
            if(err != MMP_NAND_ERR_NOT_COMPLETE) {
                return err;
            }
        }
    } while (err == MMP_NAND_ERR_NOT_COMPLETE);

    do{
        err = MMPF_SFNAND_SwapSmallPages();
        if(err) {
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_SwapPages2
*
*
*/
MMP_ERR MMPF_SFNAND_SwapPages2(void)
{
    MMP_ERR err;

    do{
        err = MMPF_SFNAND_SwapSmallPages();
        if(err){
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    do{
        err = MMPF_SFNAND_SwapBigPages();
        if(err){
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_SFNAND_SwapPagesShort(void) 
{
    //MMP_USHORT i;
    MMP_USHORT * p_ra_buf;
    MMP_ERR status;
    MMP_USHORT k, j;
    k = SFWriteInfo.loop_cntr;
	for(j = 0; j < k; j++) {
	    if(SFWriteInfo.cur_blk_state == OLD_BLOCK){
			//RTNA_DBG_Str(0, "S old\r\n");

	        MMPF_SFNAND_ReadPhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.old_phy_blk_addr);
	        status = MMPF_SFNAND_WritePhysicalSector(SF_A8_COPY_BACK_ADDR, SFWriteInfo.new_phy_blk_addr);
	        if (status)
	            return status;

	    }
	    else {
	#ifdef  LOG_READ_WRITE_TEST
	ait81x_uart_write("New block only writeRAsmall:\r\n");
	#endif
			//RTNA_DBG_Str(0, "S new\r\n");
	        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
	        //for(i = 0; i< 32; i++){
	        //    *p_ra_buf++ = 0xFFFF;
	        //}
	        MEMSET(p_ra_buf, 0xFF, 64);

	        p_ra_buf = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
	        
	        // For 2K SFNAND
	        if (!m_gbSFMShortPage) {
	        	*(p_ra_buf + ((SFWriteInfo.new_phy_blk_addr & 3)<<3) + 1) = SFWriteInfo.log_blk_offset;
	        }
	        else {
	        	// For 512 SFNAND
	        	*(p_ra_buf + 1) = SFWriteInfo.log_blk_offset;
			}
	        MMPF_SFNAND_WriteRA( SF_A8_RA_BUF_ADDR, SFWriteInfo.new_phy_blk_addr, 1, 2);
	    }

	    // Not necessary SUNNY_050812
	//    phy_page_addr = SFWriteInfo.new_phy_blk_addr;

	    /* move to here for non-proper irq */
	    MMPF_SFNAND_GetWriteAddr();
	    SFWriteInfo.loop_cntr -= 1;
	}
	return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SFNAND_WriteSector
*
*
*/
MMP_ERR MMPF_SFNAND_WriteSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount)
{
    MMP_USHORT *mem_ptr;
    MMP_ULONG   phy_page_addr;
    MMP_USHORT i;
    MMP_USHORT tmp_zone_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT ubPagePerBlockMask = 0;
	MMP_UBYTE  ERRbyte = 0;
	
	if (!m_gbSFMShortPage) {
		ubBlockSizeShift = 8;
		ubPagePerBlockMask = 0xFF;
	}
	else {
		ubBlockSizeShift = 5;
		ubPagePerBlockMask = 0x1F;
	} 
    #if 0
    RTNA_DBG_Str0("Write: startsect : sectcount: ");
    //RTNA_DBG_Long0(dmastartaddr);
    //RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(startsect);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(sectcount);
    RTNA_DBG_Str0("\r\n");
    #endif
/*	if (startsect == 0) {
		MMP_ULONG *temp = (MMP_ULONG *)dmastartaddr;
		RTNA_DBG_Str(0, "temp:");
		RTNA_DBG_Long(0, *temp++);
		RTNA_DBG_Str(0, "\r\n");
		RTNA_DBG_Str(0, "temp1:");
		RTNA_DBG_Long(0, *temp++);
		RTNA_DBG_Str(0, "\r\n");
		RTNA_DBG_Str(0, "temp2:");
		RTNA_DBG_Long(0, *temp);
		RTNA_DBG_Str(0, "\r\n");

	}
  */
    while (sectcount) {
        if ((sectcount == 1) && (startsect == 0)) {
            RTNA_DBG_Str(0, "SI:");
            RTNA_DBG_Long(0, startsect);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, SFWriteLogPageAddr);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, SFWriteInfo.new_phy_blk_addr);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, SFPreparePostCopy);            
            RTNA_DBG_Str(0, "\r\n");            

        }
        tmp_zone_num = MMPF_SFNAND_GetLogBlk(startsect);

        if((startsect != SFWriteLogPageAddr)){
            if(SFPreparePostCopy == 0) {
              RTNA_DBG_Str(0, "PCP0\r\n");
              ERRbyte = MMPF_SFNAND_PreCopyBack(startsect);
              if (ERRbyte)
                  return ERRbyte;
              
              SFPreparePostCopy = 0x01;
            }
            else {
                RTNA_DBG_Str(0, "PCP1\r\n");
                SFWriteInfo.old_phy_blk_addr -= 1;
                SFWriteInfo.new_phy_blk_addr -= 1;
                ERRbyte = MMPF_SFNAND_PostCopyBack();
                if (ERRbyte)
                    return ERRbyte;
                
                ERRbyte = MMPF_SFNAND_PreCopyBack(startsect);
                if (ERRbyte)
                    return ERRbyte;
                
                SFPreparePostCopy = 0x01;
            }
        }
        else if((SFSMCZoneNum - tmp_zone_num) != 0) {
            RTNA_DBG_Str(0, "PCP3\r\n");
            //MMPF_SFNAND_PreCopyBack(startsect);
            ERRbyte = MMPF_SFNAND_PreCopyBack(startsect);
            if (ERRbyte)
                return ERRbyte;  
            
            SFPreparePostCopy = 0x01;
        }

        phy_page_addr = SFWriteInfo.new_phy_blk_addr;
       /* RTNA_DBG_Str(0, "PA:");
        RTNA_DBG_Long(0, phy_page_addr);
        RTNA_DBG_Str(0, "\r\n");
*/
        mem_ptr = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *mem_ptr++ = 0xFFFF;
        //}
        MEMSET(mem_ptr, 0xFF, 64);

		if (!m_gbSFMShortPage) {
        	i = startsect & 3;
        }
        else {
        	i = 0;
        }
        mem_ptr = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR+(i << 4)+2);
        *mem_ptr = SFWriteInfo.log_blk_offset;

        ERRbyte = MMPF_SFNAND_WritePhysicalSector(dmastartaddr, phy_page_addr);
        if (ERRbyte)
            return ERRbyte;
            
        if ((sectcount == 1) && (startsect == 0)) {
            RTNA_DBG_Str(0, "SI:");
            RTNA_DBG_Long(0, *(volatile MMP_ULONG *)dmastartaddr);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, phy_page_addr);
            RTNA_DBG_Str(0, "\r\n");            
        }

        if((SFWriteInfo.new_phy_blk_addr & ubPagePerBlockMask) == ubPagePerBlockMask) {
            SFPreparePostCopy = 0x0;
            SFWriteLogPageAddr = 0xFFFFFFFF;

            if(SFWriteInfo.cur_blk_state == OLD_BLOCK) {
                MMPF_SFNAND_EraseBlock((MMP_ULONG)(((SFSMCZoneNum << 10) + SFWriteInfo.old_phy_blk_offset) << ubBlockSizeShift));
            }
            SFWriteInfo.cur_blk_state = 0;
        }
        else{
            SFWriteLogPageAddr = startsect+1;
            MMPF_SFNAND_GetWriteAddr();
        }

        dmastartaddr += 512;
        if(--sectcount){
          startsect++;
        }
    } // end while (sectcount) 
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SM_LowLevelFormat
*
*
*/
MMP_ERR  MMPF_SFNAND_LowLevelFormat(void)
{

    MMP_ULONG   size;
    MMP_ULONG   loop_cnt;
    MMP_USHORT  i, j, k, l, m;
    MMP_ULONG   rw_page_addr;
    MMP_UBYTE   *src_buf = (MMP_UBYTE *)SNDMAAddr;
    MMP_USHORT  *mem_ptr2;
    MMP_USHORT  bad_block_label= 0;
    MMP_USHORT  log_blk_num = 0;
    MMP_ERR     err;
	MMP_UBYTE   ubPagePerBlock = 0;
	MMP_UBYTE   ubRADataCount = 0;
	MMP_ULONG   ulRWPageField = 0;
	if (!m_gbSFMShortPage) {
		ubPagePerBlock = 8;
		ubRADataCount = 32;
		ulRWPageField = 0xFFFFFF00;
	}
	else {
		ubPagePerBlock = 5;
		ubRADataCount = 8;
		ulRWPageField = 0xFFFFFFE0;

	}
    MMPF_SFNAND_InitialInterface();
    
    if (MMPF_SFNAND_Reset()) {
        return MMP_NAND_ERR_RESET;
    }
    MMPF_SFNAND_GetSize(&size);
    loop_cnt = SFSMCTotalZoneCnt;

    /// Bad Block Management (BBM)
    /// Read RA from each block's 1st page.
    /// If it is a good block, the 1st byte is 0xFF.
    /// Otherwise, this block is a "Bad Block".
    /// (Default BBM from factory)
    for(j = 0;j < loop_cnt; j++) {
        for(i = 0; i < 1024; i++) {
            rw_page_addr = (0 +(((j<<10)+i) << ubPagePerBlock));

            if(MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, rw_page_addr)) {
                DBG_S0("read ra error\r\n");
            }
            src_buf = (MMP_UBYTE *) SF_A8_RA_BUF_ADDR;
            if ((src_buf[0] != 0xFF)) {
                DBG_S0("org block status:");
                DBG_L0(rw_page_addr);
                DBG_S0("!=0xFF");
                DBG_S0("\r\n");
            }
            else {
                
                err = MMPF_SFNAND_EraseBlock((MMP_ULONG)rw_page_addr);
                if(err == MMP_NAND_ERR_ERASE) {
                    // SPI NAND need to modify for write all BLOCK to be 0.
                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                    for(m = 0; m< ubRADataCount; m++){
                        mem_ptr2[m] = 0x0000;
                    }

                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
                        DBG_S0("write ra error\r\n");
                    }
                }
            }
        }
    }
    DBG_S0("add log addr\r\n");

    for(i = 0; i < loop_cnt; i++) {
        log_blk_num = 0;
        for(j = 0; j < 1024; j++) {
            for(k = 0; k < 1; k++) {
                rw_page_addr = (k +(((i<<10)+j)<<ubPagePerBlock));
               /// Due to AIT erase every block before.
               /// All value in block shoule be 0xFF.
               /// Otherwise, this is a runtime bad block.
                if(MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, rw_page_addr)) {
                    DBG_S0("read ra error2:");
                    RTNA_DBG_Long(0, rw_page_addr);
                    DBG_S0("\r\n");

                }

                src_buf = (MMP_UBYTE *)(SF_A8_RA_BUF_ADDR);
                for(l = 0; l < 16; l++){
                    if(src_buf[l] != 0xFF) {
                        bad_block_label = 1;
                        DBG_S0("bad_block_label:");
                        DBG_L0(rw_page_addr);
                        DBG_S0("\r\n");
                        break;
                    }
                }
            }
            /// Prepare RA info for writing (programing)
            if (bad_block_label == 0) {
                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0xFFFF;
                }
                // Add logical block address. 寫0~1000個LOGICAL BLOCK.
                if(log_blk_num <1000){
                    mem_ptr2[1] = log_blk_num;
                }
                else{
                    mem_ptr2[1] = 0xFFFF;
                }
            }
            else{
//                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR + (rw_page_addr& 3)<<4);
                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0x0000;
                }
                //RTNA_DBG_Str3("bad_block_label3:\r\n");
            }
            // 假設寫0XFF 代表可以再寫???
            if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))) {
                DBG_S0("write ra error2:");
                RTNA_DBG_Long(0, rw_page_addr);
                DBG_S0("\r\n");

            }
            if(bad_block_label == 0){
                log_blk_num++;
            }
            bad_block_label = 0;

        }
    RTNA_DBG_Str0(" fast 2k erase OK\r\n");
}
    return MMP_ERR_NONE;
}
#if 1
/** @brief Check bad block and mark the bad block to RA area.
@return NONE
*/
MMP_ERR    MMPF_SFNAND_CheckBadBlock(void)
{
    MMP_ULONG   size;
    MMP_ULONG   loop_cnt;
    MMP_USHORT  i, j, k, l, m;
    MMP_ULONG   rw_page_addr;
    MMP_UBYTE   *src_buf = (MMP_UBYTE *)SNDMAAddr;
    MMP_USHORT  *mem_ptr2;
    MMP_USHORT  bad_block_label= 0;
    MMP_USHORT  log_blk_num = 0;
    MMP_ERR     err = MMP_ERR_NONE;
	MMP_UBYTE   ubPagePerBlock = 0;
	MMP_UBYTE   ubRADataCount = 0;
	MMP_ULONG   ulRWPageField = 0;
	MMP_UBYTE   tempWriteDMA2[512];
	MMP_UBYTE   tempWriteDMA1[512];
	MMP_UBYTE   *tempDMAAddr = (MMP_UBYTE *)m_ulSFMDmaAddr;
	
	RTNA_DBG_Str(0, "Check BAd Block\r\n");
	
	for ( i = 0 ; i < 512; i ++) { // 512 per sector size.
		tempWriteDMA1[i] = (0x55);       // 0x55 is use for taggle the data area. Erase block the data will become FF.
	}
	for ( i = 0 ; i < 512; i ++) { // 512 per sector size.
		tempWriteDMA2[i] = 0xAA;       // 0xAA is use for taggle the data area. 
	}

	if (!m_gbSFMShortPage) {
		ubPagePerBlock = 8;
		ubRADataCount = 32;
		ulRWPageField = 0xFFFFFF00;
	}
	else {
		ubPagePerBlock = 5;
		ubRADataCount = 8;
		ulRWPageField = 0xFFFFFFE0;

	}
    MMPF_SFNAND_InitialInterface();
    if (MMPF_SFNAND_Reset()) {
        return MMP_NAND_ERR_RESET;
    }
    MMPF_SFNAND_GetSize(&size);
    loop_cnt = SFSMCTotalZoneCnt;
    /// Bad Block Management (BBM)
    /// Read RA from each block's 1st page.
    /// If it is a good block, the 1st byte is 0xFF.
    /// Otherwise, this block is a "Bad Block".
    /// (Default BBM from factory)
    for(j = 0;j < loop_cnt; j++) {
        for(i = 0; i < 1024; i++) {
            rw_page_addr = (0 +(((j<<10)+i) << ubPagePerBlock));
            DBG_S0("RWP:");
            DBG_L0(rw_page_addr);   
            DBG_S0("\r\n");     

            if(MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, rw_page_addr)) {
                DBG_S0("read ra error\r\n");
            }
            src_buf = (MMP_UBYTE *) SF_A8_RA_BUF_ADDR;
            if ((src_buf[0] != 0xFF)) {
                DBG_S0("org block status:");
                DBG_L0(rw_page_addr);
                DBG_S0("!=0xFF");
                DBG_L0(src_buf[0]);
                DBG_S0("\r\n");
            }
            else {
            RTNA_DBG_Str(0, "Pass block\r\n");             
                err = MMPF_SFNAND_EraseBlock((MMP_ULONG)rw_page_addr);
                if(err == MMP_NAND_ERR_ERASE) {
                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                    for(m = 0; m< ubRADataCount; m++){
                        mem_ptr2[m] = 0x0000;
                    }

                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
                        DBG_S0("write ra error\r\n");
                    }
                }
                else {
                    for (k = 0 ; k < (1 << ubPagePerBlock); k++) {
	                	// Check Block data area.
	                	//RTNA_DBG_Str(0 , "R 0xFF\r\n");
	                	MMPF_SFNAND_WritePhysicalSector((MMP_ULONG)tempWriteDMA2, (rw_page_addr | k)); 
	                	err = MMPF_SFNAND_ReadPhysicalSector((MMP_ULONG)m_ulSFMDmaAddr, (rw_page_addr | k));
	                	if (err == MMP_NAND_ERR_ECC) {
		                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                    RTNA_DBG_Str(0, "Find Bad Block with 0x0:");	                		
		                    RTNA_DBG_Long(0, rw_page_addr);	                		
		                    RTNA_DBG_Str(0, "\r\n");	                		
							break;
	                	}
	                	else {
	                		tempDMAAddr = (MMP_UBYTE *)m_ulSFMDmaAddr;
	                		for (l = 0; l < 512; l ++) {
	                			if (*tempDMAAddr ++ != tempWriteDMA2[l]) {
				                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
				                    for(m = 0; m< ubRADataCount; m++){
				                        mem_ptr2[m] = 0x0000;
				                    }

				                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
				                        DBG_S0("write ra error\r\n");
				                    }
				                    RTNA_DBG_Str(0, "Find Bad Block with 0x0_1:");	                		
				                    RTNA_DBG_Long(0, rw_page_addr);	                		
				                    RTNA_DBG_Str(0, "\r\n");	                		
									break;
	                			}
	                		}
	                	}
					}
					if (err == MMP_ERR_NONE) {
					    // Erase again for write data 0.
		                err = MMPF_SFNAND_EraseBlock((MMP_ULONG)rw_page_addr);
		                if(err == MMP_NAND_ERR_ERASE) {
		                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                }
						else {
		                    for (k = 0 ; k < (1 << ubPagePerBlock); k++) {
			                	// Check Block data area.
			                	//RTNA_DBG_Str(0 , "W0x0\r\n");
			                	MMPF_SFNAND_WritePhysicalSector((MMP_ULONG)tempWriteDMA1, (rw_page_addr | k)); 
			                	err = MMPF_SFNAND_ReadPhysicalSector((MMP_ULONG)m_ulSFMDmaAddr, (rw_page_addr | k));
			                	if (err == MMP_NAND_ERR_ECC) {
				                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
				                    for(m = 0; m< ubRADataCount; m++){
				                        mem_ptr2[m] = 0x0000;
				                    }

				                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
				                        DBG_S0("write ra error\r\n");
				                    }
				                    RTNA_DBG_Str(0, "Find Bad Block with 1:");	                		
				                    RTNA_DBG_Long(0, rw_page_addr);	                		
				                    RTNA_DBG_Str(0, "\r\n");	                		
									break;
			                	} 
			                	else {
			                		tempDMAAddr = (MMP_UBYTE *)m_ulSFMDmaAddr;
			                		for (l = 0; l < 512; l ++) {
			                			if (*tempDMAAddr ++ != (tempWriteDMA1[l])) {
						                   
						                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
						                    for(m = 0; m< ubRADataCount; m++){
						                        mem_ptr2[m] = 0x0000;
						                    }

						                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
						                        DBG_S0("write ra error\r\n");
						                    }
						                    RTNA_DBG_Str(0, "Find Bad Block with 0x80_1:");	                		
						                    RTNA_DBG_Long(0, rw_page_addr);	                		
						                    RTNA_DBG_Str(0, "\r\n");	                		
											break;
			                			}
			                		}
			                	}			                	
							}						
						}
					}
					// Erase again.
					if (err == MMP_ERR_NONE) {
					    // Erase again for write data 0.
		                err = MMPF_SFNAND_EraseBlock((MMP_ULONG)rw_page_addr);
		                if(err == MMP_NAND_ERR_ERASE) {
		                    mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                }
		            }	                	
                }
            }
        }
    }
    DBG_S0("add log addr\r\n");
    for(i = 0; i < loop_cnt; i++) {
        log_blk_num = 0;
        for(j = 0; j < 1024; j++) {
            for(k = 0; k < 1; k++) {
                rw_page_addr = (k +(((i<<10)+j)<<ubPagePerBlock));
               // Step 1. Check RA area 
               /// Due to AIT erase every block before.
               /// All value in block shoule be 0xFF.
               /// Otherwise, this is a runtime bad block.
                if(MMPF_SFNAND_ReadRA(SF_A8_RA_BUF_ADDR, rw_page_addr)) {
                    DBG_S0("read ra error2\r\n");
                }

                src_buf = (MMP_UBYTE *)(SF_A8_RA_BUF_ADDR);
                for(l = 0; l < 16; l++){
                    if(src_buf[l] != 0xFF) {
                        bad_block_label = 1;
                        DBG_S0("bad_block_label:");
                        DBG_L0(rw_page_addr);
                        DBG_S0("\r\n");
                        break;
                    }
                }
            }
            /// Prepare RA info for writing (programing)
            if (bad_block_label == 0) {
                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0xFFFF;
                }
                // Add logical block address
                if(log_blk_num <1000){
                    mem_ptr2[1] = log_blk_num;
                }
                else{
                    mem_ptr2[1] = 0xFFFF;
                }
            }
            else{
//                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR + (rw_page_addr& 3)<<4);
                mem_ptr2 = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0x0000;
                }
                //RTNA_DBG_Str3("bad_block_label3:\r\n");
            }

            if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField), 0, (ubRADataCount << 1))) {
                DBG_S0("write ra error2\r\n");
            }
            if(bad_block_label == 0){
                log_blk_num++;
            }

            bad_block_label = 0;

        }
    }
    RTNA_DBG_Str0(" fast 2k erase OK\r\n");
    return MMP_ERR_NONE;
    	
}
#endif
/**
*   @brief  MMPF_SFNAND_FinishWritePage
*
*
*/
MMP_ERR MMPF_SFNAND_FinishWritePage(MMP_ULONG ulStartSector)
{
    MMP_ERR   ERRbyte = 0;   
//	RTNA_DBG_Str(0, "MMPF_SFNAND_FinishWritePage\r\n");
    if(SFWriteLogPageAddr != 0xFFFFFFFF) {
        if( (SFWriteLogPageAddr - ulStartSector) == 1){
            SFWriteInfo.old_phy_blk_addr -= 1;
            SFWriteInfo.new_phy_blk_addr -= 1;
        }
        if( (SFWriteInfo.new_phy_blk_addr & PAGE_PER_BLOCK_MASK) != PAGE_PER_BLOCK_MASK){
            ERRbyte = MMPF_SFNAND_PostCopyBack(); 
            if (ERRbyte)
                return ERRbyte;
        }

        SFPreparePostCopy = 0x0;
        SFWriteLogPageAddr = 0xFFFFFFFF;
    }
    return MMP_ERR_NONE;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_ReadPhysicalSector
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_ReadPhysicalSector(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    MMP_UBYTE     status;
    MMP_ULONG     stampstart = 0; 
    MMP_ULONG     stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMDPage = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMDRDCache = {0, 0, 512, NAD_READFROMCAHCE, 3, NAND_READ};
        
    SFCMDRDCache.ulDMAAddr = ulAddr;
    // 1. Page Read to Cache command 13.
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPage.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
    /*RTNA_DBG_Str(0, "A:");
    RTNA_DBG_Long(0, ulPage);
    RTNA_DBG_Str(0, "\r\n");
*/
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPage);   
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_PLL_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);     
        if (ERRbyte)
            return ERRbyte;
          
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 50){
                RTNA_DBG_Str(0, "MMPF_SFNAND_ReadPhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }       
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Read Sector Error by P_FAIL,it's ok\r\n");
        //P_FAIL flag do not affect the read process,so don't return errer at this situation
        //return MMP_NAND_ERR_PROGRAM;
    }
        
    // 2. Write Enable.
    MMPF_SFNAND_GetSectorAddr(ulPage, 0, MMP_TRUE);
    SFCMDRDCache.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;        
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDRDCache);   
    if (ERRbyte)
        return ERRbyte;
    
    // 3. Wait DMA done.(TODO)
    // 4. Check ECC
    ERRbyte = MMPF_SFNAND_GetStatus(&status);    
    if (ERRbyte)
        return ERRbyte;
    
    if (gsSPINandType == 0) {
        if ((status & NAD_STATUS_ECC_MASK) == NAD_STATUS_ECC_Fail) {
            RTNA_DBG_Str(0, "Read Physical failed:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK) == 0) {
                return MMP_ERR_NONE;
            }
            else {
                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }
    else {
        if ((status & NAD_STATUS_ECC_MASK_NEW) == NAD_STATUS_ECC_FAIL_NEW) {
            RTNA_DBG_Str(0, "Read Physical failed new:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, ulPage);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK_NEW) == 0) {
                return MMP_ERR_NONE;
            }
            else {
                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }
    return MMP_ERR_NONE;
}
MMP_BOOL gbFirstWrite = 0;
MMP_ULONG glFirstNandAddr = 0;
MMP_ULONG glCurrentNandAddr = 0;
MMP_BOOL gbContinueWrite = 0;

//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_WritePhysicalSector
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_WritePhysicalSector(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
//    SFNANDCommand SFCMDRead2Cache = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMD = {0, 0, 512, NAD_PROGRAMLOAD, 2, NAND_WRITE};
    SFNANDCommand SFCMDPE = {0, 0, 0, NAD_PROGRAMEXECUTE, 3, NAND_NONE_ACCESS};
    MMP_UBYTE   status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0;    
    MMP_ERR   ERRbyte = 0;   
    //MMP_UBYTE   *ra;
    MMP_USHORT  *ra_short, i;
    MMP_BOOL    bNeedWrite = MMP_FALSE;
    MMP_USHORT  usNeedWriteSize = 0;
    MMP_ULONG   ulTempStartNAND = 0;
    MMP_BOOL    ubNeedCopyData = 0;
    if (gbContinueWrite == 0) {
        ra_short = (MMP_USHORT *)(m_ulSFMDmaAddr + 2048);
        for(i = 0; i< 32; i++){
            ra_short[i] = 0xFFFF;
        }
        glCurrentNandAddr = ulPage;
        glFirstNandAddr = ulPage;
        gbContinueWrite = 1;
        for ( i = 0 ; i < (512 >> 2); i++) {
            *(volatile MMP_ULONG *)(m_ulSFMDmaAddr + ((ulPage & 3) << 9) + i*4) = *(volatile MMP_ULONG *)(ulAddr + i*4);
        }
        if(SFSetMapLabel == 0){
	    	if (!m_gbSFMShortPage) {
	            i = ulPage &3;
		    }
		    else {
		    	i = 0;
		    }
            ra_short = (MMP_USHORT *)(m_ulSFMDmaAddr + 2048 +(i << 4)+0x2);
            *ra_short = SFWriteInfo.log_blk_offset;
        }            
        if ((ulPage & 3) == 3) { // 2K Page
//            RTNA_DBG_Str(0, "NW3\r\n");
            bNeedWrite = MMP_TRUE;
            usNeedWriteSize = 512;
            gbContinueWrite = 0;
            ulTempStartNAND = glFirstNandAddr;             
        }
    }
    else {
        if (ulPage == (glCurrentNandAddr + 1)) { 
            glCurrentNandAddr = ulPage;
            for ( i = 0 ; i < (512 >> 2); i++) {
                *(volatile MMP_ULONG *)(m_ulSFMDmaAddr + ((ulPage & 3) << 9) + i*4) = *(volatile MMP_ULONG *)(ulAddr + i*4);
            }

            if ((ulPage & 3) == 3) { // 2K Page.
//                RTNA_DBG_Str(0, "NW1\r\n");
                bNeedWrite = MMP_TRUE;
                usNeedWriteSize = ((ulPage & 3) - (glFirstNandAddr & 3) + 1) << 9;
                gbContinueWrite = 0;
                ulTempStartNAND = glFirstNandAddr; 
            }
            if(SFSetMapLabel == 0){
    	    	if (!m_gbSFMShortPage) {
    	            i = ulPage &3;
    		    }
    		    else {
    		    	i = 0;
    		    }
                ra_short = (MMP_USHORT *)(m_ulSFMDmaAddr + 2048 +(i << 4)+0x2);
                *ra_short = SFWriteInfo.log_blk_offset;
            }            
        }
        else {
//            RTNA_DBG_Str(0, "NW2\r\n");
            bNeedWrite = MMP_TRUE;
            usNeedWriteSize = ((glCurrentNandAddr & 3) - (glFirstNandAddr & 3) + 1) << 9; 
            ulTempStartNAND = glFirstNandAddr;            
            glCurrentNandAddr = ulPage;
            glFirstNandAddr = ulPage;            
            gbContinueWrite = 1;
            ubNeedCopyData = 1;
        }
    }
    if (!bNeedWrite) {
        return MMP_ERR_NONE;
    }
    RTNA_DBG_Str(0, "WP0:");
    RTNA_DBG_Short(0, usNeedWriteSize + 64);
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, ulTempStartNAND);
    if ((ulTempStartNAND & 0x3E900) == 0x3E900) {
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, *(volatile MMP_ULONG *)(m_ulSFMDmaAddr + 2048));
    RTNA_DBG_Str(0, ":");
    RTNA_DBG_Long(0, *(volatile MMP_ULONG *)(m_ulSFMDmaAddr + 2052));

    }
    RTNA_DBG_Str(0, "\r\n");

    SFCMD.ulDMAAddr = m_ulSFMDmaAddr;//ulAddr;
    SFCMD.usDMASize = usNeedWriteSize + 64;
    // 1. Program Load
    MMPF_SFNAND_GetSectorAddr(ulTempStartNAND, 0, MMP_FALSE);
    SFCMD.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;  
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);    
    if (ERRbyte)
        return ERRbyte;
    
    // ?: Wait DMA Done
    // 2. Write Enable
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_TRUE);      
    if (ERRbyte)
        return ERRbyte;
    
    // 3. Program execute.
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPE.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR; 
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPE);     
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_PLL_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);      
        if (ERRbyte)
            return ERRbyte;
        
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_SFNAND_WritePhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }        
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Write Sector Error\r\n");
        return MMP_NAND_ERR_PROGRAM;
    }
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_FALSE);    
    if (ERRbyte)
        return ERRbyte;
     
    if (ubNeedCopyData) {
        for ( i = 0 ; i < (512 >> 2); i++) {
            *(volatile MMP_ULONG *)(m_ulSFMDmaAddr + ((ulPage & 3) << 9) + i*4) = *(volatile MMP_ULONG *)(ulAddr + i*4);
        }
    }
    #if 0
    /* // SPINAND no need to use ECC. NAND Device internal support
	RTNA_DBG_Str(0, "pNAD->NAD_ECC0:");
	RTNA_DBG_Byte(0, pNAD->NAD_ECC0);
	RTNA_DBG_Str(0, "\r\n");
	RTNA_DBG_Str(0, "pNAD->NAD_ECC1:");
	RTNA_DBG_Byte(0, pNAD->NAD_ECC1);
	RTNA_DBG_Str(0, "\r\n");
	RTNA_DBG_Str(0, "pNAD->NAD_ECC2:");
	RTNA_DBG_Byte(0, pNAD->NAD_ECC2);
	RTNA_DBG_Str(0, "\r\n");
    */
    ra_short = (MMP_USHORT *)SF_A8_RA_BUF_ADDR;
    for(i = 0; i< 32; i++){
        ra_short[i] = 0xFFFF;
    }

    if(SFSetMapLabel == 0){
		if (!m_gbSFMShortPage) {
	        i = ulPage &3;
		}
		else {
			i = 0;
		}
        ra_short = (MMP_USHORT *)(SF_A8_RA_BUF_ADDR+(i << 4)+0x2);
        *ra_short = SFWriteInfo.log_blk_offset;
        /*RTNA_DBG_Str(0, "RA:");
        RTNA_DBG_Short(0, SFWriteInfo.log_blk_offset);
        RTNA_DBG_Str(0, "\r\n");
*/
    }
	if (!m_gbSFMShortPage) {
    	i = ulPage &3;
	}
	else {
		i = 0;
	}
    ra = (MMP_UBYTE *)SF_A8_RA_BUF_ADDR;
	#if 0 // SPI NAND no need to use ECC.
    *(ra+(i << 4)+13) = pNAD->NAD_ECC0;
    *(ra+(i << 4)+14) = pNAD->NAD_ECC1;
    *(ra+(i << 4)+15) = pNAD->NAD_ECC2;
    *(ra+(i << 4)+8)  = pNAD->NAD_ECC3;
    *(ra+(i << 4)+9)  = pNAD->NAD_ECC4;
    *(ra+(i << 4)+10) = pNAD->NAD_ECC5;
    #endif

    if(SFSetMapLabel == 1){
        *(ra+(i << 4)+1) =  0x4D;
        *(ra+(i << 4)+5) =  0x41;
    }
/*
    RTNA_DBG_Str(0, "WF:");
    RTNA_DBG_Short(0, ulPage);
    RTNA_DBG_Str(0, "\r\n");
*/
    if(MMPF_SFNAND_WriteRA(SF_A8_RA_BUF_ADDR, ulPage, 0 + (i << 4), 16)){//(i << 4)+0x2, 2)) {
        RTNA_DBG_Long(3, ulPage);
        RTNA_DBG_Str(1, "write RA fail\r\n");
    }
    #endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_ReadPhysicalPage
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_ReadPhysicalPage(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    MMP_UBYTE     status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMDPage = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMDRDCache = {0, 0, 2112, NAD_READFROMCAHCE, 3, NAND_READ};
    SFCMDRDCache.ulDMAAddr = ulAddr;
    // 1. Page Read to Cache command 13.
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPage.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
    /*RTNA_DBG_Str(0, "A:");
    RTNA_DBG_Long(0, ulPage);
    RTNA_DBG_Str(0, "\r\n");
*/
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPage);   
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_PLL_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);      
        if (ERRbyte)
            return ERRbyte;
         
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 50){
                RTNA_DBG_Str(0, "MMPF_SFNAND_ReadPhysicalPage Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        } 
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Read Page Error by P_FAIL,it's ok\r\n");
        //P_FAIL flag do not affect the read process,so don't return errer at this situation
        //return MMP_NAND_ERR_PROGRAM;
    }
        
    // 2. Write Enable.
    MMPF_SFNAND_GetSectorAddr(ulPage, 0, MMP_TRUE);
    SFCMDRDCache.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;        
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDRDCache);   
    if (ERRbyte)
        return ERRbyte;
    
    // 3. Wait DMA done.(TODO)
    // 4. Check ECC
    ERRbyte = MMPF_SFNAND_GetStatus(&status);    
    if (ERRbyte)
        return ERRbyte;
    
    if (gsSPINandType == 0) {
        if ((status & NAD_STATUS_ECC_MASK) == NAD_STATUS_ECC_Fail) {
            RTNA_DBG_Str(0, "Read Physical failed:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK) == 0) {
            RTNA_DBG_Str(0, "RPP0:");    
            RTNA_DBG_Long(0, ulPage); 
            RTNA_DBG_Str(0, ":");    
            RTNA_DBG_Long(0, *(MMP_ULONG *)(ulAddr + 2048));
            RTNA_DBG_Str(0, ":");    
            RTNA_DBG_Long(0, ulAddr);     
            RTNA_DBG_Str(0, "\r\n");

                return MMP_ERR_NONE;
            }
            else {
            RTNA_DBG_Str(0, "RPP1:");    
            RTNA_DBG_Long(0, ulPage); 
            RTNA_DBG_Str(0, ":");    
            RTNA_DBG_Long(0, *(MMP_ULONG *)(ulAddr + 2048));
            RTNA_DBG_Str(0, ":");    
            RTNA_DBG_Long(0, ulAddr);     
            RTNA_DBG_Str(0, "\r\n");

                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }
    else {
        if ((status & NAD_STATUS_ECC_MASK_NEW) == NAD_STATUS_ECC_FAIL_NEW) {
            RTNA_DBG_Str(0, "Read Physical failed new:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, ulPage);          
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK_NEW) == 0) {
                return MMP_ERR_NONE;
            }
            else {
                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }
//    RTNA_DBG_Str(0, "Done");    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_WritePhysicalPage
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_WritePhysicalPage(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
//    SFNANDCommand SFCMDRead2Cache = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMD = {0, 0, 2112, NAD_PROGRAMLOAD, 2, NAND_WRITE};
    SFNANDCommand SFCMDPE = {0, 0, 0, NAD_PROGRAMEXECUTE, 3, NAND_NONE_ACCESS};
    MMP_UBYTE   status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    
    SFCMD.ulDMAAddr = ulAddr;
    // 1. Program Load
    MMPF_SFNAND_GetSectorAddr(ulPage, 0, MMP_FALSE);
    SFCMD.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;  
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);    
    if (ERRbyte)
        return ERRbyte;
    
    // ?: Wait DMA Done
    // 2. Write Enable
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_TRUE);   
    if (ERRbyte)
        return ERRbyte;
     
    // 3. Program execute.
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPE.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR; 
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPE);   
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_PLL_WaitCount(2);
    stampstart = OSTimeGet(); 
    RTNA_DBG_Str(0, "WPP:");    
    RTNA_DBG_Long(0, ulPage); 
    RTNA_DBG_Str(0, ":");    
    RTNA_DBG_Long(0, *(MMP_ULONG *)(ulAddr + 2048));
    RTNA_DBG_Str(0, ":");    
    RTNA_DBG_Long(0, ulAddr);     
    RTNA_DBG_Str(0, "\r\n");       
    
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);      
        if (ERRbyte)
            return ERRbyte;
         
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_SFNAND_WritePhysicalPage Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Write Sector Error\r\n");
        return MMP_NAND_ERR_PROGRAM;
    }
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_FALSE);    
    if (ERRbyte)
            return ERRbyte;
     
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_WriteRA
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_WriteRA(MMP_ULONG ulAddr, MMP_ULONG ulPage, MMP_ULONG ulOffset, MMP_UBYTE ubcount)
{
    MMP_UBYTE status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMD = {0, 0, 2, NAD_PROGRAMLOAD, /*NAD_PROGRAMLOAD*/ 2, NAND_WRITE};
    SFNANDCommand SFCMDPE = {0, 0, 0, NAD_PROGRAMEXECUTE, 3, NAND_NONE_ACCESS};
    SFCMD.usDMASize = ubcount;//ubcount;
    SFCMD.ulDMAAddr = ulAddr + ulOffset;
    MMPF_SFNAND_GetSectorAddr(ulPage, MMP_TRUE, MMP_FALSE);
    SFCMD.ulNandAddr = *(MMP_ULONG *)(A8_INFO_ADDR) + ulOffset;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);    
    if (ERRbyte)
        return ERRbyte;
    
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_TRUE);     
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPE.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR; 
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPE);   
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_SFNAND_WaitCount(2);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);    
        if (ERRbyte)
            return ERRbyte;
           
        //if (!(status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_SFNAND_WriteRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }
    }
    if (status & NAD_STATUS_PFAIL) {
        RTNA_DBG_Str(0, "Write RA Error\r\n");
        return MMP_NAND_ERR_PROGRAM;
    }
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_FALSE);   
    if (ERRbyte)
        return ERRbyte;
        
    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_EraseBlock
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_EraseBlock(MMP_ULONG ulStartPage)

{
    SFNANDCommand SFCMD = {0, 0, 0x00, NAD_BLOCKERASE, 3, MMP_FALSE};
    MMP_UBYTE   nand_status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    #if 0
	RTNA_DBG_Str(0, "MMPF_SFNAND_EraseBlock:");
	RTNA_DBG_Long(0, ulStartPage);
	RTNA_DBG_Str(0, "\r\n");
	#endif    
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_TRUE);   
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_SFNAND_GetPageAddr(ulStartPage);
    SFCMD.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMD);     
    if (ERRbyte)
        return ERRbyte;
    
    /* Check success or not */
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&nand_status);   
        if (ERRbyte)
            return ERRbyte;
        
        //if (!(nand_status & NAD_STATUS_OIP)) {
            //break;
        //}
        
        if( !(nand_status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 200){
                RTNA_DBG_Str(0, "MMPF_SFNAND_EraseBlock Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        } 
    }
    ERRbyte = MMPF_SFNAND_GetStatus(&nand_status);   
    if (ERRbyte)
        return ERRbyte;
    
    if (nand_status & NAD_STATUS_EFAIL) {
        RTNA_DBG_Str(0, "SF Nand Erase Fail:");
        RTNA_DBG_Byte(0, nand_status);
        RTNA_DBG_Str(0, "\r\n");
    }

    /* Disable SMC */
    ERRbyte = MMPF_SFNAND_WriteEn(MMP_FALSE); 
    if (ERRbyte)
        return ERRbyte;
    
    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_SFNAND_ReadRA
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SFNAND_ReadRA(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    MMP_UBYTE     status;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    MMP_ERR   ERRbyte = 0;   
    SFNANDCommand SFCMDPage = {0, 0, 0, NAD_PAGEREAD, 3, NAND_NONE_ACCESS};
    SFNANDCommand SFCMDRDCache = {0, 0, 64, NAD_READFROMCAHCE, 3, NAND_READ};

    SFCMDRDCache.ulDMAAddr = ulAddr;
    // 1. Page Read to Cache command 13.
    MMPF_SFNAND_GetPageAddr(ulPage);
    SFCMDPage.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR;        
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDPage);    
    if (ERRbyte)
        return ERRbyte;
    
    MMPF_SFNAND_WaitCount(10);
    stampstart = OSTimeGet(); 
    while(1) {
        ERRbyte = MMPF_SFNAND_GetStatus(&status);    
        if (ERRbyte)
            return ERRbyte;
        
        //if (!(status & NAD_STATUS_OIP))
            //break;
        
        if( !(status & NAD_STATUS_OIP) )
            break;
        else {
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 50){
                RTNA_DBG_Str(0, "MMPF_SFNAND_ReadRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;
            }        
        }     
            
    }
    // 2. Read Data from Cache.
    MMPF_SFNAND_GetSectorAddr(ulPage, MMP_TRUE, MMP_TRUE);
    SFCMDRDCache.ulNandAddr = *(MMP_ULONG *)A8_INFO_ADDR; 
    ERRbyte = MMPF_SFNAND_SendCommand(&SFCMDRDCache);    
    if (ERRbyte)
        return ERRbyte;
    
    // 3. Wait DMA done.(TODO)
    // 4. Check ECC
    ERRbyte = MMPF_SFNAND_GetStatus(&status);   
    if (ERRbyte)
        return ERRbyte;
    
    if (gsSPINandType == 0) {
        if ((status & NAD_STATUS_ECC_MASK) == NAD_STATUS_ECC_Fail) {
            RTNA_DBG_Str(0, "Read Physical failed:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Byte(0, ulPage);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK) == 0) {
                return MMP_ERR_NONE;
            }
            else {
                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }
    else {
        if ((status & NAD_STATUS_ECC_MASK_NEW) == NAD_STATUS_ECC_FAIL_NEW) {
            RTNA_DBG_Str(0, "Read Physical failed new:");
            RTNA_DBG_Byte(0, status);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, ulPage);
            RTNA_DBG_Str(0, "\r\n");
            return MMP_NAND_ERR_ECC;
        }
        else  {
            if ((status & NAD_STATUS_ECC_MASK_NEW) == 0) {
                return MMP_ERR_NONE;
            }
            else {
                return MMP_NAND_ERR_ECC_CORRECTABLE;
            }
        }
    }


    return MMP_ERR_NONE;
}
#pragma

/** @brief Set temp buffer address for SN driver

@param[in] ulStartAddr Start address for temp buffer address.
@param[in] ulSize Size for temp buffer. Currently buffer size is only 64 Byte for SDHC message handshake
@return NONE
*/
void    MMPF_SFNAND_SetTmpAddr(MMP_ULONG ulStartAddr, MMP_ULONG ulSize)
{
    m_ulSFMDmaAddr = ulStartAddr;
    ulSize = ulSize; /* dummy for prevent compile warning */
    RTNA_DBG_Str(0, "sn dma addr: ");
    RTNA_DBG_Long(0, ulStartAddr);
    RTNA_DBG_Str(0, "\r\n");
}

/** @brief Get temp buffer address for SM driver
@param[out] ulStartAddr Start address for temp buffer address.
@return NONE
*/
void    MMPF_SFNAND_GetTmpAddr(MMP_ULONG *ulStartAddr)
{
    *ulStartAddr = m_ulSFMDmaAddr;
}
#endif //#if (USING_SN_IF)

#endif //#if (CHIP == MCR_V2))

/** @} */ // MMPF_SM
