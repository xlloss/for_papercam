//==============================================================================
//
//  File        : sensor_ar0330_OTPM.c
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
#if (BIND_SENSOR_AR0330_OTPM)

#include "mmpf_sensor.h"
#include "mmpf_i2cm.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

static MMP_UBYTE gOTPM_Ver = 0xFF;

MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	20,				/* ubSensorModeNum */
	0,				/* ubDefPreviewMode */
	0,				/* ubDefCaptureMode */
    2000,           /* usPixelSize      */

//  Mode0   	Mode1		Mode2   	Mode3   	Mode4(X)	Mode5   	Mode6		Mode7   	Mode8   	Mode9(X)	Mode10		Mode11		Mode12		Mode13		Mode14 		Mode15(Max)	Mode16(1)	Mode17(6)	Mode18		Mode19
//  1080@30(W)	720@60(W)   720@30(W)   WVGA@30(W) 	WVGA@120(W)	1080@30(S)	720@60(S)   720@30(S)   WVGA@30(S) 	WVGA@120(S) 2M@30(W)P   2M@15(W)C   2M@30(S)P   2M@15(S)C   3M@30C		2304x1536   1080@60(W)  1080@60(S)  720@60(W,SM)720@60(S,SM)
    {1,			1,			1,			1,			1,			161,		173,		161,		173,		84,			1,			1,			1,			1,			64,			1,			1,			1,			1,			173,		},	/* usVifGrabStX */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			64,			1,			1,			1,			1,			1,			},	/* usVifGrabStY */    
    {2266,		2266,		2266,		2266,		1132,		1940,		1928,		1940,		1928,		964,		2184,		2184,		1928,		1928,		1768,		2304,		2266,       1928,       2266,       1928		},	/* usVifGrabW */
    {1280,		1280,		1280,		1280,		640,		1096,		1096,		1096,		1096,		544,		1232,		1232,		1088,		1088,		1332,		1536,		1280,		1096,		1280,		1096		},	/* usVifGrabH */
 
#if (CHIP == MCR_V2)    
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* usBayerInGrabX */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* usBayerInGrabY */
    {10, 		10,			10,			18,			12,			4, 			8,			4,			8,			18,			8,			8,			8,			8,			8,			2,			10,			8,			10,			8			},  /* usBayerDummyInX */ 
    {11,		11,			11,			8,			6,			4,			16,			4,			8,			8,			8,			8,			8,			8,			4,			2,			11,			16,			11,			16			},  /* usBayerDummyInY */
    {1920,  	1920,		1920,		1696,		1120,		1936,  		1920,		1936,		1920,		946,		2176,		2176,		1920,		1920,		1760,		2304-2,		1920,		1920,		1280,		1280		},	/* usBayerOutW */
    {1080,  	1080,		1080,		960,		634,		1092,  		1080,		1092,		1088,		536,		1224,		1224,		1080,		1080,		1328,		1536-2,		1080,		1080,		720,		720			},	/* usBayerOutH */
#endif

    {1920,  	1920,		1920,		1696,		1120,		1920,  		1920,		1920,		1920,		946,		2176,		2176,		1920,		1920,		1760,		2304-2,		1920,		1920,		1280,		1280		},	/* usScalInputW */
    {1080,  	1080,		1080,		960,		634,		1080,  		1080,		1080,		1088,		536,		1224,		1224,		1080,		1080,		1328,		1536-2,		1080,		1080,		720,		720			},	/* usScalInputH */

    {300,   	600,		300,		300,		1200,		300,   		600,		300,		300,		1200,		300,		150,		300,		150,		300,		100,		600,		600,		600,		600			},	/* usTargetFpsx10 */  
    {2634,		1321,		2634,		2634,		666,		2630,		1321,		2630,		2630,		666,		3538,		3538,		3538,		3538,		1548,		1248,		1320,		1321,		1320,		1321		},	/* usVsyncLine */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* ubWBinningN */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* ubWBinningN */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* ubWBinningN */
    {1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1,			1			},  /* ubWBinningN */
    {0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF		},  /* ubWBinningN */
    {0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF,		0xFF		},  /* ubWBinningN */
    
};

// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE       m_OprTable;
MMPF_SENSOR_VIF_SETTING     m_VifSetting;

// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
    #include "isp_8428_iq_data_v2_AR0330_MP.xls.ciq.txt"
};

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = 
{
    MMP_I2CM0,  // i2cmID
    #ifdef SENSOR_I2C_ADDR
    SENSOR_I2C_ADDR,
    #else
    0x10,       // ubSlaveAddr
    #endif
    16,         // ubRegLen
    16,         // ubDataLen
    0,          // ubDelayTime
    MMP_FALSE,  // bDelayWaitEn
    MMP_TRUE,   // bInputFilterEn
    MMP_FALSE,  // b10BitModeEn
    MMP_FALSE,  // bClkStretchEn
    0,          // ubSlaveAddr1
    0,          // ubDelayCycle
    0,          // ubPadNum
    350000,     // ulI2cmSpeed 300KHZ
    MMP_TRUE,   // bOsProtectEn
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

MMP_USHORT SNR_AR0330_Reg_Init_OTPM_V1_Customer[] = 
{
	//Table 2. Recommend default register and Sequencer
	0x30BA, 0x2C  , 								   
	0x30FE, 0x0080, 								   
	0x31E0, 0x0003, 								   
	0x3ECE, 0xFF  , 								   
	0x3ED0, 0xE4F6, 								   
	0x3ED2, 0x0146, 								   
	0x3ED4, 0x8F6C, 								   
	0x3ED6, 0x66CC, 								   
	0x3ED8, 0x8C42, 								   
	0x3EDA, 0x8822, 								   
	0x3EDC, 0x2222, 								   
	0x3EDE, 0x22C0, 								   
	0x3EE0, 0x1500, 								   
	0x3EE6, 0x0080, 								   
	0x3EE8, 0x2027, 								   
	0x3EEA, 0x001D, 								   
	0x3F06, 0x046A, 								   
	0x305E, 0x00A0, 								   
													   
	//Sequencer A	
	0x301A, 0x0058,     // Disable streaming
	SNR_REG_DELAY, 10,
	
	0x3088, 0x8000,
	0x3086, 0x4540,
	0x3086, 0x6134,
	0x3086, 0x4A31,
	0x3086, 0x4342,
	0x3086, 0x4560,
	0x3086, 0x2714,
	0x3086, 0x3DFF,
	0x3086, 0x3DFF,
	0x3086, 0x3DEA,
	0x3086, 0x2704,
	0x3086, 0x3D10,
	0x3086, 0x2705,
	0x3086, 0x3D10,
	0x3086, 0x2715,
	0x3086, 0x3527,
	0x3086, 0x053D,
	0x3086, 0x1045,
	0x3086, 0x4027,
	0x3086, 0x0427,
	0x3086, 0x143D,
	0x3086, 0xFF3D,
	0x3086, 0xFF3D,
	0x3086, 0xEA62,
	0x3086, 0x2728,
	0x3086, 0x3627,
	0x3086, 0x083D,
	0x3086, 0x6444,
	0x3086, 0x2C2C,
	0x3086, 0x2C2C,
	0x3086, 0x4B01,
	0x3086, 0x432D,
	0x3086, 0x4643,
	0x3086, 0x1647,
	0x3086, 0x435F,
	0x3086, 0x4F50,
	0x3086, 0x2604,
	0x3086, 0x2684,
	0x3086, 0x2027,
	0x3086, 0xFC53,
	0x3086, 0x0D5C,
	0x3086, 0x0D60,
	0x3086, 0x5754,
	0x3086, 0x1709,
	0x3086, 0x5556,
	0x3086, 0x4917,
	0x3086, 0x145C,
	0x3086, 0x0945,
	0x3086, 0x0045,
	0x3086, 0x8026,
	0x3086, 0xA627,
	0x3086, 0xF817,
	0x3086, 0x0227,
	0x3086, 0xFA5C,
	0x3086, 0x0B5F,
	0x3086, 0x5307,
	0x3086, 0x5302,
	0x3086, 0x4D28,
	0x3086, 0x6C4C,
	0x3086, 0x0928,
	0x3086, 0x2C28,
	0x3086, 0x294E,
	0x3086, 0x1718,
	0x3086, 0x26A2,
	0x3086, 0x5C03,
	0x3086, 0x1744,
	0x3086, 0x2809,
	0x3086, 0x27F2,
	0x3086, 0x1714,
	0x3086, 0x2808,
	0x3086, 0x164D,
	0x3086, 0x1A26,
	0x3086, 0x8317,
	0x3086, 0x0145,
	0x3086, 0xA017,
	0x3086, 0x0727,
	0x3086, 0xF317,
	0x3086, 0x2945,
	0x3086, 0x8017,
	0x3086, 0x0827,
	0x3086, 0xF217,
	0x3086, 0x285D,
	0x3086, 0x27FA,
	0x3086, 0x170E,
	0x3086, 0x2681,
	0x3086, 0x5300,
	0x3086, 0x17E6,
	0x3086, 0x5302,
	0x3086, 0x1710,
	0x3086, 0x2683,
	0x3086, 0x2682,
	0x3086, 0x4827,
	0x3086, 0xF24D,
	0x3086, 0x4E28,
	0x3086, 0x094C,
	0x3086, 0x0B17,
	0x3086, 0x6D28,
	0x3086, 0x0817,
	0x3086, 0x014D,
	0x3086, 0x1A17,
	0x3086, 0x0126,
	0x3086, 0x035C,
	0x3086, 0x0045,
	0x3086, 0x4027,
	0x3086, 0x9017,
	0x3086, 0x2A4A,
	0x3086, 0x0A43,
	0x3086, 0x160B,
	0x3086, 0x4327,
	0x3086, 0x9445,
	0x3086, 0x6017,
	0x3086, 0x0727,
	0x3086, 0x9517,
	0x3086, 0x2545,
	0x3086, 0x4017,
	0x3086, 0x0827,
	0x3086, 0x905D,
	0x3086, 0x2808,
	0x3086, 0x530D,
	0x3086, 0x2645,
	0x3086, 0x5C01,
	0x3086, 0x2798,
	0x3086, 0x4B12,
	0x3086, 0x4452,
	0x3086, 0x5117,
	0x3086, 0x0260,
	0x3086, 0x184A,
	0x3086, 0x0343,
	0x3086, 0x1604,
	0x3086, 0x4316,
	0x3086, 0x5843,
	0x3086, 0x1659,
	0x3086, 0x4316,
	0x3086, 0x5A43,
	0x3086, 0x165B,
	0x3086, 0x4327,
	0x3086, 0x9C45,
	0x3086, 0x6017,
	0x3086, 0x0727,
	0x3086, 0x9D17,
	0x3086, 0x2545,
	0x3086, 0x4017,
	0x3086, 0x1027,
	0x3086, 0x9817,
	0x3086, 0x2022,
	0x3086, 0x4B12,
	0x3086, 0x442C,
	0x3086, 0x2C2C,
	0x3086, 0x2C00,
									   
	0x301A, 0x0004, 								   
													   
	//Initialization								   
	0x301A, 	0x0059, 								 
	SNR_REG_DELAY, 10,								 
	0x3052, 0xA114, // fix low temp OTPM wrong issue   
	0x304A, 0x0070, // fix low temp OTPM wrong issue   
	SNR_REG_DELAY, 10,								 

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x31AE, 	0x0204,
    #elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x31AE, 	0x0202,
    #endif

	0x301A, 	0x0058, 								 
	SNR_REG_DELAY, 35,								 
	0x3064, 	0x1802, 								 
	0x3078, 	0x0001, 								 
	0x31E0, 	0x0203, 								 
														 
	//configure PLL
	0x302A, 5	  , // VT_PIX_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x302C, 1	  , // VT_SYS_CLK_DIV
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x302C, 2	  , // VT_SYS_CLK_DIV
    #endif

	0x302E, 2	  , // PRE_PLL_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x3030, 41	  , // PLL_MULTIPLIER
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x3030, 82	  , // PLL_MULTIPLIER
    #endif    

	0x3036, 10	  , // OP_PIX_CLK_DIV
	0x3038, 1	  , // OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, // DATA_FORMAT_BITS
	0x31B0, 47	  , 								   
	0x31B2, 19	  , 								   
	0x31B4, 0x3C44, 								   
	0x31B6, 0x314D, 								   
	0x31B8, 0x208A, 								   
	0x31BA, 0x0207, 								   
	0x31BC, 0x8005, 								   
	0x31BE, 0x2003, 								   
};

MMP_USHORT SNR_AR0330_Reg_Init_OTPM_V2_Customer[] = 
{
	//Table 2. Recommend default register and Sequencer
	0x30BA, 0x2C  , 								   
	0x30FE, 0x0080, 								   
	0x31E0, 0x0003, 								   
	0x3ECE, 0xFF  , 								   
	0x3ED0, 0xE4F6, 								   
	0x3ED2, 0x0146, 								   
	0x3ED4, 0x8F6C, 								   
	0x3ED6, 0x66CC, 								   
	0x3ED8, 0x8C42, 								   
	0x3EDA, 0x889B, 								   
	0x3EDC, 0x8863, 								   
	0x3EDE, 0xAA04, 								   
	0x3EE0, 0x15F0, 								   
	0x3EE6, 0x008C, 								   
	0x3EE8, 0x2024, 								   
	0x3EEA, 0xFF1F, 								   
	0x3F06, 0x046A, 								   
	0x305E, 0x00A0, 								   
													   
	//Sequencer B	
	0x301A, 0x0058,     // Disable streaming
	SNR_REG_DELAY, 2,
	0x3088, 0x8000, 								   
	0x3086, 0x4A03, 								   
	0x3086, 0x4316, 								   
	0x3086, 0x0443, 								   
	0x3086, 0x1645, 								   
	0x3086, 0x4045, 								   
	0x3086, 0x6017, 								   
	0x3086, 0x2045, 								   
	0x3086, 0x404B, 								   
	0x3086, 0x1244, 								   
	0x3086, 0x6134, 								   
	0x3086, 0x4A31, 								   
	0x3086, 0x4342, 								   
	0x3086, 0x4560, 								   
	0x3086, 0x2714, 								   
	0x3086, 0x3DFF, 								   
	0x3086, 0x3DFF, 								   
	0x3086, 0x3DEA, 								   
	0x3086, 0x2704, 								   
	0x3086, 0x3D10, 								   
	0x3086, 0x2705, 								   
	0x3086, 0x3D10, 								   
	0x3086, 0x2715, 								   
	0x3086, 0x3527, 								   
	0x3086, 0x053D, 								   
	0x3086, 0x1045, 								   
	0x3086, 0x4027, 								   
	0x3086, 0x0427, 								   
	0x3086, 0x143D, 								   
	0x3086, 0xFF3D, 								   
	0x3086, 0xFF3D, 								   
	0x3086, 0xEA62, 								   
	0x3086, 0x2728, 								   
	0x3086, 0x3627, 								   
	0x3086, 0x083D, 								   
	0x3086, 0x6444, 								   
	0x3086, 0x2C2C, 								   
	0x3086, 0x2C2C, 								   
	0x3086, 0x4B01, 								   
	0x3086, 0x432D, 								   
	0x3086, 0x4643, 								   
	0x3086, 0x1647, 								   
	0x3086, 0x435F, 								   
	0x3086, 0x4F50, 								   
	0x3086, 0x2604, 								   
	0x3086, 0x2684, 								   
	0x3086, 0x2027, 								   
	0x3086, 0xFC53, 								   
	0x3086, 0x0D5C, 								   
	0x3086, 0x0D57, 								   
	0x3086, 0x5417, 								   
	0x3086, 0x0955, 								   
	0x3086, 0x5649, 								   
	0x3086, 0x5307, 								   
	0x3086, 0x5302, 								   
	0x3086, 0x4D28, 								   
	0x3086, 0x6C4C, 								   
	0x3086, 0x0928, 								   
	0x3086, 0x2C28, 								   
	0x3086, 0x294E, 								   
	0x3086, 0x5C09, 								   
	0x3086, 0x6045, 								   
	0x3086, 0x0045, 								   
	0x3086, 0x8026, 								   
	0x3086, 0xA627, 								   
	0x3086, 0xF817, 								   
	0x3086, 0x0227, 								   
	0x3086, 0xFA5C, 								   
	0x3086, 0x0B17, 								   
	0x3086, 0x1826, 								   
	0x3086, 0xA25C, 								   
	0x3086, 0x0317, 								   
	0x3086, 0x4427, 								   
	0x3086, 0xF25F, 								   
	0x3086, 0x2809, 								   
	0x3086, 0x1714, 								   
	0x3086, 0x2808, 								   
	0x3086, 0x1701, 								   
	0x3086, 0x4D1A, 								   
	0x3086, 0x2683, 								   
	0x3086, 0x1701, 								   
	0x3086, 0x27FA, 								   
	0x3086, 0x45A0, 								   
	0x3086, 0x1707, 								   
	0x3086, 0x27FB, 								   
	0x3086, 0x1729, 								   
	0x3086, 0x4580, 								   
	0x3086, 0x1708, 								   
	0x3086, 0x27FA, 								   
	0x3086, 0x1728, 								   
	0x3086, 0x5D17, 								   
	0x3086, 0x0E26, 								   
	0x3086, 0x8153, 								   
	0x3086, 0x0117, 								   
	0x3086, 0xE653, 								   
	0x3086, 0x0217, 								   
	0x3086, 0x1026, 								   
	0x3086, 0x8326, 								   
	0x3086, 0x8248, 								   
	0x3086, 0x4D4E, 								   
	0x3086, 0x2809, 								   
	0x3086, 0x4C0B, 								   
	0x3086, 0x6017, 								   
	0x3086, 0x2027, 								   
	0x3086, 0xF217, 								   
	0x3086, 0x535F, 								   
	0x3086, 0x2808, 								   
	0x3086, 0x164D, 								   
	0x3086, 0x1A17, 								   
	0x3086, 0x0127, 								   
	0x3086, 0xFA26, 								   
	0x3086, 0x035C, 								   
	0x3086, 0x0145, 								   
	0x3086, 0x4027, 								   
	0x3086, 0x9817, 								   
	0x3086, 0x2A4A, 								   
	0x3086, 0x0A43, 								   
	0x3086, 0x160B, 								   
	0x3086, 0x4327, 								   
	0x3086, 0x9C45, 								   
	0x3086, 0x6017, 								   
	0x3086, 0x0727, 								   
	0x3086, 0x9D17, 								   
	0x3086, 0x2545, 								   
	0x3086, 0x4017, 								   
	0x3086, 0x0827, 								   
	0x3086, 0x985D, 								   
	0x3086, 0x2645, 								   
	0x3086, 0x5C01, 								   
	0x3086, 0x4B17, 								   
	0x3086, 0x0A28, 								   
	0x3086, 0x0853, 								   
	0x3086, 0x0D52, 								   
	0x3086, 0x5112, 								   
	0x3086, 0x4460, 								   
	0x3086, 0x184A, 								   
	0x3086, 0x0343, 								   
	0x3086, 0x1604, 								   
	0x3086, 0x4316, 								   
	0x3086, 0x5843, 								   
	0x3086, 0x1659, 								   
	0x3086, 0x4316, 								   
	0x3086, 0x5A43, 								   
	0x3086, 0x165B, 								   
	0x3086, 0x4345, 								   
	0x3086, 0x4027, 								   
	0x3086, 0x9C45, 								   
	0x3086, 0x6017, 								   
	0x3086, 0x0727, 								   
	0x3086, 0x9D17, 								   
	0x3086, 0x2545, 								   
	0x3086, 0x4017, 								   
	0x3086, 0x1027, 								   
	0x3086, 0x9817, 								   
	0x3086, 0x2022, 								   
	0x3086, 0x4B12, 								   
	0x3086, 0x442C, 								   
	0x3086, 0x2C2C, 								   
	0x3086, 0x2C00, 								   
	0x3086, 0x0000, 								   
	0x301A, 0x0004, 								   
													   
	//Initialization								   
	0x301A, 	0x0059, 								 
	SNR_REG_DELAY, 2,								 
	0x3052, 0xA114, // fix low temp OTPM wrong issue   
	0x304A, 0x0070, // fix low temp OTPM wrong issue   
	SNR_REG_DELAY, 2,								 

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x31AE, 	0x0204,
    #elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x31AE, 	0x0202,
    #endif

	0x301A, 	0x0058, 								 
	SNR_REG_DELAY, 2,								 
	0x3064, 	0x1802, 								 
	0x3078, 	0x0001, 								 
	0x31E0, 	0x0203, 								 
														 
	//configure PLL
	0x302A, 5	  , // VT_PIX_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x302C, 1	  , // VT_SYS_CLK_DIV
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x302C, 2	  , // VT_SYS_CLK_DIV
    #endif

	0x302E, 2	  , // PRE_PLL_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x3030, 41	  , // PLL_MULTIPLIER
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x3030, 82	  , // PLL_MULTIPLIER
    #endif

	0x3036, 10	  , // OP_PIX_CLK_DIV
	0x3038, 1	  , // OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, // DATA_FORMAT_BITS
	0x31B0, 47	  ,
	0x31B2, 19	  ,
	0x31B4, 0x3C44,
	0x31B6, 0x314D,
	0x31B8, 0x208A,
	0x31BA, 0x0207,
	0x31BC, 0x8005,
	0x31BE, 0x2003,
};


MMP_USHORT SNR_AR0330_Reg_Init_OTPM_V3_Customer[] = 
{
	//Table 2. Recommend default register and Sequencer
	0x31E0, 0x0003,
	0x3ED2, 0x0146,
	0x3ED4, 0x8F6C,
	0x3ED6, 0x66CC,
    0x3ED8, 0x8C42,
	0x3EDA, 0x88BC,
	0x3EDC, 0xAA63,
	0x305E, 0x00A0,
	0x3046, 0x4038,
	0x3048, 0x8480,
	
	//Sequencer Patch 1
	0x301A, 0x0058,     // Disable streaming
	SNR_REG_DELAY, 10,		
	0x3088, 0x800C,
    0x3086, 0x2045,
    0x301A, 0x0004, 
	
	//Initialization								   
	0x301A, 	0x0059, 								 
	SNR_REG_DELAY, 10,								 
	0x3052, 0xA114, // fix low temp OTPM wrong issue   
	0x304A, 0x0070, // fix low temp OTPM wrong issue   
	SNR_REG_DELAY, 10,								 

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x31AE, 	0x0204,
    #elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x31AE, 	0x0202,
    #endif

	0x301A, 	0x0058,
	SNR_REG_DELAY, 35,
	0x3064, 	0x1802,
	0x3078, 	0x0001,
	0x31E0, 	0x0203,

	//configure PLL
	0x302A, 5	  , // VT_PIX_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x302C, 1	  , // VT_SYS_CLK_DIV
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x302C, 2	  , // VT_SYS_CLK_DIV
    #endif

	0x302E, 2	  , // PRE_PLL_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x3030, 41	  , // PLL_MULTIPLIER
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x3030, 82	  , // PLL_MULTIPLIER
    #endif

	0x3036, 10	  , // OP_PIX_CLK_DIV
	0x3038, 1	  , // OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, // DATA_FORMAT_BITS
	0x31B0, 47	  ,
	0x31B2, 19	  ,
	0x31B4, 0x3C44,
	0x31B6, 0x314D,
	0x31B8, 0x208A,
	0x31BA, 0x0207,
	0x31BC, 0x8005,
	0x31BE, 0x2003,
};


MMP_USHORT SNR_AR0330_Reg_Init_OTPM_V4_Customer[] = 
{
	//Table 2. Recommend default register and Sequencer
	0x31E0, 0x0003,
	0x3ED2, 0x0146,
	0x3ED6, 0x66CC,
    0x3ED8, 0x8C42,
	0x3EDA, 0x88BC,
	0x3EDC, 0xAA63,
	0x305E, 0x00A0,
	0x3046, 0x4038,
	0x3048, 0x8480,
	
	//Initialization								   
	0x301A, 	0x0059, 								 
	SNR_REG_DELAY, 10,								 
	0x3052, 0xA114, // fix low temp OTPM wrong issue   
	0x304A, 0x0070, // fix low temp OTPM wrong issue   
	SNR_REG_DELAY, 10,								 

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x31AE, 	0x0204,
    #elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x31AE, 	0x0202,
    #endif

	0x301A, 	0x0058,
	SNR_REG_DELAY, 35,
	0x3064, 	0x1802,
	0x3078, 	0x0001,
	0x31E0, 	0x0203,

	//configure PLL
	0x302A, 5	  , // VT_PIX_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x302C, 1	  , // VT_SYS_CLK_DIV
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x302C, 2	  , // VT_SYS_CLK_DIV
    #endif

	0x302E, 2	  , // PRE_PLL_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x3030, 41	  , // PLL_MULTIPLIER
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x3030, 82	  , // PLL_MULTIPLIER
    #endif

	0x3036, 10	  , // OP_PIX_CLK_DIV
	0x3038, 1	  , // OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, // DATA_FORMAT_BITS
	0x31B0, 47	  ,
	0x31B2, 19	  ,
	0x31B4, 0x3C44,
	0x31B6, 0x314D,
	0x31B8, 0x208A,
	0x31BA, 0x0207,
	0x31BC, 0x8005,
	0x31BE, 0x2003,
};

MMP_USHORT SNR_AR0330_Reg_Init_OTPM_V5_Customer[] = 
{
    #if 1 //Rogers Test
	//Table 2. Recommend default register and Sequencer
	0x3ED2, 0x0146,
	0x3EDA, 0x88BC,
	0x3EDC, 0xAA63,
	0x305E, 0x0080,
	//0x3046, 0x4038,
	//0x3048, 0x8480,
	#endif
	
	//Initialization								   
	0x301A, 0x0059, 								 
	SNR_REG_DELAY, 10,								 
	0x3052, 0xA114, // fix low temp OTPM wrong issue   
	0x304A, 0x0070, // fix low temp OTPM wrong issue   
	SNR_REG_DELAY, 2, //10		

    #if 0 //Rogers Test
	//Table 2. Recommend default register and Sequencer
	0x3ED2, 0x0146,
	0x3EDA, 0x88BC,
	0x3EDC, 0xAA63,
	0x305E, 0x00A0,
	0x3046, 0x4038,
	0x3048, 0x8480,
    #endif 

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x31AE, 	0x0204,
    #elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x31AE, 	0x0202,
    #endif

	0x301A, 	0x0058, 								 
	SNR_REG_DELAY, 3, //35,								 
	0x3064, 	0x1802, 								 
	0x3078, 	0x0001, 								 
	0x31E0, 	0x0200, //0x0701,//0x0201, //0x0601, //0x0003, //0x0203, 	//Diane:for defect pixel correction							 

    //configure PLL
	0x302A, 5	  , // VT_PIX_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x302C, 1	  , // VT_SYS_CLK_DIV
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x302C, 2	  , // VT_SYS_CLK_DIV
    #endif

	0x302E, 2	  , // PRE_PLL_CLK_DIV

    #if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	0x3030, 41	  , // PLL_MULTIPLIER
    #else //elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	0x3030, 82	  , // PLL_MULTIPLIER
    #endif

	0x3036, 10	  , // OP_PIX_CLK_DIV
	0x3038, 1	  , // OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, // DATA_FORMAT_BITS
	0x31B0, 47	  ,
	0x31B2, 19	  ,
	0x31B4, 0x3C44,
	0x31B6, 0x314D,
	0x31B8, 0x208A,
	0x31BA, 0x0207,
	0x31BC, 0x8005,
	0x31BE, 0x2003,
};

//Mode0
//for 1080p30(W) / 720p30(W) / WVGA30(W)
//Interface	MIPI 4Lane
//sensor output	2266x1280
//frame rate	29.97
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2266x1280_30fps[] = 
{
	0x3004, 20,//26	  , 	//X_ADDR_START											
	0x3008, 2291  , 		//X_ADDR_END												
	0x3002, 134   , 		//Y_ADDR_START											
	0x3006, 1413  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000,  	    //Row Bin
	#endif
	0x300C, 1242  , 		//LINE_LENGTH_PCK
	0x300A, 661,//2634  , 	//FRAME_LENGTH_LINES
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 658,//2640  , 		//Coarse_Integration_Time						
	0x3042, 677   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed	

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,			//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,	//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  

	//0x3044, 0x0C00,       //Diane test OB 
										
	0x301A, 0x005C,		    //Enable Streaming									
};

//Mode1
//for 1080p60(W) -> 720p60(W)
//Interface	MIPI 4Lane
//sensor output	2266x1280
//frame rate	59.94
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2266x1280_60fps[] = 
{
	0x3004, 20,//26	  , 		//X_ADDR_START											
	0x3008, 2291  , 		//X_ADDR_END												
	0x3002, 134   , 		//Y_ADDR_START											
	0x3006, 1413  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif
	0x300C, 1242  , 		//LINE_LENGTH_PCK										
	0x300A, 1321,//1320  , 		//FRAME_LENGTH_LINES								
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 1320  , 		//Coarse_Integration_Time						
	0x3042, 959   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed	
    
	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
								//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode2
//for WVGA@120(W)
//Interface	MIPI 4Lane
//sensor output	1132x640
//frame rate	119.88
//Sampling	xy: 2xbin
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_1132x640_120fps[] = 
{
	0x3004, 20,//26	  , 	//X_ADDR_START											
	0x3008, 2289  , 		//X_ADDR_END												
	0x3002, 134   , 		//Y_ADDR_START											
	0x3006, 1411  , 		//Y_ADDR_END												
	0x30A2, 0x0003, 		//X_ODD_INCREMENT										
	0x30A6, 0x0003, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xF000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x3000, 		//Row Bin
	#endif
	0x300C, 1232,//1154  , 	//LINE_LENGTH_PCK										
	0x300A, 666,//711   , 	//FRAME_LENGTH_LINES								
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 665,//710   , 	//Coarse_Integration_Time						
	0x3042, 326   , 		//EXTRA_DELAY												
	0x30BA, 0x006C,//0x002C,//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0xBF46,
	0x3ED4,	0x8F3C,
	0x3ED6,	0x33CC,
	0x3ECC,	0x1D0D,
							//Sequencer
	0x301A, 0x0058,			//Disable Streaming
	SNR_REG_DELAY, 3, //30,	//Delay
	0x3088, 0x80BA,
	0x3086, 0xE653,

	0x301A, 0x005C,		    //Enable Streaming									
};

//------------------------------------------------------
//Mode3
//for 1080p30(S) / 720p30(S) / WVGA30(S)
//Interface	MIPI 4Lane
//sensor output	1928x1096
//frame rate	29.97
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_1928x1096_30fps[] = 
{
	0x3004, 22	  , 		//X_ADDR_START											
	0x3008, 2293  , 		//X_ADDR_END												
	0x3002, 226   , 		//Y_ADDR_START											
	0x3006, 1321  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif

	0x300C, 1248,//2200,//1242  , 		//LINE_LENGTH_PCK
    0x300A, 2630,//1492,//2643  , 		//FRAME_LENGTH_LINES

	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 2630,//1492,//2643,//2640  , 		//Coarse_Integration_Time						
	0x3042, 677   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed	

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653,//0x0253, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode4
//for 1080p60(S) -> 720p60(S)
//Interface	MIPI 4Lane
//sensor output	1928x1096
//frame rate	59.94
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_1928x1096_60fps[] = 
{
	0x3004, 20	  , 		//X_ADDR_START
	0x3008, 2291  , 		//X_ADDR_END
	0x3002, 226   , 		//Y_ADDR_START
	0x3006, 1321  , 		//Y_ADDR_END
	0x30A2, 0x0001, 		//X_ODD_INCREMENT
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif
	0x300C, 1242  , 		//LINE_LENGTH_PCK
	0x300A, 1321  , 		//FRAME_LENGTH_LINES
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 1320  , 		//Coarse_Integration_Time
	0x3042, 959   , 		//EXTRA_DELAY
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed
	
	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode5
//for WVGA@120(S)
//Interface	MIPI 4Lane
//sensor output	964x544
//frame rate	119.88
//Sampling	xy: 2xbin
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_964x544_120fps[] = 
{
	0x3004, 20,//194	  , 		//X_ADDR_START											
	0x3008, 2289,//2119,//2121  , 		//X_ADDR_END												
	0x3002, 230   , 		//Y_ADDR_START											
	0x3006, 1317  , 		//Y_ADDR_END												
	0x30A2, 0x0003, 		//X_ODD_INCREMENT										
	0x30A6, 0x0003, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xF000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x3000, 		//Row Bin
	#endif
	0x300C, 1232,//1154  , 		//LINE_LENGTH_PCK										
	0x300A, 666,//711   , 		//FRAME_LENGTH_LINES								
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 665,//710   , 		//Coarse_Integration_Time						
	0x3042, 326   , 		//EXTRA_DELAY												
	0x30BA, 0x006C, 		//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0xBF46,
	0x3ED4,	0x8F3C,
	0x3ED6,	0x33CC,
	0x3ECC,	0x1D0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode6
//for 2M@30(W) preview
//Interface	MIPI 4Lane
//sensor output	2184x1232
//frame rate	30
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2184x1232_30fps[] = 
{
	0x3004, 66	  , 		//X_ADDR_START
	0x3008, 2249  , 		//X_ADDR_END
	0x3002, 158   , 		//Y_ADDR_START
	0x3006, 1389  , 		//Y_ADDR_END
	0x30A2, 0x0001, 		//X_ODD_INCREMENT
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif
	0x300C, 2643  , 		//LINE_LENGTH_PCK
	0x300A, 1242  , 		//FRAME_LENGTH_LINES								
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 2640  , 		//Coarse_Integration_Time						
	0x3042, 677   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode7
//for 2M@15(W) Capture
//Interface	MIPI 4Lane
//sensor output	2184x1232
//frame rate	15
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2184x1232_15fps[] = 
{
	0x3004, 66	  , 		//X_ADDR_START											
	0x3008, 2249  , 		//X_ADDR_END												
	0x3002, 158   , 		//Y_ADDR_START											
	0x3006, 1389  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif

	0x300C, 1854,//1840,//1242,//5276,//5281  , 		//LINE_LENGTH_PCK										
	0x300A, 3538,//3568,//5281,//1244,//1242  , 		//FRAME_LENGTH_LINES								

	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 3538,//3568,//5281,//1244,//4806  , 		//Coarse_Integration_Time						
	0x3042, 998   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed
	
	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode8
//for 2M@30(S) preview
//Interface	MIPI 4Lane
//sensor output	1928x1088
//frame rate	30
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_1928x1088_30fps[] = 
{
	0x3004, 194	  , 		//X_ADDR_START
	0x3008, 2121  , 		//X_ADDR_END
	0x3002, 230   , 		//Y_ADDR_START
	0x3006, 1317  , 		//Y_ADDR_END
	0x30A2, 0x0001, 		//X_ODD_INCREMENT
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif
	0x300C, 2643  , 		//LINE_LENGTH_PCK
	0x300A, 1242  , 		//FRAME_LENGTH_LINES
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 2641  , 		//Coarse_Integration_Time						
	0x3042, 677   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed		
	
	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode9
//for 2M@15(S) Capture
//Interface	MIPI 4Lane
//sensor output	1928x1088
//frame rate	15
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_1928x1088_15fps[] = 
{
	0x3004, 194	  , 		//X_ADDR_START											
	0x3008, 2121  , 		//X_ADDR_END												
	0x3002, 230   , 		//Y_ADDR_START											
	0x3006, 1317  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif

	0x300C, 1854,//1840,//1242,//5282  , 		//LINE_LENGTH_PCK										
	0x300A, 3538,//3568,//5281,//1242  , 		//FRAME_LENGTH_LINES

	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 3538,//3568,//5281  , 		//Coarse_Integration_Time						
	0x3042, 998   , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode110
//for 3M@30(S) Capture
//Interface	MIPI 4Lane
//sensor output	2048x1536
//frame rate	30
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2048x1536_30fps[] = 
{ 
	0x3004, 128	  , 		//X_ADDR_START											
	0x3008, 2175  , 		//X_ADDR_END												
	0x3002, 6     , 		//Y_ADDR_START											
	0x3006, 1541  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif

	0x300C, 2118  , 		//LINE_LENGTH_PCK										
	0x300A, 1548  , 		//FRAME_LENGTH_LINES

	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 2118  , 		//Coarse_Integration_Time						
	0x3042, 0     , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
};

//Mode15 
//for 2304x1536@25 (full FOV)
//Interface	MIPI 4Lane
//sensor output	2304x1536
//frame rate	25
//Sampling	no sampling
//PLL setting	PLL-01
ISP_UINT16 SNR_AR0330_Reg_2304x1536_25fps[] = 
{
	0x3004, 6	  , 		//X_ADDR_START											
	0x3008, 2309  , 		//X_ADDR_END												
	0x3002, 6     , 		//Y_ADDR_START											
	0x3006, 1541  , 		//Y_ADDR_END												
	0x30A2, 0x0001, 		//X_ODD_INCREMENT										
	0x30A6, 0x0001, 		//Y_ODD_INCREMENT
	#if (WIMI360_REALBOARD)
	0x3040, 0xC000,         // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#else
	0x3040, 0x0000, 		//Row Bin
	#endif
	0x300C, 1248  , 		//LINE_LENGTH_PCK										

	//0x300A, 3153  , 		//FRAME_LENGTH_LINES //30 fps
	0x300A, 9459  , 		//FRAME_LENGTH_LINES //10 fps  3153*(30/10) = 9459
	
	0x3014, 0	  , 		//FINE_INTEGRATION_TIME(fixed value)
	0x3012, 0     , 		//Coarse_Integration_Time						
	0x3042, 0     , 		//EXTRA_DELAY												
	0x30BA, 0x002C, 		//Digital_Ctrl_Adc_High_Speed

	0x3ED2,	0x0146,
	0x3ED4,	0x8F6C,
	0x3ED6,	0x66CC,
	0x3ECC,	0x0E0D,
							//Sequencer 							
	0x301A, 0x0058,				//Disable Streaming 								
	SNR_REG_DELAY, 3, //30,		//Delay 							  
	0x3088, 0x80BA, 																			  
	0x3086, 0xE653, 																			  
																	
	0x301A, 0x005C,		//Enable Streaming									
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
#if (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	RTNA_DBG_Str(0, "SNR_Cust_InitConfig AR0330 MIPI 2-lane\r\n");
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	RTNA_DBG_Str(0, "SNR_Cust_InitConfig AR0330 MIPI 4-lane\r\n");
#endif

    SensorCustFunc.OprTable->bBinTableExist                         = MMP_FALSE;
    SensorCustFunc.OprTable->bInitDoneTableExist                    = MMP_FALSE;

    // Init OPR Table
#if (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)

    SensorCustFunc.OprTable->usInitSize                             = (sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                           = &SNR_AR0330_Reg_Init_OTPM_V5_Customer[0];

    // For Video Wide
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30FPS_W]		= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30FPS_W]	= &SNR_AR0330_Reg_2266x1280_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_30FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_30FPS_W]		= &SNR_AR0330_Reg_2266x1280_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_30FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_30FPS_W]		= &SNR_AR0330_Reg_2266x1280_30fps[0];

    // For Video Standard
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30FPS_S]		= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30FPS_S]	= &SNR_AR0330_Reg_1928x1096_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_30FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_30FPS_S]		= &SNR_AR0330_Reg_1928x1096_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_30FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_30FPS_S]		= &SNR_AR0330_Reg_1928x1096_30fps[0];

#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)

    SensorCustFunc.OprTable->usInitSize 				            = (sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable				            = &SNR_AR0330_Reg_Init_OTPM_V5_Customer[0];

    // For Video Wide
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30FPS_W]		= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30FPS_W]	= &SNR_AR0330_Reg_2266x1280_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_60FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2266x1280_60fps)/sizeof(SNR_AR0330_Reg_2266x1280_60fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_60FPS_W]		= &SNR_AR0330_Reg_2266x1280_60fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_30FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_30FPS_W]		= &SNR_AR0330_Reg_2266x1280_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_30FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2266x1280_30fps)/sizeof(SNR_AR0330_Reg_2266x1280_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_30FPS_W]		= &SNR_AR0330_Reg_2266x1280_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_120FPS_W]	  	= (sizeof(SNR_AR0330_Reg_1132x640_120fps)/sizeof(SNR_AR0330_Reg_1132x640_120fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_120FPS_W]		= &SNR_AR0330_Reg_1132x640_120fps[0];

    // For Video Standard
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30FPS_S]		= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30FPS_S]	= &SNR_AR0330_Reg_1928x1096_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_60FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_60fps)/sizeof(SNR_AR0330_Reg_1928x1096_60fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_60FPS_S]	= &SNR_AR0330_Reg_1928x1096_60fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_60FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_60fps)/sizeof(SNR_AR0330_Reg_1928x1096_60fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_60FPS_S]		= &SNR_AR0330_Reg_1928x1096_60fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_30FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_30FPS_S]		= &SNR_AR0330_Reg_1928x1096_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_30FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1096_30fps)/sizeof(SNR_AR0330_Reg_1928x1096_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_30FPS_S]		= &SNR_AR0330_Reg_1928x1096_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_848x480_120FPS_S]	  	= (sizeof(SNR_AR0330_Reg_964x544_120fps)/sizeof(SNR_AR0330_Reg_964x544_120fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_848x480_120FPS_S]		= &SNR_AR0330_Reg_964x544_120fps[0];

    // For Photo
    SensorCustFunc.OprTable->usSize[RES_IDX_2184x1232_30FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2184x1232_30fps)/sizeof(SNR_AR0330_Reg_2184x1232_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_2184x1232_30FPS_W]	= &SNR_AR0330_Reg_2184x1232_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_2184x1232_15FPS_W]	  	= (sizeof(SNR_AR0330_Reg_2184x1232_15fps)/sizeof(SNR_AR0330_Reg_2184x1232_15fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_2184x1232_15FPS_W]	= &SNR_AR0330_Reg_2184x1232_15fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1928x1088_30FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1088_30fps)/sizeof(SNR_AR0330_Reg_1928x1088_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1928x1088_30FPS_S]	= &SNR_AR0330_Reg_1928x1088_30fps[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1928x1088_15FPS_S]	  	= (sizeof(SNR_AR0330_Reg_1928x1088_15fps)/sizeof(SNR_AR0330_Reg_1928x1088_15fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1928x1088_15FPS_S]	= &SNR_AR0330_Reg_1928x1088_15fps[0];

    SensorCustFunc.OprTable->usSize[RES_IDX_2048x1536_30FPS]	  	= (sizeof(SNR_AR0330_Reg_2048x1536_30fps)/sizeof(SNR_AR0330_Reg_2048x1536_30fps[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_2048x1536_30FPS]		= &SNR_AR0330_Reg_2048x1536_30fps[0];

#endif

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
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin         = MMP_GPIO_MAX;	
	SensorCustFunc.VifSetting->powerOnSetting.bExtPowerPinHigh	    = SENSOR_GPIO_ENABLE_ACT_LEVEL;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh       = (SENSOR_GPIO_ENABLE_ACT_LEVEL == GPIO_HIGH) ? MMP_TRUE : MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay     = 0;
    SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh        = (SENSOR_GPIO_ENABLE_ACT_LEVEL == GPIO_HIGH) ? MMP_FALSE : MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay      = 2;
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh      = (SENSOR_RESET_ACT_LEVEL == GPIO_HIGH) ? MMP_TRUE : MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay    = 2;
    SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh       = (SENSOR_RESET_ACT_LEVEL == GPIO_HIGH) ? MMP_FALSE : MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay     = 2;
    
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
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]            = MMPF_VIF_MIPI_DATA_SRC_PHY_0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]            = MMPF_VIF_MIPI_DATA_SRC_PHY_1;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]              = 0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]             = 0x1F;
#endif

	// Init Vif Setting : Color ID Setting
	SensorCustFunc.VifSetting->colorId.VifColorId				    = MMPF_VIF_COLORID_01;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId   = MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
