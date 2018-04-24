#if (SENSOR_IF == SENSOR_IF_MIPI_2_LANE)

MMPF_SENSOR_RESOLUTION m_SensorRes = 
{
	4,				// ubSensorModeNum
	0,				// ubDefPreviewMode
	1,				// ubDefCaptureMode
	2200,           // usPixelSize
//  Mode0   Mode1   Mode2   Mode3
    {1,     1,      1,      1},	    // usVifGrabStX
    {1,     1,      1,      1},	    // usVifGrabStY
    {2312,  2311,   1156,   1284},	// usVifGrabW
    {1300,  1540,   652,    724},	// usVifGrabH
	#if (CHIP == MCR_V2)
    {1,     1,      1,      1},	    // usVifGrabStX
    {1,     1,      1,      1},	    // usVifGrabStY
    {8,     263,    4,      4},     // usBayerDummyInX
    {4,     4,      4,      4},     // usBayerDummyInY
    {2304,  2048,   1152,   1280},	// usBayerOutW
    {1296,  1536,   648,    720},	// usBayerOutH
  	#endif
    {2304,  2048,   800,    1280},	// usScalInputW
    {1296,  1536,   600,    720},	// usScalInputH
    {300,   250,    600,    600},	// usTargetFpsx10
    {1329,  1540,   672,    811},	// usVsyncLine
    {1,     1,      1,      1},     // ubWBinningN
    {1,     1,      1,      1},     // ubWBinningN
    {1,     1,      1,      1},     // ubWBinningN
    {1,     1,      1,      1},     // ubWBinningN
    {0xFF,	0xFF,   0xFF,   0xFF},  // ubCustIQmode
    {0xFF,  0xFF,	0xFF,   0xFF}   // ubCustAEmode
};

#if 0
void ____Sensor_Init_OPR_Table____(){ruturn;} //dummy
#endif

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_Init_Customer[] = 
{
	0x31AE, 0x0202,
	0x301A, 0x0058,     // Disable streaming
	
	SENSOR_DELAY_REG, 10,
	
	0x31AE, 0x202,      // Output 2-lane MIPI
	
	//Configure for Serial Interface
	0x301A, 0x0058,     // Drive Pins,Parallel Enable,SMIA Serializer Disable
	0x3064, 0x1802, 	// Disable Embedded Data
	
	//Optimized Gain Configuration
    0x3EE0, 0x1500, 	// DAC_LD_20_21
	0x3EEA, 0x001D,
	0x31E0, 0x0003,
	0x3F06, 0x046A,
	0x3ED2, 0x0186,
	0x3ED4, 0x8F2C,	
	0x3ED6, 0x2244,	
	0x3ED8, 0x6442,	
	0x30BA, 0x002C,     // Dither enable
	0x3046, 0x4038,		// Enable Flash Pin
	0x3048, 0x8480,		// Flash Pulse Length
	0x3ED0, 0x0016, 	// DAC_LD_4_5
	0x3ED0, 0x0036, 	// DAC_LD_4_5
	0x3ED0, 0x0076, 	// DAC_LD_4_5
	0x3ED0, 0x00F6, 	// DAC_LD_4_5
	0x3ECE, 0x1003, 	// DAC_LD_2_3
	0x3ECE, 0x100F, 	// DAC_LD_2_3
	0x3ECE, 0x103F, 	// DAC_LD_2_3
	0x3ECE, 0x10FF, 	// DAC_LD_2_3
	0x3ED0, 0x00F6, 	// DAC_LD_4_5
	0x3ED0, 0x04F6, 	// DAC_LD_4_5
	0x3ED0, 0x24F6, 	// DAC_LD_4_5
	0x3ED0, 0xE4F6, 	// DAC_LD_4_5
	0x3EE6, 0xA480, 	// DAC_LD_26_27
	0x3EE6, 0xA080, 	// DAC_LD_26_27
	0x3EE6, 0x8080, 	// DAC_LD_26_27
	0x3EE6, 0x0080, 	// DAC_LD_26_27
	0x3EE6, 0x0080, 	// DAC_LD_26_27
	0x3EE8, 0x2024, 	// DAC_LD_28_29
	0x30FE, 128,        // Noise Pedestal of 128

	//Assuming Input Clock of 24MHz.  Output Clock will be 70Mpixel/s
	0x302A, 0x0005, 	// VT_PIX_CLK_DIV
	0x302C, 0x0002, 	// VT_SYS_CLK_DIV
	0x302E, 0x0004, 	// PRE_PLL_CLK_DIV
	0x3030, 82, 	    // PLL_MULTIPLIER
	0x3036, 0x000A, 	// OP_PIX_CLK_DIV
	0x3038, 0x0001, 	// OP_SYS_CLK_DIV
	0x31AC, 0x0A0A, 	// DATA_FORMAT_BITS
	
	//MIPI TIMING
	0x31B0, 40,         // FRAME PREAMBLE
	0x31B2, 14,         // LINE PREAMBLE
	0x31B4, 0x2743,     // MIPI TIMING 0
	0x31B6, 0x114E,     // MIPI TIMING 1
	0x31B8, 0x2049,     // MIPI TIMING 2
	0x31BA, 0x0186,     // MIPI TIMING 3
	0x31BC, 0x8005,     // MIPI TIMING 4
	0x31BE, 0x2003,     // MIPI CONFIG STATUS

	//Sequencer
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
    0x3086, 0x644C,
    0x3086, 0x0928,
    0x3086, 0x2428,
    0x3086, 0x234E,
    0x3086, 0x1718,
    0x3086, 0x26A2,
    0x3086, 0x5C03,
    0x3086, 0x1744,
    0x3086, 0x27F2,
    0x3086, 0x1708,
    0x3086, 0x2803,
    0x3086, 0x2808,
    0x3086, 0x4D1A,
    0x3086, 0x27FA,
    0x3086, 0x2683,
    0x3086, 0x45A0,
    0x3086, 0x1707,
    0x3086, 0x27FB,
    0x3086, 0x1729,
    0x3086, 0x4580,
    0x3086, 0x1708,
    0x3086, 0x27FA,
    0x3086, 0x1728,
    0x3086, 0x2682,
    0x3086, 0x5D17,
    0x3086, 0x0E48 ,
    0x3086, 0x4D4E,
    0x3086, 0x2803,
    0x3086, 0x4C0B,
    0x3086, 0x175F,
    0x3086, 0x27F2,
    0x3086, 0x170A,
    0x3086, 0x2808,
    0x3086, 0x4D1A,
    0x3086, 0x27FA,
    0x3086, 0x2602,
    0x3086, 0x5C00,
    0x3086, 0x4540,
    0x3086, 0x2798,
    0x3086, 0x172A,
    0x3086, 0x4A0A,
    0x3086, 0x4316,
    0x3086, 0x0B43,
    0x3086, 0x279C,
    0x3086, 0x4560,
    0x3086, 0x1707,
    0x3086, 0x279D,
    0x3086, 0x1725,
    0x3086, 0x4540,
    0x3086, 0x1708,
    0x3086, 0x2798,
    0x3086, 0x5D53,
    0x3086, 0x0026,
    0x3086, 0x445C,
    0x3086, 0x014b,
    0x3086, 0x1244,
    0x3086, 0x5251,
    0x3086, 0x1702,
    0x3086, 0x6018,
    0x3086, 0x4A03,
    0x3086, 0x4316,
    0x3086, 0x0443,
    0x3086, 0x1658,
    0x3086, 0x4316,
    0x3086, 0x5943,
    0x3086, 0x165A,
    0x3086, 0x4316,
    0x3086, 0x5B43,
    0x3086, 0x4540,
    0x3086, 0x279C,
    0x3086, 0x4560,
    0x3086, 0x1707,
    0x3086, 0x279D,
    0x3086, 0x1725,
    0x3086, 0x4540,
    0x3086, 0x1710,
    0x3086, 0x2798,
    0x3086, 0x1720,
    0x3086, 0x224B,
    0x3086, 0x1244,
    0x3086, 0x2C2C,
    0x3086, 0x2C2C
};

