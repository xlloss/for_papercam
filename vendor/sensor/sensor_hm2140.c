//==============================================================================
//
//  File        : sensor_hm2140.c
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
#if (BIND_SENSOR_HM2140)

#include "mmpf_sensor.h"
#include "sensor_Mod_Remapping.h"
#include "isp_if.h"

//#define LOW_POWER_MODE    (1) // lower 35mW

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

// Resolution Table
MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	2,   // ubSensorModeNum
	0,    // ubDefPreviewMode
	0,    // ubDefCaptureMode
	3000, // usPixelSize
// 	Mode0   Mode1   Mode2	Mode3	Mode4   Mode5   Mode6   Mode7   Mode8   Mode9   Mode10  Mode11	Mode12	Mode13
    {1,		1		},    	// usVifGrabStX
    {1, 	1		},    	// usVifGrabStY 
    {1928,	1928	}, 	// usVifGrabW -> WOI_HSIZE(0xA9,0xAA)
    {1088,	1088    }, 	// usVifGrabH -> WOI_VSIZE(0xA5,0xA6)
/* #if (CHIP == MCR_V2) */
    {1,		1       },    	// usBayerInGrabX
    {1,		1       },    	// usBayerInGrabY
    {8,		8       },   	// usBayerInDummyX
    {8,		8       },    	// usBayerInDummyY
    {1920,	1280    },	// usBayerOutW
    {1080,	 720    }, 	// usBayerOutH
/* #endif */
    {1920,	1280    }, 	// usScalInputW
    {1080,	720    }, 	// usScalInputH
    {300,	300     }, 	// usTargetFpsx10
    {1110,	1110	},	// usVsyncLine -> LPF(0x0A,0x0B)
    {1,		1		},    	// ubWBinningN
    {1,		1		},    	// ubWBinningM
    {1,		1		},    	// ubHBinningN
    {1,		1		},     // ubHBinningM
    {0x00,	0x00	},  // ubCustIQmode
    {0x00,	0x00    }   // ubCustAEmode
};

// OPR Table and Vif Setting
MMPF_SENSOR_OPR_TABLE 	m_OprTable;
MMPF_SENSOR_VIF_SETTING m_VifSetting;

// IQ Table
const ISP_UINT8 Sensor_IQ_CompressedText[] = 
{
#include "isp_8428_iq_data_v3_HM2140_ezmode.xls.ciq.txt"
};

#if (SUPPORT_UVC_ISP_EZMODE_FUNC==1) //TBD
const  ISP_UINT8 Sensor_EZ_IQ_CompressedText[] =
{
//#include "eziq_0509.txt"
0
};

ISP_UINT32 eziqsize = sizeof(Sensor_EZ_IQ_CompressedText);
#endif

// I2cm Attribute
static MMP_I2CM_ATTR m_I2cmAttr = 
{
	MMP_I2CM0,      // i2cmID
        0x24,       	// ubSlaveAddr
	16, 			    // ubRegLen
	8, 				// ubDataLena4
	0, 				// ubDelayTime
	MMP_FALSE, 		// bDelayWaitEn
	MMP_TRUE, 		// bInputFilterEn
	MMP_FALSE, 		// b10BitModeEn
	MMP_FALSE, 		// bClkStretchEn
	0, 				// ubSlaveAddr1
	0, 				// ubDelayCycle
	0, 				// ubPadNum
	400000, 		// ulI2cmSpeed 150KHZ
	MMP_TRUE, 		// bOsProtectEn
	0, 			// sw_clk_pin
	0, 			// sw_data_pin
	MMP_FALSE, 		// bRfclModeEn
	MMP_FALSE,      // bWfclModeEn
	MMP_FALSE,		// bRepeatModeEn
    0               // ubVifPioMdlId
};

// 3A Timing
MMPF_SENSOR_AWBTIMIMG m_AwbTimeSlow = 
{
	6, 	// ubPeriod
	1, 	// ubDoAWBFrmCnt
	3 	// ubDoCaliFrmCnt
};

