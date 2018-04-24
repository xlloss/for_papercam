#include "includes_fw.h"
#include "lib_retina.h"
#include "mmpf_nand.h"
#include "lib_retina.h"
#include "mmp_reg_gpio.h"
#include "mmp_reg_nand.h"
#include "mmp_reg_pad.h"
#include "mmp_reg_gbl.h"

/** @addtogroup MMPF_NAND
@{
*/
#if (USING_SM_IF) && (SM_2K_PAGE)
extern void MMPF_MMU_FlushDCacheMVA(MMP_ULONG ulRegion, MMP_ULONG ulSize);
#pragma O0
//==============================================================================
//
//                              Define
//
//==============================================================================
#define _LOGADDRESS         0
#define _PHYADDRESS         0

#define FULL_ZONE           1
#define TWO_ZONE            0
#define ONE_ZONE            0

#define SM_CPU_NONBLOCKING  1

#pragma O2
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
MMPF_OS_SEMID nandBusySemID = 0;
MMP_ULONG   SMC_MAP_DATA_BUF;         // size = 2048 bytes
MMP_ULONG   SMC_BLK_ADDR_BUF;         // size = 2048 bytes
MMP_ULONG   NAND_IO_DATA_ADDR;        // size = 8 bytes
MMP_ULONG   A8_RA_BUF_ADDR;           // size = 64 bytes
MMP_ULONG   A8_COPY_BACK_ADDR;        // size = 512 bytes
volatile    SMCMAPWRITEINFO WriteInfo;
MMP_UBYTE   Zone0BuildCnt = 0;
MMP_USHORT  SMCTotalZoneCnt = 0;
MMP_USHORT  SMMAPInfoBlkNum;
MMP_USHORT  CurrMapTabNum = 0;
MMP_USHORT  *SMCMapTabBuf;
MMP_USHORT  *SMCBlkAddrBuf;
volatile    MMP_USHORT  SMCZoneNum = 0xFFFF;
volatile    MMP_ULONG   WriteLogPageAddr = 0xFFFFFFFF;
volatile    MMP_UBYTE   PreparePostCopy = 0;
MMP_USHORT  PwrOnFirst = 1;



MMP_UBYTE   SMAddr[5];
MMP_UBYTE   SMAddrCyc = 4;
MMP_USHORT  SetMapLabel = 0;
MMP_ULONG   SMTmpAddr = 0;
MMP_ULONG   m_ulSMDmaAddr;
MMP_UBYTE   bSMShortPage = 0;

MMPF_OS_SEMID  SMDMASemID;


//==============================================================================
//
//                              External
//
//==============================================================================
extern  MMP_ULONG SMDMAAddr;

//==============================================================================
//
//                              FUNCTION Body
//
//==============================================================================

/**
*   @brief  MMPF_NAND_WaitInterruptDone
*
*
*/
MMP_ERR MMPF_NAND_WaitInterruptDone(void)
{
    if (MMPF_OS_AcquireSem(SMDMASemID, 0)) {
        RTNA_DBG_Str(3, "SM OSSemPend: Fail\r\n");
        return MMP_NAND_ERR_HW_INT_TO;
    }
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_WaitCount
*
*
*/
void MMPF_NAND_WaitCount(MMP_ULONG count)
{
    MMP_ULONG i;

    for (i = 0; i < count; i++);
}
/**
*   @brief  MMPF_NAND_ISR
*
*
*/
void    MMPF_NAND_ISR(void)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_USHORT  intsrc;
    #if (SM_CPU_NONBLOCKING)
    MMP_UBYTE ret;
    #endif

    intsrc = pNAD->NAD_CPU_INT_SR & (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE);

    pNAD->NAD_CPU_INT_EN = 0;
    pNAD->NAD_CPU_INT_SR = intsrc; // write 1 to clear previous status

    #if (SM_CPU_NONBLOCKING)
    if (intsrc & (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE) /*0xA0*/ ) {
        ret = MMPF_OS_ReleaseSem(SMDMASemID);
        if (ret) {
            RTNA_DBG_Str(3, "SM OSSemPost: Fail\r\n");
        }
    }
    #endif
}
/**
*   @brief  MMPF_NAND_SetMemory
*
*
*/
MMP_ERR  MMPF_NAND_SetMemory(MMP_ULONG ulStartAddr)
{
    #if 0
    RTNA_DBG_Str1("NAND Logical->Physical Use Address:");
    RTNA_DBG_Long1(ulStartAddr);
    RTNA_DBG_Str1("\r\n");
    #endif

    if (ulStartAddr == 0) {
        return MMP_NAND_ERR_PARAMETER;
    }

#if (FULL_ZONE)
    A8_COPY_BACK_ADDR = ulStartAddr;
    A8_RA_BUF_ADDR = A8_COPY_BACK_ADDR + 0x200;
    SMC_BLK_ADDR_BUF = A8_RA_BUF_ADDR + 0x40;
    SMC_MAP_DATA_BUF = SMC_BLK_ADDR_BUF + 0x800;
#endif
	MMPF_SM_SetTmpAddr(SMC_MAP_DATA_BUF + 0x800, 512);
	
    PreparePostCopy = 0;
    WriteLogPageAddr = 0xFFFFFFFF;
    SMCZoneNum = 0xFFFF;

    CurrMapTabNum = 0;
    Zone0BuildCnt = 0;
    SMCMapTabBuf = (MMP_USHORT *)SMC_MAP_DATA_BUF;
    SMCBlkAddrBuf = (MMP_USHORT *)SMC_BLK_ADDR_BUF;

    if (ulStartAddr != SMTmpAddr && SMTmpAddr != 0) {
        MMPF_NAND_BuildAllMapTable();
    }
    SMTmpAddr = ulStartAddr;

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_FinishWrite
*
*
*/
MMP_ERR    MMPF_NAND_FinishWrite(void)
{
    MMP_ERR   ERRbyte = 0;   

	#if !defined(MBOOT_FW)
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_AcquireSem(nandBusySemID, 0)) {
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif
	#endif
    if(WriteLogPageAddr != 0xFFFFFFFF) {
        ERRbyte = MMPF_NAND_FinishWritePage(WriteLogPageAddr-1);   
        if (ERRbyte)
            return ERRbyte;
    }
    #if !defined(MBOOT_FW)
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_ReleaseSem(nandBusySemID)) {
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif
	#endif
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SM_MemoryAlloc
*
*
*/
MMP_USHORT MMPF_SM_MemoryAlloc(void)
{

    MMP_USHORT  size = 0;
#if (FULL_ZONE)
    size = 2*2048+40+512+512; // Last 512 is for temp buffer;
#endif
    return size;
}
/**
*   @brief  MMPF_NAND_Enable
*
*
*/
MMP_ERR    MMPF_NAND_Enable(MMP_BOOL bEnable, MMP_BOOL bWProtect)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;

    if (bEnable) {
        if(bWProtect) {
            pNAD->NAD_CTL = NAD_CE2 | NAD_EN;
        }
        else {
            pNAD->NAD_CTL = NAD_CE2 | NAD_WPN | NAD_EN;
        }
    }
    else {
        pNAD->NAD_CTL = NAD_CE1 | NAD_CE2;
    }
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_GetPageAddr
*
*
*/
void    MMPF_NAND_GetPageAddr(MMP_ULONG address, MMP_USHORT spare)
{
	if (!bSMShortPage) {
	    MMP_USHORT tmp_addr;

	    tmp_addr = (address >> 2);
	    SMAddr[4] = (tmp_addr & 0xFF0000) >> 16;
	    SMAddr[3] = (tmp_addr & 0xFF00) >> 8;
	    SMAddr[2] = (tmp_addr & 0xFF);

	    if(spare == 0) {
	        SMAddr[1] = 2 *(address & 3);
	    }
	    else{
	        SMAddr[1] = 8;
	    }
	    SMAddr[0] = 0;
	}
	else {
		SMAddr[2] = (address & 0xFF00) >> 8;
		SMAddr[1] = (address & 0xFF);
		SMAddr[0] = 0;
	}
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
*   @brief  MMPF_NAND_GetStatus
*
*
*/
MMP_ERR  MMPF_NAND_GetStatus(MMP_UBYTE *status)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;

    MMPF_NAND_SendCommand(NAD_READ_STATUS);

    *status = pNAD->NAD_CAD;

    return MMP_ERR_NONE;
}


/**
*   @brief  MMPF_SM_InitialInterface
*
*
*/
MMP_ERR MMPF_NAND_InitialInterface(void)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    AITPS_AIC   pAIC = AITC_BASE_AIC;
    AITPS_PAD   pPAD = AITC_BASE_PAD;
    AITPS_GBL   pGBL = AITC_BASE_GBL;
    AITPS_GPIO  pGPIO = AITC_BASE_GPIO;

    RTNA_DBG_Str0("Init SM\r\n");

    pPAD->PAD_IO_CFG_PBGPIO[6] = 0x05; // modified from pPAD->PAD_GPIO[16] = 0x14;  //pull up R/B pin

    pGBL->GBL_CLK_EN[0] |= GBL_CLK_SM; //modified from pGBL->GBL_CLK_EN |= GBL_CLK_SM;
    pGBL->GBL_MISC_IO_CFG |= GBL_NAND_PAD_EN; // modified from pGBL->GBL_MIO_SM_CTL |= GBL_SM_GPIO_EN;

    pNAD->NAD_DMA_CTL = NAD_GPIO_EN;
    pNAD->NAD_TIMING = NAD_RCV_CYC(0) | NAD_CMD_CYC(3);

    if(PwrOnFirst) {
        PwrOnFirst = 0;
        SMDMASemID = MMPF_OS_CreateSem(0);
		#if !defined(MBOOT_FW)    
	    #if (SM_BUS_REENTRY_PROTECT == 1)
		nandBusySemID = MMPF_OS_CreateSem(1);
    	#endif
    	#endif
        pNAD->NAD_CPU_INT_EN = 0;

        RTNA_AIC_Open(pAIC, AIC_SRC_SM, nand_isr_a, AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 2);
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_SM);

        pGBL->GBL_CLK_EN[1] |= GBL_CLK_GPIO; //modified from pGBL->GBL_CLK_EN |= GBL_CLK_GPIO;
        pGPIO->GPIO_OUT_EN[0] &= ~(1<<16);
        pGPIO->GPIO_INT_CPU_EN[0] &= ~(1<<16);
        
        //modified from pGPIO->GPIO_INT_CPU_SR[0] |= (1<<16);
        pGPIO->GPIO_INT_H2L_SR[0] |= (1<<16);
        pGPIO->GPIO_INT_L2H_SR[0] |= (1<<16); 
        pGPIO->GPIO_INT_H_SR[0] |= (1<<16); 
        pGPIO->GPIO_INT_L_SR[0] |= (1<<16);        
        
        pGPIO->GPIO_INT_L2H_EN[0] |= (1<<16);
        pGPIO->GPIO_INT_H2L_EN[0] &= ~(1<<16);
        
        #if (!IMPLEMENT_FLM)
        RTNA_AIC_Open(pAIC, AIC_SRC_GPIO, gpio_isr_a,AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_GPIO);
        #endif

        MMPF_NAND_SetMemory(SMTmpAddr);
        MMPF_NAND_BuildAllMapTable();
    }

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_SendCommand
*
*
*/
MMP_ERR MMPF_NAND_SendCommand(MMP_UBYTE ubCommand)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;

    pNAD->NAD_CTL |= NAD_CLE;
    pNAD->NAD_CAD = ubCommand;
    pNAD->NAD_CTL &= ~NAD_CLE;

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_Reset
*
*
*/
MMP_ERR MMPF_NAND_Reset(void)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_ULONG   size;
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
//	RTNA_DBG_Str(0, "MMPF_NAND_Reset\r\n");
	
    MMPF_NAND_Enable(MMP_TRUE, MMP_TRUE);

    MMPF_NAND_SendCommand(NAD_CMD_RESET);
    MMPF_NAND_WaitCount(30);    
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB)); 
	while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_Reset Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }
	
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);
    MMPF_NAND_GetSize(&size);
