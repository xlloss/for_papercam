//==============================================================================
//
//  File        : sensor_AR0237.c
//  Description : Firmware Sensor Control File
//  Author      :
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "Customer_config.h"
#include "mmpf_pll.h"

#if (SENSOR_EN)
#if (BIND_SENSOR_AR0237)

#include "mmpf_sensor.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"
#include "sensor_ar0237.h"

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

// Resolution Table
MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
  3,   // ubSensorModeNum
  0,    // ubDefPreviewMode
  0,    // ubDefCaptureMode
  3000, // usPixelSize
  // 	Mode0   Mode1   Mode2	Mode3	Mode4   Mode5   Mode6   Mode7   Mode8   Mode9   Mode10  Mode11	Mode12	Mode13
  {1,		  1		 ,1   },    	// usVifGrabStX
  {1, 	  1		 ,1   },    	// usVifGrabStY 
  {1928,	1928 ,1288}, 	// usVifGrabW -> WOI_HSIZE(0xA9,0xAA)
  {1088,	1088 ,728 }, 	// usVifGrabH -> WOI_VSIZE(0xA5,0xA6)
  /* #if (CHIP == MCR_V2) */
  {1,		  1    ,1   },    	// usBayerInGrabX
  {1,		  1    ,1   },    	// usBayerInGrabY
  {8,		  8    ,8   },   	// usBayerInDummyX
  {8,		  8    ,8   },    	// usBayerInDummyY
  {1920,	1280 ,1280},	// usBayerOutW
  {1080,	 720 ,720 }, 	// usBayerOutH
  /* #endif */
  {1920,	1280 ,1280}, 	// usScalInputW
  {1080,	720  ,720 }, 	// usScalInputH
  {300,   300  ,600 }, 	// usTargetFpsx10
  {1124,	1124 ,741 },	// usVsyncLine -> LPF(0x0A,0x0B)
  {1,		  1		 ,1   },    	// ubWBinningN
  {1,		  1		 ,1   },    	// ubWBinningM
  {1,		  1		 ,1   },    	// ubHBinningN
  {1,		  1		 ,1   },     // ubHBinningM
  {0x00,	0x00 ,0x00},  // ubCustIQmode
  {0x00,	0x00 ,0x00 }   // ubCustAEmode
};


// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE       m_OprTable;
MMPF_SENSOR_VIF_SETTING     m_VifSetting;

// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
#if defined(SUPPORT_UVC_ISP_EZMODE_FUNC) && (SUPPORT_UVC_ISP_EZMODE_FUNC) //TBD        














#include "isp_8428_iq_data_v3_PS5226_ezmode_20170608.xls.ciq.txt"

#else //Use old IQ table
/* #include "isp_8428_iq_data_v3_AR0237_ezmode_CDA_E_20170327.xls.ciq.xls.ciq.txt" */
#include "isp_8428_iq_data_v2_PS5226_20170511_5FPS.xls.ciq.txt"
#endif
};

#if defined(SUPPORT_UVC_ISP_EZMODE_FUNC) && (SUPPORT_UVC_ISP_EZMODE_FUNC) //TBD          
const  __align(4) ISP_UINT8 Sensor_EZ_IQ_CompressedText[] = 
{
NULL
//#include "eziq_0509.txt"
//#include "0920.txt"
};

ISP_UINT32 eziqsize = sizeof(Sensor_EZ_IQ_CompressedText);
#endif

// IQ Data
/* #define ISP_IQ_DATA_NAME "isp_8428_iq_data_v3_AR0237_ezmode_CDA_E_20170327.xls.ciq.txt" */
#define ISP_IQ_DATA_NAME "isp_8428_iq_data_v3_PS5226_ezmode_20170608.xls.ciq.txt"

static const MMP_UBYTE s_IqCompressData[] =
{
	#include ISP_IQ_DATA_NAME
};

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = {
	MMP_I2CM0,      // i2cmID
//#if (SENSOR_IF == SENSOR_IF_PARALLEL)
    0x20>>1,       // ubSlaveAddr
//#else
//    0x10,       // ubSlaveAddr
//#endif
	16, 			// ubRegLen
	16, 			// ubDataLen
	0, 				// ubDelayTime
	MMP_FALSE, 		// bDelayWaitEn
	MMP_TRUE, 		// bInputFilterEn
	MMP_FALSE, 		// b10BitModeEn
	MMP_FALSE, 		// bClkStretchEn
	0, 				// ubSlaveAddr1
	0, 				// ubDelayCycle
	0, 				// ubPadNum
	150000, 		// ulI2cmSpeed 250KHZ
	MMP_TRUE, 		// bOsProtectEn
	0, 			// sw_clk_pin
	0, 			// sw_data_pin
	MMP_FALSE, 		// bRfclModeEn
	MMP_FALSE,      // bWfclModeEn
	MMP_FALSE,		// bRepeatModeEn
    0               // ubVifPioMdlId
};

// 3A Timing
MMPF_SENSOR_AWBTIMIMG m_AwbTime = 
{
	6,	/* ubPeriod */
	1, 	/* ubDoAWBFrmCnt */
	3	/* ubDoCaliFrmCnt */
};

