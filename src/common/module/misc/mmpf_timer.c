//==============================================================================
//
//  File        : mmpf_timer.c
//  Description : Firmware Timer Control Function
//  Author      : Jerry Lai
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_timer.h"
#include "mmpf_timer.h"
#include "mmpf_pll.h"

void						tcs_isr_a(void);

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

#if !defined(MINIBOOT_FW)
static MMP_BOOL				m_bTimerEnabled[MMPF_TIMER_MAX];
#endif
static MMP_BOOL				m_bIntMode[MMPF_TIMER_MAX] = { 0, };

#if !defined(MINIBOOT_FW)
static MMP_BOOL				m_bTimerIntOpened[MMPF_TIMER_MAX] = { MMP_FALSE, };
static MMP_BOOL				m_bTimerIntEnabled[MMPF_TIMER_MAX] = { MMP_FALSE, };
#endif

static TimerCallBackFunc	*TimerCallBacks[MMPF_TIMER_MAX] = { NULL, };
#if (ITCM_PUT)
void MMPF_Timers_ISR(void) ITCMFUNC;
#endif

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

////----------------------------------------------------------------------------
//  Function    : MMPF_TC0_Start
//  Description : Called by MMPF_BSP_InitializeTimer()
//  Note        : Timer0 is dedicated for OS use
//  Return      :
//------------------------------------------------------------------------------
void MMPF_TC0_Start(IntHandler pfHandler)
{
	AITPS_AIC	pAIC = AITC_BASE_AIC;
	AITPS_TCB	pTCB = AITC_BASE_TCB;
	AITPS_TC	pTC0 = AITC_BASE_TC0;
	MMP_ULONG	ulG0Clock;
    
	//USING_PROTECT_MODE
	pTCB->TC_DBG_MODE |= TC_DBG_EN;

#if defined(MBOOT_FW) || defined(UPDATER_FW) || defined(ALL_FW) || defined(MBOOT_EX_FW)
	RTNA_AIC_Open(pAIC, AIC_SRC_TC0, pfHandler, 
            AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 7);
#endif

	// enable TC0
	pTC0->TC_CTL = TC_CLK_DIS;
	pTC0->TC_INT_DIS = TC_COMP_VAL_HIT;

    // Clear status
    pTC0->TC_SR         = pTC0->TC_SR;

    pTC0->TC_MODE       = TC_COMP_TRIG | TC_CLK_MCK_D8;

    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulG0Clock);

    ulG0Clock = ulG0Clock >> 1;

	pTC0->TC_COMP_VAL = (ulG0Clock * 1000 / 8) / OS_TICKS_PER_SEC;

	pTC0->TC_CTL = TC_CLK_EN;

	RTNA_AIC_IRQ_En(pAIC, AIC_SRC_TC0);


    // Clear counter
    pTC0->TC_CTL |= TC_SW_TRIG;
    
    // Enable interrupt
    pTC0->TC_SR = pTC0->TC_SR;
    pTC0->TC_INT_EN = TC_COMP_VAL_HIT;
}

////----------------------------------------------------------------------------
//  Function    : MMPF_TC0_ClearSr
//  Description : Clear Timer0 interrupt status
//  Note        : Timer0 is dedicated for OS use
//  Return      :
//------------------------------------------------------------------------------
void MMPF_TC0_ClearSr(void)
{
	AITPS_TC pTC0 = AITC_BASE_TC0;

	// both for normal mode and debug mode
	pTC0->TC_SR = pTC0->TC_SR;
}