//	RTNA_DBG_Str0("size:");
//	RTNA_DBG_Long0(size);
//	RTNA_DBG_Str0("\r\n");
    if (size == 0)
        return MMP_NAND_ERR_RESET;

//	RTNA_DBG_Str(0, "NAnd REset OK\r\n");
    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_SendAddress
*/
MMP_ERR MMPF_NAND_SendAddress(MMP_UBYTE *ubAddr, MMP_USHORT usPhase)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_ULONG   i;
    pNAD->NAD_CTL |= NAD_ALE;
    for (i = 0; i < usPhase; i++)
        pNAD->NAD_CAD = ubAddr[i];

    pNAD->NAD_CTL &= ~NAD_ALE;

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_GetSize
*
*
*/
MMP_ERR MMPF_NAND_GetSize(MMP_ULONG *pSize)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_UBYTE   makerID, deviceID;
    MMP_UBYTE   null;
	MMP_UBYTE   Byte4 = 0;
	MMP_UBYTE   Byte5 = 0;
	MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 

    MMPF_NAND_Enable(MMP_TRUE, MMP_TRUE);
    MMPF_NAND_SendCommand(NAD_READ_ID);

    SMAddr[0] = 0x00;
    MMPF_NAND_SendAddress(SMAddr, 1);

    MMPF_NAND_WaitCount(30);
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));   
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_GetSize Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }   

    makerID = pNAD->NAD_CAD;
    deviceID = pNAD->NAD_CAD;
    null = pNAD->NAD_CAD;
    Byte4 = pNAD->NAD_CAD;
    Byte5 = pNAD->NAD_CAD;    
	if (!bSMShortPage) {
	    switch(deviceID) {
	    case 0xF1:
	    case 0xD1:    // add for ESMT Parallel Nand Flash 1Gb
	        *pSize = (1*1000*128);
	        SMCTotalZoneCnt = 1;
	        SMAddrCyc = 4;
	        break;
	    case 0xDC:
	        *pSize = (1*1000*256);
	        SMCTotalZoneCnt = 1;
	        SMAddrCyc = 5;
	        break;        
	    default:
	        *pSize = 0;
	        SMCTotalZoneCnt = 0;
	        SMAddrCyc = 0;
	        break;
	    }
	}
	else {
    	switch(deviceID) {
        case 0x73: *pSize = (1*1000*32);
                    SMCTotalZoneCnt = 1;
                    SMAddrCyc = 3;
            break;

        case 0x75: *pSize = (2*1000*32);
                    SMCTotalZoneCnt = 2;
            break;

        case 0x76: *pSize = (4*1000*32);
                    SMCTotalZoneCnt = 4;
            break;

        case 0x79: *pSize = (8*1000*32);
                    SMCTotalZoneCnt = 8;
            break;

        default: *pSize = 0;
                    SMCTotalZoneCnt = 0;
            break;
    	}	
    	SMAddrCyc = 3;
	}
#if 1
    RTNA_DBG_Str(0, "makerID : deviceID = ");
    RTNA_DBG_Byte(0, makerID);
    RTNA_DBG_Str(0, " : ");
    RTNA_DBG_Byte(0, deviceID);
    RTNA_DBG_Str(0, "\r\n");
    RTNA_DBG_Str(0, "size = ");
    RTNA_DBG_Long(0, *pSize);
    RTNA_DBG_Str(0, "\r\n");
#endif
    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_ECCDecode
*
*
*/
MMP_ERR MMPF_NAND_ECCDecode(MMP_UBYTE* p_buf, MMP_UBYTE* p_ecc, MMP_UBYTE* ecc_data)
{
  MMP_UBYTE offset, corrected;
  MMP_ERR   status;

  if ((p_ecc[0] != ecc_data[0]) || (p_ecc[1] != ecc_data[1]) || (p_ecc[2] != ecc_data[2])) {
      status = MMPF_NAND_ECCCorrect(p_buf, p_ecc, ecc_data, &offset, &corrected);
      if (status == MMP_NAND_ERR_ECC_CORRECTABLE) {
          *((MMP_UBYTE*)p_buf + offset) = corrected;
          return MMP_NAND_ERR_ECC_CORRECTABLE;
      }
      else if (status == MMP_NAND_ERR_ECC)
          return MMP_NAND_ERR_ECC;
  }

  return MMP_ERR_NONE;
}

