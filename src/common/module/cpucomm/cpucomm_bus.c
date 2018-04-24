//==============================================================================
//
//  File        : cpucomm_core.c
//  Description : CPU communication between dual CPU
//  Author      : Chiket Lin
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================
#include "cpucomm_bus.h"
#include "lib_retina.h"

#ifdef __GNUC__
#define __inline	inline
#endif

//------------------------------------------------------------------------------
//  Function    : CpuComm_IsrEnable
//  Description : Enable ISR
//------------------------------------------------------------------------------
static __inline void CpuComm_IsrEnable( _CPU_ID ulCpuId )
{
    extern void cpu_isr_b(void);

    AITPS_AIC   pAIC    = AITC_BASE_AIC;

    // Clear IRQ flag
    MMP_CPUCOMM_IRQ_CLEAR( ulCpuId );

    // Register and enable IRQ
	RTNA_AIC_Open(pAIC, AIC_SRC_CPU2CPU, cpu_isr_b, AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
    RTNA_AIC_IRQ_En(pAIC, AIC_SRC_CPU2CPU);
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_IsrDisable
//  Description : Disable ISR
//------------------------------------------------------------------------------
static __inline void CpuComm_IsrDisable( _CPU_ID ulCpuId )
{
    AITPS_AIC   pAIC = AITC_BASE_AIC;

    // Clear IRQ flag
    MMP_CPUCOMM_IRQ_CLEAR( ulCpuId );

    // Disable IRQ
    RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_CPU2CPU);

    // Clear IRQ flag
    MMP_CPUCOMM_IRQ_CLEAR( ulCpuId );
}


//------------------------------------------------------------------------------
//  Function    : CpuComm_Init
//  Description : Init the comm table
//------------------------------------------------------------------------------
CPU_COMM_ERR CpuComm_Init(void)
{
#ifdef __GNUC__
	extern unsigned char	__CPU_COMM_MODULE_INIT_START__;
	extern unsigned char	__CPU_COMM_MODULE_INIT_END__;

	MMP_ULONG				i, ulCount = (MMP_ULONG) (&__CPU_COMM_MODULE_INIT_END__ - &__CPU_COMM_MODULE_INIT_START__) /sizeof(CPUCOMM_MODULE_FUN);
	CPUCOMM_MODULE_FUN		*pfTable = (void *) (&__CPU_COMM_MODULE_INIT_START__);
#else
	extern CPUCOMM_MODULE_FUN Image$$CPU_COMM_MODULE_INIT$$Base;
	extern MMP_ULONG Image$$CPU_COMM_MODULE_INIT$$Length;

MMP_ULONG			i, ulCount = (MMP_ULONG) (&Image$$CPU_COMM_MODULE_INIT$$Length) / sizeof(CPUCOMM_MODULE_FUN);
	CPUCOMM_MODULE_FUN	*pfTable = (&Image$$CPU_COMM_MODULE_INIT$$Base);
#endif
	CPU_COMM_ERR		ulRet;

	// No entry in the table, nothing to init
	if(pfTable == NULL || ulCount == 0)
	{
		return CPU_COMM_ERR_INIT_FAIL;
	}

	// Init HW
	CpuComm_HwInit(_CPU_ID_B);
	CpuComm_IsrEnable(_CPU_ID_B);

	// call moudle init
	for(i = 0; i < ulCount; i++)
	{
		ulRet = pfTable[i]();
		if(ulRet != CPU_COMM_ERR_NONE) goto init_fail;
	}

	return CPU_COMM_ERR_NONE;

init_fail:
     CpuComm_IsrDisable( _CPU_ID_B );

	return ulRet;
}

//------------------------------------------------------------------------------
//  Function    : CpuComm_Destroy
//  Description : Destroy the comm table
//------------------------------------------------------------------------------
void CpuComm_Destroy(void)
{
#ifdef __GNUC__
	extern unsigned char	__CPU_COMM_MODULE_EXIT_START__;
	extern unsigned char	__CPU_COMM_MODULE_EXIT_END__;

	MMP_ULONG				i, ulCount = (MMP_ULONG) (&__CPU_COMM_MODULE_EXIT_END__ - &__CPU_COMM_MODULE_EXIT_START__) /
		sizeof(CPUCOMM_MODULE_FUN);
	CPUCOMM_MODULE_FUN		*pfTable = (void *) (&__CPU_COMM_MODULE_EXIT_START__);
#else
	extern CPUCOMM_MODULE_FUN Image$$CPU_COMM_MODULE_EXIT$$Base;
	extern MMP_ULONG Image$$CPU_COMM_MODULE_EXIT$$Length;

	MMP_ULONG			i, ulCount = (MMP_ULONG) (&Image$$CPU_COMM_MODULE_EXIT$$Length) / sizeof(CPUCOMM_MODULE_FUN);
	CPUCOMM_MODULE_FUN	*pfTable = (&Image$$CPU_COMM_MODULE_EXIT$$Base);
#endif

	// No entry in the table, nothing to init
	if(pfTable == NULL || ulCount == 0)
	{
		return;
	}

	// call moudle exit
	for(i = 0; i < ulCount; i++)
	{
		pfTable[i]();
	}

	CpuComm_IsrDisable(_CPU_ID_B);
}
