//==============================================================================
//
//  File        : mmpf_dsc.h
//  Description : INCLUDE File for the DSC Driver Function
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_DSC_H_
#define _MMPF_DSC_H_

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "mmp_dsc_inc.h"
#include "mmpf_mp4venc.h"

/** @addtogroup MMPF_DSC
@{
*/

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define DSC_SINGLE_LINE_BUF_TH	    (1024)

#define JPEG_SCALE_FACTOR_HQ	    (1)
#define JPEG_SCALE_FACTOR_NQ      	(16)
#define JPEG_SCALE_FACTOR_LQ	    (64 * 4)
#ifdef PROJECT_LD
#define JPEG_SCALE_FACTOR_CUR	    (JPEG_SCALE_FACTOR_LQ)
#else
#define JPEG_SCALE_FACTOR_CUR	    (JPEG_SCALE_FACTOR_NQ)
#endif
#define JPEG_SCALE_FACTOR_MIN	    (1)
#define JPEG_SCALE_FACTOR_MAX       (0x3FFF)

// In normal case, polling time is 34ms, set x10 for time out
#define DSC_TIMEOUT_MS			    (340)

#define DSC_CAPTURE_DONE_SEM_TIMEOUT    (5000)

//==============================================================================
//
//                              ENUMERATION
//
//==============================================================================

typedef enum _MMPF_DSC_IMAGE_SOURCE
{
    MMPF_DSC_SRC_SENSOR   			= 0x01,	///< Image source is "sensor(Bayer) -> ISP -> JPEG" or "sensor(YUV) -> JPEG"
    MMPF_DSC_SRC_RAWPROC     		= 0x02,	///< Image source is "sensor -> rawproc -> ISP -> JPEG"
    MMPF_DSC_SRC_GRAPHICS   		= 0x04,	///< Image source is "graphics -> JPEG"
    MMPF_DSC_SRC_GRAPHICS_YUV420	= 0x08,	///< Image source is "graphics(yuv420) -> JPEG"
    MMPF_DSC_SRC_GRAPHICS_NV12    	= 0x10, ///< Image source is "graphics(NV12) -> JPEG"
    MMPF_DSC_SRC_H264REF            = 0x20, ///< Image source is "H.264 reference frame -> graphics(NV12) -> JPEG"
    MMPF_DSC_SRC_LDC_RDY_FRM		= 0x40  ///< Image source is "graphics(NV12) -> JPEG" but not trigger GRA.
} MMPF_DSC_IMAGE_SOURCE;

typedef enum _MMPF_DSC_SIZE_CTRL_MODE 
{
    MMPF_DSC_SIZE_CTL_DISABLE = 0,      	///< Disable size control function
    MMPF_DSC_SIZE_CTL_NORMAL           		///< Enable size control function
} MMPF_DSC_SIZE_CTRL_MODE;

typedef enum _MMPF_JPEG_ENCDONE_MODE
{
    MMPF_DSC_ENCDONE_SIZE_CTL = 0,			///< JPEG encode done then do size control
    MMPF_DSC_ENCDONE_UVC_MJPG,				///< JPEG encode done then transfer by UVC
    MMPF_DSC_ENCDONE_CONTINUOUS_SHOT,		///< JPEG encode done then trigger continuous shot
    MMPF_DSC_ENCDONE_WIFI_MJPG,				///< JPEG encode done then transfer by Wifi
    MMPF_DSC_ENCDONE_VID_TO_MJPG,			///< Video frame JPEG encode done then transfer by Wifi
    MMPF_DSC_ENCDONE_NONE = 0xFF
} MMPF_JPEG_ENCDONE_MODE;

typedef enum _MMPF_DSC_CAPTUREMODE
{
    MMPF_DSC_SHOTMODE_SINGLE = 0,			///< Still shot mode
    MMPF_DSC_SHOTMODE_MULTI,                ///< Multi-shot mode
    MMPF_DSC_SHOTMODE_CONTINUOUS,         	///< Continuous-shot mode
    MMPF_DSC_SHOTMODE_MJPEG,        		///< MJPEG mode for wifi streaming
	MMPF_DSC_SHOTMODE_VID2MJPEG,       		///< Video to MJPEG mode for wifi streaming
	MMPF_DSC_SHOTMODE_NUM
} MMPF_DSC_CAPTUREMODE;

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================


