#ifndef DUALCPU_ALSA_H
#define DUALCPU_ALSA_H

#include "mmp_lib.h"
#include "aitu_ringbuf.h"

//==============================================================================
//
//                              COMPILING OPTIONS
//
//==============================================================================

#define ALSA_RINGBUF_DBG    (0)

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

#define ALSA_MIC            (0)
#define ALSA_SPK            (1)
#define ALSA_SRC_END        (2)
#define ALSA_MAX_STREAMS    (ALSA_SRC_END)

#define AFE_OWNER_IS_RTOS   (0)
#define AFE_OWNER_IS_LINUX  (1)

#define MCRV2_SPK_MIN_DB    (-38)
#define MCRV2_SPK_DEF_DB    (0)
#define MCRV2_SPK_MAX_DB    (12)

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define ALSA_DGAIN(gain)    (gain & 0xffff)
#define ALSA_AGAIN(gain)    ((gain >> 16) & 0xffff)

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum {
    ALSA_IPC_STATE_CLOSE = 0,
    ALSA_IPC_STATE_OPEN,
    ALSA_IPC_STATE_ON,
    ALSA_IPC_STATE_OFF,
    ALSA_IPC_STATE_NUM
} alsa_ipc_state;

/* Audio enhancement */
typedef enum {
    AUDIO_ENH_NONE,
    AUDIO_ENH_AEC,
    AUDIO_ENH_AEC_DBG,
    AUDIO_ENH_NR,
    AUDIO_ENH_WNR
} AUDIO_ENH_MODE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef struct ring_bufctl_s {
    void            *dma_inbuf_virt_addr;
    void            *dma_outbuf_virt_addr;
    MMP_ULONG       dma_inbuf_phy_addr;
    MMP_ULONG       dma_outbuf_phy_addr;
    int             dma_inbuf_buffer_size;
    int             dma_outbuf_buffer_size;
    AUTL_RINGBUF    in_buf_h;
    AUTL_RINGBUF    out_buf_h;    
} ring_bufctl_t;

typedef struct __attribute__((__packed__)) _alsa_audio_info {
    int             aud_path;
    int             sample_rate;
    int             channels;
    int             irq_threshold;
}alsa_audio_info;

typedef struct __attribute__((__packed__)) _alsa_ipc_info {
    alsa_audio_info aud_info;
    ring_bufctl_t   *pcm_buffer;
}alsa_ipc_info;

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

#endif //DUALCPU_ALSA_H

