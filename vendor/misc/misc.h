/************************************************************************************************
** File:        Misc.h
** Description: Macro and function definitions 
**
** Copyright 2015 Zilog Inc. ALL RIGHTS RESERVED.
*
*************************************************************************************************
* The source code in this file was written by an authorized Zilog employee or a licensed 
* consultant. The source code has been verified to the fullest extent possible. 
*
* Permission to use this code is granted on a royalty-free basis. However, users are cautioned to
* authenticate the code contained herein. 
* 
* ZILOG DOES NOT GUARANTEE THE VERACITY OF THIS SOFTWARE; ANY SOFTWARE CONTAINED HEREIN IS
* PROVIDED "AS IS." NO WARRANTIES ARE GIVEN, WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
* IMPLIED WARRANTIES OF FITNESS FOR PARTICULAR PURPOSE OR MERCHANTABILITY. IN NO EVENT WILL ZILOG
* BE LIABLE FOR ANY SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY LIABILITY IN TORT,
* NEGLIGENCE, OR OTHER LIABILITY INCURRED AS A RESULT OF THE USE OF THE SOFTWARE, EVEN IF ZILOG 
* HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. ZILOG ALSO DOES NOT WARRANT THAT THE USE 
* OF THE SOFTWARE, OR OF ANY INFORMATION CONTAINED THEREIN WILL NOT INFRINGE ANY PATENT, 
* COPYRIGHT, OR TRADEMARK OF ANY THIRD PERSON OR ENTITY.

* THE SOFTWARE IS NOT FAULT-TOLERANT AND IS NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE IN 
* CONJUNCTION WITH ON-LINE CONTROL EQUIPMENT, IN HAZARDOUS ENVIRONMENTS, IN APPLICATIONS 
* REQUIRING FAIL-SAFE PERFORMANCE, OR WHERE THE FAILURE OF THE SOFTWARE COULD LEAD DIRECTLY TO 
* DEATH, PERSONAL INJURY OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE (ALL OF THE FOREGOING, 
* "HIGH RISK ACTIVITIES"). ZILOG SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY TO HIGH 
* RISK ACTIVITIES.
*
************************************************************************************************/

#ifndef _MISC_H_
#define _MISC_H_


// Register address and bit definitions for JSA1214 Ambient Light Sensor
#define CG5162TC_ADDRESS	(0x20>>1) //(0x20<<1)	//Terry

#define READ_CMD			0x01	//Terry
#define	WRITE_CMD			0x00	//Terry

// CG5162TC register		//Terry
#define	CG5162TC_REG_PRODUCTNUMBER_8BIT		0x00	//read only
#define	CG5162TC_REG_PRODUCTNUMBER_16BIT	0x00	//read only
#define	CG5162TC_REG_OPMODE		0x03	// 0000 01X0 => [1] Power down control: 0 chip active ; 1 chip power down
#define	CG5162TC_REG_TIGSEL		0x04	// Integration time, default is 0x94 (400ms)
#define	CG5162TC_REG_CGAIN		0x05	// Current gain, CCGIN used to increase the light sensitivity, default is 0xFF
#define	CG5162TC_REG_UPDATE		0x20	// User should read this register first for updateing following register 0x21 ~ 0x24
#define CG5162TC_REG_CH0LB		0x21	// 8-bit for reading ADC channel0, low byte
#define CG5162TC_REG_CH0HB		0x22	// 16-bit for reading ADC channel0, hight byte
#define CG5162TC_REG_CH1LB		0x23	// 8-bit for reading ADC channel1, low byte
#define CG5162TC_REG_CH1HB		0x24	// 16-bit for reading ADC channel1, hight byte

#define	CG5162TC_DATA_POWERDOWN	0x06	// 0000 0110 => power down
#define	CG5162TC_DATA_ACTIVE	0x04	// 0000 0100 => active mode

#define IRLED_OFF             (0)
#define IRLED_ON              (1)
//#define _LUX_THRESHOLD_  		  20
//#define _LUX_LOW_THRESHOLD_		10 //2.5//3.5
//#define _LUX_HIGH_THRESHOLD_	14 //5.0//7.0