//==============================================================================
//
//                              DATA TYPES
//
//==============================================================================

typedef MMP_BOOL (*PFuncRegCheck)(void);

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

/* Internal Function */
MMP_ERR MMPF_JPG_ResetEncDoneMode(void);
MMP_ERR MMPF_JPG_SetEncDoneMode(MMPF_JPEG_ENCDONE_MODE jpegmode);
MMPF_JPEG_ENCDONE_MODE MMPF_JPG_GetEncDoneMode(void);
MMP_ERR MMPF_JPG_TriggerEncode(void);

/* Rate Control Function */
MMP_ERR MMPF_DSC_SetQFactor(MMP_USHORT qctl1, MMP_USHORT qctl2);
MMP_ERR MMPF_DSC_SetQTableLow(void);
MMP_ERR MMPF_DSC_SetQTableIntoOpr(MMP_UBYTE ubIdx);
MMP_ERR MMPF_DSC_SetQTableInfo(MMP_UBYTE ubIdx, MMP_UBYTE *ubQtable, MMP_UBYTE *ubUQtable, MMP_UBYTE *ubVQtable, MMP_DSC_JPEG_QTABLE QTSet);
MMP_ERR	MMPF_DSC_SetJpegQualityCtl( MMP_UBYTE   ubRcIdx,
                                    MMP_BOOL	bTargetCtlEnable,
                                    MMP_BOOL 	bLimitCtlEnable,
			                        MMP_ULONG 	ulTargetSize, 
			                        MMP_ULONG 	ulLimitSize,
			                        MMP_USHORT 	usMaxCount);
/* Encode Function */
MMP_ERR MMPF_DSC_RegisterEncIntrCallBack(MMP_DSC_ENC_EVENT event, JpegEncCallBackFunc *pCallBack, void *pArgument);
MMP_ERR MMPF_DSC_SetCapturePath(MMP_USHORT	usScaPipe,
                                MMP_USHORT	usIcoPipe,
                                MMP_USHORT	usIbcPipe);
MMP_ERR MMPF_DSC_SetCaptureBuffers(MMP_DSC_CAPTURE_BUF *pBuf);
MMP_ERR MMPF_DSC_SetLineBufType(void);
MMP_ERR MMPF_DSC_ChangeCompBufAddr(MMP_ULONG ulExtCompStartAddr, MMP_ULONG ulExtCompEndAddr);
MMP_ERR	MMPF_DSC_EncodeJpeg(MMP_UBYTE ubSnrSel, MMPF_DSC_IMAGE_SOURCE SourcePath, MMPF_DSC_CAPTUREMODE shot_mode);
MMP_ERR MMPF_DSC_SetCardModeExifEnc(MMP_USHORT  usExifNodeId,
									MMP_BOOL 	bEncodeExif, 
									MMP_BOOL 	bEncodeThumbnail, 
									MMP_USHORT 	usThumbnailwidth, 
									MMP_USHORT 	usThumbnailheight);
MMP_ERR MMPF_DSC_RestorePreview(MMP_UBYTE ubSnrSel, MMP_USHORT usPreviewEn, MMP_USHORT usWindowsTime);
MMP_ERR MMPF_DSC_JpegDram2Card(MMP_ULONG ulStartAddr, MMP_ULONG ulWritesize, MMP_BOOL bFirstwrite, MMP_BOOL bLastwrite);
MMP_ERR MMPF_DSC_GetJpegSizeAfterEncodeDone(MMP_ULONG *ulJpegSize);
MMP_ERR MMPF_DSC_GetJpegSize(MMP_BOOL bStableEnable, MMP_ULONG *ulJpegSize);
MMP_ERR MMPF_DSC_GetJpegBitStream(MMP_USHORT *usJpegbufAddr, MMP_ULONG ulHostBufLimit);
MMP_ERR MMPF_DSC_GetJpeg(MMP_ULONG *ulJpegBuf, MMP_ULONG *ulSize, MMP_ULONG *ulTime);
MMP_ERR MMPF_DSC_GetEncJpegResol(MMP_USHORT *usWidth, MMP_USHORT *usHeight);
MMP_ERR MMPF_DSC_SetJpegResol(MMP_USHORT usJpegWidth, MMP_USHORT usJpegHeight, MMP_UBYTE ubRcIdx);

