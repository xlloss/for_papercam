#ifndef _HDM_CTL_H_
#define _HDM_CTL_H_

#include "isp_if.h"
#include "config_fw.h"

#define IQ_OPR_DMA_ON		(1)
#define ISP_BUFFER_NUM      (1)

#if (CHIP == MCR_V2)
#define ISP_AWB_BUF_SIZE    (5*1024)
#define ISP_AE_BUF_SIZE     (1024)
#define ISP_AF_BUF_SIZE     (1024)
#define ISP_DFT_BUF_SIZE    (0)
#define ISP_HIST_BUF_SIZE   (256*4)
#define ISP_FLICK_BUF_SIZE  (256*4)
#endif

#if (IQ_OPR_DMA_ON == 1)
    #define IQ_BANK_SIZE    (256*16)
    #define LS_BANK_SIZE    (256*(3+1)*2*2)
    #define CS_BANK_SIZE    (42*32*3*2*2)
#else
    #define IQ_BANK_SIZE    (0)
    #define LS_BANK_SIZE    (0)
    #define CS_BANK_SIZE    (0)
#endif

#if (IQ_OPR_DMA_ON == 1)
#define IQ_BANK_ADDR		(((m_glISPBufferStartAddr+ISP_AWB_BUF_SIZE+ISP_AE_BUF_SIZE+ISP_AF_BUF_SIZE+ISP_DFT_BUF_SIZE+ISP_HIST_BUF_SIZE+ISP_FLICK_BUF_SIZE+256) >> 8 << 8))
#define LS_BANK_ADDR		(((m_glISPBufferStartAddr+ISP_AWB_BUF_SIZE+ISP_AE_BUF_SIZE+ISP_AF_BUF_SIZE+ISP_DFT_BUF_SIZE+ISP_HIST_BUF_SIZE+ISP_FLICK_BUF_SIZE+256) >> 8 << 8)+IQ_BANK_SIZE)
#define CS_BANK_ADDR		(((m_glISPBufferStartAddr+ISP_AWB_BUF_SIZE+ISP_AE_BUF_SIZE+ISP_AF_BUF_SIZE+ISP_DFT_BUF_SIZE+ISP_HIST_BUF_SIZE+ISP_FLICK_BUF_SIZE+256) >> 8 << 8)+IQ_BANK_SIZE+LS_BANK_SIZE)
#else
#define IQ_BANK_ADDR		(0x80000000)
#define LS_BANK_ADDR		(0x80000000)
#define CS_BANK_ADDR		(0x80000000)
#endif

#define IQ_OPR_DMA_SIZE     (IQ_BANK_SIZE+LS_BANK_SIZE+CS_BANK_SIZE+256) //256 alignment
#define ISP_BUFFER_SIZE     (ISP_AE_BUF_SIZE+ISP_AF_BUF_SIZE+ISP_AWB_BUF_SIZE+ISP_DFT_BUF_SIZE+ISP_HIST_BUF_SIZE+ISP_FLICK_BUF_SIZE+IQ_OPR_DMA_SIZE)

#define ISP_GNR_OFF         (0)

void ISP_HDM_IF_CALI_GetData(void);
ISP_UINT8 ISP_HDM_IF_IQ_IsApicalClkOff(void);
ISP_UINT32 ISP_HDM_IF_LIB_GetBufAddr(ISP_BUFFER_CLASS buf_class, ISP_UINT32 buf_size, ISP_BUFFER_TYPE buf_type);

ISP_UINT32 ISP_HDM_IF_LIB_RamAddrV2P(ISP_UINT32 addr);
ISP_UINT32 ISP_HDM_IF_LIB_RamAddrP2V(ISP_UINT32 addr);
ISP_UINT32 ISP_HDM_IF_LIB_OprAddrV2P(ISP_UINT32 addr);
ISP_UINT32 ISP_HDM_IF_LIB_OprAddrP2V(ISP_UINT32 addr);


#endif // _HDM_CTL_H_

