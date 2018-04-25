//==============================================================================
//
//  File        : sensor_JXK03.c
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

#define IPC_I2C_TEST  (1)
#if IPC_I2C_TEST==1
#include "dualcpu_i2c.h"
#include "mmpf_i2cm.h"
#endif

#if (SENSOR_EN)
#if (BIND_SENSOR_JXK03)

#include "mmpf_sensor.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"
#include "sensor_jxk03.h"
//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

#define IQ_TABLE "isp_8428_iq_data_v3_JXF03_ezmode_20180111.xls.ciq.txt"

// Resolution Table
MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	2,				/* ubSensorModeNum */
	0,				/* ubDefPreviewMode */
	0,				/* ubDefCaptureMode */
	3000,			// usPixelSize
//  Mode0   Mode1
    {1,     1},	/* usVifGrabStX */
    {1,     1},	/* usVifGrabStY */
    {2568,  2592},	/* usVifGrabW */
    {1928,  1944},	/* usVifGrabH */
    #if (CHIP == MCR_V2)
    {1,     1}, 	// usBayerInGrabX
    {1,     1},    // usBayerInGrabY
    {8,     8},  /* usBayerDummyInX */
    {8,     8},  /* usBayerDummyInY */
    {2560,  1920},	/* usBayerOutW */
    {1920,  1080},	/* usBayerOutH */
    #endif
    {2560,  1920},	/* usScalInputW */
    {1920,  1080},	/* usScalInputH */
    {300,   600},	/* usTargetFpsx10 */
    {2001,  2001},	/* usVsyncLine */  //(Reg 0x22 0x23) + 1
    {1,     1},  /* ubWBinningN */
    {1,     1},  /* ubWBinningN */
    {1,     1},  /* ubWBinningN */
    {1,     1},  /* ubWBinningN */
    {1,     0},    	// ubCustIQmode
    {1,     0}     	// ubCustAEmode  
};

// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE 	m_OprTable;
MMPF_SENSOR_VIF_SETTING m_VifSetting;

//lv1-7, 2	3	6	11	17	30	60
#if 1//	new extent node for18//LV1,		LV2,		LV3,		LV4,		LV5,		LV6,		LV7,		LV8,		LV9,		LV10,	LV11,	LV12,	LV13,	LV14,	LV15,	LV16 	LV17  	LV18
//abby curve iq 12
ISP_UINT32 AE_Bias_tbl[54] =
/*lux*/						{1,			2,			3,			6,			10,			17, 		26, 		54, 		101, 		206,	407,	826,	1638,	3277,	6675,	13554,	27329,	54961/*930000=LV17*/ //with  1203
/*ENG*/						,0x2FFFFFF, 4841472*2,	3058720,	1962240,	1095560,  	616000, 	334880, 	181720,     96600,	 	52685,	27499,	14560,	8060,	4176,	2216,	1144,	600,   300
/*Tar*/			//			,55,		60,		 	65,	        90,			110, 		122,	 	180,	 	200,	    205,	    210,	215,	220,	225,	230,	235,	240,	250,   250 //with max 15x 1202V1
/*Tar*/			//			,42,		48,		 	55,	        60,			75, 		90,	 		130,	 	180,	    220,	    240,	242,	244,	246,	247,	248,	248,	249,   250 //with max 13x 1202V2
/*Tar*/			//			,30,		40,			45,		 	52,	        60,			75, 		90,	 		150,	 	200,	    220,	240,	242,	244,	246,	247,	248,	248,	249 //with max 11x 1203V1
/*Tar*/						,30,		40,			45,		 	52,	        65,			80, 	   100,	 		150,	 	200,	    220,	240,	255,	265,	268,	270,	274,	277,	280 //with max 11x 1207

 };	
#define AE_tbl_size  (18)	//32  35  44  50
#endif


// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
//#include "isp_8428_iq_data_v2_JXK03_20160629_5FPS.xls.ciq.txt"
#if SUPPORT_UVC_ISP_EZMODE_FUNC==1
// improve low lux in HDK sensor
	#include IQ_TABLE
#else
	//#include "isp_8428_iq_data_v2_JXF02_20160505_Daniel.xls.ciq.txt"
    #include IQ_TABLE
#endif
};

