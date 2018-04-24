#include "config_fw.h"
#include "hdm_ctl.h"
#include "text_ctl.h"
#include "mmpf_sensor.h"

#if (USER_STRING)
extern void small_sprintf(char* s,char *fmt, ...);
#define sprintf small_sprintf
#endif

MMP_ULONG	AEAcc_Buffer[256];
MMP_ULONG	AWBAcc_Buffer[256* 5];
MMP_ULONG	AFAcc_Buffer[60];
MMP_ULONG	HistAcc_Buffer[256];
MMP_ULONG   FlickerAcc_Buffer[256];

extern	MMP_ULONG	m_glISPBufferStartAddr;
extern const ISP_UINT8 Sensor_IQ_CompressedText[];
#if (SUPPORT_UVC_ISP_EZMODE_FUNC==1)
extern const ISP_UINT8 Sensor_EZ_IQ_CompressedText[];     
extern ISP_UINT32 eziqsize;
#endif
 ISP_UINT8 DstAddr[64*1024] __attribute((aligned(4)));
 ISP_UINT8 TmpBufAddr[1024]  __attribute((aligned(4)));

void ISP_HDM_IF_CALI_GetData(void)
{
}

ISP_UINT8 ISP_HDM_IF_IQ_IsApicalClkOff(void)
{
	return 1;
}

ISP_UINT32 ISP_HDM_IF_LIB_GetBufAddr(ISP_BUFFER_CLASS buf_class, ISP_UINT32 buf_size, ISP_BUFFER_TYPE buf_type)
{
	ISP_UINT32 addr = 0;

	switch (buf_class) {
	case ISP_BUFFER_CLASS_IQ_BIN_SRC:
	  #if ISP_USE_RAW_IQ_DATA==1
	  addr = (ISP_UINT32)0;
	  #else
		addr = (ISP_UINT32)Sensor_IQ_CompressedText;
		#endif
		break;
	case ISP_BUFFER_CLASS_IQ_BIN_DST:
	  #if ISP_USE_RAW_IQ_DATA==1
	  addr = (ISP_UINT32)Sensor_IQ_CompressedText;
	  #else
		addr = (ISP_UINT32)DstAddr;
		#endif
		break;
	case ISP_BUFFER_CLASS_IQ_BIN_TMP:
		addr = (ISP_UINT32)TmpBufAddr;
		break;
	case ISP_BUFFER_CLASS_MEMOPR_IQ:
		addr = IQ_BANK_ADDR;
		break;
	case ISP_BUFFER_CLASS_MEMOPR_LS:
		addr = LS_BANK_ADDR;
		break;
	case ISP_BUFFER_CLASS_MEMOPR_CS:
		addr = CS_BANK_ADDR;
		break;
	case ISP_BUFFER_CLASS_AE_HW:
		addr = (m_glISPBufferStartAddr + ISP_AWB_BUF_SIZE);
		break;
	case ISP_BUFFER_CLASS_AE_SW:
		addr = (ISP_UINT32)AEAcc_Buffer;
		break;
	case ISP_BUFFER_CLASS_AF_HW:
		addr = (m_glISPBufferStartAddr + ISP_AWB_BUF_SIZE + ISP_AE_BUF_SIZE);
		break;
	case ISP_BUFFER_CLASS_AF_SW:
		addr = (ISP_UINT32)AFAcc_Buffer;
		break;
	case ISP_BUFFER_CLASS_AWB_HW:
		addr = m_glISPBufferStartAddr;
		break;
	case ISP_BUFFER_CLASS_AWB_SW:
		addr = (ISP_UINT32)AWBAcc_Buffer;
		break;
	case ISP_BUFFER_CLASS_FLICKER_HW:
		addr = (m_glISPBufferStartAddr + ISP_AWB_BUF_SIZE + ISP_AE_BUF_SIZE + ISP_AF_BUF_SIZE + ISP_DFT_BUF_SIZE + ISP_HIST_BUF_SIZE);
		break;
	case ISP_BUFFER_CLASS_FLICKER_SW:
		addr = (ISP_UINT32)FlickerAcc_Buffer;
		break;
	case ISP_BUFFER_CLASS_HIST_HW:
		addr = (m_glISPBufferStartAddr + ISP_AWB_BUF_SIZE + ISP_AE_BUF_SIZE + ISP_AF_BUF_SIZE + ISP_DFT_BUF_SIZE);
		break;
	case ISP_BUFFER_CLASS_HIST_SW:
		addr = (ISP_UINT32)HistAcc_Buffer;
		break;
  case ISP_BUFFER_CLASS_IQ_BIN_EZ:
#if SUPPORT_UVC_ISP_EZMODE_FUNC==1
      addr = (ISP_UINT32)Sensor_EZ_IQ_CompressedText;
      ISP_IF_IQ_SetEZIQSize(eziqsize);
#else
      ISP_IF_IQ_SetEZIQSize( 0 );
#endif
      break;        
      
	default:
		break;
	}

	return addr;
}

