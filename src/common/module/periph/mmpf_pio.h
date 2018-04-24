#ifndef _MMPF_PIO_H_
#define _MMPF_PIO_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "os_wrap.h"
#include "mmp_gpio_inc.h"

//==============================================================================
//
//                              OPTIONS
//
//==============================================================================

#define PIO_INTERRUPT_EN        (0) // Enable or disable GPIO ISR

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define  PIO_BITPOSITION_INFO   0x1F
#define  PIO_GET_INDEX(x)       (x >> 5)
#define  PIO_SEM_TIMEOUT        0x0

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

MMP_ERR MMPF_PIO_Initialize(void);
void MMPF_PIO_Enable(MMP_GPIO_PIN piopin, MMP_BOOL bEnable);
void MMPF_PIO_WakeUpEnable(MMP_BOOL bEnable);
MMP_ERR MMPF_PIO_EnableOutputMode(MMP_GPIO_PIN piopin, MMP_BOOL bEnable,
                                    MMP_BOOL bEnableOsProtect);
MMP_ERR MMPF_PIO_SetData(MMP_GPIO_PIN piopin, MMP_UBYTE outputValue,
                                    MMP_BOOL bEnableOsProtect);
MMP_ERR MMPF_PIO_GetData(MMP_GPIO_PIN piopin, MMP_UBYTE *returnValue);
MMP_ERR MMPF_PIO_EnableTrigMode(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG trigmode,
                                    MMP_BOOL bEnable, MMP_BOOL bEnableOsProtect);
MMP_ERR MMPF_PIO_EnableInterrupt(MMP_GPIO_PIN piopin, MMP_BOOL bEnable,
                                    MMP_ULONG boundingTime, GpioCallBackFunc *cb,
                                    MMP_BOOL bEnableOsProtect);
MMP_ERR MMPF_PIO_ClearHostStatus(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG int_sr);
MMP_BOOL MMPF_PIO_GetHostStatus(MMP_GPIO_PIN piopin, MMP_GPIO_TRIG int_type);
void MMPF_PIO_ISR(void);
MMP_ERR MMPF_PIO_PadConfig(MMP_GPIO_PIN piopin, MMP_UBYTE bConfig, MMP_BOOL bOsProtectEn);

#endif //_MMPF_PIO_H_