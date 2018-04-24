/**
 @file mmps_ystream.c
 @brief Gray Stream Control Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmps_sensor.h"
#include "mmps_ystream.h"
#include "mmpd_scaler.h"
#include "mmpf_system.h"
#include "mmpf_ystream.h"
#include "dualcpu_v4l2.h"
/** @addtogroup MMPS_YSTREAM
@{
*/

#if (V4L2_GRAY)

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 *  Local Variables
 */
static MMPS_YSTREAM_CLASS   m_YStream[MAX_GRAY_STREAM_NUM];

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR  MMPS_YStream_ReserveHeap(void);
static MMP_ERR  MMPS_YStream_AssignBuf(MMPS_YSTREAM_CLASS   *obj);
#if (DBG_YSTREAM_MEM_MAP)
static void     MMPS_YStream_MemMapDbg(MMPS_YSTREAM_CLASS   *obj);
#endif

static MMP_ERR  MMPS_YStream_ConfigPipe(MMPS_YSTREAM_CLASS  *obj);
static MMP_ERR  MMPS_YStream_EnablePipe(MMPS_YSTREAM_CLASS  *obj,
                                        MMP_BOOL            bEnable);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____YStream_Module_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize gray stream module

 @retval It reports the status of the operation.
*/
int MMPS_YStream_ModInit(void)
{
    static MMP_BOOL _init = MMP_FALSE;
    MMP_ULONG s;
    MMPS_YSTREAM_CLASS *obj = NULL;

    if (_init == MMP_FALSE) {
        for(s = 0; s < MAX_GRAY_STREAM_NUM; s++) {
            obj = &m_YStream[s];
            MEMSET(obj, 0, sizeof(MMPS_YSTREAM_CLASS));
            if (s < gulMaxYStreamNum)
                obj->cap = &gstYStreamCap[s];
        }

        MMPS_YStream_ReserveHeap();

        _init = MMP_TRUE;
    }
    return 0;
}



#if 0
void ____YStream_Obj_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of gray stream object

 @param[in] propt   Requested properties of a gray stream

 @retval The object instance of gray streamer
