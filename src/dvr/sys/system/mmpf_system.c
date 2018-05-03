//==============================================================================
//
//  File        : mmpf_system.c
//  Description : MMPF_SYS functions
//  Author      : Jerry Tsao
//  Revision    : 1.0
//
//===============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//============================================================================== 

#include "includes_fw.h"
#include "lib_retina.h"
#include "ait_utility.h"
#include "mmp_reg_audio.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_timer.h"

#if defined(ALL_FW)
#include "mmp_reg_display.h"
#include "mmp_reg_hdmi.h"
#endif

#include "mmpf_hif.h"
#include "mmpf_pll.h"
#include "mmpf_dma.h"
#include "mmpf_display.h"
#include "mmpf_system.h"
#include "mmpf_graphics.h"
#include "mmpf_vif.h"
#include "mmpf_pio.h"
#include "mmpf_audio_ctl.h"
#include "mmpf_timer.h"
#include "mmpf_dram.h"
#if defined(ALL_FW)
#include "mmpf_sensor.h"
#include "mmpf_usbphy.h"
#include "mmpf_icon.h"
#include "mmpf_i2cm.h"
#include "mmp_reg_mci.h"
#include "mmpf_monitor.h"
#endif
#include "mmph_hif.h"
#if defined(MINIBOOT_FW)||defined(UPDATER_FW)|| \
    defined(SD_BOOT)||defined(FAT_BOOT)
#include "mmp_reg_pad.h"
#endif

#include "ipc_cfg.h"

/** @addtogroup MMPF_SYS
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#if PLL_CONFIG==PLL_FOR_POWER
#define PLL_VER "(power)"
#endif
#if PLL_CONFIG==PLL_FOR_PERFORMANCE
#define PLL_VER "(performance)"
#endif
#if PLL_CONFIG==PLL_FOR_BURNING
#define PLL_VER "(burning)"
#endif
#if PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192
#define PLL_VER "(ulow192x2)"
#endif
#if PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168
#define PLL_VER "(ulow168x3)"
#endif
#define RTOS_TYPE "(GCC)"
#define RTOS_IPC_VER    "v3.6.3"PLL_VER DDR3_VER RTOS_TYPE  // mapping to video driver first

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

MMP_UBYTE                   gbSystemCoreID;
MMP_UBYTE                   gbEcoVer = 0;
MMP_USHORT                  m_gsISPCoreID;

MMP_SYSTEM_BUILD_TIME 		gFwBuildTime = {__DATE__, __TIME__};

#if !defined(MINIBOOT_FW)
static MMP_ULONG   			m_ulMdlClkRefCnt[MMPF_SYS_CLK_MDL_NUM] = {0, };
#endif

#if defined(MINIBOOT_FW)||defined(UPDATER_FW)||defined(SD_BOOT)||defined(FAT_BOOT)
MMP_SYSTEM_BOOT_PARAM       m_BootParam __attribute__((section (".BootParam")));
#else
extern unsigned char* __DTCM_START__;
MMP_SYSTEM_BOOT_PARAM       *m_BootParam ;//__attribute__((section (".BootParam")));
#endif

typedef struct {
    MMP_ULONG id;
    MMP_ULONG tick;
} MMPF_SYS_TMR_MARKER;

#if !defined(MINIBOOT_FW)
#define MAX_SYS_TMR_MARKER  (40)
static MMP_ULONG            m_TimeMarkerCnt = 0;
static MMPF_SYS_TMR_MARKER  m_TimeMarker[MAX_SYS_TMR_MARKER];
#endif

/*
 * System Heap memory control
 */
#if defined(ALL_FW)
static MMP_BOOL             m_bSysHeapInit  = MMP_FALSE;
static MMP_ULONG            m_ulSysHeapBase[SYS_HEAP_ZONE] = {
    SYS_HEAP_MEM_INVALID,
    SYS_HEAP_MEM_INVALID,
};
static MMP_ULONG            m_ulSysHeapEnd[SYS_HEAP_ZONE] = {
    0,
    0,
};
static MMP_ULONG            m_ulSysHeapPos[SYS_HEAP_ZONE] = {
    SYS_HEAP_MEM_INVALID,
    SYS_HEAP_MEM_INVALID,
};
#endif

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMPF_OS_FLAGID       DSC_UI_Flag;
extern MMPF_OS_FLAGID       SYS_Flag_Hif;

