/************************************************************************************************
** File:        misc.c
** Description: 
**
** Copyright 2016 Zilog Inc. ALL RIGHTS RESERVED.
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
//#include <eZ8.h>
//#include "main.h"								// Main application header file
//#include "i2c.h"
//#include "UART_Util.h"							// UART Utilities definition header
#include "mmp_lib.h"
#include "misc.h"
#include "mmpf_i2cm.h"
#include "mmpf_pio.h"


#define DEBUG_LEVEL_H


unsigned char IRLedStatus = IRLED_OFF;
unsigned char IRLedLevel  = (unsigned char)-1 ;
unsigned char NightVision ;

extern MMP_BOOL MMPF_Sensor_SetTest(MMP_UBYTE ubSnrSel, MMP_UBYTE val );

#if SUPPORT_OPT3006==1
extern  int opt3001_probe(void *client);
extern  int opt3001_get_lux( int *val, int *val2);
extern  int opt3001_get_prelux(void);

static MMP_I2CM_ATTR OPT3006_Attr ={
  MMP_I2CM1,   // I2CM interface, depend on used IO pads 
  0x44,                          // I2CM slave address, please check device¡¦s spec.
  8,                                // I2CM address bits
  16,                                // I2CM data bits
  0,
  MMP_FALSE, 
  MMP_TRUE,
  MMP_FALSE,
  MMP_FALSE, 
  0,
  0, 
  1,                                // I2CM pad number
  250000/*250KHZ*/,    // 250KHz, up to 400KHz
  MMP_TRUE,              // Semaphore protect or not
  0,//NULL, 
  0,//NULL,
  FALSE,
  FALSE,
  FALSE,
  0
};
#endif

#if SUPPORT_CG5162==1
static MMP_I2CM_ATTR CG5162_Attr ={
  MMP_I2CM1,   // I2CM interface, depend on used IO pads 
  CG5162TC_ADDRESS,                          // I2CM slave address, please check device¡¦s spec.
  8,                                // I2CM address bits
  8,                                // I2CM data bits
  0,
  MMP_FALSE, 
  MMP_TRUE,
  MMP_FALSE,
  MMP_FALSE, 
  0,
  0, 
  1,                                // I2CM pad number
  250000/*250KHZ*/,    // 250KHz, up to 400KHz
  MMP_TRUE,              // Semaphore protect or not
  0,//NULL, 
  0,//NULL,
  FALSE,
  FALSE,
  FALSE,
  0
};
#endif

#if (SUPPORT_LM75A==1) && defined(ALL_FW)
static MMP_I2CM_ATTR LM75A_Attr ={
  MMP_I2CM1,   // I2CM interface, depend on used IO pads 
  LM75A_ADDRESS,                          // I2CM slave address, please check device¡¦s spec.
  0,//8,                                // I2CM address bits
  8,                                // I2CM data bits
  0,
  MMP_FALSE, 
  MMP_TRUE,
  MMP_FALSE,
  MMP_FALSE, 
  0,
  0, 
  1,                                // I2CM pad number
  250000/*250KHZ*/,    // 250KHz, up to 400KHz
  MMP_TRUE,              // Semaphore protect or not
  0,//NULL, 
  0,//NULL,
  FALSE,
  FALSE,
  FALSE,
  0
};
#endif


int I2C_WriteToSlave(MMP_I2CM_ATTR *attr,unsigned char reg,unsigned short data)
{
  MMP_ERR err = MMP_ERR_NONE ;
//  printc("[MISC.W]:Slave %x, reg:%x,data:%x\r\n",attr->ubSlaveAddr, reg,data);
  
  if( attr->ubDataLen == 8 ) {
    err = MMPF_I2cm_WriteReg(attr, (MMP_USHORT)reg, (MMP_USHORT)( data & 0xff ) ) ;
  }
  else {
    err = MMPF_I2cm_WriteReg(attr, (MMP_USHORT)reg, (MMP_USHORT) data ) ;    
  }

  if(err) {
    return -1 ;
  }
  
  return 0 ;
}

