//==============================================================================
//
//  File        : bsp.c
//  Description : Board support package source code
//  Author      : Jerry Tsao
//  Revision    : 1.0
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmpf_pll.h"

/** @addtogroup BSP
@{
*/

#define USING_PROTECT_MODE                      //[jerry] for debugger variant

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define  IRQ_STK_SIZE           512     // entry count
#define  FIQ_STK_SIZE           8

#ifdef ARMUL
MMP_ULONG   TickCtr;
#endif // ARMUL

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

/* MMP_ULONG   *pIRQStkTop; */
/* MMP_ULONG   *pFIQStkTop; */
/* MMP_ULONG   *pIRQStkBottom; */
/* MMP_ULONG   *pFIQStkBottom; */

ARM_EXCEPTION_HANDLERCB gARMExceptionHandlerCB = 0;

static MMP_ULONG   sysTicks;
//==============================================================================
//
//                              PROTOTYPES
//
//==============================================================================

/* void  SetStackPointers(void); */

#ifdef ARMUL
void  INT_Initialize(void);
#endif

extern void MMPF_TC0_Start(IntHandler pfHandler);
extern void MMPF_TC0_ClearSr(void);

MMP_ULONG MMPF_BSP_GetTick()
{
    return sysTicks ;
}

#if !defined(MINIBOOT_FW)
#ifdef __GNUC__
#define __inline	inline
#endif

#if (ITCM_PUT)
void __inline RTNA_AIC_Open(AITPS_AIC pAIC, MMP_ULONG id, IntHandler pfHandler, MMP_ULONG type) ITCMFUNC;
void __inline RTNA_AIC_IRQ_En(AITPS_AIC pAIC, MMP_ULONG id) ITCMFUNC;
void __inline RTNA_AIC_IRQ_Dis(AITPS_AIC pAIC, MMP_ULONG id) ITCMFUNC;
void __inline RTNA_AIC_IRQ_DisAll(AITPS_AIC pAIC) ITCMFUNC;
void __inline RTNA_AIC_IRQ_ClearAll(AITPS_AIC pAIC) ITCMFUNC;
MMP_ERR MMPF_BSP_TimerHandler(void) ITCMFUNC;
void __inline RTNA_AIC_IRQ_EOI(AITPS_AIC pAIC) ITCMFUNC;
MMP_ERR MMPF_BSP_IntHandler(void) ITCMFUNC;
MMP_ERR MMPF_BSP_InitializeTimer(void) ITCMFUNC;
#endif

void __inline RTNA_AIC_Open(AITPS_AIC pAIC, MMP_ULONG id, IntHandler pfHandler, MMP_ULONG type)
{
    // Disable Interrupt of the specified ID on AIC
    if (id < 32)
        pAIC->AIC_IDCR_LSB = 0x1 << id;
    else
        pAIC->AIC_IDCR_MSB = 0x1 << (id - 32);

    // Setup the vector for the specified interrupt
    pAIC->AIC_SVR[id] = (MMP_ULONG)pfHandler;
    // Setup the priority/trigger mode
    pAIC->AIC_SMR[id] = type;
}

void __inline RTNA_AIC_IRQ_En(AITPS_AIC pAIC, MMP_ULONG id)
{
    // Enable interrupt at AIC level
    if (id < 32)
        pAIC->AIC_IECR_LSB = 0x1 << id;
    else
        pAIC->AIC_IECR_MSB = 0x1 << (id - 32);
}

void __inline RTNA_AIC_IRQ_Dis(AITPS_AIC pAIC, MMP_ULONG id)
{
    // Disable timer interrupt at AIC level
    if (id < 32)
        pAIC->AIC_IDCR_LSB = 0x1 << id;
    else
        pAIC->AIC_IDCR_MSB = 0x1 << (id - 32);
}

void __inline RTNA_AIC_IRQ_DisAll(AITPS_AIC pAIC)
{
    // Disable timer interrupt at AIC level
    pAIC->AIC_IDCR_LSB = 0xFFFFFFFF;
    pAIC->AIC_IDCR_MSB = 0xFFFFFFFF;
}

void __inline RTNA_AIC_IRQ_ClearAll(AITPS_AIC pAIC)
{
    // Clear ALL interrups if any pending
    pAIC->AIC_ICCR_LSB = 0xFFFFFFFF;
    pAIC->AIC_ICCR_MSB = 0xFFFFFFFF;
}

void __inline RTNA_AIC_IRQ_EOI(AITPS_AIC pAIC)
{
    // Support both protect mode and non-protect mode
    pAIC->AIC_IVR = pAIC->AIC_IVR;
    pAIC->AIC_EOICR = pAIC->AIC_EOICR;
}
#endif
//------------------------------------------------------------------------------
//  Function    : MMPF_BSP_Initialize
//  Description : Initialize BSP hardware configuration.
//------------------------------------------------------------------------------