/**
*   @brief  MMPF_NAND_ECCCorrect
*
*
*/
MMP_ERR MMPF_NAND_ECCCorrect(MMP_UBYTE *p_data, MMP_UBYTE *ecc1, MMP_UBYTE *ecc2, MMP_UBYTE *p_offset, MMP_UBYTE *p_corrected)
{
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

  return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_GetMapAddress
*
*
*/
MMP_ULONG MMPF_NAND_GetMapAddress(MMP_ULONG ulLogicalAddr)
{
    MMP_USHORT page_offset;
    MMP_USHORT zone_num =0;
    MMP_USHORT blk_offset = 0;
    short blk_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT usBlockSize = 0;
	MMP_USHORT ubPagePerBlockMask = 0;
	if (!bSMShortPage) {
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

    if((zone_num != SMCZoneNum)) {
#if (FULL_ZONE)
        MMPF_NAND_BuildMapTab(zone_num, SMCBlkAddrBuf, SMCMapTabBuf);
#endif
    }

  return  ((*(MMP_USHORT *)(SMCMapTabBuf + blk_offset) + (SMCZoneNum << 10)) << ubBlockSizeShift) + page_offset;
}

/**
*   @brief  MMPF_NAND_ReadSector
*
*
*/
MMP_ERR MMPF_NAND_ReadSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount)
{
    MMP_ULONG phy_page_addr;
    MMP_USHORT tmp_zone_num = 0xFFFF;
    MMP_USHORT *tmp_sm_map_tab_buf;
    MMP_UBYTE  ERRbyte = 0;

	#if !defined(MBOOT_FW)    
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_AcquireSem(nandBusySemID, 0)) {
    	RTNA_DBG_Str(0, "NAND ACQ FAILED!!!!!!!!");
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif
	#endif
    #if 0
    RTNA_DBG_Str0("Read: startsect : sectcount: ");
    RTNA_DBG_Long0(dmastartaddr);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(startsect);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(sectcount);
    RTNA_DBG_Str0("\r\n");
    #endif
/*
	if (startsect == 0) {
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
    if(WriteLogPageAddr != 0xFFFFFFFF) {
        tmp_zone_num = SMCZoneNum;
        tmp_sm_map_tab_buf = SMCMapTabBuf;
    }

    /// @todo need to add async write state in read operation
    while (sectcount) {
        if(((startsect & 0xFFFFFF00) == (WriteLogPageAddr & 0xFFFFFF00))
                && (startsect >= WriteLogPageAddr)) {

            if(WriteLogPageAddr != 0xFFFFFFFF) {
                #if (_LOGADDRESS)
                RTNA_DBG_Str3("readsector finish:");
                RTNA_DBG_Str3("need to add async write state in read operation");
                RTNA_DBG_Str3("\r\n");
                #endif
                ERRbyte = MMPF_NAND_FinishWritePage(WriteLogPageAddr - 1);   
                if (ERRbyte)
                    return ERRbyte;
            }
        }
        phy_page_addr = MMPF_NAND_GetMapAddress(startsect);
        ERRbyte = MMPF_NAND_ReadPhysicalSector(dmastartaddr, phy_page_addr);
        if(ERRbyte)
            return ERRbyte;

        sectcount--;
        dmastartaddr += 512;
        startsect++;
    }

    if(tmp_zone_num != 0xFFFF) {
        SMCZoneNum = tmp_zone_num;
        SMCMapTabBuf = tmp_sm_map_tab_buf;
    }
	#if !defined(MBOOT_FW)    
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_ReleaseSem(nandBusySemID)) {
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif
	#endif
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_BuildOneMapTable
*
*
*/
void    MMPF_NAND_BuildOneMapTable(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf, MMP_USHORT *smcmaptabbuf)
{
    MMP_USHORT *smcblkaddrbuf1 = (MMP_USHORT *)SMC_BLK_ADDR_BUF;

    MMPF_NAND_BuildMapTab(zone_num, smcblkaddrbuf1, smcmaptabbuf);
    // smcblkaddrbuf1 physcial->logical map table
//    MMPF_NAND_GetAllBlkAddr(zone_num, smcblkaddrbuf1);
    // smcmaptabbuf logical to physical map table
//    MMPF_NAND_GetMapTab(smcblkaddrbuf1, smcmaptabbuf);
}
/**
*   @brief  MMPF_NAND_BuildAllMapTable
*
*
*/
MMP_USHORT MMPF_NAND_BuildAllMapTable(void)
{
    MMP_ULONG   size;
    MMP_LONG    i;

#if (FULL_ZONE)
    MMPF_NAND_GetSize(&size);
    for(i = (SMCTotalZoneCnt - 1); i >= 0; i--) {
        MMPF_NAND_BuildOneMapTable(i, SMCBlkAddrBuf, SMCMapTabBuf);
    }
#endif
    return MMP_ERR_NONE;
}


/**
*   @brief  MMPF_NAND_GetAllBlkAddr
*
*
*/
MMP_UBYTE MMPF_NAND_GetAllBlkAddr(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf)
{
    MMP_USHORT  map_blk_num;
    MMP_USHORT  free_blk_num, bad_blk_num;
    MMP_UBYTE  *read_ra_buf2;
    MMP_ULONG   start_zone_addr;
    MMP_USHORT  i;
    MMP_USHORT  blk_status;
    MMP_USHORT  *i3;
    MMP_ERR     ERRbyte = 0;   
//	MMP_UBYTE kk = 0;
    SMCZoneNum = zone_num;
	if (!bSMShortPage) {
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
        ERRbyte = MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, start_zone_addr);   
        if (ERRbyte)
            RTNA_DBG_Str(0, "MMPF_NAND_ReadRA Error.\r\n"); 
        
/*
		RTNA_DBG_Str(0, "ReadRa:");
		RTNA_DBG_Long(0, start_zone_addr);
		RTNA_DBG_Str(0, "\r\n");
		{
			MMP_USHORT *RData = (MMP_USHORT *)A8_RA_BUF_ADDR;
			RTNA_DBG_Str(0, "RAData:"); 
			for (kk = 0; kk < 8; kk++) {
				RTNA_DBG_Short(0, *RData++);
				RTNA_DBG_Str(0, ":"); 
			}
			RTNA_DBG_Str(0, "\r\n"); 

		}
*/ 
        read_ra_buf2 = (MMP_UBYTE *)A8_RA_BUF_ADDR;

        /* Check MAP! label */
        if((read_ra_buf2[1] == 0x4D) && (read_ra_buf2[5] == 0x41)) {
        	//RTNA_DBG_Str(0, "ER ??\r\n");

            ERRbyte = MMPF_NAND_EraseBlock(start_zone_addr); 
            if (ERRbyte)
                RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Error.\r\n"); 
                        
            RTNA_DBG_Str(1, "erase reserve blk\r\n");
        }

        if(read_ra_buf2[0] == 0xFF) {
            if( ( read_ra_buf2[2] == 0xFF) && ( read_ra_buf2[3] == 0xFF)) {
                blk_status = FREE_BLOCK;
		       	//RTNA_DBG_Str(0, "Free ??\r\n"); 				
            }
            else{
               blk_status = (MMP_USHORT)read_ra_buf2[2] + ((MMP_USHORT)read_ra_buf2[3] <<8) ;
	        	/*
	        	RTNA_DBG_Str(0, "blk_status:");
	        	RTNA_DBG_Long(0, blk_status);
	        	RTNA_DBG_Str(0, "\r\n");
				*/
            }
        }

        /* map block bad block or reserved block */
        else{
        	RTNA_DBG_Str(0, "Bad ??:");
        	RTNA_DBG_Short(0, i);
        	RTNA_DBG_Str(0, "\r\n");
            blk_status = REV_BAD_BLOCK;
            bad_blk_num++;
        }

        *i3++ = blk_status;

        if( (blk_status - FREE_BLOCK) == 0 ){
            free_blk_num++;
        }
        if (!bSMShortPage) {
       		start_zone_addr += 256;
    	}
    	else {
    		start_zone_addr += 32;
    	}
    }

    RTNA_DBG_PrintShort(3, free_blk_num);
    RTNA_DBG_PrintShort(3, bad_blk_num);

    return 0;

}

/**
*   @brief  MMPF_NAND_BuildMapTab
*/
void    MMPF_NAND_BuildMapTab(MMP_USHORT zone_num, MMP_USHORT *smcblkaddrbuf, MMP_USHORT *smcmaptabbuf)
{
	RTNA_DBG_Str(0, "BuildMap\r\n");
#if (FULL_ZONE)
    if(zone_num == 0) {
        SMCMapTabBuf = (MMP_USHORT *)(SMC_MAP_DATA_BUF + (zone_num<<11));
        SMCZoneNum = 0;
    }
#endif
	MMPF_NAND_GetAllBlkAddr(zone_num, smcblkaddrbuf);
	MMPF_NAND_GetMapTab(smcblkaddrbuf, smcmaptabbuf);
    WriteInfo.free_blk_idx = SMCMapTabBuf + (MAX_BLOCK_NUM - 1);
}

/**
*   @brief  MMPF_NAND_GetLogBlk
*
*
*/
MMP_UBYTE  MMPF_NAND_GetLogBlk(MMP_ULONG ulPageAddr)
{
    MMP_USHORT blk_num, zone_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	if (!bSMShortPage) {
		ubBlockSizeShift = 8;
	}
	else {
		ubBlockSizeShift = 5;
	}
    blk_num = ulPageAddr >> ubBlockSizeShift;

    zone_num = blk_num / MAX_BLOCK_NUM;
    WriteInfo.log_blk_offset = blk_num - zone_num*MAX_BLOCK_NUM;

    return(zone_num);
}

/**
*   @brief  MMPF_NAND_GetMapTab
*
*
*/
void MMPF_NAND_GetMapTab(MMP_USHORT * smcblkaddrbuf, MMP_USHORT * smcmaptabbuf)
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
	MMP_ERR    ERRbyte = 0;   
	if (!bSMShortPage) {
		ubBlockSizeShift = 8;
		ubBlockSize = 0xFF;
	}
	else {
		ubBlockSizeShift = 5;
		ubBlockSize = 0x1F;
	}
    i1 = SMCMapTabBuf;
    for(i = 0;i < 1024; i++){
        *i1++ = NULL_BLK_ENTRY;
    }

    i1 = smcblkaddrbuf;
    i2 = SMCMapTabBuf + MAX_BLOCK_NUM;

    pend_addr_free_blk = SMCMapTabBuf + ZONE_SIZE;
    for(i = 0; i<ZONE_SIZE; i++){
        log_addr_state = *i1++;
        if( ( log_addr_state - FREE_BLOCK) >= 0){
            #if 0
            RTNA_DBG_Str(0, "F:");
            RTNA_DBG_Long(0, log_addr_state);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, blk_idx);
            RTNA_DBG_Str(0, "\r\n");
            #endif
            if( (log_addr_state - (MMP_USHORT)FREE_BLOCK) == 0){
                if( (i2 - pend_addr_free_blk) < 0){             
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
            i3 = log_addr_state + SMCMapTabBuf;
            #if 0
            RTNA_DBG_Str(0, "N:");
            RTNA_DBG_Long(0, log_addr_state);
            RTNA_DBG_Str(0, ":");
            RTNA_DBG_Long(0, blk_idx);
            RTNA_DBG_Str(0, "\r\n");
			#endif

#if 0
                *i3++ = blk_idx;
#else
            if( *i3 == NULL_BLK_ENTRY){
                *i3++ = blk_idx;
            }
            else{
                phy_block_addr =  ( ( (SMCZoneNum <<10) + *i3)<<ubBlockSizeShift) + ubBlockSize;
                ERRbyte = MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, phy_block_addr); 
                if (ERRbyte)
                    RTNA_DBG_Str(0, "MMPF_NAND_ReadRA Error.\r\n"); 
                
                read_ra_buf = (MMP_UBYTE *)A8_RA_BUF_ADDR;
                if( ( (MMP_USHORT)read_ra_buf[2] + ( (MMP_USHORT)read_ra_buf[3] <<8)) == log_addr_state){
                    RTNA_DBG_Str0("=========duplicated phy addr=========\r\n");
                    phy_block_addr =  ( ( (SMCZoneNum <<10) + blk_idx)<<ubBlockSizeShift) + ubBlockSize;
                    ERRbyte = MMPF_NAND_EraseBlock(phy_block_addr); 
                    if (ERRbyte)
                        RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Error.\r\n"); 
                }
                else{
                    RTNA_DBG_Str0("*********duplicated phy addr**********\r\n");
                    phy_block_addr =  ( ( (SMCZoneNum <<10) + *i3)<<ubBlockSizeShift) + ubBlockSize;
                    ERRbyte = MMPF_NAND_EraseBlock(phy_block_addr); 
                    if (ERRbyte)
                        RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Error.\r\n"); 
                        
                    *i3++ = blk_idx;
                }

            }

            blk_idx++;
#endif
       }
    }



}
/**
*   @brief  MMPF_NAND_GetWriteAddr
*
*
*/
void MMPF_NAND_GetWriteAddr(void)
{
    WriteInfo.old_phy_blk_addr += 1;
    WriteInfo.new_phy_blk_addr += 1;
}
/**
*   @brief  MMPF_NAND_CopyBack
*
*
*/
MMP_ERR MMPF_NAND_CopyBack(MMP_ULONG  ulSrcPage, MMP_ULONG ulDstPage)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_UBYTE   nand_status;
    MMP_USHORT  i;
    MMP_ERR   ERRbyte = 0;   
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    
	if (!bSMShortPage) {
	    stampstart = OSTimeGet(); 
        //while(!(pNAD->NAD_CTL & NAD_RB));   
        while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                    }
            }
        }      

        MMPF_NAND_Enable(MMP_TRUE, MMP_FALSE);

        pNAD->NAD_CMD_1 = NAD_COPYBACK_READ_1;
        pNAD->NAD_CMD_2 = NAD_COPYBACK_READ_2;
        MMPF_NAND_GetPageAddr(ulSrcPage, 0);
        for (i = 0; i < 4; i++) {
            pNAD->NAD_ADDR[i] = SMAddr[i];
        }
        pNAD->NAD_DMA_CTL = NAD_GPIO_EN;

        pNAD->NAD_ADDR_CTL = SMAddrCyc - 1; 
        pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_CMD2;

        pNAD->NAD_RDN_CTL = 0;

#if (SM_CPU_NONBLOCKING)
        pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
        pNAD->NAD_CPU_INT_EN = NAD_AUTO_ADDR_DONE;
#endif

        pNAD->NAD_EXC_CTL = NAD_EXC_ST;

#if (SM_CPU_NONBLOCKING)
        ERRbyte = MMPF_NAND_WaitInterruptDone();   	
        if (ERRbyte)
            return ERRbyte;        
        	    
