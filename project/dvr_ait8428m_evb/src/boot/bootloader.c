//==============================================================================
//
//  File        : bootloader.c
//  Description : Firmware loader function
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================

#include "includes_fw.h"

#include "mmp_reg_gbl.h"

#include "mmps_system.h"
#include "mmpf_mci.h"
#include "mmpf_dram.h"
#include "mmpf_pll.h"
#include "mmpf_storage_api.h"
#include "mmpf_sf.h"
#include "mtd_layout.h"
#if MTD_OTA_EN
#include "mtd_ota.h"
#endif
#include "mmpf_system.h"
#include "mmp_reg_wd.h"
#include "mmpf_wd.h"

//==============================================================================
//
//                              COMPILING OPTIONS
//
//==============================================================================
#if GPIO_MAP==QD_EVB_GPIO_MAP
#define OTA_FAIL_CHECK      (1)
#else
#define OTA_FAIL_CHECK      (0) 
#endif

#if (CPUB_JTAG_DEBUG)
#define RUN_RTOS_ON_CPU_B   (0)
#else
#define RUN_RTOS_ON_CPU_B   (1)
#endif
#define RUN_LINUX_ON_CPU_A  (1)
#define LOAD_USER_SETTINGS  (1)

#define DUMP_RAM_EN         (0)
#define DEBUG_TIMING        (0)

#define EMBEDDED_FFS_EN     (1)
//==============================================================================
//
//                              CONSTANT
//
//==============================================================================

#define AIT_FW_TEMP_BUFFER_ADDR  	(0x106000 )
#define AIT_BOOT_HEADER_ADDR     	(0x106200 )
#define STORAGE_TEMP_BUFFER 		(0x106000 )

#define CPUB_ITCM_ADDR              (0x900000)
#define DRAM_ADDR                   (0x01000000)

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

#if RUN_LINUX_ON_CPU_A==1
// header for uImage
#define IH_MAGIC 0x27051956
#define IH_NMLEN 32
typedef struct image_header {
        unsigned long        ih_magic;       /* Image Header Magic Number    */
        unsigned long        ih_hcrc;        /* Image Header CRC Checksum    */
        unsigned long        ih_time;        /* Image Creation Timestamp     */
        unsigned long        ih_size;        /* Image Data Size              */
        unsigned long        ih_load;        /* Data  Load  Address          */
        unsigned long        ih_ep;          /* Entry Point Address          */
        unsigned long        ih_dcrc;        /* Image Data CRC Checksum      */
        unsigned char        ih_os;          /* Operating System             */
        unsigned char        ih_arch;        /* CPU architecture             */
        unsigned char        ih_type;        /* Image Type                   */
        unsigned char        ih_comp;        /* Compression Type             */
        unsigned char        ih_name[IH_NMLEN];      /* Image Name           */
} image_header_t;


#define ATAG_CORE       0x54410001
#define ATAG_CMDLINE    0x54410009
#define ATAG_NONE       0x00000000

/* structures for each atag */
struct atag_header {
        unsigned long size; /* length of tag in words including this header */
        unsigned long tag;  /* tag type */
};

struct atag_core {
        unsigned long flags;
        unsigned long pagesize;
        unsigned long rootdev;
};

struct atag_cmdline {
    char    cmdline[384];
};


struct atag {
    struct atag_header  hdr;
    union {
        struct atag_core    core ;
        struct atag_cmdline cmdline;
        
    } u ;
};

#endif

