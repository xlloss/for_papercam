//==============================================================================
//
//  File        : lib_retina.c
//  Description : Retina function library
//  Author      : Jerry Tsao
//  Revision    : 1.0
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_wd.h"
#include "mmp_reg_vif.h"
#include "mmpf_system.h"
#include "mmpf_uart.h"
#include "mmpf_vif.h"
#include "mmpf_sensor.h"

/** @addtogroup BSP
@{
*/
//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

MMP_ULONG   dbg_level   = DBG_LEVEL;

#if (DEBUG_RX_EN == 1)
#define RX_ENTER_SIGNAL (13)
#define RX_SENSITIVE	(100)
#define RX_BUF_SIZE		(128)

/* static MMP_BYTE		m_bDebugRxStrBuf[RX_BUF_SIZE]; */
/* static MMP_ULONG	m_bDebugRxStrLen = 0; */
#endif

#if defined(MINIBOOT_FW) && (ITCM_PUT == 1)
void RTNA_DBG_Str(MMP_ULONG level, const char *str) ITCMFUNC;
void RTNA_DBG_StrS(MMP_ULONG level, const char *str) ITCMFUNC;
void MMPF_DBG_Int(MMP_ULONG val, MMP_SHORT digits) ITCMFUNC;
void RTNA_DBG_Dec(MMP_ULONG level, MMP_ULONG val) ITCMFUNC;
void RTNA_DBG_Long(MMP_ULONG level, MMP_ULONG val) ITCMFUNC;
void RTNA_DBG_Short(MMP_ULONG level, MMP_USHORT val) ITCMFUNC;
void RTNA_DBG_Byte(MMP_ULONG level, MMP_UBYTE val) ITCMFUNC;
#endif

//==============================================================================
//
//                              DBG Functions
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Open
//  Description : Initialize and enable RTNA debug output

