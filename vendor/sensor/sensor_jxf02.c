//==============================================================================
//
//  File        : sensor_JXF02.c
//  Description : Firmware Sensor Control File
//  Author      : Philip Lin
//  Revision    : 1.0
//
//=============================================================================

#include "includes_fw.h"
#include "Customer_config.h"

#if (SENSOR_EN)
#if (BIND_SENSOR_JXF02)

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmpf_sensor.h"
#include "mmpf_i2cm.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define SENSOR_IF 			(SENSOR_IF_MIPI_1_LANE)

// Customer Defined Index (Large to Small)
#define RES_IDX_1920x1080   (0)
//#define RES_IDX_1280x720    (1)

#define SENSOR_ROTATE_180   (0)

#define MAX_SENSOR_GAIN     (16)          // 16x

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

// Resolution Table
MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	1,				/* ubSensorModeNum */
	0,				/* ubDefPreviewMode */
	0,				/* ubDefCaptureMode */
	3000,			// usPixelSize
//  Mode0   Mode1
    {1},	/* usVifGrabStX */
    {1},	/* usVifGrabStY */
    {1928},	/* usVifGrabW */
    {1088},	/* usVifGrabH */
    #if (CHIP == MCR_V2)
    {1}, 	// usBayerInGrabX
	{1},// usBayerInGrabY
    {8},  /* usBayerDummyInX */
    {8},  /* usBayerDummyInY */
    {1920},	/* usBayerOutW */
    {1080},	/* usBayerOutH */
    #endif
    {1920},	/* usScalInputW */
    {1080},	/* usScalInputH */
    {300},	/* usTargetFpsx10 */
    {1130},	/* usVsyncLine */
    {1},  /* ubWBinningN */
    {1},  /* ubWBinningN */
    {1},  /* ubWBinningN */
    {1},  /* ubWBinningN */
    {0},    	// ubCustIQmode
    {0}     	// ubCustAEmode  
};

// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE       m_OprTable;
MMPF_SENSOR_VIF_SETTING     m_VifSetting;

// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
#if(CHIP == MCR_V2)
#include "isp_8428_iq_data_v2_AR0330_MP.xls.ciq.txt" // Tmp use AR0330 setting.
#endif
};

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = {
    MMP_I2CM0, // i2cmID
    0x40,       // ubSlaveAddr
    8,          // ubRegLen
    8,          // ubDataLen
    0,          // ubDelayTime
    MMP_FALSE,  // bDelayWaitEn
    MMP_TRUE,   // bInputFilterEn
    MMP_FALSE,  // b10BitModeEn
    MMP_FALSE,  // bClkStretchEn
    0,          // ubSlaveAddr1
    0,          // ubDelayCycle
    0,          // ubPadNum
    150000, //150KHZ,,,,,,400000,     // ulI2cmSpeed 400KHZ
    MMP_FALSE,   // bOsProtectEn
    0,       // sw_clk_pin
    0,       // sw_data_pin
    MMP_FALSE,  // bRfclModeEn
    MMP_FALSE,  // bWfclModeEn
    MMP_FALSE,  // bRepeatModeEn
    0           // ubVifPioMdlId
};

// 3A Timing
MMPF_SENSOR_AWBTIMIMG   m_AwbTime    = 
{
	6,	/* ubPeriod */
	1, 	/* ubDoAWBFrmCnt */
	2	/* ubDoCaliFrmCnt */
};

MMPF_SENSOR_AETIMIMG    m_AeTime     = 
{	
	4, /* ubPeriod */
	0, 	/* ubFrmStSetShutFrmCnt */
	0	/* ubFrmStSetGainFrmCnt */
};

MMPF_SENSOR_AFTIMIMG    m_AfTime     = 
{
	1, 	/* ubPeriod */
	0	/* ubDoAFFrmCnt */
};

#if (ISP_EN)
static ISP_UINT32 s_gain;
#endif

//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Sensor_OPR_Table____(){ruturn;} //dummy
#endif

MMP_USHORT SNR_JXF02_Reg_Init_Customer[] = 
{
0x12, 0x80,  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
};