MMPF_SENSOR_AETIMIMG m_AeTimeSlow = 
{
	4, 	// ubPeriod
	0, 	// ubFrmStSetShutFrmCnt
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




const ISP_UINT16 HM2140_GainTable[] = {
	4096, //1x
	3855, 
	3641, 
	3449, 
	3277,
  3121, 
  2979, 
  2849,
  2731, 
  2621,
  2521, 
  2427, 
  2341, 
  2260, 
  2185, 
  2114, 
  2048, //2x
  1928, 
  1820, 
  1725, 
  1638, 
  1560,
  1489, 
  1425, 
  1365, 
  1311, 
  1260, 
  1214, 
  1170, 
  1130, 
  1092, 
  1057, 
  1024, //4x
  964,  
  910, 
  862, 
  819,  
  780,  
  745,  
  712, 
  683,  
  655,  
  630,  
  607,  
  585, 
  565,  
  546,  
  529,  
  512, //8x
  482, 
  455,  
  431,  
  410,  
  390,  
  372,  
  356,  
  341,  
  328,  
  315,  
  303,
  293,  
  282,  
  273,  
  264,  
  256, //16x
  241,  
  227,  
  215,  
  205,  
  195, 
  186,  
  178,  
  171,  
  164,  
  158, 
  152,  
  146,  
  141,  
  137,  
  132, 
  128  //32x   
};

#define SZ_GAIN_TBL (sizeof(HM2140_GainTable) /sizeof(ISP_UINT16) )
#define __INTERNAL_LDO__ (0) 
//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____Sensor_Init_OPR_Table____(){ruturn;} //dummy
#endif

ISP_UINT16 SNR_HM2140_Reg_Unsupport[] =
{
    SENSOR_DELAY_REG, 100 // delay
};

ISP_UINT16 SNR_HM2140_Reg_Init_Customer[] =
{
	// TBD
	SENSOR_DELAY_REG, 1,//100 // delay
};

#if 0
void ____Sensor_Res_OPR_Table____(){ruturn;} //dummy
#endif

#define __FAST_INIT__   (1)
ISP_UINT16 SNR_HM2140_Reg_1920x1080_30P[] =
{
    0x0103,00,
    //0x0100,00,
    SENSOR_DELAY_REG, 5, // delay
    //LDO config
#if ( __INTERNAL_LDO__ == 1)
    0x5227,0x6F,
    0x030b,0x01,
#else
    0x5227,0x74,
    0x030b,0x11,
#endif
    //SENSOR_DELAY_REG, 100, // delay
    //PLL setting
    0x0307,0x00,
    0x0309,0x00,
    0x030A,0x0A,
    0x030D,0x02,
    0x030F,0x10,
    //SSCG
    0x5235,0x24,
    0x5236,0x92,
    0x5237,0x23,
    0x5238,0xDC,
    0x5239,0x01,
    0x0100,0x02,    
    //---------------------------------------------------   
    // Digital function                                     
    //---------------------------------------------------   
                                                            
    0x4001,0x00,                                          
    0x4002,0x2B,                                          
    0x0101,0x00,                                          
                                            
    // -----------------------------------------------------
    // Resolution : FULL 1928x1088, dark row 12             
    // -----------------------------------------------------
                                                            
    0x4026,0x3B,                                                                                              
    0x0202,0x02,                                          
    0x0203,0xEC,                                          
    0x0340,0x04,                                          
    0x0341,0x56,                                          
    0x0342,0x08,                                          
    0x0343,0xFC,                                          
    0x0350,0x73,                                          
    0x5015,0xB3,                                          
                                                            
                                                            
    //---------------------------------------------------   
    // Analog                                               
    //---------------------------------------------------   
                                                            
    // gain strategy                                        
    0x50DD,0x01,                                          
                                                            
    // 9bit mode                                            
    0x50CB,0xA3,                                                                                      
    0x5004,0x40,                                      
    0x5005,0x28,                                       
    0x504D,0x7F,                                      
    0x504E,0x00,                                      
    // TS                                                   
    0x5040,0x07,                                                                                              
    // BLC/BLI                                              
    0x5011,0x00,                                     
    0x501D,0x4C,                                     
                                     
    0x5011,0x0B,                                     
    0x5012,0x01,                                     
    0x5013,0x03,                                     
    0x4131,0x00,                                     
    0x5282,0xFF,                                        
    0x5283,0x07,                                        
    0x5010,0x20,                                     
    0x4132,0x20,                                     
                                                           
    // VRNP ON/OFF by INT                                   
    0x50D5,0xE0,                                                                                      
    0x50D7,0x12,                                      
    0x50BB,0x14,                                      
    
    // DDS0x  ,                                          
    0x50B7,0x00,                                      
    0x50B8,0x35,                                      
    0x50B9,0x10,                                      
    0x50BA,0xF0,                                      
    
    0x50B3,0x24,                                      
    0x50B4,0x00,                                                                                            
    ////[ DDS tmg(affect fps)                               
    0x50FA,0x02,                                     
    0x509B,0x01,                                     
    0x50AA,0xFF,                                     
    0x50AB,0x27,                                     
    0x509C,0x00,
#if (__INTERNAL_LDO__ == 1)
    0x50AD,0x0C,
#else    
    0x50AD,0x0B,
#endif
    0x5096,0x00,                                     
    0x50AF,0x31,                                     
    0x50A0,0x11,                                     
    0x50A2,0x22,                                     
    0x509D,0x20,                                     
    0x50AC,0x55,                                     
    0x50AE,0x26,                                     
    0x509E,0x03,                                     
    0x509F,0x01,                                     
    0x5097,0x12,                                     
    0x5099,0x00,                                     
                                                            
    // vscan tmg                                            
    0x50B5,0x00,                                      
    0x50B6,0x10,                                      
    0x5094,0x08,                                      
                                                            
                                                            
    //[ analog parameter (0x 5200~0x 523F,0x 50E8,9,B)      
                                                            
    0x5200,0x43,                                          
    0x5201,0xC0,                                          
    0x5202,0x00,                                          
    0x5203,0x00,                                          
    0x5204,0x00,                                          
    0x5205,0x05,                                          
    0x5206,0xA1,                                          
    0x5207,0x01,                                          
    0x5208,0x05,                                          
    0x5209,0x0C,                                          
    0x520A,0x00,                                          
    0x520B,0x45,                                          
    0x520C,0x15,                                          
    0x520D,0x40,                                          
    0x520E,0x50,                                          
    0x520F,0x10,                                          
    0x5214,0x40,                                                                                           
    0x5215,0x14,                                          
    0x5216,0x00,                                          
    0x5217,0x02,                                          
    0x5218,0x07,                                          
    0x521C,0x00,                                          
    0x521E,0x00,                                          
    0x522A,0x3F,                                          
    0x522C,0x00,                                          
    0x5230,0x00,                                          
    0x5232,0x05,                                          
    0x523A,0x20,                                          
    0x523B,0x34,                                          
    0x523C,0x03,                                          
    0x523D,0x40,                                          
    0x523E,0x00,                                          
    0x523F,0x70,                                          
    0x50E8,0x14,                                          
    0x50E9,0x00,                                          
    0x50EB,0x0F,                                          
                                                            
    // IO                                                   
    // MIPI analog part (0x4B02~0x4BD3)                     
                                                            
    0x4B11,0x0F,                                    
    0x4B12,0x0F,                                    
    0x4B31,0x04,                                    
                                    
    0x4B3B,0x02,                                    
                                        
    0x4B44,0x80,                                    
                                        
    0x4B45,0x00,                                    
                                      
    0x4B47,0x00,                                    
    0x4B4E,0x30,                                      
                                                                    
                                                            
    // parallel port                                        
    0x4020,0x20,                                          
                                                            
    //---- Analog gain table:-----------                    
    // GT1                                                  
    // A-gain combination-1/5(10bit mdoe)                   
    // AGAINTB_10bit_A0~6                                   
    0x5100,0x1B,                                   
    0x5101,0x2B,                                   
    0x5102,0x3B,                                   
    0x5103,0x4B,                                   
    0x5104,0x5F,                                   
    0x5105,0x6F,                                   
    0x5106,0x7F,                                   
                                                            
    // A-gain combination-2/5(10bit mdoe)                   
    // AGAINTB_10bit_B0~6                                   
    0x5108,0x00,                                    
    0x5109,0x00,                                    
    0x510A,0x00,                                    
    0x510B,0x00,                                    
    0x510C,0x00,                                    
    0x510D,0x00,                                    
    0x510E,0x00,                                    
                                                            
    // A-gain combination-3/5(10bit mdoe)                   
    // AGAINTB_10bit_C0~6                                   
    0x5110,0x0E,                                     
    0x5111,0x0E,                                     
    0x5112,0x0E,                                     
    0x5113,0x0E,                                     
    0x5114,0x0E,                                     
    0x5115,0x0E,                                     
    0x5116,0x0E,                                     
                                                            
    // A-gain combination-4/5(10bit mdoe)                   
    // AGAINTB_10bit_D0~6                                   
    0x5118,0x09,                                     
    0x5119,0x09,                                     
    0x511A,0x09,                                     
    0x511B,0x09,                                     
    0x511C,0x09,                                     
    0x511D,0x09,                                     
    0x511E,0x09,                                     
                                                            
    // A-gain combination-5/5(10bit mdoe)                   
    // AGAINTB_10bit_E0~6                                   
    0x5120,0xEA,                                     
    0x5121,0x6A,                                     
    0x5122,0x6A,                                     
    0x5123,0x6A,                                     
    0x5124,0x6A,                                     
    0x5125,0x6A,                                     
    0x5126,0x6A,                                     
    //----                                                  
    // A-gain combination-1/5(9bit mdoe)                    
    // AGAINTB_9bit_A0~6                                    
    0x5140,0x0B,                                    
    0x5141,0x1B,                                    
    0x5142,0x2B,                                    
    0x5143,0x3B,                                    
    0x5144,0x4B,                                    
    0x5145,0x5B,                                    
    0x5146,0x6B,                                    
                                                            
    // A-gain combination-2/5(9bit mdoe)                    
    // AGAINTB_9bit_B0~6                                    
    0x5148,0x02,                                     
    0x5149,0x02,                                     
    0x514A,0x02,                                     
    0x514B,0x02,                                     
    0x514C,0x02,                                     
    0x514D,0x02,                                     
    0x514E,0x02,                                     
                                                            
    // A-gain combination-3/5(9bit mdoe)                    
    // AGAINTB_9bit_C0~6                                    
    0x5150,0x08,                                     
    0x5151,0x08,                                     
    0x5152,0x08,                                     
    0x5153,0x08,                                     
    0x5154,0x08,                                     
    0x5155,0x08,                                     
    0x5156,0x08,                                     
                                                            
    // A-gain combination-4/5(9bit mdoe)                    
    // AGAINTB_9bit_D0~6                                    
    0x5158,0x02,                                      
    0x5159,0x02,                                      
    0x515A,0x02,                                      
    0x515B,0x02,                                      
    0x515C,0x02,                                      
    0x515D,0x02,                                      
    0x515E,0x02,                                      
                                                            
    // A-gain combination-5/5(9bit mdoe)                    
    // AGAINTB_9bit_E0~6                                    
                                                            
    0x5160,0x66,                                     
    0x5161,0x66,                                     
    0x5162,0x66,                                     
    0x5163,0x66,                                     
    0x5164,0x66,                                     
    0x5165,0x66,                                     
    0x5166,0x66,                                     
                                                            
    // GT2                                                  
    // parameter table w.r.t CA-gain                        
    //CAGAINTB_000_1~111_9                                  
                                                            
    0x5180,0x00,                                     
    0x5189,0x00,                                     
    0x5192,0x00,                                     
    0x519B,0x00,                                     
    0x51A4,0x00,                                     
    0x51AD,0x00,                                     
    0x51B6,0x00,                                     
    0x51C0,0x00,                                                                     
    0x5181,0x00,                                     
    0x518A,0x00,                                     
    0x5193,0x00,                                     
    0x519C,0x00,                                     
    0x51A5,0x00,                                     
    0x51AE,0x00,                                     
    0x51B7,0x00,                                     
    0x51C1,0x00,                                     
                                         
    0x5182,0x85,                                     
    0x518B,0x85,                                     
    0x5194,0x85,                                     
    0x519D,0x85,                                     
    0x51A6,0x85,                                     
    0x51AF,0x85,                                     
    0x51B8,0x85,                                     
    0x51C2,0x85,                                     
    
    0x5183,0x52,                                     
    0x518C,0x52,                                     
    0x5195,0x52,                                     
    0x519E,0x52,                                     
    0x51A7,0x52,                                     
    0x51B0,0x52,                                     
    0x51B9,0x52,                                     
    0x51C3,0x52,                                     
    0x5184,0x00,                                     
    0x518D,0x00,                                     
    0x5196,0x08,                                     
    0x519F,0x08,                                     
    0x51A8,0x08,                                     
    0x51B1,0x08,                                     
    0x51BA,0x08,                                     
    0x51C4,0x08,                                     
                                          
    0x5185,0x73,                                     
    0x518E,0x73,                                     
    0x5197,0x73,                                     
    0x51A0,0x73,                                     
    0x51A9,0x73,                                     
    0x51B2,0x73,                                     
    0x51BB,0x73,                                     
    0x51C5,0x73,                                     
                                          
    0x5186,0x34,                                     
    0x518F,0x34,                                     
    0x5198,0x34,                                     
    0x51A1,0x34,                                     
    0x51AA,0x34,                                     
    0x51B3,0x3F,                                     
    0x51BC,0x3F,                                     
    0x51C6,0x3F,                                     
                                                            
    // CA-gain combination(in/out cap)                      
    // ca_gain_out,ca_gain_in                               
    0x5187,0x40,                                      
    0x5190,0x38,                                      
    0x5199,0x20,                                      
    0x51A2,0x08,                                      
    0x51AB,0x04,                                      
    0x51B4,0x04,                                      
    0x51BD,0x02,                                      
    0x51C7,0x01,                                      
                                                       
    0x5188,0x20,                                      
    0x5191,0x40,                                      
    0x519A,0x40,                                      
    0x51A3,0x40,                                      
    0x51AC,0x40,                                      
    0x51B5,0x78,                                      
    0x51BE,0x78,                                      
    0x51C8,0x78,                                      
                                                            
    // parameter table w.r.t Ramp-gain                      
    0x51E1,0x07,                                                                                
    0x51E3,0x07,                                      
    0x51E5,0x07,                                      
                                                                  
                                                            
    // comp-gain compensation(CGC_2)                        
    0x51ED,0x00,                                          
    0x51EE,0x00,                                          
                                                            
    //--------------------                                  
    0x4002,0x2B,                                          
    0x3132,0x00,                                          
                                                            
    //I/F sel(MIPI/parallel)                                  
    0x4024,0x40,                                          
    0x5229,0xFC,                                          
                                                       
    0x4002,0x2B,                                          
                                                            
    //---------------------------------------------------   
    // BPC                                                  
    //---------------------------------------------------   
    //0x3110,0x03,
    0x3110,0x00,
    0x373D,0x12,                                   
                                                            
    //---------------------------------------------------   
    // Static BPC                                           
    //---------------------------------------------------   
    0xBAA2,0xC0,                                      
    0xBAA2,0x40,                                      
                                          
    0xBA90,0x01,                                      
    0xBA93,0x02,                                      
                                          
    0x350D,0x01,                                      
    0x3514,0x00,                                      
    0x350C,0x01,                                      
                                          
    0x3519,0x00,                                      
    0x351A,0x01,                                      
    0x351B,0x1E,                                      
    0x351C,0x90,                                      
                                     
    0x351E,0x05,                                      
    0x351D,0x05,                                      
    //---------------------------------------------------   
    //mipi-tx settings (pkt_clk 96mhz)                      
    //---------------------------------------------------   
    //Continuous clock w/ LS/LE                             
    0x4B20,0x9E,                                   
    0x4B18,0x00,                                   
    0x4B3E,0x00,                                   
    0x4B0E,0x21,                                                                           
    //---------------------------------------------------   
    //CMU update                                            
    //---------------------------------------------------   
    0x4800,0xAC,                                     
    0x0104,0x01,                                     
    0x0104,0x00,                                     
    0x4801,0xAE,                                                                                    
    //---------------------------------------------------   
    // Turn on rolling shutter                              
    //---------------------------------------------------   
    0x0000,0x00,                                                                                           
    0x0100,0x01,                                        


};

#if 0
void ____Sensor_Customer_Func____(){ruturn;} // dummy
#endif

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_InitConfig
//  Description :
//------------------------------------------------------------------------------
static void SNR_Cust_InitConfig(void)
{
	MMP_USHORT 	i;
#if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)||(SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    MMP_ULONG ulFreq, ulSot;
#endif
	//MMP_ULONG 	ulSensorMCLK = 27000; // 27 M

#if (SENSOR_IF == SENSOR_IF_PARALLEL)
	RTNA_DBG_Str(0, "\r\nHM2140 Parallel\r\n");
#elif (SENSOR_IF == SENSOR_IF_MIPI_1_LANE)
    RTNA_DBG_Str(0, FG_PURPLE("\r\nHM2140 MIPI 1-lane\r\n"));
#elif (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    RTNA_DBG_Str(0, FG_PURPLE("\r\nHM2140 MIPI 2-lane\r\n"));
#elif (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)
	RTNA_DBG_Str(0, "\r\nHM2140 MIPI 4-lane\r\n");
#endif

	// Init OPR Table
    SensorCustFunc.OprTable->usInitSize                             = (sizeof(SNR_HM2140_Reg_Init_Customer)/sizeof(SNR_HM2140_Reg_Init_Customer[0]))/2;
    SensorCustFunc.OprTable->uspInitTable                           = &SNR_HM2140_Reg_Init_Customer[0];    

    SensorCustFunc.OprTable->bBinTableExist                         = MMP_FALSE;
    SensorCustFunc.OprTable->bInitDoneTableExist                    = MMP_FALSE;

	for (i = 0; i < MAX_SENSOR_RES_MODE; i++)
	{
		SensorCustFunc.OprTable->usSize[i] 							= (sizeof(SNR_HM2140_Reg_Unsupport)/sizeof(SNR_HM2140_Reg_Unsupport[0]))/2;
		SensorCustFunc.OprTable->uspTable[i] 						= &SNR_HM2140_Reg_Unsupport[0];
	}

	// 16:9
    SensorCustFunc.OprTable->usSize[RES_IDX_1920x1080_30P]          = (sizeof(SNR_HM2140_Reg_1920x1080_30P)/sizeof(SNR_HM2140_Reg_1920x1080_30P[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1920x1080_30P]        = &SNR_HM2140_Reg_1920x1080_30P[0];    
    
    #if RES_IDX_1280x720_30P > 0
    SensorCustFunc.OprTable->usSize[RES_IDX_1280x720_30P]          = (sizeof(SNR_HM2140_Reg_1920x1080_30P)/sizeof(SNR_HM2140_Reg_1920x1080_30P[0]))/2;
    SensorCustFunc.OprTable->uspTable[RES_IDX_1280x720_30P]        = &SNR_HM2140_Reg_1920x1080_30P[0];    
    #endif
    
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

	SensorCustFunc.VifSetting->VifPadId 							= MMPF_VIF_MDL_ID0;

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
	SensorCustFunc.VifSetting->powerOnSetting.ubFirstEnPinDelay 	= 0;//10;
	SensorCustFunc.VifSetting->powerOnSetting.bNextEnPinHigh 		= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOnSetting.ubNextEnPinDelay 		= 0;//100;
	SensorCustFunc.VifSetting->powerOnSetting.bTurnOnClockBeforeRst = MMP_TRUE;
	SensorCustFunc.VifSetting->powerOnSetting.bFirstRstPinHigh 		= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOnSetting.ubFirstRstPinDelay 	= 2;//100;
	SensorCustFunc.VifSetting->powerOnSetting.bNextRstPinHigh 		= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOnSetting.ubNextRstPinDelay 	= 1;//100;

	// Init Vif Setting : PowerOff Setting
	SensorCustFunc.VifSetting->powerOffSetting.bEnterStandByMode 	= MMP_FALSE;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeReg 	= 0x00;
	SensorCustFunc.VifSetting->powerOffSetting.usStandByModeMask 	= 0x00;
	SensorCustFunc.VifSetting->powerOffSetting.bEnPinHigh 			= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOffSetting.ubEnPinDelay 		= 5;//20;
	SensorCustFunc.VifSetting->powerOffSetting.bTurnOffMClock 		= MMP_TRUE;
	SensorCustFunc.VifSetting->powerOffSetting.bTurnOffExtPower 	= MMP_TRUE;
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
	SensorCustFunc.VifSetting->mipiAttr.ubBClkLatchTiming 			= MMPF_VIF_SNR_LATCH_POS_EDGE; //MMPF_VIF_SNR_LATCH_NEG_EDGE; 
	
#if (SENSOR_IF == SENSOR_IF_MIPI_4_LANE)||(SENSOR_IF == SENSOR_IF_MIPI_2_LANE)
    MMPF_PLL_GetGroupFreq(CLK_GRP_SNR, &ulFreq);
    ulSot =  0x1F;//0x14;//F;//(0x1F * ulFreq) / 400000; // SOT 0x1F is for VIF 400MHz
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
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= 0;//0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= 0;//0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= 0;//0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= 0;//0x08;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= 0x1F;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= 0x1F;
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
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[0] 				= 0; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[1] 				= 0; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[2] 				= 0; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.usDataDelay[3] 				= 0; //0x08;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= ulSot;
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
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2] 			= ulSot;
	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3] 			= ulSot;
#endif

	// Init Vif Setting : Color ID Setting
	SensorCustFunc.VifSetting->colorId.VifColorId 					= MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.bUseCustomId 	= MMP_TRUE;
	SensorCustFunc.VifSetting->colorId.CustomColorId.Rot0d_Id[0]   = MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.Rot0d_Id[1]   = MMPF_VIF_COLORID_11;	
	SensorCustFunc.VifSetting->colorId.CustomColorId.V_Flip_Id[0]   = MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.V_Flip_Id[1]   = MMPF_VIF_COLORID_11;
	SensorCustFunc.VifSetting->colorId.CustomColorId.H_Flip_Id[0]   = MMPF_VIF_COLORID_10;
	SensorCustFunc.VifSetting->colorId.CustomColorId.H_Flip_Id[1]   = MMPF_VIF_COLORID_10;
	SensorCustFunc.VifSetting->colorId.CustomColorId.HV_Flip_Id[0]   = MMPF_VIF_COLORID_10;
	SensorCustFunc.VifSetting->colorId.CustomColorId.HV_Flip_Id[1]   = MMPF_VIF_COLORID_10;

	/*
	printc("SOT [%d,%d,%d,%d],SNR.CLK:%d\r\n", SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[0],
                                    SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[1],
                                	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[2],
                                	SensorCustFunc.VifSetting->mipiAttr.ubDataSotCnt[3],
                                	ulFreq ); 
    */      
}

//20150526
//lv1-7, 2	3	6	11	17	30	60
#if 1//	new extent node for18//LV1,		LV2,		LV3,		LV4,		LV5,		LV6,		LV7,		LV8,		LV9,		LV10,	LV11,	LV12,	LV13,	LV14,	LV15,	LV16 	LV17  	LV18
//abby curve iq 12
ISP_UINT32 AE_Bias_tbl[54] =
/*lux*/						{2,			3,			5,			9,			16, 		31, 		63, 		122, 		252,		458,	836,	1684,	3566,	6764,	13279,	27129,	54640, 108810/*930000=LV17*/
/*ENG*/						,0x2FFFFFF, 4841472*2,	3058720,	1962240,	1095560,  	616000, 	334880, 	181720,     96600,	 	52685,	27499,	14560,	8060,	4176,	2216,	1144,	600,   300
/*Tar*/						,50,		50,		 	58,	        70,			86,	 		94,	 		140,	 	180,	    220,	    230,	250,	255,	260,	265,	275,	282,	283,   284
 };	
#define AE_tbl_size  (18)	//32  35  44  50
#endif

#define AGAIN_1X  0x100
#define DGAIN_1X  0x100
#define HM2140_MaxGain 28

ISP_UINT32 ISP_dgain, s_gain;
//ISP_UINT32 isp_gain;
ISP_UINT32 sensor_gain_set = 4096;

#define SENSOR_HIGH_TEMPERATURE_CONTROL 0
#if SENSOR_HIGH_TEMPERATURE_CONTROL
	MMP_USHORT sensor_rgE1 = 0xFF, sensor_rgE2 = 0xFF;
	MMP_USHORT sensor_rg1B = 0x0F, sensor_rg1C = 0xFF;
#endif


//------------------------------------------------------------------------------
//  Function    : SNR_Cust_DoAE_FrmSt
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_DoAE_FrmSt(MMP_UBYTE ubSnrSel, MMP_ULONG ulFrameCnt)
{
#if 1
	static ISP_UINT16 ae_gain;
	static ISP_UINT16 ae_shutter;
	static ISP_UINT16 ae_vsync;
					
	MMP_ULONG ulVsync = 0;
	MMP_ULONG ulShutter = 0;

	if(*(MMP_UBYTE *) 0x800070c8 == 0) 
		return ;
	
	if (ulFrameCnt <  1/*m_AeTime.ubPeriod*/) {
		return ;
	}


	if ((ulFrameCnt % m_AeTime.ubPeriod) == 1) 
	{
		ISP_IF_AE_Execute();
    
		ae_shutter 	= ISP_IF_AE_GetShutter();
		ae_vsync 	= ISP_IF_AE_GetVsync();
		ulVsync 	= (gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetVsync()) / ISP_IF_AE_GetVsyncBase();
		ulShutter 	= (gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter()) / ISP_IF_AE_GetShutterBase();
		
		s_gain = ISP_IF_AE_GetGain(); 
		if( s_gain > ISP_IF_AE_GetGainBase() * HM2140_MaxGain )
		{
			ISP_dgain  = DGAIN_1X * s_gain /( ISP_IF_AE_GetGainBase()  * HM2140_MaxGain );		
			s_gain = ISP_IF_AE_GetGainBase()  * HM2140_MaxGain;
		}
	    else
		{
		    ISP_dgain = DGAIN_1X;
		}
	 	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//set to bank1
		gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, ulShutter, ulVsync);
		gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, s_gain);
		gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x09, 0x01); //shutter & gain update, if update in frame active next 2 frames, if blanking next frame.
		//dbg_printf(0,"%x %x %x\r\n", ISP_IF_AWB_GetGainR(), ISP_IF_AWB_GetGainG(), ISP_IF_AWB_GetGainB());
		//printc("%x\r\n", ISP_IF_AE_GetGain());
	}
	
	
	if ((ulFrameCnt % m_AeTime.ubPeriod) == 1)
	{
		ISP_IF_IQ_SetAEGain(ISP_dgain, DGAIN_1X);
	}
	
	
	#if SENSOR_HIGH_TEMPERATURE_CONTROL // update  sensor_rgE1 / sensor_rgE2
		if ((ulFrameCnt % m_AeTime.ubPeriod) != m_AeTime.ubFrmStSetShutFrmCnt)
		{
			{
			 	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank Group1		
				gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0xE1, &sensor_rgE1);//read 0xE1
				gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0xE2, &sensor_rgE2);//read 0xE2
			}
			
			{
				ISP_UINT16 DigDac1 = 0;
			 	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x02);//bank Group2
				gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x1B, &sensor_rg1B);//read 0x1B
				gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x1C, &sensor_rg1C);//read 0x1C
			
				DigDac1 = ((sensor_rg1B &0x07) <<8) + (sensor_rg1C & 0xFF);
			
				if(DigDac1 > 0x20  || sensor_gain_set < 0x200) 
				{
				 	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank GroupB		
					gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xE1, sensor_rgE1 |0x10);//read 0xE1
					gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xE2, sensor_rgE2 |0x08);//read 0xE2		
				}	
				else if(DigDac1 < 0x1C  && sensor_gain_set > 0x2AB)
				{
				 	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank GroupB		
					gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xE1, sensor_rgE1 &0xEF);//read 0xE1
					gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xE2, sensor_rgE2 &0xF7);//read 0xE2		
				}
				gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x09, 0x01); //shutter & gain update, if update in frame active next 2 frames, if blanking next frame.			
			}
		}
	#endif	
		
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
/*
static MMP_UBYTE SNR_GetGainIndex( MMP_USHORT val)
{
  MMP_UBYTE i ;
  for(i=0;i<SZ_GAIN_TBL;i++) {
    if(val >= HM2140_GainTable[i] ) {
      return i ;
    }
  }
  printc("[HM2140] : Bad Gain Idx\r\n"); 
  return 0 ;
}
*/
//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetGain
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetGain(MMP_UBYTE ubSnrSel, MMP_ULONG ulGain)
{
#if 1

	ISP_UINT32 i;
	ISP_UINT32 set_gain = 0x100, sensor_gain;
	ISP_UINT32 target_gain;
  ISP_UINT8  gain_idx ;
  	
	target_gain = ISP_MAX(ulGain,ISP_IF_AE_GetGainBase()) ;

	sensor_gain = 0x10 * target_gain / ISP_IF_AE_GetGainBase();
/*	
	if(sensor_gain < 0x20){
		i = sensor_gain - 0x10;  //index 0~15
	}	
	else if(sensor_gain < 0x40){
		i = sensor_gain / 2; //index 16~31
	}	
	else if(sensor_gain < 0x80){
		i = sensor_gain / 4 + 0x10; //index 32~47
	}
	else if(sensor_gain < 0x100){
		i = sensor_gain / 8 + 0x20; //index 48~63
	}
	else if(sensor_gain < 0x200){
		i = sensor_gain / 16 + 0x30; //index 64~79
	}
	else
		i = 80;
		
	sensor_gain_set =  HM2140_GainTable[i];
  set_gain = ISP_IF_AE_GetGainBase() * 4096 / sensor_gain_set;
  ISP_dgain = target_gain * ISP_dgain / set_gain;  	
  	// Don't let sensor gain under 1.25x ( HM2140 is 1.25x)
	if (sensor_gain_set > 3277)
	    sensor_gain_set = 3277; 
  
  gain_idx = SNR_GetGainIndex( sensor_gain_set ); 	    
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank GroupB
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x83, gain_idx);//bank GroupB
*/
    gain_idx = sensor_gain - 0x10;
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0205, gain_idx);//bank GroupB
	