// GSensor Slave address
#define LIS2DH12_ADDRESS		0x32

// GSensor Register address
#define LIS2DH12_REG_WHOAMI		0x0F
#define LIS2DH12_CTRL_REG1		0x20
#define LIS2DH12_CTRL_REG2		0x21
#define LIS2DH12_CTRL_REG3		0x22
#define LIS2DH12_CTRL_REG4		0x23
#define LIS2DH12_CTRL_REG5		0x24
#define LIS2DH12_CTRL_REG6		0x25
#define LIS2DH12_REG_STATUS		0x27
#define LIS2DH12_INT1_CFG		0x30
#define LIS2DH12_INT2_CFG		0x34
#define LIS2DH12_FIFO_CTRL_REG	0x2E
#define LIS2DH12_OUT_X_L		0x28
#define LIS2DH12_OUT_X_H		0x29
#define LIS2DH12_OUT_Y_L		0x2A
#define LIS2DH12_OUT_Y_H		0x2B
#define LIS2DH12_OUT_Z_L		0x2C
#define LIS2DH12_OUT_Z_H		0x2D
#define LIS2DH12_INT1_THS		0x32
#define LIS2DH12_INT2_THS		0x36

// CTRL_REG1(20h)
#define LIS2DH12_DATA_ODR_PD	0x00	// 0000 => Power-Down mode
#define LIS2DH12_DATA_ODR_1		0x10	// 0001 => HR/Normal/Low-power mode 1Hz
#define LIS2DH12_DATA_ODR_10	0x20	// 0010 => HR/Normal/Low-power mode 10Hz
#define LIS2DH12_DATA_ODR_25	0x30	// 0011 => HR/Normal/Low-power mode 25Hz
#define LIS2DH12_DATA_ODR_50	0x40	// 0100 => HR/Normal/Low-power mode 50Hz
#define LIS2DH12_DATA_ODR_100	0x50	// 0101 => HR/Normal/Low-power mode 100Hz
#define LIS2DH12_DATA_ODR_200	0x60	// 0110 => HR/Normal/Low-power mode 200Hz
#define LIS2DH12_DATA_ODR_400	0x70	// 0111 => HR/Normal/Low-power mode 400Hz
#define LIS2DH12_DATA_ODR_1620	0x80	// 1000 => Low-power mode 1.620kHz
#define LIS2DH12_DATA_ODR_5376	0x90	// 1001 => HR/Normal 1.344kHz ; Low-power mode 5.376kHz

#define LIS2DH12_DATA_LPen		0x08	// xxxx 1xxx in CTRL_REG1; 0:normal mode; 1:low-power mode
#define LIS2DH12_DATA_ZPen		0x04	// xxxx x1xx in CTRL_REG1; Z-axis enable; 0:disable; 1:enable
#define LIS2DH12_DATA_YPen		0x02	// xxxx xx1x in CTRL_REG1; Y-axis enable; 0:disable; 1:enable
#define LIS2DH12_DATA_XPen		0x01	// xxxx xxx1 in CTRL_REG1; X-axis enable; 0:disable; 1:enable

// CTRL_REG3(22h)
#define LIS2DH12_DATA_I1_CLICK		0x80	// CLICK interrupt on INT1 pin; 0:disable; 1:enable
#define LIS2DH12_DATA_I1_AOI1		0x40	// AOI1 interrupt on INT1 pin
#define LIS2DH12_DATA_I1_AOI2		0x20	// AOI2 interrupt on INT1 pin
#define LIS2DH12_DATA_I1_DRDY1		0x10	// DRDY1 interrupt on INT1 pin
#define LIS2DH12_DATA_I1_DRDY2		0x08	// DRDY2 interrupt on INT1 pin
#define LIS2DH12_DATA_I1_WTM		0x04	// FIFO watermark interrupt on INT1 pin
#define LIS2DH12_DATA_I1_OVERRUN	0x02	// FIFO overrun interrupt on INT1 pin