#if defined(ALL_FW)||defined(MBOOT_EX_FW)
extern char* 		    __RESET_END__;
extern char* 			__ALLSRAM_END__;
#endif
#if defined(UPDATER_FW)
extern char* 			__ALLSRAM_END__;
#endif
#if defined(ALL_FW)&&(AGC_SUPPORT == 1)
extern MMP_ULONG            gulAgcUnprocSamples;
#endif

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern void MMPF_MMU_CleanDCache(void);
extern void AT91F_DisableDCache(void);
extern void AT91F_DisableICache(void);
extern void AT91F_DisableMMU(void);

#if defined(ALL_FW)
static void MMPF_SYS_InitHeap(void);
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Local_Function____(){ruturn;} //dummy
#endif

#if defined(ALL_FW)
//------------------------------------------------------------------------------
//  Function    : _SYS_InstallModules
//  Description :
//------------------------------------------------------------------------------
#ifndef __GNUC__    
static void _SYS_InstallModules(void)
{
    MMP_ULONG       __initcall_start = 0, __initcall_end = 0, init_loop = 0;
    ait_initcall_t  *fn = NULL;
    int             ret;
    MMP_SYS_MEM_MAP *mem_map ;
    MMP_ULONG       heap_pos , allocated_size ;
    extern MMP_ULONG Image$$MODULE_INIT$$Base;
    extern MMP_ULONG Image$$MODULE_INIT$$ZI$$Limit;

    MMPF_SYS_InitHeap();

    __initcall_start    = (MMP_ULONG)&Image$$MODULE_INIT$$Base;
    __initcall_end      = (MMP_ULONG)&Image$$MODULE_INIT$$ZI$$Limit;
    
    for (init_loop = __initcall_start; init_loop < __initcall_end; init_loop += sizeof(MMP_ULONG))
    {                               
        fn = (ait_initcall_t *)(init_loop);

        if (fn != NULL) {
            ret = (*fn)();
        }
    }
}
#else
static void _SYS_InstallModules(void)
{
    MMP_ULONG       __initcall_start = 0, __initcall_end = 0, init_loop = 0;
    ait_initcall_t  *fn = NULL;
    int             ret;
    MMP_SYS_MEM_MAP *mem_map ;
    MMP_ULONG       heap_pos , allocated_size ;
    extern char* __MODULE_INIT_START__;
    extern char* __MODULE_INIT_END__;
    MMPF_SYS_InitHeap();

    __initcall_start    = (int)&__MODULE_INIT_START__;
    __initcall_end      = (int)&__MODULE_INIT_END__;

    
    for (init_loop = __initcall_start; init_loop < __initcall_end; init_loop += sizeof(MMP_ULONG))
    {                               
        fn = (ait_initcall_t *)(init_loop);

        if (fn != NULL) {
            ret = (*fn)();
        }
    }
    // overwrite new heap-end and notify to cpu-a linux to construct cpu-b working space
    // cpu-share reg last 16 bytes as share register
    mem_map = (MMP_SYS_MEM_MAP *)&AITC_BASE_CPU_SAHRE->CPU_SAHRE_REG[48] ;
    // simplify OSD address, allocated by cpu-b , not cpub-a
    mem_map->ulOSDPhyAddr = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,4096*2,32);
    // zero data , 4K non-initialized area is for MD osd
    memset((void *)mem_map->ulOSDPhyAddr,0,4096);
    
    heap_pos = MMPF_SYS_HeapTell(SYS_HEAP_DRAM) ;
    allocated_size = heap_pos - m_ulSysHeapBase[SYS_HEAP_DRAM] ;
    // real-adjust heap end 
    m_ulSysHeapEnd[SYS_HEAP_DRAM] = m_ulSysHeapBase[SYS_HEAP_DRAM] + allocated_size ;
    m_ulSysHeapEnd[SYS_HEAP_DRAM] = ALIGN_X(m_ulSysHeapEnd[SYS_HEAP_DRAM],2*1024*1024);
    //printc("*Adj New heap end : 0x%08x\r\n",m_ulSysHeapEnd[SYS_HEAP_DRAM]);
    mem_map->ubV4L2Base     = m_ulSysHeapEnd[SYS_HEAP_DRAM] ;
    mem_map->ubV4L2Size_Mb  = DBUF_SIZE_4_LINUX >> 20 ;
    mem_map->ubResetBase    = gulDramBase ;
    mem_map->ubWorkingSize_Mb = (m_ulSysHeapEnd[SYS_HEAP_DRAM] - gulDramBase) >> 20 ;
    //mem_map->ulOSDPhyAddr   = 0;
    #if V4L2_JPG
    mem_map->ubFlag = JPG_CAPTURE_EN ;
    #else
    mem_map->ubFlag = 0 ;
    #endif
    #if MAX_H264_STREAM_NUM > 2 
    mem_map->ubFlag |= H264_3RD_STREAM_EN; 
    #endif
    //mem_map->ubNonUsed  = 0 ;
    printc("V4L2(0x%08x,%d),flag:%x\r\n",mem_map->ubV4L2Base,mem_map->ubV4L2Size_Mb,mem_map->ubFlag);
    
    #if 0
    printc("CPU-base : 0x%08x,Working size : %d MB, V4L2-base : 0x%08x, V4L2-size : %d MB\r\n",
       mem_map->ubResetBase,
       mem_map->ubWorkingSize_Mb,
       mem_map->ubV4L2Base,
       mem_map->ubV4L2Size_Mb ); 
    printc("OSD-base : 0x%08x\r\n",mem_map->ulOSDPhyAddr );       
    #endif   
}