#endif
}

//------------------------------------------------------------------------------
//  Function    : SNR_Cust_SetShutter
//  Description :
//------------------------------------------------------------------------------
void SNR_Cust_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG shutter, MMP_ULONG vsync)
{ // shutter(2~ LPF-2) = LPF(vsync 0x0A~0x0B) - Ny(0x0C~0x0D)
#if 1
	ISP_UINT32 new_vsync 	= vsync;
	ISP_UINT32 new_shutter 	= shutter;
	
	if( shutter == 0 || vsync == 0)
	{
		new_vsync 	= (gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetVsync()) / ISP_IF_AE_GetVsyncBase();
		new_shutter = (gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter()) / ISP_IF_AE_GetShutterBase();
	}

	new_vsync 	= ISP_MIN(ISP_MAX((new_shutter), new_vsync), 0xFFFF);
	new_shutter = ISP_MIN(ISP_MAX(new_shutter, 3), (new_vsync - 2));
	
	// vsync fix FPS, so vsync not changed.
	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank GroupB
	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0A, (new_vsync >> 8));
	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0B, new_vsync);

	// shutter = Vsync - Ny
	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0C, (((new_vsync - new_shutter) >> 8) & 0xFF));
	//gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0D, (( new_vsync - new_shutter) & 0xFF));
	//Vsync
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0340, (new_vsync >> 8));
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0341, new_vsync);
	// shutter = Vsync - Ny
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0202, (((new_vsync - new_shutter) >> 8) & 0xFF));
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0203, (( new_vsync - new_shutter) & 0xFF));
    
    
#endif
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
#if 0//TBD
    MMP_USHORT vflip,hflip ;
    // protected by upper layer api
    //ISP_IF_3A_Control(ISP_3A_PAUSE);

    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0xEF, 0x01);//bank GroupB
    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x1B, &hflip); // 
    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, 0x1D, &vflip); // 

    switch(ubMode) {
    case MMPF_SENSOR_COLUMN_FLIP:
        vflip = vflip | 0x80 ;
        hflip = hflip & 0x7F ;
        break;
    case MMPF_SENSOR_ROW_FLIP:
        vflip = vflip & 0x7F ;
        hflip = hflip | 0x80 ;
        break;
    case MMPF_SENSOR_COLROW_FLIP:
        vflip = vflip | 0x80 ;
        hflip = hflip | 0x80 ;
        break;
    case MMPF_SENSOR_NO_FLIP:
    default:
        hflip = hflip & 0x7F ;
        vflip = vflip & 0x7F ;            
    	break;
    }

    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x1B, hflip); // 
    gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x1D, vflip); // 
    // Let it sync by shutter 
    //gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x09, 0x01); // active
    //ISP_IF_3A_Control(ISP_3A_RECOVER);
#endif    
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

void SNR_Cust_SetNightVision(MMP_BOOL night)
{
  MMPF_SENSOR_CUSTOMER *cust = (MMPF_SENSOR_CUSTOMER *)&SensorCustFunc ;
  //printc("[TBD] : NV_mode:%d\r\n", night);
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

/* #pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall", zidata = "initcall" */
/* #pragma O0 */
ait_module_init(SNR_Module_Init);
/* #pragma */
/* #pragma arm section rodata, rwdata, zidata */

#endif  //BIND_SENSOR_HM2140
#endif	//SENSOR_EN
