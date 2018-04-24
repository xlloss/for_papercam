//==============================================================================
//
//  File        : mmpf_pll.c
//  Description : Firmware PLL Control Function
//  Author      : Rogers Chen
//  Revision    : 1.0
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_gbl.h"
#include "mmpf_pll.h"
#include "mmpf_dram.h"
#if !defined(MINIBOOT_FW)
#include "mmpf_audio_ctl.h"
#endif
#include "clk_cfg.h"
#include "mmp_aud_inc.h"

/** @addtogroup MMPF_PLL
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
/*
 * Audio frequency clock group:
 * Group0: 48KHz, 32KHz, 24KHz, 16KHz, 8KHz
 * Group1: 44.1KHz, 22.05KHz, 11.025KHz
 */
#define AUDIO_GRP0_FREQ         (24576)
#define AUDIO_GRP1_FREQ         (11289)
#define AUDIO_GRP0_FRACT        (0x01BA5E)
#define AUDIO_GRP1_FRACT        (0x034395)

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

MMP_ULONG   gCpuFreqKHz;                ///< CPU clock freq. in unit of KHz
MMP_ULONG   gGrpFreqKHz[CLK_GRP_NUM];   ///< clock freq. of every group (KHz)

//Notes: MCV_V2 EXT_CLK is 24Mhz
const static  MMPF_PLL_SETTINGS m_PllSettings[GRP_CLK_SRC_MAX] = 
{
//#if CONFIG_HW_FOR_DRAM_TIMIMG_TEST==0
#if (PLL_CONFIG == PLL_FOR_BURNING)
    // M,               N,              DIV,                Frac,   VCO,              Freq}
    //DPLL0 600Hz
    {DPLL0_M_DIV_3,     DPLL0_N(50),    DPLL0_P_DIV_2,      0,      0x3F,           400000},
    //DPLL1 360MHz
    {DPLL0_M_DIV_2,     DPLL1_N(30),    DPLL0_P_DIV_2,      0,      0x27,           360000},
    //DPLL2 528MHz
    {DPLL0_M_DIV_2,     DPLL2_N(44),    DPLL0_P_DIV_2,      0,      0x3F,           528000},
    //PMCLK
    {0,                 0,              0,                  0,      0,             EXT_CLK},
    //DPLL3 147.456MHz for Audio
    {DPLL3_M(2),        DPLL3_N(17),    0,   AUDIO_GRP0_FRACT,      0,     AUDIO_GRP0_FREQ},
    //DPLL4 378MHz for TV
    {DPLL4_M(23),       DPLL4_N(188),   DPLL4_P_DIV_2,      0,      DPLL4_400MHZ,   378000},
    //DPLL5 400MHz, for DRAM
    {DPLL5_OUTPUT_DIV_2,DPLL5_N(20),    DPLL5_P_DIV_1,      0,      DPLL5_800MHZ,   480000}
#elif (PLL_CONFIG == PLL_FOR_ULTRA_LOWPOWER_192)
    // M,               N,              DIV,                Frac,   VCO,              Freq}
    //DPLL0 600Hz
    {DPLL0_M_DIV_3,     DPLL0_N(50),    DPLL0_P_DIV_2,      0,      0x3F,           400000},
    //DPLL1 360MHz, power off
    {DPLL0_M_DIV_2,     DPLL1_N(30),    DPLL0_P_DIV_2,      0,      0x27,           EXT_CLK},
    //DPLL2 384MHz
    {DPLL0_M_DIV_2,     DPLL2_N(32),    DPLL0_P_DIV_2,      0,      0x27,          384000},
    //PMCLK
    {0,                 0,              0,                  0,      0,             EXT_CLK},
    //DPLL3 147.456MHz for Audio
    {DPLL3_M(2),        DPLL3_N(17),    0,   AUDIO_GRP0_FRACT,      0,     AUDIO_GRP0_FREQ},
    //DPLL4 378MHz for TV
    {DPLL4_M(23),       DPLL4_N(188),   DPLL4_P_DIV_2,      0,      DPLL4_400MHZ,   378000},
    //DPLL5 400MHz, for DRAM, power off
    {DPLL5_OUTPUT_DIV_2,DPLL5_N(20),    DPLL5_P_DIV_1,      0,      DPLL5_800MHZ,   EXT_CLK}
#elif (PLL_CONFIG == PLL_FOR_ULTRA_LOWPOWER_168)
    // M,               N,              DIV,                Frac,   VCO,              Freq}
    //DPLL0 600Hz
    {DPLL0_M_DIV_3,     DPLL0_N(50),    DPLL0_P_DIV_2,      0,      0x3F,           400000},
    //DPLL1 360MHz, power off
    {DPLL0_M_DIV_2,     DPLL1_N(30),    DPLL0_P_DIV_2,      0,      0x27,           EXT_CLK},
    //DPLL2 576MHz
    {DPLL0_M_DIV_2,     DPLL2_N(42),    DPLL0_P_DIV_2,      0,      0x3F,           504000},
    //PMCLK
    {0,                 0,              0,                  0,      0,             EXT_CLK},
    //DPLL3 147.456MHz for Audio
    {DPLL3_M(2),        DPLL3_N(17),    0,   AUDIO_GRP0_FRACT,      0,     AUDIO_GRP0_FREQ},
    //DPLL4 378MHz for TV
    {DPLL4_M(23),       DPLL4_N(188),   DPLL4_P_DIV_2,      0,      DPLL4_400MHZ,   378000},
    //DPLL5 400MHz, for DRAM, power off
    {DPLL5_OUTPUT_DIV_2,DPLL5_N(20),    DPLL5_P_DIV_1,      0,      DPLL5_800MHZ,   EXT_CLK}
#else
    // M,               N,              DIV,                Frac,   VCO,              Freq}
    //DPLL0 600Hz
    {DPLL0_M_DIV_3,     DPLL0_N(50),    DPLL0_P_DIV_2,      0,      0x3F,           400000},
    //DPLL1 360MHz, power off
    //{DPLL0_M_DIV_2,     DPLL1_N(30),    DPLL0_P_DIV_2,      0,      0x27,           360000},
    {DPLL0_M_DIV_2,     DPLL1_N(30),    DPLL0_P_DIV_2,      0,      0x27,           EXT_CLK},
    //DPLL2 528MHz
    {DPLL0_M_DIV_2,     DPLL2_N(44),    DPLL0_P_DIV_2,      0,      0x3F,           528000},
    //PMCLK
    {0,                 0,              0,                  0,      0,             EXT_CLK},
    //DPLL3 147.456MHz for Audio
    {DPLL3_M(2),        DPLL3_N(17),    0,   AUDIO_GRP0_FRACT,      0,     AUDIO_GRP0_FREQ},
    //DPLL4 378MHz for TV
    {DPLL4_M(23),       DPLL4_N(188),   DPLL4_P_DIV_2,      0,      DPLL4_400MHZ,   378000},
    //DPLL5 400MHz, for DRAM, power off
    //{DPLL5_OUTPUT_DIV_2,DPLL5_N(20),    DPLL5_P_DIV_1,      0,      DPLL5_800MHZ,   480000}
    {DPLL5_OUTPUT_DIV_2,DPLL5_N(20),    DPLL5_P_DIV_1,      0,      DPLL5_800MHZ,   EXT_CLK}
#endif    
};

