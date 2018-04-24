/**
 @file mmps_jstream.c
 @brief JPEG Stream Control Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmps_sensor.h"
#include "mmps_jstream.h"
#include "mmpd_system.h"
#include "mmpd_ptz.h"
#include "mmpd_bayerscaler.h"
#include "mmpf_system.h"
#include "mmpf_mci.h"
#include "mmpf_dsc.h"
#include "mmpf_jstream.h"

/** @addtogroup MMPS_JSTREAM
@{
*/

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 *  Local Variables
 */
static MMPS_JSTREAM_CLASS   m_JStream[MAX_JPG_STREAM_NUM];

// Shared line buffer for all stream
static MMPS_JSTREAM_MEMBLK  m_ShareLineBuf;

// Q table
static const MMP_UBYTE m_Qtable[128] = 
{
    0x08,0x06,0x06,0x06,0x06,0x06,0x08,0x08,
    0x08,0x08,0x0c,0x08,0x06,0x08,0x0c,0x0e,
    0x0a,0x08,0x08,0x0a,0x0e,0x10,0x0c,0x0c,
    0x0e,0x0c,0x0c,0x10,0x10,0x10,0x12,0x12,
    0x12,0x12,0x10,0x14,0x14,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,

    0x08,0x0a,0x0a,0x10,0x0e,0x10,0x14,0x14,
    0x14,0x14,0x20,0x14,0x14,0x14,0x20,0x20,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
};

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPS_JStream_ReserveHeap(void);

static MMP_ERR MMPS_JStream_AssignBuf(  MMPS_JSTREAM_CLASS      *obj);

#if (DBG_JSTREAM_MEM_MAP)
static void    MMPS_JStream_MemMapDbg(  MMPS_JSTREAM_CLASS      *obj);
#endif

static MMP_ERR MMPS_JStream_ConfigPipe( MMPS_JSTREAM_CLASS      *obj);

static MMP_ERR MMPS_JStream_OpenEncoder(MMPS_JSTREAM_CLASS *obj);
static MMP_ERR MMPS_JStream_CloseEncoder(MMPS_JSTREAM_CLASS *obj);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____JStream_Module_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize JPEG stream module

 @retval It reports the status of the operation.
*/
int MMPS_JStream_ModInit(void)
{
    static MMP_BOOL _init = MMP_FALSE;
    MMP_ULONG s;
    MMPS_JSTREAM_CLASS *obj = NULL;

    if (_init == MMP_FALSE) {
        MEMSET(&m_ShareLineBuf, 0, sizeof(m_ShareLineBuf));

        for(s = 0; s < MAX_JPG_STREAM_NUM; s++) {
            obj = &m_JStream[s];
            MEMSET(obj, 0, sizeof(MMPS_JSTREAM_CLASS));
            if (s < gulMaxJStreamNum)
                obj->cap = &gstJStreamCap[s];
        }

        MMPS_JStream_ReserveHeap(); // Reserve heap buffer for each stream

        _init = MMP_TRUE;
    }
    return 0;
}

#if 0
void ____JStream_Obj_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of JPEG stream object

 @param[in] propt   Requested properties of a JPEG stream

 @retval The object instance of JPEG streamer