int I2C_ReadFromSlave(MMP_I2CM_ATTR *attr,unsigned char reg, unsigned char *data)
{
  MMP_USHORT temp ;
  MMP_ERR err = MMP_ERR_NONE ;
  err = MMPF_I2cm_ReadReg(attr, (MMP_USHORT)reg, (MMP_USHORT *)&temp ) ;
  
  if(err) {
    *data = 0 ;
    return -1 ;
  }
  if(attr->ubDataLen==8) {
    *data = (unsigned char)( temp & 0xFF ); // for api can return 16 bits data
  }
  else {
    *(unsigned short *)data =  temp ; // for api can return 16 bits data    
  }
  return 0 ;
}



static int light_sensor_inited = 0 ;
unsigned char ALS_ReadOPMode(void)
{
  unsigned char val=0 ;
#if SUPPORT_CG5162==1
	unsigned char err = I2C_ReadFromSlave( &CG5162_Attr, CG5162TC_REG_OPMODE ,&val );
#endif
	return val;
}

 
void ALS_Init(int preread)
{
#define INIT_TIG_SEL  0x1  

  unsigned char opmode,tig;
  unsigned int lux ,i , wait_ms ;
  unsigned short val ;
  
#if SUPPORT_CG5162==1  
  opmode=ALS_ReadOPMode();
  I2C_WriteToSlave( &CG5162_Attr, CG5162TC_REG_TIGSEL , INIT_TIG_SEL );
  if(opmode!=CG5162TC_DATA_ACTIVE) {
    I2C_WriteToSlave( &CG5162_Attr, CG5162TC_REG_OPMODE , CG5162TC_DATA_ACTIVE );
  }
  light_sensor_inited = 1 ;
#endif
#if SUPPORT_OPT3006==1
  if ( opt3001_probe( (void * )&OPT3006_Attr ) < 0 ) {
    light_sensor_inited = 0 ;
  }
  else {
    light_sensor_inited = 1 ;
  }
#endif
  if(preread && light_sensor_inited ) {
  // issue a read first
    ALS_Update();
    printc("\r\n--prelux :%d\r\n",OSTime);
  }
  
}

int ALS_Ready()
{
  return light_sensor_inited ;
}

unsigned char ALS_Update(void)
{
  unsigned char err = 0 ; 
	unsigned char val;
#if	SUPPORT_CG5162
	err = I2C_ReadFromSlave(&CG5162_Attr, CG5162TC_REG_UPDATE ,&val);
#endif	
#if SUPPORT_OPT3006==1
  int ret ;
  ret = opt3001_get_prelux();
#endif

	if(!err )
	{
	  return val ? 1 : 0 ;
	}
	else // could not write the register in 0x20
	{
			return 0;
	}
	
}

#if defined(ALL_FW)
float ALS_ReadLux(void)
{
#define TIG_UNIT 27
#define TIG_BASE 10
#define DEF_TIG 0x94
  int i ; 
  float lux  = 0.0 ;
  unsigned int wait_ms;
  unsigned char tig,cgain ;
  if(!light_sensor_inited) {
    return 0.0 ;
  }
#if	SUPPORT_CG5162
  I2C_ReadFromSlave( &CG5162_Attr ,CG5162TC_REG_TIGSEL , &tig  );
  I2C_ReadFromSlave( &CG5162_Attr ,CG5162TC_REG_CGAIN  , &cgain);
  wait_ms = ( TIG_UNIT * tig + TIG_BASE - 1 ) / TIG_BASE ;
  for(i=0;i<3;i++) {
    if(ALS_Update()) {
      MMPF_OS_Sleep( wait_ms  ) ;
      lux = ALS_Read_Value(tig,cgain&0xf);
      if(lux)  {
        break;
      }
    }
  }
  if(light_sensor_inited==1) {
    light_sensor_inited = 2 ;
    I2C_WriteToSlave( &CG5162_Attr, CG5162TC_REG_TIGSEL , DEF_TIG );
  }
#endif  
#if SUPPORT_OPT3006==1
  {
    int val1,val2 ,ret ;
    ret = opt3001_get_lux( &val1 , &val2 );
    if( ret < 0 ) {
      printc("OPT3001 Lux Err: %d\r\n", ret     );  
    } else {
      lux = val1 + ( val2/1000000 ) ;
    }
    
  }
#endif
  return lux ;
}