// CTRL_REG4(23h)
#define LIS2DH12_DATA_BDU		0x80	// Block data update
#define LIS2DH12_DATA_BLE		0x40	// Big/Little Endian data selection
#define LIS2DH12_DATA_FS1		0x20	// Full-scale selection (00:2g; 01:4g; 10:8g; 11:16g)
#define LIS2DH12_DATA_FS0		0x10
#define LIS2DH12_DATA_HR		0x08	// Operating mode selection
#define LIS2DH12_DATA_ST1		0x04	// Self-test enable
#define LIS2DH12_DATA_ST0		0x02
#define LIS2DH12_DATA_SIM		0x01	// SPI serial interface mode selection

// CTRL_REG5(24h)
#define LIS2DH12_DATA_BOOT		0x80	// Reboot memory content
#define LIS2DH12_DATA_FIFO		0x40	// FIFO enable
#define LIS2DH12_DATA_LIR_INT1	0x08	// Latch interrupt
#define LIS2DH12_DATA_D4D_INT1	0x04	// 4D detection is enabled on INT1 pin when 6D bit on INT1_CFG is set to 1
#define LIS2DH12_DATA_LIR_INT2	0x02	// Latch interrupt
#define LIS2DH12_DATA_D4D_INT2 	0x01	// 4D detection is enabled on INT2 pin when 6D bit on INT2_CFG is set to 1

// CTRL_REG6(25h)
#define LIS2DH12_DATA_I2_CLICKen	0x80	// Click interrupt on INT2 pin
#define LIS2DH12_DATA_I2_INT1		0x40	// interrupt1 function enable on INT2 pin
#define LIS2DH12_DATA_I2_INT2		0x20	// interrupt2 function enable on INT2 pin
#define LIS2DH12_DATA_BOOT_I2		0x10	// Boot on INT2 pin enable
#define	LIS2DH12_DATA_P2_ACT		0x08	// Activity interrupt enable on INT2 pin
#define LIS2DH12_DATA_H_LACTIVE		0x02	// 0:interrupt active-high; 1:interrupt active-low

// INT1_CFG(30h) /INT2_CFG(34h)
#define LIS2DH12_DATA_AOI		0x80	// And/Or combination of interrupt events
#define LIS2DH12_DATA_6D		0x40	// 6-direction detection function enabled
#define LIS2DH12_DATA_ZHIE		0x20	// Enable interrupt generation on Z high event or on direction recognition
#define LIS2DH12_DATA_ZLIE		0x10	// Enable interrupt generation on Z low event or on direction recognition
#define LIS2DH12_DATA_YHIE		0x08	// Enable interrupt generation on Y high event or on direction recognition
#define LIS2DH12_DATA_YLIE		0x04	// Enable interrupt generation on Y low event or on direction recognition
#define LIS2DH12_DATA_XHIE		0x02	// Enable interrupt generation on X high event or on direction recognition
#define LIS2DH12_DATA_XLIE		0x01	// Enable interrupt generation on X low event or on direction recognition

// FIFO_CTRL_REG(2Eh)
#define LIS2DH12_DATA_FM1		0x80	// FIFO mode selection
#define LIS2DH12_DATA_FM2		0x40	// FIFO mode selection		00 -> Bypass mode; 01 -> FIFO mode; 10 -> stream mode; 11 -> Stream to FIFO mode
#define LIS2DH12_DATA_TR		0x20	// Trigger selection: 0:trigger event allows triggering signal on INT1; 1:trigger event allows triggering signal on INT2