//------------------------------------------------------------------------------
void RTNA_DBG_Open(MMP_ULONG fclk, MMP_ULONG baud)
{
	MMP_UART_ATTR	uartAttr;

	uartAttr.bParityEn = MMP_FALSE;
	uartAttr.parity = MMP_UART_PARITY_ODD;
	uartAttr.bFlowCtlEn = MMP_FALSE;
	uartAttr.ubFlowCtlSelect = 1;
	uartAttr.padset = DEBUG_UART_PAD;
	uartAttr.ulMasterclk = fclk * 1000;
	uartAttr.ulBaudrate = baud;

	MMPF_Uart_Open(DEBUG_UART_NUM, &uartAttr);
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Str
//  Description : Debug output a string
//------------------------------------------------------------------------------
void RTNA_DBG_Str(MMP_ULONG level, const char *str)
{
    int			size;
    const char	*pchar;

    if(level > dbg_level)
    {
        return;
    }

    size = 0;
    pchar = str;
    while(*pchar++ != 0)
    {
        size++;
    }

#if (uart_printc == 0)
    MMPF_Uart_Write(DEBUG_UART_NUM, str, size);
#else
    MMPF_Uart_Write(DEBUG_UART_NUM, str);
#endif
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_StrS
//  Description : Debug output a string

//------------------------------------------------------------------------------
void RTNA_DBG_StrS(MMP_ULONG level, const char *str)
{
    int		size;
    const char  *pchar;
    char 	c = '\r';
  	
    if (level > dbg_level) {
        return;
    }

    size = 0;
    pchar = str;
    while(*pchar != 0) {
    	if (*pchar == '\n') {
         #if (uart_printc ==0 )
			MMPF_Uart_Write(DEBUG_UART_NUM, str, size);
			MMPF_Uart_Write(DEBUG_UART_NUM, &c, 1);
         #else
			MMPF_Uart_Write(DEBUG_UART_NUM, str);
			MMPF_Uart_Write(DEBUG_UART_NUM, &c);
         #endif

			str += size;
			size = 1;
    	}
    	else {
	        size++;
    	}
    	pchar++;
    }
    #if (uart_printc ==0)
	MMPF_Uart_Write(DEBUG_UART_NUM, str, size);
    #else
	MMPF_Uart_Write(DEBUG_UART_NUM, str);
    #endif

}

/** @brief Print the number via UART

@post The UART is initialized in current CHIP frequency.
@param[in] val The value to be printeded.
@param[in] digits The number of digits to be printed in hexadecimal or decimal. If the digits is > 0,
                  use hexadecimal output, otherwise it's a decimal output.
@remarks When outputting a decimal, the val is always be regarded as a signed value.
         When outputting a hexadecimal, the val is always be regarded as an unsigned value.
@note Using signed digits would cause confusing sometimes but it could save extra 4 bytes each calling.
*/

// 108 bytes, extra 38 bytes to RTNA_DBG_Hex
extern unsigned divu10(unsigned n) ;
extern int remu10(unsigned n);
#if (MINIBOOT_FW)
unsigned divu10(unsigned n) {
 unsigned q, r;
 q = (n >> 1) + (n >> 2);
 q = q + (q >> 4);
 q = q + (q >> 8);
 q = q + (q >> 16);
 q = q >> 3;
 r = n - q*10;
 return q + ((r + 6) >> 4);
// return q + (r > 9);
}

int remu10(unsigned n) {
     static char table[16] = {0, 1, 2, 2, 3, 3, 4, 5,
          5, 6, 7, 7, 8, 8, 9, 0};
      n = (0x19999999*n + (n >> 1) + (n >> 3)) >> 28;
       return table[n];
}
#endif
void MMPF_DBG_Int(MMP_ULONG val, MMP_SHORT digits)
{
	char		str[12];
	MMP_USHORT	i;
    MMP_SHORT	base;
	MMP_SHORT	stopAt;

	str[0] = ' ';
	str[1] = 'x';
	if(digits < 0)
	{
        base = 10;
		digits = -digits;
		stopAt = 1;
		if(val > 0x80000000)
		{
			val = -val;
			str[0] = '-';
		}
	}
	else
	{
        base = 16;
		stopAt = 2;
	}

	for(i = digits + stopAt - 1; i >= stopAt; i--)
	{
		MMP_USHORT	num;
        num = val % base;
        
		if(num >= 10)
		{
            str[i] = num + ('A' - 10);
		}
		else
		{
			str[i] = num + '0';
		}

        val /= base;
	}

	str[stopAt + digits] = '\0';

	RTNA_DBG_Str(0, str);
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Dec
//  Description : Debug output a long number

//------------------------------------------------------------------------------
void RTNA_DBG_Dec(MMP_ULONG level, MMP_ULONG val)
{
	if(level <= dbg_level)
	{
		MMPF_DBG_Int(val, -10);
	}
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Long
//  Description : Debug output a long number

/* //------------------------------------------------------------------------------ */
void RTNA_DBG_Long(MMP_ULONG level, MMP_ULONG val)
{
    if(level <= dbg_level)
    {
        MMPF_DBG_Int(val, 8);
    }
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Short
//  Description : Debug output a short number

//------------------------------------------------------------------------------
void RTNA_DBG_Short(MMP_ULONG level, MMP_USHORT val)
{
	if(level <= dbg_level)
	{
		MMPF_DBG_Int(val, 4);
	}
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Byte
//  Description : Debug output a byte number

//------------------------------------------------------------------------------
void RTNA_DBG_Byte(MMP_ULONG level, MMP_UBYTE val)
{
    if(level <= dbg_level)
    {
        MMPF_DBG_Int(val, 2);
    }
}

#if (DEBUG_RX_EN == 1) && (UART_RXINT_MODE_EN == 1)

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_RxCallback
//  Description : Callback to handle input data via FIFO port

//------------------------------------------------------------------------------
void RTNA_DBG_RxCallback(MMP_UBYTE size, volatile MMP_UBYTE *fifo)
{
	MMP_LONG	i;
	MMP_BYTE	rx_byte;

	for(i = 0; i < size; i++)
	{
		rx_byte = *fifo;
		m_bDebugRxStrBuf[(m_bDebugRxStrLen + i) % RX_BUF_SIZE] = rx_byte;

		// Uart Echo
         #if (uart_printc ==0 )
		MMPF_Uart_Write(DEBUG_UART_NUM, &rx_byte, 1);
         #else 
		MMPF_Uart_Write(DEBUG_UART_NUM, &rx_byte);
         #endif
	}

	m_bDebugRxStrLen += size;
	if(m_bDebugRxStrLen >= RX_BUF_SIZE)
	{
		RTNA_DBG_Str(0, "Error: UART RX overflow\r\n");
	}
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_Gets
//  Description : Debug read input string
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)

/* */
void RTNA_DBG_Gets(char *str, MMP_ULONG *size)
{
	MMP_LONG	i;
	MMP_BOOL	bEnterDetect = MMP_FALSE;

	if(MMPF_Uart_IsRxEnable(DEBUG_UART_NUM) == MMP_FALSE)
	{
		MMPF_Uart_EnableRx(DEBUG_UART_NUM, 1, (UartCallBackFunc *) &RTNA_DBG_RxCallback);
	}

	m_bDebugRxStrLen = 0;
	do
	{
		for(i = m_bDebugRxStrLen; i >= 0; i--)
		{
			if(m_bDebugRxStrBuf[i] == RX_ENTER_SIGNAL)
			{
				*size = i;	//Copy string except the "Enter" signal
				MEMCPY(str, m_bDebugRxStrBuf, i);

				str[i] = '\0';
				bEnterDetect = MMP_TRUE;
				MEMSET0(&m_bDebugRxStrBuf);
				break;
			}
		}

		MMPF_OS_Sleep(RX_SENSITIVE);
	} while(bEnterDetect == MMP_FALSE);

	MMPF_Uart_DisableRx(DEBUG_UART_NUM);
}

//------------------------------------------------------------------------------
//  Function    : RTNA_DBG_GetChar_NoWait
//  Description : get UART input and no wait. If no data return 0

//------------------------------------------------------------------------------
unsigned char RTNA_DBG_GetChar_NoWait()
{
	if(m_bDebugRxStrLen != 0)
	{
		m_bDebugRxStrLen = 0;
		return m_bDebugRxStrBuf[0];
	}

	return 0;
}
#endif // !defined(MINIBOOT_FW)
#endif

//==============================================================================
//
//                              MISC. Functions
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : RTNA_Wait_Count
//  Description : RTNA delay loop function
//------------------------------------------------------------------------------

//[jerry] it may not a ciritical function, call it by function.
// It's about 4 cycle one loop in ARM mode
__attribute__((optimize("O0")))
void RTNA_Wait_Count(MMP_ULONG count)
{
	while(count--);
}


//------------------------------------------------------------------------------
//  Function    : RTNA hardware initialization
//  Description : Initialize RTNA register status.
//------------------------------------------------------------------------------
void RTNA_Init(void)
{
    AITPS_GBL   pGBL  = AITC_BASE_GBL;
	AITPS_VIF   pVIF  = AITC_BASE_VIF;
	MMP_BOOL	bClockEnable = 0;
	#if !defined(ALL_FW)
	AITPS_WD 	pWD   = AITC_BASE_WD;
	#endif

    MMPF_SYS_ReadCoreID();

	bClockEnable = (pGBL->GBL_CLK_DIS[0] & GBL_CLK_VIF) ? MMP_FALSE : MMP_TRUE;

	if (bClockEnable) {
	    pVIF->VIF_OPR_UPD[MMPF_VIF_MDL_ID0] = VIF_OPR_UPD_EN;
	    pVIF->VIF_OPR_UPD[MMPF_VIF_MDL_ID1] = VIF_OPR_UPD_EN;
	}
    else {
	    pGBL->GBL_CLK_EN[0] = GBL_CLK_VIF;
	    pVIF->VIF_OPR_UPD[MMPF_VIF_MDL_ID0] = VIF_OPR_UPD_EN;
	    pVIF->VIF_OPR_UPD[MMPF_VIF_MDL_ID1] = VIF_OPR_UPD_EN;
	    pGBL->GBL_CLK_DIS[0] = GBL_CLK_VIF;
	}
	
	// Disable watchdog
	#if !defined(ALL_FW)
	pWD->WD_MODE_CTL0 = WD_CTL_ACCESS_KEY;
	#endif
	
    MMPF_SYS_StartTimerCount();
}

//==============================================================================
//
//                    Complement Missing ISR Functions
//
//==============================================================================

/// @brief keep integrity between lib_retina.c and irq.s

#if defined(ALL_FW)
#define COMPLEMENT_MISSING_ISR
#if (!DSC_R_EN)
void MMPF_JPG_ISR(void) {}
#endif
#if (!USB_EN)
void MMPF_USB_ISR(void) {}
#endif
#if (!VIDEO_R_EN)
void MMPF_VIDEO_ISR(void) {}
void MMPF_H264ENC_ISR(void) {}
#endif
#if (!SENSOR_EN)
void MMPF_VIF_ISR(void) {}
void MMPF_ISP_ISR(void) {}
#endif
#if (!VIDEO_P_EN)
void MMPF_DISPLAY_ISR(void) {}
#endif
#endif

#if defined(UPDATER_FW)||defined(MBOOT_FW)||defined(MINIBOOT_FW)||defined(MBOOT_EX_FW)
#define COMPLEMENT_MISSING_ISR
void MMPF_DISPLAY_ISR(void) {}
#endif

#if (!USING_SM_IF)
MMP_ERR MMPF_NAND_ReadSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount){return 1;}
MMP_ERR MMPF_NAND_WriteSector(MMP_ULONG dmastartaddr, MMP_ULONG startsect, MMP_USHORT sectcount){return 1;}
MMP_ERR MMPF_NAND_GetSize(MMP_ULONG *pSize){*pSize = 0;return 1;}
MMP_ERR MMPF_NAND_InitialInterface(void){return 1;}
MMP_ERR MMPF_NAND_Reset(void){return 1;}
MMP_ERR MMPF_NAND_LowLevelFormat(void){return 1;}
MMP_ERR MMPF_NAND_FinishWrite(void){return 1;}
void MMPF_NAND_ISR(void){}
#endif

#ifndef COMPLEMENT_MISSING_ISR
/**  Keep this invalid variable so that compiler can automatically detect the problem
  *  while new package forget to define missing ISR.
  */
//Please_Read_Me;
/**
 * If you encountered an compiler error, you have to complement the missing ISR here.
 *
 * Each package has to add its own section here, or else compiler would occur error.
 * @todo Each section has to put it's missing ISR with empty function here.
 *       You can reference the existing code and compiler messages for what ISR you're missing
 *       after #define COMPLEMENT_MISSING_ISR.
 */
#endif
// Truman Logic --

/** @}*/ //BSP
