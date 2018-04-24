//==============================================================================
//
//  File        : sensor_imx175.c
//  Description : Firmware Sensor Control File
//  Author      : Philip Lin
//  Revision    : 1.0
//
//=============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "Customer_config.h"

#if (SENSOR_EN)
#if (BIND_SENSOR_IMX326)
#include "mmpf_pll.h"
#include "mmpf_sensor.h"
#include "mmpf_i2cm.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"
#define MODE_0  0 // 1920x1920
#define MODE_1  1 // 1920x1080
#define MODE_2  2


#define SENSOR_RANGE_MODE MODE_0
#undef ISP_EN
#define ISP_EN 1

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================
#define FULL_SNR_W   3080
#define FULL_SNR_H   2168

#if SENSOR_RANGE_MODE==MODE_0
#define SNR_W   1920//2584
#define SNR_H   1920//1938
#define SNR_X   (( FULL_SNR_W - SNR_W ) >> 1)
#define SNR_Y   (( FULL_SNR_H - SNR_H ) >> 1)
#elif  SENSOR_RANGE_MODE==MODE_1
#define SNR_W   1920
#define SNR_H   1080
#elif  SENSOR_RANGE_MODE==MODE_2
#define SNR_W   1920 //1920
#define SNR_H   1352 //1080
#endif


MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	2,				// ubSensorModeNum
	0,				// ubDefPreviewMode
	0,				// ubDefCaptureMode
	1400,           // usPixelSize
//  Mode0   Mode1
#if SENSOR_RANGE_MODE==MODE_0
    {SNR_X,    1 },	// usVifGrabStX
    {SNR_Y,   1 },	// usVifGrabStY
    {SNR_W+8,  1156    },	// usVifGrabW
    {SNR_H+8,  652     },	// usVifGrabH
    #if (CHIP == MCR_V2)
    {1,     1   },  // usBayerInGrabX
    {1,     1   },  // usBayerInGrabY
    {8,     4	},  // usBayerInDummyX
    {8,    	4	},  // usBayerInDummyY
    {SNR_W/*1920*/,  1152},	// usBayerOutW
    {SNR_H/*1080*/,  648 },	// usBayerOutH
    #endif
#elif SENSOR_RANGE_MODE==MODE_1
    {8,   	1	    },	// usVifGrabStX
    {223,   1	    },	// usVifGrabStY
    {3080,  1156    },	// usVifGrabW
    {1736,  652     },	// usVifGrabH
    #if (CHIP == MCR_V2)
    {1,     1   },  // usBayerInGrabX
    {1,     1   },  // usBayerInGrabY
    {8,     4	},  // usBayerInDummyX
    {8,    	4	},  // usBayerInDummyY
    {SNR_W,  1152},	// usBayerOutW
    {SNR_H,  648	},	// usBayerOutH
    #endif
#elif  SENSOR_RANGE_MODE==MODE_2
    {8,   	1	    },	// usVifGrabStX
    {8,   1	        },	// usVifGrabStY
    {3080,  1156    },	// usVifGrabW
    {2168,  652     },	// usVifGrabH
    #if (CHIP == MCR_V2)
    {1,     1   },  // usBayerInGrabX
    {1,     1   },  // usBayerInGrabY
    {8,     4	},  // usBayerInDummyX
    {8,    	4	},  // usBayerInDummyY
    {SNR_W,  1152},	// usBayerOutW
    {SNR_H,  648	},	// usBayerOutH
    #endif
#endif
    
    {SNR_W/*1920*/,  800	},	// usScalInputW
    {SNR_H/*1080*/,  600	},	// usScalInputH
    {300,   600	},	// usTargetFpsx10
    {4464,  672	},	// usVsyncLine
    {1,		1	},  // ubWBinningN
    {1,		1	},  // ubWBinningM
    {1,		1	},  // ubHBinningN
    {1,		1	},  // ubHBinningM
    {0xFF,	0xFF},  // ubCustIQmode
    {0xFF,	0xFF}   // ubCustAEmode
};

// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE       m_OprTable;
MMPF_SENSOR_VIF_SETTING     m_VifSetting;

// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
#include "isp_8428_iq_data_v2_IMX326_IQ1.xls.ciq.txt"

};

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = {
    MMP_I2CM0,  // i2cmID
    0x1A,       // ubSlaveAddr
    16,         // ubRegLen
    8,          // ubDataLen
    0,          // ubDelayTime
    MMP_FALSE,  // bDelayWaitEn
    MMP_TRUE,   // bInputFilterEn
    MMP_FALSE,  // b10BitModeEn
    MMP_FALSE,  // bClkStretchEn
    0,          // ubSlaveAddr1
    0,          // ubDelayCycle
    0,          // ubPadNum
    400000,     // ulI2cmSpeed 250KHZ
    MMP_TRUE,   // bOsProtectEn
    0,       // sw_clk_pin
    0,       // sw_data_pin
    MMP_FALSE,  // bRfclModeEn
    MMP_FALSE,  // bWfclModeEn
    MMP_FALSE,	// bRepeatModeEn
	0           // ubVifPioMdlId
};