#if 0
void ____Sensor_Res_OPR_Table____(){ruturn;} //dummy
#endif

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_2304x1296[] = 
{
	0x3030, (82+1), 	// PLL_MULTIPLIER  @ 490Mbps/lane
    0xFFFF, 100,
    
	//ARRAY READOUT SETTINGS
	// +2 for X axis dead pixel.
	0x3004, 0+2,		// X_ADDR_START
	0x3008, 2311+2,	    // X_ADDR_END
	0x3002, 122,    	// Y_ADDR_START
	0x3006, 1421,	    // Y_ADDR_END

	//Sub-sampling
	0x30A2, 1,			// X_ODD_INCREMENT
	0x30A6, 1,			// Y_ODD_INCREMENT
    #if SENSOR_ROTATE_180
	0x3040, 0xC000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
    #else
	0x3040, 0x0000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#endif
    
    //Frame-Timing
	0x300C, 1250,       // LINE_LENGTH_PCK
	0x300A, 1312,		// FRAME_LENGTH_LINES
	0x3014, 0,		    // FINE_INTEGRATION_TIME
	0x3012, 152,		// Coarse_Integration_Time
	0x3042, 0,			// EXTRA_DELAY
	0x30BA, 0x2C,

	0x301A, 0x000C,     // Grouped Parameter Hold = 0x0
};

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_2304x1536[] = 
{
	0x3030, 82, 	    // PLL_MULTIPLIER  @ 490Mbps/lane

	//ARRAY READOUT SETTINGS
	0x3004, 0+2,		// X_ADDR_START
	0x3008, 2311+2,	    // X_ADDR_END
	0x3002, 0 + 6,    	// Y_ADDR_START
	0x3006, 0 + 6 + (1536 + 4 - 1),	// Y_ADDR_END

	//Sub-sampling
	0x30A2, 1,			// X_ODD_INCREMENT
	0x30A6, 1,			// Y_ODD_INCREMENT
    #if SENSOR_ROTATE_180
	0x3040, 0xC000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
    #else
	0x3040, 0x0000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#endif
	
	//Frame-Timing
	0x300C, 1250,       // LINE_LENGTH_PCK
	0x300A, 1536+4,     // FRAME_LENGTH_LINES
	0x3014, 0,		    // FINE_INTEGRATION_TIME
	0x3012, 1000,		// Coarse_Integration_Time
	0x3042, 0,			// EXTRA_DELAY
	0x30BA, 0x2C,

	0x301A, 0x000C,     // Grouped Parameter Hold = 0x0
};

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_1920x1080[] = 
{
	0x3030, 82, 	    // PLL_MULTIPLIER  @ 490Mbps/lane

	//ARRAY READOUT SETTINGS
	0x3004, 192,		// X_ADDR_START
	0x3008, 2127,		// X_ADDR_END
	0x3002, 232,    	// Y_ADDR_START
	0x3006, 1319,		// Y_ADDR_END

	//Sub-sampling
	0x30A2, 1,			// X_ODD_INCREMENT
	0x30A6, 1,			// Y_ODD_INCREMENT
    #if SENSOR_ROTATE_180
	0x3040, 0xC000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
    #else
	0x3040, 0x0000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#endif
    
    //Frame-Timing
	0x300C, 1260,		// LINE_LENGTH_PCK
	0x300A, 1270,		// FRAME_LENGTH_LINES
	0x3014, 0,		    // FINE_INTEGRATION_TIME
	0x3012, 1000,		// Coarse_Integration_Time
	0x3042, 0,			// EXTRA_DELAY
	0x30BA, 0x2C,

	0x301A, 0x000C,     // Grouped Parameter Hold = 0x0
};

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_1152x648[] = 
{
	0x3030, 82, 	    // PLL_MULTIPLIER @ 490Mbps/lane

    //ARRAY READOUT SETTINGS
	0x3004, 2+0,		// X_ADDR_START
	0x3008, 2+ (1152 + 4 )* 2 - 1,  // X_ADDR_END
	0x3002, 122,   		// Y_ADDR_START
	0x3006, 122 + (648 + 4) * 2 - 1,// Y_ADDR_END

    //Sub-sampling
    0x30A2, 3,			// X_ODD_INCREMENT
    0x30A6, 3,			// Y_ODD_INCREMENT
    #if SENSOR_ROTATE_180
	0x3040, 0xF000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
    #else
	0x3040, 0x3000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	#endif

    //Frame-Timing
    0x300C, 1232,       // LINE_LENGTH_PCK
    0x300A, 672,		// FRAME_LENGTH_LINES
    0x3014, 0,			// FINE_INTEGRATION_TIME
    0x3012, 552,		// Coarse_Integration_Time
    0x3042, 0,			// EXTRA_DELAY
    0x30BA, 0x2C,	    // Digital_Ctrl_Adc_High_Speed
  
    0x301A, 0x000C,     // Grouped Parameter Hold = 0x0
};