MMPF_SENSOR_AETIMIMG    m_AeTime     = 
{	
	6, 	/* ubPeriod */
	0, 	/* ubFrmStSetShutFrmCnt */
	0	/* ubFrmStSetGainFrmCnt */
};

MMPF_SENSOR_AFTIMIMG m_AfTime = 
{
	1, 	// ubPeriod
	0 	// ubDoAFFrmCnt
};


//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Sensor_Init_OPR_Table____(){ruturn;} //dummy
#endif

ISP_UINT16 SNR_AR0237_Reg_Unsupport[] =
{
	SENSOR_DELAY_REG, 100 // delay
};

ISP_UINT16 SNR_AR0237_Reg_Init_Customer[] =
{
	// TBD
	SENSOR_DELAY_REG, 100 // delay
};
//#if (SENSOR_IF == SENSOR_IF_PARALLEL)

ISP_UINT16 SNR_AR0237_Reg_1920x1080_30P[] = // PARA12_1928*1088_30fps_27MCLK_74.25PCLK
{
    0x301A, 0x0001,    //RESET_REGISTER
	0xffff, 0x0002,	   //APTINA necessary 
    0x301A, 0x10D8,    //RESET_REGISTER
	0xffff, 0x0002,	   //APTINA necessary 
    0x3088, 0x8000,    //SEQ_CTRL_PORT
    
	// New linear mode from Joyce 0509
	//#if (!REMOVE_SENSOR_MANUAL_OTP_SETTINGS)    
    0x3086, 0x4558,    //SEQ_DATA_PORT
    0x3086, 0x72A6,
    0x3086, 0x4A31,
    0x3086, 0x4342,
    0x3086, 0x8E03,
    0x3086, 0x2A14,
    0x3086, 0x4578,
    0x3086, 0x7B3D,
    0x3086, 0xFF3D,
    0x3086, 0xFF3D,
    0x3086, 0xEA2A,
    0x3086, 0x043D,
    0x3086, 0x102A,
    0x3086, 0x052A,
    0x3086, 0x1535,
    0x3086, 0x2A05,
    0x3086, 0x3D10,
    0x3086, 0x4558,
    0x3086, 0x2A04,
    0x3086, 0x2A14,
    0x3086, 0x3DFF,
    0x3086, 0x3DFF,
    0x3086, 0x3DEA,
    0x3086, 0x2A04,
    0x3086, 0x622A,
    0x3086, 0x288E,
    0x3086, 0x0036,
    0x3086, 0x2A08,
    0x3086, 0x3D64,
    0x3086, 0x7A3D,
    0x3086, 0x0444,
    0x3086, 0x2C4B,
    0x3086, 0xA403,
    0x3086, 0x430D,
    0x3086, 0x2D46,
    0x3086, 0x4316,
    0x3086, 0x2A90,
    0x3086, 0x3E06,
    0x3086, 0x2A98,
    0x3086, 0x5F16,
    0x3086, 0x530D,
    0x3086, 0x1660,
    0x3086, 0x3E4C,
    0x3086, 0x2904,
    0x3086, 0x2984,
    0x3086, 0x8E03,
    0x3086, 0x2AFC,
    0x3086, 0x5C1D,
    0x3086, 0x5754,
    0x3086, 0x495F,
    0x3086, 0x5305,
    0x3086, 0x5307,
    0x3086, 0x4D2B,
    0x3086, 0xF810,
    0x3086, 0x164C,
    0x3086, 0x0955,
    0x3086, 0x562B,
    0x3086, 0xB82B,
    0x3086, 0x984E,
    0x3086, 0x1129,
    0x3086, 0x9460,
    0x3086, 0x5C19,
    0x3086, 0x5C1B,
    0x3086, 0x4548,
    0x3086, 0x4508,
    0x3086, 0x4588,
    0x3086, 0x29B6,
    0x3086, 0x8E01,
    0x3086, 0x2AF8,
    0x3086, 0x3E02,
    0x3086, 0x2AFA,
    0x3086, 0x3F09,
    0x3086, 0x5C1B,
    0x3086, 0x29B2,
    0x3086, 0x3F0C,
    0x3086, 0x3E03,
    0x3086, 0x3E15,
    0x3086, 0x5C13,
    0x3086, 0x3F11,
    0x3086, 0x3E0F,
    0x3086, 0x5F2B,
    0x3086, 0x902B,
    0x3086, 0x803E,
    0x3086, 0x062A,
    0x3086, 0xF23F,
    0x3086, 0x103E,
    0x3086, 0x0160,
    0x3086, 0x29A2,
    0x3086, 0x29A3,
    0x3086, 0x5F4D,
    0x3086, 0x1C2A,
    0x3086, 0xFA29,
    0x3086, 0x8345,
    0x3086, 0xA83E,
    0x3086, 0x072A,
    0x3086, 0xFB3E,
    0x3086, 0x6745,
    0x3086, 0x8824,
    0x3086, 0x3E08,
    0x3086, 0x2AFA,
    0x3086, 0x5D29,
    0x3086, 0x9288,
    0x3086, 0x102B,
    0x3086, 0x048B,
    0x3086, 0x1686,
    0x3086, 0x8D48,
    0x3086, 0x4D4E,
    0x3086, 0x2B80,
    0x3086, 0x4C0B,
    0x3086, 0x3F36,
    0x3086, 0x2AF2,
    0x3086, 0x3F10,
    0x3086, 0x3E01,
    0x3086, 0x6029,
    0x3086, 0x8229,
    0x3086, 0x8329,
    0x3086, 0x435C,
    0x3086, 0x155F,
    0x3086, 0x4D1C,
    0x3086, 0x2AFA,
    0x3086, 0x4558,
    0x3086, 0x8E00,
    0x3086, 0x2A98,
    0x3086, 0x3F0A,
    0x3086, 0x4A0A,
    0x3086, 0x4316,
    0x3086, 0x0B43,
    0x3086, 0x168E,
    0x3086, 0x032A,
    0x3086, 0x9C45,
    0x3086, 0x783F,
    0x3086, 0x072A,
    0x3086, 0x9D3E,
    0x3086, 0x305D,
    0x3086, 0x2944,
    0x3086, 0x8810,
    0x3086, 0x2B04,
    0x3086, 0x530D,
    0x3086, 0x4558,
    0x3086, 0x3E08,
    0x3086, 0x8E01,
    0x3086, 0x2A98,
    0x3086, 0x8E00,
    0x3086, 0x76A7,
    0x3086, 0x77A7,
    0x3086, 0x4644,
    0x3086, 0x1616,
    0x3086, 0xA57A,
    0x3086, 0x1244,
    0x3086, 0x4B18,
    0x3086, 0x4A04,
    0x3086, 0x4316,
    0x3086, 0x0643,
    0x3086, 0x1605,
    0x3086, 0x4316,
    0x3086, 0x0743,
    0x3086, 0x1658,
    0x3086, 0x4316,
    0x3086, 0x5A43,
    0x3086, 0x1645,
    0x3086, 0x588E,
    0x3086, 0x032A,
    0x3086, 0x9C45,
    0x3086, 0x787B,
    0x3086, 0x3F07,
    0x3086, 0x2A9D,
    0x3086, 0x530D,
    0x3086, 0x8B16,
    0x3086, 0x863E,
    0x3086, 0x2345,
    0x3086, 0x5825,
    0x3086, 0x3E10,
    0x3086, 0x8E01,
    0x3086, 0x2A98,
    0x3086, 0x8E00,
    0x3086, 0x3E10,
    0x3086, 0x8D60,
    0x3086, 0x1244,
    0x3086, 0x4BB9,
    0x3086, 0x2C2C,
    0x3086, 0x2C2C,    // SEQ_DATA_PORT
    //#endif
    
    0x301A, 0x10D8,    // RESET_REGISTER
    0x30B0, 0x1A38,    // DIGITAL_TEST
    0x31AC, 0x0C0C,    // DATA_FORMAT_BITS
    0x302A, 0x0008,    // VT_PIX_CLK_DIV
    0x302C, 0x0001,    // VT_SYS_CLK_DIV
//    0x302E, 0x0008,    // PRE_PLL_CLK_DIV , 74M
//    0x3030, 0x00C6,    // PLL_MULTIPLIER C6h=198
    0x302E, 0x0008,    // PRE_PLL_CLK_DIV , 74M
    0x3030, 0x00C6,    // PLL_MULTIPLIER C6h=198

    0x3036, 0x000C,    // OP_PIX_CLK_DIV
    0x3038, 0x0001,    // OP_SYS_CLK_DIV
    0x3002, 0x0000,    // Y_ADDR_START
    0x3004, 0x0000,    // X_ADDR_START
    0x3006, 0x043F,    // Y_ADDR_END,         1088
    0x3008, 0x0787,    // X_ADDR_END,         1928
    0x300A, 0x0465,    // FRAME_LENGTH_LINES, VT,1125
    0x300C, 0x044C,    // LINE_LENGTH_PCK,    HT,1100
    0x3012, 0x0416,    // COARSE_INTEGRATION_TIME
    0x30A2, 0x0001,    // X_ODD_INC,subsampling
    0x30A6, 0x0001,    // Y_ODD_INC
    0x30AE, 0x0001,    // X_ODD_INC_CB
    0x30A8, 0x0001,    // Y_ODD_INC_CB
    0x3040, 0x0000,    // READ_MODE
    0x31AE, 0x0301,    // SERIAL_FORMAT
    0x3082, 0x0009,    // OPERATION_MODE_CTRL,ERS linear
    0x30BA, 0x760C,    // DIGITAL_CTRL
    0x3100, 0x0000,    // AECTRLREG
    0x3060, 0x000B,    // GAIN
    0x31D0, 0x0000,    // COMPANDING
    0x3064, 0x1802,    // SMIA_TEST
    0x3EEE, 0xA0AA,    // DAC_LD_34_35
    0x30BA, 0x762C,    // DIGITAL_CTRL
    0x3F4A, 0x0F70,    // DELTA_DK_PIX_THRES
    0x309E, 0x016C,    // HIDY_PROG_START_ADDR
    0x3092, 0x006F,    // ROW_NOISE_CONTROL
    0x3EE4, 0x9937,    // DAC_LD_24_25
    0x3EE6, 0x3863,    // DAC_LD_26_27
    0x3EEC, 0x3B0C,    // DAC_LD_32_33
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30BA, 0x762C,    // DIGITAL_CTRL
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30B0, 0x0A3A,    // DIGITAL_TEST
    0x3EEA, 0x2838,    // DAC_LD_30_31
    0x3ECC, 0x4E2D,    // DAC_LD_0_1
    0x3ED2, 0xFEA6,    // DAC_LD_6_7
    0x3ED6, 0x2CB3,    // DAC_LD_10_11
    0x3EEA, 0x2819,    // DAC_LD_30_31
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    //0x306E, 0x2418,    // DATAPATH_SELECT
    //0x306E, 0x2C18,    // DATAPATH_SELECT , 12:10 , PCLK slew rate
    0x306E, 0x9018,    // DATAPATH_SELECT , 12:10 , PCLK slew rate
    0x301A, 0x10DC,    // RESET_REGISTER,para,stream on
    //0x3070, 0x0003,    // TestPattern,
};