#else
        stampstart = OSTimeGet(); 
        //while(!(pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE));    
        while(1){
            if( (pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }  

        pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status			
#endif
        pNAD->NAD_CMD_1 = NAD_COPYBACK_PRG_1;
        pNAD->NAD_CMD_2 = NAD_COPYBACK_PRG_2;
        MMPF_NAND_GetPageAddr(ulDstPage, 0);
        for (i = 0; i < 4; i++) {
            pNAD->NAD_ADDR[i] = SMAddr[i];
        }
        pNAD->NAD_DMA_CTL = NAD_GPIO_EN;

        pNAD->NAD_ADDR_CTL = SMAddrCyc - 1; 
        pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_CMD2;

        pNAD->NAD_RDN_CTL = 0;

#if (SM_CPU_NONBLOCKING)
        pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
        pNAD->NAD_CPU_INT_EN = NAD_AUTO_ADDR_DONE;
#endif

        pNAD->NAD_EXC_CTL = NAD_EXC_ST;

#if (SM_CPU_NONBLOCKING)
        ERRbyte = MMPF_NAND_WaitInterruptDone();   	
        if (ERRbyte)
            return ERRbyte;         
        	    
#else
        stampstart = OSTimeGet(); 
        //while(!(pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE));   
        while(1){
            if( (pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        } 

        pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status			
#endif
		
        MMPF_NAND_GetStatus(&nand_status);
        if(nand_status & PROGRAM_FAIL){
            RTNA_DBG_Str(1, "copyback RA fail\r\n");
            return  MMP_NAND_ERR_PROGRAM;
        }
        MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);
	}
	else {
	    stampstart = OSTimeGet(); 
	    //while(!(pNAD->NAD_CTL & NAD_RB));      
	    while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }	    

	    MMPF_NAND_Enable(MMP_TRUE, MMP_FALSE);

	    MMPF_NAND_SendCommand(NAD_COPYBACK_READ_1);

	    MMPF_NAND_GetPageAddr(ulSrcPage, 0);
	    MMPF_NAND_SendAddress(SMAddr, SMAddrCyc);
	    MMPF_NAND_WaitCount(10);
	    stampstart = OSTimeGet(); 
	    //while(!(pNAD->NAD_CTL & NAD_RB));      
	    while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }	    
	    
	    MMPF_NAND_SendCommand(NAD_COPYBACK_PRG_1_512);

	    MMPF_NAND_GetPageAddr(ulDstPage, 0);
	    MMPF_NAND_SendAddress(SMAddr, SMAddrCyc);

	    MMPF_NAND_WaitCount(10);
	    stampstart = OSTimeGet(); 
    	//while(!(pNAD->NAD_CTL & NAD_RB)); 
    	while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_CopyBack Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }
    	
    	
    	
    	
    	
    	
    	
	
	    MMPF_NAND_GetStatus(&nand_status);
	    if(nand_status & PROGRAM_FAIL){
	        RTNA_DBG_Str(1, "copyback RA fail\r\n");
	        return  MMP_NAND_ERR_PROGRAM;
	    }
	    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);		
	}
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_PreCopyBack
*
*
*/
MMP_ERR MMPF_NAND_PreCopyBack(MMP_ULONG ulPageAddr)
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
	MMP_ERR   ERRbyte = 0;   
	#if 0
	RTNA_DBG_Str(0, "PRC:");
	RTNA_DBG_Long(0, ulPageAddr);
	RTNA_DBG_Str(0, "\r\n");
	#endif
	if (!bSMShortPage) {
		ubBlockSizeShift = 8;
		usBlockSize = 256;
	}
	else {
		ubBlockSizeShift = 5;
		usBlockSize = 32;
	}
    WriteInfo.loop_cntr = ulPageAddr & (usBlockSize - 1);
	
    tmp_zone_num = MMPF_NAND_GetLogBlk(ulPageAddr);
    if((SMCZoneNum - tmp_zone_num) != 0){
        RTNA_DBG_Str(1, "wtmp_zone_num:");
        RTNA_DBG_Long(1, tmp_zone_num);
        RTNA_DBG_Str(1, ":");
        RTNA_DBG_Long(1, SMCZoneNum);
        RTNA_DBG_Str(1, "\r\n");
        MMPF_NAND_BuildMapTab(tmp_zone_num, SMCBlkAddrBuf, SMCMapTabBuf);
    }


    while(rebuild_free_cntr){
        i1 = SMCMapTabBuf + MAX_BLOCK_NUM;
        for(i = 0; i < MAX_FREE_BLK_NUM; i++){
            WriteInfo.free_blk_idx = i1;
            tmp_new_phy_block = *i1++;
            if((tmp_new_phy_block-NULL_BLK_ENTRY) != 0) {
                result = 0;
                break;
            }
        }
        if((tmp_new_phy_block - NULL_BLK_ENTRY) == 0) {
            RTNA_DBG_Str(1, "NULL_BLK_ENTRY\r\n");
            MMPF_NAND_BuildMapTab(tmp_zone_num , SMCBlkAddrBuf, SMCMapTabBuf);
            result = 1;
        }
        if(result == 0){
            break;
        }
        rebuild_free_cntr -= 1;
    }
    WriteInfo.new_phy_blk_offset = tmp_new_phy_block;
    WriteInfo.new_phy_blk_addr = ((SMCZoneNum << 10) + WriteInfo.new_phy_blk_offset)<<ubBlockSizeShift;

    i1 = WriteInfo.log_blk_offset + SMCMapTabBuf;
    tmp_old_phy_block = *i1;
    WriteInfo.old_phy_blk_offset = tmp_old_phy_block;

    *i1 = WriteInfo.new_phy_blk_offset;

    i1 = WriteInfo.free_blk_idx;
    if((tmp_old_phy_block - NULL_BLK_ENTRY) != 0){
        *i1 = tmp_old_phy_block;
        WriteInfo.old_phy_blk_addr = ((SMCZoneNum <<10) + WriteInfo.old_phy_blk_offset) << ubBlockSizeShift;
        WriteInfo.cur_blk_state = OLD_BLOCK;
    }
    else{
        *i1 = NULL_BLK_ENTRY;
        WriteInfo.cur_blk_state = NEW_BLOCK;
    }
    #if 0
	RTNA_DBG_Str(0, "blk_offset:");
	RTNA_DBG_Long(0, WriteInfo.old_phy_blk_addr);
	RTNA_DBG_Str(0, ":");
	RTNA_DBG_Long(0, WriteInfo.new_phy_blk_addr);
	RTNA_DBG_Str(0, "\r\n");
	#endif
    if(WriteInfo.loop_cntr == 0){
        return MMP_ERR_NONE;
    }
	if (!bSMShortPage) {
	    //while(MMPF_NAND_SwapPages1());
	    while(1){
            ERRbyte = MMPF_NAND_SwapPages1();   
            if (ERRbyte)
                return ERRbyte;
            else
                break;    
	    }
    }
    else {
    	MMPF_NAND_SwapPagesShort();
    }

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_PostCopyBack
*
*
*/
MMP_ERR MMPF_NAND_PostCopyBack(void)
{
    MMP_USHORT tmp_loop_cntr;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT usBlockSize = 0;
	MMP_ERR   ERRbyte = 0;   
	// RTNA_DBG_Str(0, "POC\r\n");
	
	if (!bSMShortPage) {
		ubBlockSizeShift = 8;
		usBlockSize = 256;
	}
	else {
		ubBlockSizeShift = 5;
		usBlockSize = 32;
	}    


    tmp_loop_cntr = (WriteInfo.new_phy_blk_addr & (usBlockSize - 1) );

    if((WriteInfo.loop_cntr = (usBlockSize-1) - tmp_loop_cntr) != 0){
        MMPF_NAND_GetWriteAddr();
    	if (!bSMShortPage) {
        	//while(MMPF_NAND_SwapPages2());
            while(1){
                ERRbyte = MMPF_NAND_SwapPages2();   
                if (ERRbyte)
                    return ERRbyte;
                else
                    break;    
	        }        	
        }
        else {
        	MMPF_NAND_SwapPagesShort();
        }
    }

    if(WriteInfo.cur_blk_state == OLD_BLOCK){
        ERRbyte =MMPF_NAND_EraseBlock((MMP_ULONG)(((SMCZoneNum << 10) + WriteInfo.old_phy_blk_offset) << ubBlockSizeShift));   
        if (ERRbyte)
            return ERRbyte;
            //RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Error.\r\n");     
    }

    WriteInfo.cur_blk_state = 0;

    return  MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_SwapBigPages
*
*
*/
MMP_ERR MMPF_NAND_SwapBigPages(void)
{
    //MMP_USHORT  i;
    MMP_USHORT  *p_ra_buf;
    MMP_ERR   ERRbyte = 0;   


    if( !( (WriteInfo.loop_cntr>>2) & 0x3F) ){
        return MMP_ERR_NONE;
    }

    if(WriteInfo.cur_blk_state == OLD_BLOCK){
#if 1   // Has problem SUNNY_050816
        ERRbyte = MMPF_NAND_CopyBack(WriteInfo.old_phy_blk_addr, WriteInfo.new_phy_blk_addr);   
        if (ERRbyte)
            return ERRbyte;
        
#else
        for(i = 0; i < 4; i++){
            status = MMPF_NAND_ReadPhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.old_phy_blk_addr+i);      
            if(status)
                return status;
            
            status = MMPF_NAND_WritePhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.new_phy_blk_addr+i);
            if(status)
                return status;
        }
#endif
    }
    else {
#ifdef  LOG_READ_WRITE_TEST
ait81x_uart_write("New block only writeRAbig:\r\n");
#endif
        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *p_ra_buf++ = 0xFFFF;
        //}
        MEMSET(p_ra_buf, 0xFF, 64);
        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
        *(p_ra_buf + ((WriteInfo.new_phy_blk_addr & 3)<<3)+ 1) = WriteInfo.log_blk_offset;

        MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, WriteInfo.new_phy_blk_addr);
    }
    // Not necessary SUNNY_050812
//    phy_page_addr = WriteInfo.new_phy_blk_addr;

    /* move to here for non-proper irq */
    WriteInfo.old_phy_blk_addr += 4;
    WriteInfo.new_phy_blk_addr += 4;
    WriteInfo.loop_cntr -= 4;

    // replac by writera and copyback SUNNY_050812
//    if(MMPF_NAND_WritePhysicalSector(A8_COPY_BACK_ADDR, phy_page_addr)){
//      return SM_NOT_COMPLETE;
//    }

    return MMP_NAND_ERR_NOT_COMPLETE;
}
/**
*   @brief  MMPF_NAND_SwapSmallPages
*
*
*/
MMP_ERR  MMPF_NAND_SwapSmallPages(void)
{
    //MMP_USHORT i;
    MMP_USHORT * p_ra_buf;
    MMP_ERR status;

    if(!(WriteInfo.loop_cntr & 3) ){
        return MMP_ERR_NONE;
    }

    if(WriteInfo.cur_blk_state == OLD_BLOCK){
        status = MMPF_NAND_ReadPhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.old_phy_blk_addr); 
        if (status)
            return status;
        
        status = MMPF_NAND_WritePhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.new_phy_blk_addr);
        if (status)
            return status;

    }
    else {
#ifdef  LOG_READ_WRITE_TEST
ait81x_uart_write("New block only writeRAsmall:\r\n");
#endif

        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *p_ra_buf++ = 0xFFFF;
        //}
        MEMSET(p_ra_buf, 0xFF, 64);

        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
        
        // For 2K NAND

       	*(p_ra_buf + ((WriteInfo.new_phy_blk_addr & 3)<<3) + 1) = WriteInfo.log_blk_offset;

        MMPF_NAND_WriteRA( A8_RA_BUF_ADDR, WriteInfo.new_phy_blk_addr);
    }

    // Not necessary SUNNY_050812
//    phy_page_addr = WriteInfo.new_phy_blk_addr;

    /* move to here for non-proper irq */
    MMPF_NAND_GetWriteAddr();
    WriteInfo.loop_cntr -= 1;

    // Not necessary SUNNY_050812