////----------------------------------------------------------------------------
//  Function    : Return the AITPS_TC of given id
//  Description : Helper function to get correct pTC from ID
//  Note        : Because timer hardware is split into 2 group. The ID
//                to pTC is handled here
//  Return      :
//------------------------------------------------------------------------------
inline void *GetpTC(MMPF_TIMER_ID id)
{
	AITPS_TCB	pTCB = AITC_BASE_TCB;

	if (id < MMPF_TIMER_3) {
		return ((AITPS_TC)(&(pTCB->TC0_2[id])));
	}
	else {
		return ((AITPS_TC)(&(pTCB->TC3_5[id - MMPF_TIMER_3])));
	}
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_EnableAICTimerSRC
//  Description :
//  Note        :
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)
static MMP_ERR MMPF_Timer_EnableAICTimerSRC(MMPF_TIMER_ID id, MMP_BOOL bEnable)
{
    AITPS_AIC pAIC = AITC_BASE_AIC;

    if (bEnable == MMP_TRUE)
        RTNA_AIC_IRQ_En(pAIC, AIC_SRC_TC(id));
    else
    	RTNA_AIC_IRQ_Dis(pAIC, AIC_SRC_TC(id));

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_OpenInterrupt
//  Description :
//------------------------------------------------------------------------------
/**
@brief  To set AIC interrupt source
@param[in]  id   The timer you want to use as interrupt source
*/
static MMP_ERR MMPF_Timer_OpenInterrupt(MMPF_TIMER_ID id)
{
    AITPS_AIC pAIC = AITC_BASE_AIC;
	
    if (m_bTimerIntOpened[id] == MMP_TRUE) {
        return MMP_ERR_NONE;
	}

    m_bTimerIntOpened[id] = MMP_TRUE;

    RTNA_AIC_Open(pAIC, AIC_SRC_TC(id), tcs_isr_a, AIC_INT_TO_IRQ |
                        AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 2);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_EnableInterrupt
//  Description :
//------------------------------------------------------------------------------
/**
@brief  Start to recieve timer IRQ
@param[in]  id   The timer you want to use as interrupt source
@param[in]  bEnable  1 start the timer IRQ. 0 stop the timer IRQ
*/
static MMP_ERR MMPF_Timer_EnableInterrupt(MMPF_TIMER_ID id, MMP_BOOL bEnable)
{
	AITPS_TC	pTC = (AITPS_TC) GetpTC(id);

	if (bEnable == MMP_TRUE) {

        if (m_bTimerEnabled[id] == MMP_FALSE) {
		    //Enable AIC's Timer Source
		    if (m_bTimerIntEnabled[id] == MMP_FALSE) {
                MMPF_Timer_EnableAICTimerSRC(id, MMP_TRUE);
                m_bTimerIntEnabled[id] = MMP_TRUE;
		    }
		    m_bTimerEnabled[id] = MMP_TRUE;
		}

		// Clear counter to 0
		pTC->TC_COUNT_VAL = 0;
		//Enable Timer IRQ
		pTC->TC_SR = pTC->TC_SR;			//clear SR
		pTC->TC_INT_EN = TC_COMP_VAL_HIT;	//enable compare interrupt
	}
	else {

        if (m_bTimerEnabled[id] == MMP_TRUE) {
            //Disable AIC's Timer Source
	        if (m_bTimerIntEnabled[id] == MMP_TRUE) {
		        MMPF_Timer_EnableAICTimerSRC(id, MMP_FALSE);
                m_bTimerIntEnabled[id] = MMP_FALSE;
		    }

		    m_bTimerEnabled[id] = MMP_FALSE;
	    }

		//Disable Timer IRQ
		pTC->TC_INT_DIS = TC_COMP_VAL_HIT;	//disable compare interrupt
		pTC->TC_SR = pTC->TC_SR;			//clear SR
	}

	return MMP_ERR_NONE;
}
#endif // !defined(MINIBOOT_FW)

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_Open
//  Description :
//------------------------------------------------------------------------------
/**
@brief  Set Timer triggered time in sec, msec or usec. And set timer callback function
@param[in]  id              The timer you want to use as interrupt source
@param[in]  ulTimeUnits     The units of time precision
- The maximun units of sec: 357
- The maximun units of milli-sec: 357913
- The maximun units of micro-sec: 357913941
@param[in]  Func            The call back function. The function is bind to the id.
@param[in]  TimePrecision   There are three kinds of time precision as: sec, msec, usec
*/
static MMP_ERR MMPF_Timer_Open(MMPF_TIMER_ID id, MMPF_TIMER_PRECISION TimePrecision,
                               MMP_ULONG ulTimeUnits, TimerCallBackFunc *Func)
{
	MMP_ULONG count = 0;
	MMP_ULONG mode;
	MMP_ULONG TimerClkFreq;
	AITPS_TC  pTC = (AITPS_TC)GetpTC(id);

    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &TimerClkFreq);

    /* #if(USE_DIV_CONST) */
    /* #if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168) */
    /* TimerClkFreq = 168000;		//	CLK_GRP_GBL    168MHz					 */
    /* #endif */
    /* #if (PLL_CONFIG==PLL_FOR_POWER)||(PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192) || (PLL_CONFIG==PLL_FOR_PERFORMANCE) || (PLL_CONFIG==PLL_FOR_BURNING) */
    /* TimerClkFreq = 264000;     //	CLK_GRP_GBL    264MHz					 */
    /* #endif */
    /* #endif */

    TimerClkFreq = TimerClkFreq >> 1;

    if (TimerClkFreq == 0) {
		RTNA_DBG_Str(0, "ERROR : FW PLL not set.");
		return MMP_SYSTEM_ERR_SETPLL;
	}

    switch(TimePrecision) {
    case MMPF_TIMER_SEC:
        //(1/Timer FREQ) * count = n * 1
        count = ulTimeUnits * TimerClkFreq * 1000;
        break;
    case MMPF_TIMER_MILLI_SEC:
        //(1/Timer FREQ) * count = n * (1/1000)
        count = ulTimeUnits * TimerClkFreq;
        break;
    case MMPF_TIMER_MICRO_SEC:
        //(1/Timer FREQ) * count = n * (1/1000000)
        count = ulTimeUnits * (TimerClkFreq / 1000);
        break;
    }

	mode = TC_COMP_TRIG | TC_CLK_MCK;
	
	// Should check if the delay is not too long for a timer

    // Start the Timer and set compare value
    pTC->TC_CTL = TC_CLK_DIS;  				//Stop Timer
    pTC->TC_INT_DIS = TC_COMP_VAL_HIT;    	//Disable Interrupt mode

	pTC->TC_SR = pTC->TC_SR; 				//Clear Status Register
	pTC->TC_CTL |= TC_SW_TRIG;  			//Reset Timer
    pTC->TC_MODE = mode;      
    pTC->TC_COMP_VAL = count;
    pTC->TC_CTL = TC_CLK_EN;   				//Start Timer again

	//Save the callback
	TimerCallBacks[id] = Func;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_Close
//  Description :
//------------------------------------------------------------------------------
/**
@brief  Stop timer triggered time in ms, and clear timer callback function

@param[in]  id   The timer you want to use as interrupt source
*/
static MMP_ERR MMPF_Timer_Close(MMPF_TIMER_ID id)
{
	AITPS_TC	pTC = (AITPS_TC) GetpTC(id);

	//Stop timer
	pTC->TC_CTL = TC_CLK_DIS;
	pTC->TC_INT_DIS = TC_COMP_VAL_HIT;

	// Clear callback
	TimerCallBacks[id] = NULL;

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_Start
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Timer_Start(MMPF_TIMER_ID id, MMPF_TIMER_ATTR *pTimerAttr)
{
	MMP_ERR status = MMP_ERR_NONE;

    m_bIntMode[id] = pTimerAttr->bIntMode;

    #if !defined(MINIBOOT_FW)
    status |= MMPF_Timer_OpenInterrupt(id);
    #endif

    status |= MMPF_Timer_Open(id, 
    						  pTimerAttr->TimePrecision,
                              pTimerAttr->ulTimeUnits, 
                              pTimerAttr->Func);
                              
    #if !defined(MINIBOOT_FW)
    status |= MMPF_Timer_EnableInterrupt(id, MMP_TRUE);
	#endif
	
    return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_Stop
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Timer_Stop(MMPF_TIMER_ID id)
{
	MMP_ERR status = MMP_ERR_NONE;

    status |= MMPF_Timer_Close(id);

    #if !defined(MINIBOOT_FW)
    status |= MMPF_Timer_EnableInterrupt(id, MMP_FALSE);
    #endif
    
    return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timer_GetTimeDetail
//  Description : 
//------------------------------------------------------------------------------
MMP_ULONG MMPF_Timer_GetTimeDetail(MMPF_TIMER_ID id, MMP_ULONG precision)
{
    AITPS_TC	pTC = (AITPS_TC) GetpTC(id);

    /* return pTC->TC_COUNT_VAL * precision / pTC->TC_COMP_VAL; */
    return 0x10 * precision / 0x1C;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Timers_ISR
//  Description : The ISR operation. Clear AIC and call the callback
//------------------------------------------------------------------------------
#if !defined(MINIBOOT_FW)
void MMPF_Timers_ISR(void)
{
	AITPS_TC  pTC;
    AITPS_AIC pAIC = AITC_BASE_AIC;
	MMP_UBYTE i;
	MMP_UBYTE TimerTriggered[MMPF_TIMER_MAX] = {0, };

    //Check All Timer
    for(i = 1; i < MMPF_TIMER_MAX; i++) {

        pTC = (AITPS_TC)GetpTC(i);

        //Timer i is triggered
        if (pTC->TC_SR & TC_COMP_VAL_HIT) {

            if (m_bIntMode[i] == MMPF_TIMER_ONE_SHOT) {
                // Disable compare interrupt
                pTC->TC_INT_DIS = TC_COMP_VAL_HIT; 
            }
			
            // Clear the SR
            pTC->TC_SR = pTC->TC_SR; 

            //Check if callback is set
            if (TimerCallBacks[i] != NULL) {
                if (m_bTimerEnabled[i]) {
                    // Execute later
                    TimerTriggered[i] = 1; 
                }
            }
    		
            // Clear TC Interrupt on AIC
            if (i < 3)
                pAIC->AIC_ICCR_LSB = 0x1 << (AIC_SRC_TC(i));
            else
                pAIC->AIC_ICCR_MSB = 0x1 << (AIC_SRC_TC(i) - 0x20);
        }
    }

	//Execute the Timer callback
	for(i = 1; i < MMPF_TIMER_MAX; ++i) {
		if (TimerTriggered[i] == 1)
			TimerCallBacks[i]();
	}
}
#endif

#if 0
void ____StopWatch_Function____(){ruturn;} //dummy
#endif

#if defined(ALL_FW)

/* Stop watch Usage:

Task init:
    0. MMPF_Stopwatch_Open(&handle);

Usage:
    1. (Optional) Write your own report function to customize display messages.
    2. MMPF_Stopwatch_Register()
    3. repeated event {
           MMPF_Stopwatch_Start()
           TheActionsToBeMeasured
           MMPF_Stopwatch_Stop()
       }

	Step 1 ~ 3 could be used somewhere else to reuse the stop watch resource.
	As long as those are not be used at the same time:
    4. MMPF_Stopwatch_Reset()
    5. Repeat step 1 ~ 3
*/

#define MAX_NAME_SIZE(h) 	(sizeof(h->name)-1)
#define MAX_MEASURE_NUM 	(10)

#define NO_OVERFLOW 		(1) //1: Ensure the detail and tick number are the same in the view of CPU.
#define DEFAULT_STOPWATCH_PRECISION (1000)

static MMPF_STOPWATCH m_measure[MAX_MEASURE_NUM];
static short m_measure_idx = 0;

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Reset
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Stopwatch_Reset(MMPF_STOPWATCH_HANDLE h)
{
    if (h == NULL) {
        return MMP_SYSTEM_ERR_PARAMETER;
    }

    h->sumMs        = 0;
    h->sumDetail    = 0;
    h->count        = 0;
    h->trigger      = 0;    
    h->reportFunc   = 0;
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Search1msTimer
//  Description : 
//------------------------------------------------------------------------------
/** @brief Find the only one 1ms timer, which should be OS tick timer
 *
 * The OS timer is in the library and might be changed.
 * @pre Assume 1ms timer does not change all the time and there is only 1 ms timer in the system.
 * @return MMPF_TIMER_MAX if the timer could not be found.
 */
static MMPF_TIMER_ID MMPF_Stopwatch_Search1msTimer(void)
{
    AITPS_TC  		pTC;
    MMP_ULONG 		TimerClkFreq;
    MMPF_TIMER_ID 	id;
    MMP_USHORT 		multiplier = 1;
    const MMP_ULONG precision = DEFAULT_STOPWATCH_PRECISION;
    
    static MMPF_TIMER_ID foundID = MMPF_TIMER_MAX;

    // Assume 1ms timer does not change all the time and there is only 1 ms timer in the system.
    if (foundID != MMPF_TIMER_MAX) {
        return foundID;
    }

    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &TimerClkFreq);
    TimerClkFreq = TimerClkFreq>>1;
    
    if (TimerClkFreq == 0) {
        RTNA_DBG_Str(0, "ERROR : FW PLL not set.");
        return MMPF_TIMER_MAX;
    }

    for (id = 0; id < MMPF_TIMER_MAX; ++id) {

        pTC = GetpTC(id);

        switch(pTC->TC_MODE & TC_CLK_MCK_MASK) {
        case TC_CLK_MCK_D2:
            multiplier = 2;
            break;
        case TC_CLK_MCK_D8:
            multiplier = 8;
            break;
        case TC_CLK_MCK_D32:
            multiplier = 32;
            break;
        case TC_CLK_MCK_D128:
            multiplier = 128;
            break;
        case TC_CLK_MCK_D1024:
            multiplier = 1024;
            break;
        case TC_CLK_MCK_D4:
            multiplier = 4;
            break;
        case TC_CLK_MCK_D16:
            multiplier = 16;
            break;
        default:
            break;
        }

        if ((pTC->TC_COMP_VAL * multiplier) == TimerClkFreq &&
            (pTC->TC_SR & TC_IS_COUNTING) &&
            (pTC->TC_MODE & TC_COMP_TRIG)) {
            break;
        }
    }

    // Check if clock setting might overflows
    if (((pTC->TC_COMP_VAL * precision) / precision) != pTC->TC_COMP_VAL) {
        RTNA_DBG_Str(0, "HW timer error 1\r\n");
        return MMPF_TIMER_MAX;
    }

    foundID = id;
    
    return foundID;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Open
//  Description : 
//------------------------------------------------------------------------------
/** @brief Open the stop watch and return the handle.
 *
 * @param[out] h The pointer of the handle. Note that it would set to NULL even with error.
 * @note This function should be called once
 * @return MMP_SYSTEM_ERR_TIMER of there is any error during open.
 */
MMP_ERR MMPF_Stopwatch_Open(MMPF_STOPWATCH_HANDLE* h)
{
    MMPF_TIMER_ID id;

    *h = NULL;

    id = MMPF_Stopwatch_Search1msTimer();
    
    if (MMPF_TIMER_MAX == id) {
        RTNA_DBG_Str(0, "Counter not find OS timer\r\n");
        return MMP_SYSTEM_ERR_TIMER;
    }

    if (m_measure_idx == MAX_MEASURE_NUM) {
        RTNA_DBG_Str(0, "Out of stop watch resource\r\n");
        return MMP_SYSTEM_ERR_TIMER;
    }

    *h = &m_measure[m_measure_idx++]; //warning! not thread safe. But it's in init phase only.
    
    MMPF_Stopwatch_Reset(*h);
    
    (*h)->precision = DEFAULT_STOPWATCH_PRECISION;
    (*h)->id        = id;
    (*h)->name[0]   = '\0';

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_RegisterHandler
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Stopwatch_RegisterHandler(MMPF_STOPWATCH_HANDLE h, MMP_ULONG triggerCount, MMPF_STOPWATCH_REPORT reportFunc)
{
    MMP_ULONG product = 0;

    if (h == NULL) {
        return MMP_SYSTEM_ERR_PARAMETER;
    }    
    if (triggerCount == 0) {
        return MMP_SYSTEM_ERR_PARAMETER;
    }
        
    product = triggerCount * h->precision;
    
    if ((product / triggerCount) != h->precision) {
        return MMP_SYSTEM_ERR_PARAMETER; // Overflow
	}

    MMPF_Stopwatch_Reset(h);
    
    h->trigger      = triggerCount;
    h->reportFunc   = reportFunc;
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Register
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Stopwatch_Register(MMPF_STOPWATCH_HANDLE h, MMP_ULONG triggerCount, MMPF_STOPWATCH_REPORT reportFunc, char *szName)
{
    int len;
    
    if (NULL == szName) {
        len = 0;
    } 
    else {
        len = strlen(szName);
        
        if (len > MAX_NAME_SIZE(h)) {
            len = MAX_NAME_SIZE(h);
        }
    }
    
    h->name[len] = '\0';
    
    MEMCPY(h->name, szName, len);

    return MMPF_Stopwatch_RegisterHandler(h, triggerCount, reportFunc);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_GetTime
//  Description : 
//------------------------------------------------------------------------------
static MMP_ERR MMPF_Stopwatch_GetTime(MMPF_STOPWATCH_HANDLE h, MMP_ULONG *ms, MMP_ULONG *detail)
{
    AITPS_TC  pTC = GetpTC(h->id);
    
#if (NO_OVERFLOW)
    MMP_ULONG org_ms, org_detail;

    org_detail = pTC->TC_COUNT_VAL * h->precision / pTC->TC_COMP_VAL;
    
    MMPF_OS_GetTime(ms);
    
    org_ms = *ms;

    while ((*ms - org_ms) < 2) {
    
        *detail = pTC->TC_COUNT_VAL * h->precision / pTC->TC_COMP_VAL;
    
        if (org_detail <= *detail) {
            //detail are recorded in the same tick
            return MMP_ERR_NONE;
        }
        org_detail = *detail;
        
        MMPF_OS_GetTime(ms);
    }

    return MMP_SYSTEM_ERR_TIMER;
#else
    MMPF_OS_GetTime(ms);
    
    *detail = pTC->TC_CVR * h->precision / pTC->TC_RC;
    
    return MMP_ERR_NONE;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Start
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Stopwatch_Start(MMPF_STOPWATCH_HANDLE h)
{
    if (h == NULL) {
        return MMP_SYSTEM_ERR_PARAMETER;
	}

    return MMPF_Stopwatch_GetTime(h, &(h->ms), &(h->detail));
}

//------------------------------------------------------------------------------
//  Function    : MMPF_Stopwatch_Stop
//  Description : 
//------------------------------------------------------------------------------
MMP_ERR MMPF_Stopwatch_Stop(MMPF_STOPWATCH_HANDLE h)
{
    MMP_ULONG   ms, detail;
    MMP_LONG    deltaMs;
    #if (NO_OVERFLOW)
    MMP_ERR     err;
    #endif

    if (h == NULL) {
        return MMP_SYSTEM_ERR_PARAMETER;
	}

    // Warning! Assume this 1ms timer is synchronized with OS tick. It might introduce some borrowing error.
    // it can not be solved since this is SW stop watch and ISR delay
    err = MMPF_Stopwatch_GetTime(h, &ms, &detail);
    
    if (MMP_ERR_NONE != err) {
        return err;
    }
    
    deltaMs = ms - h->ms;
    h->sumMs += deltaMs;

    // Borrow
    if (detail < h->detail) {
    	#if (NO_OVERFLOW == 0)
        if (deltaMs > 0) //the Software ms ticks so we could have something to borrow
    	#endif
            h->sumMs --;
        detail += h->precision;
    }
    
    h->sumDetail += detail - h->detail;

    // Carry
    if (h->sumDetail >= h->precision) {
        h->sumDetail -= h->precision;
        h->sumMs++;
    }

    h->count++;
    
    if (h->reportFunc && (h->count >= h->trigger)) {
    
        MMP_ULONG avgMs, avgDetail;

        avgMs = h->sumMs / h->trigger;
    
        // Warning, possible overflow here if trigger is too big
        avgDetail = ((h->sumMs % h->trigger) * h->precision + h->sumDetail) / (h->trigger);

        h->reportFunc(avgMs, avgDetail);

        h->count = 0;
        h->sumMs = 0;
        h->sumDetail = 0;
    }

    return MMP_ERR_NONE;
}

#endif //defined(ALL_FW)