float ALS_Read_Value(unsigned char tig,unsigned char cgain)
{
	float lux = 0.0 ;
#if	SUPPORT_CG5162
#define FIX_VAL  (15 * 148 * 22)
#define FIX_BASE (1000)
  
  unsigned char ReadData ,data[4] ;
	unsigned short CH0Value = 0;
	unsigned short CH1Value = 0;
	
  I2C_ReadFromSlave(&CG5162_Attr, CG5162TC_REG_CH0LB ,(unsigned char *)&ReadData);
  CH0Value = ReadData ;
  I2C_ReadFromSlave(&CG5162_Attr, CG5162TC_REG_CH0HB ,(unsigned char *)&ReadData);
	CH0Value = (unsigned short)(CH0Value | (ReadData << 8));	
  
  I2C_ReadFromSlave(&CG5162_Attr, CG5162TC_REG_CH1LB ,(unsigned char *)&ReadData);
  CH1Value = ReadData ;
  I2C_ReadFromSlave(&CG5162_Attr, CG5162TC_REG_CH1HB ,(unsigned char *)&ReadData);
	CH1Value = (unsigned short)(CH1Value | (ReadData << 8 ));	
  
  if( CH0Value < 65535) {
    if( CH0Value > CH1Value) {
      CH0Value -= CH1Value ;
    }
    else {
      CH0Value = 0 ;
    }
  }
  lux = (float)(( (unsigned int)CH0Value * FIX_VAL ) / ( (unsigned int)cgain * tig * FIX_BASE ));
#endif
	return (float)(lux);
	
}

#define IRLED_DIM0_LOW  MMPF_PIO_SetData(MMP_GPIO25 , 0 , MMP_TRUE)
#define IRLED_DIM1_LOW  MMPF_PIO_SetData(MMP_GPIO24 , 0 , MMP_TRUE)
#define IRLED_DIM2_LOW  MMPF_PIO_SetData(MMP_GPIO23 , 0 , MMP_TRUE)
#define IRLED_DIM3_LOW  MMPF_PIO_SetData(MMP_GPIO22 , 0 , MMP_TRUE)
#define IRLED_DIM0_HIGH MMPF_PIO_SetData(MMP_GPIO25 , 1 , MMP_TRUE)
#define IRLED_DIM1_HIGH MMPF_PIO_SetData(MMP_GPIO24 , 1 , MMP_TRUE)
#define IRLED_DIM2_HIGH MMPF_PIO_SetData(MMP_GPIO23 , 1 , MMP_TRUE)
#define IRLED_DIM3_HIGH MMPF_PIO_SetData(MMP_GPIO22 , 1 , MMP_TRUE)

#define IRCUT_CTRL_LOW  MMPF_PIO_SetData(MMP_GPIO9 , 0 , MMP_TRUE)
#define IRCUT_CTRL_HIGH MMPF_PIO_SetData(MMP_GPIO9 , 1 , MMP_TRUE)
              
// LD config
#define IRLED_CTRL_LOW  MMPF_PIO_SetData(MMP_GPIO6 , 0 , MMP_TRUE)
#define IRLED_CTRL_HIGH MMPF_PIO_SetData(MMP_GPIO6 , 1 , MMP_TRUE)

void change_effect_mode(unsigned char mode)
{
    printc("%s mode %d\r\n", __func__, mode);
    MMPF_Sensor_SetTest(0, mode);
}
              
void IRCut_OnOff(int on)
{ 
  if(on) {
    IRCUT_CTRL_LOW;   // IRCut On, IRLedmpff 
    #ifdef PROJECT_LD
    IRLED_CTRL_LOW;  // LD config
    #endif
  }
  else {
    IRCUT_CTRL_HIGH ;
    #ifdef PROJECT_LD
    IRLED_CTRL_HIGH  ; // LD config
    #endif
  }
}

void IRLed_OnOff(unsigned char led_on_level)
{
#define INVERT_LED_VAL 1
#define IRLED_MIN 0
#define IRLED_MASK 0xf

  unsigned char led_val = led_on_level ;
  #if INVERT_LED_VAL
  led_val = ~led_val ;
  #endif
  led_val = IRLED_MASK & led_val;
  if( led_val & 0x1) {
    IRLED_DIM0_LOW ;  
  }
  else {
    IRLED_DIM0_HIGH ;  
  }
  if( led_val & 0x2) {
    IRLED_DIM1_LOW ;    
  }
  else {
    IRLED_DIM1_HIGH ;  
  }
  if( led_val & 0x4) {
    IRLED_DIM2_LOW ;  
  }
  else {
    IRLED_DIM2_HIGH ;  
  }
  if( led_val & 0x8) {
    IRLED_DIM3_LOW ;    
  }
  else {
    IRLED_DIM3_HIGH ;  
  }
  IRLedLevel = led_on_level ;
}