// 1080p 30FPS
MMP_USHORT SNR_JXF02_Reg_1920x1080_Customer[] = 
{
//=========================================
//INI Create Date : 2015/9/15
//Terra Ver : Terra20150731-01
//Create By easonlin
//==================INI==================
//Product Ver:VAB07
//Output Detail:
//MCLK:24 MHz
//PCLK:81.6
//Mipi PCLK:81.6
//VCO:816
//FrameW:2408
//FrameH:1130

//[JXF02_1928x1092x30_Mipi_1L_10b.reg]
//INI Start
0x12, 0x40,  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
//DVP Setting
0x0D, 0xA4,  // DVP control 2
//PLL Setting
0x0E, 0x10,  // [1:0]: PLL pre-div(1+PLL[1:0]),
0x0F, 0x09,  // [3:0]: PLLclk = VCO/(1+PLL2[3:0]), [7:4]: Mipiclk = VCO/(1+PLL2[7:4])
0x10, 0x22,  // VCO = input clock * PLL3[7:0]/PLL_Pre_Ratio
0x11, 0x80,  // [7]: system clock use PLLclk.
//Frame/Window
0x20, 0xB4,  // FrameW[7:0] // 0x4B4*2 = 2408
0x21, 0x04,  // FrameW[15:8]
0x22, 0x6A,  // FrameH[7:0] // 0x46A = 1130
0x23, 0x04,  // FrameH[15:8]
0x24, 0xC4,  // Hwin[7:0]  // 0x3C4*2 = 0x788 = 1928 
0x25, 0x44,  // Vwin[7:0]  // 0x444 = 1092
0x26, 0x43,  // {Vwin[11:8], Hwin[11:8]}
0x27, 0x90,  // HwinSt[7:0]
0x28, 0x12,  // VwinSt[7:0]
0x29, 0x01,  // {VwinSt[11:8], HwinSt[11:8]}
0x2A, 0x82,  // RSVD
0x2B, 0x21,  // RSVD
0x2C, 0x02,  // SenHASt[7:0], by 8 pixels
0x2D, 0x00,  // SenVSt[7:0], by 4 lines
0x2E, 0x15,  // SenVEnd[7:0], by 4 lines
0x2F, 0x44,  // [1:0]: SenVSt[9:8], [3:2]: SenVEnd[9:8]
//Sensor Timing
0x31, 0x0C,
0x32, 0x20,
0x33, 0x18,
0x36, 0x00,
0x37, 0x68,
0x39, 0x92,
0x3A, 0x08,
//Interface
0x1D, 0x00,  // DVP control 3
0x1E, 0x00,  // DVP control 4
0x6C, 0x50,  // [4]: Second data lane disable on/off, 0: enable, 1: disable, [7]: Mipi interface power down, 0:power down, 1:normal
0x74, 0x52,
0x75, 0x2B,
0x70, 0xC1,  // [1]: 0: 10-bits, 1: 8-bits mode
0x72, 0xAA,
0x73, 0x53,
0x76, 0x6A,
0x77, 0x09,
0x78, 0x0F,
//Array/AnADC/PWC
0x41, 0xD1,
0x42, 0x23,
0x67, 0x70,
0x68, 0x02,
0x69, 0x74,
0x6A, 0x4A,
//SRAM/RAMP
0x5A, 0x04,
0x5C, 0x0f,
0x5D, 0x30,
0x5E, 0xe6,
0x60, 0x16,
0x62, 0x30,
0x64, 0x50,
0x65, 0x30,
//AE/AG/ABLC
0x47, 0x00,
0x50, 0x02,
0x13, 0x87,
0x14, 0x80,
0x16, 0xC0,
0x17, 0x40,
0x18, 0x1A,
0x19, 0xC5,
0x4a, 0x03,
0x49, 0x10,
//INI End
0x12, 0x00,  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
//Auto Vramp
0x45, 0x19,
0x1F, 0x01,
//PWDN Setting
//None
};