//    if(MMPF_NAND_WritePhysicalSector(A8_COPY_BACK_ADDR, phy_page_addr)){
//        return SM_NOT_COMPLETE;
//    }

    return MMP_NAND_ERR_NOT_COMPLETE;
}
/**
*   @brief  MMPF_NAND_SwapPages1
*
*
*/
MMP_ERR MMPF_NAND_SwapPages1(void)
{
    MMP_ERR err;

    do{
        err = MMPF_NAND_SwapBigPages();
        if(err) {
            if(err != MMP_NAND_ERR_NOT_COMPLETE) {
                return err;
            }
        }
    } while (err == MMP_NAND_ERR_NOT_COMPLETE);

    do{
        err = MMPF_NAND_SwapSmallPages();
        if(err) {
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_SwapPages2
*
*
*/
MMP_ERR MMPF_NAND_SwapPages2(void)
{
    MMP_ERR err;

    do{
        err = MMPF_NAND_SwapSmallPages();
        if(err){
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    do{
        err = MMPF_NAND_SwapBigPages();
        if(err){
            if(err != MMP_NAND_ERR_NOT_COMPLETE){
                return err;
            }
        }
    } while(err == MMP_NAND_ERR_NOT_COMPLETE);

    return MMP_ERR_NONE;
}

MMP_ERR MMPF_NAND_SwapPagesShort(void) 
{
    //MMP_USHORT i;
    MMP_USHORT * p_ra_buf;
    MMP_ERR status;
    MMP_USHORT k, j;
    k = WriteInfo.loop_cntr;
	for(j = 0; j < k; j++) {
	    if(WriteInfo.cur_blk_state == OLD_BLOCK){
			//RTNA_DBG_Str(0, "S old\r\n");

	        status = MMPF_NAND_ReadPhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.old_phy_blk_addr);  
	        if (status)
	            return status;
	        
	        status = MMPF_NAND_WritePhysicalSector(A8_COPY_BACK_ADDR, WriteInfo.new_phy_blk_addr);
	        if (status)
	            return status;

	    }
	    else {
	#ifdef  LOG_READ_WRITE_TEST
	ait81x_uart_write("New block only writeRAsmall:\r\n");
	#endif
			//RTNA_DBG_Str(0, "S new\r\n");
	        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
	        //for(i = 0; i< 32; i++){
	        //    *p_ra_buf++ = 0xFFFF;
	        //}
	        MEMSET(p_ra_buf, 0xFF, 64);

	        p_ra_buf = (MMP_USHORT *)A8_RA_BUF_ADDR;
	        
	        // For 2K NAND
	        if (!bSMShortPage) {
	        	*(p_ra_buf + ((WriteInfo.new_phy_blk_addr & 3)<<3) + 1) = WriteInfo.log_blk_offset;
	        }
	        else {
	        	// For 512 NAND
	        	*(p_ra_buf + 1) = WriteInfo.log_blk_offset;
			}
	        status = MMPF_NAND_WriteRA( A8_RA_BUF_ADDR, WriteInfo.new_phy_blk_addr);   
	        if (status)
	            return status; 
	    }

	    // Not necessary SUNNY_050812
	//    phy_page_addr = WriteInfo.new_phy_blk_addr;

	    /* move to here for non-proper irq */
	    MMPF_NAND_GetWriteAddr();
	    WriteInfo.loop_cntr -= 1;
	}
	return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_NAND_WriteSector
*
*
*/
MMP_ERR MMPF_NAND_WriteSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount)
{
    MMP_USHORT *mem_ptr;
    MMP_ULONG   phy_page_addr;
    MMP_USHORT i;
    MMP_USHORT tmp_zone_num;
	MMP_UBYTE  ubBlockSizeShift = 0;
	MMP_USHORT ubPagePerBlockMask = 0;
	MMP_UBYTE  ERRbyte = 0;

	#if !defined(MBOOT_FW)
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_AcquireSem(nandBusySemID, 0)) {
    	RTNA_DBG_Str(0, "NAND ACQ FAILED!!!!!!!!");
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif
	#endif	
	if (!bSMShortPage) {
		ubBlockSizeShift = 8;
		ubPagePerBlockMask = 0xFF;
	}
	else {
		ubBlockSizeShift = 5;
		ubPagePerBlockMask = 0x1F;
	} 
    #if 0
    RTNA_DBG_Str0("Write: startsect : sectcount: ");
    RTNA_DBG_Long0(dmastartaddr);
    RTNA_DBG_Str0(" ");
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
        tmp_zone_num = MMPF_NAND_GetLogBlk(startsect);

        if((startsect != WriteLogPageAddr)){
            if(PreparePostCopy == 0) {
              ERRbyte = MMPF_NAND_PreCopyBack(startsect);   
              if (ERRbyte)
                  return ERRbyte;
              
              PreparePostCopy = 0x01;
            }
            else {
                WriteInfo.old_phy_blk_addr -= 1;
                WriteInfo.new_phy_blk_addr -= 1;
                ERRbyte = MMPF_NAND_PostCopyBack();   
                if (ERRbyte)
                    return ERRbyte;

                ERRbyte = MMPF_NAND_PreCopyBack(startsect);   
                if (ERRbyte)
                    return ERRbyte;
                
                PreparePostCopy = 0x01;
            }
        }
        else if((SMCZoneNum - tmp_zone_num) != 0) {
            ERRbyte = MMPF_NAND_PreCopyBack(startsect);   
            if (ERRbyte)
                return ERRbyte;
            
            PreparePostCopy = 0x01;
        }

        phy_page_addr = WriteInfo.new_phy_blk_addr;

        mem_ptr = (MMP_USHORT *)A8_RA_BUF_ADDR;
        //for(i = 0; i< 32; i++){
        //    *mem_ptr++ = 0xFFFF;
        //}
        MEMSET(mem_ptr, 0xFF, 64);

		if (!bSMShortPage) {
        	i = startsect & 3;
        }
        else {
        	i = 0;
        }
        mem_ptr = (MMP_USHORT *)(A8_RA_BUF_ADDR+(i << 4)+2);
        *mem_ptr = WriteInfo.log_blk_offset;

        ERRbyte = MMPF_NAND_WritePhysicalSector(dmastartaddr, phy_page_addr);
        if (ERRbyte)
            return ERRbyte;

        if((WriteInfo.new_phy_blk_addr & ubPagePerBlockMask) == ubPagePerBlockMask) {
            PreparePostCopy = 0x0;
            WriteLogPageAddr = 0xFFFFFFFF;

            if(WriteInfo.cur_blk_state == OLD_BLOCK) {
                ERRbyte = MMPF_NAND_EraseBlock((MMP_ULONG)(((SMCZoneNum << 10) + WriteInfo.old_phy_blk_offset) << ubBlockSizeShift));  
                if (ERRbyte)
                    RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Error.\r\n");   
                
            }
            WriteInfo.cur_blk_state = 0;
        }
        else{
            WriteLogPageAddr = startsect+1;
            MMPF_NAND_GetWriteAddr();
        }

        dmastartaddr += 512;
        if(--sectcount){
          startsect++;
        }
    }
	#if !defined(MBOOT_FW)
	#if (SM_BUS_REENTRY_PROTECT == 1)
    if (MMP_ERR_NONE != MMPF_OS_ReleaseSem(nandBusySemID)) {
        return MMP_NAND_ERR_NOT_COMPLETE;
    }
	#endif	
	#endif
    return MMP_ERR_NONE;
}
/**
*   @brief  MMPF_SM_LowLevelFormat
*
*
*/
MMP_ERR  MMPF_NAND_LowLevelFormat(void)
{

    MMP_ULONG   size;
    MMP_ULONG   loop_cnt;
    MMP_USHORT  i, j, k, l, m;
    MMP_ULONG   rw_page_addr;
    MMP_UBYTE   *src_buf = (MMP_UBYTE *)SMDMAAddr;
    MMP_USHORT  *mem_ptr2;
    MMP_USHORT  bad_block_label= 0;
    MMP_USHORT  log_blk_num = 0;
    MMP_ERR     err;
	MMP_UBYTE   ubPagePerBlock = 0;
	MMP_UBYTE   ubRADataCount = 0;
	MMP_ULONG   ulRWPageField = 0;
	if (!bSMShortPage) {
		ubPagePerBlock = 8;
		ubRADataCount = 32;
		ulRWPageField = 0xFFFFFF00;
	}
	else {
		ubPagePerBlock = 5;
		ubRADataCount = 8;
		ulRWPageField = 0xFFFFFFE0;

	}
    MMPF_NAND_InitialInterface();
    if (MMPF_NAND_Reset()) {
        return MMP_NAND_ERR_RESET;
    }
    MMPF_NAND_GetSize(&size);
    loop_cnt = SMCTotalZoneCnt;

#if 1
    /// Bad Block Management (BBM)
    /// Read RA from each block's 1st page.
    /// If it is a good block, the 1st byte is 0xFF.
    /// Otherwise, this block is a "Bad Block".
    /// (Default BBM from factory)
    for(j = 0;j < loop_cnt; j++) {
        for(i = 0; i < 1024; i++) {
            rw_page_addr = (0 +(((j<<10)+i) << ubPagePerBlock));

            if(MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, rw_page_addr)) {
                DBG_S0("read ra error\r\n");
            }
            src_buf = (MMP_UBYTE *) A8_RA_BUF_ADDR;
            if ((src_buf[0] != 0xFF)) {
                DBG_S0("org block status:");
                DBG_L0(rw_page_addr);
                DBG_S0("!=0xFF");
                DBG_S0("\r\n");
            }
            else {
                err = MMPF_NAND_EraseBlock((MMP_ULONG)rw_page_addr);
                //if(err == MMP_NAND_ERR_ERASE) {
                if(err != 0) {
                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
                    for(m = 0; m< ubRADataCount; m++){
                        mem_ptr2[m] = 0x0000;
                    }

                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
                        DBG_S0("write ra error\r\n");
                    }
                }
            }
        }
    }
#endif
    DBG_S0("add log addr\r\n");

#if 1
    for(i = 0; i < loop_cnt; i++) {
        log_blk_num = 0;
        for(j = 0; j < 1024; j++) {
            for(k = 0; k < 1; k++) {
                rw_page_addr = (k +(((i<<10)+j)<<ubPagePerBlock));
#if 1
               /// Due to AIT erase every block before.
               /// All value in block shoule be 0xFF.
               /// Otherwise, this is a runtime bad block.
                if(MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, rw_page_addr)) {
                    DBG_S0("read ra error2\r\n");
                }

                src_buf = (MMP_UBYTE *)(A8_RA_BUF_ADDR);
                for(l = 0; l < 16; l++){
                    if(src_buf[l] != 0xFF) {
                        bad_block_label = 1;
                        DBG_S0("bad_block_label:");
                        DBG_L0(rw_page_addr);
                        DBG_S0("\r\n");
                        break;
                    }
                }
#endif

            }
#if 1
            /// Prepare RA info for writing (programing)
            if (bad_block_label == 0) {
                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
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
//                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR + (rw_page_addr& 3)<<4);
                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0x0000;
                }
                //RTNA_DBG_Str3("bad_block_label3:\r\n");
            }

            if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))) {
                DBG_S0("write ra error2\r\n");
            }
            if(bad_block_label == 0){
                log_blk_num++;
            }
#endif

            bad_block_label = 0;

        }
