/**
 @file mmpf_mdtc.c
 @brief Control functions of Motion Detection
 @author Alterman
 @version 1.0
*/

#include "includes_fw.h"
#include "mmpf_mdtc.h"
#include "mmu.h"
#include "md.h"

#include "ipc_cfg.h"

#if (SUPPORT_MDTC)
#if (USER_LOG)
extern unsigned int log10(unsigned int v);
#endif

/** @addtogroup MMPF_MDTC
@{
*/

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================
/*
 * Local Variables
 */
static MMPF_MDTC_HANDLE m_MD;
static unsigned int md_hist_obj_cntsum = 0;
//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define MMPF_MDTC_QueueDepth(q)     ((q)->size)

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================


//==============================================================================
//
//                              FUNCTION DEFINITION
//
//==============================================================================
/* log functions for MD library, please porting for it */
void MD_printk(char *fmt, ...)
{
    //printc(fmt);
    //RTNA_DBG_Str0("\r");
}

#if 0
void _____MDTC_INTERNAL_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Enqueue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Insert one element into the end of queue.

 @param[in] q       Queue
 @param[in] data    Data to be inserted
 
 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_MD_Enqueue(MMPF_MDTC_QUEUE *queue, MMP_UBYTE data)
{
    MMP_ULONG idx;
    OS_CRITICAL_INIT();

    if (!queue)
        return MMP_MDTC_ERR_POINTER;

    if (queue->size == MDTC_LUMA_SLOTS) {
        RTNA_DBG_Str0("md queue full\r\n");
        return MMP_MDTC_ERR_FULL;
    }

    OS_ENTER_CRITICAL();
    idx = queue->head + queue->size++;
    if (idx >= MDTC_LUMA_SLOTS)
        idx -= MDTC_LUMA_SLOTS;
    queue->data[idx] = data;
    OS_EXIT_CRITICAL();

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Dequeue
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Retrieve one element from the head of queue.

 @param[in]  q      Queue
 @param[out] data   Data retrieved from queue

 @retval It reports the status of the operation.
*/
static MMP_ERR MMPF_MD_Dequeue(MMPF_MDTC_QUEUE *queue, MMP_UBYTE *data)
{
    OS_CRITICAL_INIT();

    if (!queue)
        return MMP_MDTC_ERR_POINTER;

    if (queue->size == 0) {
        RTNA_DBG_Str0("md queue empty\r\n");
        return MMP_MDTC_ERR_EMPTY;
    }

    OS_ENTER_CRITICAL();
    *data = queue->data[queue->head];
    queue->size--;
    queue->head++;
    if (queue->head == MDTC_LUMA_SLOTS)
        queue->head = 0;
    OS_EXIT_CRITICAL();

    return MMP_ERR_NONE;
}

#if 0
void _____MDTC_PUBLIC_INTERFACE_____(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_BufSize
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get the size of working buffer needed.

 @param[in] w       Input luma width
 @param[in] h       Input luma height
 @param[in] div_x   Window divisions in horizontal direction
 @param[in] div_y   Window divisions in vertical direction
 
 @retval The size of working buffer.
*/
MMP_ULONG MMPF_MD_BufSize(MMP_ULONG   w,
                            MMP_ULONG   h,
                            MMP_UBYTE   div_x,
                            MMP_UBYTE   div_y)
{
    int bufsize = MD_get_buffer_info(w, h,  (1) , div_x, div_y);

    return bufsize;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Init
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Initialize motion detction engine.

 @param[in] md      Motion detction object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_MD_Init(MMPF_MDTC_CLASS *md)
{
    int ret,i;
    MMP_UBYTE *buf;
    /* MMP_ULONG x, y; */

    /* MD library initialize */
    buf = (MMP_UBYTE *)(DRAM_CACHE_VA(md->workbuf.base));
    ret = MD_init(buf, md->workbuf.size, md->w, md->h, MD_COLOR_Y);
    if (ret) {
        RTNA_DBG_Str0("MD_init failed\r\n");
        return MMP_MDTC_ERR_CFG;
    }
    #if MD_USE_ROI_TYPE==0
    /* Set MD detcting window */
    ret = MD_set_detect_window( md->window.x_start,
                                md->window.y_start,
                                md->window.x_end,
                                md->window.y_end,
                                md->window.div_x,
                                md->window.div_y);
    #else
    ret = MD_set_region_info(md->window.roi_num, md->roi);
    #endif
    #if 0 // for debug
    printc("md->w,h = %d,%d,roi=%d\r\n",md->w,md->h ,md->window.roi_num );
    for(i=0;i<md->window.roi_num;i++) {
      printc("[%d]=%d,%d,%d,%d\r\n",i,md->roi[i].st_x,md->roi[i].st_y,md->roi[i].end_x,md->roi[i].end_y );
    }    
    #endif                            
    if (ret) {
        RTNA_DBG_Str0("MD_setwin failed\r\n");
        return MMP_MDTC_ERR_CFG;
    }

    /* Set parameters for each sub-windows */
    #if MD_USE_ROI_TYPE==0
    for(x = 0; x < md->window.div_x; x++) {
        for(y = 0; y < md->window.div_y; y++) {
            ret = MD_set_window_params_in(x, y, &md->param[x][y]);
            if (ret) {
                RTNA_DBG_Str0("MD_setparam failed\r\n");
                return MMP_MDTC_ERR_CFG;
            }
        }
    }
    #else
    MD_set_window_params_in(0,0, md->param );
    #endif
    md->tick = 0;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Enable motion detection engine.

 @param[in] md      Motion detction object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_MD_Open(MMPF_MDTC_CLASS *md)
{
    MMP_ULONG i;

    MEMSET(&m_MD.free_q, 0, sizeof(MMPF_MDTC_QUEUE));
    MEMSET(&m_MD.rdy_q,  0, sizeof(MMPF_MDTC_QUEUE));

    /* Put luma slot buffer into free queue. The 1st slot is already occupied */
    for(i = 1; i < md->luma_slots; i++) {
        MMPF_MD_Enqueue(&m_MD.free_q, i);
    }

    m_MD.busy   = MMP_FALSE;
    m_MD.obj    = md;

    /*clean md histgram*/
    MEMSET( &md->md_hist,0, sizeof(md->md_hist) );

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Disable motion detection engine.

 @param[in] md      Motion detction object

 @retval It reports the status of the operation.
*/
MMP_ERR MMPF_MD_Close(MMPF_MDTC_CLASS *md)
{
    m_MD.obj = NULL;

    /* Wakeup MDTC task */
    MMPF_OS_SetFlags(MDTC_Flag, MD_FLAG_RUN, MMPF_OS_FLAG_SET);

    while(m_MD.busy)
        MMPF_OS_Sleep(5);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_FrameReady
//  Description :
//------------------------------------------------------------------------------
/**
 @brief One input luma frame for motion detection is ready.

 @param[in] buf_idx Index of buffer which stored a ready luma frame

 @retval It reports the status of the operation.
*/
void MMPF_MD_FrameReady(MMP_UBYTE *buf_idx)
{
    if (!m_MD.obj)  // not yet open
        return;

    MMPF_MD_Enqueue(&m_MD.rdy_q, *buf_idx);

    if (MMPF_MDTC_QueueDepth(&m_MD.free_q) > 0) {
        MMPF_MD_Dequeue(&m_MD.free_q, buf_idx);
    }
    else {
        MMPF_MD_Dequeue(&m_MD.rdy_q, buf_idx);
    }

    /* Trigger motion detection */
    MMPF_OS_SetFlags(MDTC_Flag, MD_FLAG_RUN, MMPF_OS_FLAG_SET);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_MD_Run
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Do motion detection.

 @retval It reports the status of the operation.
*/
void MMPF_MD_Run(void)
{
#if SUPPORT_AEC
extern MMP_BOOL MMPF_AEC_IsEnable(void);
#endif

static int last = 0 ;
    int ret ;
    MMP_UBYTE buf_idx;
    MMP_ULONG x, /*y,*/ now;
    MMP_UBYTE *luma;
    MMPF_MDTC_CLASS *md = NULL;
    int md_run_ticks ;
    
    m_MD.busy = MMP_TRUE;

    md = m_MD.obj;
    if (md) {
        if (MMPF_MDTC_QueueDepth(&m_MD.rdy_q) == 0)
            goto _exit_md_run;

        /* Updates time gap to library for fine tune internal parameters */
        now = OSTime;
        if (md->tick && (now > md->tick))
            MD_set_time_ms(now - md->tick);

        
        /* Do motion detection */
        MMPF_MD_Dequeue(&m_MD.rdy_q, &buf_idx);
        luma = (MMP_UBYTE *)(DRAM_CACHE_VA(md->luma[buf_idx].base));
        
        ret = MD_run(luma);

        /* Get motion detection result */
        
        if (md->tick /* && (ret > 0)*/) { // alyways update so that dualcpu_md.c can get real-time result
            /* MMP_USHORT md_win ; */
            if(ret != last) {
                printc("[#MD %d]",ret);
            }
            #if MD_USE_ROI_TYPE==0
            for (x = 0; x < md->window.div_x; x++) {
                for (y = 0; y < md->window.div_y; y++) {
                    MD_get_window_params_out(x, y, &md->result[x][y]);
                    if (md->result[x][y].md_result) {
                        /*
                        RTNA_DBG_Str0("\t(");
                        MMPF_DBG_Int(x, -1);
                        RTNA_DBG_Str0(",");
                        MMPF_DBG_Int(y, -1);
                        RTNA_DBG_Str0(")");
                        */
                       // break;
                       md_win =  x * md->window.div_y + y  ;
                       md->md_hist.obj_axis[md_win]++ ;
                       md->md_hist.obj_cnt++;
                       md_hist_obj_cntsum++;
                    }
                }
            }
            #else
            MD_get_window_params_out(0,0,md->result);
            for (x = 0; x < md->window.roi_num ; x++) {
                if (md->result[x].md_result) {
                    md->md_hist.obj_axis[x]++;
                    md->md_hist.obj_cnt++;
		    md_hist_obj_cntsum++;    
                }
            }
            #endif
            if(ret != last) {
                if(md->md_change) {
                    md->md_change( ret,last ) ;
                }
                printc("\r\n");
            }
            last = ret ;
        }
        #if SUPPORT_AEC
        if( MMPF_AEC_IsEnable()) {
          md_run_ticks = 500 ;
        }
        else 
        #endif
        {
          md_run_ticks = 100 ;
        }
        
        if( (OSTime - now ) < md_run_ticks ) {
          MMPF_OS_Sleep( md_run_ticks - (OSTime - now ) );
        }

        md->tick = now;
        /* Release luma buffer */
        MMPF_MD_Enqueue(&m_MD.free_q, buf_idx);
    }

_exit_md_run:
    md->md_hist.obj_cnt_sum = md_hist_obj_cntsum;
    m_MD.busy = MMP_FALSE;
}

MMP_ERR MMPF_MD_Update( MD_params_in_t *param)
{
#if MD_USE_ROI_TYPE==1
//static MD_params_in_t cur_param[MDTC_MAX_ROI] ;
  MMPF_MDTC_CLASS *md = m_MD.obj;  
  if( md /*&& m_MD.busy */) {
      int i;
      MD_params_in_t *in_roi_param ;
      MD_get_window_params_in(0,0, md->param ) ;
      printc("Cur ROI info...\r\n");
#if 0      
      for(i=0;i<MDTC_MAX_ROI;i++) {
        if( md->param[i].enable ) {
          printc("ROI[%d].sensitivity : %d\r\n",i, md->param[i].sensitivity );
          printc("ROI[%d].size_perct_thd_min : %d\r\n",i, md->param[i].size_perct_thd_min );
          printc("ROI[%d].size_perct_thd_max : %d\r\n",i, md->param[i].size_perct_thd_max );
          printc("ROI[%d].learn_rate : %d\r\n",i, md->param[i].learn_rate );          
        }
      }
#endif
      
      for(i=0;i<MDTC_MAX_ROI;i++) {
        in_roi_param = (MD_params_in_t *)( (MMP_ULONG)param + i * sizeof( MD_params_in_t ) );

        
        if( in_roi_param->enable!= (unsigned char)-1) {
          md->param[i].enable = in_roi_param->enable ;
          printc("ROI#%d sensitivity from %d to %d\r\n",i,md->param[i].sensitivity, in_roi_param->sensitivity );
        }
        
        if(1/* md->param[i].enable*/ ) {
          if( in_roi_param->size_perct_thd_min != (unsigned char)-1 ) {
            md->param[i].size_perct_thd_min = in_roi_param->size_perct_thd_min ;
          }
          if( in_roi_param->size_perct_thd_max != (unsigned char)-1 ) {
            md->param[i].size_perct_thd_max = in_roi_param->size_perct_thd_max;
          }
          if( in_roi_param->sensitivity != (unsigned char)-1) {
            md->param[i].sensitivity = in_roi_param->sensitivity;
          }
          if( in_roi_param->learn_rate != (unsigned short)-1) {
            md->param[i].learn_rate = in_roi_param->learn_rate ;
          }
        }
        
      } 
      MD_set_window_params_in(0,0, md->param ) ;
  }    
  return MMP_ERR_NONE ;
#endif  
}

/**
 @brief Main routine of Motion Detection task.
*/
void MMPF_MD_Task(void)
{
    MMPF_OS_FLAGS wait_flags = 0, flags;

    RTNA_DBG_Str3("MD_Task()\r\n");

    wait_flags = MD_FLAG_RUN;

    while(1) {
        MMPF_OS_WaitFlags(  MDTC_Flag, wait_flags,
                            MMPF_OS_FLAG_WAIT_SET_ANY | MMPF_OS_FLAG_CONSUME,
                            0, &flags);

        if (flags & MD_FLAG_RUN)
            MMPF_MD_Run();
    }
}

/// @}

#endif // (SUPPORT_MDTC)