#if (SUPPORT_UVC_ISP_EZMODE_FUNC==1) //TBD
const  ISP_UINT8 Sensor_EZ_IQ_CompressedText[] =
{
#include "eziq_0509.txt"
};

ISP_UINT32 eziqsize = sizeof(Sensor_EZ_IQ_CompressedText);
#endif

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = 
{
	MMP_I2CM0,      // i2cmID
//    0x8C>>1,       	// ubSlaveAddr
    0x80>>1,       	// ubSlaveAddr
	8, 			    // ubRegLen
	8, 				// ubDataLena4
	0, 				// ubDelayTime
	MMP_FALSE, 		// bDelayWaitEn
	MMP_TRUE, 		// bInputFilterEn
	MMP_FALSE, 		// b10BitModeEn
	MMP_FALSE, 		// bClkStretchEn
	0, 				// ubSlaveAddr1
	0, 				// ubDelayCycle
	0, 				// ubPadNum
	150000, 		// ulI2cmSpeed 150KHZ
	MMP_TRUE, 		// bOsProtectEn
	0, 			// sw_clk_pin
	0, 			// sw_data_pin
	MMP_FALSE, 		// bRfclModeEn
	MMP_FALSE,      // bWfclModeEn
	MMP_FALSE,		// bRepeatModeEn
    0               // ubVifPioMdlId
};

#if IPC_I2C_TEST==1
static int JXK03_probe(void *driver_data);
static int JXK03_read(void *driver_data, unsigned short addr, unsigned short *data);
static int JXK03_write(void *driver_data, unsigned short addr, unsigned short data);

i2c_device_t JXK03_i2c =
{
  .id   = -1 ,
  .name  = "JXK03" , 
  .probe = JXK03_probe ,
  .read  = JXK03_read ,
  .write = JXK03_write,
  .driver_data = &m_I2cmAttr ,
};


static int JXK03_probe(void *driver_data)
{
  MMP_I2CM_ATTR *attr = (MMP_I2CM_ATTR *)driver_data ;
  if(!attr) {
    return -1 ;
  }
  JXK03_i2c.slave_addr = attr->ubSlaveAddr ;
  JXK03_i2c.addr_len = attr->ubRegLen ;
  JXK03_i2c.data_len = attr->ubDataLen ;
  return 0;   
}

static int JXK03_read(void *driver_data, unsigned short addr, unsigned short *data)
{
  MMP_ERR err ;
  err = MMPF_I2cm_ReadReg((MMP_I2CM_ATTR *)driver_data,addr,data) ;
  if(!err) {
    return 0;
  }
  return -1 ;
}

static int JXK03_write(void *driver_data, unsigned short addr, unsigned short data)
{
  MMP_ERR err ;
  err = MMPF_I2cm_WriteReg((MMP_I2CM_ATTR *)driver_data,addr,data) ;
  if(!err) {
    return 0;
  }
  return -1 ;
}




#endif









// 3A Timing
MMPF_SENSOR_AWBTIMIMG m_AwbTimeSlow = 
{
	6, 	// ubPeriod
	1, 	// ubDoAWBFrmCnt
	2 	// ubDoCaliFrmCnt
};

MMPF_SENSOR_AETIMIMG m_AeTimeSlow = 
{
	6, 	// ubPeriod
	1, 	// ubFrmStSetShutFrmCnt
	0 	// ubFrmStSetGainFrmCnt
};

MMPF_SENSOR_AFTIMIMG m_AfTimeSlow = 
{
	1, 	// ubPeriod
	0 	// ubDoAFFrmCnt
};

MMPF_SENSOR_AWBTIMIMG m_AwbTime = 
{
	2, 	// ubPeriod
	0, 	// ubDoAWBFrmCnt
	2 	// ubDoCaliFrmCnt
};