//MMP_ERR MMPF_BSP_Initialize(void) ITCMFUNC;
MMP_ERR MMPF_BSP_Initialize(void)
{
    //[jerry] declare 32bit for compiler alignment.
    /* static MMP_ULONG    IRQStackPool[IRQ_STK_SIZE]; */
    /* static MMP_ULONG    FIQStackPool[FIQ_STK_SIZE]; */

    /* pIRQStkTop = &IRQStackPool[IRQ_STK_SIZE - 1]; */
    /* pFIQStkTop = &FIQStackPool[FIQ_STK_SIZE - 1]; */
    /* pIRQStkBottom = IRQStackPool; */
    /* pFIQStkBottom = FIQStackPool; */

    #if !defined(MINIBOOT_FW)
    MMPF_BSP_InitializeInt(); /* Initialize the interrupt controller */
    /* SetStackPointers();       [> Initialize the default and exception stacks <] */
    #endif
    
    RTNA_Init();

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BSP_InitializeInt
//  Description : Initialize(reset) BSP interrupts
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)

//MMP_ERR MMPF_BSP_InitializeInt(void) ITCMFUNC;
MMP_ERR MMPF_BSP_InitializeInt(void)
{
    int i;

    AITPS_AIC pAIC = AITC_BASE_AIC;

    RTNA_AIC_IRQ_DisAll(pAIC);                  // Disable ALL interrupts
    RTNA_AIC_IRQ_ClearAll(pAIC);                // Clear ALL interrups if any pending

    #ifdef USING_PROTECT_MODE
    pAIC->AIC_DBR = AIC_DBG_EN;
    #else
    pAIC->AIC_DBR = 0;                          // Disable Protect Mode
    #endif

    for (i = 0; i < 8; i++) {
        RTNA_AIC_IRQ_EOI(pAIC);                 // End of all pending interrupt.
    }

    // 0xE51FFF20 is opcode of (ldr pc,[pc,#-0xf20])
    *(MMP_ULONG *)0x00000018L = 0xE51FFF20;     // IRQ exception vector

    return MMP_ERR_NONE; //[TBD]
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BSP_InitializeTimer
//  Description : Initialize hardware tick timer interrupt
//------------------------------------------------------------------------------
extern void time_primer(void);
MMP_ERR MMPF_BSP_InitializeTimer(void) 
{
    // System Timer initialization
    // Initialize TC0 to generate 10000 tick/sec
    MMPF_TC0_Start(time_primer);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BSP_TimerHandler
//  Description : BSP tick timer interrupt handler
//------------------------------------------------------------------------------
MMP_ERR MMPF_BSP_TimerHandler(void)
{
    AITPS_AIC pAIC = AITC_BASE_AIC;
	
	sysTicks++ ;
	
	#if !defined(MINIBOOT_FW)
    OSTimeTick();           // call OSTimeTick()

    #if (OS_TMR_EN > 0)
    OSTmrSignal();
    #endif
	#endif // !defined(MINIBOOT_FW)
	
    #ifdef USING_PROTECT_MODE
    pAIC->AIC_IVR = 0x0;    // Write IVR to end interrupt (protect mode used)
    #endif

    MMPF_TC0_ClearSr();     // clear interrupt status register

    // Disable TC0 Interrupt on AIC
    #if (CHIP == MCR_V2)
    pAIC->AIC_ICCR_LSB = 0x1 << AIC_SRC_TC0;
    #endif

    pAIC->AIC_EOICR = 0x0;  // End of interrupt handler

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_BSP_IntHandler
//  Description : BSP interrupt handler
//------------------------------------------------------------------------------
MMP_ERR MMPF_BSP_IntHandler(void)
{
    return MMPF_BSP_TimerHandler();
}

//------------------------------------------------------------------------------
// Function    : MMPF_Register_ARM_ExceptionHandlerCB
// Description : ARM exception handler
//------------------------------------------------------------------------------
MMP_ERR MMPF_Register_ARM_ExceptionHandlerCB(ARM_EXCEPTION_HANDLERCB pARMExceptionHandlerCB)
{
    gARMExceptionHandlerCB = pARMExceptionHandlerCB;
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : ARM_ExceptionHandler
//  Description : ARM exception handler
//------------------------------------------------------------------------------
void ARM_ExceptionHandler(MMP_ULONG exceptID, MMP_ULONG lr)
{
    switch(exceptID) {
    case 0x04:
        RTNA_DBG_Str0("Undefined Instruction at");
        break;
    case 0x08:
        RTNA_DBG_Str0("Software Interrupt at");
        break;
    case 0x0C:
        RTNA_DBG_Str0("Prefetch Abort at");
        break;
    case 0x10:
        RTNA_DBG_Str0("Data Abort at");
        break;
    default:
        RTNA_DBG_Str0("Unknow Exception at");
        break;
    }
    RTNA_DBG_Long(0, lr);
    RTNA_DBG_Str(0, "\r\n");

    if (gARMExceptionHandlerCB) {
        (*gARMExceptionHandlerCB)();
    }

    while(1);
}
#endif // !defined(MINIBOOT_FW)

/** @} */ // end of BSP