unsigned char ALS_Threshold(
  unsigned int   lux,
  unsigned int   lux_H,
  unsigned int   lux_L, 
  
  unsigned int   temp_C,
  unsigned int   temp_H,
  unsigned int   temp_L,
  unsigned char  led_on_val ) // 0 ~ 255 , 0 is led off
{
extern  MMP_BOOL MMPF_Sensor_SetNightVisionMode(MMP_UBYTE ubSnrSel,MMP_UBYTE val) ;

  unsigned char  nv_mode ; // night vision mode
  unsigned char irled = IRLED_OFF ;  
  nv_mode = NightVision ;
  switch( nv_mode ) {
    case 0: // IRLED off
      if(IRLedStatus == IRLED_ON ) {
        irled   = IRLED_OFF ;
      }
      break ;
    case 1:
      if( temp_C != (unsigned int)-1) {
        if( IRLedStatus == IRLED_ON ) {
          if( temp_C >= temp_H ) {
            irled = IRLED_OFF ;
          } 
          else {
            irled = IRLED_ON  ;
          }
        }
        else {
          if ( temp_C < temp_L ) {
            irled = IRLED_ON;
          }  
          else {
            irled = IRLED_OFF;
          }
        }
      }
      else {
        irled = IRLED_ON ;  
      }
      break ;   
    case 2: 
      // auto  
      if( lux != (unsigned int)-1 ) {
      	if (IRLedStatus == IRLED_OFF) {
      		if (lux <=  lux_L ) {
      			irled = IRLED_ON ;
      		}
      		else {
      		  irled = IRLED_OFF;
      		}
      	}
      	else {
      		if (lux > lux_H ) {
            irled = IRLED_OFF ;
      		}		
      		else {
      		  irled = IRLED_ON ;
      		} 
      	}
      }
      else {
        irled = IRLedStatus ;
      }
      // after lux say irled is on...     
      if( temp_C != (unsigned int)-1 ) {    	
        if(irled == IRLED_ON ) {
          if ( temp_C >= temp_H ) {
            irled = IRLED_OFF ;
          }
          else if ( temp_C < temp_L ) {
            irled = IRLED_ON ;
          }
        } 
        else {
        //  if ( temp_C < temp_L ) {
        //    irled = IRLED_ON ;
        //  }
        }
      }
      else {
      //  irled =IRLedStatus ;
      }
      break;
  }
  
  if( irled == IRLED_ON ) {
    MMPF_Sensor_SetNightVisionMode(0,1) ;
    #if SUPPORT_IRLED
    IRCut_OnOff( 0 ) ; // IRCut off
    IRLed_OnOff( led_on_val ) ;
    #endif
  }
  else {
    #if SUPPORT_IRLED
    IRCut_OnOff( 1 ) ; // IRCut on
    IRLed_OnOff( 0 );
    #endif
    MMPF_Sensor_SetNightVisionMode(0,0) ;
  }
  
	IRLedStatus = irled ;
  NightVision = nv_mode ;
  printc("#LED : %d, NV : %d,Lux : %d(%d,%d),Temp : %d(%d,%d)\r\n",IRLedStatus,NightVision,lux,lux_L,lux_H,temp_C,temp_L,temp_H);
	return irled ;
}

void ALS_DeInit(void)
{
#if  SUPPORT_CG5162 
	I2C_WriteToSlave( &CG5162_Attr, CG5162TC_REG_OPMODE , CG5162TC_DATA_POWERDOWN );
#endif	
}
#endif

#if defined(ALL_FW)
unsigned char ALS_GetNightVision(void)
{
  return NightVision ;
}

void ALS_SetNightVision(unsigned char nv_mode)
{
  NightVision = nv_mode  ;
}
#endif

#if (SUPPORT_LM75A==1) && defined(ALL_FW)

int I2C_ReadFromSlaveMode1(MMP_I2CM_ATTR *attr,unsigned char reg, unsigned char *data,int cnt)
{
static  MMP_USHORT temp[10] ;
  int i ;
  MMPF_I2cm_WriteNoRegMode(attr,reg) ;
  MMPF_I2cm_ReadNoRegMode(attr, temp,cnt );
  for(i=0;i<cnt;i++) {
    data[i] = temp[i] & 0xFF ;
//    printc("[MISC.R%d]=%x\r\n",i,data[i]);
  }
  return 0;
}