//#endif


ISP_UINT16 SNR_AR0237_Reg_1280_720_30P[] = // PARA12_1928*1088_30fps_27MCLK_74.25PCLK
{
    0x301A, 0x0001,    //RESET_REGISTER
	0xffff, 0x0002,	   //APTINA necessary 
    0x301A, 0x10D8,    //RESET_REGISTER
	0xffff, 0x0002,	   //APTINA necessary 
    0x3088, 0x8000,    //SEQ_CTRL_PORT
    
	// New linear mode from Joyce 0509
	//#if (!REMOVE_SENSOR_MANUAL_OTP_SETTINGS)    
    0x3086, 0x4558,    //SEQ_DATA_PORT
    0x3086, 0x72A6,
    0x3086, 0x4A31,
    0x3086, 0x4342,
    0x3086, 0x8E03,
    0x3086, 0x2A14,
    0x3086, 0x4578,
    0x3086, 0x7B3D,
    0x3086, 0xFF3D,
    0x3086, 0xFF3D,
    0x3086, 0xEA2A,
    0x3086, 0x043D,
    0x3086, 0x102A,
    0x3086, 0x052A,
    0x3086, 0x1535,
    0x3086, 0x2A05,
    0x3086, 0x3D10,
    0x3086, 0x4558,
    0x3086, 0x2A04,
    0x3086, 0x2A14,
    0x3086, 0x3DFF,
    0x3086, 0x3DFF,
    0x3086, 0x3DEA,
    0x3086, 0x2A04,
    0x3086, 0x622A,
    0x3086, 0x288E,
    0x3086, 0x0036,
    0x3086, 0x2A08,
    0x3086, 0x3D64,
    0x3086, 0x7A3D,
    0x3086, 0x0444,
    0x3086, 0x2C4B,
    0x3086, 0xA403,
    0x3086, 0x430D,
    0x3086, 0x2D46,
    0x3086, 0x4316,
    0x3086, 0x2A90,
    0x3086, 0x3E06,
    0x3086, 0x2A98,
    0x3086, 0x5F16,
    0x3086, 0x530D,
    0x3086, 0x1660,
    0x3086, 0x3E4C,
    0x3086, 0x2904,
    0x3086, 0x2984,
    0x3086, 0x8E03,
    0x3086, 0x2AFC,
    0x3086, 0x5C1D,
    0x3086, 0x5754,
    0x3086, 0x495F,
    0x3086, 0x5305,
    0x3086, 0x5307,
    0x3086, 0x4D2B,
    0x3086, 0xF810,
    0x3086, 0x164C,
    0x3086, 0x0955,
    0x3086, 0x562B,
    0x3086, 0xB82B,
    0x3086, 0x984E,
    0x3086, 0x1129,
    0x3086, 0x9460,
    0x3086, 0x5C19,
    0x3086, 0x5C1B,
    0x3086, 0x4548,
    0x3086, 0x4508,
    0x3086, 0x4588,
    0x3086, 0x29B6,
    0x3086, 0x8E01,
    0x3086, 0x2AF8,
    0x3086, 0x3E02,
    0x3086, 0x2AFA,
    0x3086, 0x3F09,
    0x3086, 0x5C1B,
    0x3086, 0x29B2,
    0x3086, 0x3F0C,
    0x3086, 0x3E03,
    0x3086, 0x3E15,
    0x3086, 0x5C13,
    0x3086, 0x3F11,
    0x3086, 0x3E0F,
    0x3086, 0x5F2B,
    0x3086, 0x902B,
    0x3086, 0x803E,
    0x3086, 0x062A,
    0x3086, 0xF23F,
    0x3086, 0x103E,
    0x3086, 0x0160,
    0x3086, 0x29A2,
    0x3086, 0x29A3,
    0x3086, 0x5F4D,
    0x3086, 0x1C2A,
    0x3086, 0xFA29,
    0x3086, 0x8345,
    0x3086, 0xA83E,
    0x3086, 0x072A,
    0x3086, 0xFB3E,
    0x3086, 0x6745,
    0x3086, 0x8824,
    0x3086, 0x3E08,
    0x3086, 0x2AFA,
    0x3086, 0x5D29,
    0x3086, 0x9288,
    0x3086, 0x102B,
    0x3086, 0x048B,
    0x3086, 0x1686,
    0x3086, 0x8D48,
    0x3086, 0x4D4E,
    0x3086, 0x2B80,
    0x3086, 0x4C0B,
    0x3086, 0x3F36,
    0x3086, 0x2AF2,
    0x3086, 0x3F10,
    0x3086, 0x3E01,
    0x3086, 0x6029,
    0x3086, 0x8229,
    0x3086, 0x8329,
    0x3086, 0x435C,
    0x3086, 0x155F,
    0x3086, 0x4D1C,
    0x3086, 0x2AFA,
    0x3086, 0x4558,
    0x3086, 0x8E00,
    0x3086, 0x2A98,
    0x3086, 0x3F0A,
    0x3086, 0x4A0A,
    0x3086, 0x4316,
    0x3086, 0x0B43,
    0x3086, 0x168E,
    0x3086, 0x032A,
    0x3086, 0x9C45,
    0x3086, 0x783F,
    0x3086, 0x072A,
    0x3086, 0x9D3E,
    0x3086, 0x305D,
    0x3086, 0x2944,
    0x3086, 0x8810,
    0x3086, 0x2B04,
    0x3086, 0x530D,
    0x3086, 0x4558,
    0x3086, 0x3E08,
    0x3086, 0x8E01,
    0x3086, 0x2A98,
    0x3086, 0x8E00,
    0x3086, 0x76A7,
    0x3086, 0x77A7,
    0x3086, 0x4644,
    0x3086, 0x1616,
    0x3086, 0xA57A,
    0x3086, 0x1244,
    0x3086, 0x4B18,
    0x3086, 0x4A04,
    0x3086, 0x4316,
    0x3086, 0x0643,
    0x3086, 0x1605,
    0x3086, 0x4316,
    0x3086, 0x0743,
    0x3086, 0x1658,
    0x3086, 0x4316,
    0x3086, 0x5A43,
    0x3086, 0x1645,
    0x3086, 0x588E,
    0x3086, 0x032A,
    0x3086, 0x9C45,
    0x3086, 0x787B,
    0x3086, 0x3F07,
    0x3086, 0x2A9D,
    0x3086, 0x530D,
    0x3086, 0x8B16,
    0x3086, 0x863E,
    0x3086, 0x2345,
    0x3086, 0x5825,
    0x3086, 0x3E10,
    0x3086, 0x8E01,
    0x3086, 0x2A98,
    0x3086, 0x8E00,
    0x3086, 0x3E10,
    0x3086, 0x8D60,
    0x3086, 0x1244,
    0x3086, 0x4BB9,
    0x3086, 0x2C2C,
    0x3086, 0x2C2C,    // SEQ_DATA_PORT
    //#endif
    
    0x301A, 0x10D8,    // RESET_REGISTER
    0x30B0, 0x1A38,    // DIGITAL_TEST
    0x31AC, 0x0C0C,    // DATA_FORMAT_BITS
    0x302A, 0x0008,    // VT_PIX_CLK_DIV
    0x302C, 0x0001,    // VT_SYS_CLK_DIV
//    0x302E, 0x0008,    // PRE_PLL_CLK_DIV , 74M
//    0x3030, 0x00C6,    // PLL_MULTIPLIER C6h=198
    0x302E, 0x0008,    // PRE_PLL_CLK_DIV , 74M
    0x3030, 0x00C6,    // PLL_MULTIPLIER C6h=198

    0x3036, 0x000C,    // OP_PIX_CLK_DIV
    0x3038, 0x0001,    // OP_SYS_CLK_DIV
    0x3002, 0x00B8,    // Y_ADDR_START
    0x3004, 0x014C,    // X_ADDR_START
    0x3006, 0x0387,    // Y_ADDR_END,         1088
    0x3008, 0x064B,    // X_ADDR_END,         1928
    //0x300A, 0x04BE,    // FRAME_LENGTH_LINES, VT,1214
    //0x300C, 0x03FC,    // LINE_LENGTH_PCK,    HT,1020
    0x300A, 0x04BE,    // FRAME_LENGTH_LINES, VT,1214
    0x300C, 0x03FC,    // LINE_LENGTH_PCK,    HT,1020
    0x3012, 0x0416,    // COARSE_INTEGRATION_TIME
    0x30A2, 0x0001,    // X_ODD_INC,subsampling
    0x30A6, 0x0001,    // Y_ODD_INC
    0x30AE, 0x0001,    // X_ODD_INC_CB
    0x30A8, 0x0001,    // Y_ODD_INC_CB
    0x3040, 0x0000,    // READ_MODE
    0x31AE, 0x0301,    // SERIAL_FORMAT
    0x3082, 0x0009,    // OPERATION_MODE_CTRL,ERS linear
    0x30BA, 0x760C,    // DIGITAL_CTRL
    0x3100, 0x0000,    // AECTRLREG
    0x3060, 0x000B,    // GAIN
    0x31D0, 0x0000,    // COMPANDING
    0x3064, 0x1802,    // SMIA_TEST
    0x3EEE, 0xA0AA,    // DAC_LD_34_35
    0x30BA, 0x762C,    // DIGITAL_CTRL
    0x3F4A, 0x0F70,    // DELTA_DK_PIX_THRES
    0x309E, 0x016C,    // HIDY_PROG_START_ADDR
    0x3092, 0x006F,    // ROW_NOISE_CONTROL
    0x3EE4, 0x9937,    // DAC_LD_24_25
    0x3EE6, 0x3863,    // DAC_LD_26_27
    0x3EEC, 0x3B0C,    // DAC_LD_32_33
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30BA, 0x762C,    // DIGITAL_CTRL
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    0x30B0, 0x0A3A,    // DIGITAL_TEST
    0x3EEA, 0x2838,    // DAC_LD_30_31
    0x3ECC, 0x4E2D,    // DAC_LD_0_1
    0x3ED2, 0xFEA6,    // DAC_LD_6_7
    0x3ED6, 0x2CB3,    // DAC_LD_10_11
    0x3EEA, 0x2819,    // DAC_LD_30_31
    0x30B0, 0x1A3A,    // DIGITAL_TEST
    //0x306E, 0x2418,    // DATAPATH_SELECT
    //0x306E, 0x2C18,    // DATAPATH_SELECT , 12:10 , PCLK slew rate
    0x306E, 0x9018,    // DATAPATH_SELECT , 12:10 , PCLK slew rate
    0x301A, 0x10DC,    // RESET_REGISTER,para,stream on
    //0x3070, 0x0003,    // TestPattern,
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
	MMP_USHORT 	i;
	MMP_ULONG 	ulSensorClkSrc;	
#if (SENSOR_IF == SENSOR_IF_PARALLEL)
	RTNA_DBG_Str0(FG_PURPLE("SNR_Cust_InitConfig AR0237 Parallel\r\n"));
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	RTNA_DBG_Str0(FG_PURPLE("SNR_Cust_InitConfig AR0237 MIPI 2-lane\r\n"));
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	RTNA_DBG_Str0(FG_PURPLE("SNR_Cust_InitConfig AR0237 MIPI 4-lane\r\n"));
#endif
	// Init OPR Table
    SensorCustFunc.OprTable->usInitSize                   			= (sizeof(SNR_AR0237_Reg_Init_Customer)/sizeof(SNR_AR0237_Reg_Init_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                 			= &SNR_AR0237_Reg_Init_Customer[0];    

    SensorCustFunc.OprTable->bBinTableExist                         = MMP_FALSE;
    SensorCustFunc.OprTable->bInitDoneTableExist                    = MMP_FALSE;

    for (i = 0; i < MAX_SENSOR_RES_MODE; i++)
    {
        SensorCustFunc.OprTable->usSize[i] 							= (sizeof(SNR_AR0237_Reg_Unsupport)/sizeof(SNR_AR0237_Reg_Unsupport[0]))/2;
        SensorCustFunc.OprTable->uspTable[i] 						= &SNR_AR0237_Reg_Unsupport[0];
    }

    // 16:9
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30FPS]        = (sizeof(SNR_AR0237_Reg_1920x1080_30P)/sizeof(SNR_AR0237_Reg_1920x1080_30P[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30FPS]      = &SNR_AR0237_Reg_1920x1080_30P[0];
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_25FPS]        = (sizeof(SNR_AR0237_Reg_1920x1080_30P)/sizeof(SNR_AR0237_Reg_1920x1080_30P[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_25FPS]      = &SNR_AR0237_Reg_1920x1080_30P[0];    
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_60FPS]        = (sizeof(SNR_AR0237_Reg_1920x1080_30P)/sizeof(SNR_AR0237_Reg_1920x1080_30P[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_60FPS]      = &SNR_AR0237_Reg_1920x1080_30P[0];        

    // 4:3

    // Init Vif Setting : Common
    SensorCustFunc.VifSetting->SnrType                              = MMPF_VIF_SNR_TYPE_BAYER;
#if (SENSOR_IF == SENSOR_IF_PARALLEL)
    SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_PARALLEL;
#elif (SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
	#if (PRM_SENSOR_VIF_ID == MMPF_VIF_MDL_ID1)
    SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_MIPI_SINGLE_1;//use for 2531project
	#else 	
    SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_MIPI_SINGLE_0;//use for HSG
	#endif
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	#if (PRM_SENSOR_VIF_ID == MMPF_VIF_MDL_ID1) //MIPI 2 lane
    SensorCustFunc.VifSetting->OutInterface                         = MMPF_VIF_IF_MIPI_DUAL_01;
	#else
	SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_MIPI_DUAL_01;
	#endif
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	SensorCustFunc.VifSetting->OutInterface							= MMPF_VIF_IF_MIPI_QUAD;
#endif

#if (PRM_SENSOR_VIF_ID == MMPF_VIF_MDL_ID1) //MIPI 2 lane or 1 lane
    SensorCustFunc.VifSetting->VifPadId = MMPF_VIF_MDL_ID1; 
#else 
    SensorCustFunc.VifSetting->VifPadId = MMPF_VIF_MDL_ID0; 
#endif     
    // Init Vif Setting : PowerOn Setting
 	/********************************************/
	// Power On serquence
	// 1. Supply Power
	// 2. Deactive RESET
	// 3. Enable MCLK
	// 4. Active RESET (1ms)
	// 5. Deactive RESET (Wait 150000 clock of MCLK, about 8.333ms under 24MHz)
	/********************************************/
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnExtPower 		= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin 		= MMP_GPIO_MAX; // it might be defined in Config_SDK.h
    SensorCustFunc.VifSetting->powerOnSetting.bExtPowerPinHigh 		= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPinDelay 	= 100; // 0 ms which is suggested by Kenny Shih @ 20150327
    SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh 		= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay 	= 100;
    SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh 		= MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay 		= 100;
    SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst = MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh 		= MMP_FALSE;
    SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay 	= 100;
    SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh 		= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay 	= 100;

    // Init Vif Setting : PowerOff Setting
    SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode 	= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg 	= 0x00;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask 	= 0x00;
    SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh 			= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay 		= 100;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock 		= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower 	= MMP_TRUE;
    SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin 		= MMP_GPIO_MAX; // it might be defined in Config_SDK.h

    // Init Vif Setting : Sensor MClock Setting
	MMPF_PLL_GetGroupFreq(CLK_GRP_SNR, &ulSensorClkSrc);
    SensorCustFunc.VifSetting->clockAttr.bClkOutEn 					= MMP_TRUE;
    SensorCustFunc.VifSetting->clockAttr.ubClkFreqDiv 				= 0;
    SensorCustFunc.VifSetting->clockAttr.ulMClkFreq 				= 24000;
    SensorCustFunc.VifSetting->clockAttr.ulDesiredFreq 				= 24000;
    SensorCustFunc.VifSetting->clockAttr.ubClkPhase 				= MMPF_VIF_SNR_PHASE_DELAY_NONE;
    SensorCustFunc.VifSetting->clockAttr.ubClkPolarity 				= MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->clockAttr.ubClkSrc 					= MMPF_VIF_SNR_CLK_SRC_PMCLK;

    // Init Vif Setting : Parallel Sensor Setting
    SensorCustFunc.VifSetting->paralAttr.ubLatchTiming 				= MMPF_VIF_SNR_LATCH_POS_EDGE;
    SensorCustFunc.VifSetting->paralAttr.ubHsyncPolarity 			= MMPF_VIF_SNR_CLK_POLARITY_POS;
	SensorCustFunc.VifSetting->paralAttr.ubVsyncPolarity 			= MMPF_VIF_SNR_CLK_POLARITY_POS;
    SensorCustFunc.VifSetting->paralAttr.ubBusBitMode               = MMPF_VIF_SNR_PARAL_BITMODE_10;

    // Init Vif Setting : MIPI Sensor Setting
    SensorCustFunc.VifSetting->mipiAttr.bClkDelayEn 				= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bClkLaneSwapEn 				= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.usClkDelay 					= 0;
	SensorCustFunc.VifSetting->mipiAttr.ubBClkLatchTiming 			= MMPF_VIF_SNR_LATCH_POS_EDGE;
#if (SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0] 				= MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1] 				= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2] 				= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3] 				= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0] 			= MMP_TRUE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2] 			= MMP_FALSE;
    SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3] 			= MMP_FALSE;
	#if (PRM_SENSOR_VIF_ID == MMPF_VIF_MDL_ID1)
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_1;//use for 2531project	
	#else
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_0;//use for HSG project
	#endif
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= 0x08;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= 0x1F;
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
#if (PRM_SENSOR_VIF_ID == MMPF_VIF_MDL_ID1)    
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]            = MMPF_VIF_MIPI_DATA_SRC_PHY_1;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]            = MMPF_VIF_MIPI_DATA_SRC_PHY_2;
#else
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0]            = MMPF_VIF_MIPI_DATA_SRC_PHY_0;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1]            = MMPF_VIF_MIPI_DATA_SRC_PHY_1;
#endif    
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3]            = MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0]              = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1]              = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2]              = 0x08;
    SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3]              = 0x08;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2]             = 0x1F;
    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3]             = 0x1F;
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_0;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_1;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_2;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_3;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= 0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= 0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= 0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= 0x08;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= 0x1F;
#endif

	// Init Vif Setting : Color ID Setting
    SensorCustFunc.VifSetting->colorId.VifColorId 					= MMPF_VIF_COLORID_01;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId 	= MMP_FALSE;
}

