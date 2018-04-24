/**
 @file ex_vidrec.h
 @brief Header file for video record related sample code
 @author Alterman
 @version 1.0
*/
#ifndef _EX_MISC_H_
#define _EX_MISC_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "aitu_calendar.h"
#include "mmpf_pio.h"
#include "mmpf_saradc.h"
#include "mmpf_pwm.h"
#include "mmpf_rtc.h"
//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================
typedef struct _PWM_ATTR {
  MMP_GPIO_PIN gpio;  
  MMP_PWM_PIN  pwm ;  
  int freq ;
  int duty ;
  int on   ;
} PWM_ATTR ;

#define LED_R	0
#define LED_L	1

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================
void IO_Init(void);

int PWM_Init(void);
int PWM_Enable(int pwm_n,int en);
int PWM_Set(int pwn_n, unsigned int freq , unsigned int duty );

void LED_Off(unsigned int num);
void LED_On(unsigned int num);
void LED_Flash(unsigned int num);
void LED_Beat(unsigned int num);

int RTC_Init(void);
int RTC_SetTime(AUTL_DATETIME *dt);
int RTC_GetTime(AUTL_DATETIME *dt);
int RTC_SetAlarm(int enable,AUTL_DATETIME *alarmt,RtcAlarmCallBackFunc *cb) ;


int ADC_Init(void);

int IRLED_Init(void);

// channel is from 0
unsigned short ADC_Get(int channel);
unsigned int ADC_Get_Effect(void);


unsigned int LightSensorLux(void) ;
unsigned int TempSensorVal(void) ;
void SetNightVision(unsigned char nv_mode);

extern unsigned int key_map;

#endif //_EX_MISC_H_