/* Decode Function */
MMP_ERR MMPF_DSC_RegisterDecIntrCallBack(MMP_DSC_DEC_EVENT event, JpegDecCallBackFunc *pCallBack, void *pArgument);
MMP_ERR MMPF_DSC_EnableDecInterrupt(MMP_DSC_DEC_EVENT event, MMP_BOOL bEnable);

MMP_ERR MMPF_DSC_GetJpegInfo(MMP_DSC_JPEG_INFO *pJpegInfo);
MMP_ERR MMPF_DSC_DecodeJpegInFile(void);
MMP_ERR MMPF_DSC_DecodeJpegInMem(MMP_ULONG ulAddr, MMP_ULONG ulSize, MMP_BOOL bSetBuf, MMP_BOOL bBlocking);
MMP_ERR	MMPF_DSC_SetDecodeInputBuf(MMP_ULONG ulDataTmpAddr, MMP_ULONG ulDataTmpSize,MMP_ULONG ulBSBufAddr, MMP_ULONG ulBSBufSize);
MMP_ERR MMPF_DSC_SetCardModeExifDec(MMP_BOOL bDecodeThumb, MMP_BOOL bDecodeLargeThumb);
MMP_ERR MMPF_DSC_ResetPreSeekPos(void);
MMP_ERR MMPF_DSC_SetJpgDecOffset(MMP_ULONG ulStartOffset,  MMP_ULONG ulEndOffset);
MMP_ERR MMPF_DSC_OpenJpegDecFile(void);
MMP_ERR MMPF_DSC_CloseJpegDecFile(void);
MMP_ERR MMPF_DSC_SetJpgDecOutPixelDelay(MMP_UBYTE ubPixDelay);
MMP_ERR MMPF_DSC_SetJpgDecOutLineDelay(MMP_UBYTE ubLineDelay);
MMP_ERR MMPF_DSC_SetJpgDecOutBusy(MMP_BOOL bEnable);
MMP_ERR	MMPF_DSC_SetDecodeCompBuf(MMP_ULONG ulStarAddr, MMP_ULONG ulEndAddr, MMP_ULONG ulLineAddr);

/* Exif Function */
MMP_ERR MMPF_DSC_ResetEXIFWorkingBufPtr(MMP_USHORT usExifNodeId, MMP_BOOL bForUpdate);
MMP_ERR MMPF_DSC_GetEXIFInfo(MMP_USHORT usExifNodeId, MMP_USHORT usIfd, MMP_USHORT usTag, MMP_ULONG ulDataAddr, MMP_ULONG *ulSize);
MMP_ERR MMPF_DSC_OpenEXIFFile(MMP_USHORT usExifNodeId);
MMP_ERR MMPF_DSC_CloseEXIFFile(MMP_USHORT usExifNodeId);
MMP_ERR MMPF_DSC_SetEXIFWorkingBuffer(MMP_USHORT usExifNodeId, MMP_UBYTE *pBuf, MMP_ULONG ulBufSize, MMP_BOOL bForUpdate);
MMP_ERR MMPF_DSC_UpdateExifInfo(MMP_USHORT	usExifNodeId,
                                MMP_USHORT 	usIfd, 		MMP_USHORT 	usTag, 		
                                MMP_USHORT 	usType, 	MMP_ULONG 	ulCount, 
                                MMP_UBYTE  	*pData, 	MMP_ULONG 	ulDataSize,
                                MMP_BOOL 	bForUpdate);
MMP_ERR MMPF_DSC_UpdateMpfInfo( MMP_USHORT 	usIfd, 		MMP_USHORT 	usTag, 		
                                MMP_USHORT 	usType, 	MMP_ULONG 	ulCount, 
                                MMP_UBYTE  	*pData, 	MMP_ULONG 	ulDataSize,
                                MMP_BOOL 	bForUpdate);
MMP_ERR MMPF_DSC_UpdateMpfEntry(MMP_USHORT  	usIfd,			MMP_USHORT  usEntryID,
							    MMP_ULONG 		ulImgAttr, 	 	MMP_ULONG   ulImgSize,  
							    MMP_ULONG 		ulImgOffset, 
							    MMP_USHORT		usImg1EntryNum, MMP_USHORT	usImg2EntryNum,
							    MMP_BOOL    	bForUpdate);

#endif // _MMPF_DSC_H_