#if 1//	new extent node for18//LV1,		LV2,		LV3,		LV4,		LV5,		LV6,		LV7,		LV8,		LV9,		LV10,	LV11,	LV12,	LV13,	LV14,	LV15,	LV16 	LV17  	LV18
ISP_UINT32 AE_Bias_tbl[54] =
/*lux*/						{2,			5,			10,			26,			42, 		63, 		80, 		122, 		252,		458,	836,	1684,	3566,	6764,	13279,	27129,	54640, 108810
/*ENG*/						,0x2FFFFFF, 4841472*2,	3058720,	1962240,	1095560,  	616000, 	334880, 	181720,     96600,	 	52685,	27499,	14560,	8060,	4176,	2216,	1144,	600,   300
/*Tar*/						//,76,		76,		 	86,	        98,		    120,	 	136,	 	162,	 	174,	    190,	    208,	226,	243,	256,	256,	256,	256,	256,   256
///*Tar*/						,100,		106,		114,	    120,		126,	 	134,	 	142,	 	150,	    162,	    170,	190,	210,	220,	228,	234,	237,	238,   241
///*Tar*/ --NORMAL					,100,		114,		126,		134,		148,		160,		178,		186,		200,		210,	224,	236,	236,	236,	236,	237,	238,   241
	/*Tar*/ 					,110,		126,		134,		148,		154,		168,		178,		186,		200,		210,	224,	236,	236,	236,	236,	237,	238,   241

};
#define AE_tbl_size  (18)	//32  35  44  50
#endif

