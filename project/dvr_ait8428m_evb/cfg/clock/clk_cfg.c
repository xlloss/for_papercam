//------------------------------------------------------------------------------
//
//  File        : clk_cfg.c
//  Description : Source file of clock frequency configuration
//  Author      : Alterman
//  Revision    : 1.0
//
//------------------------------------------------------------------------------

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "clk_cfg.h"
#include "mmpf_dram.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * The following table decides the clock frequency of each clock domain.
 * The settings could seriously impact the system reliability & performance,
 * you MUST modifiy it carefully.
 *
 * The clock frequency of each group depends on the frequency of its source
 * (PLL or PMCLK), and its clock divider.
 */
GRP_CLK_CFG gGrpClkCfg[CLK_GRP_NUM] = {

#if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168)
/*    src    div  */
    { DPLL2, 3 }, // CLK_GRP_GBL    168MHz
    { DPLL2, 1 }, // CLK_GRP_CPUA   504Mhz
    { DPLL2, 1 }, // CLK_GRP_CPUB   504Mhz
#if (DRAM_ID == DRAM_DDR)
    { DPLL0, 2 }, // CLK_GRP_DRAM   200MHz/180MHz
#endif
#if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2)
    { DPLL0, 1 }, // CLK_GRP_DRAM   400MHz
#endif
    { DPLL3, 6 }, // CLK_GRP_AUD 24.576MHz (may be changed in driver)
    { DPLL0, 2 }, // CLK_GRP_SNR    200MHz
    { DPLL0, 2 }, // CLK_GRP_BAYER  200MHz
    { DPLL0, 2 }, // CLK_GRP_COLOR  200MHz
    { PMCLK, 1 }, // CLK_GRP_USB     24MHz 
    { DPLL4, 7 }, // CLK_GRP_TV      54MHz
    { PMCLK, 1 }, // CLK_GRP_HDMI    24MHz (may be changed in driver)
    { PMCLK, 1 }, // CLK_GRP_MIPITX  24MHz
    { PMCLK, 1 }, // CLK_GRP_RXBIST  24MHz
#endif


#if (PLL_CONFIG==PLL_FOR_POWER)||(PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192)
/*    src    div  */
    { DPLL2, 2 }, // CLK_GRP_GBL    264MHz
    { DPLL2, 1 }, // CLK_GRP_CPUA   528MHz
    { DPLL2, 1 }, // CLK_GRP_CPUB   528MHz
#if (DRAM_ID == DRAM_DDR)
    { DPLL0, 3 }, // CLK_GRP_DRAM   200MHz/180MHz
#endif
#if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2)
    { DPLL0, 1 }, // CLK_GRP_DRAM   400MHz
#endif
    { DPLL3, 6 }, // CLK_GRP_AUD 24.576MHz (may be changed in driver)
    { DPLL0, 2 }, // CLK_GRP_SNR    200MHz
    { DPLL0, 2 }, // CLK_GRP_BAYER  200MHz
    { DPLL0, 2 }, // CLK_GRP_COLOR  200MHz
    { PMCLK, 1 }, // CLK_GRP_USB     24MHz 
    { DPLL4, 7 }, // CLK_GRP_TV      54MHz
    { PMCLK, 1 }, // CLK_GRP_HDMI    24MHz (may be changed in driver)
    { PMCLK, 1 }, // CLK_GRP_MIPITX  24MHz
    { PMCLK, 1 }, // CLK_GRP_RXBIST  24MHz
#endif
    
#if PLL_CONFIG==PLL_FOR_PERFORMANCE
/*    src    div  */
    { DPLL2, 2 }, // CLK_GRP_GBL    264MHz
    { DPLL2, 1 }, // CLK_GRP_CPUA   528MHz
    { DPLL2, 1 }, // CLK_GRP_CPUB   528MHz
#if (DRAM_ID == DRAM_DDR)
    //{ DPLL0, 3 }, // CLK_GRP_DRAM   200MHz/180MHz
    { DPLL0,3 }, // CLK_GRP_DRAM   200MHz/180MHz
#endif
#if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2)
    { DPLL0, 1 }, // CLK_GRP_DRAM   400MHz
#endif
    { DPLL3, 6 }, // CLK_GRP_AUD 24.576MHz (may be changed in driver)
    { DPLL0, 1 }, // CLK_GRP_SNR    400MHz
    { DPLL0, 1 }, // CLK_GRP_BAYER  400MHz
    { DPLL0, 1 }, // CLK_GRP_COLOR  400MHz
    { PMCLK, 1 }, // CLK_GRP_USB     24MHz 
    { DPLL4, 7 }, // CLK_GRP_TV      54MHz
    { PMCLK, 1 }, // CLK_GRP_HDMI    24MHz (may be changed in driver)
    { PMCLK, 1 }, // CLK_GRP_MIPITX  24MHz
    { PMCLK, 1 }, // CLK_GRP_RXBIST  24MHz
#endif
    
#if PLL_CONFIG==PLL_FOR_BURNING
/*    src    div  */
    { DPLL2, 2 }, // CLK_GRP_GBL    264MHz
    { DPLL2, 1 }, // CLK_GRP_CPUA   528MHz
    { DPLL2, 1 }, // CLK_GRP_CPUB   528MHz
#if (DRAM_ID == DRAM_DDR)
    { DPLL0, 3 }, // CLK_GRP_DRAM   200MHz/180MHz
#endif
#if (DRAM_ID == DRAM_DDR3)||(DRAM_ID == DRAM_DDR2)
    { DPLL0, 1 }, // CLK_GRP_DRAM   400MHz
#endif
    { DPLL3, 6 }, // CLK_GRP_AUD 24.576MHz (may be changed in driver)
    { DPLL2, 1 }, // CLK_GRP_SNR    528MHz
    { DPLL2, 1 }, // CLK_GRP_BAYER  528MHz
    { DPLL2, 1 }, // CLK_GRP_COLOR  528MHz
    { PMCLK, 1 }, // CLK_GRP_USB     24MHz 
    { DPLL4, 7 }, // CLK_GRP_TV      54MHz
    { DPLL4, 2 }, // CLK_GRP_HDMI    24MHz (may be changed in driver)
    { PMCLK, 1 }, // CLK_GRP_MIPITX  24MHz
    { PMCLK, 1 }, // CLK_GRP_RXBIST  24MHz
#endif    
};

