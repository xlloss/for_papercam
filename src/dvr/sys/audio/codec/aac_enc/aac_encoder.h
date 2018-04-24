/**
 @file aac_encoder.h
 @brief Header File for AAC Encoder
 @author Alterman
 @version 1.0
*/

#ifndef _AAC_ENCODER_H_
#define _AAC_ENCODER_H_

//==============================================================================
//
//                               INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "mmp_aud_inc.h"

#include "aacenc_include.h"
#include "aacenc.h"

/** @addtogroup AAC_ENCODER
@{
*/

//==============================================================================
//
//                               CONSTANTS
//
//==============================================================================

#define AACENC_NUM              (2)         // Number of AAC encoder

#define AACENC_WORKBUF_SIZE     (0x8000)    // 32K bytes
#define AACENC_INBUF_SLOT       (15)        // Input buf can keep 15 frames
#define AACENC_INFRM_SAMPLES    (2048)      // Frame: 1024 samples per channel
#define AACENC_MAX_FRM_SIZE     (1536)      // 6144 / 8 * 2ch, max size of frame

#define AACENC_MAX_PL_SIZE      (128)       // Max. payload size
#define AACENC_ATDS_HDR_SIZE    (7)         // ADTS header 7-byte

/*
 * AAC Encoder Error Codes
 */
#define E_AACENC_RESOUCE        (-1)    ///< No encoder resource
#define E_AACENC_NOBUF          (-2)    ///< No enough buffer
#define E_AACENC_OPEN           (-3)    ///< Open encoder failed
#define E_AACENC_PROPT          (-4)    ///< Bad property
#define E_AACENC_BUFOV          (-5)    ///< Buffer overflowed
#define E_AACENC_BUFUD          (-6)    ///< Buffer underflowed

//==============================================================================
//
//                               ENUMERATION
//
//==============================================================================


//==============================================================================
//
//                               STRUCTURES
//
//==============================================================================
/*
 * AAC Encoder Class
 */
typedef struct {
    MMP_BOOL        used;       ///< Is this encoder used?
    MMP_ULONG       workbuf;    ///< Working buffer address
    MMP_ULONG       workbufsz;  ///< Working buffer size
    MMP_SHORT       *input;     ///< Input buffer address
    MMP_UBYTE       *frm;       ///< Output encoded frame address
    MMP_ULONG       frmsize;    ///< Frame size (byte)
    /* Encoder related members */
    AACENC_CONFIG   cfg;
    AuChanInfo      info;
    MMP_UBYTE       ancData[AACENC_MAX_PL_SIZE];
    MMP_SHORT       ancDataSize;
    MMP_BOOL        adts_hdr;   ///< With ADTS header?
    MMP_ULONG       ts;         ///< Timestamp
    MMP_ULONG       enc_frms;   ///< Total encoded frames
} AAC_ENCODER_CLASS;

/*
 * Bitstream Buffer Handler
 */
typedef struct {
    long numBit;          /* number of bits in buffer */
    long size;            /* buffer size in bytes */
    long currentBit;      /* current bit position in bit stream */
    long numByte;         /* number of bytes read/written (only file) */
    unsigned char *data;  /* data bits */
} BitStream;

//==============================================================================
//
//                               FUNCTION PROTOTYPES
//
//==============================================================================

extern int AACEnc_New(MMP_AUD_ENCODER *enc);

/// @}

#endif //_AAC_ENCODER_H_

