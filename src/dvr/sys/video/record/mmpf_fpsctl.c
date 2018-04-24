//==============================================================================
//
//  File        : mmpf_fpsctl.c
//  Description : Frame rate control function for recording system
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              HEADER
//
//==============================================================================

#include "mmpf_fpsctl.h"

#if (VIDEO_R_EN)&&(FPS_CTL_EN)

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

static MMPF_FPS_CTL     m_FpsCtl[FPS_CTL_MAX_STREAM];

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

//==============================================================================
//
//                              FUNCTION
//
//==============================================================================
//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_UpdateFlag
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Update frame rate control internal flags according to current fps.
    @param[in] id      stream ID
    @pre    Sensor/encode/decode frame rate must be set already.
    @return None.
*/
static void MMPF_FpsCtl_UpdateFlag(MMP_UBYTE id)
{
    MMP_BOOL    bCtlFlag    = MMP_FALSE;
    MMP_BOOL    bUpdFlag    = MMP_FALSE;
    MMP_BOOL    bMixedFlag  = MMP_FALSE;
    MMP_BOOL    bFixedFlag  = m_FpsCtl[id].bFpsFixedFlag;

    OS_CRITICAL_INIT();

    /* Update bFpsCtlFlag by compare SnrFps/EncFps */
    if ((m_FpsCtl[id].EncFps.ulIncr * m_FpsCtl[id].SnrFps.ulResol) !=
        (m_FpsCtl[id].EncFps.ulResol * m_FpsCtl[id].SnrFps.ulIncr))
    {
        bCtlFlag    = MMP_TRUE;
        bFixedFlag  = MMP_FALSE;
    }

    /* Update bFpsFixedFlag & bFpsMixedFlag by compare EncFps/DecFps */
    if ((m_FpsCtl[id].EncFps.ulIncr * m_FpsCtl[id].DecFps.ulResol) !=
        (m_FpsCtl[id].EncFps.ulResol * m_FpsCtl[id].DecFps.ulIncr))
    {
        bMixedFlag  = MMP_TRUE;
        bFixedFlag  = MMP_FALSE;
    }

    /* Update bFpsUpdFlag by compare EncFps/UpdEncFps */
    if ((m_FpsCtl[id].EncFps.ulIncr * m_FpsCtl[id].UpdEncFps.ulResol) !=
        (m_FpsCtl[id].EncFps.ulResol * m_FpsCtl[id].UpdEncFps.ulIncr))
    {
        bCtlFlag    = MMP_TRUE;
        bUpdFlag    = MMP_TRUE;
    }

    OS_ENTER_CRITICAL();
    m_FpsCtl[id].bFpsCtlFlag    = bCtlFlag;
    m_FpsCtl[id].bFpsUpdFlag    = bUpdFlag;
    m_FpsCtl[id].bFpsFixedFlag  = bFixedFlag;
    m_FpsCtl[id].bFpsMixedFlag  = bMixedFlag;
    OS_EXIT_CRITICAL();
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_SetSnrFrameRate
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Set sensor input fps info for frame rate control.
    @param[in] id      stream ID
    @param[in] inc     time increment for sensor frame rate
    @param[in] resol   time resolution for sensor frame rate
    @return None.
*/
void MMPF_FpsCtl_SetSnrFrameRate(MMP_UBYTE id, MMP_ULONG inc, MMP_ULONG resol)
{
    m_FpsCtl[id].SnrFps.ulResol     = resol;
    m_FpsCtl[id].SnrFps.ulIncr      = inc;
    m_FpsCtl[id].SnrFps.ulIncrx1000 = inc * 1000;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_SetEncFrameRate
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Set encode output fps info for frame rate control.
    @param[in] id      stream ID
    @param[in] inc     time increment for sensor frame rate
    @param[in] resol   time resolution for sensor frame rate
    @return None.
*/
void MMPF_FpsCtl_SetEncFrameRate(MMP_UBYTE id, MMP_ULONG inc, MMP_ULONG resol)
{
    m_FpsCtl[id].EncFps.ulResol     = resol;
    m_FpsCtl[id].EncFps.ulIncr      = inc;
    m_FpsCtl[id].EncFps.ulIncrx1000 = inc * 1000;

    /* Initial, set update encode fps as encode fps */
    m_FpsCtl[id].UpdEncFps = m_FpsCtl[id].EncFps;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_SetDecFrameRate
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Set decode fps info (specified by container) for frame rate control.
    @param[in] id      stream ID
    @param[in] inc     time increment for sensor frame rate
    @param[in] resol   time resolution for sensor frame rate
    @return None.
*/
void MMPF_FpsCtl_SetDecFrameRate(MMP_UBYTE id, MMP_ULONG inc, MMP_ULONG resol)
{
    m_FpsCtl[id].DecFps.ulResol     = resol;
    m_FpsCtl[id].DecFps.ulIncr      = inc;
    m_FpsCtl[id].DecFps.ulIncrx1000 = inc * 1000;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_UpdateEncFrameRate
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Update encode output fps at run-time.
    @param[in] id      stream ID
    @param[in] inc     time increment for sensor frame rate
    @param[in] resol   time resolution for sensor frame rate
    @return None.
*/
void MMPF_FpsCtl_UpdateEncFrameRate(MMP_UBYTE id, MMP_ULONG inc, MMP_ULONG resol)
{
    m_FpsCtl[id].UpdEncFps.ulResol     = resol;
    m_FpsCtl[id].UpdEncFps.ulIncr      = inc;
    m_FpsCtl[id].UpdEncFps.ulIncrx1000 = inc * 1000;

    MMPF_FpsCtl_UpdateFlag(id);
}
//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_Init
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Initialize frame rate control.
    @param[in] id      stream ID
    @pre    Sensor/encode/decode frame rate must be set already.
    @return None.
*/
void MMPF_FpsCtl_Init(MMP_UBYTE id)
{
    m_FpsCtl[id].ulSnrInAcc     = 0;
    m_FpsCtl[id].ulEncOutAcc    = 0;

    m_FpsCtl[id].bFpsCtlFlag    = MMP_FALSE;
    m_FpsCtl[id].bFpsUpdFlag    = MMP_FALSE;
    m_FpsCtl[id].bFpsMixedFlag  = MMP_FALSE;
    m_FpsCtl[id].bFpsFixedFlag  = MMP_TRUE;

    MMPF_FpsCtl_UpdateFlag(id);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_Operation
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Do frame rate control to check if drop or duplicate frame is needed.
    @param[in] id      stream ID
    @pre    Sensor/encode/decode frame rate must be set already.
    @pre    frame rate control is already initialized by @ref MMPF_FpsCtl_Init.
    @return None.
*/
void MMPF_FpsCtl_Operation(MMP_UBYTE id, MMP_BOOL *bDropFrm, MMP_ULONG *ulDupCnt)
{
    MMP_ULONG   ulOutCnt = 0;
    MMP_ULONG64 ullValue64[2];
    MMP_ULONG   ulInAcc, ulOutAcc;

    *bDropFrm = MMP_FALSE;
    *ulDupCnt = 0;

    /* is encode fps updated? */
    if (m_FpsCtl[id].bFpsUpdFlag) {
        m_FpsCtl[id].ulSnrInAcc  = 0;
        m_FpsCtl[id].ulEncOutAcc = 0;

        m_FpsCtl[id].EncFps = m_FpsCtl[id].UpdEncFps;
        MMPF_FpsCtl_UpdateFlag(id);
    }

    ulInAcc   = m_FpsCtl[id].ulSnrInAcc;
    ulOutAcc  = m_FpsCtl[id].ulEncOutAcc;

    ullValue64[0] = (MMP_ULONG64)m_FpsCtl[id].EncFps.ulResol * ulInAcc;
    ullValue64[1] = (MMP_ULONG64)m_FpsCtl[id].SnrFps.ulResol * ulOutAcc;

    if (ullValue64[0] < ullValue64[1]) {
        /* drop frame to fit encode fps */
        ulInAcc += m_FpsCtl[id].SnrFps.ulIncr;
        if (ulInAcc >= m_FpsCtl[id].SnrFps.ulResol) {
            ulInAcc  -= m_FpsCtl[id].SnrFps.ulResol;
            ulOutAcc -= m_FpsCtl[id].EncFps.ulResol;
        }
        m_FpsCtl[id].ulSnrInAcc  = ulInAcc;
        m_FpsCtl[id].ulEncOutAcc = ulOutAcc;

        *bDropFrm = MMP_TRUE;
        return;
    }

    ulInAcc  += m_FpsCtl[id].SnrFps.ulIncr;
    ulOutAcc += m_FpsCtl[id].EncFps.ulIncr;
    ulOutCnt = 1;

    /* Is enc fps is higher than snr fps? duplicate frame */
    ullValue64[0] = (MMP_ULONG64)m_FpsCtl[id].EncFps.ulResol * ulInAcc;
    ullValue64[1] = (MMP_ULONG64)m_FpsCtl[id].SnrFps.ulResol * ulOutAcc;
    while(ullValue64[0] > ullValue64[1]) {
        /* duplicate frame to fit encode fps */
        ulOutAcc += m_FpsCtl[id].EncFps.ulIncr;
        ullValue64[1] = (MMP_ULONG64)m_FpsCtl[id].SnrFps.ulResol * ulOutAcc;

        ulOutCnt++;
    }

    /* Update acc info */
    if (ulInAcc >= m_FpsCtl[id].SnrFps.ulResol) {
        ulInAcc  -= m_FpsCtl[id].SnrFps.ulResol;
        ulOutAcc -= m_FpsCtl[id].EncFps.ulResol;
    }
    m_FpsCtl[id].ulSnrInAcc  = ulInAcc;
    m_FpsCtl[id].ulEncOutAcc = ulOutAcc;

    *ulDupCnt = ulOutCnt - 1;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_IsEnabled
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Get frame rate control flag.
    @param[in] id      stream ID
    @pre    frame rate control is already initialized by @ref MMPF_FpsCtl_Init.
    @return None.
*/
MMP_BOOL MMPF_FpsCtl_IsEnabled(MMP_UBYTE id)
{
    return m_FpsCtl[id].bFpsCtlFlag;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_IsMixedFps
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Get mixed frame rate flag.
    @param[in] id      stream ID
    @pre    frame rate control is already initialized by @ref MMPF_FpsCtl_Init.
    @return None.
*/
MMP_BOOL MMPF_FpsCtl_IsMixedFps(MMP_UBYTE id)
{
    return m_FpsCtl[id].bFpsMixedFlag;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_FpsCtl_IsFixedFps
//  Description : 
//------------------------------------------------------------------------------
/**
    @brief  Get fixed frame rate flag.
    @param[in] id      stream ID
    @pre    frame rate control is already initialized by @ref MMPF_FpsCtl_Init.
    @return None.
*/
MMP_BOOL MMPF_FpsCtl_IsFixedFps(MMP_UBYTE id)
{
    return m_FpsCtl[id].bFpsFixedFlag;
}
#endif //(VIDEO_R_EN)&&(FPS_CTL_EN)