#if 0
void ____Sensor_Customer_Func____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_InitConfig
//  Description :
//------------------------------------------------------------------------------
static void SNR_Cust_InitConfig(void)
{
	RTNA_DBG_Str(0, "SNR_Cust_IniConfig JXF02\r\n");
	
    // Init OPR Table
    SensorCustFunc.OprTable->usInitSize                   = (sizeof(SNR_JXF02_Reg_Init_Customer)/sizeof(SNR_JXF02_Reg_Init_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                 = &SNR_JXF02_Reg_Init_Customer[0];    
    
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080]    = (sizeof(SNR_JXF02_Reg_1920x1080_Customer)/sizeof(SNR_JXF02_Reg_1920x1080_Customer[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080]  = &SNR_JXF02_Reg_1920x1080_Customer[0];

	#if (0)
    // Init Vif Setting : Common
    SensorCustFunc.VifSetting->SnrType                              = MMPF_VIF_SNR_TYPE_BAYER;
#if (SENSOR_IF == SENSOR_IF_PARALLEL)
	SensorCustFunc.VifSetting->OutInterface                         = MMPF_VIF_IF_PARALLEL;
#elif (SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
	SensorCustFunc.VifSetting->OutInterface 					    = MMPF_VIF_IF_MIPI_SINGLE_0;
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	SensorCustFunc.VifSetting->OutInterface 					    = MMPF_VIF_IF_MIPI_DUAL_01;
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	SensorCustFunc.VifSetting->OutInterface 					    = MMPF_VIF_IF_MIPI_QUAD;
#endif

    SensorCustFunc.VifSetting->VifPadId							    = MMPF_VIF_MDL_ID0;
    
    // Init Vif Setting : PowerOn Setting
 	/********************************************/
	// Power On serquence
	// 1. Supply Power
	// 2. Deactive RESET
	// 3. Enable MCLK
	// 4. Active RESET (1ms)
	// 5. Deactive RESET (Wait 150000 clock of MCLK, about 8.333ms under 24MHz)
	/********************************************/

    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnExtPower       = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin         = SENSOR_GPIO_ENABLE;	
	SensorCustFunc.VifSetting->powerOnSetting.bExtPowerPinHigh	    = SENSOR_GPIO_ENABLE_ACT_LEVEL; 	
    SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh       = (SENSOR_GPIO_ENABLE_ACT_LEVEL == GPIO_HIGH) ? MMP_TRUE : MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay     = 10;
    SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh        = (SENSOR_GPIO_ENABLE_ACT_LEVEL == GPIO_HIGH) ? MMP_FALSE : MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay      = 100;
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh      = (SENSOR_RESET_ACT_LEVEL == GPIO_HIGH) ? MMP_TRUE : MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay    = 100;
    SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh       = (SENSOR_RESET_ACT_LEVEL == GPIO_HIGH) ? MMP_FALSE : MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay     = 100;
    
    // Init Vif Setting : PowerOff Setting
    SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode    = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg     = 0x301A;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask    = 0x04;
    SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh           = GPIO_HIGH;
    SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay         = 20;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock       = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower     = MMP_TRUE;    
    SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin        = MMP_GPIO_MAX;
    
    // Init Vif Setting : Sensor MClock Setting
    SensorCustFunc.VifSetting->clockAttr.bClkOutEn                  = MMP_TRUE; 
    SensorCustFunc.VifSetting->clockAttr.ubClkFreqDiv               = 0;
    SensorCustFunc.VifSetting->clockAttr.ulMClkFreq                 = 24000;
    SensorCustFunc.VifSetting->clockAttr.ulDesiredFreq              = 24000;
    SensorCustFunc.VifSetting->clockAttr.ubClkPhase                 = MMPF_VIF_SNR_PHASE_DELAY_NONE;
    SensorCustFunc.VifSetting->clockAttr.ubClkPolarity              = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->clockAttr.ubClkSrc				    = MMPF_VIF_SNR_CLK_SRC_PMCLK;
    
    // Init Vif Setting : Parallel Sensor Setting
    SensorCustFunc.VifSetting->paralAttr.ubLatchTiming              = MMPF_VIF_SNR_LATCH_POS_EDGE;
    SensorCustFunc.VifSetting->paralAttr.ubHsyncPolarity            = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->paralAttr.ubVsyncPolarity            = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->paralAttr.ubBusBitMode               = MMPF_VIF_SNR_PARAL_BITMODE_16;
    
    // Init Vif Setting : MIPI Sensor Setting
    SensorCustFunc.VifSetting->mipiAttr.bClkDelayEn                 = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bClkLaneSwapEn              = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.usClkDelay                  = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubBClkLatchTiming           = MMPF_VIF_SNR_LATCH_NEG_EDGE;
    
	#if (SENSOR_IF != SENSOR_IF_PARALLEL)
	// Init Vif Setting : MIPI Sensor Setting
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0]                = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0]               = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]              = MMPF_VIF_MIPI_DATA_SRC_PHY_0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]                = 0x00;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]                = 0x00;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]                = 0x00;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]                = 0x00;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0]               = 0x3F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1]               = 0x3F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]               = 0x3F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]               = 0x3F;    dasdad
	#endif
	
    // Init Vif Setting : Color ID Setting
    SensorCustFunc.VifSetting->colorId.VifColorId              		  = MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId  	  = MMP_FALSE;
	
	#else///////////////////////////////////////////////////////////////////
	
	
	// Init Vif Setting : Common
    SensorCustFunc.VifSetting->SnrType                                = MMPF_VIF_SNR_TYPE_BAYER;
    #if (SENSOR_IF == SENSOR_IF_PARALLEL)
    SensorCustFunc.VifSetting->OutInterface                           = MMPF_VIF_IF_PARALLEL;
    #else
	SensorCustFunc.VifSetting->OutInterface 						  = MMPF_VIF_IF_MIPI_SINGLE_0;
    //SensorCustFunc.VifSetting->OutInterface                          = MMPF_VIF_IF_MIPI_QUAD;
    #endif

    SensorCustFunc.VifSetting->VifPadId								  = MMPF_VIF_MDL_ID0;
    
    // Init Vif Setting : PowerOn Setting
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnExtPower         = MMP_TRUE;
    #if (CHIP == MCR_V2)
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin           = MMP_GPIO_MAX;
    #endif   
    SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh         = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay       = 10;
    SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh          = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay        = 10;
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst   = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh        = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay      = 10;
    SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh         = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay       = 20;
    
    // Init Vif Setting : PowerOff Setting
    SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode      = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg       = 0x100;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask      = 0x0;    
    SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh             = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay           = 20;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock         = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower       = MMP_FALSE;    
    #if (CHIP == P_V2)
    SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin          = MMPF_PIO_REG_SGPIO0;
    #endif
    #if (CHIP == MCR_V2)
    SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin          = MMP_GPIO_MAX;
    #endif
    
    // Init Vif Setting : Sensor paralAttr Setting
    SensorCustFunc.VifSetting->clockAttr.bClkOutEn                    = MMP_TRUE; 
    SensorCustFunc.VifSetting->clockAttr.ubClkFreqDiv                 = 0;
    SensorCustFunc.VifSetting->clockAttr.ulMClkFreq                   = 24000;
    SensorCustFunc.VifSetting->clockAttr.ulDesiredFreq                = 24000;
    SensorCustFunc.VifSetting->clockAttr.ubClkPhase                   = MMPF_VIF_SNR_PHASE_DELAY_NONE;
    SensorCustFunc.VifSetting->clockAttr.ubClkPolarity                = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->clockAttr.ubClkSrc					  = MMPF_VIF_SNR_CLK_SRC_PMCLK;
        
    // Init Vif Setting : Parallel Sensor Setting
    SensorCustFunc.VifSetting->paralAttr.ubLatchTiming                = MMPF_VIF_SNR_LATCH_POS_EDGE;
    SensorCustFunc.VifSetting->paralAttr.ubHsyncPolarity              = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->paralAttr.ubVsyncPolarity              = MMPF_VIF_SNR_CLK_POLARITY_NEG;

	#if (SENSOR_IF != SENSOR_IF_PARALLEL)
	// Init Vif Setting : MIPI Sensor Setting
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0]                = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3]                = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0]               = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3]               = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3]            = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]              = 1;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]                = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]                = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]                = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]                = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0]               = 0x0F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1]               = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]               = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]               = 0x08;
	#endif
	
    // Init Vif Setting : Color ID Setting
    SensorCustFunc.VifSetting->colorId.VifColorId              		  = MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId  	  = MMP_FALSE;
	#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_JXF02_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