#if DRAW_TEXT_FEATURE_EN
ISP_INT8 MultiShading;
ISP_INT16 CCM_PRT[9];

void VR_PrintStringOnPreview(void)
{
#if (ISP_EN)
	if(gDrawTextEn){
		
	#if 0
		_sprintf(gDrawTextBuf, (ISP_INT8*)"AE  : Shutter=%x (%d)FPS, Gain=%x, ShutterBase = %x", (ISP_UINT32)ISP_IF_AE_GetShutter(), (ISP_UINT32)ISP_IF_AE_GetRealFPS(), (ISP_UINT32)ISP_IF_AE_GetGain(), ISP_IF_AE_GetShutterBase());
		VR_PrintString(gDrawTextBuf,  10, 0, 0x0000, 0x0000);
		_sprintf(gDrawTextBuf, (ISP_INT8*)"EV:%x, AE  : AvgLum=%x %x %x",(ISP_UINT32)ISP_IF_AE_GetDbgData(0), (ISP_UINT32)ISP_IF_AE_GetDbgData(1), (ISP_UINT32)ISP_IF_AE_GetDbgData(2), (ISP_UINT32)ISP_IF_AE_GetDbgData(3));
		VR_PrintString(gDrawTextBuf,  20, 0, 0x0000, 0x0000);
		_sprintf(gDrawTextBuf, (ISP_INT8*)"DBG8:%x, %x, %x, %x, %x",(ISP_UINT32)ISP_IF_IQ_GetID(ISP_IQ_CHECK_CLASS_COLORTEMP),  ISP_IF_IQ_GetID(ISP_IQ_CHECK_CLASS_GAIN),  ISP_IF_IQ_GetID(ISP_IQ_CHECK_CLASS_ENERGY),ISP_IF_AE_GetMetering(),(ISP_UINT32)ISP_IF_AE_GetLightCond());
		VR_PrintString(gDrawTextBuf,  80, 0, 0x0000, 0x0000);
		_sprintf(gDrawTextBuf, (ISP_INT8*)"Avglum:%x, EVTarget%x",(ISP_UINT32)ISP_IF_AE_GetCurrentLum(),  ISP_IF_AE_GetTargetLum());
		VR_PrintString(gDrawTextBuf,  90, 0, 0x0000, 0x0000);
	#endif

	#if 0
		_sprintf(gDrawTextBuf, (ISP_INT8*)"AWB : Mode=%x, GainR=%x, GainGr=%x, GainB=%x, CT = %x,%x", (ISP_UINT32)ISP_IF_AWB_GetMode(), (ISP_UINT32)ISP_IF_AWB_GetGainR(), (ISP_UINT32)ISP_IF_AWB_GetGainG(), (ISP_UINT32)ISP_IF_AWB_GetGainB(),(ISP_UINT32)ISP_IF_AWB_GetColorTemp(),(ISP_UINT32)ISP_IF_AWB_GetMode());
		VR_PrintString(gDrawTextBuf,  30, 0, 0x0000, 0x0000);
	#endif

	#if 1
		{
			ISP_UINT32 *dbgPtr = (ISP_UINT32 *)ISP_IF_AF_GetDbgDataPtr();
			/* _sprintf(gDrawTextBuf, (ISP_INT8*)"AF  : AFPos=%x, dbg=%x %x %x", (ISP_UINT32)ISP_IF_AF_GetPos(10), (ISP_UINT32)ISP_IF_AF_GetPos(10), dbgPtr[1], dbgPtr[2]); */
			sprintf(gDrawTextBuf, (ISP_INT8*)"AF  : AFPos=%x, dbg=%x %x %x", (ISP_UINT32)ISP_IF_AF_GetPos(10), (ISP_UINT32)ISP_IF_AF_GetPos(10), dbgPtr[1], dbgPtr[2]);
			VR_PrintString(gDrawTextBuf,  40, 0, 0xFC00, 0x0000);
		}
	#endif
	 }
#endif
}
#endif

ISP_UINT32 ISP_HDM_IF_LIB_RamAddrV2P(ISP_UINT32 addr)
{
    return addr;
}
ISP_UINT32 ISP_HDM_IF_LIB_RamAddrP2V(ISP_UINT32 addr)
{
    return addr;  
}
ISP_UINT32 ISP_HDM_IF_LIB_OprAddrV2P(ISP_UINT32 addr)
{
    return addr;
}
ISP_UINT32 ISP_HDM_IF_LIB_OprAddrP2V(ISP_UINT32 addr)
{
    return addr;
}