#define AXP192_ADDRESS			0x68	// PMU address
#define AXP192_REG_PSTUS		0x00	// Power status register
#define AXP192_REG_PMODE		0x01	// Power mode / charge status register
#define AXP192_REG_POUT			0x12	// Power output control, EXTEN/DCDC2/LDO3/LDO2/DCDC3/DCDC1 control
#define AXP192_REG_DCDC1		0x26	// Control DC-DC1 output voltage
#define AXP192_REG_DCDC2		0x23	// Control DC-DC2 output voltage
#define AXP192_REG_DCDC3		0x27	// Control DC-DC3 output voltage
#define AXP192_REG_GPIO0		0x90	// Control GPIO0/LDO function
#define AXP192_REG_LDOIO		0x91	// Set LDO value while GPIO0 set to LDOIO
#define AXP192_REG_LDO23		0x28	// Set LDO2/LDO3 value
#define AXP192_REG_GPIO1		0x92	// Control GPIO1 function
#define AXP192_REG_GPIO_SIG		0x94	// Control GPIO[0-2] signal
#define AXP192_REG_PEK			0x36	// PEK parameter config


#define LM75A_ADDRESS		(0x90 >> 1)	//1001 0000	//Terry

#define LM75A_REG_TEMP		0x00
#define LM75A_REG_CONF		0x01
#define LM75A_REG_THYST		0x02
#define LM75A_REG_TOS		0x03


#define OLED_ADDRESS		0x78
#define OLED_WDATA			0x40	// [6] set 1 means for RAM-data (stored in display RAM)
#define OLED_WCMD			0x00	// [6] set 0 means for command

#define LowerColumn  0x02		
#define HighColumn   0x11		
#define Max_Column	96	
#define Max_Row		64	
#define	Brightness	0xCF	

#define ram_width 24
#define ram_high 24
#define ram (ram_width * ram_high) / 8


#define RT9460_ADDRESS		0x46	// Charge IC address: 0100 101x
#define RT9460_DEVICE		0x03	// [7-4]: Vendor ID (0010b) ; [3-0]: Chip revision
#define RT9460_CONTROL1		0x00
#define RT9460_CONTROL2		0x01
#define RT9460_CONTROL3		0x02
#define RT9460_CONTROL4		0x04	// [7]: RST => 1 charge in reset mode, 0 no effect ; [6-0]: reserved
#define RT9460_CONTROL5		0x05	// (default: 0x9A) [7]: SYSUVP_HW_SEL ; [6]: OTG_OC ; [5-4]: SYS_Min ; [3-0]: IPREC	
#define RT9460_CONTROL6		0x06	// (default: 0x02) [7-4]: ICHRG ; [3]: EN_OSCSS ; [2-0]: VPREC
#define RT9460_CONTROL7		0x07	// (default: 0x50) [7]: CC_JEITA ; [6]: BATD_EN ; [5]: CHIP_EN ; [4]: CHG_EN ; [3]: TS_HOT ; [2]: TS_WARM ; [1]: TS_COOL ; [0]: TS_COLD
#define RT9460_CONTROL8		0x1C	// [7-3]: reserved ; [2-0]: PPSenseNode (111--> recommended)
#define RT9460_IRQ1			0x08	//
#define RT9460_IRQ2			0x09	//
#define RT9460_IRQ3			0x0A	//
#define RT9460_MASK1		0x0B
#define RT9460_MASK2		0x0C
#define RT9460_MASK3		0x0D
#define RT9460_CTL_DPDM		0x0E
#define RT9460_CONTROL9		0x21
#define RT9460_CONTROL10	0x22
#define RT9460_CONTROL11	0x23
#define RT9460_CONTROL12	0x24
#define RT9460_CONTROL13	0x25
#define RT9460_STATIRQ		0x26
#define RT9460_STATMASK		0x27



/* Control 7 */
#define RT9460_CCJEITA_MASK		0x80
#define RT9460_CCJEITA_SHFT		7
#define RT9460_BATDEN_MASK		0x40
#define RT9460_BATDEN_SHFT		6
#define RT9460_CHGEN_MASK		0x10
#define RT9460_CHGEN_SHFT		4
#define RT9460_VMREG_MASK		0x0f
#define RT9460_VMREG_SHFT		0
#define RT9460_TSHC_MASK		0x09 	/* hot and cold*/
#define RT9460_TSWC_MASK		0x06	/* warm and cool*/
#define RT9460_CHIPEN_MASK		0x20