MMPF_SENSOR_AETIMIMG m_AeTime = 
{
	3, 	// ubPeriod
	0, 	// ubFrmStSetShutFrmCnt
	0 	// ubFrmStSetGainFrmCnt
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


ISP_UINT16 SNR_JXK03_Reg_Init_Customer[] =
{
	// TBD
    // [7]: software reset, [6]: sleep mode, [5]:mirror,
    // [4]: flip, [3]: HDR mode
	0x12, 0x80,
};

//2560_1920
MMP_USHORT SNR_JXK03_Reg_2560x1920__30FPS[] =
{
    0x12, 0x40,
    0x39, 0x41,
    0x39, 0x01,
    0x0E, 0x10,
    0x0F, 0x08,
    0x10, 0x24,
    0x11, 0x80,
    0x0D, 0x53,
    0x70, 0x69,
    0x0C, 0x00,
    0x5F, 0x01,
    0x60, 0x12,
    0x7B, 0x0C,
    0x7C, 0x2F,
    0xAD, 0x44,
    0xAA, 0x4C,
    0x20, 0xD0,
    0x21, 0x02,
    0x22, 0xD0,
    0x23, 0x07,
    0x24, 0xA0,
    0x25, 0x98,
    0x26, 0x72,
    0x27, 0x03,
    0x28, 0x23,
    0x29, 0x02,
    0x2A, 0xF0,
    0x2B, 0x21,
    0x2C, 0x00,
    0x2D, 0x00,
    0x2E, 0xEF,
    0x2F, 0x94,
    0x30, 0xA4,
    0x76, 0x80,
    0x77, 0x0A,
    0x87, 0xB7,
    0x88, 0x0F,
    0x1D, 0x00,
    0x1E, 0x04,
    0x6C, 0x60,
    0x71, 0x4E,
    0x72, 0x21,
    0x73, 0xA4,
    0x74, 0x86,
    0x75, 0x02,
    0x78, 0x94,
    0xB0, 0x08,
    0x6B, 0x10,
    0x32, 0x0C,
    0x33, 0x0C,
    0x34, 0x13,
    0x35, 0x87,
    0x3E, 0xB8,
    0x62, 0xC4,
    0x79, 0x90,
    0x7A, 0x32,
    0x7D, 0x81,
    0x92, 0x64,
    0x7E, 0x0C,
    0xAE, 0x0F,
    0x8C, 0x80,
    0x58, 0x90,
    0x5B, 0x57,
    0x5D, 0x2D,
    0x5E, 0x88,
    0x66, 0x04,
    0x67, 0x38,
    0x68, 0x00,
    0x6A, 0x3F,
    0x91, 0x56,
    0xAB, 0x6C,
    0xAF, 0x44,
    0x80, 0x80,
    0x48, 0x81,
    0x81, 0x54,
    0x84, 0x04,
    0x49, 0x10,
    0x85, 0x80,
    0x82, 0x0F,
    0x83, 0x0A,
    0x64, 0x14,
    0x8D, 0x20,
    0x63, 0x00,
    0x89, 0x00,
    0x13, 0x30,
    0x12, 0x00,
    0x39, 0x41,
    0x39, 0x01,
};


// 1080p 30FPS
MMP_USHORT SNR_JXK03_Reg_1920x1080_Customer[] = 
{
#if (SENSOR_IF == SENSOR_IF_MIPI_1_LANE || SENSOR_IF == SENSOR_IF_PARALLEL)
    //Not support
    0x0, 0x0,
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    //Not support
    0x0, 0x0,
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
    /*
    [K03#08_2592x1944x30fps_4L_MIPI-20171017]
    InitREGdef=INI_Register
    IniINFOdef=INI_Category
    IniVersion=20170517
    Project=31	;K03
    Width=2592
    Height=1944
    disWidth=2592
    disHeight=1944
    FrameWidth=2880
    FrameHeight=2000
    H_pad=0
    V_pad=0
    SrOutputFormats=1
    Interface=4
    HDR_Mode=0
    MclkRate=24
    DVPClkRate=173		;pixel rate = 172.80000
    MipiClkRate=216
    */
    0x12, 0x40,
    0x39, 0x41,
    0x39, 0x01,
    0x0E, 0x10,
    0x0F, 0x08,
    0x10, 0x24,
    0x11, 0x80,
    0x0D, 0x53,
    0x70, 0x69,
    0x0C, 0x00,
    0x5F, 0x01,
    0x60, 0x11,
    0x7B, 0x0C,
    0x7C, 0x2F,
    0xAD, 0x44,
    0xAA, 0x4C,
    0x20, 0xD0,
    0x21, 0x02,
    0x22, 0xD0,
    0x23, 0x07,
    0x24, 0x88,
    0x25, 0x98,
    0x26, 0x72,
    0x27, 0xF0,
    0x28, 0x23,
    0x29, 0x01,
    0x2A, 0xDE,
    0x2B, 0x21,
    0x2C, 0x0C,
    0x2D, 0x00,
    0x2E, 0xEF,
    0x2F, 0x94,
    0x30, 0x92,
    0x76, 0x20,
    0x77, 0x0A,
    0x87, 0xB7,
    0x88, 0x0F,
    0x1D, 0x00,
    0x1E, 0x04,
    0x6C, 0x60,
    0x71, 0x4E,
    0x72, 0x21,
    0x73, 0xA4,
    0x74, 0x86,
    0x75, 0x02,
    0x78, 0x94,
    0xB0, 0x08,
    0x6B, 0x10,
    0x32, 0x0C,
    0x33, 0x14,
    0x34, 0x13,
    0x35, 0x87,
    0x62, 0xC4,
    0x79, 0x90,
    0x7A, 0x32,
    0x7D, 0x81,
    0x7E, 0x0C,
    0xAE, 0x0F,
    0x58, 0x90,
    0x5B, 0x57,
    0x5D, 0x2F,
    0x5E, 0x83,
    0x66, 0x04,
    0x67, 0x38,
    0x68, 0x00,
    0x6A, 0x4F,
    0x91, 0x56,
    0xAB, 0x6C,
    0xAF, 0x04,
    0x80, 0x80,
    0x48, 0x81,
    0x81, 0x54,
    0x84, 0x04,
    0x49, 0x10,
    0x85, 0x80,
    0x82, 0x0F,
    0x83, 0x0A,
    0x64, 0x14,
    0x8D, 0x20,
    0x63, 0x00,
    0x89, 0x00,
    0x13, 0x30,
    #if (JXK03_TEST_PATTERN)
    0x95, 0x02,
    0xA8, 0x3F, //0x3F or 2F (H ov V color bar)
    #endif
    0x12, 0x00,
    0x39, 0x41,
    0x39, 0x01
#endif

};

// 1080p 60FPS
MMP_USHORT SNR_JXK03_Reg_1920x1080_60_Customer[] = 
{
#if (SENSOR_IF == SENSOR_IF_MIPI_1_LANE || SENSOR_IF == SENSOR_IF_PARALLEL)
    //Not support
    0x0, 0x0,
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    //Not support
    0x0, 0x0,
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
    //Not support
    0x0, 0x0,
#endif
};


//------------------------------------------------------------------------------
//  Function    : SNR_Cust_InitConfig
//  Description :
//------------------------------------------------------------------------------
static void SNR_Cust_InitConfig(void)
{
#if MIPI_TEST_PARAM_EN
    extern void UI_GetMIPIParamters(MMP_UBYTE *sot,MMP_UBYTE *delay,MMP_UBYTE *edge) ;
#endif  
    MMP_UBYTE new_sot = 0x17 ,new_delay = 0x08 ,new_edge =  MMPF_VIF_SNR_LATCH_NEG_EDGE ;
	MMP_USHORT 	i;
#if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)||(SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    MMP_ULONG ulFreq, ulSot;
#endif

    printc("IQ_TABLE %s\r\n", IQ_TABLE);
	//MMP_ULONG 	ulSensorMCLK = 27000; // 27 M
#if IPC_I2C_TEST==1
    I2C_Register(&JXK03_i2c);  
#endif

#if (SENSOR_IF == SENSOR_IF_PARALLEL)
	RTNA_DBG_Str(0, "JXK03 Parallel\r\n");
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    RTNA_DBG_Str(0, FG_PURPLE("JXK03 MIPI 2-lane\r\n"));
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	RTNA_DBG_Str(0, "JXK03 MIPI 4-lane\r\n");
#endif
#if MIPI_TEST_PARAM_EN
    UI_GetMIPIParamters(&new_sot,&new_delay,&new_edge);
    printc("#New MIPI parameters,SOT:%d,Delay:%d,Latch Edge:%d\r\n",new_sot,new_delay,new_edge);
#endif
	// Init OPR Table
    SensorCustFunc.OprTable->usInitSize                             = (sizeof(SNR_JXK03_Reg_Init_Customer)/sizeof(SNR_JXK03_Reg_Init_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                           = &SNR_JXK03_Reg_Init_Customer[0];    

    //SensorCustFunc.OprTable->bBinTableExist                         = MMP_FALSE;
    //SensorCustFunc.OprTable->bInitDoneTableExist                    = MMP_FALSE;

    //SensorCustFunc.OprTable->usSize[RES_IDX_2560_1920]    = (sizeof(SNR_JXK03_Reg_1920x1080_Customer)/sizeof(SNR_JXK03_Reg_1920x1080_Customer[0]))/2;
    //SensorCustFunc.OprTable->uspTable[RES_IDX_2560_1920]  = &SNR_JXK03_Reg_1920x1080_Customer[0];
	SensorCustFunc.OprTable->usSize[RES_IDX_2560_1920]    = (sizeof(SNR_JXK03_Reg_2560x1920__30FPS)/sizeof(SNR_JXK03_Reg_2560x1920__30FPS[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_2560_1920]  = &SNR_JXK03_Reg_2560x1920__30FPS[0];

	// Init Vif Setting : Common
    SensorCustFunc.VifSetting->SnrType                              = MMPF_VIF_SNR_TYPE_BAYER;
#if (SENSOR_IF == SENSOR_IF_PARALLEL)
	SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_PARALLEL;
#elif (SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
	SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_MIPI_SINGLE_0;
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	SensorCustFunc.VifSetting->OutInterface 						= MMPF_VIF_IF_MIPI_DUAL_01;
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	SensorCustFunc.VifSetting->OutInterface							= MMPF_VIF_IF_MIPI_QUAD;
#endif

#if 1
	SensorCustFunc.VifSetting->VifPadId 							= MMPF_VIF_MDL_ID0;
#else
	SensorCustFunc.VifSetting->VifPadId 							= MMPF_VIF_MDL_ID1;
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
	SensorCustFunc.VifSetting->powerOnSetting.usExtPowerPin 		= MMP_GPIO_MAX;
	SensorCustFunc.VifSetting->powerOnSetting.bFirstEnPinHigh 		= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay 	= 10;//10;
	SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh 		= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay 		= 10;//100;
	SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst = MMP_TRUE;
	SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh 		= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay 	= 10;//100;
	SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh 		= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay 	= 20;//100;

	// Init Vif Setting : PowerOff Setting
	SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode 	= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg 	= 0x100;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask 	= 0x00;
	SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh 			= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay 		= 20;//20;
	SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock 		= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower 	= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOffSetting.usExtPowerPin 		= MMP_GPIO_MAX; // it might be defined in Config_SDK.h

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
	SensorCustFunc.VifSetting->mipiAttr.ubBClkLatchTiming 			= new_edge ;// MMPF_VIF_SNR_LATCH_NEG_EDGE;
	
#if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)||(SENSOR_IF == SENSOR_IF_MIPI_2_LANE)||(SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
    //MMPF_PLL_GetGroupFreq(CLK_GRP_SNR, &ulFreq);
    ulSot =  0x1F;
#endif
	
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
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_0;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= new_delay ;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= new_sot  ;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= new_sot  ;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= new_sot  ;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= new_sot  ;
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[0] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[1] 				= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[2] 				= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneEn[3] 				= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[0] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[1] 			= MMP_TRUE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[2] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataDelayEn[3] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[0] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[1] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[2] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.bDataLaneSwapEn[3] 			= MMP_FALSE;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[0] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_0;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[1] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_1;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[2] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.ubDataLaneSrc[3] 			= MMPF_VIF_MIPI_DATA_SRC_PHY_UNDEF;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= new_delay; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= new_delay; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= new_delay; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= new_delay; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= new_sot;
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
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= new_delay;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= new_sot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= new_sot;
#endif

    // Init Vif Setting : Color ID Setting
    SensorCustFunc.VifSetting->colorId.VifColorId              		  = MMPF_VIF_COLORID_11; 
    
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId  	  = MMP_TRUE; //MMP_FALSE;

    for (i = 0; i < MAX_SENSOR_RES_MODE; i++)
    {
        SensorCustFunc.VifSetting->colorId.CustomColorId.Rot0d_Id[i]   = MMPF_VIF_COLORID_11;
        SensorCustFunc.VifSetting->colorId.CustomColorId.Rot90d_Id[i]  = MMPF_VIF_COLORID_UNDEF;
        SensorCustFunc.VifSetting->colorId.CustomColorId.Rot180d_Id[i] = MMPF_VIF_COLORID_00;
        SensorCustFunc.VifSetting->colorId.CustomColorId.Rot270d_Id[i] = MMPF_VIF_COLORID_UNDEF;
        SensorCustFunc.VifSetting->colorId.CustomColorId.H_Flip_Id[i]  = MMPF_VIF_COLORID_10;
        SensorCustFunc.VifSetting->colorId.CustomColorId.V_Flip_Id[i]  = MMPF_VIF_COLORID_10;
        SensorCustFunc.VifSetting->colorId.CustomColorId.HV_Flip_Id[i] = MMPF_VIF_COLORID_00;
    }

	/*
	printc("SOT [%d,%d,%d,%d],SNR.CLK:%d\r\n", SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0],
                                    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1],
                                	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2],
                                	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3],
                                	ulFreq ); 
    */                                	
}


//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
#define	DGAIN_BASE	0x200
ISP_UINT32	Dgain = DGAIN_BASE;
void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
#if (ISP_EN)  //TBD.........
    MMP_ULONG   ulVsync = 0;
    MMP_ULONG   ulShutter = 0;
	MMP_UBYTE   ubPeriod              = (SensorCustFunc.pAeTime)->ubPeriod;
	MMP_UBYTE   ubFrmStSetShutFrmCnt  = (SensorCustFunc.pAeTime)->ubFrmStSetShutFrmCnt;	
	MMP_UBYTE   ubFrmStSetGainFrmCnt  = (SensorCustFunc.pAeTime)->ubFrmStSetGainFrmCnt;
	
	if((*(MMP_UBYTE*) 0x800070C8 ) == 0) return;

	if(ulFrameCnt % 300 == 0) ISP_IF_AE_SetFPS(30);

    if(ulFrameCnt % ubPeriod == ubFrmStSetShutFrmCnt)
    {
        ISP_IF_AE_Execute();
        gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, 0, 0);
    }
    if(ulFrameCnt % ubPeriod == ubFrmStSetGainFrmCnt)
    {
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
#if 1  //TBD.........
	MMP_UBYTE   ubPeriod              = (SensorCustFunc.pAeTime)->ubPeriod;
	MMP_UBYTE   ubFrmStSetGainFrmCnt  = (SensorCustFunc.pAeTime)->ubFrmStSetGainFrmCnt;
	
	//if(ulFrameCnt % ubPeriod == ubFrmStSetGainFrmCnt) {
	if(ulFrameCnt % ubPeriod == 1) {
		ISP_IF_IQ_SetAEGain(Dgain, DGAIN_BASE);

		//ISP_IF_F_SetImageEffect(ISP_IMAGE_EFFECT_NORMAL);
	}
	// TBD
#endif

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
MMP_USHORT sensor_rg5C = 0xFF, sensor_rg67 = 0xFF;
MMP_USHORT sensor_rg8C = 0xFF, sensor_rgAF = 0xFF;
void SNR_Cust_SetGain(MMP_UBYTE ubSnrSel, MMP_ULONG ulGain)
{
	ISP_UINT16 s_gain, gainbase;
	ISP_UINT32 H, L;
	ISP_UINT32 set_gain = 16;

	ulGain = ISP_IF_AE_GetGain();
	gainbase = ISP_IF_AE_GetGainBase();

	if (ulGain >= (ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN))
	{
		Dgain 	= DGAIN_BASE * ulGain / (ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN);
		ulGain  = ISP_IF_AE_GetGainBase() * MAX_SENSOR_GAIN;
	}else
	{
		Dgain	= DGAIN_BASE;
	}

	s_gain = ISP_MIN(ISP_MAX(ulGain, gainbase), gainbase * 16 - 1); // API input gain range : 64~511, 64=1X

	if (s_gain >= gainbase * 8)
	{
		H = 3;
		L = s_gain * 16 / gainbase / 8 - 16;
		set_gain = 8 * (16+L) * gainbase / 16; 
	}
	else if (s_gain >= gainbase * 4)
	{
		H = 2;
		L = s_gain * 16 / gainbase / 4 - 16;
		set_gain = 4*(16+L) * gainbase / 16;  
	}
	else if (s_gain >= gainbase * 2)
	{
		H = 1;
		L = s_gain * 16 / gainbase / 2 - 16;
		set_gain = 2*(16+L) * gainbase / 16;
	}
	else
	{
		H = 0;
		L = s_gain * 16 / gainbase / (H + 1) - 16;
		set_gain = 1*(16+L) * gainbase / 16; 
	}
	if (L > 15) L = 15;
	Dgain = ulGain * Dgain / set_gain;

	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x5C, &sensor_rg5C);
	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x67, &sensor_rg67);
	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x8C, &sensor_rg8C);
	gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0xAF, &sensor_rgAF);

//	if(((H<<4)+L) < 16)
//	{	
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x5C, sensor_rg5C | 0x40);		
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x67, sensor_rg67 | 0x40);		
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x8C, sensor_rg8C | 0x03);		
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xAF, sensor_rgAF | 0xC0);
//	}
//	else
//	{
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x5C, sensor_rg5C & 0xBF);
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x67, sensor_rg67 & 0xBF);
//		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x8C, sensor_rg8C & 0xFC);
//	}

	//dbg_printf(3,"EV:%x lux:%x again:%x GetGain():%x \r\n",ISP_IF_AE_GetDbgData(0), ISP_IF_AE_GetDbgData(1), (H<<4)+L, ISP_IF_AE_GetGain());

	// set sensor gain
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x00, (H << 4) + L); //Total gain = (2^PGA[6:4])*(1+PGA[3:0]/16) //PGA:0x00
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetShutter
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG shutter, MMP_ULONG vsync)
{ 
	ISP_UINT32 new_vsync;
	ISP_UINT32 new_shutter;

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

	new_vsync	= ISP_MIN(ISP_MAX(new_shutter + 5, new_vsync), 0xFFFF);
	new_shutter = ISP_MIN(ISP_MAX(new_shutter, 1), new_vsync - 5);

	//dbg_printf(0, "Shutter:%d Vsync:%d GetV:%d VBase:%d GetS:%d SBase:%d \r\n",new_shutter,new_vsync, ISP_IF_AE_GetVsync(), ISP_IF_AE_GetVsyncBase(), ISP_IF_AE_GetShutter(), ISP_IF_AE_GetShutterBase());
	//dbg_printf(0, "GetFPS()%d GetPos():%d \r\n", ISP_IF_AE_GetFPS(), ISP_IF_AF_GetPos(10));

	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x23, new_vsync >> 8);
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x22, new_vsync);
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x02, (ISP_UINT8)((new_shutter >> 8) & 0xff));
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x01, (ISP_UINT8)(new_shutter & 0xff));
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetExposure
//  Description :
//------------------------------------------------------------------------------
// defined but not used
/* static void SNR_Cust_SetExposure(MMP_UBYTE ubSnrSel, MMP_ULONG ulGain, MMP_ULONG shutter, MMP_ULONG vsync) */
/* { */
/* #if (ISP_EN) */
    /* // TBD */
