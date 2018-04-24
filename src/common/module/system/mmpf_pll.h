//==============================================================================
//
//  File        : mmpf_pll.h
//  Description : INCLUDE File for the Firmware PLL Driver.
//  Author      : Rogers Chen
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_PLL_H_
#define _MMPF_PLL_H_

#include "includes_fw.h"
#include "mmp_clk_inc.h"
#include "mmp_aud_inc.h"

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum _MMPF_PLL_MODE
{
    //For PLL3 to Audio(G5)
    MMPF_PLL_AUDIO_192K,
    MMPF_PLL_AUDIO_128K,
    MMPF_PLL_AUDIO_96K,
    MMPF_PLL_AUDIO_64K,
    MMPF_PLL_AUDIO_48K,
    MMPF_PLL_AUDIO_44d1K,
    MMPF_PLL_AUDIO_32K,
    MMPF_PLL_AUDIO_24K,
    MMPF_PLL_AUDIO_22d05K,
    MMPF_PLL_AUDIO_16K,
    MMPF_PLL_AUDIO_12K,
    MMPF_PLL_AUDIO_11d025K,
    MMPF_PLL_AUDIO_8K,

    MMPF_PLL_ExtClkOutput,
    MMPF_PLL_MODE_NUMBER
} MMPF_PLL_MODE;

typedef enum _MMPF_AUDSRC {
	MMPF_AUDSRC_MCLK = 0,
	MMPF_AUDSRC_I2S
} MMPF_AUDSRC;

typedef struct _MMPF_PLL_SETTINGS {
    MMP_UBYTE M;
    MMP_UBYTE N;
    MMP_UBYTE K;
    MMP_ULONG frac;
    MMP_UBYTE VCO;
    MMP_ULONG freq;
} MMPF_PLL_SETTINGS;

typedef enum _MMPF_PLL_HDMI_MODE
{
	MMPF_PLL_HDMI_27MHZ = 0,
	MMPF_PLL_HDMI_74_25MHZ,
	MMPF_PLL_HDMI_SYNC_DISPLAY
} MMPF_PLL_HDMI_MODE;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

void    MMPF_PLL_WaitCount(MMP_ULONG count);
MMP_ERR MMPF_PLL_PowerUp(GRP_CLK_SRC PLLNo, MMP_BOOL PLLPowerUp);
MMP_ERR MMPF_PLL_GetGroupFreq(CLK_GRP ubGroupNum, MMP_ULONG *ulGroupFreq);
MMP_ERR MMPF_PLL_GetGroupSrcAndDiv( CLK_GRP     ubGroupNum,
                                    GRP_CLK_SRC *clkSrc,
                                    MMP_UBYTE   *pubPllDiv);
MMP_ERR MMPF_PLL_Initialize(void);
MMP_ERR MMPF_PLL_SetAudioPLL(   MMPF_AUDSRC         AudPath,
                                MMPF_PLL_MODE       ulSampleRate,
                                MMP_AUD_INOUT_PATH  ubPath);
MMP_ERR MMPF_PLL_SetTVCLK(void);
MMP_ERR MMPF_PLL_ChangeVIFClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum);
MMP_ERR MMPF_PLL_ChangeISPClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum);
MMP_ERR MMPF_PLL_ChangeBayerClock(GRP_CLK_SRC PLLSrc, MMP_ULONG ulDivNum);

#endif // _MMPF_PLL_H_