// Function Prototypes

void change_effect_mode(unsigned char mode);

void uiDelayFunc(unsigned int uidelp);

void IRCut_OnOff(int on);
void IRLed_OnOff(unsigned char led_on_level);
#if 1//SUPPORT_CG5162==1
void ALS_Init(int preread);
unsigned char ALS_Update(void);
unsigned char ALS_Threshold(unsigned int   lux,unsigned int lux_H,unsigned int lux_L, 
                            unsigned int   temp_C,unsigned int temp_H,unsigned int temp_L,
                            unsigned char  led_on_val ) ; // 0 ~ 255 , 0 is led off

unsigned char ALS_ReadOPMode(void);
float ALS_Read_Value(unsigned char tig,unsigned char cgain) ;
float ALS_ReadLux() ;
void ALS_DeInit(void);
void ALS_SetNightVision(unsigned char nv_mode);
unsigned char ALS_GetNightVision(void);
int ALS_Ready();
#endif

// Gsensor setting
void GSensor_WhoAmI(void);
unsigned char GSensor_OUTReading(void);
unsigned char GSensor_Enable(void);
void GSensor_CtrlREG1_Setting(void);
void GSensor_CtrlREG3_Setting(void);
void GSensor_CtrlREG4_Setting(void);
void GSensor_CtrlREG5_Setting(void);
void GSensor_INT1_CFG_Setting(void);
void GSensor_INT1_THS(void);
void GSensor_Disable(void);

#ifdef SUPPORT_AXP192
void PMU_PowerOfAIT(unsigned char sw);
void PMU_DCDC1_Ctrl(unsigned char sw);
void PMU_DCDC2_Ctrl(unsigned char sw);
void PMU_DCDC3_Ctrl(unsigned char sw);
void PMU_LDOio_Ctrl(unsigned char sw);
void PMU_LDO2LDO3_Ctrl(unsigned char sw);
void PMU_LED_Ctrl(unsigned char sw);
void PMU_PEK_Ctrl(void);
#endif

#if 1//SUPPORT_LM75A==1
int Temperature_Read(void);
void Temperature_Conf(void);
#endif

#ifdef SUPPORT_OLED
void OLED_Write_Data(unsigned char ucDate);
void OLED_Write_Command(unsigned char ucDate);
void OLED_Clear_Screen(void);
void OLED_All_Screen(void);
void OLED_ResetIC(void);
void OLED_InitIC(void);
void Set_Start_Column(unsigned char d);
void Set_Start_Page(unsigned char d);
void Set_Contrast_Control(unsigned char d);
void Set_Segment_Remap(unsigned char d);
void Set_Common_Remap(unsigned char d);
void Set_Inverse_Display(unsigned char d);
void Set_Display_On_Off(unsigned char d);
void Set_VCOMH(unsigned char d);
void Set_NOP();
void Fill_RAM(unsigned char Data);
void Fill_Block(unsigned char Data, unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void Checkerboard(void);
void Show_Font57(unsigned char a, unsigned char b, unsigned char c, unsigned char d);
void Show_String(unsigned char a,  char *Data_Pointer, unsigned char b, unsigned char c);
void Show_Pattern(unsigned char *Data_Pointer, unsigned char a, unsigned char b, unsigned char c, unsigned char d);

void Show_Number(unsigned char ucNum, unsigned char ucSPage, unsigned char ucSColumn);

void Show_Battery(unsigned char c, unsigned char d, unsigned char ucStatus);
void Show_LTESignal(unsigned char c, unsigned char d, unsigned char ucStatus);
void Show_Background(unsigned char ucSPage, unsigned char ucSColumn, unsigned char ucStatus);
void Show_Content(unsigned char ucSPage, unsigned char ucSColumn, unsigned char ucStatus);

void Fade_In(void);
void Fade_Out(void);



#endif


#ifdef SUPPORT_RT9460
void RT9460_Reg_Init(void);
void RT9460_Chip_Disable(void);
void RT9460_Chip_Enable(void);


#endif

#endif