static ISP_UINT32 isp_dgain, dGainBase = 0x200;

void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
    if ((ulFrameCnt % m_AeTime.ubPeriod) == 0) {
		ISP_IF_AE_Execute();
    }
	if ((ulFrameCnt % m_AeTime.ubPeriod) == m_AeTime.ubFrmStSetShutFrmCnt) {
		gsSensorFunction->MMPF_Sensor_SetShutter(PRM_SENSOR, 0, 0);
    }
	if ((ulFrameCnt % m_AeTime.ubPeriod) == m_AeTime.ubFrmStSetGainFrmCnt) {
		gsSensorFunction->MMPF_Sensor_SetGain(PRM_SENSOR, ISP_IF_AE_GetGain());
		ISP_IF_IQ_SetAEGain(isp_dgain, dGainBase);
	}

    #if 0
	RTNA_DBG_Short(0,ulFrameCnt);
	RTNA_DBG_Str(0,"\r\n");
    RTNA_DBG_Short(0,ISP_IF_AE_GetDbgData(0));
    RTNA_DBG_Short(0,ISP_IF_AE_GetDbgData(1));
    RTNA_DBG_Dec(0,ISP_IF_AWB_GetColorTemp());
    RTNA_DBG_Short(0,ISP_IF_AE_GetShutter());
    RTNA_DBG_Short(0,ISP_IF_AE_GetGain());
    RTNA_DBG_Str(0,"\r\n");
    RTNA_DBG_Str(0,"\r\n");
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
    ISP_UINT16 sensor_again;
    ISP_UINT16 sensor_dgain;

    ulGain = ulGain * 0x40 / ISP_IF_AE_GetGainBase();
	isp_dgain = dGainBase;

    // Sensor Gain Mapping
    if(ulGain < 0x80){
        sensor_dgain = ulGain << 1;   
        sensor_again = 0x00;     // 1X ~ 2X
    }
    else if (ulGain < 0x100){
        sensor_dgain = ulGain;   
        sensor_again = 0x10;    // 2X ~ 4X
    }       
    else if (ulGain < 0x200){
        sensor_dgain = ulGain >> 1;   
        sensor_again = 0x20;    // 4X ~ 8X
    }   
    else{
        sensor_dgain = ulGain >> 2;  
        sensor_again = 0x30;    // 8X ~16X

   	    if (sensor_dgain >= 0x100) { // >=15.99x , sensor_dgain<=1.99x
	        sensor_again = 0x30;    // 8X 
			sensor_dgain = 0xFF;	// 1.99x
			isp_dgain = ulGain * dGainBase * 0x80 / 0x08/ 0xFF/0x40; //ulGain base=0x40, again base = 0x40, ispgain base=0x200
		}
    }      

    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x305E, sensor_dgain);
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3060, sensor_again);
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetShutter
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG shutter, MMP_ULONG vsync)
{
#if (ISP_EN)
	ISP_UINT32 new_vsync    = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetVsync() / ISP_IF_AE_GetVsyncBase();
	ISP_UINT32 new_shutter  = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter() / ISP_IF_AE_GetShutterBase();

	new_vsync   = ISP_MIN(ISP_MAX(new_shutter + 3, new_vsync), 0xFFFF);
	new_shutter = ISP_MIN(ISP_MAX(new_shutter, 1), new_vsync - 3);
	
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x300A, new_vsync);
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3012, new_shutter);
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetFlip
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetFlip(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
    MMP_USHORT  usRdVal;
    
	if (ubMode == MMPF_SENSOR_NO_FLIP) 
	{
	    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x3040, &usRdVal);
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3040, usRdVal & 0x3FFF);
        SensorCustFunc.VifSetting->colorId.VifColorId = MMPF_VIF_COLORID_01;
	}	
	else if (ubMode == MMPF_SENSOR_COLROW_FLIP) 
	{
	    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x3040, &usRdVal);
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3040, usRdVal | 0xC000);
        SensorCustFunc.VifSetting->colorId.VifColorId = MMPF_VIF_COLORID_10;
	}
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
	MMP_USHORT usRdVal300E,usRdVal30F0,usRdVal3072 = 0;
    
    if(gOTPM_Ver == 0xFF)
    {
        /* Initial I2cm */
        MMPF_I2cm_Initialize(&m_I2cmAttr);
        MMPF_OS_SleepMs(2);

    	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x301A, 0x0001);
    	MMPF_OS_SleepMs(2);
    
        gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x300E, &usRdVal300E);
    	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x30F0, &usRdVal30F0); 
    	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x3072, &usRdVal3072); 

        if (usRdVal3072 != 0x0) {
    		if (usRdVal3072 == 0x0008)
    			gOTPM_Ver = 5;
    		else if (usRdVal3072 == 0x0007)
    			gOTPM_Ver = 4;
    		else
    			gOTPM_Ver = 3;
        }
    	else {
    		if (usRdVal300E == 0x10)
    			gOTPM_Ver = 1;
    		else
    			gOTPM_Ver = 2;
    	}

    	printc(FG_BLUE("@@@AR0330-Ver%d")"\r\n", gOTPM_Ver);

    	switch(gOTPM_Ver) {
    	case 1:
        	SensorCustFunc.OprTable->usInitSize   = (sizeof(SNR_AR0330_Reg_Init_OTPM_V1_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V1_Customer[0]))/2;
        	SensorCustFunc.OprTable->uspInitTable = &SNR_AR0330_Reg_Init_OTPM_V1_Customer[0];
        	break;
    	case 2:
        	SensorCustFunc.OprTable->usInitSize   = (sizeof(SNR_AR0330_Reg_Init_OTPM_V2_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V2_Customer[0]))/2;
        	SensorCustFunc.OprTable->uspInitTable = &SNR_AR0330_Reg_Init_OTPM_V2_Customer[0];
        	break;
        case 3:
        	SensorCustFunc.OprTable->usInitSize   = (sizeof(SNR_AR0330_Reg_Init_OTPM_V3_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V3_Customer[0]))/2;
        	SensorCustFunc.OprTable->uspInitTable = &SNR_AR0330_Reg_Init_OTPM_V3_Customer[0];
        	break;
    	case 4:
        	SensorCustFunc.OprTable->usInitSize   = (sizeof(SNR_AR0330_Reg_Init_OTPM_V4_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V4_Customer[0]))/2;
        	SensorCustFunc.OprTable->uspInitTable = &SNR_AR0330_Reg_Init_OTPM_V4_Customer[0];
        	break;
    	case 5:
        	SensorCustFunc.OprTable->usInitSize   = (sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer)/sizeof(SNR_AR0330_Reg_Init_OTPM_V5_Customer[0]))/2;
        	SensorCustFunc.OprTable->uspInitTable = &SNR_AR0330_Reg_Init_OTPM_V5_Customer[0];
        	break;
    	}
    }
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
    &m_AfTime,
    MMP_SNR_PRIO_PRM
};

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

#endif // (BIND_SENSOR_AR0330_OTPM)
#endif // (SENSOR_EN)