// 3A Timing
MMPF_SENSOR_AWBTIMIMG   m_AwbTime    = 
{
	2,	/* ubPeriod */
	0, 	/* ubDoAWBFrmCnt */
	2	/* ubDoCaliFrmCnt */
};

MMPF_SENSOR_AETIMIMG    m_AeTime     = 
{	
	2, 	/* ubPeriod */
	0, 	/* ubFrmStSetShutFrmCnt */
	0	/* ubFrmStSetGainFrmCnt */
};

MMPF_SENSOR_AFTIMIMG    m_AfTime     = 
{
	1, 	/* ubPeriod */
	0	/* ubDoAFFrmCnt */
};

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Sensor_Init_OPR_Table____(){ruturn;} //dummy
#endif

#if (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)

MMP_USHORT SNR_IMX326_Reg_Init_Customer[] = 
{
	SENSOR_DELAY_REG, 100
};

#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)

MMP_USHORT SNR_IMX326_Reg_Init_Customer[] = 
{
    0x3000, 0x12,
    0x3120, 0xF0,
    0x3121, 0x00,
    0x3122, 0x02,
    0x3123, 0x01,
    0x3129, 0x9C,
    0x312A, 0x02,
    0x312D, 0x02,
    0x3AC4, 0x01, //?
    0x310B, 0x00,
    0x304C, 0x00,
    0x304D, 0x03,
    0x331C, 0x1A,
    0x331D, 0x00,
    0x3502, 0x02,
    0x3529, 0x0E,
    0x352A, 0x0E,
    0x352B, 0x0E,
    0x3538, 0x0E,
    0x3539, 0x0E,
    0x3553, 0x00,
    0x357D, 0x05,
    0x357F, 0x05,
    0x3581, 0x04,
    0x3583, 0x76,
    0x3587, 0x01,
    0x35BB, 0x0E,
    0x35BC, 0x0E,
    0x35BD, 0x0E,
    0x35BE, 0x0E,
    0x35BF, 0x0E,
    0x366E, 0x00,
    0x366F, 0x00,
    0x3670, 0x00,
    0x3671, 0x00,
    0x30EE, 0x01,
    0x3304, 0x32,
    0x3305, 0x00,
    0x3306, 0x32,
    0x3307, 0x00,
    0x3590, 0x32,
    0x3591, 0x00,
    0x3686, 0x32,
    0x3687, 0x00,
    0x3134, 0x77,
    0x3135, 0x00,
    0x3136, 0x67,
    0x3137, 0x00,
    0x3138, 0x37,
    0x3139, 0x00,
    0x313A, 0x37,
    0x313B, 0x00,
    0x313C, 0x37,
    0x313D, 0x00,
    0x313E, 0xDF,
    0x313F, 0x00,
    0x3140, 0x37,
    0x3141, 0x00,
    0x3142, 0x2F,
    0x3143, 0x00,
    0x3144, 0x0F,
    0x3145, 0x00,
    0x3A86, 0x47,
    0x3A87, 0x00,
    SENSOR_DELAY_REG,10,
    0x3000, 0x00,
    0x303E, 0x02,
    SENSOR_DELAY_REG,7,
    0x30F4, 0x00,
    0x3018, 0xA2,
};
#endif

#if 0
void ____Sensor_Res_OPR_Table____(){ruturn;} //dummy
#endif

#if (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)

#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)		