*/
MMPS_JSTREAM_CLASS *MMPS_JStream_Open(MMPS_JSTREAM_PROPT *propt)
{
    MMP_ERR   err;
    MMP_ULONG s, pipe;
    MMP_ULONG req_frm_size, req_buf_size;
    MMP_ULONG frm_size = 0, buf_size = 0;
    MMP_ULONG cand_frm_size = 0, cand_buf_size = 0;
    MMPS_JSTREAM_CLASS *obj = NULL;

    if (!propt)
    	return NULL;

    req_frm_size = ALIGN16(propt->w) * ALIGN16(propt->h);
    req_buf_size = propt->bufsize;

    /* Search a stream whose capability is best-fit with the request feature */
    for(s = 0; s < MAX_JPG_STREAM_NUM; s++) {
        if (m_JStream[s].state == IPC_STREAMER_IDLE) {
            frm_size = ALIGN16(m_JStream[s].cap->img_w) *
                       ALIGN16(m_JStream[s].cap->img_h);
            buf_size = m_JStream[s].cap->bufsize;
            if ((frm_size == req_frm_size) && (buf_size == req_buf_size)) {
                obj = &m_JStream[s];
                break;
            }
            else if ((frm_size >= req_frm_size) && (buf_size >= req_buf_size)) {
                if (!obj) {
                    obj = &m_JStream[s];
                    cand_frm_size = frm_size;
                    cand_buf_size = buf_size;
                }
                else if ((frm_size < cand_frm_size) ||
                        (buf_size < cand_buf_size))
                {
                    obj = &m_JStream[s];
                    break;
                }
            }
        }
    }

    if (!obj)
        return NULL;

    /* New an jstreamer instance */
    obj->id = MMPF_JStream_New();
    if (obj->id >= MAX_JPG_STREAM_NUM) {
        printc("JStream open err\r\n");
        return NULL;
    }

    /* Assign pipe resource */
    pipe = MMPD_Fctl_AllocatePipe(ALIGN16(propt->w), PIPE_LINK_JPG);
    if (pipe > MMP_IBC_PIPE_MAX) {
        RTNA_DBG_Str(0, "#JStream alloc pipe err\r\n");
		goto _jpipe_err;
    }

    FCTL_PIPE_TO_LINK(pipe, obj->pipe);

    /* Open encoder */
    obj->propt = *propt;
    err = MMPS_JStream_OpenEncoder(obj);
    if (err != MMP_ERR_NONE) {
        RTNA_DBG_Str(0, "#JStream open enc err\r\n");
        goto _jenc_err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return obj;

_jenc_err:
    MMPD_Fctl_ReleasePipe(pipe);

_jpipe_err:
    MMPF_JStream_Delete(obj->id);

    return NULL;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of JPEG stream object

 @param[in] obj     JPEG stream object
 @param[in] v4l2_id Corresponded V4L2 stream id
 @param[in] opt     Option of streaming

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_JStream_Start( MMPS_JSTREAM_CLASS  *obj,
                            MMP_ULONG           v4l2_id,
                            IPC_STREAMER_OPTION opt)
{
    MMP_ERR err;

    if (!obj)
        return MMP_JSTREAM_ERR_PARAMETER;
    if (obj->state != IPC_STREAMER_OPEN)
        return MMP_JSTREAM_ERR_STATE;

    err = MMPF_JStream_Start(obj->id, v4l2_id, opt);
    if (err) {
        RTNA_DBG_Str(0, "#JStream start err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the JPEG stream

 @param[in] obj     JPEG stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_JStream_Stop(MMPS_JSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
        return MMP_JSTREAM_ERR_PARAMETER;
    if (obj->state != IPC_STREAMER_START)
        return MMP_JSTREAM_ERR_STATE;

    err = MMPF_JStream_Stop(obj->id);
    if (err) {
        RTNA_DBG_Str(0, "#JStream stop err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close a instance of JPEG stream object

 @param[in] obj     JPEG stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_JStream_Close(MMPS_JSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
		return MMP_JSTREAM_ERR_PARAMETER;
	if (obj->state != IPC_STREAMER_OPEN)
		return MMP_JSTREAM_ERR_STATE;

    err = MMPS_JStream_CloseEncoder(obj);
    if (err != MMP_ERR_NONE) {
		RTNA_DBG_Str(0, "#JStream close enc err\r\n");
    }

    MMPD_Fctl_ReleasePipe(obj->pipe.ibcpipeID);
    MMPF_JStream_Delete(obj->id);

    obj->state = IPC_STREAMER_IDLE;

    return MMP_ERR_NONE;
}

#if 0
void ____JStream_Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_ReserveBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for JPEG stream.
*/
static MMP_ERR MMPS_JStream_ReserveHeap(void)
{
    MMP_ULONG s;
    MMP_ULONG w;
    MMP_ULONG max_w = 0;
    MMPS_JSTREAM_CLASS *obj = NULL;

    for(s = 0; s < MAX_JPG_STREAM_NUM; s++) {
        obj = &m_JStream[s];
        if (obj->cap) {
            w = ALIGN8(obj->cap->img_w);
            if (w > max_w) max_w = w;
        }
    }

    // Line buffers is shared by all stream
    m_ShareLineBuf.size = ALIGN8(max_w)* 2 * 16;
    m_ShareLineBuf.base = MMPF_SYS_HeapMalloc(SYS_HEAP_SRAM,
                                              m_ShareLineBuf.size, 32);
    m_ShareLineBuf.end  = m_ShareLineBuf.base + m_ShareLineBuf.size;

    // Assign heap for each stream according to its capability
    for (s = 0; s < MAX_JPG_STREAM_NUM; s++) {
        obj = &m_JStream[s];
        if (obj->cap) {
            // compress buffer
            obj->heap.size = ALIGN32(obj->cap->bufsize);
            obj->heap.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                 obj->heap.size, 32);
            if (obj->heap.base == SYS_HEAP_MEM_INVALID)
                return MMP_ASTREAM_ERR_MEM_EXHAUSTED;
            obj->heap.end  = obj->heap.base + obj->heap.size;
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_AssignBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign buffers for a JPEG stream object

 @param[in] obj     JPEG stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_JStream_AssignBuf(MMPS_JSTREAM_CLASS *obj)
{
    MMPS_JSTREAM_PROPT  *propt = &obj->propt;
    MMP_DSC_CAPTURE_BUF dscbuf;

    /*
	 * Assign line buffers from the shared buf
	 */
	obj->linebuf.base = m_ShareLineBuf.base;
    obj->linebuf.size = ALIGN8(propt->w) * 2 * 16;
    obj->linebuf.end  = obj->linebuf.base + obj->linebuf.size;
    if (obj->linebuf.end > m_ShareLineBuf.end)
        return MMP_JSTREAM_ERR_BUF;

    /*
     * Allocate compress buffer
     */
	obj->compbuf.base = obj->heap.base;
    obj->compbuf.size = propt->bufsize;
    obj->compbuf.end  = obj->compbuf.base + obj->compbuf.size;
    if (obj->compbuf.end > obj->heap.end)
        return MMP_JSTREAM_ERR_BUF;

    // TODO: assign the buffer to encoder

    #if (DBG_JSTREAM_MEM_MAP)
    MMPS_JStream_MemMapDbg(obj);
    #endif

    dscbuf.ulCompressStart  = obj->compbuf.base;
    dscbuf.ulCompressEnd    = obj->compbuf.end;
    dscbuf.ulLineStart      = obj->linebuf.base;
    dscbuf.ul2ndLineStart   = 0;
    MMPF_DSC_SetCaptureBuffers(&dscbuf);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_MemMapDbg
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign memory for a JPEG stream object

 @param[in] obj     JPEG stream object

 @retval None.
*/
#if (DBG_JSTREAM_MEM_MAP)
static void MMPS_JStream_MemMapDbg(MMPS_JSTREAM_CLASS *obj)
{
    printc("=============================================================\r\n");
    printc("\tjstream %d mem map, x%X-x%X\r\n",         obj->id,
                                                        obj->heap.base,
                                                        obj->heap.end);
    printc("-------------------------------------------------------------\r\n");
    printc("\tLinebuf : x%X-x%X (%d)\r\n",  obj->linebuf.base,
                                            obj->linebuf.end,
                                            obj->linebuf.size);
    printc("\tCompbuf : x%X-x%X (%d)\r\n",  obj->compbuf.base,
                                            obj->compbuf.end,
                                            obj->compbuf.size);
    printc("=============================================================\r\n");
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_ConfigPipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Configure JPEG stream pipe

 @param[in] *obj        JPEG stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_JStream_ConfigPipe(MMPS_JSTREAM_CLASS *obj)
{
    MMP_ULONG           ulScalInW, ulScalInH;
    MMP_SCAL_FIT_RANGE  fitrange;
    MMP_SCAL_GRAB_CTRL  grabEnc;
    MMPD_FCTL_ATTR      fctlAttr;
    MMP_SCAL_FIT_RANGE  fitRangeBayer;
    MMP_SCAL_GRAB_CTRL  grabBayer;
    MMPS_JSTREAM_PROPT  *propt = &obj->propt;

    /* Parameter Check */
    if (!obj) {
        return MMP_JSTREAM_ERR_PARAMETER;
    }

    /* Get the sensor parameters */
    MMPS_Sensor_GetCurPrevScalInputRes(propt->snrID, &ulScalInW, &ulScalInH);

    MMPD_BayerScaler_GetZoomInfo(MMP_BAYER_SCAL_DOWN, &fitRangeBayer, &grabBayer);

    // Config video stream pipe
    fitrange.fitmode        = MMP_SCAL_FITMODE_OUT;
    fitrange.scalerType 	= MMP_SCAL_TYPE_SCALER;
    fitrange.ulInWidth 	    = ulScalInW;
    fitrange.ulInHeight	    = ulScalInH;
    fitrange.ulOutWidth     = propt->w;
    fitrange.ulOutHeight    = propt->h;
    fitrange.ulInGrabX 		= 1;
    fitrange.ulInGrabY 		= 1;
    fitrange.ulInGrabW 		= fitrange.ulInWidth;
    fitrange.ulInGrabH 		= fitrange.ulInHeight;

    MMPD_PTZ_InitPtzInfo(obj->pipe.scalerpath,
                        fitrange.fitmode,
                        fitrange.ulInWidth, fitrange.ulInHeight,
                        fitrange.ulOutWidth, fitrange.ulOutHeight);

    MMPD_PTZ_CalculatePtzInfo(obj->pipe.scalerpath, 0, 0, 0);
    MMPD_PTZ_GetCurPtzInfo(obj->pipe.scalerpath, &fitrange, &grabEnc);

    fctlAttr.colormode          = MMP_DISPLAY_COLOR_YUV422;
    fctlAttr.fctllink           = obj->pipe;
    fctlAttr.fitrange           = fitrange;
    fctlAttr.grabctl            = grabEnc;
    fctlAttr.scalsrc            = MMP_SCAL_SOURCE_ISP;
    fctlAttr.bSetScalerSrc      = MMP_TRUE;
    fctlAttr.ubPipeLinkedSnr    = propt->snrID;
    fctlAttr.usBufCnt           = 0;
    fctlAttr.bUseRotateDMA      = MMP_FALSE;

    MMPD_Fctl_ResetIBCLinkType(obj->pipe.ibcpipeID);
    MMPD_Fctl_SetPipeAttrForJpeg(&fctlAttr, MMP_TRUE, MMP_FALSE);

    // Tune MCI priority of encode pipe
    MMPF_MCI_SetIBCMaxPriority(obj->pipe.ibcpipeID);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_EnablePipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on or off pipe for JPEG stream.

 @param[in] *obj    JPEG stream object
 @param[in] bEnable Enable and disable pipe path for JPEG stream.
 
 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPS_JStream_EnablePipe( MMPS_JSTREAM_CLASS  *obj,
                                        MMP_BOOL            bEnable)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_UBYTE snrID = obj->propt.snrID;
    MMP_IBC_PIPEID ibcID = obj->pipe.ibcpipeID;

    if (obj->pipe_en != bEnable)
        err = MMPD_Fctl_EnablePipe(snrID, ibcID, bEnable);

    obj->pipe_en = bEnable;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_OpenEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on the JPEG stream encoder

 @param[in] *obj        JPEG stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_JStream_OpenEncoder(MMPS_JSTREAM_CLASS *obj)
{
    MMP_ULONG           width, height;
    MMPS_JSTREAM_PROPT  *propt = &obj->propt;

    width  = ALIGN8(propt->w);
    height = ALIGN8(propt->h);

    /* Enable clock */
    MMPD_System_EnableClock(MMPD_SYS_CLK_JPG, MMP_TRUE);

    if (MMPS_JStream_AssignBuf(obj)) {
        printc("Alloc mem for jstream failed\r\n");
        #if (DBG_JSTREAM_MEM_MAP)
        MMPS_JStream_MemMapDbg(obj);
        #endif
        MMPD_System_EnableClock(MMPD_SYS_CLK_JPG, MMP_FALSE);
        return MMP_JSTREAM_ERR_BUF;
    }

    MMPS_JStream_ConfigPipe(obj);
    if (MMPS_JStream_EnablePipe(obj, MMP_TRUE) != MMP_ERR_NONE) {
		printc("Enable Jpeg pipe: Fail\r\n");
        return MMP_JSTREAM_ERR_PIPE;
    }

    /* Configure encoder */
    MMPF_DSC_SetJpegResol(width, height, MMP_DSC_JPEG_RC_ID_CAPTURE);
    MMPF_DSC_SetJpegQualityCtl( MMP_DSC_JPEG_RC_ID_CAPTURE,
                                MMP_FALSE,
                                MMP_TRUE,
			                    obj->propt.size,
			                    (obj->propt.size * 5) >> 2,
			                    1);
    MMPF_DSC_SetQTableInfo( MMP_DSC_JPEG_RC_ID_CAPTURE,
                            (MMP_UBYTE *)&m_Qtable[0],
                            (MMP_UBYTE *)&m_Qtable[DSC_QTABLE_ARRAY_SIZE],
                            (MMP_UBYTE *)&m_Qtable[DSC_QTABLE_ARRAY_SIZE],
                            MMP_DSC_JPEG_QT_AIT_HIGH);
    MMPF_DSC_SetQTableIntoOpr(MMP_DSC_JPEG_RC_ID_CAPTURE);
    MMPF_DSC_SetCapturePath(obj->pipe.scalerpath,
                            obj->pipe.icopipeID,
                            obj->pipe.ibcpipeID);

    MMPF_JStream_TriggerEncode(obj->id);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_JStream_CloseEncoder
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn off the JPEG stream encoder

 @param[in] *obj        JPEG stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_JStream_CloseEncoder(MMPS_JSTREAM_CLASS *obj)
{
    MMPS_JStream_EnablePipe(obj, MMP_FALSE);

    MMPD_System_ResetHModule(MMPD_SYS_MDL_JPG, MMP_FALSE);
    MMPD_System_EnableClock(MMPD_SYS_CLK_JPG, MMP_FALSE);

    return MMP_ERR_NONE;
}

//#pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall" 
//#pragma O0
ait_module_init(MMPS_JStream_ModInit);
//#pragma
//#pragma arm section rodata, rwdata, zidata


/// @}