#endif

#if defined(ALL_FW)
void MemMonitor_Callback(MMPF_MONITOR_CB_ARGU *argu)
{
    RTNA_DBG_Str(0, "Fault: Seg");
    RTNA_DBG_Byte(0, argu->ubSegID);
    if (argu->bFaultWr)
        RTNA_DBG_Str(0, " write");
    if (argu->bFaultRd)
        RTNA_DBG_Str(0, " read");
    RTNA_DBG_Long(0, argu->ulFaultSrc);
    RTNA_DBG_Long(0, argu->ulFaultAddr);
    RTNA_DBG_Byte(0, argu->ubFaultSize);
    RTNA_DBG_Str(0, "\r\n");
}
#endif

//------------------------------------------------------------------------------
//  Function    : _SYS_InitialDriver
//  Description :
//------------------------------------------------------------------------------
static void _SYS_InitialDriver(void)
{
    MMPF_PIO_Initialize();
    MMPF_I2cm_InitDriver();

    #if (DSC_R_EN)||(VIDEO_R_EN)||(VIDEO_P_EN)
    MMPF_DMA_Initialize();
    #endif
    #if (VIDEO_P_EN)||(VIDEO_R_EN)
    MMPF_Graphics_Initialize();
    #endif

    // For default sensor structure
    MMPF_Sensor_LinkFunctionTable();

    MMPF_I2cm_InitDriver();

    #if 0
    MMPF_PIO_Initialize();
    #endif

    #if 0 //defined(ALL_FW)
    MMPF_Monitor_Init();
    if (1) {
        MMPF_MONITOR_SEQ_ATTR attr;

        attr.ulAddrLowBD    = 0x4E00000;
        attr.ulAddrUpBD     = 0x4E00010;
        attr.ulWrAllowSrc   = 0;
        attr.ulRdAllowSrc   = 0;

        MMPF_Monitor_Enable(&attr, MemMonitor_Callback);
    }
    #endif
}
#endif

