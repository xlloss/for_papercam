/**
 @file aac_encoder.c
 @brief AAC Encoder Function
 @author Alterman
 @version 1.0
*/

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "aac_encoder.h"

/** @addtogroup AAC_ENCODER
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
static AAC_ENCODER_CLASS    m_AACEncoder[AACENC_NUM];

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

static int  AACEnc_Op(MMP_AUD_ENCODER *enc, MMP_AENC_OP op);
static int  AACEnc_SetPropt(MMP_AUD_ENCODER *enc, MMP_AENC_PROPTY p, void *value);
static int  AACEnc_GetPropt(MMP_AUD_ENCODER *enc, MMP_AENC_PROPTY p, void *value);
static int  AACEnc_Encode(MMP_AUD_ENCODER *enc);
static void AACEnc_AdtsHeader(MMP_AUD_ENCODER *enc, MMP_UBYTE *hdr_buf);

static int  PutBit(BitStream *bitStream, unsigned long data, int numBit);
static int  WriteByte(BitStream *bitStream, unsigned long data, int numBit);
static int  GetSRIndex(unsigned int sampleRate);
static BitStream *OpenBitStream(int size, unsigned char *buffer);

//==============================================================================
//
//                              MACRO FUNCTIONS
//
//==============================================================================

#define AAC_ENCODER(id)     (&m_AACEncoder[id])
#define AAC_ENCODER_ID(enc) (enc - &m_AACEncoder[0])

#ifndef min
#define min(a, b)           ((a < b) ? (a) : (b))
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

#if 0
void ____AACEnc_Instance_Function____(){ruturn;}
#endif

//------------------------------------------------------------------------------
//  Function    : AACEnc_New
//  Description :
//------------------------------------------------------------------------------
/**
 @brief New an AAC encoder instance.

 @param[in] enc     Encoder object to be instanced

 @retval 0 for Success, others are the error code.
*/
int AACEnc_New(MMP_AUD_ENCODER *enc)
{
    int i;

    for(i = 0; i < AACENC_NUM; i++) {
        if (m_AACEncoder[i].used == MMP_FALSE)
            break;
    }
    if (i == AACENC_NUM)
        return E_AACENC_RESOUCE;

    m_AACEncoder[i].used = MMP_TRUE;

    enc->id         = i;
    enc->state      = AENC_STATE_OFF;
    enc->enc_op     = AACEnc_Op;
    enc->set_propt  = AACEnc_SetPropt;
    enc->get_propt  = AACEnc_GetPropt;
    enc->encode     = AACEnc_Encode;

    return 0;
}

#if 0
void ____AACEnc_Operation____(){ruturn;}
#endif

//------------------------------------------------------------------------------
//  Function    : AACEnc_Open
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Start the AAC encoder.

 @param[in] enc     Encoder instance

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_Open(MMP_AUD_ENCODER *enc)
{
    int ret = 0;
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);

    /* Assign working buffer */
    ret = AACEnc_SetWorkingBuf(enc->id, (int *)encoder->workbuf);
    //printc("[AAC] work sz:%d,srate : %d Hz \r\n", ret,encoder->info.sampleRate);
    if (ret > encoder->workbufsz) {
      RTNA_DBG_Str(0,"Error set aacenc working buffer\r\n");
      return E_AACENC_NOBUF;
    }

    /* Reset encoder information */
    encoder->ts = 0;
    encoder->enc_frms = 0;

    encoder->info.bitsPerSample     = 16;
    encoder->info.nChannels         = 2;
    encoder->info.nSamples          = 0;
    encoder->info.isLittleEndian    = 1;

    encoder->info.fpScaleFactor     = 1;   /* must be set */
    encoder->info.valid             = 1;   /* must be set */
    encoder->info.useWaveExt        = 0;

    /* Output bitstream with 7-Byte ADTS header */
    encoder->adts_hdr = MMP_TRUE;

    /* make reasonable default settings */
    AACEnc_AacInitDefaultConfig(enc->id, &encoder->cfg);

    /* open encoder */
    encoder->cfg.nChannelsIn  = encoder->info.nChannels;
    encoder->cfg.nChannelsOut = encoder->info.nChannels;
    encoder->cfg.bandWidth    = (int)((encoder->info.sampleRate >> 1) * 4/5);
    encoder->cfg.sampleRate   = encoder->info.sampleRate;
    ret = AACEnc_AacEncOpen(enc->id, &encoder->cfg);
    if (ret) {
        RTNA_DBG_Str0("AacEncOpen failed\r\n");
        return E_AACENC_OPEN;
    }

    enc->state = AENC_STATE_ON;
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_Close
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Stop the AAC encoder.

 @param[in] enc     Encoder instance

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_Close(MMP_AUD_ENCODER *enc)
{
    enc->state = AENC_STATE_OFF;
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_Release
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Release an AAC encoder instance.

 @param[in] enc     Encoder instance

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_Release(MMP_AUD_ENCODER *enc)
{
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);

    enc->state = AENC_STATE_INVALID;
    encoder->used = MMP_FALSE;
    return 0;
}

