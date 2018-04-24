//==============================================================================
//
//  File        : mmp_reg_dma.h
//  Description : INCLUDE File for the Retina register map.
//  Author      : Rogers Chen
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMP_REG_DMA_H_
#define _MMP_REG_DMA_H_

#include "mmp_register.h"

/** @addtogroup MMPH_reg
@{
*/

#define DMA_M_NUM 2
#define DMA_R_NUM 2

#if (CHIP == MCR_V2)
//--------------------------------------------
// DMA_M structure
//--------------------------------------------
typedef struct _AITS_DMA_M {
    AIT_REG_D   DMA_M_SRC_ADDR;                                     // 0x10
    AIT_REG_D   DMA_M_BYTE_CNT;                                     // 0x14
        /*-DEFINE-----------------------------------------------------*/  
        #define DMA_M_MAX_BYTE_CNT       0x0FFFFFFF
        /*------------------------------------------------------------*/    
    AIT_REG_D   DMA_M_DST_ADDR;                                     // 0x18
    AIT_REG_D   DMA_M_LEFT_BYTE;                                    // 0x1C [RO]
} AITS_DMA_M, *AITPS_DMA_M;

//--------------------------------------------
// DMA_M_LOFFS structure
//--------------------------------------------
typedef struct _AITS_DMA_M_OFFL {
    AIT_REG_D   DMA_M_SRC_LOFFS_W;                                  // 0x70
    AIT_REG_D   DMA_M_SRC_LOFFS_OFFS;                               // 0x74
    AIT_REG_D   DMA_M_DST_LOFFS_W;                                  // 0x78
    AIT_REG_D   DMA_M_DST_LOFFS_OFFS;                               // 0x7C
} AITS_DMA_M_LOFFS, *AITPS_DMA_M_LOFFS;

//--------------------------------------------
// DMA_R structure
//--------------------------------------------
typedef struct _AITS_DMA_R {
    AIT_REG_D   DMA_R_SRC_ADDR;                                     // 0x30
    AIT_REG_W   DMA_R_SRC_OFST;                                     // 0x34
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_R_MAX_OFST          0x7FFF
        /*------------------------------------------------------------*/
    AIT_REG_B                           _x06[0x2];   
    AIT_REG_D   DMA_R_DST_ADDR;                                     // 0x38
    AIT_REG_W   DMA_R_DST_OFST;                                     // 0x3C
    AIT_REG_B                           _x3E[0x2];
    
    AIT_REG_W   DMA_R_PIX_W;                                        // 0x40
    AIT_REG_W   DMA_R_PIX_H;                                        // 0x42
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_R_MAX_PIX_W_H       0x2000
        /*------------------------------------------------------------*/
    AIT_REG_B   DMA_R_CTL;                                          // 0x44
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_R_NO                0x00
        #define DMA_R_90            	0x01
        #define DMA_R_270           	0x02
        #define DMA_R_180           	0x03
        #define DMA_R_BLK_128X128       0x04
        #define DMA_R_BLK_64X64         0x00
        #define DMA_R_BPP_32            0x18
        #define DMA_R_BPP_24            0x10
        #define DMA_R_BPP_16            0x08
        #define DMA_R_BPP_8             0x00
        /*------------------------------------------------------------*/ 
    AIT_REG_B	DMA_R_MIRROR_EN;								    // 0x45
        /*-DEFINE-----------------------------------------------------*/
    	#define DMA_R_MIRROR_DISABLE    0x00
        #define DMA_R_H_ENABLE          0x01
        #define DMA_R_V_ENABLE          0x02
        /*------------------------------------------------------------*/
    AIT_REG_B                           _x46[0x2];
    AIT_REG_D   DMA_R_RD_BLK_START_ADDR;                            // 0x48 [RO]
    AIT_REG_B                           _x4C[0x4];
    
    AIT_REG_D   DMA_R_BYTE_CNT;                                     // 0x50
    AIT_REG_W   DMA_R_BYTE_W;                                       // 0x54
    AIT_REG_B                           _x56[0x2];
    AIT_REG_D   DMA_R_LEFT_BYTE;                                    // 0x58 [RO]
    AIT_REG_B                           _x5C[0x4];
} AITS_DMA_R, *AITPS_DMA_R;

//--------------------------------------------
// DMA_DESCP structure
//--------------------------------------------
typedef struct _AITS_DMA_DESCP {
    AIT_REG_D   DMA_DESCP_ADDR;                             // 0x00
    AIT_REG_B   DMA_DESCP_CHK_SIG;                          // 0x04
    AIT_REG_B                           _x05;
    AIT_REG_B   DMA_DESCP_IDX;                              // 0x06 [RO]
    AIT_REG_B                           _x07;
    AIT_REG_W   DMA_DESCP_INT_TH;                           // 0x08
    AIT_REG_W   DMA_DESCP_DONE_CNT;                         // 0x0A [RO]
    AIT_REG_B                           _x0C[0x4];
} AITS_DMA_DESCP, *AITPS_DMA_DESCP;