#if (ISP_EN)
    MMP_ULONG   ulVsync = 0;
    MMP_ULONG   ulShutter = 0;
	MMP_UBYTE   ubPeriod              = (SensorCustFunc.pAeTime)->ubPeriod;
	MMP_UBYTE   ubFrmStSetShutFrmCnt  = (SensorCustFunc.pAeTime)->ubFrmStSetShutFrmCnt;	
	MMP_UBYTE   ubFrmStSetGainFrmCnt  = (SensorCustFunc.pAeTime)->ubFrmStSetGainFrmCnt;
	//return; // MATT.
	if(ulFrameCnt % ubPeriod == ubFrmStSetShutFrmCnt) {
		ISP_IF_AE_Execute();
		gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, 0, 0);
    }
	else if(ulFrameCnt % ubPeriod == ubFrmStSetGainFrmCnt) {
	    gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, 0);
	}
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmEnd
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_DoAE_FrmEnd(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAWB
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_DoAWB(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoIQ
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_DoIQ(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetGain
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetGain(MMP_UBYTE ubSnrSel, MMP_ULONG ulGain)
{
    ISP_UINT16 s_gain, gainbase;
    ISP_UINT32 H, L;
	//return; // MATT.
    ulGain = ISP_IF_AE_GetGain();
    gainbase = ISP_IF_AE_GetGainBase();

    s_gain = ISP_MIN(ISP_MAX(ulGain, gainbase), gainbase*16-1); // API input gain range : 64~511, 64=1X
  		
    if (s_gain >= gainbase * 8) {
        H = 3;
        L = s_gain * 16 / gainbase / 8 - 16;         
    } else if (s_gain >= gainbase * 4) {
        H = 2;
        L = s_gain * 16 / gainbase / 4 - 16;         
    } else if (s_gain >= gainbase * 2) {
        H = 1;
        L = s_gain * 16 / gainbase / 2 - 16;        
    } else {
        H = 0;
        L = s_gain * 16 / gainbase / (H+1) - 16;
    }
    if (L > 15) L = 15;	   
  
    // set sensor gain
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x00, (H<<4)+L); //Total gain = (2^PGA[6:4])*(1+PGA[3:0]/16) //PGA:0x00
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetShutter
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG shutter, MMP_ULONG vsync)
{
    ISP_UINT32 new_vsync;
    ISP_UINT32 new_shutter;
	//return; // MATT.
    if(shutter == 0 || vsync == 0)
    {
        new_vsync = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetVsync() / ISP_IF_AE_GetVsyncBase();
        new_shutter = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter() / ISP_IF_AE_GetShutterBase();
    }
    else
    {
        new_vsync = gSnrLineCntPerSec[ubSnrSel] * vsync / ISP_IF_AE_GetVsyncBase();
        new_shutter = gSnrLineCntPerSec[ubSnrSel] * shutter / ISP_IF_AE_GetShutterBase();
    }

	new_vsync   = ISP_MIN(ISP_MAX(new_shutter + 5, new_vsync), 0xFFFF);
	new_shutter = ISP_MIN(ISP_MAX(new_shutter, 1), new_vsync - 5);
	
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x23, new_vsync >> 8);
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x22, new_vsync);
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x02, (ISP_UINT8)((new_shutter >> 8) & 0xff));
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x01, (ISP_UINT8)(new_shutter & 0xff));
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetFlip
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetFlip(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetRotate
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetRotate(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_CheckVersion
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_CheckVersion(MMP_UBYTE ubSnrSel, MMP_ULONG *pulVersion)
{
}

MMPF_SENSOR_CUSTOMER  SensorCustFunc = 
{
    SNR_Cust_InitConfig,
    SNR_Cust_DoAE_FrmSt,
    SNR_Cust_DoAE_FrmEnd,
    SNR_Cust_DoAWB,
    SNR_Cust_DoIQ,
    SNR_Cust_SetGain,
    SNR_Cust_SetShutter,
    SNR_Cust_SetFlip,
    SNR_Cust_SetRotate,
    SNR_Cust_CheckVersion,
    
    &m_SensorRes,
    &m_OprTable,
    &m_VifSetting,
    &m_I2cmAttr,
    &m_AwbTime,
    &m_AeTime,
    &m_AfTime
};

int SNR_Module_Init(void)
{
    MMPF_SensorDrv_Register(PRM_SENSOR, &SensorCustFunc);

    return 0;
}

/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall", zidata = "initcall" */
/* #pragma O0 */
ait_module_init(SNR_Module_Init);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

#endif  //BIND_SENSOR_JXF02
#endif	//SENSOR_EN