typedef struct _FlashCfg_Map_t {
    char name[8];
    MMP_ULONG load_src_addr;
    MMP_ULONG load_dst_addr;
    MMP_ULONG size;
} FlashCfg_Map_t;
#ifndef __GNUC__
extern MMP_ULONG Image$$BOOT_MENU$$Base;
#else
extern unsigned char* __BOOT_START__;
#endif
FlashCfg_Map_t CfgMap[]= {
    {
        "user",
        #ifdef MTD_OFFSET_config
        MTD_OFFSET_config,
        #else
        0xFFC000,
        //0xE7C000,
        #endif
        //DRAM_ADDR + (62 * 1024 - 16 - 260 ) * 1024,
        (int)&__BOOT_START__,
        8*1024 
    },
    {   
        "ddr3",
        #ifdef MTD_OFFSET_ddr3        
        MTD_OFFSET_ddr3,
        #else
        0xFF000, // 1M-4K
        #endif
        STORAGE_TEMP_BUFFER,
        16,
    },
    #if MTD_OTA_EN
    {
        "mtdtab",
        #ifdef MTD_OFFSET_mtdtab
        MTD_OFFSET_mtdtab,
        #else
        MTD_TABLE_LOAD_ADDR,
        #endif
        STORAGE_TEMP_BUFFER,
        MTD_TABLE_MAX_SIZE 
    },
    #endif
    #if EMBEDDED_FFS_EN
    {
        "ffs" ,
        #ifdef MTD_OFFSET_embedded
        MTD_OFFSET_embedded,
        #else
        0xE00000,
        #endif
        0,
        0
        
    } ,
    #endif
    #if OTA_FAIL_CHECK
    {
        "mtdtab_b",
        #ifdef MTD_OFFSET_mtdtab
        MTD_OFFSET_mtdtab + 0xE000 ,
        #else
        MTD_TABLE_LOAD_ADDR + 0xE000 ,
        #endif
        STORAGE_TEMP_BUFFER,
        MTD_TABLE_MAX_SIZE 
    }
    , 
    {
        "ota_f",
        #ifdef MTD_OFFSET_mtdtab
        MTD_OFFSET_mtdtab + 0x8000 ,
        #else
        MTD_TABLE_LOAD_ADDR + 0x8000 ,
        #endif
        STORAGE_TEMP_BUFFER,
        16
    }, 
       
    #endif
    
    
};

#define CFG_SIZE    ((sizeof(CfgMap)) / sizeof(FlashCfg_Map_t))

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern int SF_Module_Init(void);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
#if (RUN_RTOS_ON_CPU_B)
/*
return 1 if cpu-b occur watch dog
*/
static int BootLoader_Start_CpuB(MMP_ULONG entry,MMP_BOOL chk_wd_only)
{
    AITPS_GBL   pGBL  = AITC_BASE_GBL;
    AITPS_CORE  pCORE = AITC_BASE_CORE;
    volatile MMP_ULONG *reset_vector = (volatile MMP_ULONG *)CPUB_ITCM_ADDR;

    /* Enable CPU_B clock */
    pGBL->GBL_CLK_EN[0] |= (GBL_CLK_CPU_B | GBL_CLK_CPU_B_PHL);

    pCORE->CORE_B_WD_CFG |= HOST_RST_CPU_EN;
    /* Enable CPU_A download code to CPU_B */
    pCORE->CORE_MISC_CFG &= ~(CPU_A2B_DNLD_EN | CPU_B2A_DNLD_EN);
    pCORE->CORE_MISC_CFG |= CPU_A2B_DNLD_EN;
    /* Halt CPU_B */
    pCORE->CORE_B_CFG |= CPU_RST_EN;

    if( *(reset_vector+5)==0x1234bbbb ) {
      *(reset_vector+5) = 0x0;
      return 1 ;
    }


    /* Program a jump instruction to ITCM of CPU_B */
    *(reset_vector+0) = 0xea000006;
    *(reset_vector+8) = 0xe51ff004;
    *(reset_vector+9) = entry;
    
    
    /* Reset CPU_B */
    pCORE->CORE_MISC_CFG &= ~(CPU_A2B_DNLD_EN);
    pCORE->CORE_B_WD_CFG &= ~(HOST_RST_CPU_EN);
    pCORE->CORE_B_CFG &= ~(CPU_RST_EN);
    
    return 0 ;
}

static void BootLoader_Start_Rtos(MMP_BOOL cpu_b,MMP_ULONG entry)
{
    if(cpu_b) {
      BootLoader_Start_CpuB(  entry , MMP_FALSE );
    }
    else {
      volatile MMP_ULONG *reset_vector = (volatile MMP_ULONG *)0;
      void (*kernel_entry)(void);
      OS_CRITICAL_INIT();
      /* Program a jump instruction to ITCM of CPU_A */
      OS_ENTER_CRITICAL();
      *(reset_vector+0) = 0xea000006;
      *(reset_vector+8) = 0xe51ff004;
      *(reset_vector+9) = entry;
      kernel_entry = (void (*)(void))(0);
      kernel_entry();
      while(1);
    }
}