static MMP_UBYTE        m_bAudSampleGroup = 0;
static MMPF_PLL_MODE    m_bADCSamplerate  = 0;
static MMPF_PLL_MODE    m_bDACSamplerate  = 0;


MMP_ERR MMPF_PLL_ConfigGroupClk(void) ITCMFUNC;
#if defined(MBOOT_FW)
MMP_ERR MMPF_PLL_PowerUp(GRP_CLK_SRC PLLNo, MMP_BOOL bPowerUp) ITCMFUNC;
MMP_ERR MMPF_PLL_Initialize(void) ITCMFUNC;
#endif
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

__attribute__((optimize("O0")))
void MMPF_PLL_WaitCount(MMP_ULONG ulCount)
{
    while (ulCount--);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_PowerUp
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_PowerUp(GRP_CLK_SRC PLLNo, MMP_BOOL bPowerUp)
{
    GBL_DECL;
    MMP_USHORT usWaitCycle = 50;
    MMP_UBYTE  offset = 0;

    switch(PLLNo) {
    case DPLL0:
        offset = 0;
        break;
    case DPLL1:
        offset = 0x10;
        break;
    case DPLL2:
        offset = 0x20;
        break;
    case DPLL3:
        offset = 0x30;
        break;
    case DPLL4:
        offset = 0x40;
        break;
    case DPLL5:
        offset = 0x50;
        break;
    default:
        return MMP_PLL_ERR_PARAMETER;
    }

    if (bPowerUp) {
        GBL_WR_OFST_B(GBL_DPLL0_PWR, offset,
                      GBL_RD_OFST_B(GBL_DPLL0_PWR, offset) &~ DPLL_PWR_DOWN);
        MMPF_PLL_WaitCount(usWaitCycle);
    }
    else {
        GBL_WR_OFST_B(GBL_DPLL0_PWR, offset,
                      GBL_RD_OFST_B(GBL_DPLL0_PWR, offset) | DPLL_PWR_DOWN);
        MMPF_PLL_WaitCount(usWaitCycle);
    }
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_GetGroupFreq
//  Description :
//------------------------------------------------------------------------------
/** @brief The function get the Group frequence.
@param[in] ubGroupNum select to get group number (ex: 0 -> group 0)
@param[out] ulGroupFreq 1000*(GX frequency) (ex: 133MHz -> 133000)
@return It reports the status of the operation.
*/
MMP_ERR MMPF_PLL_GetGroupFreq(CLK_GRP ubGroupNum, MMP_ULONG *ulGroupFreq)
{
    *ulGroupFreq = 0;

    if (ubGroupNum < CLK_GRP_NUM)
        *ulGroupFreq = gGrpFreqKHz[ubGroupNum];
    else
        return MMP_SYSTEM_ERR_PARAMETER;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_GetGroupFreq
//  Description :
//------------------------------------------------------------------------------
/** @brief The function get the Group PLL source and division.
@param[in] ubGroupNum select to get group number (ex: 0 -> group 0)
@param[out] pllNo PLL number
@return It reports the status of the operation.
*/
#if !defined(MINIBOOT_FW)
MMP_ERR MMPF_PLL_GetGroupSrcAndDiv(CLK_GRP ubGroupNum, GRP_CLK_SRC *clkSrc, MMP_UBYTE* pubPllDiv)
{
    *clkSrc = GRP_CLK_SRC_MAX;
    *pubPllDiv  = 1;

    if (ubGroupNum < CLK_GRP_NUM) {
        *clkSrc = gGrpClkCfg[ubGroupNum].src;
        *pubPllDiv  = gGrpClkCfg[ubGroupNum].div;
    }
    else {
        return MMP_SYSTEM_ERR_PARAMETER;
    }

    return MMP_ERR_NONE;
}
#endif

#if !(defined(ALL_FW)||(defined(MBOOT_FW)&&!(defined(SD_BOOT)||defined(FAT_BOOT))))
//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_ConfigGroupClk
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_ConfigGroupClk(void)
{
    AITPS_GBL  pGBL  = AITC_BASE_GBL;
    AITPS_CORE pCORE = AITC_BASE_CORE;
    MMP_ULONG  i;

    // Setup each group's clock source and diviver

    /* CLK_GRP_GBL: global clock domain (G0) */
    if (gGrpClkCfg[CLK_GRP_GBL].src < GRP_CLK_SRC_MAX) {
        if (pGBL->GBL_CLK_CTL & GRP_CLK_SEL_SRC1) {
            pGBL->GBL_CLK_SRC = gGrpClkCfg[CLK_GRP_GBL].src << 4;
        }
        else {
            pGBL->GBL_CLK_SRC = gGrpClkCfg[CLK_GRP_GBL].src;
        }
        pGBL->GBL_CLK_DIV = GRP_CLK_DIV_EN |
                            GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_GBL].div);
        pGBL->GBL_CLK_CTL &= ~(GRP_CLK_DIV2_EN);
    }
    else {
        while(1);
    }

    /* CLK_GRP_DRAM: DRAM clock domain */
    #if (DRAM_ID == DRAM_DDR)
    if (gGrpFreqKHz[CLK_GRP_DRAM] != gGrpFreqKHz[CLK_GRP_GBL]) {
        // DRAM clock async mode
        if (gGrpClkCfg[CLK_GRP_DRAM].src < GRP_CLK_SRC_MAX) {
            pGBL->GBL_DRAM_CLK_SRC = gGrpClkCfg[CLK_GRP_DRAM].src;
        }
        else {
           while(1);
        }
        // post divide enable
        pGBL->GBL_DRAM_CLK_DIV = GRP_CLK_DIV_EN |
                                 GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_DRAM].div);
        // disable dram_2x
        pGBL->GBL_DRAM_CLK_SRC &= ~(DRMA_CLK_DIV2_EN);
        pGBL->GBL_DRAM_CLK_SRC |= DRAM_CLK_ASYNC;
    }
    else {
        // DRAM clock sync mode
        pGBL->GBL_DRAM_CLK_SRC &= ~(DRAM_CLK_ASYNC);
    }
    #endif
    #if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2)
    if (gGrpFreqKHz[CLK_GRP_DRAM] != (gGrpFreqKHz[CLK_GRP_GBL]*2)) {
        // DRAM clock async mode
        if (gGrpClkCfg[CLK_GRP_DRAM].src < GRP_CLK_SRC_MAX) {
            pGBL->GBL_DRAM_CLK_SRC = gGrpClkCfg[CLK_GRP_DRAM].src;
        }
        else {
           while(1);
        }
        // post divide enable
        pGBL->GBL_DRAM_CLK_DIV = GRP_CLK_DIV_EN |
                                 GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_DRAM].div);
        // disable dram_2x
        pGBL->GBL_DRAM_CLK_SRC |= DRMA_CLK_DIV2_EN;
        pGBL->GBL_DRAM_CLK_SRC |= DRAM_CLK_ASYNC;
    }
    else {
        // DRAM clock sync mode
        pGBL->GBL_CLK_DIV = GRP_CLK_DIV_EN |
                                GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_DRAM].div);
        pGBL->GBL_CLK_CTL      |=  GRP_CLK_DIV2_EN;
        pGBL->GBL_DRAM_CLK_SRC &= ~(DRAM_CLK_ASYNC);
    }
    #endif

    /* CLK_GRP_USB: USB PHY clock domain */
    if (gGrpClkCfg[CLK_GRP_USB].src < GRP_CLK_SRC_MAX) {
        /* For MercuryV2 MP
         * We fed 24MHz clock as the USB PHY input clock freq always,
         * because the PLL inside USB PHY is designed for 24MHz.
         */
        // bit USBPHY_CLK_SRC_PMCLK dominate actually
        pGBL->GBL_USB_CLK_SRC  = PMCLK | GBL_USB_ROOT_CLK_SEL_MCLK |
                                 USBPHY_CLK_SRC_PMCLK;
        pGBL->GBL_USB_CLK_DIV  = GRP_CLK_DIV_EN | GRP_CLK_DIV(1);
    }
    else {
        while(1);
    }

    /* CLK_GRP_RXBIST: Rx BIST clock domain */
    if (gGrpClkCfg[CLK_GRP_RXBIST].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_MIPI_RX_BIST_CLK_SRC = gGrpClkCfg[CLK_GRP_RXBIST].src;
    }
    else {
        while(1);
    }

    /* CLK_GRP_SNR: sensor clock domain */
    if (gGrpClkCfg[CLK_GRP_SNR].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_VIF_CLK_SRC = gGrpClkCfg[CLK_GRP_SNR].src;
        pGBL->GBL_VIF_CLK_DIV = GRP_CLK_DIV_EN |
                                GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_SNR].div);
    }
    else {
        while(1);
    }

    /* CLK_GRP_AUD: audio clock domain */
    if (gGrpClkCfg[CLK_GRP_AUD].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_AUD_CLK_SRC = gGrpClkCfg[CLK_GRP_AUD].src;
        pGBL->GBL_AUD_CLK_DIV[0x00] = GRP_CLK_DIV_EN |
                                      GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_AUD].div);
        // Set default ADC & DAC sample rate to 32KHz
        pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(6); // x5da2
        pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(3); // x5da3
        pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(6); // x5da4
        pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(3); // x5da5

        m_bADCSamplerate = MMPF_PLL_AUDIO_32K;
        m_bDACSamplerate = MMPF_PLL_AUDIO_32K;
        m_bAudSampleGroup = 0;
    }
    else {
        while(1);
    }

    /* CLK_GRP_COLOR: ISP clock domain */
    if (gGrpClkCfg[CLK_GRP_COLOR].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_ISP_CLK_SRC = gGrpClkCfg[CLK_GRP_COLOR].src;
        pGBL->GBL_ISP_CLK_DIV = GRP_CLK_DIV_EN |
                                GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_COLOR].div);
    }
    else {
        while(1);
    }

    /* CLK_GRP_BAYER: bayer, raw fetch clock domain */
    if (gGrpClkCfg[CLK_GRP_BAYER].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_BAYER_CLK_SRC = gGrpClkCfg[CLK_GRP_BAYER].src;
        pGBL->GBL_BAYER_CLK_DIV = GRP_CLK_DIV_EN |
                                  GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_BAYER].div);
    }
    else {
        while(1);
    }

    /* CLK_GRP_TV: TV clock domain */
    if (gGrpClkCfg[CLK_GRP_TV].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_TV_CLK_SRC = gGrpClkCfg[CLK_GRP_TV].src;
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN |
                                 GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_TV].div);
    }
    else {
        while(1);
    }

    /* CLK_GRP_HDMI: HDMI clock domain */
    if (gGrpClkCfg[CLK_GRP_HDMI].src < GRP_CLK_SRC_MAX) {
        pGBL->GBL_HDMI_CLK_SRC = gGrpClkCfg[CLK_GRP_HDMI].src;
        pGBL->GBL_HDMI_CLK_DIV = GRP_CLK_DIV_EN |
                                 GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_HDMI].div);
    }
    else {
        while(1);
    }

    /* CLK_GRP_CPUA: CPU_A clock domain */
    if (gGrpFreqKHz[CLK_GRP_CPUA] != gGrpFreqKHz[CLK_GRP_GBL]) {
        if (gGrpClkCfg[CLK_GRP_CPUA].src < GRP_CLK_SRC_MAX) {
            if (pGBL->GBL_CPU_A_CLK_CTL & GRP_CLK_SEL_SRC1) {
                pGBL->GBL_CPU_A_CLK_SRC = gGrpClkCfg[CLK_GRP_CPUA].src << 4;
            }
            else {
                pGBL->GBL_CPU_A_CLK_SRC = gGrpClkCfg[CLK_GRP_CPUA].src;
            }
            //Note: We can not turn off the PLL_POST_DIV_EN bit in CPU clk path
            for(i = 7; i > 0; i--) {
                if ((gGrpClkCfg[CLK_GRP_CPUA].div % i) == 0) {
                    /* mod 1 most be zero, don't worry div by zero. */
                    pGBL->GBL_CPU_A_SMOOTH_DIV &= ~(PLL_SMOOTH_DIV_MASK);
                    pGBL->GBL_CPU_A_SMOOTH_DIV |= i;
                    break;
                }
            }
            pGBL->GBL_CPU_A_CLK_DIV = GRP_CLK_DIV_EN |
                                      GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_CPUA].div/i);
        }
        else {
            while(1);
        }
        // async mode
        pCORE->CORE_A_CFG |= CPU_CLK_ASYNC;
    }
    else {
        // sync mode
        pCORE->CORE_A_CFG &= ~CPU_CLK_ASYNC;
    }

    /* CLK_GRP_CPUB: CPU_B clock domain */
    if (gGrpFreqKHz[CLK_GRP_CPUB] != gGrpFreqKHz[CLK_GRP_GBL]) {
        if (gGrpClkCfg[CLK_GRP_CPUB].src < GRP_CLK_SRC_MAX) {
            if (pGBL->GBL_CPU_B_CLK_CTL & GRP_CLK_SEL_SRC1) {
                pGBL->GBL_CPU_B_CLK_SRC = gGrpClkCfg[CLK_GRP_CPUB].src << 4;
            }
            else {
                pGBL->GBL_CPU_B_CLK_SRC = gGrpClkCfg[CLK_GRP_CPUB].src;
            }
            //Note: We can not turn off the PLL_POST_DIV_EN bit in CPU clk path
            for(i = 7; i > 0; i--) {
                if ((gGrpClkCfg[CLK_GRP_CPUB].div % i) == 0) {
                    //mod 1 most be zero, don't worry div by zero.
                    pGBL->GBL_CPU_B_SMOOTH_DIV &= ~(PLL_SMOOTH_DIV_MASK);
                    pGBL->GBL_CPU_B_SMOOTH_DIV |= i;
                    break;
                }
            }

            pGBL->GBL_CPU_B_CLK_DIV = GRP_CLK_DIV_EN |
                                      GRP_CLK_DIV(gGrpClkCfg[CLK_GRP_CPUB].div/i);
        }
        else {
            while(1);
        }
        // async mode
        pCORE->CORE_B_CFG |= CPU_CLK_ASYNC;
    }
    else {
        // sync mode
        pCORE->CORE_B_CFG &= ~CPU_CLK_ASYNC;
    }

    MMPF_PLL_WaitCount(50);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_Setting
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_PLL_Setting(void)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;
    const MMPF_PLL_SETTINGS *pllSettings;

    pllSettings = m_PllSettings;

    //Step 1: Bypass PLL0, PLL1, PLL2, PLL3, PLL4 & PLL5
    pGBL->GBL_DPLL0_CFG |= DPLL_BYPASS;
    pGBL->GBL_DPLL1_CFG |= DPLL_BYPASS;
    pGBL->GBL_DPLL2_CFG |= DPLL_BYPASS;
    pGBL->GBL_DPLL3_CFG |= DPLL_BYPASS;
    pGBL->GBL_DPLL4_CFG |= DPLL_BYPASS;
    pGBL->GBL_DPLL5_CFG |= DPLL_BYPASS;

    MMPF_PLL_WaitCount(100);

    //Step 2: Update PLL to the target frequency
    if (pllSettings[DPLL0].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 / M * N / P). It doesn't include post divider

        // power-on & BIST off
        pGBL->GBL_DPLL0_PWR = 0x00;// 0x5d02
        // pre-divider
        pGBL->GBL_DPLL0_M = pllSettings[DPLL0].M; // 0x5d03
        // loop-divider
        pGBL->GBL_DPLL0_N = pllSettings[DPLL0].N; // 0x5d04
        // post-divider
        pGBL->GBL_DPPL0_PARAM[0x00] = pllSettings[DPLL0].K; // 0x5d05
        // PLL setting VCO 400~600MHz
        pGBL->GBL_DPPL0_PARAM[0x01] = pllSettings[DPLL0].VCO; // 0x5d06
        // PLL control setting CTAT
        pGBL->GBL_DPPL0_PARAM[0x02] = 0xC0; // 0x5d07

        //Update DPLL0 configuration
        pGBL->GBL_DPLL0_CFG |= DPLL_UPDATE_PARAM; // 0x5d00

        MMPF_PLL_PowerUp(DPLL0, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL0, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL0, 0);
    }

    if (pllSettings[DPLL1].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 / M * N / P). It doesn't include post divider

        // power-on & BIST off
        pGBL->GBL_DPLL1_PWR = 0x00;// 0x5d12
        // pre-divider
        pGBL->GBL_DPLL1_M = pllSettings[DPLL1].M; // 0x5d13
        // loop-divider
        pGBL->GBL_DPLL1_N = pllSettings[DPLL1].N; // 0x5d14
        // post-divider
        pGBL->GBL_DPPL1_PARAM[0x00] = pllSettings[DPLL1].K; // 0x5d15
        // PLL setting VCO 400~600MHz
        pGBL->GBL_DPPL1_PARAM[0x01] = pllSettings[DPLL1].VCO; // 0x5d16
        // PLL control setting CTAT
        pGBL->GBL_DPPL1_PARAM[0x02] = 0xC0; // 0x5d17

        //Update DPLL1 configuration
        pGBL->GBL_DPLL1_CFG |= DPLL_UPDATE_PARAM; // 0x5d10
        MMPF_PLL_PowerUp(DPLL1, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL1, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL1, 0);
    }

    if (pllSettings[DPLL2].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 / M * N / P). It doesn't include post divider

        // power-on & BIST off
        pGBL->GBL_DPLL2_PWR = 0x00;// 0x5d22
        // pre-divider
        pGBL->GBL_DPLL2_M = pllSettings[DPLL2].M; // 0x5d23
        // loop-divider
        pGBL->GBL_DPLL2_N = pllSettings[DPLL2].N; // 0x5d24
        // post-divider
        pGBL->GBL_DPPL2_PARAM[0x00] = pllSettings[DPLL2].K; // 0x5d25
        // PLL setting VCO 600~800MHz
        pGBL->GBL_DPPL2_PARAM[0x01] = pllSettings[DPLL2].VCO; // 0x5d26
        // PLL control setting CTAT
        pGBL->GBL_DPPL2_PARAM[0x02] = 0xC0; // 0x5d27

        //Update DPLL2 configuration
        pGBL->GBL_DPLL2_CFG |= DPLL_UPDATE_PARAM; // 0x5d20
        MMPF_PLL_PowerUp(DPLL2, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL2, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL2, 0);
    }

    if (pllSettings[DPLL3].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 * (1/(M+1)*(N+1).Frac)). It doesn't include post divider

        // Post-Divider
	    pGBL->GBL_DPLL3_M = pllSettings[DPLL3].M; // 0x5d33
	    // Loop-Divider
	    pGBL->GBL_DPLL3_N = pllSettings[DPLL3].N; // 0x5d34
	    // Fractional Part
	    pGBL->GBL_DPPL3_PARAM[0x00] = (MMP_UBYTE)(pllSettings[DPLL3].frac & 0xFF); // 0x5d35
	    pGBL->GBL_DPPL3_PARAM[0x01] = (MMP_UBYTE)((pllSettings[DPLL3].frac & 0xFF00) >> 8); // 0x5d36
	    pGBL->GBL_DPPL3_PARAM[0x02] = (MMP_UBYTE)((pllSettings[DPLL3].frac & 0xFF0000) >> 16); // 0x5d37

        // PLL Control Setting
        pGBL->GBL_DPPL3_PARAM[0x03] = 0x00; // 0x5d38
        // Enable BOOST CP & MAIN CP DIODE MODE
        pGBL->GBL_DPPL3_PARAM[0x04] = DPLL3_BOOST_CP_EN | DPLL3_MAIN_CP_VP_DIODE; // 0x5d39
        MMPF_PLL_WaitCount(50);
        // Enable BOOST CP & MAIN CP OP MODE
        pGBL->GBL_DPPL3_PARAM[0x04] &= ~DPLL3_MAIN_CP_VP_DIODE; // 0x5d39

        MMPF_PLL_WaitCount(50);
        //Update DPLL3 configuration
        pGBL->GBL_DPLL3_CFG |= DPLL_UPDATE_PARAM; // 0x5d30
        //MMPF_PLL_PowerUp(DPLL3, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL3, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL3, 0);
    }

    if (pllSettings[DPLL4].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 * (1/(M+1)*((N+1)*4)/P). It doesn't include post divider
        // Pre-Divider
        pGBL->GBL_DPLL4_M = pllSettings[DPLL4].M; // 0x5d43
        // Loop-Divider
        pGBL->GBL_DPLL4_N = pllSettings[DPLL4].N; // 0x5d44
        // SS frequency divider
        pGBL->GBL_DPPL4_PARAM[0x03] = 0x82; // 0x5d48
        // VCO frequency, CP current
        pGBL->GBL_DPPL4_PARAM[0x04] = DPLL4_800MHZ | DPLL4_CP_5uA; // 0x5d49
        pGBL->GBL_DPPL4_PARAM[0x00] = 0x00; // 0x5d45
        // SS-EN, Frange Setting
        pGBL->GBL_DPPL4_PARAM[0x01] = pllSettings[DPLL4].K; // 0x5d46
        // SS source select
        pGBL->GBL_DPPL4_PARAM[0x02] = DPLL4_SS_SRCSEL_0d62uA; // 0x5d47
        MMPF_PLL_WaitCount(50);
        // charge pump OP mode
        pGBL->GBL_DPPL4_PARAM[0x04] |= DPLL3_CP_OPMODE; // 0x5d49

        //Update DPLL4 configuration
        pGBL->GBL_DPLL4_CFG |= DPLL_UPDATE_PARAM; // 0x5d40
        MMPF_PLL_PowerUp(DPLL4, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL4, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL4, 0);
    }

    if (pllSettings[DPLL5].freq != EXT_CLK) {
        //Adjust Analog gain settings
        //below clcok region is (24 *(((N*x).Frac)/P/O)). It doesn't include post divider

        // Start-UP @ Diode Mode
        pGBL->GBL_DPPL5_PARAM[0x05] = 0x04; // 0x5d5a
        // PLL Settings
        pGBL->GBL_DPPL5_PARAM[0x04] = DPLL5_DIV2_LOOPDIV_EN | DPLL5_SS_DOWN; // 5d59
        pGBL->GBL_DPPL5_PARAM[0x03] = DPLL5_CHARGE_PUMP_5uA; // 5d58
        pGBL->GBL_DPPL5_PARAM[0x03] |= DPLL5_KVCO_OFFSET_10uA | 0x08; // 5d58
        // Post Divider Divide 2
        pGBL->GBL_DPPL5_PARAM[0x02] = pllSettings[DPLL5].M; // 5d57
        // Fractional Part = 0.6666
        if (pllSettings[DPLL5].freq == 400000 ||
            pllSettings[DPLL5].freq == 800000){
            pGBL->GBL_DPPL5_PARAM[0x02] |= 0x02; // 5d57
            pGBL->GBL_DPPL5_PARAM[0x01] = 0xAA; // 5d56
            pGBL->GBL_DPPL5_PARAM[0x00] = 0xAA; // 5d55
        }
        else {
            pGBL->GBL_DPPL5_PARAM[0x02] &= ~(0x03); // 5d57
            pGBL->GBL_DPPL5_PARAM[0x01] = 0x00; // 5d56
            pGBL->GBL_DPPL5_PARAM[0x00] = 0x00; // 5d55
        }
        // Loop Divider N part
        pGBL->GBL_DPLL5_N = pllSettings[DPLL5].N; // 0x5d54
        // Pre-Divider Pass
        pGBL->GBL_DPLL5_P = pllSettings[DPLL5].K; // 0x5d53
        MMPF_PLL_WaitCount(50);
        /*// change into  OP mode
        pGBL->GBL_DPPL5_PARAM[0x05] = 0x02; // 0x5d5a
        MMPF_PLL_WaitCount(50);
        //Update DPLL5 configuration
        pGBL->GBL_DPLL5_CFG |= DPLL_UPDATE_PARAM; // 0x5d50
        // PLL bug, must update twice.*/
        // change into  OP mode
        pGBL->GBL_DPPL5_PARAM[0x05] = 0x02; // 0x5d5a

        MMPF_PLL_WaitCount(50);
        //Update DPLL5 configuration
        pGBL->GBL_DPLL5_CFG |= DPLL_UPDATE_PARAM; // 0x5d50

        MMPF_PLL_PowerUp(DPLL5, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL5, 1);
    }
    else {
        MMPF_PLL_PowerUp(DPLL5, 0);
    }

    //wait PLL power up time, min 50us needed
    MMPF_PLL_WaitCount(200);

    //Step 4: Setup each group's mux and diviver
    MMPF_PLL_ConfigGroupClk();

    //Step 5: Switch clock by using the target PLLs
    pGBL->GBL_DPLL0_CFG &= ~DPLL_BYPASS;
    pGBL->GBL_DPLL1_CFG &= ~DPLL_BYPASS;
    pGBL->GBL_DPLL2_CFG &= ~DPLL_BYPASS;
    pGBL->GBL_DPLL3_CFG &= ~DPLL_BYPASS;
    pGBL->GBL_DPLL4_CFG &= ~DPLL_BYPASS;
    pGBL->GBL_DPLL5_CFG &= ~DPLL_BYPASS;

    MMPF_PLL_WaitCount(5000);
    RTNA_DBG_Open((gGrpFreqKHz[CLK_GRP_GBL] >> 1), 115200);

    RTNA_DBG_PrintLong(3, gCpuFreqKHz);
    return MMP_ERR_NONE;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_Initialize
