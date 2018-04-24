//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

/**
 @file ex_misc.c
 @brief IO sample code ( PWM / ADC / RTC )
 @author Sean
 @version 1.0
*/

#include "lib_retina.h"
#include "ait_utility.h"
#include "config_fw.h"
#include "mmp_err.h"
#include "mmpf_rtc.h"
#include "mmpf_pwm.h"
#include "mmpf_saradc.h"
#include "mmpf_pio.h"
#include "isp_if.h"
#include "ex_misc.h"
#include "misc.h"


#define KEY_0 115
#define KEY_1 251
#define KEY_2 364
#define KEY_3 470
#define ADC_OFFSET 30


extern MMP_UBYTE    UI_Get_MdEnable(void); 
extern MMP_UBYTE    UI_Get_NightVision(void);
extern MMP_UBYTE    UI_Get_LightSensorTH_High(void);
extern MMP_UBYTE    UI_Get_LightSensorTH_Low(void);
extern MMP_UBYTE    UI_Get_TempSensorTH_High(void);
extern MMP_UBYTE    UI_Get_TempSensorTH_Low(void);
// here return 0~255
extern MMP_UBYTE    UI_Get_IRLED_Level(void);
 


OS_STK  MISC_IO_Task_Stk[TASK_B_MISC_IO_SIZE];
 
#if SUPPORT_PWM
static PWM_ATTR ait_pwm[] = {
  {MMP_GPIO3,MMP_PWM0_PIN_AGPIO1, 10000,0,0},
  {MMP_GPIO4,MMP_PWM1_PIN_AGPIO2, 10000,0,0},
  {MMP_GPIO5,MMP_PWM2_PIN_AGPIO3, 10000,0,0}
} ;

int PWM_Init(void)
{
  MMP_ERR err = MMPF_PWM_Initialize();
  return (err)?-1:0 ;
}

int PWM_Enable(int pwm_n,int en)
{
  if( en ) {
    if(!ait_pwm[pwm_n].on) {
      MMPF_PIO_Enable((MMP_GPIO_PIN) ait_pwm[pwm_n].gpio, MMP_FALSE); 
      ait_pwm[pwm_n].on = 1;
    }
  }
  else {
    if(ait_pwm[pwm_n].on) {
      MMPF_PWM_ControlSet(PWM_GET_ID(ait_pwm[pwm_n].pwm), 0);
      MMPF_PWM_EnableOutputPin(ait_pwm[pwm_n].pwm, MMP_FALSE);
      // switch to gpio mode
      MMPF_PIO_Enable((MMP_GPIO_PIN) ait_pwm[pwm_n].gpio, MMP_TRUE);
      ait_pwm[pwm_n].on = 0 ;
    }
  }
  return 0;
}

int PWM_Set(int pwm_n, unsigned int freq , unsigned int duty )
{
    if(duty > 100) {
      duty = 100 ;
    }
    MMPF_PWM_SetFreqDuty( ait_pwm[pwm_n].pwm,freq,duty) ;
    return 0;  
}
#endif

void LED_Off(unsigned int num)
{
	if (num <= LED_L) {
#if SUPPORT_PWM
		PWM_Set(num, 1, 0);
		PWM_Enable(num, 0);
#endif
	}
}

void LED_On(unsigned int num)
{
	if (num <= LED_L) {
#if SUPPORT_PWM
		PWM_Set(num, 1000, 30);
		PWM_Enable(num, 1);
#endif
	}
}

void LED_Flash(unsigned int num)
{
	if (num <= LED_L) {
#if SUPPORT_PWM
		PWM_Set(num, 2, 50);
		PWM_Enable(num, 1);
#endif
	}
}

void LED_Beat(unsigned int num)
{
	if (num <= LED_L) {
#if SUPPORT_PWM
		PWM_Set(num, 1, 50);
		PWM_Enable(num, 1);
#endif
	}
}

#if SUPPORT_RTC
static AUTL_DATETIME m_RtcDateTime = {
        2017,   ///< Year
        5,      ///< Month: 1 ~ 12
        17,      ///< Day of month: 1 ~ 28/29/30/31
        0,      ///< Sunday ~ Saturday
        0,      ///< 0 ~ 11 for 12-hour, 0 ~ 23 for 24-hour
        0,      ///< 0 ~ 59
        0,      ///< 0 ~ 59
        0,      ///< AM: 0; PM: 1 (for 12-hour only)
        0       ///< 24-hour: 0; 12-hour: 1	
};
int RTC_Init(void)
{
  int ret = 0;
  #if SUPPORT_RTC==INTERNAL_RTC
  //TBD : Don't install interrupt in timelapse case
  //MMP_ERR err = MMPF_RTC_Initialize();
  //if(err) {
  //  ret = -1 ;
  //}
  #endif
  #if SUPPORT_RTC==EXTERNAL_RTC
  ret = -1 ;
  #endif
  return ret ;  
}