*/
MMPS_YSTREAM_CLASS *MMPS_YStream_Open(MMPS_YSTREAM_PROPT *propt)
{
    MMP_ERR   err;
    MMP_ULONG s, pipe;
    MMP_ULONG req_frm_size;
    MMP_ULONG frm_size = 0;
    MMP_ULONG cand_frm_size = 0;
    MMPS_YSTREAM_CLASS *obj = NULL;

    if (!propt)
    	return NULL;

    req_frm_size = propt->w * propt->h;

    /* Search a stream whose capability is best-fit with the request feature */
    for(s = 0; s < MAX_GRAY_STREAM_NUM; s++) {
        if (m_YStream[s].state == IPC_STREAMER_IDLE) {

            if( propt->fmt == m_YStream[s].cap->img_fmt ) {
                frm_size = m_YStream[s].cap->img_w * m_YStream[s].cap->img_h;
                if (frm_size == req_frm_size) {
                    obj = &m_YStream[s];
                    break;
                }
                else if (frm_size >= req_frm_size) {
                    if (!obj) {
                        obj = &m_YStream[s];
                        cand_frm_size = frm_size;
                    }
                    else if (frm_size < cand_frm_size) {
                        obj = &m_YStream[s];
                        break;
                    }
                }
            }
        }
    }

    if (!obj)
        return NULL;

    obj->propt = *propt;

    /* New an Ystreamer instance */
    obj->id = MMPF_YStream_New(propt->w, propt->h ,  (propt->fmt==V4L2_PIX_FMT_GREY) ? MMP_TRUE : MMP_FALSE );
    if (obj->id >= MAX_GRAY_STREAM_NUM) {
        printc("#YStream open err\r\n");
        return NULL;
    }

    /* Assign buffers */
    err = MMPS_YStream_AssignBuf(obj);
    if (err)
        goto _Ypipe_res_err;

    if( propt->fmt== V4L2_PIX_FMT_GREY ) {
        pipe = MMPD_Fctl_AllocatePipe(propt->w, PIPE_LINK_FB_Y);
    } 
    else {
        pipe = MMPD_Fctl_AllocatePipe(propt->w, PIPE_LINK_FB);
    }
    
    if (pipe > MMP_IBC_PIPE_MAX) {
        RTNA_DBG_Str(0, "#YStream alloc pipe err\r\n");
		goto _Ypipe_res_err;
    }

    FCTL_PIPE_TO_LINK(pipe, obj->pipe);

    /* Config pipe */
    err = MMPS_YStream_ConfigPipe(obj);
    if (err) {
        printc("#YStream config pipe err\r\n");
        goto _Ypipe_cfg_err;
    }

    /* enable pipe */
    err = MMPS_YStream_EnablePipe(obj, MMP_TRUE);
    if (err) {
        printc("#YStream enable pipe err\r\n");
        goto _Ypipe_cfg_err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return obj;

_Ypipe_cfg_err:
    MMPD_Fctl_ReleasePipe(pipe);

_Ypipe_res_err:
    MMPF_YStream_Delete(obj->id);

    return NULL;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start the gray stream

 @param[in] obj     Gray stream object
 @param[in] v4l2_id Corresponded V4L2 stream id

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_YStream_Start(MMPS_YSTREAM_CLASS *obj, MMP_ULONG v4l2_id)
{
    MMP_ERR err;

    if (!obj)
        return MMP_YSTREAM_ERR_PARAMETER;
    if (obj->state != IPC_STREAMER_OPEN)
        return MMP_YSTREAM_ERR_STATE;

    err = MMPF_YStream_Start(obj->id, v4l2_id);
    if (err) {
        RTNA_DBG_Str(0, "#YStream start err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the gray stream

 @param[in] obj     Gray stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_YStream_Stop(MMPS_YSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
        return MMP_YSTREAM_ERR_PARAMETER;
    if (obj->state != IPC_STREAMER_START)
        return MMP_YSTREAM_ERR_STATE;

    err = MMPF_YStream_Stop(obj->id);
    if (err) {
        RTNA_DBG_Str(0, "#YStream stop err\r\n");
        return err;
    }

    obj->state = IPC_STREAMER_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close a instance of gray stream object

 @param[in] obj     Gray stream object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_YStream_Close(MMPS_YSTREAM_CLASS *obj)
{
    MMP_ERR err;

    if (!obj)
		return MMP_YSTREAM_ERR_PARAMETER;
	if (obj->state != IPC_STREAMER_OPEN)
		return MMP_YSTREAM_ERR_STATE;

    err = MMPS_YStream_EnablePipe(obj, MMP_FALSE);
    if (err != MMP_ERR_NONE) {
		RTNA_DBG_Str(0, "#YStream close err\r\n");
    }
    MMPD_Fctl_ReleasePipe(obj->pipe.ibcpipeID);
    MMPF_YStream_Delete(obj->id);

    obj->state = IPC_STREAMER_IDLE;

    return MMP_ERR_NONE;
}

#if 0
void ____YStream_Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_ReserveHeap
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for gray streams.
*/
static MMP_ERR MMPS_YStream_ReserveHeap(void)
{
    MMP_ULONG i, max_w, max_h, max_luma;
    MMPS_YSTREAM_CLASS *obj;

    // Assign heap according to the capability
    for(i = 0; i < MAX_GRAY_STREAM_NUM; i++) {
        obj = &m_YStream[i];
        if (obj->cap) {
            max_w = obj->cap->img_w;
            max_h = obj->cap->img_h;
            max_luma = ALIGN256(max_w * max_h); // Y
            if(obj->cap->img_fmt!=V4L2_PIX_FMT_GREY) {
                max_luma = ( max_luma * 3 ) >> 1 ;
            }
            // 2 luma frame buffer
            obj->heap.size = max_luma * GRAY_FRM_SLOTS;
            obj->heap.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                 obj->heap.size, 256);

            if (obj->heap.base == SYS_HEAP_MEM_INVALID)
                return MMP_YSTREAM_ERR_BUF;
            obj->heap.end = obj->heap.base + obj->heap.size;
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_AssignBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign buffers for a motion detection object

 @param[in] obj     Motion detection object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_YStream_AssignBuf(MMPS_YSTREAM_CLASS *obj)
{
    MMP_ULONG   i, ulCurAddr, ulFrmSize;

    if( obj->propt.fmt==V4L2_PIX_FMT_GREY ) {
	    ulFrmSize = obj->propt.w * obj->propt.h;
	}
	else {
	    ulFrmSize = (obj->propt.w * obj->propt.h * 3) >> 1;
	}
    if ((ulFrmSize * GRAY_FRM_SLOTS) > obj->heap.size) {
        printc("\t= [HeapMemErr] YStream =\r\n");
        return MMP_YSTREAM_ERR_BUF;
    }

    /*
	 * Assign buffers for luma frame
	 */
    ulCurAddr = obj->heap.base;
    for(i = 0; i < GRAY_FRM_SLOTS; i++) {
        obj->frmbuf[i].base = ulCurAddr;
        obj->frmbuf[i].size = ulFrmSize;
        obj->frmbuf[i].end  = obj->frmbuf[i].base + obj->frmbuf[i].size;
        ulCurAddr += ulFrmSize;
    }

    #if (DBG_YSTREAM_MEM_MAP)
    MMPS_YStream_MemMapDbg(obj);
    #endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_MemMapDbg
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Dump memory usage for a gray stream object

 @param[in] obj     Gray stream object

 @retval None.
*/
#if (DBG_YSTREAM_MEM_MAP)
static void MMPS_YStream_MemMapDbg(MMPS_YSTREAM_CLASS *obj)
{
    MMP_ULONG i;

    printc("=============================================================\r\n");
    printc("\tystream mem map, x%X-x%X\r\n",    obj->heap.base,
                                                obj->heap.end);
    printc("-------------------------------------------------------------\r\n");
    for(i = 0; i < GRAY_FRM_SLOTS; i++) {
        printc("\tfrm[%d]   : x%X\r\n",     i,
                                            obj->frmbuf[i].base);
    }
    printc("=============================================================\r\n");
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_ConfigPipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Configure pipe for gray stream

 @param[in] *obj    Gray stream object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_YStream_ConfigPipe(MMPS_YSTREAM_CLASS *obj)
{
    MMP_USHORT          i;
    MMP_ULONG           ulScalInW, ulScalInH , ulYSize = 0 ,ulUVSize = 0 ;
    MMP_SCAL_FIT_RANGE  fitrange;
    MMP_SCAL_GRAB_CTRL  grabctl;
    MMPD_FCTL_ATTR      fctlAttr;
    MMPS_YSTREAM_PROPT  *propt = &obj->propt;

    /* Parameter Check */
    if (!obj)
        return MMP_YSTREAM_ERR_PARAMETER;

    /* Get the sensor parameters */
    MMPS_Sensor_GetCurPrevScalInputRes(propt->snrID, &ulScalInW, &ulScalInH);

    // Config gray stream pipe
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

    MMPD_Scaler_GetGCDBestFitScale(&fitrange, &grabctl);
    ulYSize = propt->w * propt->h ;

    switch(obj->propt.fmt) {
    case V4L2_PIX_FMT_NV12:
        fctlAttr.colormode          = MMP_DISPLAY_COLOR_YUV420_INTERLEAVE ;
        ulUVSize = ulYSize >> 1 ;
        break;        
    case V4L2_PIX_FMT_I420:
        fctlAttr.colormode          = MMP_DISPLAY_COLOR_YUV420 ;
        ulUVSize = ulYSize >> 2 ;
        break;
    case V4L2_PIX_FMT_GREY:        
    default:
    fctlAttr.colormode          = MMP_DISPLAY_COLOR_Y;
        break;
    }
    fctlAttr.fctllink           = obj->pipe;
    fctlAttr.fitrange           = fitrange;
    fctlAttr.grabctl            = grabctl;
    fctlAttr.scalsrc            = MMP_SCAL_SOURCE_ISP;
    fctlAttr.bSetScalerSrc      = MMP_TRUE;
    fctlAttr.ubPipeLinkedSnr    = propt->snrID;
    fctlAttr.usBufCnt           = GRAY_FRM_SLOTS;
    fctlAttr.bUseRotateDMA      = MMP_FALSE;

    for (i = 0; i < fctlAttr.usBufCnt; i++) {
        fctlAttr.ulBaseAddr[i] = obj->frmbuf[i].base;
        switch(obj->propt.fmt) {
        case V4L2_PIX_FMT_NV12:
            fctlAttr.ulBaseUAddr[i] = fctlAttr.ulBaseAddr[i] + ulYSize ;
            fctlAttr.ulBaseVAddr[i] = fctlAttr.ulBaseAddr[i] + ulYSize ;
            break ;
        case V4L2_PIX_FMT_I420:
            fctlAttr.ulBaseUAddr[i] = fctlAttr.ulBaseAddr[i] + ulYSize;
            fctlAttr.ulBaseVAddr[i] = fctlAttr.ulBaseUAddr[i] + ulUVSize;
            break ;
        case V4L2_PIX_FMT_GREY:
        default:
            fctlAttr.ulBaseUAddr[i] = 0;
            fctlAttr.ulBaseVAddr[i] = 0;
            break ;
        }
    }

    MMPD_Fctl_SetPipeAttrForIbcFB(&fctlAttr);
    if(obj->propt.fmt==V4L2_PIX_FMT_GREY) {
        MMPD_Fctl_LinkPipeToWithId(obj->pipe.ibcpipeID, obj->id,LINK_GRAY );
    }
    else {
        MMPD_Fctl_LinkPipeToWithId(obj->pipe.ibcpipeID, obj->id,LINK_IVA );
    }
//printc(" ibc w ,h , fmt : %d,%d,0x%08x\r\n",propt->w , propt->h , obj->propt.fmt );
//printc(" ibc Y : 0x%08x , 0x%08x, 0x%08x\r\n",fctlAttr.ulBaseAddr[0],fctlAttr.ulBaseUAddr[0],fctlAttr.ulBaseVAddr[0] );
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_YStream_EnablePipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on or off pipe for gray stream.

 @param[in] *obj    Gray stream object
 @param[in] bEnable Enable and disable pipe path for gray stream.

 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPS_YStream_EnablePipe(MMPS_YSTREAM_CLASS *obj, MMP_BOOL bEnable)
{
    MMP_ERR         err = MMP_ERR_NONE;
    MMP_UBYTE       snrID = obj->propt.snrID;
    MMP_IBC_PIPEID  ibcID = obj->pipe.ibcpipeID;

    if ((obj->pipe_en == MMP_FALSE) && bEnable)
        err = MMPD_Fctl_EnablePreview(snrID, ibcID, bEnable, MMP_FALSE);
    else if (obj->pipe_en && (bEnable == MMP_FALSE))
        err = MMPD_Fctl_EnablePreview(snrID, ibcID, bEnable, MMP_FALSE);

    obj->pipe_en = bEnable;

    return err;
}
//#pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall" 
//#pragma O0
ait_module_init(MMPS_YStream_ModInit);
//#pragma
//#pragma arm section rodata, rwdata, zidata

#endif //(V4L2_GRAY)

/// @}