ISP_UINT16 SNR_IMX326_Reg_3072x2160[] = 
{
   /*mode 1 3072x2160*/
    0x3004, 0x01,
    0x3005, 0x01,
    0x3006, 0x00,
    0x3007, 0x02,
    
    #if 1 
    0x3037, 0x01,
    0x3038, 0x80,
    0x3039, 0x01,
    0x303A, 0x98,
    0x303B, 0x0D,

    0x306B, 0x05,
    0x30DD, 0x00,
    0x30DE, 0x00,
    0x30DF, 0x00,

    0x30E0, 0x00,
    0x30E1, 0x00,
    #endif

    0x30E2, 0x01,
    0x30EE, 0x01,
    
    0x30F6, 0x1A,
    0x30F7, 0x02,
    0x30F8, 0x70,
    0x30F9, 0x11,
    0x30FA, 0x00,
    0x3130, 0x86,
    0x3131, 0x08,
    0x3132, 0x7E,
    0x3133, 0x08,

    0x3342, 0x0A,
    0x3343, 0x00,
    0x3344, 0x16,
    0x3345, 0x00,
    0x33A6, 0x01,
    0x3528, 0x0E,
    0x3554, 0x1F,
    0x3555, 0x01,
    0x3556, 0x01,
    0x3557, 0x01,
    0x3558, 0x01,
    0x3559, 0x00,
    0x355A, 0x00,
    0x35BA, 0x0E,
    0x366A, 0x1B,
    0x366B, 0x1A,
    0x366C, 0x19,
    0x366D, 0x17,
    
    0x3A41, 0x08,
   
};

#endif

#if 0
void ____Sensor_Customer_Func____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_InitConfig
//  Description :
//------------------------------------------------------------------------------
static void SNR_Cust_InitConfig(void)
{
    // Init OPR Table
    SensorCustFunc.OprTable->usInitSize                   		= (sizeof(SNR_IMX326_Reg_Init_Customer)/sizeof(SNR_IMX326_Reg_Init_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                 		= &SNR_IMX326_Reg_Init_Customer[0];    

    SensorCustFunc.OprTable->bBinTableExist                     = MMP_FALSE; 
    SensorCustFunc.OprTable->bInitDoneTableExist                = MMP_FALSE;

    SensorCustFunc.OprTable->usSize[RES_IDX_3072x2160_30FPS]    = (sizeof(SNR_IMX326_Reg_3072x2160)/sizeof(SNR_IMX326_Reg_3072x2160[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_3072x2160_30FPS]  = &SNR_IMX326_Reg_3072x2160[0];
    

    // Init Vif Setting : Common
    SensorCustFunc.VifSetting->SnrType                              = MMPF_VIF_SNR_TYPE_BAYER;
    SensorCustFunc.VifSetting->OutInterface                         = MMPF_VIF_IF_MIPI_QUAD;
    SensorCustFunc.VifSetting->VifPadId							    = MMPF_VIF_MDL_ID0;

    // Init Vif Setting : PowerOn Setting
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnExtPower       = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin         = SENSOR_GPIO_ENABLE; // it might be defined in Config_SDK.h
    SensorCustFunc.VifSetting->powerOnSetting.bExtPowerPinHigh	    = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPinDelay    = 50;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh      	= MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay    	= 1;
    SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh        = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay    	= 10;
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst	= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh      = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay    = 10;
    SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh       = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay     = 10;

    // Init Vif Setting : PowerOff Setting
    SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode    = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg     = 0x301A;
    SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask    = 0x04;
    SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh           = MMP_FALSE;
    SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay         = 20;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock       = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower     = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin        = SENSOR_GPIO_ENABLE; // it might be defined in Config_SDK.h

    // Init Vif Setting : Sensor MClock Setting
    SensorCustFunc.VifSetting->clockAttr.bClkOutEn                  = MMP_TRUE; 
    SensorCustFunc.VifSetting->clockAttr.ubClkFreqDiv               = 0;
    SensorCustFunc.VifSetting->clockAttr.ulMClkFreq                 = 24000;
    SensorCustFunc.VifSetting->clockAttr.ulDesiredFreq              = 24000;
    SensorCustFunc.VifSetting->clockAttr.ubClkPhase                 = MMPF_VIF_SNR_PHASE_DELAY_NONE;
    SensorCustFunc.VifSetting->clockAttr.ubClkPolarity              = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->clockAttr.ubClkSrc                   = MMPF_VIF_SNR_CLK_SRC_PMCLK; 

    // Init Vif Setting : Parallel Sensor Setting
    SensorCustFunc.VifSetting->paralAttr.ubLatchTiming              = MMPF_VIF_SNR_LATCH_POS_EDGE;
    SensorCustFunc.VifSetting->paralAttr.ubHsyncPolarity            = MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->paralAttr.ubVsyncPolarity            = MMPF_VIF_SNR_CLK_POLARITY_NEG;
    SensorCustFunc.VifSetting->paralAttr.ubBusBitMode               = MMPF_VIF_SNR_PARAL_BITMODE_16;
    
    // Init Vif Setting : MIPI Sensor Setting
    SensorCustFunc.VifSetting->mipiAttr.bClkDelayEn                 = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bClkLaneSwapEn              = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.usClkDelay                  = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubBClkLatchTiming           = MMPF_VIF_SNR_LATCH_NEG_EDGE;
#if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]            = MMPF_VIF_MIPI_DATA_SRC_PHY_0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]            = MMPF_VIF_MIPI_DATA_SRC_PHY_1;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]            = MMPF_VIF_MIPI_DATA_SRC_PHY_2;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]            = MMPF_VIF_MIPI_DATA_SRC_PHY_3;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]             = 0x1F;
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1]              = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2]              = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3]              = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1]             = MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2]             = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3]             = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3]          = MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]            = MMPF_VIF_MIPI_DATA_SRC_PHY_1;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]            = MMPF_VIF_MIPI_DATA_SRC_PHY_2;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]             	= 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]             = 0x1F;
#endif

    // Init Vif Setting : Color ID Setting
    SensorCustFunc.VifSetting->colorId.VifColorId              		= MMPF_VIF_COLORID_10;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId  	= MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
