//==============================================================================
//
//  File        : mmpf_rtc.c
//  Description : RTC Control Interface
//  Author      : Ben Lu
//  Revision    : 1.0
//
//==============================================================================
/**
 *  @file mmpf_rtc.c
 *  @brief The RTC control functions
 *  @author Ben Lu
 *  @version 1.0
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_rtc.h"
#include "mmpf_rtc.h"
#include "aitu_calendar.h"
#if SUPPORT_RTC
//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

static AUTL_DATETIME m_RtcBaseTime = {
	1970,   ///< Year
	1,      ///< Month: 1 ~ 12
	1,      ///< Day of month: 1 ~ 28/29/30/31
	0,      ///< Sunday ~ Saturday
	0,      ///< 0 ~ 11 for 12-hour, 0 ~ 23 for 24-hour
	0,      ///< 0 ~ 59
	0,      ///< 0 ~ 59
	0,      ///< AM: 0; PM: 1 (for 12-hour only)
	0       ///< 24-hour: 0; 12-hour: 1	
};

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static RtcAlarmCallBackFunc *cbRtcCallBack = NULL;
static AUTL_DATETIME        m_ShadowTime = {0};
static MMP_ULONG            m_ulRtcInSeconds = 0;
static MMP_ULONG            m_ulBaseTimeInSeconds = 0;

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_ForceReset
//  Description :
//------------------------------------------------------------------------------
MMP_ERR MMPF_RTC_ForceReset(void)
{
	m_ulBaseTimeInSeconds = 0;
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_ISOCtl
//  Description :
//------------------------------------------------------------------------------
/** @brief The internal function to send ISO_EN control signal.

The function to enable ISO cell 
@return It reports the status of the operation.
*/
static void MMPF_RTC_ISOCtl(void)
{
    AITPS_RTC pRTC = AITC_BASE_RTC;
	MMP_UBYTE ubCheck = 5;

    // Input ISO ctrl sequence , 1->2->3->4->5->2->1
    // The switch of state needs delay, 1ms at least according to designer,
    // but in our test, set to 3ms will still causes incorrect data read.
    // And the sequence should be finished within 1 sec.
	pRTC->RTC_SEQ = RTC_EN_SEQ_1;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == RTC_ISO_CTL_ACK) && (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_2;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == 0) && (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_3;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == RTC_ISO_CTL_ACK) && (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_4;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == 0) && (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_5;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == RTC_ISO_CTL_ACK) && (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_2;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == 0)&& (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	ubCheck = 5;
	pRTC->RTC_SEQ = RTC_EN_SEQ_1;
	while(((pRTC->RTC2DIG_CTL & RTC_ISO_CTL_ACK) == RTC_ISO_CTL_ACK)&& (ubCheck --)) {
		MMPF_OS_Sleep(1);
	}
	MMPF_OS_Sleep(5);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_Initialize
//  Description :
//------------------------------------------------------------------------------
/** @brief The function Initialize the RTC.

The function Initialize the RTC 
@return It reports the status of the operation.
*/
MMP_ERR MMPF_RTC_Initialize(void)
{
    AITPS_GBL       pGBL = AITC_BASE_GBL;
	AITPS_AIC       pAIC = AITC_BASE_AIC;
	static MMP_BOOL m_bRtcHasInit = MMP_FALSE;

    if (!m_bRtcHasInit) {
        m_bRtcHasInit = MMP_TRUE;

        RTNA_AIC_Open(pAIC, AIC_SRC_GBL, rtc_isr_a,
                  	  AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 3);
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_GBL);

        pGBL->GBL_WAKEUP_INT_CPU_SR  = RTC2DIG_INT_MASK;
        pGBL->GBL_WAKEUP_INT_CPU_EN &= ~(RTC2DIG_INT_MASK);
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_IsValid
//  Description :
//------------------------------------------------------------------------------
/** @brief The function check if current RTC is valid

The function verify the RTC status
@return It reports the status of the operation.
*/
MMP_BOOL MMPF_RTC_IsValid(void)
{
	AITPS_RTC pRTC = AITC_BASE_RTC;

    return (pRTC->RTC2DIG_CTL & RTC_VALID_STATUS) ? MMP_TRUE : MMP_FALSE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_SetTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for setting time information to RTC.

This function is used for setting time information to RTC.
@warning This function uses OS sleep, please don't call this function in ISR.
@param[in] pointer of structure AUTL_DATETIME.
@return It reports the status of the operation.
*/
MMP_ERR	MMPF_RTC_SetTime(AUTL_DATETIME *pTime)
{
    AITPS_RTC   pRTC = AITC_BASE_RTC;
    MMP_ULONG   chk_times = 5;

    if (MMPF_OS_InInterrupt()) {
        return MMP_RTC_ERR_ISR;
	}
	
    if (AUTL_Calendar_ValidateTime(pTime, &m_RtcBaseTime) == MMP_FALSE) {
        RTNA_DBG_Str(0, "Set RTC bad time\r\n");
        return MMP_RTC_ERR_FMT;
    }

    m_ulBaseTimeInSeconds = AUTL_Calendar_DateToSeconds(pTime, &m_RtcBaseTime);
	RTNA_DBG_Str(0, "RTC Set Time: ");
	RTNA_DBG_Long(0, m_ulBaseTimeInSeconds);
	RTNA_DBG_Str(0, "\r\n");
    // Set RTC Base from user
    pRTC->DIG2RTC_CTL |= RTC_BASE_WR;
    pRTC->RTC_WR_DATA  = m_ulBaseTimeInSeconds;
    MMPF_RTC_ISOCtl();
    pRTC->DIG2RTC_CTL &= ~(RTC_BASE_WR);

    // Reset RTC Counter
    pRTC->DIG2RTC_CTL |= RTC_CNT_RST | RTC_SET_VALID | RTC_SET_CNT_VALID;

    MMPF_RTC_ISOCtl();

    pRTC->DIG2RTC_CTL &= ~(RTC_CNT_RST);

    while (chk_times--) {
        if (MMPF_RTC_IsValid())
            return MMP_ERR_NONE;
	}
	
    // Retry if set RTC failed
    return MMPF_RTC_SetTime(pTime);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_GetTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting RTC information.

This function is used for getting RTC time information.
@warning This function uses OS sleep, please don't call this function in ISR.
@param[in] pointer of structure AUTL_DATETIME.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_RTC_GetTime(AUTL_DATETIME *pTime)
{
    AITPS_RTC  pRTC = AITC_BASE_RTC;
    MMP_ULONG  run_sec;
    MMP_ULONG64 ullSeconds;

    if (MMPF_OS_InInterrupt()) {
        return MMP_RTC_ERR_ISR;
	}
	
    if (MMPF_RTC_IsValid() == MMP_FALSE) {
        RTNA_DBG_Str(0, "RTC INVALID\r\n");
        return MMP_RTC_ERR_INVALID;
	}
	
    // Read RTC Base
    if (m_ulBaseTimeInSeconds == 0) {
	    do {
	        pRTC->DIG2RTC_CTL |= RTC_BASE_RD;
	        MMPF_RTC_ISOCtl();
	        m_ulBaseTimeInSeconds = pRTC->RTC_RD_DATA;
	        pRTC->DIG2RTC_CTL &= ~(RTC_BASE_RD);
	    } while(m_ulBaseTimeInSeconds & RTC_CNT_BUSY);

    	m_ulBaseTimeInSeconds = m_ulBaseTimeInSeconds & RTC_RD_DATA_MASK; // not necessary actually
		RTNA_DBG_Str(0, "GBT: ");
		RTNA_DBG_Long(0, m_ulBaseTimeInSeconds);
		RTNA_DBG_Str(0, "\r\n");
	}

    // Read RTC Counter
    do {
        pRTC->DIG2RTC_CTL |= RTC_CNT_RD;
        MMPF_RTC_ISOCtl();
        run_sec = pRTC->RTC_RD_DATA;
        pRTC->DIG2RTC_CTL &= ~(RTC_CNT_RD);
    } while(run_sec & RTC_CNT_BUSY);

    run_sec = pRTC->RTC_RD_DATA & RTC_RD_DATA_MASK; // not necessary actually

    ullSeconds = m_ulBaseTimeInSeconds + run_sec;

    if (ullSeconds > 0xFFFFFFFF) {
        ullSeconds = 0xFFFFFFFF;
	}

    m_ulRtcInSeconds = (MMP_ULONG)ullSeconds;

    AUTL_Calendar_SecondsToDate((MMP_ULONG)ullSeconds, pTime, &m_RtcBaseTime);

    MEMCPY(&m_ShadowTime, pTime, sizeof(AUTL_DATETIME));

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_GetShadowTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting the last RTC time information.

This function is used for getting the last RTC time information.
@param[out] pointer of structure AUTL_DATETIME with the last time info.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_RTC_GetShadowTime(AUTL_DATETIME *pTime)
{
    if (pTime) {
        MEMCPY(pTime, &m_ShadowTime, sizeof(AUTL_DATETIME));
        return MMP_ERR_NONE;
    }
	return MMP_RTC_ERR_PARAM;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_DateToSeconds
//  Description :
//------------------------------------------------------------------------------
MMP_BOOL MMPF_RTC_SecondsToDate(MMP_ULONG ulSeconds, AUTL_DATETIME *pTime)
{
    return AUTL_Calendar_SecondsToDate(ulSeconds, pTime, &m_RtcBaseTime);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_GetTime_InSeconds
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting RTC time in seconds "right after" MMPF_RTC_GetTime

This function is used for getting RTC time time in seconds "right after" MMPF_RTC_GetTime.
@return It reports the RTC time in seconds
*/
MMP_ULONG MMPF_RTC_GetTime_InSeconds(void)
{
    return m_ulRtcInSeconds;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_ISR
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is the interrupt service routine of RTC alarm.

@param[in] None.
@return None.
*/
void MMPF_RTC_ISR(void)
{
    AITPS_GBL   pGBL = AITC_BASE_GBL;
    AITPS_RTC   pRTC = AITC_BASE_RTC;
    MMP_UBYTE   status;

    status = pGBL->GBL_WAKEUP_INT_CPU_EN & pGBL->GBL_WAKEUP_INT_CPU_SR;
    pGBL->GBL_WAKEUP_INT_CPU_SR = status;

    if (status & RTC2DIG_INT) {
        // Disable Alarm Interrupt
        pGBL->GBL_WAKEUP_INT_CPU_EN &= ~(RTC2DIG_INT);
        // Clear Alarm Interrupt 
        pRTC->DIG2RTC_CTL |= RTC_ALARM_INT_CLR;
        
        //The ISO sequence taks too long time to execute in ISR,
        //UI flow should handle it by call MMPF_RTC_SetAlarmEnable
        //to clear alarm interrupt.
        //MMPF_RTC_ISOCtl();
        //pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_INT_CLR);

        if (cbRtcCallBack) {
            (*cbRtcCallBack)();
        }
    }
}

//------------------------------------------------------------------------------
//  Function    : MMPF_RTC_SetAlarmEnable
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for setting RTC alarm.

This function is used for setting RTC alarm.
@warning This function uses OS sleep, please don't call this function in ISR.
@param[in] bEnable : enalbe/disable RTC
@param[in] alarm_time_info : pointer of structure AUTL_DATETIME.
@param[in] p_callback : call back function when alarm time expired.
@return It reports the status of the operation.
*/
MMP_ERR MMPF_RTC_SetAlarmEnable(MMP_BOOL bEnable, AUTL_DATETIME *pAlarmTime,
                                RtcAlarmCallBackFunc *p_callback)
{
    MMP_ERR     err = MMP_ERR_NONE;
    AITPS_GBL   pGBL = AITC_BASE_GBL;
    AITPS_RTC   pRTC = AITC_BASE_RTC;
    MMP_ULONG   cur_sec, alarm_sec, alarm_sec_ofst;
    AUTL_DATETIME curTime;

    if (MMPF_OS_InInterrupt()) {
        return MMP_RTC_ERR_ISR;
	}

    if (bEnable) {
        if (AUTL_Calendar_ValidateTime(pAlarmTime, &m_RtcBaseTime) == MMP_FALSE) {
            goto L_BadAlarmTime;
		}
		
        err = MMPF_RTC_GetTime(&curTime);
        if (err) {
            return err;
		}

        // Make sure alarm time is later than current time
        cur_sec     = AUTL_Calendar_DateToSeconds(&curTime, &m_RtcBaseTime);
        alarm_sec   = AUTL_Calendar_DateToSeconds(pAlarmTime, &m_RtcBaseTime);

        if (alarm_sec <= cur_sec) {
            goto L_BadAlarmTime;
		}

        cbRtcCallBack = p_callback;
        alarm_sec_ofst = alarm_sec - cur_sec;

        // Disable Alarm first
        pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_EN);
        pRTC->DIG2RTC_CTL |= RTC_ALARM_INT_CLR;
        MMPF_RTC_ISOCtl();
        pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_INT_CLR);

        // Read RTC Counter
        do {
            pRTC->DIG2RTC_CTL |= RTC_CNT_RD;
            MMPF_RTC_ISOCtl();
            cur_sec = pRTC->RTC_RD_DATA;
            pRTC->DIG2RTC_CTL &= ~(RTC_CNT_RD);
        } while(cur_sec & RTC_CNT_BUSY);

        cur_sec = pRTC->RTC_RD_DATA & RTC_RD_DATA_MASK; // not necessary actually

        // Set Alarm Counter & Enable Alarm
        pRTC->DIG2RTC_CTL |= RTC_ALARM_WR | RTC_ALARM_EN;
        pRTC->RTC_WR_DATA = cur_sec + alarm_sec_ofst;
        MMPF_RTC_ISOCtl();
        pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_WR);

        if (p_callback) {
            // Enable interrupt only if callback registered
            pGBL->GBL_WAKEUP_INT_CPU_SR  = RTC2DIG_INT;
            pGBL->GBL_WAKEUP_INT_CPU_EN |= RTC2DIG_INT;
        }
    }
    else {
   
        pGBL->GBL_WAKEUP_INT_CPU_EN &= ~(RTC2DIG_INT);
        pGBL->GBL_WAKEUP_INT_CPU_SR  = RTC2DIG_INT;

        cbRtcCallBack = NULL;

        // Disable Alarm
        pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_EN);
        pRTC->DIG2RTC_CTL |= RTC_ALARM_INT_CLR;
        MMPF_RTC_ISOCtl();
        pRTC->DIG2RTC_CTL &= ~(RTC_ALARM_INT_CLR);
    }

    return MMP_ERR_NONE;

L_BadAlarmTime:
    RTNA_DBG_Str(0, "Set RTC bad alarm time\r\n");
    return MMP_RTC_ERR_FMT;
}

#endif
