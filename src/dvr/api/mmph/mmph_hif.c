//==============================================================================
//
//  File        : mmph_hif.c
//  Description : Ritian Host Interface Hardware Control Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================
///@ait_only

#define AIT_REG_FUNC_DECLARE

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "ait_utility.h"
#include "mmph_hif.h"
#include "mmpf_hif.h"
#include "mmpf_system.h"
#include "mmp_reg_gbl.h"

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

MMP_ULONG                   m_ulHifCmd[GRP_IDX_NUM] = {0};
__attribute ((aligned(32))) MMP_ULONG       m_ulHifParam[GRP_IDX_NUM][MAX_HIF_ARRAY_SIZE] = {{0, 0}};

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

extern MMPF_OS_FLAGID 	    DSC_UI_Flag;
extern MMPF_OS_FLAGID       SYS_Flag_Hif;
extern MMPF_OS_FLAGID       SENSOR_Flag;
extern MMPF_OS_FLAGID       DSC_Flag;
extern MMPF_OS_FLAGID       AUD_REC_Flag;
extern MMPF_OS_FLAGID       AUD_STREAM_Flag;
extern MMPF_OS_FLAGID       VID_REC_Flag;

extern MMPF_OS_SEMID        SYS_Sem_Hif[];

/** @addtogroup MMPH_HIF
@{
*/

/* #pragma O0 */
/* #pragma GCC push_options */
/* #pragma GCC optimize("O0") */

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_SetInterface
//  Description :
//------------------------------------------------------------------------------
MMP_ERR __attribute__((optimize("O0"))) MMPH_HIF_SetInterface(void)
{
    MMPH_HIF_RegGetB    = Ind_RegGetB;
    MMPH_HIF_RegGetW    = Ind_RegGetW;
    MMPH_HIF_RegGetL    = Ind_RegGetL;
    MMPH_HIF_RegSetB    = Ind_RegSetB;
    MMPH_HIF_RegSetW    = Ind_RegSetW;
    MMPH_HIF_RegSetL    = Ind_RegSetL;

    MMPH_HIF_MemGetB    = Ind_MemGetB;
    MMPH_HIF_MemGetW    = Ind_MemGetW;
    MMPH_HIF_MemGetL    = Ind_MemGetL;
    MMPH_HIF_MemSetB    = Ind_MemSetB;
    MMPH_HIF_MemSetW    = Ind_MemSetW;
    MMPH_HIF_MemSetL    = Ind_MemSetL;

    return MMP_ERR_NONE;
}

/* #pragma arm section code = "EnterSelfSleepMode", rwdata = "EnterSelfSleepMode",  zidata = "EnterSelfSleepMode" */
//------------------------------------------------------------------------------
//  Function    : Ind_RegGetB
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE __attribute__((optimize("O0"))) Ind_RegGetB(MMP_ULONG ulAddr)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	return *(AIT_REG_B*)(pReg+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_RegGetW
//  Description :
//------------------------------------------------------------------------------
MMP_USHORT __attribute__((optimize("O0"))) Ind_RegGetW(MMP_ULONG ulAddr)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	return *(MMP_USHORT*)(pReg+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_RegGetL
//  Description :
//------------------------------------------------------------------------------
MMP_ULONG __attribute__((optimize("O0"))) Ind_RegGetL(MMP_ULONG ulAddr)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	return *(AIT_REG_D*)(pReg+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_RegSetB
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_RegSetB(MMP_ULONG ulAddr, MMP_UBYTE ubData)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	*(AIT_REG_B*)(pReg+ulAddr) = ubData;
}

//------------------------------------------------------------------------------
//  Function    : Ind_RegSetW
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_RegSetW(MMP_ULONG ulAddr, MMP_USHORT usData)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	*(MMP_USHORT*)(pReg+ulAddr) = usData;
}

//------------------------------------------------------------------------------
//  Function    : Ind_RegSetL
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_RegSetL(MMP_ULONG ulAddr, MMP_ULONG ulData)
{
	AIT_REG_B* pReg = (AIT_REG_B*)AITC_BASE_OPR; 
	*(AIT_REG_D*)(pReg+ulAddr) = ulData;
}
/* #pragma arm section code, rwdata,  zidata */

//------------------------------------------------------------------------------
//  Function    : Ind_MemGetB
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE __attribute__((optimize("O0"))) Ind_MemGetB(MMP_ULONG ulAddr)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	return *(MMP_UBYTE*)(pMem+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_MemGetW
//  Description :
//------------------------------------------------------------------------------
MMP_USHORT __attribute__((optimize("O0"))) Ind_MemGetW(MMP_ULONG ulAddr)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	return *(MMP_USHORT*)(pMem+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_MemGetL
//  Description : 
//------------------------------------------------------------------------------
MMP_ULONG __attribute__((optimize("O0"))) Ind_MemGetL(MMP_ULONG ulAddr)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	return *(MMP_ULONG*)(pMem+ulAddr);
}

//------------------------------------------------------------------------------
//  Function    : Ind_MemSetB
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_MemSetB(MMP_ULONG ulAddr, MMP_UBYTE ubData)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	*(MMP_UBYTE*)(pMem+ulAddr) = ubData;
}

//------------------------------------------------------------------------------
//  Function    : Ind_MemSetW
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_MemSetW(MMP_ULONG ulAddr, MMP_USHORT usData)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	*(MMP_USHORT*)(pMem+ulAddr) = usData;
}