/* #endif */
/* } */

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetFlip
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetFlip(MMP_UBYTE ubSnrSel, MMP_UBYTE ubMode)
{
	printc("SNR_Cust_SetFlip %d \r\n", ubMode);
	if (ubMode == MMPF_SENSOR_NO_FLIP)
	{
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x12, 0x00);  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
	}else if (ubMode == MMPF_SENSOR_COLUMN_FLIP)
	{
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x12, 0x20);  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
	
	}else if (ubMode == MMPF_SENSOR_ROW_FLIP)
	{
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x12, 0x10);  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
	
	}else if (ubMode == MMPF_SENSOR_COLROW_FLIP)
	{
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel,0x12, 0x30);  // [7]: software reset, [6]: sleep mode, [5]:mirror, [4]: flip, [3]: HDR mode
	
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
	// TBD
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_StreamEnable
//  Description : Enable/Disable streaming of sensor
//------------------------------------------------------------------------------
//defined but not used
/* static void SNR_Cust_StreamEnable(MMP_UBYTE ubSnrSel, MMP_BOOL bEnable) */
/* { */
    /* // TBD */
/* } */



void SNR_Cust_Switch3ASpeed(MMP_BOOL slow)
{
}

void SNR_Cust_SetNightVision(MMP_BOOL night)
{
}

void SNR_Cust_SetTest(MMP_UBYTE effect_mode)
{
    printc("%s effect_mode %d\r\n", __func__, effect_mode);
    ISP_IF_F_SetImageEffect(effect_mode);
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
    SNR_Cust_SetTest,
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

#endif  //BIND_SENSOR_JXK03
#endif	//SENSOR_EN