//--------------------------------------------
// DMA structure (0x8000 7600)
//--------------------------------------------
typedef struct _AITS_DMA {
    AIT_REG_B   DMA_EN;                                     // 0x00
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_M0_EN               0x01
        #define DMA_M1_EN               0x02
        #define DMA_R0_EN               0x04
        #define DMA_R1_EN               0x08
        /*------------------------------------------------------------*/
    AIT_REG_B                           _x01;
    AIT_REG_B   DMA_M_LOFFS_EN;                             // 0x02
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_M0_LOFFS_EN         0x01
        #define DMA_M1_LOFFS_EN         0x02
        /*------------------------------------------------------------*/
    AIT_REG_B   DMA_R_CONT_EN;                              // 0x03
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_R0_CONT_EN       	0x01
        #define DMA_R0_CONT_LOFFS_EN 	0x02
        #define DMA_R1_CONT_EN      	0x04
        #define DMA_R1_CONT_LOFFS_EN	0x08
        /*------------------------------------------------------------*/
    AIT_REG_B   DMA_DESCP_EN;                               // 0x04
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_M0_DESCP_EN      	0x01
        #define DMA_M1_DESCP_EN         0x02
        #define DMA_R0_DESCP_EN         0x04
        #define DMA_R1_DESCP_EN         0x08
        /*------------------------------------------------------------*/
    AIT_REG_B   DMA_CHK_DESCP_EN;                           // 0x05
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_M0_CHK_DESCP_EN     0x01
        #define DMA_M1_CHK_DESCP_EN     0x02
        #define DMA_R0_CHK_DESCP_EN     0x04
        #define DMA_R1_CHK_DESCP_EN     0x08
        /*------------------------------------------------------------*/
    AIT_REG_B                           _x06[0x2];
    AIT_REG_B   DMA_M0_SRAM_DEL_SEL;                        // 0x08
    AIT_REG_B   DMA_M1_SRAM_DEL_SEL;                        // 0x09
    AIT_REG_W   DMA_R0_SRAM_DEL_SEL;                        // 0x0A
    AIT_REG_W   DMA_R1_SRAM_DEL_SEL;                        // 0x0C
    AIT_REG_B                           _x0E[0x2];
    
    AITS_DMA_M          DMA_M0;                             // 0x10~0x1F
    AITS_DMA_M_LOFFS    DMA_M0_LOFFS;                       // 0x20~0x2F
    AITS_DMA_R          DMA_R0;                             // 0x30~0x5F
    AITS_DMA_M          DMA_M1;                             // 0x60~0x6F
    AITS_DMA_M_LOFFS    DMA_M1_LOFFS;                       // 0x70~0x7F
    
    AIT_REG_D   DMA_INT_CPU_EN;                             // 0x80
    AIT_REG_D   DMA_INT_CPU_SR;                             // 0x84
    AIT_REG_D   DMA_INT_HOST_EN;                            // 0x88
    AIT_REG_D   DMA_INT_HOST_SR;                            // 0x8C
        /*-DEFINE-----------------------------------------------------*/
        #define DMA_INT_M0_DONE  		0x000001
        #define DMA_INT_M1_DONE      	0x000002
        #define DMA_INT_R0_DONE      	0x000004
        #define DMA_INT_R1_DONE       	0x000008
        #define DMA_INT_M0_DESCP_DONE   0x000100
        #define DMA_INT_M0_DESCP_ERR    0x000200
        #define DMA_INT_M1_DESCP_DONE   0x000400
        #define DMA_INT_M1_DESCP_ERR    0x000800
        #define DMA_INT_R0_DESCP_DONE   0x001000
        #define DMA_INT_R0_DESCP_ERR    0x002000
        #define DMA_INT_R1_DESCP_DONE   0x004000
        #define DMA_INT_R1_DESCP_ERR    0x008000
        #define DMA_INT_M0_DESCP_EQ_TH  0x010000
        #define DMA_INT_M1_DESCP_EQ_TH  0x020000
        #define DMA_INT_R0_DESCP_EQ_TH 	0x040000
        #define DMA_INT_R1_DESCP_EQ_TH  0x080000
        #define DMA_INT_MASK            0x0FFF0F
        /*------------------------------------------------------------*/

    AITS_DMA_DESCP      DMA_M0_DESCP;                       // 0x90~0x9F
    AITS_DMA_DESCP      DMA_M1_DESCP;                       // 0xA0~0xAF
    AITS_DMA_DESCP      DMA_R0_DESCP;                       // 0xB0~0xBF
    AITS_DMA_DESCP      DMA_R1_DESCP;                       // 0xC0~0xCF
    AITS_DMA_R          DMA_R1;                             // 0xD0~0xFF
} AITS_DMA, *AITPS_DMA;
#endif

/// @}

#endif // _MMP_REG_DMA_H_
