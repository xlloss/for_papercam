/**
 @file mmps_mdtc.c
 @brief Motion Detection Control Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmps_mdtc.h"
#include "mmps_sensor.h"
#include "mmpd_scaler.h"
#include "mmpf_system.h"

/** @addtogroup MMPS_MDTC
@{
*/

#if (SUPPORT_MDTC)

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 *  Local Variables
 */
static MMPS_MDTC_CLASS      m_MdHanle;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static MMP_ERR MMPS_MDTC_ReserveHeap(void);

static MMP_ERR MMPS_MDTC_AssignBuf(MMPS_MDTC_CLASS *obj);

#if (MDTC_MEM_MAP_DBG)
static void    MMPS_MDTC_MemMapDbg(MMPS_MDTC_CLASS *obj);
#endif

static MMP_ERR MMPS_MDTC_ConfigPipe(MMPS_MDTC_CLASS *obj);

static MMP_ERR MMPS_MDTC_EnablePipe(MMPS_MDTC_CLASS *obj, MMP_BOOL bEnable);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____MDTC_Module_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_ModInit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize motion detection module

 @retval It reports the status of the operation.
*/
int MMPS_MDTC_ModInit(void)
{
    static MMP_BOOL _init = MMP_FALSE;

    if (_init == MMP_FALSE) {

        MEMSET(&m_MdHanle, 0, sizeof(MMPS_MDTC_CLASS));

        m_MdHanle.cap = &gstMdtcCap;
        MMPS_MDTC_ReserveHeap(); // Reserve heap buffer for each stream

        _init = MMP_TRUE;
    }
    return 0;
}