MMP_USHORT SNR_AR0330_MIPI_2_LANE_Reg_1280x720_60fps[] = 
{
	0x3004,   384,      // X_ADDR_START
	0x3008,   384+1283, // X_ADDR_END
	0x3002,   408,      // Y_ADDR_START
	0x3006,   408+723,  // Y_ADDR_END
	0x30A2,      1,     // X_ODD_INC
	0x30A6,      1,     // Y_ODD_INC
    #if SENSOR_ROTATE_180
	0x3040, 0xC000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	0x3040, 0xC000,	    // READ_MODE
	0x3040, 0xC000,	    // READ_MODE
    #else
	0x3040, 0x0000,	    // [15]: flip, [14]: mirror, [12]: Row binning, [13]: column binning
	0x3040, 0x0000,     // READ_MODE
	0x3040, 0x0000,     // READ_MODE
	#endif
	0x300C,   1014,     // LINE_LENGTH_PCK
	0x300A,    788,     // FRAME_LENGTH_LINES
	0x3014,      0,     // FINE_INTEGRATION_TIME
	0x3012,    785,     // COARSE_INTEGRATION_TIME
	0x3042,    968,     // EXTRA_DELAY
	0x30BA, 0x002C,     // DIGITAL_CTRL

    0x301A, 0x000C,     // Grouped Parameter Hold = 0x0

//	0x301A, 0x10D8,  // RESET_REGISTER
//	0xFFFF, 100,
//	0x3088, 0x80BA,  // SEQ_CTRL_PORT
//	0x3086, 0x0253,  // SEQ_DATA_PORT
//	0x301A, 0x10DC,  // RESET_REGISTER
//	0x301A, 0x10DC,  // RESET_REGISTER
//	0xFFFF,    100   // delay = 100
};

#endif      // SENSOR_IF