#define AR0237_MaxGain 40 // ~= 16 * 2.7x
//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
extern MMP_ULONG  m_ulISPFrameCount;
void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
	static ISP_UINT32	d_gain, s_gain;
	ISP_UINT32			AGAIN_1X = ISP_IF_AE_GetGainBase();
	ISP_UINT32			DGAIN_1X = 0x200;

    MMP_ULONG           m_ulSetShutCurISPFrameCnt;
    MMP_ULONG           m_ulSetGainDoneCurISPFrameCnt;

	if(ulFrameCnt < 2 || *(ISP_UINT8*) 0x800070c8 == 0) return;

	if (ulFrameCnt % 1000 == 10) {
		ISP_IF_F_SetWDREn(1);
		ISP_IF_CMD_SendCommandtoAE(0x51,AE_Bias_tbl,AE_tbl_size,0);	// <<AE table set once at preview start
		ISP_IF_NaturalAE_Enable(2);	//0: no , 1:ENERGY 2: Lux 3: test mode
		ISP_IF_CMD_SendCommandtoAE(0x52,0,0,1);
	}

	if ((ulFrameCnt % m_AeTime.ubPeriod) == 0)
	{
		#if (ISP_EN)
		ISP_IF_AE_Execute();

		s_gain = ISP_IF_AE_GetGain();

		if(s_gain >= AGAIN_1X * AR0237_MaxGain)
		{
			d_gain = DGAIN_1X * s_gain / (AGAIN_1X * AR0237_MaxGain);
			s_gain = AGAIN_1X * AR0237_MaxGain;
		}
		else
		{
			d_gain = DGAIN_1X;
		}

		m_ulSetShutCurISPFrameCnt = m_ulISPFrameCount;
		gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, 0, 0);
	    gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, s_gain);
        m_ulSetGainDoneCurISPFrameCnt = m_ulISPFrameCount;
        if( m_ulSetShutCurISPFrameCnt != m_ulSetGainDoneCurISPFrameCnt) {
            RTNA_DBG_Str0("Set AE and Shutter over ");
            RTNA_DBG_Byte0(m_ulSetGainDoneCurISPFrameCnt-m_ulSetShutCurISPFrameCnt);
            RTNA_DBG_Str0(" frame duration\r\n");
        }
		#endif
	}

	if ((ulFrameCnt % m_AeTime.ubPeriod) == 2)
		ISP_IF_IQ_SetAEGain(d_gain, DGAIN_1X);
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
    ISP_UINT32 sensor_again;
    ISP_UINT32 sensor_dgain;
    ISP_UINT32 CG_val;

    ulGain = ISP_MAX(ulGain * 0x40 / ISP_IF_AE_GetGainBase(), 0x60); //need > 1.5x

    if(ulGain >= 864) //if > 13.5x, use HCG
    {
        ulGain = ulGain * 10 / 27;
        CG_val = 4; //HCG
    }
    else
    {
        CG_val = 0; //LCG
    }

    // Sensor Gain Mapping
    if(ulGain < 0x80){
        sensor_dgain = (ulGain << 2) / 3;
        sensor_again = 0xB;     //AGain need > 1.5x
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
    }      

    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x305E, sensor_dgain * 133 / 128); //need > 0x85 (1.04x)
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3060, sensor_again);
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x3100, CG_val);
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
	new_shutter = ISP_MIN(ISP_MAX(new_shutter, 8), new_vsync - 3);

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