#if 0
void ____AACEnc_Interface____(){ruturn;}
#endif

//------------------------------------------------------------------------------
//  Function    : AACEnc_Op
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Execute the encoder opeation, including reset/open/close/release.

 @param[in] enc     Encoder instance
 @param[in] op      Operation (reset/open/close/release)

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_Op(MMP_AUD_ENCODER *enc, MMP_AENC_OP op)
{
    int err = 0;

    switch(op) {
    case AENC_OP_OPEN:
        err = AACEnc_Open(enc);
        break;
    case AENC_OP_CLOSE:
        err = AACEnc_Close(enc);
        break;
    case AENC_OP_RELEASE:
        err = AACEnc_Release(enc);
        break;
    default:
        RTNA_DBG_Str0("Unsupported enc op:");
        RTNA_DBG_Byte0(op);
        RTNA_DBG_Str0("\r\n");
        break;
    }
    return err;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_SetPropt
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Set encoder properties. Allow to set propertyies before encoder opened
        only.

 @param[in] enc     Encoder instance
 @param[in] p       Property type
 @param[in] value   Pointer to the property value

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_SetPropt(MMP_AUD_ENCODER *enc, MMP_AENC_PROPTY p, void *value)
{
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);

    switch(p) {
    case AENC_PROPT_FS:
        encoder->info.sampleRate = *(MMP_ULONG *)value;
        #if SRC_SUPPORT
        if(encoder->info.sampleRate == 8000) {
            encoder->info.sampleRate = 16000 ;
        }
        #endif
        break;
    case AENC_PROPT_BITRATE:
        encoder->cfg.bitRate = *(MMP_ULONG *)value;
        break;
    case AENC_PROPT_WORKBUF_ADDR:
        encoder->workbuf = *(MMP_ULONG *)value;
        break;
    case AENC_PROPT_WORKBUF_SIZE:
        encoder->workbufsz = *(MMP_ULONG *)value;
        break;
    case AENC_PROPT_INPUT:
        encoder->input = (MMP_SHORT *)value;
        break;
    case AENC_PROPT_OUTPUT:
        encoder->frm = (MMP_UBYTE *)value;
        break;
    default:
        RTNA_DBG_Str0("Unsupported propt to set:");
        RTNA_DBG_Byte0(p);
        RTNA_DBG_Str0("\r\n");
        return E_AACENC_PROPT;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_GetPropt
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get encoder properties.

 @param[in] enc     Encoder instance
 @param[in] p       Property type
 @param[out] value  Pointer to hold the returned value

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_GetPropt(MMP_AUD_ENCODER *enc, MMP_AENC_PROPTY p, void *value)
{
    MMP_ULONG *argu = (MMP_ULONG *)value;
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);


    switch(p) {
    case AENC_PROPT_FS:
        *argu = encoder->info.sampleRate;
        break;
    case AENC_PROPT_BITRATE:
        *argu = encoder->cfg.bitRate;
        break;
    case AENC_PROPT_INCH_NUM:
        *argu = encoder->cfg.nChannelsIn;
        break;
    case AENC_PROPT_OUTCH_NUM:
        *argu = encoder->cfg.nChannelsOut;
        break;
    case AENC_PROPT_FRAME_SAMPLES:
        *argu = AACENC_INFRM_SAMPLES;
        break;
    case AENC_PROPT_FRAME_SIZE:
        *argu = encoder->frmsize;
        break;
    case AENC_PROPT_FRAME_TS:
    #if (USE_DIV_CONST)
    if (encoder->cfg.sampleRate == 32000) 
    {
        *argu = encoder->ts +
                (MMP_ULONG)(((MMP_ULONG64)encoder->enc_frms *
                             (AACENC_INFRM_SAMPLES >> 1)) / 32);
    }
    if (encoder->cfg.sampleRate == 16000) 
    {
        *argu = encoder->ts +
                (MMP_ULONG)(((MMP_ULONG64)encoder->enc_frms *
                             (AACENC_INFRM_SAMPLES >> 1)) / 16);
    }
    #else
    *argu = encoder->ts +
            (MMP_ULONG)(((MMP_ULONG64)encoder->enc_frms *
                         (AACENC_INFRM_SAMPLES >> 1)) /
                        (encoder->cfg.sampleRate / 1000));
    #endif



        break;
    default:
        RTNA_DBG_Str0("Unsupported propt to get:");
        RTNA_DBG_Byte0(p);
        RTNA_DBG_Str0("\r\n");
        return E_AACENC_PROPT;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_Encode
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Encode one audio frame.

 @param[in] enc     Encoder instance
 @param[in] in      Address of input RAW data
 @param[in] out     Address to output encoded data

 @retval 0 for Success, others are the error code.
*/
static int AACEnc_Encode(MMP_AUD_ENCODER *enc)
{
    UWord8 *frm_buf;
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);

    /* Encode starting timestamp */
    if (encoder->ts == 0)
        encoder->ts = OSTime;

    if (encoder->adts_hdr)
        frm_buf = encoder->frm + AACENC_ATDS_HDR_SIZE;
    else
        frm_buf = encoder->frm;

    /* Encode one frame */
    AACEnc_AacEncEncode(enc->id,
                        (Word16 *)encoder->input,
                        (const UWord8 *)encoder->ancData,
                        (Word16 *)&encoder->ancDataSize,
                        frm_buf,
                        (Word32 *)&encoder->frmsize);

    /* Generate ADTS header */
    if (encoder->adts_hdr) {
        AACEnc_AdtsHeader(enc, encoder->frm);
        encoder->frmsize += AACENC_ATDS_HDR_SIZE;
    }

    encoder->enc_frms++;

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : AACEnc_AdtsHeader
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Generate the ADTS header.

 @param[in] enc     Encoder instance
 @param[in] hdr_buf Buffer to keep ADTS header

 @retval 0 for Success, others are the error code.
*/
static void AACEnc_AdtsHeader(MMP_AUD_ENCODER *enc, MMP_UBYTE *hdr_buf)
{
    int sampleRateIdx;
    BitStream *bitStream;
    AAC_ENCODER_CLASS *encoder = AAC_ENCODER(enc->id);

    bitStream = OpenBitStream(AACENC_ATDS_HDR_SIZE, hdr_buf);

    PutBit(bitStream, 0xFFFF, 12); /* 12 bit Syncword */
    PutBit(bitStream, 1, 1); /* ID == 0 for MPEG4 AAC, 1 for MPEG2 AAC */
    PutBit(bitStream, 0, 2); /* layer == 0 */
    PutBit(bitStream, 1, 1); /* protection absent */
    PutBit(bitStream, 2 - 1, 2); /* profile */
    sampleRateIdx = GetSRIndex(encoder->info.sampleRate);
    PutBit(bitStream, sampleRateIdx, 4); /* sampling rate */
    PutBit(bitStream, 0, 1); /* private bit */
    PutBit(bitStream, encoder->info.nChannels, 3); /* ch. config (must be > 0) */
    PutBit(bitStream, 0, 1); /* original/copy */
    PutBit(bitStream, 0, 1); /* home */

    /* Variable ADTS header */
    PutBit(bitStream, 0, 1); /* copyr. id. bit */
    PutBit(bitStream, 0, 1); /* copyr. id. start */
    PutBit(bitStream, encoder->frmsize + AACENC_ATDS_HDR_SIZE, 13);
    PutBit(bitStream, 0x7FF, 11); /* buffer fullness (0x7FF for VBR) */
    PutBit(bitStream, 0, 2); /* raw data blocks (0+1=1) */
}

//------------------------------------------------------------------------------
//  Function    : OpenBitStream
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Create a bitstream buffer handler

 @param[in] size    Size of bitstream buffer in byte
 @param[in] buffer  Bitstream buffer

 @retval Bitstream buffer handler
*/
static BitStream *OpenBitStream(int size, unsigned char *buffer)
{
    static BitStream BitBuf;
    BitStream *bitStream;

    bitStream = (BitStream *)&BitBuf.numBit;
    bitStream->size = size;

    bitStream->numBit = 0;
    bitStream->currentBit = 0;

    bitStream->data = buffer;

    return bitStream;
}

//------------------------------------------------------------------------------
//  Function    : PutBit
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Put bits of data into the bitstream buffer

 @param[in] bitStream   Bitstream buffer handler
 @param[in] data        Data to write
 @param[in] numBit      Number of bits to write

 @retval 0 for Success, others are the error code.
*/
static int PutBit(BitStream *bitStream, unsigned long data, int numBit)
{
    int num,maxNum,curNum;
    unsigned long bits;

    if (numBit == 0)
        return 0;

    /* write bits in packets according to buffer byte boundaries */
    num = 0;
    maxNum = 8 - bitStream->currentBit % 8;
    while (num < numBit) {
        curNum = min(numBit-num,maxNum);
        bits = data>>(numBit-num-curNum);
        if (WriteByte(bitStream, bits, curNum)) {
            return 1;
        }
        num += curNum;
        maxNum = 8;
    }

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : WriteByte
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Put bytes of data into the bitstream buffer

 @param[in] bitStream   Bitstream buffer handler
 @param[in] data        Data to write
 @param[in] numBit      Number of bits to write

 @retval 0.
*/
static int WriteByte(BitStream *bitStream, unsigned long data, int numBit)
{
    long numUsed,idx;
#if USE_DIV_CONST
    idx = (bitStream->currentBit >> 3) % bitStream->size;
    numUsed = bitStream->currentBit & 0x7;
    printc
#else
    idx = (bitStream->currentBit / 8) % bitStream->size;
    numUsed = bitStream->currentBit % 8;
#endif

    if (numUsed == 0)
        bitStream->data[idx] = 0;

    bitStream->data[idx] |= (data & ((1<<numBit)-1)) << (8-numUsed-numBit);
    bitStream->currentBit += numBit;
    bitStream->numBit = bitStream->currentBit;

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : GetSRIndex
//  Description :
//------------------------------------------------------------------------------
/**
 @brief Get sample rate index

 @param[in] sampleRate  AAC sample rate

 @retval Index of sample rate.
*/
static int GetSRIndex(unsigned int sampleRate)
{
    if (92017 <= sampleRate) return 0;
    if (75132 <= sampleRate) return 1;
    if (55426 <= sampleRate) return 2;
    if (46009 <= sampleRate) return 3;
    if (37566 <= sampleRate) return 4;
    if (27713 <= sampleRate) return 5;
    if (23004 <= sampleRate) return 6;
    if (18783 <= sampleRate) return 7;
    if (13856 <= sampleRate) return 8;
    if (11502 <= sampleRate) return 9;
    if (9391 <= sampleRate) return 10;

    return 11;
}

/// @}