//  Description :
//------------------------------------------------------------------------------
extern void MMPF_SYS_AddTimerMark(MMP_ULONG id);
MMP_ERR MMPF_PLL_Initialize(void) ITCMFUNC;
MMP_ERR MMPF_PLL_Initialize(void)
{
    MMP_ULONG grp;
    #if (DRAM_ID == DRAM_DDR)
    AITPS_GBL pGBL = AITC_BASE_GBL;
	MMP_UBYTE id = pGBL->GBL_CHIP_VER;

	if ((id & 0x03) == 0x00) {
        // 8428D dram speed can't be 200MHz, set to 360MHz/2 = 180MHz.
        gGrpClkCfg[CLK_GRP_DRAM].src = DPLL1;
        gGrpClkCfg[CLK_GRP_DRAM].div = 2;
    }
	#endif
    /* #if(USE_DIV_CONST) */
        /* for(grp = CLK_GRP_GBL; grp < CLK_GRP_NUM; grp++) { */
                /* #if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168) */
                /* if(grp == CLK_GRP_GBL)  */
                    /* gGrpFreqKHz[grp] = 168000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /3; <] */
                /* #else */
                /* if(grp == CLK_GRP_GBL)  */
                    /* gGrpFreqKHz[grp] = 264000;[>252000;<] */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /2; <] */
                /* #endif */

                /* if((grp == CLK_GRP_CPUA) || (grp ==CLK_GRP_CPUB )) */
                    /* gGrpFreqKHz[grp] = 528000;[>504000;<] */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */

                /* #if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2) */
                /* if(grp==CLK_GRP_DRAM) */
                    /* gGrpFreqKHz[grp] = 400000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */
                /* #endif */

                /* #if (DRAM_ID == DRAM_DDR) */
                /* if(grp==CLK_GRP_DRAM) */
                    /* gGrpFreqKHz[grp] = 200000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /2; <] */
                /* #endif */

                /* //    { DPLL3, 6 }, // CLK_GRP_AUD 24.576MHz (may be changed in driver) */
                /* //    { DPLL0, 2 }, // CLK_GRP_SNR    200MHz */
                /* //    { DPLL0, 2 }, // CLK_GRP_BAYER  200MHz */
                /* //    { DPLL0, 2 }, // CLK_GRP_COLOR  200MHz */
                /* //    { PMCLK, 1 }, // CLK_GRP_USB     24MHz  */
                /* //    { DPLL4, 7 }, // CLK_GRP_TV      54MHz */
                /* //    { PMCLK, 1 }, // CLK_GRP_HDMI    24MHz (may be changed in driver) */
                /* //    { PMCLK, 1 }, // CLK_GRP_MIPITX  24MHz */
                /* //    { PMCLK, 1 }, // CLK_GRP_RXBIST  24MHz */
                /* if(grp==CLK_GRP_AUD) */
                    /* gGrpFreqKHz[grp] = 4096; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /6; <] */
                /* if(grp==CLK_GRP_SNR) */
                    /* gGrpFreqKHz[grp] = 200000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /2; <] */
                /* if(grp==CLK_GRP_BAYER) */
                    /* gGrpFreqKHz[grp] = 200000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /2; <] */
                /* if(grp==CLK_GRP_COLOR) */
                    /* gGrpFreqKHz[grp] = 200000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /2; <] */
                /* if(grp==CLK_GRP_USB) */
                    /* gGrpFreqKHz[grp] = 24000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */
                /* if(grp==CLK_GRP_TV) */
                    /* gGrpFreqKHz[grp] = 54000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /7; <] */
                /* if(grp==CLK_GRP_HDMI) */
                    /* gGrpFreqKHz[grp] = 24000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */
                /* if(grp==CLK_GRP_MIPITX) */
                    /* gGrpFreqKHz[grp] = 24000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */
                /* if(grp==CLK_GRP_RXBIST) */
                    /* gGrpFreqKHz[grp] = 24000; */
                    /* [> gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /1; <] */
        /* } */
    for(grp = CLK_GRP_GBL; grp < CLK_GRP_NUM; grp++) {
        gGrpFreqKHz[grp] = m_PllSettings[gGrpClkCfg[grp].src].freq /
                                         gGrpClkCfg[grp].div;
    }

    #if (CPU_ID == CPU_A)
    gCpuFreqKHz = gGrpFreqKHz[CLK_GRP_CPUA];
    #endif
    #if (CPU_ID == CPU_B)
    gCpuFreqKHz = gGrpFreqKHz[CLK_GRP_CPUB];
    #endif