#if 0
void ____System_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_ResetHModule
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_SYS_ResetHModule(MMPF_SYS_MDL moduletype, MMP_BOOL bResetRegister) ITCMFUNC;
MMP_ERR MMPF_SYS_ResetHModule(MMPF_SYS_MDL moduletype, MMP_BOOL bResetRegister)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;
    AIT_REG_D *plMdlRstEnReg, *plMdlRstDisReg, *plRegRstEnReg;
    MMP_ULONG ulMdlRstVal;

    if (moduletype >= MMPF_SYS_MDL_NUM) {
        RTNA_DBG_Str0("Invalid module in sw reset\r\n");
        return MMP_SYSTEM_ERR_PARAMETER;
    }
    
    if (moduletype < MMPF_SYS_MDL_CPU_PHL) {
        plMdlRstEnReg 	= &pGBL->GBL_SW_RST_EN[0];
        plMdlRstDisReg 	= &pGBL->GBL_SW_RST_DIS[0];
        plRegRstEnReg 	= &pGBL->GBL_REG_RST_EN[0];
        ulMdlRstVal 	= 1 << moduletype;
    }
    else {
        plMdlRstEnReg 	= &pGBL->GBL_SW_RST_EN[1];
        plMdlRstDisReg 	= &pGBL->GBL_SW_RST_DIS[1];
        plRegRstEnReg 	= &pGBL->GBL_REG_RST_EN[1];
        ulMdlRstVal 	= 1 << (moduletype - MMPF_SYS_MDL_CPU_PHL);
    }

    if (bResetRegister) {
        *plRegRstEnReg |= ulMdlRstVal;
    }

    *plMdlRstEnReg = ulMdlRstVal;
    MMPF_PLL_WaitCount(10);
    *plMdlRstDisReg = ulMdlRstVal;

    if (bResetRegister) {
        *plRegRstEnReg &= ~ulMdlRstVal;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_GetFWEndAddr
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_SYS_GetFWEndAddr(MMP_ULONG *ulEndAddr)
{
#if defined(ALL_FW)||defined(MBOOT_EX_FW)||defined(UPDATER_FW)
    #if defined(ALL_FW)||defined(MBOOT_EX_FW) 
    *ulEndAddr = ((int)&__RESET_END__ + RSVD_HEAP_SIZE + 0xFFF) & (~0xFFF);
    #endif
    #if defined(UPDATER_FW)
    *ulEndAddr = ((int)&__ALLSRAM_END__ + 0xFFF) & (~0xFFF);
    #endif

    printc("FW end:0x%08X\r\n", *ulEndAddr);
#else
    *ulEndAddr = 0;
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_GetFWFBEndAddr
//  Description : Return the end address of FW in frame buffer region
//------------------------------------------------------------------------------
MMP_ERR MMPF_SYS_GetFWFBEndAddr(MMP_ULONG *ulEndAddr)
{
#if defined(ALL_FW)||defined(MBOOT_EX_FW) 
    *ulEndAddr = ALIGN4K((int)&__ALLSRAM_END__);
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_ReadCoreID
//  Description : This function Get the code ID
//------------------------------------------------------------------------------
MMP_UBYTE MMPF_SYS_ReadCoreID(void)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;
    #if defined(MINIBOOT_FW)||defined(UPDATER_FW)|| \
        defined(SD_BOOT)||defined(FAT_BOOT)
    AITPS_PAD pPAD  = AITC_BASE_PAD;
    #endif
    MMP_UBYTE bFeature = 0;

    bFeature = (pGBL->GBL_CHIP_VER & GBL_CHIP_FEATURE_MASK & (~GBL_CHIP_APPLICATION_MASK));   

    pGBL->GBL_ID_RD_CTL |= GBL_PROJECT_ID_RD_EN;

    gbSystemCoreID = pGBL->GBL_PROJECT_ID;
    if ((bFeature == 0x08) || (bFeature == 0x04) || (bFeature == 0x0C)) {
        gbSystemCoreID += 0x40;
    }
    pGBL->GBL_ID_RD_CTL &= ~(GBL_PROJECT_ID_RD_EN);

    #if (CHIP == MCR_V2)
    #if defined(MINIBOOT_FW)||defined(UPDATER_FW)|| \
        defined(SD_BOOT)||defined(FAT_BOOT)
    /* The flow to get the ECO version:
     * 1. set PLL0 BIST_EN=0        (0x8000_5D02[1])
     * 2. set PLL0 CTRL_0=1         (0x8000_5D07[0])
     * 3. Update PLL0 Parameter=1   (0x8000_5D00[3])
     * 4. read PLL0 SEND            (0x8000_5D0E[4])
     *    = 0 -> MP version
     *    = 1 -> ECO version
     */
    pGBL->GBL_DPLL0_PWR &= ~(DPLL_BIST_EN);
    pGBL->GBL_DPPL0_PARAM[2] |= 0x01;
    pGBL->GBL_DPLL0_CFG |= DPLL_UPDATE_PARAM;
    gbEcoVer = (pGBL->GBL_DPPL0_PARAM[9] & 0x10) ? 1 : 0;
    pGBL->GBL_DPPL0_PARAM[2] &= ~(0x01);
    pGBL->GBL_DPLL0_CFG |= DPLL_UPDATE_PARAM;

    //For issue: I2C pull-up leakage from 5V to 3.3V 
    pPAD->PAD_IO_CFG_HDMI[0] &= ~(PAD_PULL_UP); //Disable HDMI_SCL pull-up 
    pPAD->PAD_IO_CFG_HDMI[1] &= ~(PAD_PULL_UP); //Disable HDMI_SDA pull-up

    // Pass the ECO version as booting parameter
    m_BootParam.ubEcoVer = gbEcoVer;
    #else
    #ifndef __GNUC__
    m_BootParam = (MMP_SYSTEM_BOOT_PARAM *)&Image$$DTCM_BOOT$$Base;
    #else
    m_BootParam = (MMP_SYSTEM_BOOT_PARAM *)&__DTCM_START__;
    #endif
    // Get the ECO version from booting parameter
    gbEcoVer = m_BootParam->ubEcoVer;
    #endif
    #endif

    m_gsISPCoreID = 888;

    return gbSystemCoreID;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_EnableClock
//  Description :
//------------------------------------------------------------------------------
/** @brief The function enables or disables the specified clock

The function enables or disables the specified clock from the clock type input by programming the
Global controller registers.

@param[in] ulClockType the clock type to be selected
@param[in] bEnable enable or disable the clock
@return It reports the status of the operation.
*/
MMP_ERR MMPF_SYS_EnableClock(MMPF_SYS_CLK clockTypeIdx, MMP_BOOL bEnable)
{
#if !defined(MINIBOOT_FW)
	AITPS_GBL   pGBL = AITC_BASE_GBL;
	MMP_ULONG	ulClockType = 0;
	MMP_ULONG	ulClockType2 = 0;
    AIT_REG_D   *pClkEn  = &pGBL->GBL_CLK_EN[0];
    AIT_REG_D   *pClkEn2 = &pGBL->GBL_CLK_EN[1];
    AIT_REG_D   *pClkDis = &pGBL->GBL_CLK_DIS[0];
    AIT_REG_D   *pClkDis2= &pGBL->GBL_CLK_DIS[1];    

    #if defined(ALL_FW)||defined(MBOOT_EX_FW)
    OS_CRITICAL_INIT();
    #endif

    if (clockTypeIdx >= MMPF_SYS_CLK_MDL_NUM) {
        PRINTF("Invalid module in clock control\r\n");
        return MMP_SYSTEM_ERR_PARAMETER;
    }

    if (bEnable) {

        #if defined(ALL_FW)||defined(MBOOT_EX_FW)
        OS_ENTER_CRITICAL();
        #endif

        if (m_ulMdlClkRefCnt[clockTypeIdx] == 0) {
                            
            if (clockTypeIdx < MMPF_SYS_CLK_GRP0_NUM) {
                ulClockType = 1 << clockTypeIdx;
                *pClkEn |= ulClockType;
            }
            else if (clockTypeIdx < MMPF_SYS_CLK_MDL_NUM) {
                ulClockType2 = 1 << (clockTypeIdx - MMPF_SYS_CLK_GRP0_NUM);
                *pClkEn2 |= ulClockType2;
            }
        }
        m_ulMdlClkRefCnt[clockTypeIdx]++;

        #if defined(ALL_FW)||defined(MBOOT_EX_FW)
        OS_EXIT_CRITICAL();
        #endif
    }
    else {
    	
    	#if (CHIP == MCR_V2) //EROY TBD : For Raw Preview
    	if(	clockTypeIdx == MMPF_SYS_CLK_RAW_F  || 
    		clockTypeIdx == MMPF_SYS_CLK_RAW_S0 ||
    		clockTypeIdx == MMPF_SYS_CLK_RAW_S1 )
    	{
    		return MMP_ERR_NONE;
    	}
    	#endif

        #if defined(ALL_FW)||defined(MBOOT_EX_FW)
        OS_ENTER_CRITICAL();
        #endif

        if (m_ulMdlClkRefCnt[clockTypeIdx]) {
            m_ulMdlClkRefCnt[clockTypeIdx]--;
            
            if (m_ulMdlClkRefCnt[clockTypeIdx] == 0) {
                if (clockTypeIdx < MMPF_SYS_CLK_GRP0_NUM) {
                    ulClockType = 1 << clockTypeIdx;
                    *pClkDis |= ulClockType;
                }
                else if (clockTypeIdx < MMPF_SYS_CLK_MDL_NUM) {
                    ulClockType2 = 1 << (clockTypeIdx - MMPF_SYS_CLK_GRP0_NUM);
                    *pClkDis2 |= ulClockType2;
                }
            }
        }

        #if defined(ALL_FW)||defined(MBOOT_EX_FW)
        OS_EXIT_CRITICAL();
        #endif
    }
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_CheckClockEnable
//  Description :
//------------------------------------------------------------------------------
/** @brief The function check the specified clock enable status

@param[in] ulClockType the clock type to be selected
@return It reports the the clock enable status.
*/
MMP_BOOL MMPF_SYS_CheckClockEnable(MMPF_SYS_CLK clockTypeIdx)
{
#if !defined(MINIBOOT_FW)
	AITPS_GBL   pGBL = AITC_BASE_GBL;
	MMP_ULONG	ulClockType = 0;
	MMP_ULONG	ulClockType2 = 0;
    AIT_REG_D   *pClkEn  = &pGBL->GBL_CLK_DIS[0];
    AIT_REG_D   *pClkEn2 = &pGBL->GBL_CLK_DIS[1];  

    if (clockTypeIdx >= MMPF_SYS_CLK_MDL_NUM) {
        PRINTF("Invalid module in clock control\r\n");
        return MMP_FALSE;
    }

   	if (clockTypeIdx < MMPF_SYS_CLK_GRP0_NUM) {
        ulClockType = 1 << clockTypeIdx;
        if (*pClkEn & ulClockType)
        	return MMP_FALSE;
    }
    else if (clockTypeIdx < MMPF_SYS_CLK_MDL_NUM) {
        ulClockType2 = 1 << (clockTypeIdx - MMPF_SYS_CLK_GRP0_NUM);
        if (*pClkEn2 & ulClockType2)
        	return MMP_FALSE;	
    }
#endif

    return MMP_TRUE;
}

#if defined(ALL_FW)
//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_PowerSaving
//  Description :
//------------------------------------------------------------------------------
/** @brief The function power down unused analog PHY to save power consumption

@return None.
*/
void MMPF_SYS_PowerSaving(void)
{
    AITPS_TV pTV = AITC_BASE_TV;
    AITPS_HDMI_PHY pHDMI = AITC_BASE_HDMI_PHY;

    /* Power down TV */
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_TV, MMP_TRUE);
    pTV->TVIF_DAC_IF_1ST_CTL &= ~(TV_DAC_POWER_DOWN_EN|TV_BGREF_POWER_DOWN_EN);
    pTV->TVENC_MODE_CTL &= ~(TV_ENCODER_EN);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_TV, MMP_FALSE);

    /* Power down HDMI */
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_HDMI, MMP_TRUE);
    pHDMI->HDMI_PHY_TMDS_CTL[0] |= (TDMS_BUF_POWER_DOWN );
    pHDMI->HDMI_PHY_PLL_CTL[0] |= HDMI_PHY_PWR_DOWN_PLL;
    pHDMI->HDMI_PHY_BANDGAP_CTL |= (BG_PWR_DOWN | BS_PWR_DOWN);
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_HDMI, MMP_FALSE);

    /* Power down USB PHY */
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_USB, MMP_TRUE);
    MMPF_USBPHY_PowerDown();
    MMPF_SYS_EnableClock(MMPF_SYS_CLK_USB, MMP_FALSE);
}
#endif