#endif

#if (RUN_LINUX_ON_CPU_A)
//#define CMD_LINE "lpj=1314816 mem=128M console=ttyS0,115200 rootfstype=squashfs init=/init root=1f02 mtdparts=spi0.0:1M(boot),2M(kernel),6M(cramfs);"
#define tag_next(t)     ((struct tag *)((unsigned long *)(t) + (t)->hdr.size))
#define tag_size(type)  ((sizeof(struct atag_header) + sizeof(struct type)) >> 2)
static struct atag *params; /* used to point at the current tag */

#if (USER_STRING)
extern size_t strlen (const char *str) ;
extern char * strcpy(char *strDest, const char *strSrc);
#endif
static void 
setup_start_tag(void *addr)
{
    params = (struct atag *)addr ;
    params->hdr.tag = ATAG_CORE;            /* start with the core tag */
    params->hdr.size = tag_size(atag_core); /* size the tag */

    params->u.core.flags = 1;               /* ensure read-only */
    params->u.core.pagesize = 4096;     /* systems pagesize (4k) */
    params->u.core.rootdev = 0;             /* zero root device (typicaly overidden from commandline )*/

    params = (struct atag *)tag_next(params);              /* move pointer to next tag */
}

static void
setup_cmdline_tag(const char * line)
{
    int linelen = strlen(line);

    if(!linelen)
        return;                             /* do not insert a tag for an empty commandline */

    params->hdr.tag = ATAG_CMDLINE;         /* Commandline tag */
    params->hdr.size = (sizeof(struct atag_header) + linelen + 1 + 4) >> 2;

    strcpy(params->u.cmdline.cmdline,line); /* place commandline into tag */

    params = (struct atag *)tag_next(params);              /* move pointer to next tag */
}


static void
setup_end_tag(void)
{
    params->hdr.tag = ATAG_NONE;            /* Empty tag ends list */
    params->hdr.size = 0;                   /* zero length */
}

static void
setup_tags(void *addr,char *cmd_line)
{
    setup_start_tag((void *)addr);
    if(cmd_line) {
        setup_cmdline_tag(cmd_line);    /* commandline setting root device */
    }
    setup_end_tag();                    /* end of tags */
}

static unsigned long get_ih_item(int item,unsigned char *ih)
{
    unsigned char *ptr = (unsigned char *)(ih + item*4) ;
    unsigned long val ;
    val = ( ptr[0] << 24 ) | ( ptr[1] << 16 ) | (ptr[2] << 8) | ptr[3] ;
    return val ;
}

