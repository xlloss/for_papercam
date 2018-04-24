//==============================================================================
//
//                              INCLUDE FILE
//
//============================================================================== 
 
#include "includes_fw.h"
#include "mmpf_monitor.h"

#if (MEM_MONITOR_EN)
#include "lib_retina.h"
#include "mmp_reg_mci.h"

/** @addtogroup MMPF_MONITOR
 *  @{
 */
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static MMP_BOOL     m_bMonitorInit = MMP_FALSE;
static MMP_BOOL     m_bSeqFree[MCI_MONITOR_SEG_NUM];
static MONITOR_SEQ_FAULT_CB m_pFaultCBFunc[MCI_MONITOR_SEG_NUM];
static MMPF_MONITOR_CB_ARGU m_sCallBackArgu[MCI_MONITOR_SEG_NUM];

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_Monitor_ISR
//  Description :
//------------------------------------------------------------------------------
void MMPF_Monitor_ISR(void)
{
    AITPS_MONITOR   pMNT = AITC_BASE_MONITOR;
    MMP_UBYTE       i, intsrc;

    intsrc = pMNT->MONITOR_INT_CPU_SR;
    pMNT->MONITOR_INT_CPU_SR = intsrc;

    for (i = 0; i < MCI_MONITOR_SEG_NUM; i++) {

        if ((intsrc & (MONITOR_SEG(i))) && m_pFaultCBFunc[i]) {
        
            m_sCallBackArgu[i].ubSegID      = i;
            m_sCallBackArgu[i].ulFaultSrc   = pMNT->MONITOR_SEG_FAULT_SRC[i];
            m_sCallBackArgu[i].ulFaultAddr  = pMNT->MONITOR_SEG_FAULT_ADDR[i];
            m_sCallBackArgu[i].ubFaultSize  = pMNT->MONITOR_SEG_FAULT_CNT[i];
            
            if (pMNT->MONITOR_SEG_WR_FAULT & (MONITOR_SEG(i)))
                m_sCallBackArgu[i].bFaultWr = MMP_TRUE;
            else
                m_sCallBackArgu[i].bFaultWr = MMP_FALSE;
            
            if (pMNT->MONITOR_SEG_RD_FAULT & (MONITOR_SEG(i)))
                m_sCallBackArgu[i].bFaultRd = MMP_TRUE;
            else
                m_sCallBackArgu[i].bFaultRd = MMP_FALSE;

            m_pFaultCBFunc[i](&m_sCallBackArgu[i]);
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Monitor_Init
//  Description : Initialize memory monitor utility
//------------------------------------------------------------------------------
/**
    @brief  Initialize memory monitor utility.
    @return None
*/
void MMPF_Monitor_Init(void)
{
    if (m_bMonitorInit == MMP_FALSE) {
        MMP_UBYTE   i;
        AITPS_AIC   pAIC = AITC_BASE_AIC;

        RTNA_AIC_Open(pAIC, AIC_SRC_MCI, mci_isr_a,
                            AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_MCI);

        for (i = 0; i < MCI_MONITOR_SEG_NUM; i++) {
            m_bSeqFree[i] = MMP_TRUE;
        }

        MEMSET(m_pFaultCBFunc, 0, sizeof(m_pFaultCBFunc));
        MEMSET(m_sCallBackArgu, 0, sizeof(m_sCallBackArgu));

        m_bMonitorInit = MMP_TRUE;
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Monitor_Enable
//  Description :
//------------------------------------------------------------------------------
/**
    @brief  Enable to monitor one memory segment.
    @param[in] pAttr : Specify the area of memory segment & read/write
                       permission of each source. Please refer to mmp_reg_mci.h
                       for the bitmap of source.
    @param[in] cb    : Callback fucntion while access fault occurs.
    @return Segment ID
*/
MMP_UBYTE MMPF_Monitor_Enable(MMPF_MONITOR_SEQ_ATTR *pAttr,
                              MONITOR_SEQ_FAULT_CB 	cb)
{
    MMP_UBYTE i;
    AITPS_MONITOR pMNT = AITC_BASE_MONITOR;

    OS_CRITICAL_INIT();

    if (!pAttr) {
        return INVALID_MONITOR_SEQ;
    }

    OS_ENTER_CRITICAL();

    for (i = 0; i < MCI_MONITOR_SEG_NUM; i++) {
        if (m_bSeqFree[i]) {
            m_bSeqFree[i] = MMP_FALSE;
            m_pFaultCBFunc[i] = cb;
            break;
        }
    }

    OS_EXIT_CRITICAL();

    if (i == MCI_MONITOR_SEG_NUM) {
        return NO_FREE_MONITOR_SEQ;
    }

    /* Disable monitor segment first */
    pMNT->MONITOR_EN &= ~(MONITOR_SEG(i));

    /* Set monitor segment address area */
    pMNT->MONITOR_SEG_LOW_BD[i] = pAttr->ulAddrLowBD;
    pMNT->MONITOR_SEG_UP_BD[i] = pAttr->ulAddrUpBD;

    /* Set read/write permission */
    pMNT->MONITOR_SEG_WR_ALLOW[i] = pAttr->ulWrAllowSrc;
    pMNT->MONITOR_SEG_RD_ALLOW[i] = pAttr->ulRdAllowSrc;

    /* Enable monitor segment */
    pMNT->MONITOR_INT_CPU_SR = MONITOR_INT_SEG_FAULT(i);
    pMNT->MONITOR_INT_CPU_EN |= MONITOR_SEG(i);
    pMNT->MONITOR_EN |= MONITOR_SEG(i);

    return i;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Monitor_Disable
//  Description :
//------------------------------------------------------------------------------
/**
    @brief  Disable monitor memory segment of the specified ID.
    @param[in] ubSegID : The segment ID.
    @return None
*/
void MMPF_Monitor_Disable(MMP_UBYTE ubSegID)
{
    AITPS_MONITOR pMNT = AITC_BASE_MONITOR;

    if (ubSegID < MCI_MONITOR_SEG_NUM) {
        pMNT->MONITOR_INT_CPU_EN &= ~(MONITOR_SEG(ubSegID));
        pMNT->MONITOR_EN &= ~(MONITOR_SEG(ubSegID));
        pMNT->MONITOR_INT_CPU_SR = MONITOR_INT_SEG_FAULT(ubSegID);
        m_bSeqFree[ubSegID] = MMP_TRUE;
    }
}

/// @}
/// @end_ait_only

#endif //(MEM_MONITOR_EN)