#if defined(SD_BOOT)||defined(FAT_BOOT)||defined(MINIBOOT_FW)||defined(UPDATER_FW)
    return MMPF_PLL_Setting();
#else
    RTNA_DBG_Open((gGrpFreqKHz[CLK_GRP_GBL] >> 1), 115200);
    return MMP_ERR_NONE;
#endif
}

#if !defined(MINIBOOT_FW)
//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_ChangeAudioClock
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_PLL_ChangeAudioClock(   MMPF_PLL_MODE   ulSampleRate,
                                            MMPF_AUDSRC     AudPath)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;

    if (AudPath != MMPF_AUDSRC_MCLK) {
        RTNA_DBG_Str(0, "Unsupported audio PLL config!\r\n");
        return MMP_PLL_ERR_PARAMETER;
    }

    pGBL->GBL_DPLL3_CFG |= DPLL_BYPASS;

    // Clock out = (24 * (1/(M+1)*(N+1).Frac))
    if (m_bAudSampleGroup == 0) {
        /* PLL output frequency: 147.456Mhz */
	    pGBL->GBL_DPLL3_M = DPLL3_M(2); // 0x5d33
	    pGBL->GBL_DPLL3_N = DPLL3_N(17); // 0x5d34
	    // Fractional Part
	    pGBL->GBL_DPPL3_PARAM[0x00] = 0x5E; // 0x5d35
	    pGBL->GBL_DPPL3_PARAM[0x01] = 0xBA; // 0x5d36
	    pGBL->GBL_DPPL3_PARAM[0x02] = 0x01; // 0x5d37

        // 24.576MHz
        pGBL->GBL_AUD_CLK_DIV[0x00] = GRP_CLK_DIV_EN | GRP_CLK_DIV(6); // 0x5da1
        gGrpFreqKHz[CLK_GRP_AUD] = AUDIO_GRP0_FREQ;
	}
    else {
        /* PLL output frequency: 22.579Mhz */
	    pGBL->GBL_DPLL3_M = DPLL3_M(19); // 0x5d33
	    pGBL->GBL_DPLL3_N = DPLL3_N(17); // 0x5d34
	    // Fractional Part
	    pGBL->GBL_DPPL3_PARAM[0x00] = 0x95; // 0x5d35
	    pGBL->GBL_DPPL3_PARAM[0x01] = 0x43; // 0x5d36
	    pGBL->GBL_DPPL3_PARAM[0x02] = 0x03; // 0x5d37

        // 11.2896MHz
	    pGBL->GBL_AUD_CLK_DIV[0x00] = GRP_CLK_DIV_EN | GRP_CLK_DIV(2); // 0x5da1
        gGrpFreqKHz[CLK_GRP_AUD] = AUDIO_GRP1_FREQ;
	}

    pGBL->GBL_DPLL3_CFG |= DPLL_UPDATE_PARAM;
    MMPF_PLL_WaitCount(50);
    pGBL->GBL_DPLL3_CFG &= ~DPLL_BYPASS;

	return MMP_ERR_NONE;
}    

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_SetAudioPLL
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_SetAudioPLL(   MMPF_AUDSRC         AudPath,
                                MMPF_PLL_MODE       ulSampleRate,
                                MMP_AUD_INOUT_PATH  ubPath)
{
	AITPS_GBL pGBL = AITC_BASE_GBL;
	MMP_ERR   err = MMP_ERR_NONE;
    MMP_UBYTE group = 0;

    /* Distinguish the audio frequency group */
    if ((ulSampleRate != MMPF_PLL_AUDIO_44d1K) && 
		(ulSampleRate != MMPF_PLL_AUDIO_22d05K) &&
		(ulSampleRate != MMPF_PLL_AUDIO_11d025K)) {
		group = 0;  // 48KHz, 32KHz, 24KHz, 16KHz, 8KHz
    }
    else {
        group = 1;  // 44.1KHz, 22.05KHz, 11.025KHz
    }

    if (m_bAudSampleGroup != group) {
        /* Must update m_bAudSampleGroup first before calling
         * MMPF_PLL_ChangeAudioClock().
         */
        m_bAudSampleGroup = group;
        MMPF_PLL_ChangeAudioClock(ulSampleRate, AudPath); 
    }

    if ((ubPath == MMP_AUD_AFE_IN) && (ulSampleRate != m_bADCSamplerate)) {
        switch(ulSampleRate) {
        /* group 0 frequency */
        case MMPF_PLL_AUDIO_48K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(4);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(2);   // x5da3
            break;
        case MMPF_PLL_AUDIO_32K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(6);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(3);   // x5da3
            break;
        case MMPF_PLL_AUDIO_24K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(8);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(4);   // x5da3
            break;
        case MMPF_PLL_AUDIO_16K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(12);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(6);   // x5da3
            break;
        case MMPF_PLL_AUDIO_12K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(16);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(8);   // x5da3
            break;
        case MMPF_PLL_AUDIO_8K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(24);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(12);  // x5da3
            break;
        /* group 1 frequency */
        case MMPF_PLL_AUDIO_44d1K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(2);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(1);   // x5da3
            break;
        case MMPF_PLL_AUDIO_22d05K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(4);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(2);   // x5da3
            break;
        case MMPF_PLL_AUDIO_11d025K:
            pGBL->GBL_AUD_CLK_DIV[0x01] = AUD_CLK_DIV(8);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x02] = AUD_CLK_DIV(4);   // x5da3
            break;
        default:
            //
            break;
        }
        m_bADCSamplerate = ulSampleRate;
    }
    else if ((ubPath == MMP_AUD_AFE_OUT) && (ulSampleRate != m_bDACSamplerate)) {
        switch(ulSampleRate) {
        /* group 0 frequency */
        case MMPF_PLL_AUDIO_48K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(4);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(2);   // x5da3
            break;
        case MMPF_PLL_AUDIO_32K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(6);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(3);   // x5da3
            break;
        case MMPF_PLL_AUDIO_24K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(8);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(4);   // x5da3
            break;
        case MMPF_PLL_AUDIO_16K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(12);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(6);   // x5da3
            break;
        case MMPF_PLL_AUDIO_12K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(16);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(8);   // x5da3
            break;
        case MMPF_PLL_AUDIO_8K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(24);  // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(12);  // x5da3
            break;
        /* group 1 frequency */
        case MMPF_PLL_AUDIO_44d1K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(2);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(1);   // x5da3
            break;
        case MMPF_PLL_AUDIO_22d05K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(4);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(2);   // x5da3
            break;
        case MMPF_PLL_AUDIO_11d025K:
            pGBL->GBL_AUD_CLK_DIV[0x03] = AUD_CLK_DIV(8);   // x5da2
            pGBL->GBL_AUD_CLK_DIV[0x04] = AUD_CLK_DIV(4);   // x5da3
            break;
        default:
            //
            break;
        }
        m_bDACSamplerate = ulSampleRate;
    }

	return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_SetTVCLK
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_SetTVCLK(void)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;
    MMP_ERR   err = MMP_ERR_NONE;
    const MMPF_PLL_SETTINGS *pllSettings;

    pllSettings = m_PllSettings;

    switch(gGrpClkCfg[CLK_GRP_TV].src) {
    case DPLL0:
        // 432MHZ
        // power-on & BIST off
        pGBL->GBL_DPLL0_PWR = 0x00;// 0x5d02
        // pre-divider
        pGBL->GBL_DPLL0_M = DPLL0_M_DIV_1; // 0x5d03
        // loop-divider
        pGBL->GBL_DPLL0_N = DPLL0_N(54); // 0x5d04
        // post-divider
        pGBL->GBL_DPPL0_PARAM[0x00] = DPLL0_P_DIV_3; // 0x5d05
        // PLL setting VCO 400~600MHz
        pGBL->GBL_DPPL0_PARAM[0x01] = DPLL0_ICP_15uA | DPLL0_KVCO_OFFSET_5uA | DPLL0_VCO_CTL_1d5G; // 0x5d06
        // PLL control setting CTAT
        pGBL->GBL_DPPL0_PARAM[0x02] = 0x40; // 0x5d07

        // Update DPLL0 configuration
        pGBL->GBL_DPLL0_CFG |= DPLL_UPDATE_PARAM;
        MMPF_PLL_PowerUp(DPLL0, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL0, 1);
        // Tv divide
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(0x08);
        break;
    case DPLL1:
        // 432MHZ
        // power-on & BIST off
        pGBL->GBL_DPLL1_PWR = 0x00;// 0x5d12
        // pre-divider
        pGBL->GBL_DPLL1_M = DPLL0_M_DIV_1; // 0x5d13
        // loop-divider
        pGBL->GBL_DPLL1_N = DPLL0_N(54); // 0x5d14
        // post-divider
        pGBL->GBL_DPPL1_PARAM[0x00] = DPLL0_P_DIV_3; // 0x5d15
        // PLL setting VCO 400~600MHz
        pGBL->GBL_DPPL1_PARAM[0x01] = DPLL0_ICP_15uA | DPLL0_KVCO_OFFSET_5uA | DPLL0_VCO_CTL_1d5G; // 0x5d16
        // PLL control setting CTAT
        pGBL->GBL_DPPL1_PARAM[0x02] = 0x40; // 0x5d17

        // Update DPLL1 configuration
        pGBL->GBL_DPLL1_CFG |= DPLL_UPDATE_PARAM;
        MMPF_PLL_PowerUp(DPLL1, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL1, 1);
        // Tv divide
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(0x08);
        break;
    case DPLL2:
        // 432MHZ
        // power-on & BIST off
        pGBL->GBL_DPLL2_PWR = 0x00;// 0x5d22
        // pre-divider
        pGBL->GBL_DPLL2_M = DPLL0_M_DIV_1; // 0x5d23
        // loop-divider
        pGBL->GBL_DPLL2_N = DPLL0_N(54); // 0x5d24
        // post-divider
        pGBL->GBL_DPPL2_PARAM[0x00] = DPLL0_P_DIV_3; // 0x5d25
        // PLL setting VCO 600~800MHz
        pGBL->GBL_DPPL2_PARAM[0x01] = DPLL0_ICP_15uA | DPLL0_KVCO_OFFSET_5uA | DPLL0_VCO_CTL_1d5G; // 0x5d26
        // PLL control setting CTAT
        pGBL->GBL_DPPL2_PARAM[0x02] = 0x40; // 0x5d27

        // Update DPLL2 configuration
        pGBL->GBL_DPLL2_CFG |= DPLL_UPDATE_PARAM;
        MMPF_PLL_PowerUp(DPLL2, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL2, 1);
        // Tv divide
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(0x08);
        break;
    case DPLL3:
        // 54MHZ
        // Post-Divider
        pGBL->GBL_DPLL3_M = 0x07;
        // Loop-Divider
        pGBL->GBL_DPLL3_N = 0x11;
        // Fractional Part
        pGBL->GBL_DPPL3_PARAM[0x00] = 0x00;
        pGBL->GBL_DPPL3_PARAM[0x01] = 0x00;
        pGBL->GBL_DPPL3_PARAM[0x02] = 0x00;
        // PLL Control Setting
        pGBL->GBL_DPPL3_PARAM[0x03] = DPLL3_FRAC_DIS;
        // Enable BOOST CP & MAIN CP DIODE MODE
        pGBL->GBL_DPPL3_PARAM[0x04] = DPLL3_BOOST_CP_EN | DPLL3_MAIN_CP_VP_DIODE;
        MMPF_PLL_WaitCount(50);
        // Enable BOOST CP & MAIN CP OP MODE
        pGBL->GBL_DPPL3_PARAM[0x04] &= ~DPLL3_MAIN_CP_VP_DIODE;

        MMPF_PLL_WaitCount(50);
        // Update DPLL3 configuration
        pGBL->GBL_DPLL3_CFG |= DPLL_UPDATE_PARAM;
        //MMPF_PLL_PowerUp(DPLL3, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL3, 1);
        break;
    case DPLL4:
        // 378MHZ
        // Pre-Divider
        pGBL->GBL_DPLL4_M = 0x17;
        // Loop-Divider
        pGBL->GBL_DPLL4_N = 0xBC;
        // SS frequency divider
        pGBL->GBL_DPPL4_PARAM[0x03] = 0x62;
        // VCO frequency, CP current
        pGBL->GBL_DPPL4_PARAM[0x04] = DPLL4_800MHZ | DPLL4_CP_5uA;
        pGBL->GBL_DPPL4_PARAM[0x00] = 0x00;
        // SS-EN, Frange Setting
        pGBL->GBL_DPPL4_PARAM[0x01] = DPLL4_P_DIV_2;
        // SS source select
        pGBL->GBL_DPPL4_PARAM[0x02] = DPLL4_SS_SRCSEL_0d31uA;
        MMPF_PLL_WaitCount(50);
        // charge pump OP mode
        pGBL->GBL_DPPL4_PARAM[0x04] |= DPLL3_CP_OPMODE;

        // Update DPLL4 configuration
        pGBL->GBL_DPLL4_CFG |= DPLL_UPDATE_PARAM;
        MMPF_PLL_PowerUp(DPLL4, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL4, 1);
        // Tv divide
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(0x07);
        break;
    case DPLL5:
        // 378MHZ
        // Start-UP @ Diode Mode
        pGBL->GBL_DPPL5_PARAM[0x05] = 0x00;
        // PLL Settings
        pGBL->GBL_DPPL5_PARAM[0x04] = DPLL5_DIV2_LOOPDIV_EN;
        pGBL->GBL_DPPL5_PARAM[0x03] = DPLL5_CHARGE_PUMP_5uA;
        pGBL->GBL_DPPL5_PARAM[0x03] |= DPLL5_KVCO_OFFSET_5uA;
        // Post Divider Divide 2
        pGBL->GBL_DPPL5_PARAM[0x02] = DPLL5_OUTPUT_DIV_2;
        // Fractional Part = 0.6666
        if (pllSettings[DPLL5].freq == 400000 ||
            pllSettings[DPLL5].freq == 800000){
            pGBL->GBL_DPPL5_PARAM[0x02] |= 0x03;
            pGBL->GBL_DPPL5_PARAM[0x01] = 0x00;
            pGBL->GBL_DPPL5_PARAM[0x00] = 0x00;
        }
        // Loop Divider N part
        pGBL->GBL_DPLL5_N = 0x0F;
        // Pre-Divider Pass
        pGBL->GBL_DPLL5_P = DPLL5_P_DIV_1;
        MMPF_PLL_WaitCount(50);
        // change into  OP mode
        pGBL->GBL_DPPL5_PARAM[0x05] = 0x02;
        MMPF_PLL_WaitCount(50);

        //Update DPLL5 configuration
        pGBL->GBL_DPLL5_CFG |= DPLL_UPDATE_PARAM;
        MMPF_PLL_PowerUp(DPLL5, 0);
        MMPF_PLL_WaitCount(50);
        MMPF_PLL_PowerUp(DPLL5, 1);
        // Tv divide
        pGBL->GBL_TV_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(0x07);
        break;
    default:
        break;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_ChangeVIFClock
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_ChangeVIFClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;

    pGBL->GBL_VIF_CLK_SRC = PLLSrc;
    pGBL->GBL_VIF_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(ulDivNum);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_ChangeISPClock
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_ChangeISPClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;

    pGBL->GBL_ISP_CLK_SRC = PLLSrc;
    pGBL->GBL_ISP_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(ulDivNum);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_PLL_ChangeBayerClock
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_PLL_ChangeBayerClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum)
{
    AITPS_GBL pGBL = AITC_BASE_GBL;

    pGBL->GBL_BAYER_CLK_SRC = PLLSrc;
    pGBL->GBL_BAYER_CLK_DIV = GRP_CLK_DIV_EN | GRP_CLK_DIV(ulDivNum);

    return MMP_ERR_NONE;
}



#endif // !defined(MINIBOOT_FW)

