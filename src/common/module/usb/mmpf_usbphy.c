//==============================================================================
//
//  File        : mmpf_usbphy.c
//  Description : Firmware USB PHY Driver
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================
#include "includes_fw.h"
#include "lib_retina.h"

#include "mmp_reg_usb.h"
#include "mmpf_usbphy.h"

/** @addtogroup MMPF_USB
@{
*/

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
#define PHY_DEV_ADDR_BITS       (8)
#define PHY_WORD_ADDR_BITS      (8)
#define PHY_WORD_DATA_BITS      (16)

#define PHY_WR_DEV_ADDR         (0x20)
#define PHY_RD_DEV_ADDR         (0x21)

//==============================================================================
//
//                              LOCAL VARIABLES
//
//==============================================================================
static const MMP_UBYTE mbSpiStartOpr[1] = {
    OPR_SS_HIGH,    //chip select inactive
};

static const MMP_UBYTE mbSpiStopOpr[3] = {
    OPR_SS_LOW | OPR_SCLK_LOW,
    OPR_SS_LOW | OPR_SCLK_LOW,
    OPR_SS_HIGH,    //chip select inactive
};

static MMP_UBYTE    mbSpiDevAddrOpr[PHY_DEV_ADDR_BITS << 1];
static MMP_UBYTE    mbSpiWordAddrOpr[PHY_WORD_ADDR_BITS << 1];
static MMP_UBYTE    mbSpiWordDataOpr[PHY_WORD_DATA_BITS << 1];

//==============================================================================
//
//                              EXTERN VARIABLES
//
//==============================================================================

//==============================================================================
//
//                              EXTERN FUNCTIONS
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION PROTOTYPE
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_USBPHY_SetSpiWaveformOpr
//  Description : Set waveform OPR according to the specified value and bit num
//------------------------------------------------------------------------------
void MMPF_USBPHY_SetSpiWaveformOpr(MMP_UBYTE *opr, MMP_USHORT value, MMP_LONG bits)
{
    MMP_LONG i;

    // MSB bit rx/tx first
    for(i = (bits << 1) - 1; i > 0; i -= 2) {
        if (value & 0x01) {
            opr[i]   = OPR_MOSI_HIGH | OPR_SS_LOW | OPR_SCLK_HIGH;
            opr[i-1] = OPR_MOSI_HIGH | OPR_SS_LOW;
        }
        else {
            opr[i]   = OPR_MOSI_LOW | OPR_SS_LOW | OPR_SCLK_HIGH;
            opr[i-1] = OPR_MOSI_LOW | OPR_SS_LOW;
        }
        // check bit by bit
        value >>= 1;
    }
}

#if 0
//------------------------------------------------------------------------------
//  Function    : MMPF_USBPHY_Read
//  Description : Read USB PHY controller register
//------------------------------------------------------------------------------
MMP_USHORT MMPF_USBPHY_Read(MMP_UBYTE addr)
{
    MMP_ULONG i, ofst = PHY_WORD_DATA_BITS - 1;
    MMP_USHORT data = 0;
    AITPS_USB_DMA pUSB_DMA = AITC_BASE_USBDMA;

    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiDevAddrOpr, PHY_RD_DEV_ADDR, PHY_DEV_ADDR_BITS);
    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiWordAddrOpr, addr, PHY_WORD_ADDR_BITS);
    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiWordDataOpr, 0, PHY_WORD_DATA_BITS);

    for(i = 0; i < sizeof(mbSpiStartOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiStartOpr[i];
    for(i = 0; i < sizeof(mbSpiDevAddrOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiDevAddrOpr[i];
    for(i = 0; i < sizeof(mbSpiWordAddrOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiWordAddrOpr[i];
    for(i = 0; i < sizeof(mbSpiWordDataOpr); i++) {
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiWordDataOpr[i];
        if (mbSpiWordDataOpr[i] & OPR_SCLK_HIGH) {
            data |= (pUSB_DMA->USB_PHY_SPI_CTL2 & OPR_MISO_HIGH) << ofst;
            ofst--;
        }
    }
    for(i = 0; i < sizeof(mbSpiStopOpr); i++)
         pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiStopOpr[i];

    #if 0
    RTNA_DBG_Str(0, "USB_PHY Get [");
    RTNA_DBG_Byte(0, addr + 1);
    RTNA_DBG_Str(0, ",");
    RTNA_DBG_Byte(0, addr);
    RTNA_DBG_Str(0, "]:");
    RTNA_DBG_Short(0, data);
    RTNA_DBG_Str(0, "\r\n");
    #endif

    return data;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_USBPHY_Write
//  Description : Write USB PHY controller register
//------------------------------------------------------------------------------
void MMPF_USBPHY_Write(MMP_UBYTE addr, MMP_USHORT data)
{
    MMP_ULONG i;
    AITPS_USB_DMA pUSB_DMA = AITC_BASE_USBDMA;

    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiDevAddrOpr, PHY_WR_DEV_ADDR, PHY_DEV_ADDR_BITS);
    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiWordAddrOpr, addr, PHY_WORD_ADDR_BITS);
    MMPF_USBPHY_SetSpiWaveformOpr(mbSpiWordDataOpr, data, PHY_WORD_DATA_BITS);

    for(i = 0; i < sizeof(mbSpiStartOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiStartOpr[i];
    for(i = 0; i < sizeof(mbSpiDevAddrOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiDevAddrOpr[i];
    for(i = 0; i < sizeof(mbSpiWordAddrOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiWordAddrOpr[i];
    for(i = 0; i < sizeof(mbSpiWordDataOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiWordDataOpr[i];
    for(i = 0; i < sizeof(mbSpiStopOpr); i++)
        pUSB_DMA->USB_PHY_SPI_CTL1 = mbSpiStopOpr[i];

    #if 0
    RTNA_DBG_Str(0, "USB_PHY Set [");
    RTNA_DBG_Byte(0, addr + 1);
    RTNA_DBG_Str(0, ",");
    RTNA_DBG_Byte(0, addr);
    RTNA_DBG_Str(0, "]:");
    RTNA_DBG_Short(0, data);
    RTNA_DBG_Str(0, "\r\n");
    #endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_USBPHY_PowerDown
//  Description : Power down USB PHY
//------------------------------------------------------------------------------
void MMPF_USBPHY_PowerDown(void)
{
    MMPF_USBPHY_Write(PWR_CTL_PUPD_TEST,
                      DM_PULL_DOWN | DP_PULL_UP1 | HSTX_PWR_DOWN |
                      HSRX_PWR_DOWN | FSLS_TX_PWR_DOWN | FSLS_RX_PWR_DOWN |
                      DISCON_DTC_PWR_DOWN | SQ_DTC_PWR_DOWN |
                      USB_BS_PWR_DOWN | USB_BG_PWR_DOWN);

    MMPF_USBPHY_Write(PLL_TEST_OTG_CTL, PLL_PWR_DOWN | XO_BLOCK_OFF |
                      REFCLK_CORE_OFF | ANA_TEST_MODE | OTG_VREF_PWR_DOWN);
}


/// @}