#endif
    RTNA_DBG_Str3(" fast 2k erase OK\r\n");
}
    return MMP_ERR_NONE;
}
#if 1
/** @brief Check bad block and mark the bad block to RA area.
@return NONE
*/
MMP_ERR    MMPF_NAND_CheckBadBlock(void)
{
    MMP_ULONG   size;
    MMP_ULONG   loop_cnt;
    MMP_USHORT  i, j, k, l, m;
    MMP_ULONG   rw_page_addr;
    MMP_UBYTE   *src_buf = (MMP_UBYTE *)SMDMAAddr;
    MMP_USHORT  *mem_ptr2;
    MMP_USHORT  bad_block_label= 0;
    MMP_USHORT  log_blk_num = 0;
    MMP_ERR     err = MMP_ERR_NONE;
	MMP_UBYTE   ubPagePerBlock = 0;
	MMP_UBYTE   ubRADataCount = 0;
	MMP_ULONG   ulRWPageField = 0;
	MMP_UBYTE   tempWriteDMA2[512];
	MMP_UBYTE   tempWriteDMA1[512];
	MMP_UBYTE   *tempDMAAddr = (MMP_UBYTE *)m_ulSMDmaAddr;
	
	RTNA_DBG_Str(0, "Check BAd Block\r\n");
	
	for ( i = 0 ; i < 512; i ++) { // 512 per sector size.
		tempWriteDMA1[i] = (0x55);       // 0x55 is use for taggle the data area. Erase block the data will become FF.
	}
	for ( i = 0 ; i < 512; i ++) { // 512 per sector size.
		tempWriteDMA2[i] = 0xAA;       // 0xAA is use for taggle the data area. 
	}
	MMPF_MMU_FlushDCacheMVA((MMP_ULONG)tempWriteDMA1, 512); 
	MMPF_MMU_FlushDCacheMVA((MMP_ULONG)tempWriteDMA2, 512); 

	if (!bSMShortPage) {
		ubPagePerBlock = 8;
		ubRADataCount = 32;
		ulRWPageField = 0xFFFFFF00;
	}
	else {
		ubPagePerBlock = 5;
		ubRADataCount = 8;
		ulRWPageField = 0xFFFFFFE0;

	}
    MMPF_NAND_InitialInterface();
    if (MMPF_NAND_Reset()) {
        return MMP_NAND_ERR_RESET;
    }
    MMPF_NAND_GetSize(&size);
    loop_cnt = SMCTotalZoneCnt;
#if 1
    /// Bad Block Management (BBM)
    /// Read RA from each block's 1st page.
    /// If it is a good block, the 1st byte is 0xFF.
    /// Otherwise, this block is a "Bad Block".
    /// (Default BBM from factory)
    for(j = 0;j < loop_cnt; j++) {
        for(i = 0; i < 1024; i++) {
            DBG_S0("RWP:");
            DBG_L0(rw_page_addr);   
            DBG_S0("\r\n");     
            rw_page_addr = (0 +(((j<<10)+i) << ubPagePerBlock));

            if(MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, rw_page_addr)) {
                DBG_S0("read ra error\r\n");
            }
            src_buf = (MMP_UBYTE *) A8_RA_BUF_ADDR;
            if ((src_buf[0] != 0xFF)) {
                DBG_S0("org block status:");
                DBG_L0(rw_page_addr);
                DBG_S0("!=0xFF");
                DBG_L0(src_buf[0]);
                DBG_S0("\r\n");
            }
            else {
                err = MMPF_NAND_EraseBlock((MMP_ULONG)rw_page_addr);
                //if(err == MMP_NAND_ERR_ERASE) {
                if(err != 0) {
                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
                    for(m = 0; m< ubRADataCount; m++){
                        mem_ptr2[m] = 0x0000;
                    }

                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
                        DBG_S0("write ra error\r\n");
                    }
                }
                else {
                    for (k = 0 ; k < (1 << ubPagePerBlock); k++) {
	                	// Check Block data area.
	                	//RTNA_DBG_Str(0 , "R 0xFF\r\n");
	                	MMPF_NAND_WritePhysicalSector((MMP_ULONG)tempWriteDMA2, (rw_page_addr | k)); 
	                	err = MMPF_NAND_ReadPhysicalSector((MMP_ULONG)m_ulSMDmaAddr, (rw_page_addr | k));
	                	if (err == MMP_NAND_ERR_ECC) {
		                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                    RTNA_DBG_Str(0, "Find Bad Block with 0x0:");	                		
		                    RTNA_DBG_Long(0, rw_page_addr);	                		
		                    RTNA_DBG_Str(0, "\r\n");	                		
							break;
	                	}
	                	else {
	                		tempDMAAddr = (MMP_UBYTE *)m_ulSMDmaAddr;
	                		for (l = 0; l < 512; l ++) {
	                			if (*tempDMAAddr ++ != tempWriteDMA2[l]) {
				                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
				                    for(m = 0; m< ubRADataCount; m++){
				                        mem_ptr2[m] = 0x0000;
				                    }

				                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
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
		                err = MMPF_NAND_EraseBlock((MMP_ULONG)rw_page_addr);
		                //if(err == MMP_NAND_ERR_ERASE) {    
		                if(err != 0) {
		                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                }
						else {
		                    for (k = 0 ; k < (1 << ubPagePerBlock); k++) {
			                	// Check Block data area.
			                	//RTNA_DBG_Str(0 , "W0x0\r\n");
			                	MMPF_NAND_WritePhysicalSector((MMP_ULONG)tempWriteDMA1, (rw_page_addr | k)); 
			                	err = MMPF_NAND_ReadPhysicalSector((MMP_ULONG)m_ulSMDmaAddr, (rw_page_addr | k));
			                	if (err == MMP_NAND_ERR_ECC) {
				                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
				                    for(m = 0; m< ubRADataCount; m++){
				                        mem_ptr2[m] = 0x0000;
				                    }

				                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
				                        DBG_S0("write ra error\r\n");
				                    }
				                    RTNA_DBG_Str(0, "Find Bad Block with 1:");	                		
				                    RTNA_DBG_Long(0, rw_page_addr);	                		
				                    RTNA_DBG_Str(0, "\r\n");	                		
									break;
			                	} 
			                	else {
			                		tempDMAAddr = (MMP_UBYTE *)m_ulSMDmaAddr;
			                		for (l = 0; l < 512; l ++) {
			                			if (*tempDMAAddr ++ != (tempWriteDMA1[l])) {
						                   
						                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
						                    for(m = 0; m< ubRADataCount; m++){
						                        mem_ptr2[m] = 0x0000;
						                    }

						                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
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
		                err = MMPF_NAND_EraseBlock((MMP_ULONG)rw_page_addr);
		                //if(err == MMP_NAND_ERR_ERASE) {    
		                if(err != 0) {
		                    mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
		                    for(m = 0; m< ubRADataCount; m++){
		                        mem_ptr2[m] = 0x0000;
		                    }

		                    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))){
		                        DBG_S0("write ra error\r\n");
		                    }
		                }
		            }	                	
                }
            }
        }
    }
#endif
    DBG_S0("add log addr\r\n");

#if 1
    for(i = 0; i < loop_cnt; i++) {
        log_blk_num = 0;
        for(j = 0; j < 1024; j++) {
            for(k = 0; k < 1; k++) {
                rw_page_addr = (k +(((i<<10)+j)<<ubPagePerBlock));
#if 1
               // Step 1. Check RA area 
               /// Due to AIT erase every block before.
               /// All value in block shoule be 0xFF.
               /// Otherwise, this is a runtime bad block.
                if(MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, rw_page_addr)) {
                    DBG_S0("read ra error2\r\n");
                }

                src_buf = (MMP_UBYTE *)(A8_RA_BUF_ADDR);
                for(l = 0; l < 16; l++){
                    if(src_buf[l] != 0xFF) {
                        bad_block_label = 1;
                        DBG_S0("bad_block_label:");
                        DBG_L0(rw_page_addr);
                        DBG_S0("\r\n");
                        break;
                    }
                }
#endif							
            }
#if 1
            /// Prepare RA info for writing (programing)
            if (bad_block_label == 0) {
                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
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
//                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR + (rw_page_addr& 3)<<4);
                mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
                for(m = 0; m< ubRADataCount; m++){
                    mem_ptr2[m] = 0x0000;
                }
                //RTNA_DBG_Str3("bad_block_label3:\r\n");
            }

            if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (rw_page_addr & ulRWPageField))) {
                DBG_S0("write ra error2\r\n");
            }
            if(bad_block_label == 0){
                log_blk_num++;
            }
#endif

            bad_block_label = 0;

        }
#endif
    RTNA_DBG_Str0(" fast 2k erase OK\r\n");
}
    return MMP_ERR_NONE;
    	
}
#endif
/**
*   @brief  MMPF_NAND_FinishWritePage
*
*
*/
MMP_ERR MMPF_NAND_FinishWritePage(MMP_ULONG ulStartSector)
{
    MMP_ERR   ERRbyte = 0;   
//	RTNA_DBG_Str(0, "MMPF_NAND_FinishWritePage\r\n");
    if(WriteLogPageAddr != 0xFFFFFFFF) {
        if( (WriteLogPageAddr - ulStartSector) == 1){
            WriteInfo.old_phy_blk_addr -= 1;
            WriteInfo.new_phy_blk_addr -= 1;
        }
        if( (WriteInfo.new_phy_blk_addr & PAGE_PER_BLOCK_MASK) != PAGE_PER_BLOCK_MASK){
            ERRbyte = MMPF_NAND_PostCopyBack();   
            if (ERRbyte)
                return ERRbyte; 
        }

        PreparePostCopy = 0x0;
        WriteLogPageAddr = 0xFFFFFFFF;
    }
    return MMP_ERR_NONE;
}


//------------------------------------------------------------------------------
//  Function    : MMPF_NAND_ReadPhysicalSector
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_NAND_ReadPhysicalSector(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{

    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_UBYTE   ecc_buf[6];

    MMP_ERR     status0, status1;
    MMP_UBYTE   *ecc_data;
    MMP_USHORT  i, j;
	MMP_UBYTE   ubRADataCount = 0;
	MMP_ULONG   ulRWPageField = 0;
	MMP_ERR   ERRbyte = 0;   
	MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    
	if (!bSMShortPage) {
		ubRADataCount = 32;
		ulRWPageField = 0xFFFFFF00;
	}
	else {
		ubRADataCount = 8;
		ulRWPageField = 0xFFFFFFE0;
	}    
    MMPF_MMU_FlushDCacheMVA(ulAddr, 512);  
    stampstart = OSTimeGet();     
    //while(!(pNAD->NAD_CTL & NAD_RB));      
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_ReadPhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }  
    
    MMPF_NAND_Enable(MMP_TRUE, MMP_TRUE);

	if (!bSMShortPage) {
	    MMPF_NAND_SendCommand(NAD_READ_1);

	    MMPF_NAND_GetPageAddr(ulPage, 0);
	    MMPF_NAND_SendAddress(SMAddr, SMAddrCyc);
	    MMPF_NAND_SendCommand(NAD_READ_2);
	}
	else {
		MMPF_NAND_SendCommand(NAD_READ_1);
	    MMPF_NAND_GetPageAddr(ulPage, 0);
	    MMPF_NAND_SendAddress(SMAddr, SMAddrCyc);
	}
    MMPF_NAND_WaitCount(10);
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_ReadPhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }    

    pNAD->NAD_RDN_CTL = NAD_RS_EN;

    //DMA settings
    pNAD->NAD_DMA_LEN = (0x200-1);
    pNAD->NAD_DMA_ADDR = ulAddr;

    /* Set data transfer direction */
    pNAD->NAD_CTL |= NAD_READ;

#if (SM_CPU_NONBLOCKING)
    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
    pNAD->NAD_CPU_INT_EN =  NAD_DMA_DONE | NAD_AUTO_ADDR_DONE;