//------------------------------------------------------------------------------
//  Function    : Ind_MemSetL
//  Description : 
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) Ind_MemSetL(MMP_ULONG ulAddr, MMP_ULONG ulData)
{
	MMP_UBYTE *pMem = (MMP_UBYTE *)0;
	*(MMP_ULONG*)(pMem+ulAddr) = ulData;
}

#if 0
void ___HIF_PRARAMTERS___(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_MemCopyDevToHost
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_MemCopyDevToHost(MMP_UBYTE *ubDestPtr, MMP_ULONG ulSrcAddr, MMP_ULONG ulLength)
{
	MEMCPY((MMP_UBYTE *)ubDestPtr, (MMP_UBYTE *)ulSrcAddr, ulLength);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_MemCopyHostToDev
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_MemCopyHostToDev(MMP_ULONG ulDestAddr, MMP_UBYTE *usSrcAddr, MMP_ULONG ulLength)
{
	MEMCPY((MMP_UBYTE *)ulDestAddr, usSrcAddr, ulLength);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_CmdGetStatusL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
MMP_ULONG __attribute__((optimize("O0"))) MMPH_HIF_CmdGetStatusL(void)
{
    return MMPF_HIF_GetCmdStatus(0xFFFFFFFF);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_CmdSetStatusL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_CmdSetStatusL(MMP_ULONG status)
{
    MMPF_HIF_SetCmdStatus(status);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_CmdClearStatusL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_CmdClearStatusL(MMP_ULONG status)
{
    MMPF_HIF_ClearCmdStatus(status);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_SysHifISR
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPF_HIF_SysHifISR(MMP_USHORT command)
{
    m_ulHifCmd[GRP_IDX_SYS] = command;

    MMPF_OS_SetFlags(SYS_Flag_Hif, SYS_FLAG_SYS, MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_FlowCtlHifISR
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPF_HIF_FlowCtlHifISR(MMP_USHORT command)
{
    m_ulHifCmd[GRP_IDX_FLOWCTL] = command;

    MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_SENSOR_CMD, MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_DSCHifISR
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPF_HIF_DSCHifISR(MMP_USHORT command)
{
    m_ulHifCmd[GRP_IDX_DSC] = command;

    MMPF_OS_SetFlags(DSC_Flag, DSC_FLAG_DSC_CMD, MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_HIF_VideoHifISR
//  Description :
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPF_HIF_VideoHifISR(MMP_USHORT command)
{
    m_ulHifCmd[GRP_IDX_VID] = command;

    if ((command & FUNC_MASK) < 0x11) {

        MMP_USHORT usFunc = command & (GRP_MASK | FUNC_MASK);

        if (usFunc == HIF_VID_CMD_MERGER_PARAMETER ||
             usFunc == HIF_VID_CMD_MERGER_OPERATION ||
             usFunc == HIF_VID_CMD_MERGER_TAILSPEEDMODE){
            MMPF_OS_SetFlags(VID_REC_Flag, CMD_FLAG_VSTREAM, MMPF_OS_FLAG_SET);
        }
        else if (usFunc == HIF_VID_CMD_RECD_PARAMETER ||
                 usFunc == HIF_VID_CMD_RECD_OPERATION){
            MMPF_OS_SetFlags(VID_REC_Flag, CMD_FLAG_VIDRECD, MMPF_OS_FLAG_SET);
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_WaitSem
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE __attribute__((optimize("O0"))) MMPH_HIF_WaitSem(MMP_UBYTE ubGroup, MMP_USHORT usTimeOut)
{
    return MMPF_OS_AcquireSem(SYS_Sem_Hif[ubGroup], usTimeOut);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_ReleaseSem
//  Description :
//------------------------------------------------------------------------------
MMP_UBYTE __attribute__((optimize("O0"))) MMPH_HIF_ReleaseSem(MMP_UBYTE ubGroup)
{
    return MMPF_OS_ReleaseSem(SYS_Sem_Hif[ubGroup]);
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_SendCmd
//  Description :
//------------------------------------------------------------------------------
MMP_ERR __attribute__((optimize("O0"))) MMPH_HIF_SendCmd(MMP_UBYTE ubGroup, MMP_USHORT usCommand)
{
	MMPF_OS_FLAGS   flags;
	MMP_UBYTE       ubRet = 0;

	#define HIF_CMD_TIMEOUT (8000)
	
	switch(ubGroup){
    case GRP_IDX_SYS:
		MMPF_HIF_SysHifISR(usCommand);
        ubRet = MMPF_OS_WaitFlags(DSC_UI_Flag, SYS_FLAG_SYS_CMD_DONE,
			                MMPF_OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, HIF_CMD_TIMEOUT, &flags);
        break;
    case GRP_IDX_FLOWCTL:
		MMPF_HIF_FlowCtlHifISR(usCommand);
        ubRet = MMPF_OS_WaitFlags(DSC_UI_Flag, SYS_FLAG_SENSOR_CMD_DONE,
			                MMPF_OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, HIF_CMD_TIMEOUT, &flags);
        break;
    case GRP_IDX_DSC:
    	MMPF_HIF_DSCHifISR(usCommand);
        ubRet = MMPF_OS_WaitFlags(DSC_UI_Flag, SYS_FLAG_DSC_CMD_DONE,
			                MMPF_OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, HIF_CMD_TIMEOUT, &flags);
        break;	        	
    case GRP_IDX_VID:
		MMPF_HIF_VideoHifISR(usCommand);
        ubRet = MMPF_OS_WaitFlags(DSC_UI_Flag, SYS_FLAG_VID_CMD_DONE,
			                MMPF_OS_FLAG_WAIT_SET_ANY | OS_FLAG_CONSUME, HIF_CMD_TIMEOUT, &flags);
        break;
	}
	
	if (ubRet)
	    return MMP_HIF_ERR_CMDTIMEOUT;
	else	
        return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_GetParameterL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
MMP_ULONG __attribute__((optimize("O0"))) MMPH_HIF_GetParameterL(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum)
{
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
    return ParaBufPtr[(ubParamnum >> 2)];
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_SetParameterL
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_SetParameterL(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_ULONG ulParamdata)
{
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);

    ParaBufPtr[(ubParamnum >> 2)] = ulParamdata;
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_GetParameterW
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
MMP_USHORT __attribute__((optimize("O0"))) MMPH_HIF_GetParameterW(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum)
{
	MMP_UBYTE index = (ubParamnum /4);
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);

    return (MMP_SHORT)(ParaBufPtr[index] >> ((ubParamnum%4)*8));
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_SetParameterW
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_SetParameterW(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_USHORT usParamdata)
{
 	MMP_UBYTE index = (ubParamnum /4);
 	MMP_UBYTE shifter = (ubParamnum%4)*8;
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
   	ParaBufPtr[index] &= ~((MMP_ULONG)0xFFFF << shifter);     	//Clean the related position
	ParaBufPtr[index] |= ((MMP_ULONG)usParamdata << shifter); 	//Set the value to the related position  
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_GetParameterB
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
MMP_UBYTE __attribute__((optimize("O0"))) MMPH_HIF_GetParameterB(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum)
{
	MMP_UBYTE index = (ubParamnum /4);
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
    return (MMP_UBYTE)(ParaBufPtr[index] >> ((ubParamnum%4)*8));
}

//------------------------------------------------------------------------------
//  Function    : MMPH_HIF_SetParameterB
//  Description : The parameter only has 24 bytes (12 words or 6 ulong int)
//------------------------------------------------------------------------------
void __attribute__((optimize("O0"))) MMPH_HIF_SetParameterB(MMP_UBYTE ubGroup, MMP_UBYTE ubParamnum, MMP_UBYTE ubParamdata)
{
	MMP_UBYTE index = (ubParamnum /4);
	MMP_UBYTE shifter = (ubParamnum%4)*8;
	MMP_ULONG* ParaBufPtr = &(m_ulHifParam[ubGroup][0]);
	
	ParaBufPtr[index] &= ~((MMP_ULONG)0xFF << shifter);     	//Clean the related position
	ParaBufPtr[index] |= ((MMP_ULONG)ubParamdata << shifter); 	//Set the value to the related position  
}

/// @}

/* #pragma */
/* #pragma GCC pop_options */

///@end_ait_only