int RTC_SetTime(AUTL_DATETIME *dt) 
{
  int ret = 0 ;
  #if SUPPORT_RTC==INTERNAL_RTC
  MMP_ERR err = MMPF_RTC_SetTime(dt);
  if(err) {
    ret = -1 ;  
  }
  #endif
  #if SUPPORT_RTC==EXTERNAL_RTC
  // porting
  #endif
  return ret ;
}

int RTC_GetTime(AUTL_DATETIME *dt)
{
  
  int ret = 0 ;
  #if SUPPORT_RTC==INTERNAL_RTC
  MMP_ERR err = MMPF_RTC_GetTime(dt);
  if(err) {
    ret = -1 ;
  }
  #endif
  #if SUPPORT_RTC==EXTERNAL_RTC
  // porting
  #endif
  
  return ret ;
}

int RTC_SetAlarm(int enable,AUTL_DATETIME *alarmt,RtcAlarmCallBackFunc *cb) 
{
  int ret = 0;
  #if SUPPORT_RTC==INTERNAL_RTC
  MMP_ERR err = MMPF_RTC_SetAlarmEnable(enable,alarmt,cb);
  if(err) {
    ret = -1 ;
  }
  #endif
  #if SUPPORT_RTC==EXTERNAL_RTC
  // porting
  #endif
  return ret ;
}

void dumpRTCinfo(AUTL_DATETIME *ptime)
{
    printc("%d:%d:%d:%d:%d:%d \r\n",ptime->usYear,ptime->usMonth, ptime->usDay, ptime->usHour, ptime->usMinute, ptime->usSecond);
}


void RTC_SetalarmPeriodMin(MMP_ULONG period)
{
    AUTL_DATETIME *dt = &m_RtcDateTime;
    /* RTC_Init(); */ 
    if( RTC_GetTime(dt) != MMP_ERR_NONE ) {
        RTC_SetTime(dt);
        printc("Initial RTC time by default\r\n");
    }
    //Every RTC_GetTime, it will get run_sec: 1sec, add into the m_RtcBaseTime(base date) to calculate
    //so we just need add 1sec into the alram period, to eliminate the offset
    dt->usSecond =  dt->usSecond  + 1;
    dt->usMinute =  (dt->usMinute + period );
    RTC_SetAlarm(1,dt,NULL);

}
#endif

#if SUPPORT_ADC
int ADC_Init(void)
{
    MMPF_SARADC_ATTR sar_att;
    sar_att.TPWait=1;
    MMPF_SARADC_Init(&sar_att);
    return 0 ;
}

// channel is from 0
unsigned short ADC_Get(int channel)
{
    MMP_USHORT val ;
		MMPF_SARADC_SetChannel(channel+1);  
		MMPF_SARADC_GetData( (MMP_USHORT *)&val);  
		return val ;
}

unsigned int ADC_Get_Effect(void)
{
	unsigned int key_map;
    int adc_channel = 1;
    unsigned short adc_value;
	
    adc_value = ADC_Get(adc_channel);
    printc("adc_value %d\r\n", adc_value);
    MMPF_OS_Sleep(10);
    if (adc_value < (KEY_0 + ADC_OFFSET) && adc_value > (KEY_0 - ADC_OFFSET)) {
		key_map = ISP_IMAGE_EFFECT_SEPIA;
        change_effect_mode(ISP_IMAGE_EFFECT_SEPIA);
    } else if (adc_value < (KEY_1 + ADC_OFFSET) && adc_value > (KEY_1 - ADC_OFFSET)) {
		key_map = ISP_IMAGE_EFFECT_NEGATIVE;
        change_effect_mode(ISP_IMAGE_EFFECT_NEGATIVE);
    } else if (adc_value < (KEY_2 + ADC_OFFSET) && adc_value > (KEY_2 - ADC_OFFSET)) {
        key_map = ISP_IMAGE_EFFECT_GREY;
        change_effect_mode(ISP_IMAGE_EFFECT_GREY);
    } else if (adc_value < (KEY_3 + ADC_OFFSET) && adc_value > (KEY_3 - ADC_OFFSET)) {
        key_map = ISP_IMAGE_EFFECT_NORMAL;
        change_effect_mode(ISP_IMAGE_EFFECT_NORMAL);
    } else {
        printc ("No This Key %d\r\n", adc_value);
	}

	return key_map;
}