#endif

    ///Trigger controller to start DMA
    pNAD->NAD_DMA_CTL = NAD_GPIO_EN | NAD_DMA_EN;

#if (SM_CPU_NONBLOCKING)
    ERRbyte = MMPF_NAND_WaitInterruptDone();   
    if (ERRbyte)
        return ERRbyte;
            
#else
    stampstart = OSTimeGet(); 
    //while(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE);    
    while(1){
        if( !(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_ReadPhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }
       
    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
#endif
    pNAD->NAD_RDN_CTL = 0;


	if (!bSMShortPage) {
    	i = ulPage &3;
	}
	else {
		i = 0;
	}

    // check ECC value
    ecc_buf[0] = pNAD->NAD_ECC0;
    ecc_buf[1] = pNAD->NAD_ECC1;
    ecc_buf[2] = pNAD->NAD_ECC2;
    ecc_buf[3] = pNAD->NAD_ECC3;
    ecc_buf[4] = pNAD->NAD_ECC4;
    ecc_buf[5] = pNAD->NAD_ECC5;
//  AIT_SUNNY_20051016
	if (!bSMShortPage) {
    	ERRbyte = MMPF_NAND_ReadRA(A8_RA_BUF_ADDR, ulPage);   
    	if (ERRbyte)
            return ERRbyte;    	
	}
	else {
	    //DMA settings
		#if defined(MBOOT_FW)
	    MMPF_MMU_FlushDCacheMVA(A8_RA_BUF_ADDR, 16);  
	    #endif 	    
	    pNAD->NAD_DMA_LEN = (0x10-1);
	    pNAD->NAD_DMA_ADDR = A8_RA_BUF_ADDR;
	    /* Set data transfer direction */
	    pNAD->NAD_CTL |= NAD_READ;	
#if (SM_CPU_NONBLOCKING)
   	    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
        pNAD->NAD_CPU_INT_EN =  NAD_DMA_DONE | NAD_AUTO_ADDR_DONE;
#endif	    
	    ///Trigger controller to start DMA
	    pNAD->NAD_DMA_CTL = NAD_GPIO_EN | NAD_DMA_EN;

#if (SM_CPU_NONBLOCKING)
	    ERRbyte = MMPF_NAND_WaitInterruptDone();   
        if (ERRbyte)
            return ERRbyte;
	    
#else
        stampstart = OSTimeGet(); 
	    //while(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE);    
	    while(1){
            if( !(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_ReadPhysicalSector Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }    
	    
	    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
#endif
	    stampstart = OSTimeGet(); 
	    //while(!(pNAD->NAD_CTL & NAD_RB));    
    	while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_ReadPhysicalSector Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }
          
	}
// AIT_SUNNY_051013 Ecc has problem don't use now
    status0 = MMP_ERR_NONE;
    status1 = MMP_ERR_NONE;
    ecc_data = (MMP_UBYTE *)(A8_RA_BUF_ADDR+(i << 4)+13);
    for(j = 0; j < 3; j++){
        if(ecc_buf[j] != ecc_data[j]) {
            status0 = MMP_NAND_ERR_ECC;
            break;
        }
    }
    ecc_data = (MMP_UBYTE *)(A8_RA_BUF_ADDR+(i << 4)+8);
    for(j = 0; j < 3; j++){
        if(ecc_buf[j+3] != ecc_data[j]){
            status1 = MMP_NAND_ERR_ECC;
            break;
        }
    }

    if(status0 == MMP_NAND_ERR_ECC) {
        status0 = MMPF_NAND_ECCDecode((MMP_UBYTE*)ulAddr, (MMP_UBYTE*)A8_RA_BUF_ADDR +(i << 4)+ 13, ecc_buf);

        if(status0 == MMP_NAND_ERR_ECC) {
            RTNA_DBG_Str(0, "0~255 can't correct:");
			RTNA_DBG_Long(0, ulPage);
            ecc_data = (MMP_UBYTE *)(A8_RA_BUF_ADDR+(i << 4)+13);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[0]);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[1]);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[2]);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[0]);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[1]);
            RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[2]);
    	    RTNA_DBG_Str(0, "\r\n");
    	    return MMP_NAND_ERR_ECC;
        }
        else if(status0 == MMP_NAND_ERR_ECC_CORRECTABLE){
            RTNA_DBG_Str(1, " 0~255 1bit correct\r\n");
			// Fix ECC correct and need to write back to buffer when the buffer is cacheable.
			MMPF_MMU_FlushDCacheMVA(ulAddr, 256); 
        }
    }
	if(status1 == MMP_NAND_ERR_ECC) {
	    status1 = MMPF_NAND_ECCDecode((MMP_UBYTE*)ulAddr + 256, (MMP_UBYTE*)A8_RA_BUF_ADDR +(i << 4)+ 8, ecc_buf + 3);
	    if(status1 == MMP_NAND_ERR_ECC) {
			ecc_data = (MMP_UBYTE*)(A8_RA_BUF_ADDR +(i << 4)+ 8);    
	        RTNA_DBG_Str(0, " 256~511 can't correct:");
			RTNA_DBG_Long(0, ulPage);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[3]);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[4]);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_buf[5]);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[0]);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[1]);
	        RTNA_DBG_Str(0, ":");
			RTNA_DBG_Byte(0, ecc_data[2]);
		    RTNA_DBG_Str(0, "\r\n");
		    return MMP_NAND_ERR_ECC;
	    }
	    else if(status1 == MMP_NAND_ERR_ECC_CORRECTABLE) {
	        RTNA_DBG_Str(1, "256~511 1bit correct\r\n");
			// Fix ECC correct and need to write back to buffer when the buffer is cacheable.
			MMPF_MMU_FlushDCacheMVA(ulAddr+256, 256); 
	    }
    }
    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);
#if 0
	// Bad Block mark BAD.
	if ((status0 == MMP_NAND_ERR_ECC) || (status1 == MMP_NAND_ERR_ECC)) {
		MMP_USHORT *mem_ptr2;
		MMP_UBYTE m = 0;
		RTNA_DBG_Str(0, "Mark bad block:");
		RTNA_DBG_Long(0, (ulPage & ulRWPageField));
		RTNA_DBG_Str(0, "\r\n");

        mem_ptr2 = (MMP_USHORT *)(A8_RA_BUF_ADDR);
        for(m = 0; m< ubRADataCount; m++){
            mem_ptr2[m] = 0x0000;
        }

        if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, (ulPage & ulRWPageField))){ // Mark Block to be bad block.
            DBG_S0("write ra error\r\n");
        }		
        // MMPF_NAND_BuildAllMapTable(); // Prevent Erase and make it OK again.
        return MMP_NAND_ERR_ECC;
	}
#endif
    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_NAND_WritePhysicalSector
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_NAND_WritePhysicalSector(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;

    MMP_UBYTE   nand_status;
    MMP_UBYTE   *ra;
    MMP_USHORT  *ra_short, i;
    MMP_ERR   ERRbyte = 0;   
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    
    #if 0
    RTNA_DBG_Str0("MMPF_NAND_WritePhysicalSector: ");
    RTNA_DBG_Long0(ulAddr);
    RTNA_DBG_Str0(" ");
    RTNA_DBG_Long0(ulPage);
    RTNA_DBG_Str0("\r\n");	
	#endif
	stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_WritePhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }  
    
    MMPF_NAND_Enable(MMP_TRUE, MMP_FALSE);
	if (bSMShortPage) {
		MMPF_NAND_SendCommand(NAD_READ_1);
		MMPF_NAND_WaitCount(10);
    }
    MMPF_NAND_SendCommand(NAD_PAGE_PRG_1);

    MMPF_NAND_GetPageAddr(ulPage, 0);
    MMPF_NAND_SendAddress(SMAddr, SMAddrCyc);
	if (!bSMShortPage) {
	    //Delay for NAND flash to ready
	    MMPF_NAND_WaitCount(2);
	}
    else {
	    MMPF_NAND_WaitCount(10);
	    stampstart = OSTimeGet(); 
	    //while(!(pNAD->NAD_CTL & NAD_RB));   
    	while(1){
            if( (pNAD->NAD_CTL & NAD_RB) )
                break;
            else{
                stampstop = OSTimeGet();
                if( (stampstop - stampstart) > 100){
                    RTNA_DBG_Str(0, "MMPF_NAND_WritePhysicalSector Timeout.\r\n");
                    return MMP_NAND_ERR_TimeOut;                   
                }
            }
        }	    
	}
    pNAD->NAD_RDN_CTL = NAD_RS_EN;	
    //DMA settings
    pNAD->NAD_DMA_LEN = (0x200-1);
    pNAD->NAD_DMA_ADDR = ulAddr;

    pNAD->NAD_CTL &= ~(NAD_READ);

#if (SM_CPU_NONBLOCKING)
    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
    pNAD->NAD_CPU_INT_EN = NAD_DMA_DONE | NAD_AUTO_ADDR_DONE;
#endif

    ///Trigger controller to start DMA
    pNAD->NAD_DMA_CTL = NAD_GPIO_EN | NAD_DMA_EN;

#if (SM_CPU_NONBLOCKING)
    ERRbyte = MMPF_NAND_WaitInterruptDone();    
    if (ERRbyte)
        return ERRbyte;
    
#else
    stampstart = OSTimeGet(); 
    //while(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE);    
    while(1){
        if( !(pNAD->NAD_CPU_INT_SR & NAD_DMA_DONE) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_WritePhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    } 
    
    pNAD->NAD_CPU_INT_SR |= (NAD_DMA_DONE | NAD_AUTO_ADDR_DONE); // write 1 to clear previous status
#endif

    pNAD->NAD_RDN_CTL = 0;

    //DMA Done, send second command
    MMPF_NAND_SendCommand(NAD_PAGE_PRG_2);
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_WritePhysicalSector Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }  

    ra_short = (MMP_USHORT *)A8_RA_BUF_ADDR;
    for(i = 0; i< 32; i++){
        ra_short[i] = 0xFFFF;
    }

    if(SetMapLabel == 0){
		if (!bSMShortPage) {
	        i = ulPage &3;
		}
		else {
			i = 0;
		}
        ra_short = (MMP_USHORT *)(A8_RA_BUF_ADDR+(i << 4)+2);
        *ra_short = WriteInfo.log_blk_offset;
    }
	if (!bSMShortPage) {
    	i = ulPage &3;
	}
	else {
		i = 0;
	}
    ra = (MMP_UBYTE *)A8_RA_BUF_ADDR;
    *(ra+(i << 4)+13) = pNAD->NAD_ECC0;
    *(ra+(i << 4)+14) = pNAD->NAD_ECC1;
    *(ra+(i << 4)+15) = pNAD->NAD_ECC2;
    *(ra+(i << 4)+8)  = pNAD->NAD_ECC3;
    *(ra+(i << 4)+9)  = pNAD->NAD_ECC4;
    *(ra+(i << 4)+10) = pNAD->NAD_ECC5;
    if(SetMapLabel == 1){
        *(ra+(i << 4)+1) =  0x4D;
        *(ra+(i << 4)+5) =  0x41;
    }

    MMPF_NAND_GetStatus(&nand_status);
    if (nand_status & PROGRAM_FAIL) {
        RTNA_DBG_Str(0,"error 1\r\n");
        return  MMP_NAND_ERR_PROGRAM;
    }

    if(MMPF_NAND_WriteRA(A8_RA_BUF_ADDR, ulPage)) {
        RTNA_DBG_Long(3, ulPage);
        RTNA_DBG_Str(1, "write RA fail\r\n");
    }
    MMPF_NAND_GetStatus(&nand_status);
    if (nand_status & PROGRAM_FAIL) {
        RTNA_DBG_Str(0,"error 2\r\n");
        return  MMP_NAND_ERR_PROGRAM;
    }

    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);