void SNR_Cust_Switch3ASpeed(MMP_BOOL slow)
{
  MMPF_SENSOR_CUSTOMER *cust = (MMPF_SENSOR_CUSTOMER *)&SensorCustFunc ;
  printc("[TBD] : Switch to slow 3A\r\n");
  if(slow) {
    
  }
  else {
    
  }
}

const MMP_UBYTE* SNR_Cust_GetIqCompressData(MMP_UBYTE ubSnrSel)
{
    return s_IqCompressData;
}

void SNR_Cust_SetNightVision(MMP_BOOL night)
{
  MMPF_SENSOR_CUSTOMER *cust = (MMPF_SENSOR_CUSTOMER *)&SensorCustFunc ;
  printc("[TBD] : NV_mode:%d\r\n", night);
  if(night) {
    ISP_IF_IQ_SetSysMode(0);
    //if(ISP_IF_F_GetWDREn() != 0) ISP_IF_F_SetWDREn(0);
    ISP_IF_F_SetImageEffect(ISP_IMAGE_EFFECT_GREY);  // Grey
  }
  else {
    ISP_IF_IQ_SetSysMode(1);
    //if(ISP_IF_F_GetWDREn() != 1) ISP_IF_F_SetWDREn(1);
    ISP_IF_F_SetImageEffect(ISP_IMAGE_EFFECT_NORMAL);  // Grey    
  }
}

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
    SNR_Cust_SetNightVision,

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

/* #pragma arm section code = "initcall6", rodata = "initcall6", rwdata = "initcall6", zidata = "initcall6" */
/* #pragma O0 */
ait_module_init(SNR_Module_Init);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

#endif  //BIND_SENSOR_AR0237
#endif	//SENSOR_EN