#endif

int I2C_Init(void)
{
#if 1//SUPPORT_CG5162 // light sensor
extern void ALS_Init(int preread);
#endif
#if SUPPORT_PAS7671 // motion sensor
extern void PAS7671_Init();
#endif
#if SUPPORT_LM75A // tempature sensor
extern void LM75A_Init();
#endif

#if SUPPORT_CG5162
  ALS_Init(0);
#endif
#if SUPPORT_OPT3006
  ALS_Init(1);
#endif

#if SUPPORT_LM75A
  LM75A_Init();
#endif
#if SUPPORT_PAS7671
  PAS7671_Init( (UI_Get_MdEnable() & 0x02) ? 1 : 0);
#endif
  return 0; 
}


void IO_Init(void)
{
#if SUPPORT_IRLED
  IRLED_Init(); 
#endif
  
#if SUPPORT_PWM
  PWM_Init();
#endif  
#if SUPPORT_RTC
  RTC_Init();
#endif  
#if SUPPORT_ADC
  ADC_Init();
#endif  
  I2C_Init();
}

static unsigned int light_sensor_lux = (unsigned int)-1 ;
static unsigned int temp_sensor_val = (unsigned int)-1  ;

void MISC_IO_TaskHandler(void *p_arg)
{
extern MMP_BOOL MMPS_Sensor_IsInitialized(MMP_UBYTE ubSnrSel)  ;
static int als_cnt = 0 ;

    unsigned char cur_led ;
    int adc_channel = 0;
    unsigned short adc_value;
	MMPF_OS_FLAGS flag = 1;
	unsigned char gpio_value;

    // Quick read lux of a short TIG 
    // For ISP reference in the furture
#if SUPPORT_CG5162 || SUPPORT_OPT3006  
    light_sensor_lux = (unsigned int)ALS_ReadLux();
	//printc("Read 1st Lux : %d ,tick:%d\r\n",light_sensor_lux,OSTime);
#endif

	MMPF_PIO_GetData(MMP_GPIO113, &gpio_value);
	if (gpio_value) {
		LED_On(LED_R);
	}

#if SUPPORT_ADC
    adc_value = ADC_Get(adc_channel);
    printc("=====bat adc_value %d\r\n", adc_value);
    MMPF_OS_Sleep(10);
#endif

    while(1) {
      MMPF_OS_Sleep(500);

#if SUPPORT_LM75A
      temp_sensor_val = Temperature_Read();
#endif
      MMPF_OS_Sleep(600); // light sensor TIG switch to 400ms, total delay is 1secs
#if SUPPORT_CG5162 || SUPPORT_OPT3006  // light sensor
      /*
      start to adjust night vision mode when sensor is initialized
      */
      if( ALS_Ready() ) {
        light_sensor_lux = (unsigned int)ALS_ReadLux();      
        if (MMPS_Sensor_IsInitialized(0) == MMP_TRUE) {
          cur_led = ALS_Threshold(
                                light_sensor_lux,
                                UI_Get_LightSensorTH_High(),
                                UI_Get_LightSensorTH_Low(),
                                temp_sensor_val ,
                                UI_Get_TempSensorTH_High(),
                                UI_Get_TempSensorTH_Low() ,
                                UI_Get_IRLED_Level() ) ;
          //printc("IRLed : %s,Lux : %d\r\n",(int)(cur_led==IRLED_ON)?"LED-ON":"LED-OFF",(int)light_sensor_lux);
        } else {
          #if SUPPORT_IRLED
          IRCut_OnOff( 1 ) ; // IRCut on
          IRLed_OnOff( 0 );
          #endif
        }
      }
#endif
		if (flag) {
			flag = 0;
			LED_Off(LED_R);
		}
    }
}

unsigned int LightSensorLux(void)
{
  return light_sensor_lux ;
}

unsigned int TempSensorVal(void)
{
  return temp_sensor_val ;
}

void SetNightVision(unsigned char nv_mode)
{
  ALS_SetNightVision( nv_mode ) ;
#if SUPPORT_CG5162 || SUPPORT_OPT3006
  ALS_Threshold(light_sensor_lux,
                UI_Get_LightSensorTH_High(),
                UI_Get_LightSensorTH_Low(),
                temp_sensor_val ,
                UI_Get_TempSensorTH_High(),
                UI_Get_TempSensorTH_Low() ,
                UI_Get_IRLED_Level() ) ;
#endif
}