#if 0
    // Bad block check method. Write -> read physical and check ECC....
	if (MMP_NAND_ERR_ECC == MMPF_NAND_ReadPhysicalSector(A8_TMP_READ_ADDR, ulPage)) {
		RTNA_DBG_Str(0, "W->R Ecc check fail:");
		RTNA_DBG_Long(0, ulPage);
		RTNA_DBG_Str(0, "\r\n");
	}
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_NAND_WriteRA
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_NAND_WriteRA(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_ULONG   i;
    MMP_ERR   ERRbyte = 0;   
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
	#if defined(MBOOT_FW)
	if (!bSMShortPage)  {
		MMPF_MMU_FlushDCacheMVA(ulAddr, 64);
	}
	else {
		MMPF_MMU_FlushDCacheMVA(ulAddr, 16);
	}
	#endif	
/*	{
		MMP_ULONG *temp = (MMP_ULONG *)ulAddr;
		RTNA_DBG_Str(0, "W RA:"); 
		RTNA_DBG_Long(0, *temp); 
		RTNA_DBG_Str(0, "\r\n"); 

	}*/
	stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_WriteRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }    
    
    MMPF_NAND_Enable(MMP_TRUE, MMP_FALSE);
	if (bSMShortPage) {
		MMPF_NAND_SendCommand(NAD_READ_3);
		MMPF_NAND_WaitCount(10);
    }
    pNAD->NAD_CMD_1 = NAD_PAGE_PRG_1;
    pNAD->NAD_CMD_2 = NAD_PAGE_PRG_2;

    MMPF_NAND_GetPageAddr(ulPage, 1);
	if (!bSMShortPage) {
	    for (i = 0; i < 4; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }

	    pNAD->NAD_DMA_CTL = NAD_GPIO_EN;

	    /* Set DMA start address */
	    pNAD->NAD_DMA_LEN = (0x40-1);
	    pNAD->NAD_DMA_ADDR = ulAddr;

	    pNAD->NAD_ADDR_CTL = SMAddrCyc - 1; // Set 3 is 4 Cycles.
	}
	else {
	    for (i = 0; i < 3; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }

	    pNAD->NAD_DMA_CTL = NAD_GPIO_EN;

	    /* Set DMA start address */
	    pNAD->NAD_DMA_LEN = (0x10-1);
	    pNAD->NAD_DMA_ADDR = ulAddr;

	    pNAD->NAD_ADDR_CTL = 2;
	}
    pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_DMA_CMD2;

    pNAD->NAD_RDN_CTL = 0;

#if (SM_CPU_NONBLOCKING)
    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
    pNAD->NAD_CPU_INT_EN = NAD_AUTO_ADDR_DONE;
#endif

    pNAD->NAD_EXC_CTL = NAD_EXC_ST;

#if (SM_CPU_NONBLOCKING)
    ERRbyte = MMPF_NAND_WaitInterruptDone();     
    if (ERRbyte)
        return ERRbyte;
    
#else
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE));    
    while(1){
        if( (pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_WriteRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }   

    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
#endif

    pNAD->NAD_EXC_CTL = 0;
    pNAD->NAD_RDN_CTL = 0;
    pNAD->NAD_CPU_INT_EN = 0;

    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);

    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_NAND_EraseBlock
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_NAND_EraseBlock(MMP_ULONG ulStartPage)

{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_UBYTE   nand_status;
    MMP_ULONG   i;
    MMP_ERR   ERRbyte = 0;   
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 
    #if 0
	RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock:");
	RTNA_DBG_Long(0, ulStartPage);
	RTNA_DBG_Str(0, "\r\n");
	#endif
//#if 1
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }    
    
    MMPF_NAND_Enable(MMP_TRUE, MMP_FALSE);

    pNAD->NAD_CMD_1 = NAD_BLK_ERASE_1;
    pNAD->NAD_CMD_2 = NAD_BLK_ERASE_2;

    MMPF_NAND_GetPageAddr(ulStartPage, 0);
	if (!bSMShortPage) {
	    SMAddr[0] = SMAddr[2];
	    SMAddr[1] = SMAddr[3];
	    SMAddr[2] = SMAddr[4];

	    for (i = 0; i < SMAddrCyc - 2; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }

	    pNAD->NAD_DMA_CTL = NAD_GPIO_EN;

	    pNAD->NAD_ADDR_CTL = SMAddrCyc - 3; // 4-3 = 2 cycles.
	}
	else {
		SMAddr[0] = SMAddr[1];
	    SMAddr[1] = SMAddr[2];
	    for (i = 0; i < 2; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }
	    pNAD->NAD_DMA_CTL = NAD_GPIO_EN;
	    pNAD->NAD_ADDR_CTL = 1;
	}
    pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_CMD2;

    pNAD->NAD_RDN_CTL = 0;

#if (SM_CPU_NONBLOCKING)
    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
    pNAD->NAD_CPU_INT_EN = NAD_AUTO_ADDR_DONE;
#endif

    pNAD->NAD_EXC_CTL = NAD_EXC_ST;

#if (SM_CPU_NONBLOCKING)
    ERRbyte = MMPF_NAND_WaitInterruptDone(); 
    if (ERRbyte)
        return ERRbyte;
    
#else
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE));    
    while(1){
        if( (pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_EraseBlock Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }    

    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
#endif

    pNAD->NAD_CPU_INT_EN = 0;
    pNAD->NAD_EXC_CTL = 0;

    /* Check success or not */
    MMPF_NAND_GetStatus(&nand_status);
    if(nand_status & PROGRAM_FAIL){
        RTNA_DBG_Str(0,"error 3\r\n");
        RTNA_DBG_Str(0,"MMPF_NAND_EraseBlock error\r\n");        
        return MMP_NAND_ERR_ERASE;
    }

    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);
//#endif
    return MMP_ERR_NONE;
}
//------------------------------------------------------------------------------
//  Function    : MMPF_NAND_ReadRA
//  Parameter   : None
//  Return Value : None
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_NAND_ReadRA(MMP_ULONG ulAddr, MMP_ULONG ulPage)
{
    AITPS_NAND  pNAD = AITC_BASE_NAND;
    MMP_ULONG   i;
    MMP_ERR     ERRbyte = 0;   
    MMP_ULONG stampstart = 0; 
    MMP_ULONG stampstop  = 0; 

	if (!bSMShortPage) 
        MMPF_MMU_FlushDCacheMVA(ulAddr, 64);
    else   
	    MMPF_MMU_FlushDCacheMVA(ulAddr, 16);

    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CTL & NAD_RB));    
    while(1){
        if( (pNAD->NAD_CTL & NAD_RB) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_ReadRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }   
    
    MMPF_NAND_Enable(MMP_TRUE, MMP_TRUE);
	if (!bSMShortPage) {
	    pNAD->NAD_CMD_1 = NAD_READ_1;
	    pNAD->NAD_CMD_2 = NAD_READ_2;

	    MMPF_NAND_GetPageAddr(ulPage, 1);

	    for (i = 0; i < 4; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }

	    /* Set DMA start address */
	    pNAD->NAD_DMA_LEN = (0x40-1);
	    pNAD->NAD_DMA_ADDR = ulAddr;

	    pNAD->NAD_CTL |= NAD_READ;

	    pNAD->NAD_ADDR_CTL = SMAddrCyc - 1; //address cycle

	    pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_CMD2_DMA;
	    pNAD->NAD_RDN_CTL = 0;
	}
	else {
	    pNAD->NAD_CMD_1 = NAD_READ_3;
	    MMPF_NAND_GetPageAddr(ulPage, 1);

	    for (i = 0; i < 3; i++) {
	        pNAD->NAD_ADDR[i] = SMAddr[i];
	    }

	    /* Set DMA start address */
	    pNAD->NAD_DMA_LEN = (0x10-1);
	    pNAD->NAD_DMA_ADDR = ulAddr;

	    pNAD->NAD_CTL |= NAD_READ;

	    pNAD->NAD_ADDR_CTL = 2; //address cycle

	    pNAD->NAD_CMD_CTL = NAD_CMD1_ADDR_DMA;
	    pNAD->NAD_RDN_CTL = 0;
	}
#if (SM_CPU_NONBLOCKING)
    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
    pNAD->NAD_CPU_INT_EN = NAD_AUTO_ADDR_DONE;
#endif

    pNAD->NAD_EXC_CTL = NAD_EXC_ST;

#if (SM_CPU_NONBLOCKING)
    ERRbyte = MMPF_NAND_WaitInterruptDone();   
    if (ERRbyte)
        return ERRbyte;
    
#else
    stampstart = OSTimeGet(); 
    //while(!(pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE));    
    while(1){
        if( (pNAD->NAD_CPU_INT_SR & NAD_AUTO_ADDR_DONE) )
            break;
        else{
            stampstop = OSTimeGet();
            if( (stampstop - stampstart) > 100){
                RTNA_DBG_Str(0, "MMPF_NAND_ReadRA Timeout.\r\n");
                return MMP_NAND_ERR_TimeOut;                   
            }
        }
    }  

    pNAD->NAD_CPU_INT_SR |= NAD_AUTO_ADDR_DONE; // write 1 to clear previous status
#endif

    pNAD->NAD_EXC_CTL = 0;

    /* Disable SMC */
    MMPF_NAND_Enable(MMP_FALSE, MMP_TRUE);

    return MMP_ERR_NONE;
}
#pragma

/** @brief Set temp buffer address for SM driver

@param[in] ulStartAddr Start address for temp buffer address.
@param[in] ulSize Size for temp buffer. Currently buffer size is only 64 Byte for SDHC message handshake
@return NONE
*/
void    MMPF_SM_SetTmpAddr(MMP_ULONG ulStartAddr, MMP_ULONG ulSize)
{
    m_ulSMDmaAddr = ulStartAddr;
    ulSize = ulSize; /* dummy for prevent compile warning */
    RTNA_DBG_Str(0, "sm dma addr: ");
    RTNA_DBG_Long(0, ulStartAddr);
    RTNA_DBG_Str(0, "\r\n");
}

/** @brief Get temp buffer address for SM driver
@param[out] ulStartAddr Start address for temp buffer address.
@return NONE
*/
void    MMPF_SM_GetTmpAddr(MMP_ULONG *ulStartAddr)
{
    *ulStartAddr = m_ulSMDmaAddr;
}
#endif  //#if   (USING_SM_IF) && (SM_2K_PAGE)

/** @} */ // MMPF_SM