#if 0
void ____MDTC_Obj_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Open a instance of motion detection object

 @param[in] propt   Requested properties for motion detection

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_MDTC_Open(MMPS_MDTC_PROPT *propt,void (*md_change)(int new_val,int old_val) )
{
    MMP_ERR   err;
    MMP_ULONG pipe, x /*, y*/;
    MMP_ULONG req_frmsize, cap_frmsize;
    MMPS_MDTC_CLASS *obj = &m_MdHanle;

    if (!propt)
        return 0;
    if (obj->state != MDTC_STATE_IDLE)
        return MMP_MDTC_ERR_STATE;

    req_frmsize = propt->luma_w * propt->luma_h;
    cap_frmsize = obj->cap->luma_w * obj->cap->luma_h;

    if (req_frmsize > cap_frmsize)
        return MMP_MDTC_ERR_BUF;

    /* Check properties */
    #if MD_USE_ROI_TYPE==0
    if (propt->window.div_x > MDTC_MAX_DIV_X)
        return MMP_MDTC_ERR_CFG;
    else if (propt->window.div_y > MDTC_MAX_DIV_Y)
        return MMP_MDTC_ERR_CFG;
    #else
    if (propt->window.roi_num > MDTC_MAX_ROI)
        return MMP_MDTC_ERR_CFG;
    #endif
    /* Keep properties */
    obj->snrID      = propt->snrID;
    obj->md.w       = propt->luma_w;
    obj->md.h       = propt->luma_h;
    obj->md.window  = propt->window;
    #if MD_USE_ROI_TYPE==0 
    for(x = 0; x < propt->window.div_x; x++) {
        for(y = 0; y < propt->window.div_y; y++)
            obj->md.param[x][y] = propt->param[x][y];
    }
    #else
    for(x = 0; x < propt->window.roi_num; x++) {
        obj->md.param[x] = propt->param[x];
        obj->md.roi[x]   = propt->roi[x];
        #if 0
        printc("roi[%d]=%d,%d,%d,%d\r\n",x
                                        ,obj->md.roi[x].st_x
                                        ,obj->md.roi[x].st_y
                                        ,obj->md.roi[x].end_x
                                        ,obj->md.roi[x].end_y );

        printc("param[%d]=%d,%d,%d,%d\r\n",x
                                          ,obj->md.param[x].size_perct_thd_min
                                          ,obj->md.param[x].size_perct_thd_max
                                          ,obj->md.param[x].sensitivity
                                          ,obj->md.param[x].learn_rate );
        #endif
                                        
    }
    #endif
    /* Assign buffers */
    err = MMPS_MDTC_AssignBuf(obj);
    if (err)
        return err;

    /* Assign pipe resource */
    pipe = MMPD_Fctl_AllocatePipe(propt->luma_w, PIPE_LINK_FB_Y);
    if (pipe > MMP_IBC_PIPE_MAX) {
        printc("#MDTC alloc pipe err\r\n");
		return 0;
    }

    FCTL_PIPE_TO_LINK(pipe, obj->pipe);

    /* Config pipe */
    err = MMPS_MDTC_ConfigPipe(obj);
    if (err) {
        printc("#MDTC config pipe err\r\n");
        goto _pipe_cfg_err;
    }

    err = MMPF_MD_Init(&obj->md);
    if (err) {
        printc("#MD init err\r\n");
        goto _pipe_cfg_err;
    }

    obj->state = MDTC_STATE_OPEN;
    obj->md.md_change = md_change ;
    return MMP_ERR_NONE;

_pipe_cfg_err:
    MMPD_Fctl_ReleasePipe(pipe);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_Start
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start motion detection

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_MDTC_Start(void)
{
    MMP_ERR err;
    MMPS_MDTC_CLASS *obj = &m_MdHanle;

	if (obj->state != MDTC_STATE_OPEN)
		return MMP_MDTC_ERR_STATE;

    MMPF_MD_Open(&obj->md);
    err = MMPS_MDTC_EnablePipe(obj, MMP_TRUE);
    if (err) {
        printc("#MDTC start err\r\n");
        return err;
    }

    obj->state = MDTC_STATE_START;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_Stop
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the motion detection

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_MDTC_Stop(void)
{
    MMP_ERR err;
    MMPS_MDTC_CLASS *obj = &m_MdHanle;

    if (obj->state != MDTC_STATE_START)
        return MMP_MDTC_ERR_STATE;

    MMPF_MD_Close(&obj->md);
    err = MMPS_MDTC_EnablePipe(obj, MMP_FALSE);
    if (err) {
        printc("#MDTC stop err\r\n");
        return err;
    }

    obj->state = MDTC_STATE_OPEN;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Close a instance of motion detection object

 @param[in] obj     Motion detection object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPS_MDTC_Close(void)
{
    MMP_ERR err;
    MMPS_MDTC_CLASS *obj = &m_MdHanle;

    if (obj->state != MDTC_STATE_OPEN)
        return MMP_MDTC_ERR_STATE;

    MMPD_Fctl_LinkPipeTo(obj->pipe.ibcpipeID, LINK_NONE);
    err = MMPD_Fctl_ReleasePipe(obj->pipe.ibcpipeID);
    if (err != MMP_ERR_NONE) {
        printc("#MDTC close enc err\r\n");
    }

    obj->state = IPC_STREAMER_IDLE;

    return MMP_ERR_NONE;
}

MMPS_MDTC_CLASS *MMPS_MDTC_Obj(void)
{
    return (MMPS_MDTC_CLASS *)&m_MdHanle;
}

#if 0
void ____MDTC_Internal_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_ReserveHeap
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Reserve memory for motion detection.
*/
static MMP_ERR MMPS_MDTC_ReserveHeap(void)
{
    MMP_ULONG max_w, max_h, max_luma, workbuf_size;
    MMPS_MDTC_CLASS *obj = &m_MdHanle;

    // Assign heap according to the capability
    if (obj->cap) {
        max_w = ALIGN16(obj->cap->luma_w);
        max_h = ALIGN16(obj->cap->luma_h);
        max_luma = max_w * max_h; // Y

        obj->ipc_buf.size = 1024;
        obj->ipc_buf.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                                obj->ipc_buf.size, 256);
        if (obj->ipc_buf.base == SYS_HEAP_MEM_INVALID)
            return MMP_MDTC_ERR_BUF;
        obj->ipc_buf.end  = obj->ipc_buf.base + obj->ipc_buf.size;

        // 2 luma frame buffer
        obj->heap.size = max_luma * MDTC_LUMA_SLOTS;
        // working buffer size
        workbuf_size = MMPF_MD_BufSize( max_w, max_h,
                                        obj->cap->div_x,
                                        obj->cap->div_y);
        obj->heap.size += workbuf_size;
        obj->heap.base = MMPF_SYS_HeapMalloc(SYS_HEAP_DRAM,
                                             obj->heap.size, 256);

        if (obj->heap.base == SYS_HEAP_MEM_INVALID)
            return MMP_MDTC_ERR_BUF;
        obj->heap.end  = obj->heap.base + obj->heap.size;
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_AssignBuf
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Assign buffers for a motion detection object

 @param[in] obj     Motion detection object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_MDTC_AssignBuf(MMPS_MDTC_CLASS *obj)
{
    MMP_ULONG       i, ulCurAddr;
    MMP_ULONG       ulFrmSize, ulWorkBufSize;

	ulFrmSize = ALIGN256(obj->md.w * obj->md.h);
    ulWorkBufSize = MMPF_MD_BufSize(obj->md.w, obj->md.h, 
                                    obj->md.window.div_x,
                                    obj->md.window.div_y);

    if (((ulFrmSize * MDTC_LUMA_SLOTS) + ulWorkBufSize) > obj->heap.size) {
        printc("\t= [HeapMemErr] MDTC =\r\n");
        return MMP_MDTC_ERR_BUF;
    }

    /*
	 * Assign buffers for luma frame
	 */
    ulCurAddr = obj->heap.base;
    obj->md.luma_slots = MDTC_LUMA_SLOTS;
    for(i = 0; i < obj->md.luma_slots; i++) {
        obj->md.luma[i].base = ulCurAddr;
        obj->md.luma[i].size = ulFrmSize;
        ulCurAddr += ulFrmSize;
    }

    /*
     * Allocate working buffer
     */
    obj->md.workbuf.base = ulCurAddr;
    obj->md.workbuf.size = ulWorkBufSize;

    #if (MDTC_MEM_MAP_DBG)
    MMPS_MDTC_MemMapDbg(obj);
    #endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_MemMapDbg
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Dump memory usage for a motion detection object

 @param[in] obj     Motion detection object

 @retval None.
*/
#if (MDTC_MEM_MAP_DBG)
static void MMPS_MDTC_MemMapDbg(MMPS_MDTC_CLASS *obj)
{
    MMP_ULONG i;

    printc("=============================================================\r\n");
    printc("\tmotion detection mem map, x%X-x%X\r\n",   obj->heap.base,
                                                        obj->heap.end);
    printc("-------------------------------------------------------------\r\n");

    for(i = 0; i < obj->md.luma_slots; i++) {
        printc("\tLuma[%d] : x%X\r\n",      i,
                                            obj->md.luma[i].base);
    }
    printc("\tWorkbuf : x%X (%d)\r\n",      obj->md.workbuf.base,
                                            obj->md.workbuf.size);
    printc("=============================================================\r\n");
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_ConfigPipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Configure pipe for motion detection

 @param[in] *obj        Motion detection object

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPS_MDTC_ConfigPipe(MMPS_MDTC_CLASS *obj)
{
    MMP_ULONG           ulScalInW, ulScalInH;
    MMP_SCAL_FIT_RANGE  fitrange;
    MMP_SCAL_GRAB_CTRL  grabctl;
    MMPD_FCTL_ATTR      fctlAttr;
    MMP_USHORT          i;

    /* Parameter Check */
    if (!obj)
        return MMP_MDTC_ERR_PARAMETER;

    /* Get the sensor parameters */
    MMPS_Sensor_GetCurPrevScalInputRes(obj->snrID, &ulScalInW, &ulScalInH);

    // Config pipe for Y frame
    fitrange.fitmode        = MMP_SCAL_FITMODE_OUT;
    fitrange.scalerType 	= MMP_SCAL_TYPE_SCALER;
    fitrange.ulInWidth 	    = ulScalInW;
    fitrange.ulInHeight	    = ulScalInH;
    fitrange.ulOutWidth     = obj->md.w;
    fitrange.ulOutHeight    = obj->md.h;
    fitrange.ulInGrabX 		= 1;
    fitrange.ulInGrabY 		= 1;
    fitrange.ulInGrabW 		= fitrange.ulInWidth;
    fitrange.ulInGrabH 		= fitrange.ulInHeight;

    MMPD_Scaler_GetGCDBestFitScale(&fitrange, &grabctl);

    fctlAttr.colormode          = MMP_DISPLAY_COLOR_Y;
    fctlAttr.fctllink           = obj->pipe;
    fctlAttr.fitrange           = fitrange;
    fctlAttr.grabctl            = grabctl;
    fctlAttr.scalsrc            = MMP_SCAL_SOURCE_ISP;
    fctlAttr.bSetScalerSrc      = MMP_TRUE;
    fctlAttr.ubPipeLinkedSnr    = obj->snrID;
    fctlAttr.usBufCnt           = obj->md.luma_slots;
    fctlAttr.bUseRotateDMA      = MMP_FALSE;

    for (i = 0; i < fctlAttr.usBufCnt; i++) {
        fctlAttr.ulBaseAddr[i] = obj->md.luma[i].base;
        fctlAttr.ulBaseUAddr[i] = 0;
        fctlAttr.ulBaseVAddr[i] = 0;
    }

    MMPD_Fctl_SetPipeAttrForIbcFB(&fctlAttr);
    MMPD_Fctl_LinkPipeTo(obj->pipe.ibcpipeID, LINK_MDTC);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPS_MDTC_EnablePipe
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Turn on or off pipe for motion detection.

 @param[in] *obj    Motion detection object
 @param[in] bEnable Enable and disable pipe path for motion detection.

 @retval MMP_ERR_NONE Success.
*/
static MMP_ERR MMPS_MDTC_EnablePipe(MMPS_MDTC_CLASS *obj, MMP_BOOL bEnable)
{
    MMP_ERR err = MMP_ERR_NONE;
    MMP_IBC_PIPEID ibcID = obj->pipe.ibcpipeID;

    if ((obj->pipe_en == MMP_FALSE) && bEnable)
        err = MMPD_Fctl_EnablePreview(obj->snrID, ibcID, bEnable, MMP_FALSE);
    else if (obj->pipe_en && (bEnable == MMP_FALSE))
        err = MMPD_Fctl_EnablePreview(obj->snrID, ibcID, bEnable, MMP_FALSE);

    obj->pipe_en = bEnable;

    return err;
}
//#pragma arm section code = "initcall", rodata = "initcall", rwdata = "initcall",  zidata = "initcall" 
//#pragma O0
ait_module_init(MMPS_MDTC_ModInit);
//#pragma
//#pragma arm section rodata, rwdata, zidata

#endif // (SUPPORT_MDTC)

/// @}
