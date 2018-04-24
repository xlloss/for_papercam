//==============================================================================
//
//  File        : JobDispatch.c
//  Description : ce job dispatch function
//  Author      : Robin Feng
//  Revision    : 1.0
//
//==============================================================================

#include "mmp_err.h"
#include "mmpf_typedef.h"
#include "lib_retina.h"
#include "mmpf_sf.h"
#include "mmpf_storage_api.h"
#if MTD_OTA_EN
#include "mtd_ota.h"
#endif
//==============================================================================
//
//                              COMPILING OPTIONS
//
//==============================================================================

#define JTAG_EN     (0)

//==============================================================================
//
//                              CONSTANT
//
//==============================================================================

#define STORAGE_TEMP_BUFFER         0x106000
#define AIT_FW_TEMP_BUFFER_ADDR     0x106000
#define AIT_BOOT_HEADER_ADDR        0x106200
#define AIT_BOOT_START_ADDR         0x122000

//==============================================================================
//
//                              Extern
//
//==============================================================================

extern int SF_Module_Init(void);

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void MiniBoot_Task(void *p_arg) ITCMFUNC;
void MiniBoot_Task(void *p_arg)
{
    void (*FW_Entry)(void) = NULL;

	AIT_STORAGE_INDEX_TABLE2 *pIndexTable = (AIT_STORAGE_INDEX_TABLE2 *)STORAGE_TEMP_BUFFER;
    MMP_ULONG   ulSifAddr = 0x0;
	MMP_USHORT  usCodeCrcValue = 0x0;
	MMP_ERR     err;

    RTNA_DBG_Str0("MiniBoot_Task()\r\n");

	#if (JTAG_EN)
    while(1);		// For ICE Debug
    #endif

	/* Download firmware from storage */
	SF_Module_Init();
    MMPF_SF_SetIdBufAddr((MMP_ULONG)(STORAGE_TEMP_BUFFER - 0x1000));
	MMPF_SF_SetTmpAddr((MMP_ULONG)(STORAGE_TEMP_BUFFER - 0x1000));
  	MMPF_SF_InitialInterface(MMPF_SIF_PAD_MAX);

	err = MMPF_SF_Reset();
	if (err) {
		RTNA_DBG_Str(0, "SIF Reset error !!\r\n");
		return;
	}
#if MTD_OTA_EN
{    
    MMPF_SF_FastReadData(MTD_TABLE_LOAD_ADDR, STORAGE_TEMP_BUFFER,MTD_TABLE_MAX_SIZE );
    mtd_set_desc_table( (char *)MTD_TABLE_LOAD_ADDR ) ;
    ulSifAddr = mtd_get_active_mtd_offset("boot") ;
}    
#endif

	MMPF_SF_FastReadData(ulSifAddr, STORAGE_TEMP_BUFFER, 512);  //AitMiniBootloader Header Table
	ulSifAddr = ulSifAddr + (0x1 << 12)*(pIndexTable->ulTotalSectorsInLayer);
	MMPF_SF_FastReadData(ulSifAddr, (STORAGE_TEMP_BUFFER), 512);  //AitBootloader Header Table

	if ((pIndexTable->ulHeader == INDEX_TABLE_HEADER) && 
		(pIndexTable->ulTail == INDEX_TABLE_TAIL) &&
		(pIndexTable->ulFlag & 0x1))
    {
		RTNA_DBG_PrintLong(3, pIndexTable->bt.ulDownloadDstAddress);
		FW_Entry = (void (*)(void))(pIndexTable->bt.ulDownloadDstAddress);

		#if (CHIP == MCR_V2)
		if(pIndexTable->ulFlag & 0x40000000) { //check bit 30
			MMPF_SF_EnableCrcCheck(MMP_TRUE);
		}
		#endif
		RTNA_DBG_Str3("SIF downlod start\r\n");

		RTNA_DBG_PrintLong(3, pIndexTable->bt.ulStartBlock);

		MMPF_SF_FastReadData(pIndexTable->bt.ulStartBlock << pIndexTable->ulAlignedBlockSizeShift, 
								pIndexTable->bt.ulDownloadDstAddress, 
								pIndexTable->bt.ulCodeSize);

		#if (CHIP == MCR_V2)
		if(pIndexTable->ulFlag & 0x40000000) { //check bit 30
			usCodeCrcValue = MMPF_SF_GetCrc();
			MMPF_SF_FastReadData(ulSifAddr + 0x1000, STORAGE_TEMP_BUFFER, 2); //Read CRC block value
			if(*((MMP_USHORT*)STORAGE_TEMP_BUFFER) == usCodeCrcValue) {
				RTNA_DBG_Str3("CRC pass\r\n");
			}
			else {
				RTNA_DBG_Str(0, "CRC fail!\r\n");
				RTNA_DBG_PrintShort(0, usCodeCrcValue);
				RTNA_DBG_PrintShort(0, *((MMP_USHORT*)STORAGE_TEMP_BUFFER));
				while(1);
			}
		}	
		#endif						
		RTNA_DBG_Str3("SIF downlod End\r\n");
	}
	else {
		RTNA_DBG_PrintLong(0, pIndexTable->ulHeader);
		RTNA_DBG_PrintLong(0, pIndexTable->ulTail);
		RTNA_DBG_PrintLong(0, pIndexTable->ulFlag);
		RTNA_DBG_Str(0, "Invalid Index Table!!\r\n");
		while(1);
	}

    /* Jump PC */
    FW_Entry();

    while (1) ;
}