static void BootLoader_Start_CpuA(unsigned long kernel_at,char *cmdline)
{
	void (*kernel_entry)(int zero, int arch, unsigned int params);
#ifdef MTD_OFFSET_kernel
#define NO_AIT_HDR_FW_LOAD_AD		(MTD_OFFSET_kernel)
#else
#define NO_AIT_HDR_FW_LOAD_AD		(0x100000)
#endif
#define NO_AIT_HDR_FW_LOAD_SZ		(2*1024*1024)

	MMP_ULONG start_time,current_time,delta_time;
    MMP_ULONG parm_at = DRAM_ADDR + 0x100 ;
    MMP_ULONG load_addr = 0x1008000 ;
    MMP_ULONG exec_addr = 0x1008040 ;
    MMP_ULONG load_size = NO_AIT_HDR_FW_LOAD_SZ ;
	start_time = OSTimeGet();
#if DEBUG_TIMING==1 // CGPIO29 , GPIO61
    *(volatile MMP_UBYTE *)0x80006947 |= 0x20;
    *(volatile MMP_UBYTE *)0x80006607 |= 0x20;
    *(volatile MMP_UBYTE *)0x80006617 |= 0x20;
#endif
	MMPF_SF_FastReadData(kernel_at, load_addr, 512);
	
    if(get_ih_item(0,(unsigned char *)load_addr)==IH_MAGIC) {
        MMP_ULONG offset = 0x00 ;
	    RTNA_DBG_Str(0, "\r\nLoad linux kernel...\r\n");
	    load_size = get_ih_item(3,(unsigned char *)load_addr) ;
	    load_addr = get_ih_item(4,(unsigned char *)load_addr) ;
	    exec_addr = get_ih_item(5,(unsigned char *)load_addr) ;
	    load_size = ALIGN512(load_size + sizeof( image_header_t ) ) ;
	    
	    // if load addr = exec addr, treat the image as un-compressed
	    if(load_addr==exec_addr) {
	        offset = 0x40 ;
	        //exec_addr = load_addr  ;
	    } 
	    MMPF_SF_FastReadData(kernel_at + offset , load_addr, load_size);
	    
	    RTNA_DBG_Str(0,"Load Info...\r\n");
	    MMPF_DBG_Int(offset, 8);
	    MMPF_DBG_Int(load_size, 8);
	    MMPF_DBG_Int(load_addr, 8);
	    MMPF_DBG_Int(exec_addr, 8);
	    
	}
	else {
	    RTNA_DBG_Str(0, "\r\nLoad linux kernel(no magic)...\r\n");
	    MMPF_SF_FastReadData(kernel_at, load_addr, NO_AIT_HDR_FW_LOAD_SZ);
	}
	kernel_entry = (void (*)(int zero, int arch, unsigned int params))(exec_addr);
	current_time = OSTimeGet();
	delta_time = current_time - start_time;
#if DEBUG_TIMING==1
    *(volatile MMP_UBYTE *)0x80006617 &= ~0x20;
#endif
    printc("0x%08X 0x%08X \r\n", delta_time, current_time);
    setup_tags( (void *)parm_at , cmdline );
    kernel_entry( 0, 3373,/*0x100100*/parm_at);
    while(1);
}
#endif

#if (LOAD_USER_SETTINGS)
#if (DUMP_RAM_EN)
static void dump_ram_info(MMP_UBYTE *data)
{
    MMP_ULONG i;
    for(i = 0; i < 16; i++) {
        RTNA_DBG_Byte(0, data[i]);
    }
    RTNA_DBG_Str(0, "\r\n");
}
#endif

static MMP_BOOL BootLoader_Load_Config(char *name)
{
    MMP_ULONG i;
    FlashCfg_Map_t *map;

    for (i = 0; i < CFG_SIZE; i++) {
        map = &CfgMap[i];
        if (!strcmp(map->name,name)) {
            RTNA_DBG_Str(0, name);
            RTNA_DBG_Str(0, "\r\n");
            if(map->load_dst_addr) {
	            MMPF_SF_FastReadData(map->load_src_addr,map->load_dst_addr , map->size); 
	        }
	        else {
                MMP_SYS_MEM_MAP *mem_map ;
                mem_map = (MMP_SYS_MEM_MAP *)&AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG[48] ;
                while(!mem_map->ubV4L2Base) {
                    RTNA_DBG_Str(0,"@");
                    MMPF_OS_Sleep(1);                 
                }
                if(map->size) {
                    MMPF_SF_FastReadData(map->load_src_addr,mem_map->ubV4L2Base , map->size); 
                }
                else {
                    MMP_ULONG load_size ;                    
                    MMPF_SF_FastReadData(map->load_src_addr,mem_map->ubV4L2Base , 512 );
                    
                    if(get_ih_item(0,(unsigned char *)mem_map->ubV4L2Base)==IH_MAGIC) {
                	    load_size = get_ih_item(3,(unsigned char *)mem_map->ubV4L2Base) ;
                	    load_size = ALIGN512(load_size + sizeof( image_header_t ) ) ;
                        MMPF_SF_FastReadData(map->load_src_addr,mem_map->ubV4L2Base , load_size );
                	    RTNA_DBG_Str(0,"Load ffs...\r\n");
                	    MMPF_DBG_Int(map->load_src_addr, 8);
                	    MMPF_DBG_Int(mem_map->ubV4L2Base, 8);
                	    MMPF_DBG_Int(load_size, 8);
                	    MMPF_DBG_Int( *(unsigned int *)mem_map->ubV4L2Base ,8);
                	    RTNA_DBG_Str(0,"\r\n");
                    }
                    
                    
                }
                //RTNA_DBG_Str(0,"\r\nV4L2Base :");
                //MMPF_DBG_Int(mem_map->ubV4L2Base , 8); 
	        }
	        #if (DUMP_RAM_EN)
            dump_ram_info((MMP_UBYTE *)map->load_dst_addr);
	        #endif
	        return MMP_TRUE;
	    }
	} 
	return MMP_FALSE;
}

