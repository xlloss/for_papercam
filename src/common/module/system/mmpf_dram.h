#ifndef _MMPF_DRAM_H_
#define _MMPF_DRAM_H_

/// @ait_only
/** @addtogroup MMPF_System
 *  @{
 */
//==============================================================================
//
//                              COMPILER OPTION
//
//==============================================================================

#define DRAM_DDR       (0)
#define DRAM_DDR2      (1)
#define DRAM_DDR3      (2)

#if (DRAM_ID == DRAM_DDR3)
    #define AUTO_DLL_LOCK       (1)
#if (DRAM_SIZE == 0xF000000)
    #define SETTING_FOR_DDR3_Q  (0)// For 8428Q, 8328Q
#else
    #define SETTING_FOR_DDR3_Q  (1)// For 8428Q, 8328Q
#endif
    #if (SETTING_FOR_DDR3_Q)&&(DRAM_SIZE != 0x8000000)
        #error Incorrect DRAM size selected for 8x28Q
    #endif
#else
    #define AUTO_DLL_LOCK       (1)
    #define SETTING_FOR_DDR3_Q  (0) // Must be 0
#endif

#if SETTING_FOR_DDR3_Q==1
    #define DDR3_Q_PATCH    (3)
#else
    #define DDR3_Q_PATCH    (0)
#endif
#if DDR3_Q_PATCH==0
    #define DDR3_VER        ""
#elif DDR3_Q_PATCH==1
    #define DDR3_VER        ".p1"
#elif DDR3_Q_PATCH==2
    #define DDR3_VER        ".p2"
#elif DDR3_Q_PATCH==3
    #define DDR3_VER        ".p3"
#endif    

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef enum _MMPF_DRAM_TYPE
{
    MMPF_DRAM_TYPE_NONE = 0,	// no stack memory
    MMPF_DRAM_TYPE_1,			// first used
    MMPF_DRAM_TYPE_2,			// second used
    MMPF_DRAM_TYPE_3,			// third used
    MMPF_DRAM_TYPE_EXT,
    MMPF_DRAM_TYPE_AUTO
} MMPF_DRAM_TYPE;
 
typedef enum _MMPF_DRAM_MODE
{
    MMPF_DRAM_MODE_SDRAM = 0,	// SD RAM
    MMPF_DRAM_MODE_DDR,			// DDR RAM
    MMPF_DRAM_MODE_DDR2,
    MMPF_DRAM_MODE_DDR3,
    MMPF_DRMA_MAX_MODE
} MMPF_DRAM_MODE;

typedef enum _MMPF_DRAM_ID
{
    MMPF_DRAM_256Mb_WINBOND = 0,
    MMPF_DRAM_256Mb_PIECEMK = 1,
    MMPF_DRAM_128Mb_WINBOND = 2,
    MMPF_DRAM_128Mb_FIDELIX = 4,
    MMPF_DRAM_512Mb_WINBOND = 3,
    MMPF_DRAM_512Mb_PIECEMK = 5
} MMPF_DRAM_ID;

typedef struct __attribute__((__packed__))  _MMP_DRAM_CLK_DLY_SET {
    MMP_USHORT ubClock;
    MMP_USHORT usDelay;
} MMP_DRAM_CLK_DLY_SET;

typedef struct {
	MMP_USHORT  usDQRD_DLY;
	MMP_USHORT  usMIN_CLKDLY;
	MMP_USHORT  usMAX_CLKDLY;
} MMPF_DRAM_CHKDLYINFO;

#if (SETTING_FOR_DDR3_Q)
typedef struct {
    MMP_ULONG   idx_low;
    MMP_ULONG   idx_up;
    MMP_ULONG   idx_opt;
    MMP_ULONG   idx_sel;
    MMP_ULONG   num;
} DDR3_DLY_DEBUG;
#endif

typedef enum _DDR3_DATA_ID
{
    DDR3_RDDQ = 0,
    DDR3_WRLVL,
    DDR3_ASYNCRD0,
    DDR3_ASYNCRD1,
    DDR3_DELAY,
    DDR3_DATA_ID_MAX
} DDR3_DATA_ID ;

typedef struct {
    MMP_USHORT sig ;
    MMP_USHORT val[DDR3_DATA_ID_MAX];
    MMP_UBYTE  noused[16- (DDR3_DATA_ID_MAX+1) * 2 ] ;
    
} DDR3_USER_SETTING ;

#if (DRAM_SIZE == 0x8000000)
#define SIG_ID 0x55aa
#else
#define SIG_ID 0xaa55
#endif

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ERR MMPF_DRAM_Initialize(MMPF_DRAM_TYPE dramtype, MMP_ULONG *ulSize, MMP_ULONG ulClock, MMPF_DRAM_MODE drammode);
MMP_ERR MMPF_DRAM_SendCommand(MMP_USHORT usCmd);
void    MMPF_DRAM_InitSettings(void);
MMP_ERR MMPF_DRAM_GetStackSize(MMP_ULONG *ulSize);
MMP_ERR MMPF_DRAM_SetPowerDown(MMP_BOOL bEnterPowerDown);
MMP_ERR MMPF_DRAM_SetSelfRefresh(MMP_BOOL bEnterSelfRefresh);
MMP_ERR MMPF_DRAM_ConfigPad(MMP_BOOL bEnterPowerDown);
MMP_ERR MMPF_DRAM_ConfigClock(MMP_ULONG ulClock, MMP_ULONG ulWaitCnt);
void MMPF_DRAM_SendInitCmd(void);

#if (AUTO_DRAM_LOCKCORE)&&(defined(MBOOT_FW)||defined(UPDATER_FW))
MMP_ERR MMPF_DRAM_ScanNewLockCore(MMPF_DRAM_TYPE dramtype, MMP_ULONG *ulSize, MMP_ULONG ulClock, MMPF_DRAM_MODE drammode);
#endif
void MMPF_DRAM_LoadSetting(MMP_ULONG addr);
MMP_BOOL MMPF_DRAM_SaveValidSetting(MMP_ULONG addr);

/// @}
#endif

/// @end_ait_only