static	ISP_UINT32 isp_dgain,dGainBase=0x200,s_gain;
void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
#if (ISP_EN) // TBD

	ISP_UINT32 Lux = ISP_IF_AE_GetLightCond();

    switch((ulFrameCnt % m_AeTime.ubPeriod) ) {
    case 0:
        ISP_IF_AE_Execute();	
        s_gain = ISP_MAX(ISP_IF_AE_GetGain(), ISP_IF_AE_GetGainBase());	
        if(s_gain >= ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN){
            isp_dgain = dGainBase * s_gain/ (ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN);
            s_gain = ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN;
        }
        else{
            isp_dgain = dGainBase;
        }

	    gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel,0, 0);
        break;
    case 1:
        gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel,s_gain);      
    	break;
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
#if (ISP_EN)
    ISP_UINT32 gain;
	gain	 =  0x800 - 0x800 * 0x100 / ulGain;  //reg = 2048 - (2048/Gain) 
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x300A, (gain >> 0 ) & 0xff);
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x300B, (gain >> 8 ) & 0x07);
//    printc("s_gain : %x\r\n",gain);
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetShutter
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG shutter, MMP_ULONG vsync)
{
    ISP_UINT32 new_vsync    = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetVsync() / ISP_IF_AE_GetVsyncBase();
    ISP_UINT32 new_shutter  = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter() / ISP_IF_AE_GetShutterBase();
    // 1 ~ (frame length line - 6)
    if(vsync) {
        new_vsync = vsync ;
    }
    else {
        new_vsync = VR_MIN(VR_MAX(new_shutter , new_vsync), 0xFFFF);
    }
    if(shutter) {
        new_shutter = shutter ;
    }
    else {
        new_shutter = VR_MIN(VR_MAX(new_shutter, 1), new_vsync - 12);
    }
#if (ISP_EN)
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x300C, (((new_vsync - new_shutter) >> 0) & 0xFF));
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x300D, (((new_vsync - new_shutter) >> 8) & 0xFF));
    //Vsync
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x30F8, (new_vsync >> 0 ) & 0xFF);
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x30F9, (new_vsync >> 8 ) & 0xFF);
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x30FA, (new_vsync >> 16)& 0x0F);
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetFlip
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetFlip(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetRotate
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetRotate(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_CheckVersion
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_CheckVersion(MMP_UBYTE ubSnrSel, MMP_ULONG *pulVersion)
{
    // TBD
}
void SNR_Cust_Switch3ASpeed(MMP_BOOL slow);
MMPF_SENSOR_CUSTOMER SensorCustFunc = 
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
    SNR_Cust_Switch3ASpeed,
    &m_SensorRes,
    &m_OprTable,
    &m_VifSetting,
    &m_I2cmAttr,
    &m_AwbTime,
    &m_AeTime,
    &m_AfTime,
    MMP_SNR_PRIO_PRM
};

void SNR_Cust_Switch3ASpeed(MMP_BOOL slow)
{
  MMPF_SENSOR_CUSTOMER *cust = (MMPF_SENSOR_CUSTOMER *)&SensorCustFunc ;
  printc("[TBD] : Switch to slow 3A\r\n");
  if(slow) {

  }
  else {

  }
}

int SNR_Module_Init(void)
{
    if (SensorCustFunc.sPriority == MMP_SNR_PRIO_PRM)
        MMPF_SensorDrv_Register(PRM_SENSOR, &SensorCustFunc);
    else
        MMPF_SensorDrv_Register(SCD_SENSOR, &SensorCustFunc); 
        
    return 0;
}

/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall", zidata = "initcall" */
/* #pragma O0 */
ait_module_init(SNR_Module_Init);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

#endif // (BIND_SENSOR_IMX175)
#endif // (SENSOR_EN)