#if (DRAM_ID != DRAM_DDR) || ( OTA_FAIL_CHECK)
static void BootLoader_Save_Config(char *name)
{
  MMP_ULONG i;
  FlashCfg_Map_t *map;
  for (i = 0; i < CFG_SIZE; i++) {
    map = &CfgMap[i];
    if (!strcmp(map->name,name)) {
      // only erase 4 KB
      MMPF_SF_EraseSector(map->load_src_addr);
      MMPF_SF_WriteData(map->load_src_addr, map->load_dst_addr, map->size); 
    }
  }
}
#endif

#endif

//------------------------------------------------------------------------------
//  Function    : BootLoader_Task
//  Description : Boot loader main task
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
unsigned divu1000(unsigned n) {
 unsigned q, r, t;
 t = (n >> 7) + (n >> 8) + (n >> 12);
 q = (n >> 1) + t + (n >> 15) + (t >> 11) + (t >> 14);
 q = q >> 9;
 r = n - q*1000;
 return q + ((r + 24) >> 10);
// return q + (r > 999);
}

void BootLoader_Task(void *p_arg)
{
    void (*FW_Entry)(void) = NULL;
    #if (RUN_RTOS_ON_CPU_B)
	AIT_STORAGE_INDEX_TABLE2 *pIndexTable = (AIT_STORAGE_INDEX_TABLE2 *)STORAGE_TEMP_BUFFER;
    MMP_ULONG   ulSifAddr = 0x0 , BootStartAddr = 0;
	MMP_USHORT  usCodeCrcValue = 0x0;
	#endif
	MMP_ERR     err;
    MMP_ULONG   ulDramFreq;
    AITPS_GBL   pGBL = AITC_BASE_GBL;
    MMP_ULONG   kernel_at = NO_AIT_HDR_FW_LOAD_AD ;
    char        *cmdline=0;
    RTNA_DBG_Str0("BootLoader_Task()@0\r\n");
    RTNA_DBG_Str0("BootLoader_Task()\r\n");
    MMPF_PLL_GetGroupFreq(CLK_GRP_DRAM, &ulDramFreq);
    RTNA_DBG_Str0("BootLoader_Task()@1\r\n");
    printc("ulDramFreq %d\r\n", ulDramFreq);
    #if(USE_DIV_CONST)
    ulDramFreq = divu1000(ulDramFreq);
    #else
    ulDramFreq = ulDramFreq/1000;
    #endif

    /* Set PWR_EN high */
    //*(volatile MMP_UBYTE *)0x80006945 |= 0x02;
    //*(volatile MMP_UBYTE *)0x80006605 |= 0x02;
    //*(volatile MMP_UBYTE *)0x80006615 |= 0x02;

    /*DRAM Init*/
    //*(volatile MMP_UBYTE *)0x80006940 |= 0x01;
    //*(volatile MMP_UBYTE *)0x80006600 |= 0x01;
    //*(volatile MMP_UBYTE *)0x80006610 &= ~(0x01);

#if 1 // move to before dram init, need to access flash
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
#endif
    RTNA_DBG_Str0("BootLoader_Task()@2\r\n");
#if (DRAM_ID != DRAM_DDR)
    BootLoader_Load_Config("ddr3");
    MMPF_DRAM_LoadSetting(STORAGE_TEMP_BUFFER);
#endif    
    RTNA_DBG_Str0("BootLoader_Task()@2-1\r\n");
    printc("ulDramFreq %d\r\n", ulDramFreq);
    MMPF_DRAM_ScanNewLockCore(MMPS_System_GetConfig()->stackMemoryType,
                            &(MMPS_System_GetConfig()->ulStackMemorySize),
                            ulDramFreq,
                            MMPS_System_GetConfig()->stackMemoryMode);
    RTNA_DBG_Str0("BootLoader_Task()@2-2\r\n");
    //*(volatile MMP_UBYTE *)0x80006610 |= 0x01;

    /*Bandwidth Configuration*/
    #if (MCI_READ_QUEUE_4 == 1)
    MMPF_MCI_SetQueueDepth(4);
    #endif
	MMPF_MMU_Deinitialize();
#if (DRAM_ID != DRAM_DDR)	
    if (MMPF_DRAM_SaveValidSetting(STORAGE_TEMP_BUFFER) ) {
        BootLoader_Save_Config("ddr3");
    }
#endif
    RTNA_DBG_Str0("BootLoader_Task()@3\r\n");
    #if (LOAD_USER_SETTINGS)
    BootLoader_Load_Config("user");
    #endif

    /* Workaround to avoid access failure of SPI/USB when enable 2nd CPU */
    #if (CPUB_JTAG_DEBUG)
    pGBL->GBL_BOOT_STRAP_CTL = MODIFY_BOOT_STRAP_EN |
                        ARM_JTAG_MODE_EN | ROM_BOOT_MODE |
                        JTAG_CHAIN_MODE_EN | JTAG_CHAIN_CPUB_SET0;
    #else
    pGBL->GBL_BOOT_STRAP_CTL = MODIFY_BOOT_STRAP_EN |
                        ARM_JTAG_MODE_EN | ROM_BOOT_MODE |
                        JTAG_CHAIN_MODE_DIS | JTAG_CHAIN_CPUB_SET0;
    #endif
    pGBL->GBL_BOOT_STRAP_CTL &= ~(MODIFY_BOOT_STRAP_EN);

    #if (RUN_RTOS_ON_CPU_B)
    
    #if MTD_OTA_EN 
    {
        #if OTA_FAIL_CHECK
        MMP_SYS_MEM_MAP *mem_map ;
        mem_map = (MMP_SYS_MEM_MAP *)&AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG[48] ;

    		if(BootLoader_Start_CpuB(0,MMP_TRUE)) {
          mem_map->ubOTAFlag |=  OTA_SET_RECOVER ;		 
          RTNA_DBG_Str(0, "OTA.SetR\r\n"); 
    		}

        
        if(mem_map->ubOTAFlag & OTA_SET_RECOVER ) {
          mem_map->ubOTAFlag &= ~OTA_SET_RECOVER ;
          if( BootLoader_Load_Config("mtdtab_b") ) {  
            BootLoader_Save_Config("mtdtab") ;
            
          }
        } else {
          if( BootLoader_Load_Config("ota_f") ) {
            unsigned char *ota_f = (unsigned char *)STORAGE_TEMP_BUFFER ;

            if( *ota_f==1) {
              mem_map->ubOTAFlag |=  OTA_CHK_RECOVER ;
              RTNA_DBG_Str(0, "OTA.ChkR\r\n"); 
            }  
            else {
              mem_map->ubOTAFlag &= ~OTA_CHK_RECOVER ;
            }
          }  
        }
        
        
        #endif
        
        if( BootLoader_Load_Config("mtdtab") ) {
            mtd_set_desc_table( (char *)STORAGE_TEMP_BUFFER ) ;
            ulSifAddr = mtd_get_active_mtd_offset("boot") ;
            kernel_at = mtd_get_active_mtd_offset("kernel");
            if(!kernel_at) kernel_at = NO_AIT_HDR_FW_LOAD_AD ; 
            cmdline=build_kernel_cmd_line();
            /*
            RTNA_DBG_Str(0,"kernel_at:");
            RTNA_DBG_Long(0,kernel_at);
            RTNA_DBG_Str(0,"\r\n");
            */
            
        }
    }
    #endif
retry:    
    BootStartAddr = ulSifAddr ;
    RTNA_DBG_Str0("BootLoader_Task()@4\r\n");
    printc("m-boot:0x%08X\r\n", ulSifAddr);
    MMPF_SF_FastReadData(ulSifAddr, STORAGE_TEMP_BUFFER, 512);   //MiniBoot Header Table
    ulSifAddr = ulSifAddr + (0x1 << 12)*(pIndexTable->ulTotalSectorsInLayer);
    
    
    MMPF_SF_FastReadData(ulSifAddr, (STORAGE_TEMP_BUFFER), 512); //Bootloader Header Table
    ulSifAddr = ulSifAddr + (0x1 << 12)*(pIndexTable->ulTotalSectorsInLayer);
    
    MMPF_SF_FastReadData(ulSifAddr, (STORAGE_TEMP_BUFFER), 512); //Firmware Header Table

    if ((pIndexTable->ulHeader == INDEX_TABLE_HEADER) && 
        (pIndexTable->ulTail == INDEX_TABLE_TAIL) &&
        (pIndexTable->ulFlag & 0x1))
    {
        RTNA_DBG_PrintLong(3, pIndexTable->bt.ulDownloadDstAddress);
        FW_Entry = (void (*)(void))(pIndexTable->bt.ulDownloadDstAddress);

        #if (CHIP == MCR_V2)
        if (pIndexTable->ulFlag & 0x40000000) { //check bit 30
            MMPF_SF_EnableCrcCheck(MMP_TRUE);
        }
        #endif 

        RTNA_DBG_Str3("SIF downlod start\r\n");

        RTNA_DBG_PrintLong(3, pIndexTable->bt.ulStartBlock);

        MMPF_SF_FastReadData(pIndexTable->bt.ulStartBlock << pIndexTable->ulAlignedBlockSizeShift, 
        						pIndexTable->bt.ulDownloadDstAddress, 
        						pIndexTable->bt.ulCodeSize);

    		#if (CHIP == MCR_V2)
    		if (pIndexTable->ulFlag & 0x40000000) { //check bit 30
    			usCodeCrcValue = MMPF_SF_GetCrc();
    			MMPF_SF_FastReadData(ulSifAddr + 0x1000, STORAGE_TEMP_BUFFER, 2); //Read CRC block value
    			if(*((MMP_USHORT*)STORAGE_TEMP_BUFFER) == usCodeCrcValue) {
    				RTNA_DBG_Str3("CRC pass \r\n");
    			}
    			else {
    				RTNA_DBG_Str(0, "CRC check fail !!! \r\n");
    				RTNA_DBG_PrintShort(0, usCodeCrcValue);
    				RTNA_DBG_PrintShort(0, *((MMP_USHORT*)STORAGE_TEMP_BUFFER));
    				while(1);
    			}
    		}	
    		#endif
    		RTNA_DBG_Str3("SIF downlod End\r\n");
		BootLoader_Start_Rtos(MMP_TRUE,pIndexTable->bt.ulDownloadDstAddress);
  	}
  	else {
  		RTNA_DBG_PrintLong(0, pIndexTable->ulHeader);
  		RTNA_DBG_PrintLong(0, pIndexTable->ulTail);
  		RTNA_DBG_PrintLong(0, pIndexTable->ulFlag);
  		RTNA_DBG_Str(0, "Invalid Index Table!!\r\n");
  		if(BootStartAddr) {
  		    ulSifAddr = 0 ;
  		    goto retry ;
  		}
  		else {
  		    while(1);
  		}
  	}
    #endif

    #if (LOAD_USER_SETTINGS)
    BootLoader_Load_Config("wifi");
    BootLoader_Load_Config("nvram");
    #endif

    #if EMBEDDED_FFS_EN
    BootLoader_Load_Config("ffs");
    #endif   
    #if (RUN_LINUX_ON_CPU_A)
    BootLoader_Start_CpuA(kernel_at,cmdline);
    #else
    while(1) {
        MMPF_OS_Sleep(1000);
        RTNA_DBG_Str(0, "#");
    }
    #endif

    /* Jump PC */
    FW_Entry();

    while (1);
}