void LM75A_Init(void)
{
  Temperature_Conf();
  Temperature_Read();
}


int Temperature_Read(void)
{
  static int gpio7_val=0;
  int g7_val = 0 ;
	unsigned char val[2] ;
	unsigned short temp_v = 0;
  int temp ;
  I2C_ReadFromSlaveMode1( &LM75A_Attr ,LM75A_REG_TEMP ,(unsigned char *)val ,2 );
	temp_v = (unsigned short)val[1]|(unsigned short) val[0] << 8;
	temp_v = temp_v >> 5  ;
	if( temp_v & 0x400 ) {
	  temp = (int)( temp_v - 2048 );
	}
	else {
	  temp = temp_v ;
	}
	// use float if you want !
	temp = temp / 8 ;
#if 0 // test on off
  if(temp < 50) {
    g7_val = 0 ;
  }
  else {
    //MMPF_PIO_SetData(MMP_GPIO7  , 1 , MMP_TRUE); 
    g7_val = 1 ;
  }
  if(g7_val!=gpio7_val) {
    MMPF_PIO_SetData(MMP_GPIO7  , g7_val , MMP_TRUE); 
    gpio7_val = g7_val ;
  }  
#endif	
	return temp ;
}

void Temperature_Conf(void)
{
	unsigned char Ctrl_Reg;
  I2C_ReadFromSlaveMode1( &LM75A_Attr ,LM75A_REG_CONF ,(unsigned char *)&Ctrl_Reg ,1 );
	//Ctrl_Reg = ReadData[0];
	Ctrl_Reg |= 0x02;			// set OS operation mode to interrupt
	Ctrl_Reg &= ~0x01;			// set device operation mode to normal mode
	// TBD
	I2C_WriteToSlave( &LM75A_Attr, LM75A_REG_CONF , Ctrl_Reg );
}
#endif

#if (SUPPORT_IRLED==1) && defined(ALL_FW)
void IRLED_Init(void)
{
//>>IR CUT control: GPIO 9
//>>IR LED control (DM0~DM3): GPIO 25~22
//>>Temperature INT: GPIO 7 -> Read by Linux, not here
// switch to GPIO mode
  MMPF_PIO_Enable(MMP_GPIO9 ,MMP_TRUE);
  MMPF_PIO_Enable(MMP_GPIO25,MMP_TRUE);
  MMPF_PIO_Enable(MMP_GPIO24,MMP_TRUE);
  MMPF_PIO_Enable(MMP_GPIO23,MMP_TRUE);
  MMPF_PIO_Enable(MMP_GPIO22,MMP_TRUE);
  // for LD project
#ifdef PROJECT_LD  
  MMPF_PIO_Enable(MMP_GPIO6 ,MMP_TRUE);
#endif  
/*
  MMPF_PIO_SetData(MMP_GPIO9  , 0 , MMP_TRUE); 
  MMPF_PIO_SetData(MMP_GPIO25 , 0 , MMP_TRUE); 
  MMPF_PIO_SetData(MMP_GPIO24 , 0 , MMP_TRUE); 
  MMPF_PIO_SetData(MMP_GPIO23 , 0 , MMP_TRUE); 
  MMPF_PIO_SetData(MMP_GPIO22 , 0 , MMP_TRUE); 
*/
  IRLed_OnOff(0);  
  
// ouput enable
  MMPF_PIO_EnableOutputMode( MMP_GPIO9  , MMP_TRUE , MMP_TRUE );
  MMPF_PIO_EnableOutputMode( MMP_GPIO25 , MMP_TRUE , MMP_TRUE );
  MMPF_PIO_EnableOutputMode( MMP_GPIO24 , MMP_TRUE , MMP_TRUE );
  MMPF_PIO_EnableOutputMode( MMP_GPIO23 , MMP_TRUE , MMP_TRUE );
  MMPF_PIO_EnableOutputMode( MMP_GPIO22 , MMP_TRUE , MMP_TRUE );
#ifdef PROJECT_LD
  // for LD project
  MMPF_PIO_EnableOutputMode( MMP_GPIO6  , MMP_TRUE , MMP_TRUE );
#endif  
}

#endif
