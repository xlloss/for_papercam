//==============================================================================
//
//  File        : sf_cfg.c
//  Description : SPI Nor Flash configure file
//  Author      : Rony Yeh
//  Revision    : 1.0
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
#include "config_fw.h" // add for use (USING_SF_IF )

#if (USING_SF_IF)
#include "sf_cfg.h" // add for use (module define )
#include "mmpf_sf.h" 

//==============================================================================
//
//                              EXTERNAL FUNC
//
//==============================================================================

//==============================================================================
//
//                              SPI NOR Flash Parameter assignment
//
//==============================================================================

SPINorProfile SPINORConfigMatrix[SPIMAXMODULENO] = {

#if (SPI_NORFLASH_DEFAULTSETTING) //The array member must be leave for Default Setting
{   
    0xFFFFFFFF,    // DeviceID   SPI_NorFlash_DefaultSetting
    0x400000,	   // TotalSize	256Mb 32MB
    33000,		   // parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
    0x1000,		   // 4K for each Sector	
    0x8000,		   // 32K for each Block
    #if defined(ALL_FW)
    0,             // Capability to support Quad SPI read
    1,             // Capability to support Dual SPI read
    #else // only use Quad mode reading in mini-boot & bootloader
    0,             // Capability to support Quad SPI read
    1,             // Capability to support Dual SPI read
    #endif
    0,			   // Capability to support Quad SPI write	
    0,             // option to seepdup write volatile SR process
    0,             // 4 bytes address mode enable/disable
    0,             // Support AAI-Write prrameter 	
    0,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

    1,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
    2,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
    0x10,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
    1,             // Status Register Quad SPI <Quad Enable> group 1 
    0x40,          // Bit 6 = 0x40    

    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    READ_DATA,           // Command for read data <4 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <4 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <4 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <4 Byte Address>
    PAGE_PROGRAM,        // Command for write data <4 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <4 Byte Address>

    SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
    SECTOR_ERASE,	     // Command for Erase Sector <4 Byte Address>
},
#endif   

#if (SPI_NORFLASH_GD_32MB) //The array member must be leave for Default Setting
{   
    0x00C84019,    // DeviceID   SPI_NorFlash_DefaultSetting GD256Mb
	0x2000000,	   // TotalSize	256Mb 32MB
	33000,		   //parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
	0x1000,		   // 4K for each Sector	
	0x8000,		   // 32K for each Block	
	
	1,             // Capability to support Quad SPI read	
	0,             // Capability to support Dual SPI read
	1,			   // Capability to support Quad SPI write	
	0,             // option to seepdup write volatile SR process
	0,             // 4 bytes address mode enable/disable
	0,             // Support AAI-Write prrameter 		
	1,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

	1,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
	2,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
	0x10,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
	1,             // Status Register Quad SPI <Quad Enable> group 1 
    0x40,          // Bit 6 = 0x40    
	
    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    READ_DATA,           // Command for read data <4 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <4 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <4 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <4 Byte Address>
    PAGE_PROGRAM,        // Command for write data <4 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <4 Byte Address>

	SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
	SECTOR_ERASE,	     // Command for Erase Sector <4 Byte Address>
},
#endif 

#if (SPI_NORFLASH_MXIC_32MB)
{
    0x00C22019,    // DeviceID   SPI_NorFlash_DefaultSetting   MXIC 256Mb 32MB
	0x2000000,	   // TotalSize	256Mb 32MB
	33000,		   //parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
	0x1000,		   // 4K for each Sector	
	0x8000,		   // 32K for each Block	
	
	1,             // Capability to support Quad SPI read	
	0,             // Capability to support Dual SPI read
	0,			   // Capability to support Quad SPI write	
	0,             // option to seepdup write volatile SR process
	0,             // 4 bytes address mode enable/disable
	0,             // Support AAI-Write prrameter		
	0,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

	0,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
	3,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
	0x20,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
	1,             // Status Register Quad SPI <Quad Enable> group 1 
    0x40,          // Bit 6 = 0x40    
    
    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    0x13,                // Command for read data <4 Byte Address>
    0x0C,                // Command for fast read data <4 Byte Address>
    0x3C,                // Command for Dual fast read data <4 Byte Address>
    0x6C,                // Command for Quad fast read data <4 Byte Address>
    0x12,                // Command for write data <4 Byte Address>
    0x3E,                // Command for Quad write data <4 Byte Address>

	SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
	0x21,				 // Command for Erase Sector <4 Byte Address>
},
#endif   

#if (SPI_NORFLASH_WBD_32MB)
{  
    0x00EF4019,    // DeviceID   WINBOND_25Q256
	0x2000000,	   // TotalSize	256Mb 32MB
	33000,		   //parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
	0x1000,		   // 4K for each Sector	
	0x8000,		   // 32K for each Block	 
	
	1,             // Capability to support Quad SPI read	
	0,             // Capability to support Dual SPI read
	0,			   // Capability to support Quad SPI write	
	1,             // option to seepdup write volatile SR process
	0,             // 4 bytes address mode enable/disable
	0,             // Support AAI-Write prrameter 		
	0,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

	1,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
	3,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
	0x02,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
	2,             // Status Register Quad SPI <Quad Enable> group 1 
    0x02,          // Bit 6 = 0x40    
	
    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    READ_DATA,           // Command for read data <4 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <4 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <4 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <4 Byte Address>
    PAGE_PROGRAM,        // Command for write data <4 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <4 Byte Address>

	SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
	SECTOR_ERASE,	     // Command for Erase Sector <4 Byte Address>
},
#endif   // #if (SPI_NorFlash_WINBOND_25Q128)



#if (SPI_NORFLASH_WBD_16MB)
{  
    0x00EF4018,    // DeviceID   WINBOND_25Q128
    0x1000000,	   // TotalSize	128Mb 16MB
    66000,		   //parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
    0x1000,		   // 4K for each Sector	
    0x8000,		   // 32K for each Block	 

    #if defined(ALL_FW)
    0,             // Capability to support Quad SPI read
    1,             // Capability to support Dual SPI read
    #else // only use Quad mode reading in mini-boot & bootloader
    1,             // Capability to support Quad SPI read
    0,             // Capability to support Dual SPI read
    #endif
    0,			   // Capability to support Quad SPI write	
    1,             // option to seepdup write volatile SR process
    0,             // 4 bytes address mode enable/disable
    0,             // Support AAI-Write prrameter 		
    0,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

    0,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
    3,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
    0x02,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
    2,             // Status Register Quad SPI <Quad Enable> group 1 
    0x02,          // Bit 6 = 0x40    

    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    READ_DATA,           // Command for read data <4 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <4 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <4 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <4 Byte Address>
    PAGE_PROGRAM,        // Command for write data <4 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <4 Byte Address>

    SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
    SECTOR_ERASE,	     // Command for Erase Sector <4 Byte Address>
},
#endif   // #if (SPI_NorFlash_WINBOND_25Q128)

#if (SPI_NORFLASH_GD_16MB)
{  
    0x00C84018,    // DeviceID   GD_16MB
    0x1000000,	   // TotalSize	128Mb 16MB
    66000,		   //parameter to create <ulSifClkDiv> EX: ulSifClkDiv = (ulG0Freq+32999)/SIFCLKDividor
    0x1000,		   // 4K for each Sector	
    0x8000,		   // 32K for each Block	 

    #if defined(ALL_FW)
    0,             // Capability to support Quad SPI read
    1,             // Capability to support Dual SPI read
    #else // only use Quad mode reading in mini-boot & bootloader
    1,             // Capability to support Quad SPI read
    0,             // Capability to support Dual SPI read
    #endif
    0,			   // Capability to support Quad SPI write	
    1,             // option to seepdup write volatile SR process
    0,             // 4 bytes address mode enable/disable
    0,             // Support AAI-Write prrameter 		
    0,             // Clear Status Register<Command 30h> in the reset API;(0)Not ClearSR (1)ClearSR,make sure module support capability before assigning

    1,             // Execute/NotExecute ADP mode setting,some module don't support this function<ex:MXIC>;(0)Not Execute ADP , (1)Execute ADP
    3,			   // Status Register Group Index to Enable/Disable <ADP function> for 4 Byte Address mode, group1<0x05> group2<0x35> group3<0x15>	
    0x02,		   // Status Register Bit Number to Enable/Disable <ADP function> for 4 Byte Address mode,EX: bit4=0x10,bit5=0x20
    2,             // Status Register Quad SPI <Quad Enable> group 1 
    0x02,          // Bit 1 = 0x02    

    READ_DATA,           // Command for read data <3 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <3 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <3 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <3 Byte Address>
    PAGE_PROGRAM,        // Command for write data <3 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <3 Byte Address>
    READ_DATA,           // Command for read data <4 Byte Address>
    FAST_READ_DATA,      // Command for fast read data <4 Byte Address>
    DUAL_FAST_READ_DATA, // Command for Dual fast read data <4 Byte Address>
    QUAD_FAST_READ_DATA, // Command for Quad fast read data <4 Byte Address>
    PAGE_PROGRAM,        // Command for write data <4 Byte Address>
    QUAD_PAGE_PROGRAM,   // Command for Quad write data <4 Byte Address>

    SECTOR_ERASE,        // Command for Erase Sector <3 Byte Address>
    SECTOR_ERASE,	     // Command for Erase Sector <4 Byte Address>
}
#endif   // #if (SPI_NORFLASH_GD_16MB)

};

int SF_Module_Init(void) ITCMFUNC;
int SF_Module_Init(void)
{
    MMP_ULONG dev;

    for(dev = 0; dev < SPIMAXMODULENO; dev++) {
        if (SPINORConfigMatrix[dev].DeviceID != 0)
            MMPF_SF_Register((MMP_UBYTE)dev, &SPINORConfigMatrix[dev]);
    }

    return 0;
}

/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall"  */
/* #pragma O0 */
ait_module_init(SF_Module_Init);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

#endif   // #if (USING_SF_IF)