#if 0
void ____Timer_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_StartTimerCount
//  Description : This function set Timer5 triggered time in 1/12 us.
//                MMPF_SYS_WaitUs get timer5 count value to calculate delay time.
//                MMPF_SYS_GetTimerCount get timer5 count value and calculate
//                time interval in micro-second.
//------------------------------------------------------------------------------
MMP_ERR MMPF_SYS_StartTimerCount(void)
{
#if !defined(MINIBOOT_FW)
   	AITPS_TC pTC = (AITPS_TC)GetpTC(MMPF_TIMER_5);

    pTC->TC_CTL 	= TC_CLK_DIS;
    pTC->TC_INT_DIS = TC_COMP_VAL_HIT;

    pTC->TC_SR 		= pTC->TC_SR;
    pTC->TC_MODE 	= TC_CLK_MCK;

    pTC->TC_COMP_VAL = 0;
    pTC->TC_CTL 	= TC_CLK_EN | TC_SW_TRIG;
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_WaitUs
//  Description : This function can set time delay in micro-second. When the function
//                progress, it will continue check call back funtion return value.
//                If return value is true, leave function. If false wait counting until
//                time arrive
//------------------------------------------------------------------------------
/** @brief
@param[in]  ulus        The time delay value in micro-second
@param[out] *bPollingDone The call back function return value
@param[in]  *Func       Call back function
@return It reports the status of the operation.
*/
MMP_ERR MMPF_SYS_WaitUs(MMP_ULONG ulus)
{
#if !defined(MINIBOOT_FW)
    AITPS_TC pTC = (AITPS_TC)GetpTC(MMPF_TIMER_5);
    MMP_ULONG ulcount;
    MMP_ULONG ulReadTCCVR;
    MMP_ULONG ulTargetTCCVR;

    // 1 timer tick = 1/(G0 slow_clk freq.) sec
    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulcount);
    /* #if(USE_DIV_CONST) */
    /* #if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168) */
    /* ulcount = 168000;		//	CLK_GRP_GBL    168MHz					 */
    /* #endif */
    /* #if (PLL_CONFIG==PLL_FOR_POWER)||(PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192) || (PLL_CONFIG==PLL_FOR_PERFORMANCE) || (PLL_CONFIG==PLL_FOR_BURNING) */
    /* ulcount = 264000;     //	CLK_GRP_GBL    264MHz					 */
    /* #endif */
    /* #endif */

    ulcount = (ulcount/1000) >> 1;
    ulcount *= ulus;

    ulReadTCCVR = pTC->TC_COUNT_VAL;

	// EROY CHECK    
    if ((~ulReadTCCVR) >= ulcount) {

        ulTargetTCCVR = ulReadTCCVR + ulcount;
        
        while((pTC->TC_COUNT_VAL < ulTargetTCCVR)) {
            if (pTC->TC_COUNT_VAL < ulReadTCCVR) {
                break;
            }
        }
    }
    else{

        ulTargetTCCVR = ulcount-(~ulReadTCCVR)-1;

        while(pTC->TC_COUNT_VAL >= ulReadTCCVR){
        }
        while(pTC->TC_COUNT_VAL < ulTargetTCCVR){
        }
    }
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_GetTimerCount
//  Description : Get timer5 count value.
//                The interval for ulus is micro-second.
//                This timer counter counts up to maximum value will reset to 0
//                and re-conut to maximum value
//                0 ~ 357913941.25 us
//------------------------------------------------------------------------------
MMP_ULONG MMPF_SYS_GetTimerCount(void)
{
#if !defined(MINIBOOT_FW)
    AITPS_TC pTC = (AITPS_TC)GetpTC(MMPF_TIMER_5);

    return pTC->TC_COUNT_VAL;
#else
    return 0;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_AddTimerMark
//  Description : Set a timer mark with the specified ID.
//------------------------------------------------------------------------------
void MMPF_SYS_AddTimerMark(MMP_ULONG id)
{
#if !defined(MINIBOOT_FW)
    if ((m_TimeMarkerCnt + 1) < MAX_SYS_TMR_MARKER) {
        AITPS_TC pTC = (AITPS_TC)GetpTC(MMPF_TIMER_5);

        m_TimeMarker[m_TimeMarkerCnt].id = id;
        m_TimeMarker[m_TimeMarkerCnt].tick = pTC->TC_COUNT_VAL;
        m_TimeMarkerCnt++;
    }
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_DumpTimerMark
//  Description : Dump all timer mark
//------------------------------------------------------------------------------
void MMPF_SYS_DumpTimerMark(void)
{
#define DUMP_MARKER     (0)
#if !defined(MINIBOOT_FW) && (DUMP_MARKER==1)
    MMP_ULONG i;

    RTNA_DBG_Str(0, "-----------------------------------\r\n");
    for(i = 0; i < m_TimeMarkerCnt; i++) {
        /* MMPF_DBG_Int(m_TimeMarker[i].id, -3); */
        printc("%d \t",m_TimeMarker[i].id); 
        /* RTNA_DBG_Str(0, "\t"); */
        MMPF_DBG_Int(m_TimeMarker[i].tick, -10);
        RTNA_DBG_Str(0, "\t");
        printc("%08d \r\n",m_TimeMarker[i].tick);
    }
    RTNA_DBG_Str(0, "-----------------------------------\r\n");
#endif
}

#if defined(ALL_FW)
//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_InitHeap
//  Description : Initialize system heap
//------------------------------------------------------------------------------
static void MMPF_SYS_InitHeap(void)
{
    if (!m_bSysHeapInit) {
        IPC_Cfg_Init();

        /* Heap memory in SRAM */
        m_ulSysHeapBase[SYS_HEAP_SRAM]  = gulSramBufBase;
        m_ulSysHeapEnd[SYS_HEAP_SRAM]   = gulSramBufEnd;
        m_ulSysHeapPos[SYS_HEAP_SRAM]   = m_ulSysHeapBase[SYS_HEAP_SRAM];

        /* Heap memory in DRAM */
        m_ulSysHeapBase[SYS_HEAP_DRAM]  = gulDramBufBase;
        m_ulSysHeapEnd[SYS_HEAP_DRAM]   = gulDramBufEnd;
        m_ulSysHeapPos[SYS_HEAP_DRAM]   = m_ulSysHeapBase[SYS_HEAP_DRAM];

        m_bSysHeapInit = MMP_TRUE;
        
    }
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_HeapMalloc
//  Description : Allocate memory from heap
//------------------------------------------------------------------------------
MMP_ULONG MMPF_SYS_HeapMalloc(MMP_UBYTE zone, MMP_ULONG size, MMP_ULONG align)
{
#if defined(ALL_FW)
    MMP_ULONG addr;
    OS_CRITICAL_INIT();

    if (!m_bSysHeapInit || (zone >= SYS_HEAP_ZONE))
        return SYS_HEAP_MEM_INVALID;

    OS_ENTER_CRITICAL();
    addr = ALIGN_X(m_ulSysHeapPos[zone], align);
    if ((addr + size) < m_ulSysHeapEnd[zone])
        m_ulSysHeapPos[zone] = addr + size;
    else
        addr = SYS_HEAP_MEM_INVALID;
    OS_EXIT_CRITICAL();

    if (addr == SYS_HEAP_MEM_INVALID) {
        RTNA_DBG_Str0("Fail to acquire memory from zone");
        RTNA_DBG_Byte0(zone);
        RTNA_DBG_Str0(" with");
        RTNA_DBG_Long0(size);
        RTNA_DBG_Str0("-byte\r\n");
    }
    //printc("%s : size : 0x%0x\r\n",(zone==SYS_HEAP_DRAM)?"DRAM":"SRAM",size );
    return addr;
#else
    return SYS_HEAP_MEM_INVALID;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_HeapTell
//  Description : Tell the current position of heap
//------------------------------------------------------------------------------
MMP_ULONG MMPF_SYS_HeapTell(MMP_UBYTE zone)
{
#if defined(ALL_FW)
    if (!m_bSysHeapInit || (zone >= SYS_HEAP_ZONE))
            return SYS_HEAP_MEM_INVALID;

    return m_ulSysHeapPos[zone];
#else
    return SYS_HEAP_MEM_INVALID;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SYS_Task
//  Description : SYS Task Function

void MMPF_SYS_Task(void)
{
#if !defined(MINIBOOT_FW)
    MMPF_OS_FLAGS   flags;
    AITPS_GBL pGBL = AITC_BASE_GBL;
    
    MMPF_SYS_ReadCoreID();
    printc("SYS_Task[%X] "RTOS_IPC_VER"\r\n",gbSystemCoreID);
    //printc("%d time %d\r\n",pGBL->GBL_CLK_DIV, OSTime);
//    while (TRUE) {
//        printc("MMPF_SYS_Task\r\n");
//    };

    #if defined(ALL_FW)
    _SYS_InstallModules();
    if (0) {
    _SYS_InitialDriver();
    }
    #endif

    while (TRUE) {

        MMPF_OS_WaitFlags(SYS_Flag_Hif, 
                        #if defined(ALL_FW)&&(AGC_SUPPORT == 1)
                        SYS_FLAG_SYS|SYS_FLAG_AGC,
                        #else
                        SYS_FLAG_SYS,
                        #endif                        
                    	MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME, 0, &flags);

        #if defined(ALL_FW) && (AGC_SUPPORT)
        if (flags & SYS_FLAG_AGC) {
            while(gulAgcUnprocSamples >= gsAGCBlockSize) {
                AGC_Proc();
            }
        }
        #endif
    }
#endif // !defined(MINIBOOT_FW)
}

/** @}*/ //end of MMPF_SYS
