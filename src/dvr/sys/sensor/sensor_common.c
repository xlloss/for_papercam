//==============================================================================
//
//  File        : sensor_common.c
//  Description : Firmware Sensor Control File
//  Author      :
//  Revision    : 1.0
//
//=============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "Customer_config.h"
#include "includes_fw.h"

#if (SENSOR_EN)
#include "lib_retina.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_vif.h"

#include "mmpf_sensor.h"
#include "mmpf_bayerscaler.h"
#include "mmpf_scaler.h"
#include "mmpf_i2cm.h"
#include "mmpf_pio.h"
#include "mmpf_vif.h"
#include "mmpf_system.h"
#include "isp_if.h"
#include "AIT_AECmd.h"
#include "hdr_cfg.h"

//==============================================================================
//
//                              GLOBAL VARIABLE
//
//==============================================================================

static MMPF_SENSOR_CUSTOMER *m_pSnrCustTable[VIF_SENSOR_MAX_NUM] 	= {NULL, NULL};
static MMP_BOOL			m_bCustInited[VIF_SENSOR_MAX_NUM]       	= {MMP_FALSE, MMP_FALSE};

MMPF_SENSOR_ISP_MODE    gSensorISPMode[VIF_SENSOR_MAX_NUM]      	= {MMPF_SENSOR_MODE_INIT, MMPF_SENSOR_MODE_INIT};
MMP_ULONG               gSnrLineCntPerSec[VIF_SENSOR_MAX_NUM]		= {0, 0};

MMP_BOOL                m_bLinkISPSel[VIF_SENSOR_MAX_NUM] 			= {MMP_FALSE, MMP_FALSE};

/* For Fast Change 3A Mode */
#if (ISP_EN)
static ISP_UINT32 		m_AE_PreGain 	    = 0;
static ISP_UINT32 		m_AE_PreShutter     = 0;
static ISP_UINT32 		m_AE_PreShutterBase = 0;
static ISP_UINT32 		m_AE_PreVsync	    = 0;
static ISP_UINT32 		m_AE_PreVsyncBase   = 0;
static ISP_UINT32 		m_AWB_PreGain[3]	= {0, 0, 0};
static MMP_USHORT  		m_usOldPreviewMode  = 0;
#endif

/* For Flip/Rotate Operation */
static MMPF_SENSOR_ROTATE_TYPE 	m_RotateType[VIF_SENSOR_MAX_NUM] = {MMPF_SENSOR_ROTATE_NO_ROTATE, MMPF_SENSOR_ROTATE_NO_ROTATE};
static MMPF_SENSOR_FLIP_TYPE 	m_FlipType[VIF_SENSOR_MAX_NUM]  = {MMPF_SENSOR_NO_FLIP, MMPF_SENSOR_NO_FLIP}; 

//==============================================================================
//
//                              EXTERN VARIABLE
//
//==============================================================================

extern MMP_USHORT   gsCurPreviewMode[];

//==============================================================================
//
//                              FUNCTION PROTOTYPE
//
//==============================================================================

static MMP_ERR MMPF_SensorDrv_SetPreviewMode(MMP_UBYTE ubSnrSel, MMP_USHORT usPreviewMode);
static MMP_ERR MMPF_SensorDrv_SetSensorFlip(MMP_UBYTE ubSnrSel, MMP_USHORT usMode, MMPF_SENSOR_FLIP_TYPE flipType);
static MMP_ERR MMPF_SensorDrv_SetSensorRotate(MMP_UBYTE ubSnrSel, MMP_USHORT usMode, MMPF_SENSOR_ROTATE_TYPE RotateType);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================
#if 0
void ____API_Function____(){ruturn;} //dummy
#endif

MMPF_SENSOR_CUSTOMER *SNR_Cust(MMP_UBYTE ubSnrSel) 
{
    return m_pSnrCustTable[ubSnrSel] ;
}

#if 0
void ____Internal_Function____(){ruturn;} //dummy
#endif

#if (ISP_EN) 
static void SNR_SetIQSysMode( MMP_UBYTE mode)
{
  //printc("IQ.Sys.Mode : %d\r\n", mode );
  ISP_IF_IQ_SetSysMode(mode) ;
}

static void SNR_SetAESysMode( MMP_UBYTE mode)
{
  //printc("AE.Sys.Mode : %d\r\n", mode );
  ISP_IF_AE_SetSysMode(mode) ;
}
#endif
//------------------------------------------------------------------------------
//  Function    : SNR_GetRotateColorId
//  Description : This function is supposed that VIF grab even pixels.
//------------------------------------------------------------------------------
static MMP_UBYTE SNR_GetRotateColorId(MMP_UBYTE ubSnrSel, MMP_UBYTE rotateMode)
{
	MMP_UBYTE ubNewColorID = 0;
    MMP_UBYTE ubXpos = 0, ubNewXpos = 0;
    MMP_UBYTE ubYpos = 0, ubNewYpos = 0;

    if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return 0;
    }

	switch(m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.VifColorId)
	{
		case MMPF_VIF_COLORID_00:
			ubXpos = 0;
			ubYpos = 0;
		break;
		case MMPF_VIF_COLORID_01:
			ubXpos = 0;
			ubYpos = 1;
		break;	
		case MMPF_VIF_COLORID_10:
			ubXpos = 1;
			ubYpos = 0;	
		break;
		case MMPF_VIF_COLORID_11:
			ubXpos = 1;
			ubYpos = 1;	
		break;
        default:
        //
        break;
	}

    switch(rotateMode) 
    {
    	case MMPF_SENSOR_ROTATE_NO_ROTATE:
    		ubNewXpos = ((ubXpos + 0) & 1);
    		ubNewYpos = ((ubYpos + 0) & 1);
    	break;	
    	case MMPF_SENSOR_ROTATE_RIGHT_90:
    		ubNewXpos = ((ubXpos + 1) & 1);
    		ubNewYpos = ((ubYpos + 0) & 1);
    	break;	
    	case MMPF_SENSOR_ROTATE_RIGHT_180:
    		ubNewXpos = ((ubXpos + 1) & 1);
    		ubNewYpos = ((ubYpos + 1) & 1);
    	break;
    	case MMPF_SENSOR_ROTATE_RIGHT_270:
    		ubNewXpos = ((ubXpos + 0) & 1);
    		ubNewYpos = ((ubYpos + 1) & 1);
    	break;
    }

    ubNewColorID = (MMP_UBYTE)((ubNewXpos << 1) | ubNewYpos);
    return ubNewColorID;
}

//------------------------------------------------------------------------------
//  Function    : SNR_GetFlipColorId
//  Description : This function is supposed that VIF grab even pixels.
//------------------------------------------------------------------------------
static MMP_UBYTE SNR_GetFlipColorId(MMP_UBYTE ubSnrSel, MMP_UBYTE flipMode)
{
	MMP_UBYTE ubNewColorID = 0;
    MMP_UBYTE ubXpos = 0, ubNewXpos = 0;
    MMP_UBYTE ubYpos = 0, ubNewYpos = 0;

    if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return 0;
    }

	switch(m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.VifColorId)
	{
		case MMPF_VIF_COLORID_00:
			ubXpos = 0;
			ubYpos = 0;
		break;
		case MMPF_VIF_COLORID_01:
			ubXpos = 0;
			ubYpos = 1;
		break;	
		case MMPF_VIF_COLORID_10:
			ubXpos = 1;
			ubYpos = 0;	
		break;
		case MMPF_VIF_COLORID_11:
			ubXpos = 1;
			ubYpos = 1;	
		break;
        default:
        //
        break;
	}

    switch(flipMode) 
    {
    	case MMPF_SENSOR_NO_FLIP:
    		ubNewXpos = ((ubXpos + 0) & 1);
    		ubNewYpos = ((ubYpos + 0) & 1);
    	break;	
    	case MMPF_SENSOR_COLUMN_FLIP:
    		ubNewXpos = ((ubXpos + 0) & 1);
    		ubNewYpos = ((ubYpos + 1) & 1);
    	break;	
    	case MMPF_SENSOR_ROW_FLIP:
    		ubNewXpos = ((ubXpos + 1) & 1);
    		ubNewYpos = ((ubYpos + 0) & 1);
    	break;
    	case MMPF_SENSOR_COLROW_FLIP:
    		ubNewXpos = ((ubXpos + 1) & 1);
    		ubNewYpos = ((ubYpos + 1) & 1);
    	break;
    }
    
    ubNewColorID = (MMP_UBYTE)((ubNewXpos << 1) | ubNewYpos);
    return ubNewColorID;
}

//------------------------------------------------------------------------------
//  Function    : SNR_SetResolution
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR SNR_SetResolution(MMP_UBYTE ubSnrSel, MMP_UBYTE ubResIdx)
{
#if BIND_SENSOR_PS5220==1
  static    MMP_ULONG i2c_clk = 0 ;
#endif
    MMP_ULONG   i, ulVIFGrabX, ulVIFGrabW, ulVIFGrabY, ulVIFGrabH;
    MMP_USHORT  usAddr, usVal;
    MMP_USHORT  usErrCnt = 0;
    MMP_ERR     err = MMP_ERR_NONE;
    #if (SENSOR_I2CM_DBG_EN)
    MMP_USHORT  usRdVal;
    #endif
	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
#if BIND_SENSOR_PS5220==1
    if(!i2c_clk) {
       i2c_clk = m_pSnrCustTable[ubSnrSel]->i2cmAttr->ulI2cmSpeed ;
    }   
    if( gsCurPreviewMode[ubSnrSel] == 2 ) {
      m_pSnrCustTable[ubSnrSel]->i2cmAttr->ulI2cmSpeed = 150000;
    }
    else {
      m_pSnrCustTable[ubSnrSel]->i2cmAttr->ulI2cmSpeed = i2c_clk ;  
    }
    printc("#I2C.Clk : %d\r\n",m_pSnrCustTable[ubSnrSel]->i2cmAttr->ulI2cmSpeed );
#endif    
    MMPF_SYS_AddTimerMark(1);
    /* Init Sensor OPR */
	for (i = 0; i < m_pSnrCustTable[ubSnrSel]->OprTable->usSize[ubResIdx]; i++) {

		usAddr  = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspTable[ubResIdx] + i*2);
		usVal   = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspTable[ubResIdx] + i*2+1);

		  #if 0
		  if (usAddr != SENSOR_DELAY_REG) {
		    if( usAddr == 0xEF ) {
		        gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, usAddr, usVal) ;
		        printc("0x%02X ,0x%02X,//bank%d\r\n",usAddr,usVal ,usVal);
		    }  
		    else {
		        MMP_USHORT usRdVal;
 		        gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, usAddr, &usRdVal);
 		        if(usRdVal != usVal)
		        printc("0x%02X ,0x%02X,\r\n",usAddr,usVal );
		    }
		  }
		  
		  #endif

    //printc("%02X,%02X\r\n",usAddr,usVal);
	    err = gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, usAddr, usVal);
	    
		if (MMP_ERR_NONE != err) {
	        usErrCnt++;
	    }

	    #if (SENSOR_I2CM_DBG_EN)
	    if (usAddr != SENSOR_DELAY_REG) {
    	    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, usAddr, &usRdVal);
    	    
    	    if (usVal != usRdVal) {
    	        printc(FG_YELLOW(">>>>Res Table ERROR 0x%x %x/%x")"\r\n",usAddr,usVal,usRdVal);
    	    }
	    }
	    #endif

        if (usErrCnt >= MAX_SET_RES_ERR_CNT) {
            return MMP_SENSOR_ERR_INITIALIZE;
        }
	}
    MMPF_SYS_AddTimerMark(1);
    if (usErrCnt >= MAX_SET_RES_ERR_CNT) {
        return MMP_SENSOR_ERR_INITIALIZE;
    }
	
	gSnrLineCntPerSec[ubSnrSel] = (MMP_ULONG)(m_pSnrCustTable[ubSnrSel]->ResTable)->usVsyncLine[ubResIdx] * 
	                              (MMP_ULONG)(m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[ubResIdx] / 10;
	
	ulVIFGrabX = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabStX[ubResIdx];
	ulVIFGrabW = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabW[ubResIdx];
	ulVIFGrabY = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabStY[ubResIdx];
	ulVIFGrabH = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabH[ubResIdx];

    if (m_bLinkISPSel[ubSnrSel])
    {
        #if (ISP_EN)
        ISP_IF_IQ_SetISPInputLength(ulVIFGrabW, ulVIFGrabH);
        
        /* Note: Not to change the ISP color ID */
    	ISP_IF_IQ_SetColorID(0);
        
        ISP_IF_AE_SetMaxSensorFPSx10((m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[ubResIdx]);

        ISP_IF_AE_SetShutterBase(gSnrLineCntPerSec[ubSnrSel]);

        //set default exposure program  (LV10)
        ISP_IF_AE_SetShutter(658, ISP_IF_AE_GetShutterBase());
        ISP_IF_AE_SetGain(ISP_IF_AE_GetGainBase(), ISP_IF_AE_GetGainBase());
        
        /* Initial HDR Status */
    	if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
    	    (gsHdrCfg.ubMode == HDR_MODE_STAGGER))
            ISP_IF_IQ_SetHDRStatus(ISP_IQ_HDR_MODE_STAGGER, MMP_TRUE);
    	else
            ISP_IF_IQ_SetHDRStatus(ISP_IQ_HDR_MODE_STAGGER, MMP_FALSE);

    	/* Calculate shading ratio */ //EROY CHECK
    	{
    		ISP_UINT32 base_x, base_y;
    		ISP_UINT32 x_scale_n = 0x200;
    		ISP_UINT32 y_scale_n = 0x200;
    		ISP_UINT32 y_offset  = 0;
    		
    		ISP_IF_IQ_GetCaliBase(&base_x, &base_y);
    		
    		#if 1//TBD
    		x_scale_n = ulVIFGrabW * 0x200 / base_x;
    		y_scale_n = ulVIFGrabH * 0x200 / base_y;
    		#else
    		x_scale_n = y_scale_n = ulVIFGrabW * 0x200 / base_x;
    		y_offset  = ISP_MAX(((ulVIFGrabW * base_y / base_x) - ulVIFGrabH)/2, 0);		
    		#endif
    		
    		ISP_IF_IQ_SetCaliRatio(	x_scale_n,	//x_scale_n, 
    								0x200,		//x_scale_m, 
    								0,			//x_offset, 
    								y_scale_n,	//y_scale_n, 
    								0x200,		//y_scale_m, 
    								y_offset);	//y_offset
    	}
        #endif
	}

	/* Set VIF Grab Range */
	{
        MMP_UBYTE   		ubVifId = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;
        MMPF_VIF_GRAB_INFO 	vifGrab;
        
        vifGrab.usStartX	= ulVIFGrabX;
        vifGrab.usStartY	= ulVIFGrabY;
        vifGrab.usGrabW		= ulVIFGrabW;
        vifGrab.usGrabH		= ulVIFGrabH;
        
        MMPF_VIF_SetGrabPosition(ubVifId, &vifGrab);
        MMPF_VIF_SetInterruptLine(ubVifId, ulVIFGrabH - 60);
	}
    
    if (m_bLinkISPSel[ubSnrSel])
    {
        #if (ISP_EN)
        /* Set IQ/AE Mode */
        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[ubResIdx] != 0xFF) {
            SNR_SetIQSysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[ubResIdx]);
        }
        else {
            SNR_SetIQSysMode(1); // Set as default mode
        }
        
        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[ubResIdx] != 0xFF) {
            SNR_SetAESysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[ubResIdx]);
        }
        #endif

    	/* Update ISP Bayer raw input and output resolution */
    	#if (CHIP == MCR_V2)
    	{
        	MMP_SCAL_FIT_RANGE		fitrange;
        	MMP_SCAL_FIT_MODE  		sFitmode = MMP_SCAL_FITMODE_OPTIMAL;
        	MMP_SCAL_GRAB_CTRL		grabOut;
        	MMP_ULONG 				ulBayerInW, ulBayerInH;
    		MMP_ULONG 				ulBayerInStX, ulBayerInStY;
         	MMP_ULONG 				ulBayerOutW, ulBayerOutH; 
        	MMP_USHORT 				usDummyOutX, usDummyOutY;

    		ulBayerInStX = (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInGrabX[ubResIdx];
    		ulBayerInStY = (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInGrabY[ubResIdx];

        	ulBayerInW 	= ulVIFGrabW - (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInDummyX[ubResIdx];
        	ulBayerInH 	= ulVIFGrabH - (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInDummyY[ubResIdx];
        	ulBayerOutW	= (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerOutW[ubResIdx];
        	ulBayerOutH = (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerOutH[ubResIdx];

            /* These pixels will be consumed by ISP */
            usDummyOutX = ISP_CONSUMED_PIXEL_X;
            usDummyOutY = ISP_CONSUMED_PIXEL_Y;

            MMPF_BayerScaler_SetZoomInfo(	MMP_BAYER_SCAL_BYPASS, 
            								sFitmode,
    					                	ulVIFGrabW, 	ulVIFGrabH,
    					                	ulBayerInStX, 	ulBayerInStY,
    					                	ulBayerInW, 	ulBayerInH, 
    					                	ulBayerInW, 	ulBayerInH, 
    					                	usDummyOutX, 	usDummyOutY);
            
            MMPF_BayerScaler_SetZoomInfo(	MMP_BAYER_SCAL_DOWN, 
            								sFitmode,
    						                ulVIFGrabW, 	ulVIFGrabH,
    						                ulBayerInStX, 	ulBayerInStY,
    						                ulBayerInW, 	ulBayerInH, 
    						                ulBayerOutW, 	ulBayerOutH, 
    						                usDummyOutX, 	usDummyOutY);

            MMPF_BayerScaler_GetZoomInfo(MMP_BAYER_SCAL_DOWN, &fitrange, &grabOut);

            MMPF_BayerScaler_SetEngine(&fitrange, &grabOut);
            
            if (sFitmode == MMP_SCAL_FITMODE_OPTIMAL) {
    	        if ((grabOut.ulScaleXN != grabOut.ulScaleXM) || (grabOut.ulScaleYN != grabOut.ulScaleYM))
    	            MMPF_BayerScaler_SetEnable(MMP_TRUE);
    	        else
    	            MMPF_BayerScaler_SetEnable(MMP_FALSE);
            }
            else {
     	        if ((grabOut.ulScaleN != grabOut.ulScaleM))
    	            MMPF_BayerScaler_SetEnable(MMP_TRUE);
    	        else
    	            MMPF_BayerScaler_SetEnable(MMP_FALSE);
            }
    	}
    	#endif
        
        
        #if (ISP_EN)
    	/* Set IQ (NR, Edge, CCM, Gamma, etc.) and functions (saturation, contrast, sharpness, hue, etc.) */
    	ISP_IF_IQ_SetAll();

    	// SetBeforePreview and SetBeforeCapture should be called after getting new configurations of the current resolution
    	if (gSensorISPMode[ubSnrSel] == MMPF_SENSOR_SNAPSHOT) {

    		ISP_IF_AE_UpdateBeforeCapture();
    		ISP_IF_AWB_UpdateBeforeCapture();
    	}
    	else if (gSensorISPMode[ubSnrSel] == MMPF_SENSOR_PREVIEW) {

    		ISP_IF_AE_UpdateBeforePreview();
    		ISP_IF_AWB_UpdateBeforePreview();
    	}

    	/* Set direction (color ID, orientation, etc.) */
    	ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_ORIGINAL);

    	/* Set exposure parameters */
    	if(0){ //SEAN.DEL
    		MMP_ULONG ulVsync = 0;
    		MMP_ULONG ulShutter = 0;
    		
    		ulVsync     = gSnrLineCntPerSec[ubSnrSel] * 10 / (m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[ubResIdx];
    		ulShutter   = gSnrLineCntPerSec[ubSnrSel] * ISP_IF_AE_GetShutter() / ISP_IF_AE_GetShutterBase();		

    		gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, ISP_IF_AE_GetGain());		
    		gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, ulShutter, ulVsync);
    	}
    	
    	/* Update ISP window */
    	ISP_IF_IQ_UpdateInputSize();

    	ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AE,  1);
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AWB, 1);
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AF,  1);
        
    	/* Check if modules are bypassed */
    	ISP_IF_IQ_CheckBypass();
    	
    	/* Force update IQ to hardware */
    	ISP_IF_IQ_UpdateOprtoHW(ISP_IQ_SWITCH_ALL, 1);

        /* Set AE metering mode */
    	ISP_IF_AE_SetMetering(ISP_AE_METERING_AVERAGE);
        #endif
    }
    
    return MMP_ERR_NONE;
}

#if 0
void ____Common_Function____(){ruturn;} //dummy
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_Register
//  Description : This function should be called before using sensor function.
//------------------------------------------------------------------------------
MMP_ERR MMPF_SensorDrv_Register(MMP_UBYTE ubSnrSel, MMPF_SENSOR_CUSTOMER* pCust)
{
    m_pSnrCustTable[ubSnrSel] = pCust;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_InitCustFunc
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_InitCustFunc(MMP_UBYTE ubSnrSel)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    m_pSnrCustTable[ubSnrSel]->MMPF_Cust_InitConfig();

	m_bCustInited[ubSnrSel] = MMP_TRUE;

	m_RotateType[ubSnrSel]   = MMPF_SENSOR_ROTATE_NO_ROTATE;
	m_FlipType[ubSnrSel]     = MMPF_SENSOR_NO_FLIP;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_InitSensor
//  Description :
//------------------------------------------------------------------------------
/* static MMP_ERR MMPF_SensorDrv_InitSensor(MMP_UBYTE ubSnrSel) ITCMFUNC; */
static MMP_ERR MMPF_SensorDrv_InitSensor(MMP_UBYTE ubSnrSel)
{
	MMP_ULONG   i = 0;
	MMP_USHORT  usAddr, usVal;
    MMP_USHORT  usErrCnt = 0;
	MMP_USHORT  usSizeCnt = 0;
	MMP_ERR     I2cmErr = MMP_ERR_NONE;
    #if (SENSOR_I2CM_DBG_EN)
    MMP_USHORT  usRdVal;
    #endif

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
    
    /* Reset VIF module */
	MMPF_VIF_ResetModule(m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId);

	#if (BIND_SENSOR_OV10822) // SW Reset TBD
	gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x0103, 0x01);
	MMPF_OS_Sleep_MS(20);
	#endif
	
	/* Initial I2cm */
    MMPF_I2cm_Initialize(m_pSnrCustTable[ubSnrSel]->i2cmAttr);
    MMPF_OS_Sleep_MS(2);

    MMPF_SYS_AddTimerMark(0);
    /* Write Initial Table */
	for (i = 0; i < m_pSnrCustTable[ubSnrSel]->OprTable->usInitSize; i++) 
	{
		usAddr  = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspInitTable + i*2);
		usVal   = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspInitTable + i*2+1);	

		while(1)
		{
    		if (MMP_ERR_NONE == gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, usAddr, usVal)) {

        	    #if (SENSOR_I2CM_DBG_EN)
        	    if (usAddr != SENSOR_DELAY_REG) {
            	    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, usAddr, &usRdVal);

            	    if (usVal != usRdVal) {
            	        printc(FG_YELLOW(">>>>Init Table ERROR 0x%x %x/%x")"\r\n",usAddr,usVal,usRdVal);
            	    }
        	    }
        	    #endif
		        break;
		    }
		    else {
		        MMPF_OS_Sleep_MS(2);
		        usErrCnt++;
		    }

            if (usErrCnt >= MAX_INIT_ERR_CNT) {
                return MMP_SENSOR_ERR_INITIALIZE;
            }
		}
	}
    MMPF_SYS_AddTimerMark(0);
    /* Write Binary Table */
    if (m_pSnrCustTable[ubSnrSel]->OprTable->bBinTableExist)
    {
    	for (i = 0; i < m_pSnrCustTable[ubSnrSel]->OprTable->usBinTableNum; i++) 
    	{
    		usAddr      = m_pSnrCustTable[ubSnrSel]->OprTable->usBinRegAddr[i];
    		usSizeCnt   = m_pSnrCustTable[ubSnrSel]->OprTable->usBinSize[i];
#if 1
            I2cmErr = MMPF_I2cm_DMAWriteBurstData(m_pSnrCustTable[ubSnrSel]->i2cmAttr,
                                                  usAddr,
                                                  m_pSnrCustTable[ubSnrSel]->OprTable->ubBinTable[i],
                                                  usSizeCnt);

            if (I2cmErr != MMP_ERR_NONE) {
                printc(FG_YELLOW(">>>>Bin Table ERROR 0x%x")"\r\n",I2cmErr);
            }
#else
            for (j = 0; j < usSizeCnt; j++) {
                I2cmErr = MMPF_I2cm_WriteReg(m_pSnrCustTable[ubSnrSel]->i2cmAttr, usAddr, m_pSnrCustTable[ubSnrSel]->OprTable->ubBinTable[i][j]);
            }
#endif
    	}
    }
    
    /* Write Initial Done Table */
    if (m_pSnrCustTable[ubSnrSel]->OprTable->bInitDoneTableExist)
    {
    	for (i = 0; i < m_pSnrCustTable[ubSnrSel]->OprTable->usInitDoneSize; i++) 
    	{
    		usAddr  = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspInitDoneTable + i*2);
    		usVal   = *(m_pSnrCustTable[ubSnrSel]->OprTable->uspInitDoneTable + i*2+1);	

    		while(1)
    		{
        		if (MMP_ERR_NONE == gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, usAddr, usVal)) {

            	    #if (SENSOR_I2CM_DBG_EN)
            	    if (usAddr != SENSOR_DELAY_REG) {
                	    gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, usAddr, &usRdVal);

                	    if (usVal != usRdVal) {
                	        printc(FG_YELLOW(">>>>Init Table ERROR 0x%x %x/%x")"\r\n",usAddr,usVal,usRdVal);
                	    }
            	    }
            	    #endif
    		        break;
    		    }
    		    else {
    		        MMPF_OS_Sleep_MS(2);
    		        usErrCnt++;
    		    }

                if (usErrCnt >= MAX_INIT_ERR_CNT) {
                    return MMP_SENSOR_ERR_INITIALIZE;
                }
    		}
    	}
	}

    if (usErrCnt >= MAX_INIT_ERR_CNT) {
        return MMP_SENSOR_ERR_INITIALIZE;
    }
    
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_InitVIF
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_InitVIF(MMP_UBYTE ubSnrSel)
{
    AITPS_VIF   pVIF  = AITC_BASE_VIF;
    AITPS_GBL   pGBL  = AITC_BASE_GBL;
    MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
    
    ubVifId = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;

    MMPF_VIF_SetSensorType(ubVifId, m_pSnrCustTable[ubSnrSel]->VifSetting->SnrType);

    // Turn On Sensor Power
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bTurnOnExtPower == MMP_TRUE)
    {
        MMP_GPIO_PIN piopin = m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.usExtPowerPin;

        if (piopin != MMP_GPIO_MAX) {
            MMPF_PIO_EnableOutputMode(piopin, MMP_TRUE, MMP_TRUE);
            MMPF_PIO_SetData(piopin, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bExtPowerPinHigh, MMP_TRUE);
            MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.usExtPowerPinDelay);
        }    
    }
    
    // Sensor Pad setting
    if (ubVifId == MMPF_VIF_MDL_ID0)
    	pGBL->GBL_MISC_IO_CFG |= GBL_VIF0_GPIO_EN;
    else if (ubVifId == MMPF_VIF_MDL_ID1)
    	pGBL->GBL_MISC_IO_CFG |= GBL_VIF1_GPIO_EN;
    	
    pGBL->GBL_SW_RESOL_SEL = GBL_SENSOR_8M; // Default always enable 8M. If need, can modify by sensor reolution later.
    
    // Power-On Sequence
    {
        // Enable Pin
	    MMPF_VIF_SetPIODir(ubVifId, VIF_SIF_SEN, MMP_TRUE);
	    MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_SEN, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bFirstEnPinHigh);
	    MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.ubFirstEnPinDelay);
	    MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_SEN, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bNextEnPinHigh);
	    MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.ubNextEnPinDelay);
        
        // Delay more 10ms
        //MMPF_OS_Sleep(10);

        if (m_pSnrCustTable[ubSnrSel]->VifSetting->clockAttr.ubClkSrc == MMPF_VIF_SNR_CLK_SRC_PMCLK)
        {
            if (ubVifId == MMPF_VIF_MDL_ID0)
            	pGBL->GBL_SENSOR_CLK_SRC &= ~(SENSR0_CLK_SRC_VI);
            else if (ubVifId == MMPF_VIF_MDL_ID1)
            	pGBL->GBL_SENSOR_CLK_SRC &= ~(SENSR1_CLK_SRC_VI);
        }
        else
        {
            if (ubVifId == MMPF_VIF_MDL_ID0)
            	pGBL->GBL_SENSOR_CLK_SRC |= (SENSR0_CLK_SRC_VI);
            else if (ubVifId == MMPF_VIF_MDL_ID1)
            	pGBL->GBL_SENSOR_CLK_SRC |= (SENSR1_CLK_SRC_VI);
        }

        // Set Sensor Clock Attribute
        if (m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bTurnOnClockBeforeRst) {
            MMPF_VIF_SetSensorMClockAttr(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->clockAttr);
        }

        // Reset Pin
        MMPF_VIF_SetPIODir(ubVifId, VIF_SIF_RST, MMP_TRUE);
        MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_RST, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bFirstRstPinHigh);
        MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.ubFirstRstPinDelay);
        MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_RST, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bNextRstPinHigh);
        MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.ubNextRstPinDelay);
    }
    
    // Initial VIF setting
	pVIF->VIF_INT_HOST_EN[ubVifId]  = 0;
	pVIF->VIF_INT_CPU_SR[ubVifId]   = VIF_INT_ALL;
	pVIF->VIF_OUT_EN[ubVifId]       = VIF_OUT_DISABLE;
	pVIF->VIF_FRME_SKIP_NO[ubVifId] = 0;
	pVIF->VIF_FRME_SKIP_EN[ubVifId] = 0;	
	pVIF->VIF_INT_MODE[ubVifId]     = VIF_INT_EVERY_FRM;
	pVIF->VIF_IGBT_EN[ubVifId]      = 0;
	pVIF->VIF_RAW_OUT_EN[ubVifId]   = VIF_2_ISP_EN;
	pVIF->VIF_YUV_CTL[ubVifId]      = VIF_YUV422_FMT_VYUY;
    pVIF->VIF_OPR_UPD[ubVifId]      = VIF_OPR_UPD_EN | VIF_OPR_UPD_FRAME_SYNC;	

	if ((gsHdrCfg.bVidEnable || gsHdrCfg.bDscEnable) && 
   		(gsHdrCfg.ubMode == HDR_MODE_STAGGER)) {
		pVIF->VIF_IMG_BUF_EN[ubVifId] = VIF_TO_ISP_IMG_BUF_EN | VIF_TO_ISP_CLR_IMG_BUF_DIS;
	}
    
    // Set Sensor Clock Attribute if UnInitialized
    if (!m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bTurnOnClockBeforeRst) {
        MMPF_VIF_SetSensorMClockAttr(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->clockAttr);
    }
    
    // Set MIPI/Parallel Attribute
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->OutInterface != MMPF_VIF_IF_PARALLEL) {
    	
    	MMPF_VIF_SetSensorMipiAttr(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->mipiAttr);

    	if (m_pSnrCustTable[ubSnrSel]->VifSetting->vcAttr.bEnable) {
    		MMPF_VIF_SetVirtualChannel(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->vcAttr);
    	}

    	MMPF_VIF_SetInputInterface(ubVifId, MMPF_VIF_MIPI);
    }
    else {
        MMPF_VIF_SetParallelTimingAttr(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->paralAttr);
        
        if (m_pSnrCustTable[ubSnrSel]->VifSetting->paralAttr.ubBusBitMode == MMPF_VIF_SNR_PARAL_BITMODE_10)
            pVIF->VIF_PARALLEL_SNR_BUS_CFG = VIF_SNR_INPUT_BUS_10BIT_EN;
        else
            pVIF->VIF_PARALLEL_SNR_BUS_CFG = VIF_SNR_INPUT_BUS_16BIT_EN;

        MMPF_VIF_SetInputInterface(ubVifId, MMPF_VIF_PARALLEL);  
    }

    // Set Color ID
    MMPF_VIF_SetColorID(ubVifId, m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.VifColorId);

    // Set YUV Store
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->SnrType == MMPF_VIF_SNR_TYPE_YUV ||
        m_pSnrCustTable[ubSnrSel]->VifSetting->SnrType == MMPF_VIF_SNR_TYPE_YUV_TV_DEC) {
        MMPF_VIF_SetYUVAttr(ubVifId, &m_pSnrCustTable[ubSnrSel]->VifSetting->yuvAttr);
    }

	// Check Sensor Version
	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_CheckVersion(ubSnrSel, NULL);

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_InitISP
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_InitISP(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
	
	static MMP_BOOL bInitISP = MMP_FALSE;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

    if (m_bLinkISPSel[ubSnrSel])
    {
    	if (!bInitISP) {

        	// Initialize ISP Lib
    		ISP_IF_LIB_Init();
        
    		// Check if ISP Lib and IQ are mis-matched.
    		if ((int)ISP_IF_LIB_CompareIQVer() != (int)MMP_ERR_NONE) {
    			RTNA_DBG_Str(0, "Wrong ISP lib version!\r\n");
    		}

    		// Initialize 3A
    		ISP_IF_3A_Init();

    		bInitISP = MMP_TRUE;
    	}

        // Callback function
        if (MMP_SensorDrv_PostInitISP != NULL) {
            MMP_SensorDrv_PostInitISP();
        }
    }

#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_PowerDown
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_PowerDown(MMP_UBYTE ubSnrSel)
{
    AITPS_VIF   pVIF    = AITC_BASE_VIF;
	AITPS_MIPI  pMIPI   = AITC_BASE_MIPI;
    MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

	ubVifId = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;

    // Enter Standby Mode (if nessarsary)
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.bEnterStandByMode) {
    
        MMP_USHORT data = 0;
        MMP_USHORT addr = m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.usStandByModeReg;
        MMP_USHORT mask = m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.usStandByModeMask;
        
        gsSensorFunction->MMPF_Sensor_GetReg(ubSnrSel, addr, &data);
        data &= ~(mask);
        gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, addr, data); 
        MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_RST, MMP_FALSE);    
        MMPF_OS_Sleep(10);
    }
    
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->OutInterface != MMPF_VIF_IF_PARALLEL) {
        // Disable MIPI clock and all data lane
        pMIPI->MIPI_CLK_CFG[ubVifId] &= ~(MIPI_CSI2_EN);
        
        pMIPI->DATA_LANE[0].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_EN);
        pMIPI->DATA_LANE[1].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_EN);
        pMIPI->DATA_LANE[2].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_EN);
        pMIPI->DATA_LANE[3].MIPI_DATA_CFG[ubVifId] &= ~(MIPI_DAT_LANE_EN);
    }
    else {
        // Disable VIF input
        MMPF_VIF_EnableInputInterface(ubVifId, MMP_FALSE);
    }
    
    // Enable Pin pull high/low
    MMPF_VIF_SetPIODir(ubVifId, VIF_SIF_SEN, MMP_TRUE);
    MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_SEN, m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.bEnPinHigh);
    MMPF_OS_Sleep(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.ubEnPinDelay);
    // Turn Off sensor main clock
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.bTurnOffMClock) {
        pVIF->VIF_SENSR_MCLK_CTL[ubVifId] &= ~(VIF_SENSR_MCLK_EN);
    }
    
    // Turn Off Sensor Power
    if (m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.bTurnOffExtPower) {

        MMP_GPIO_PIN piopin = m_pSnrCustTable[ubSnrSel]->VifSetting->powerOffSetting.usExtPowerPin;
     
        if (piopin != MMP_GPIO_MAX) {
            MMPF_PIO_EnableOutputMode(piopin, MMP_TRUE, MMP_TRUE);
            MMPF_PIO_SetData(piopin, ~(m_pSnrCustTable[ubSnrSel]->VifSetting->powerOnSetting.bExtPowerPinHigh), MMP_TRUE);
        }
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_ChangeMode
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_ChangeMode(MMP_UBYTE ubSnrSel, MMP_USHORT usNewSnrMode, MMP_USHORT usWaitFrames)
{
	MMP_UBYTE   ubVifId = MMPF_VIF_MDL_ID0;
    MMP_UBYTE   ubStatusAE = 0;
    MMP_UBYTE   ubStatusAWB = 0;
    MMP_UBYTE   ubStatusAF = 0;
    MMP_ERR     err     = MMP_ERR_NONE;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

	ubVifId = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;

    /*
     * UI may switch 3A status by ISP API directly and want to keep 3A status.
     * To avoid 3A switch on anyway in SNR_SetResolution(),
     * we keep 3A status here and restore after change sensor mode.
     */
    #if (ISP_EN)
    if (m_bLinkISPSel[ubSnrSel])
    {
        ubStatusAE  = ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AE);
        ubStatusAWB = ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AWB);
        ubStatusAF  = ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AF);
    }
    #endif

	err = MMPF_SensorDrv_SetPreviewMode(ubSnrSel, usNewSnrMode);
	
	MMPF_VIF_CheckFrameSig(ubVifId, MMPF_VIF_INT_EVENT_FRM_ST, usWaitFrames);

   	//MMPF_SensorDrv_SetSensorRotate(ubSnrSel, usNewSnrMode, m_RotateType[ubSnrSel]);
    MMPF_SensorDrv_SetSensorFlip(ubSnrSel, usNewSnrMode, m_FlipType[ubSnrSel]);

    /*
     * Restore 3A status
     */
    #if (ISP_EN)
    if (m_bLinkISPSel[ubSnrSel])
    {
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AE,  ubStatusAE);
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AWB, ubStatusAWB);
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AF,  ubStatusAF);
    }
    #endif

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetPreviewMode
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetPreviewMode(MMP_UBYTE ubSnrSel, MMP_USHORT usPreviewMode)
{
    MMP_USHORT  usSnrResIdx = usPreviewMode;
    MMP_ERR     err = MMP_ERR_NONE;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    gsCurPreviewMode[ubSnrSel] = usPreviewMode; 
       
    switch (gsSensorMode[ubSnrSel]) {
    #if (VIDEO_R_EN)
    case SENSOR_VIDEO_MODE:
		RTNA_DBG_Str(3, "SENSOR_VIDEO_MODE\r\n");
		if (m_bLinkISPSel[ubSnrSel]) {
		    MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_FALSE);
		}
		gSensorISPMode[ubSnrSel] = MMPF_SENSOR_PREVIEW;
        err = SNR_SetResolution(ubSnrSel, usSnrResIdx);
		if (m_bLinkISPSel[ubSnrSel])
		{
	        #if (ISP_EN)
    		if ((m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[usSnrResIdx]%10 == 0)
    			ISP_IF_AE_SetFPS((m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[usSnrResIdx]/10);	
    		else
    			ISP_IF_AE_SetMaxSensorFPSx10((m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[usSnrResIdx]);
            #endif
            MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
        }
        ISP_IF_AE_SetFastMode(1);
        ISP_IF_AWB_SetFastMode(1);
        //SEAN : TBD Disable WDR here !
        ISP_IF_F_SetWDREn(1);
		ISP_IF_F_SetWDR(255);
        break;
    #endif
    #if (DSC_R_EN)
    case SENSOR_DSC_MODE:
        RTNA_DBG_Str(3, "SENSOR_DSC_MODE\r\n");
        if (m_bLinkISPSel[ubSnrSel]) {
            MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_FALSE);
		}
		gSensorISPMode[ubSnrSel] = MMPF_SENSOR_PREVIEW;
        err = SNR_SetResolution(ubSnrSel, usSnrResIdx);
		if (m_bLinkISPSel[ubSnrSel])
		{
    		#if (ISP_EN)
            //ISP_IF_AE_SetFastMode(1);   // Enable AE Fast Mode
            ISP_IF_AE_SetFPS(0);        // Auto FPS
    		#endif
            MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
        }
        break;
    #endif
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetReg
//  Description :
// -----------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetReg(MMP_UBYTE ubSnrSel, MMP_USHORT usAddr, MMP_USHORT usData) ITCMFUNC;
static MMP_ERR MMPF_SensorDrv_SetReg(MMP_UBYTE ubSnrSel, MMP_USHORT usAddr, MMP_USHORT usData)
{
    MMP_ERR err = MMP_ERR_NONE;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (usAddr == SENSOR_DELAY_REG) {
        RTNA_WAIT_MS(usData);
    }
    else {
        err = MMPF_I2cm_WriteReg(m_pSnrCustTable[ubSnrSel]->i2cmAttr, usAddr, usData);
    }
    return err;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetReg
//  Description :
// -----------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_GetReg(MMP_UBYTE ubSnrSel, MMP_USHORT usAddr, MMP_USHORT *usData)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

	return MMPF_I2cm_ReadReg(m_pSnrCustTable[ubSnrSel]->i2cmAttr, usAddr, usData);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_DoAWBOperation
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_DoAWBOperation(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
    MMP_ULONG  ulFrameCnt 	   = 0;
	MMP_UBYTE  ubPeriod        = 0;
	MMP_UBYTE  ubDoAWBFrmCnt   = 0;	
	MMP_UBYTE  ubDoCaliFrmCnt  = 0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
    
    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }
    
    if (ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AWB) != 1) { 
        return MMP_ERR_NONE;
    }

	ubPeriod        = (m_pSnrCustTable[ubSnrSel]->pAwbTime)->ubPeriod;
	ubDoAWBFrmCnt   = (m_pSnrCustTable[ubSnrSel]->pAwbTime)->ubDoAWBFrmCnt;	
	ubDoCaliFrmCnt  = (m_pSnrCustTable[ubSnrSel]->pAwbTime)->ubDoCaliFrmCnt;

	MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulFrameCnt);

    if (ulFrameCnt == 30) {
        ISP_IF_AE_SetFastMode(0);
        ISP_IF_AWB_SetFastMode(0);
    }
    else if (ulFrameCnt < 10) {
        ISP_IF_AWB_Execute();
		ISP_IF_IQ_SetAWBGains(ISP_IF_AWB_GetGainR(), ISP_IF_AWB_GetGainG(), ISP_IF_AWB_GetGainB(), ISP_IF_AWB_GetGainBase());
        return MMP_ERR_NONE;
    }

	if (ulFrameCnt % ubPeriod == ubDoAWBFrmCnt) {
		ISP_IF_AWB_Execute();
		ISP_IF_IQ_SetAWBGains(ISP_IF_AWB_GetGainR(), ISP_IF_AWB_GetGainG(), ISP_IF_AWB_GetGainB(), ISP_IF_AWB_GetGainBase());
    }

    if (ulFrameCnt % ubPeriod == ubDoCaliFrmCnt) {
		ISP_IF_CALI_Execute(); // Update Lens Shading Table
	}
	
	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_DoAWBOperation(ubSnrSel, ulFrameCnt);
#endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_DoAEOperation_ST
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_DoAEOperation_ST(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
    MMP_ULONG ulFrameCnt = 0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }

    if (ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AE) != 1) { 
        return MMP_ERR_NONE;
    }

    MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulFrameCnt);

	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_DoAEOperation_ST(ubSnrSel, ulFrameCnt);
    /*
    RTNA_DBG_Long(0, ISP_IF_AE_GetDbgData(0));
    RTNA_DBG_Long(0, ISP_IF_AE_GetDbgData(1));
    MMPF_DBG_Int(ISP_IF_AWB_GetColorTemp(), -5);
    RTNA_DBG_Str(0, "\r\n");
    */
#endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_DoAEOperation_END
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_DoAEOperation_END(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
    MMP_ULONG ulFrameCnt = 0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }

    if (ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AE) != 1) { 
        return MMP_ERR_NONE;
    }
    
    MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulFrameCnt);
	ISP_IF_AE_GetHWAcc(1);

	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_DoAEOperation_END(ubSnrSel, ulFrameCnt);
#endif

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_DoAFOperation
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_DoAFOperation(MMP_UBYTE ubSnrSel)
{
#if (SUPPORT_AUTO_FOCUS) && (ISP_EN)

	MMP_ULONG   ulFrameCnt    = 0;
    MMP_USHORT  ubPeriod      = 0;
    MMP_USHORT  ubDoAFFrmCnt  = 0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }

    if (ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AF) != 1) { 
        return MMP_ERR_NONE;
    }

    ubPeriod      = (m_pSnrCustTable[ubSnrSel]->pAfTime)->ubPeriod;
    ubDoAFFrmCnt  = (m_pSnrCustTable[ubSnrSel]->pAfTime)->ubDoAFFrmCnt;
	
	MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulFrameCnt);

	if (ulFrameCnt % ubPeriod == ubDoAFFrmCnt) {
		ISP_IF_AF_GetHWAcc(0);
		ISP_IF_AF_Execute();
	}
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_DoIQOperation
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_DoIQOperation(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
	MMP_ULONG ulFrameCnt = 0;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }

    MMPF_Sensor_GetParam(ubSnrSel, MMPF_SENSOR_ISP_FRAME_COUNT, &ulFrameCnt);

	// Set IQ at frame end to ensure frame sync
	ISP_IF_IQ_SetAll();
	ISP_IF_IQ_CheckBypass();

	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_DoIQOperation(ubSnrSel, ulFrameCnt);
#endif

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetExposureLimit
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetExposureLimit(MMP_ULONG ulBufaddr, MMP_ULONG ulDataTypeByByte, MMP_ULONG ulSize)
{
#if (ISP_EN) //TBD
	ISP_CMD_STATUS cmd_status;
	
	cmd_status = ISP_IF_CMD_SendCommand(ISP_CMD_CLASS_AE, AIT_AEV1_SET_PREVIEWEXPOSCHEME, (void*)ulBufaddr, ulDataTypeByByte, ulSize);	
	
	if (cmd_status == ISP_CMD_STATUS_SUCCESS)
		return MMP_ERR_NONE;
	else 
		return 1;//Err
#else
    return MMP_ERR_NONE;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetGain
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetGain(MMP_UBYTE ubSnrSel, MMP_ULONG ulGain)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    m_pSnrCustTable[ubSnrSel]->MMPF_Cust_SetGain(ubSnrSel, ulGain);

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetShutter
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetShutter(MMP_UBYTE ubSnrSel, MMP_ULONG ulShutter, MMP_ULONG ulVsync)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
    
    m_pSnrCustTable[ubSnrSel]->MMPF_Cust_SetShutter(ubSnrSel, ulShutter, ulVsync);
    
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetSensorFlip
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetSensorFlip(MMP_UBYTE ubSnrSel, MMP_USHORT usMode, MMPF_SENSOR_FLIP_TYPE flipType)
{
	MMP_BOOL 	bUseCustomId = MMP_FALSE;
	MMP_UBYTE 	ubVifId 	 = MMPF_VIF_MDL_ID0;
	MMPF_VIF_COLOR_ID CustomColorId = MMPF_VIF_COLORID_UNDEF;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

	bUseCustomId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.bUseCustomId;
	ubVifId 	 = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;

	MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_FALSE);

    switch(flipType) {
    case MMPF_SENSOR_NO_FLIP:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.Rot0d_Id[usMode];
        break;
    case MMPF_SENSOR_COLUMN_FLIP:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.V_Flip_Id[usMode];
    	break;
    case MMPF_SENSOR_ROW_FLIP:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.H_Flip_Id[usMode];
        break;
    case MMPF_SENSOR_COLROW_FLIP:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.HV_Flip_Id[usMode];
        break;
    default:
        return MMP_SENSOR_ERR_PARAMETER;
    }

	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_SetFlip(ubSnrSel, (MMP_UBYTE)flipType);

    switch(flipType) {
    case MMPF_SENSOR_NO_FLIP:
       	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetFlipColorId(ubSnrSel, flipType));
    	#if (ISP_EN)
        if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_ORIGINAL);
    	}
    	#endif
    	break;
    case MMPF_SENSOR_COLUMN_FLIP:
       	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetFlipColorId(ubSnrSel, flipType));
        #if (ISP_EN)
        if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_V_MIRROR);
        }
        #endif
        break;
    case MMPF_SENSOR_ROW_FLIP:
       	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetFlipColorId(ubSnrSel, flipType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_H_MIRROR);
        }
        #endif
        break;
    case MMPF_SENSOR_COLROW_FLIP:
       	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetFlipColorId(ubSnrSel, flipType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_180_DEGREE);
        }
        #endif
        break;
    default:
        MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
        return MMP_SENSOR_ERR_PARAMETER;
    }

	MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);

	m_FlipType[ubSnrSel] = flipType;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_SetSensorRotate
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_SetSensorRotate(MMP_UBYTE ubSnrSel, MMP_USHORT usMode, MMPF_SENSOR_ROTATE_TYPE RotateType)
{
	MMP_BOOL 	bUseCustomId = MMP_FALSE;
	MMP_UBYTE 	ubVifId 	 = MMPF_VIF_MDL_ID0;
	MMPF_VIF_COLOR_ID CustomColorId = MMPF_VIF_COLORID_UNDEF;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

	bUseCustomId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.bUseCustomId;
	ubVifId 	 = m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;
	
    switch(RotateType) {
    case MMPF_SENSOR_ROTATE_NO_ROTATE:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.Rot0d_Id[usMode];
        break;
    case MMPF_SENSOR_ROTATE_RIGHT_90:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.Rot90d_Id[usMode];
    	break;
    case MMPF_SENSOR_ROTATE_RIGHT_180:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.Rot180d_Id[usMode];
        break;
    case MMPF_SENSOR_ROTATE_RIGHT_270:
		CustomColorId = m_pSnrCustTable[ubSnrSel]->VifSetting->colorId.CustomColorId.Rot270d_Id[usMode];
    	break;
    default:
        return MMP_SENSOR_ERR_PARAMETER;
    }

	MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_FALSE);

	m_pSnrCustTable[ubSnrSel]->MMPF_Cust_SetRotate(ubSnrSel, (MMP_UBYTE)RotateType);

    switch(RotateType) {
    case MMPF_SENSOR_ROTATE_NO_ROTATE:
    	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetRotateColorId(ubSnrSel, RotateType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_ORIGINAL);
        }
        #endif
        break;
    case MMPF_SENSOR_ROTATE_RIGHT_90:
    	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetRotateColorId(ubSnrSel, RotateType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
    	    ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_90_DEGREE);
    	}
    	#endif
    	break;
    case MMPF_SENSOR_ROTATE_RIGHT_180:
    	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetRotateColorId(ubSnrSel, RotateType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
            ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_180_DEGREE);
        }
        #endif
        break;
    case MMPF_SENSOR_ROTATE_RIGHT_270:
    	if (bUseCustomId && CustomColorId != MMPF_VIF_COLORID_UNDEF)
    		MMPF_VIF_SetColorID(ubVifId, CustomColorId);
    	else
    		MMPF_VIF_SetColorID(ubVifId, SNR_GetRotateColorId(ubSnrSel, RotateType));
    	#if (ISP_EN)
    	if (m_bLinkISPSel[ubSnrSel]) {
    	    ISP_IF_IQ_SetDirection(ISP_IQ_DIRECTION_270_DEGREE);
    	}
    	#endif
    	break;
    default:
		MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
        return MMP_SENSOR_ERR_PARAMETER;
    }

	MMPF_ISP_Set3AFunction(MMP_ISP_3A_FUNC_SET_3A_ENABLE, MMP_TRUE);
	
	m_RotateType[ubSnrSel] = RotateType;

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetRealFPS
//  Description :
//------------------------------------------------------------------------------
static MMP_USHORT MMPF_SensorDrv_GetRealFPS(MMP_UBYTE ubSnrSel)
{
#if (ISP_EN)
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return 30;
	}

    if (m_bLinkISPSel[ubSnrSel]) {
	    return ISP_IF_AE_GetRealFPS();
	}
	else {
	    return 30;
	}
#else
    return 30;
#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetSensorVifPad
//  Description :
//------------------------------------------------------------------------------
static MMP_UBYTE MMPF_SensorDrv_GetSensorVifPad(MMP_UBYTE ubSnrSel)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return 0;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return 0;
    }

	if (m_bCustInited[ubSnrSel])
		return m_pSnrCustTable[ubSnrSel]->VifSetting->VifPadId;
	else
		return 0;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetScalInputRes
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_GetScalInputRes(MMP_UBYTE ubSnrSel, MMP_USHORT usResIdx, MMP_ULONG *pulWidth, MMP_ULONG *pulHeight)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (pulWidth != NULL) {
	    *pulWidth = (m_pSnrCustTable[ubSnrSel]->ResTable)->usScalInputW[usResIdx];
	}
	if (pulHeight != NULL) {
	    *pulHeight = (m_pSnrCustTable[ubSnrSel]->ResTable)->usScalInputH[usResIdx];
    }

	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetBayerRes
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_GetBayerRes(MMP_UBYTE ubSnrSel, MMP_USHORT usResIdx, 
									      MMP_ULONG *pulInWidth, MMP_ULONG *pulInHeight,
										  MMP_ULONG *pulOutWidth, MMP_ULONG *pulOutHeight)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }
	
	if (pulInWidth != NULL) {
		*pulInWidth = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabW[usResIdx] - 
					  (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInDummyX[usResIdx];
	}
	if (pulInHeight != NULL) {
		*pulInHeight = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVifGrabH[usResIdx] - 
					   (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerInDummyY[usResIdx];
	}
	if (pulOutWidth != NULL) {
		*pulOutWidth = (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerOutW[usResIdx];
	}
	if (pulOutHeight != NULL) {
		*pulOutHeight = (m_pSnrCustTable[ubSnrSel]->ResTable)->usBayerOutH[usResIdx];
	}
	
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetTargetFpsx10
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_GetTargetFpsx10(MMP_UBYTE ubSnrSel, MMP_USHORT usResIdx, MMP_ULONG *pulFpsx10)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (pulFpsx10 != NULL) {
        *pulFpsx10 = m_pSnrCustTable[ubSnrSel]->ResTable->usTargetFpsx10[usResIdx];
    }

    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetResTable
//  Description :
//------------------------------------------------------------------------------
static MMPF_SENSOR_RESOLUTION* MMPF_SensorDrv_GetResTable(MMP_UBYTE ubSnrSel)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return NULL;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
        return NULL;
    }

    return m_pSnrCustTable[ubSnrSel]->ResTable;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_GetSnrType
//  Description :
//------------------------------------------------------------------------------
static MMPF_VIF_SNR_TYPE MMPF_SensorDrv_GetSnrType(MMP_UBYTE ubSnrSel)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMPF_VIF_SNR_TYPE_BAYER;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
        return MMPF_VIF_SNR_TYPE_BAYER;
    }

	if (m_bCustInited[ubSnrSel])
		return m_pSnrCustTable[ubSnrSel]->VifSetting->SnrType;
	else
		return MMPF_VIF_SNR_TYPE_BAYER;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_FastDo3AOperation
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_FastDo3AOperation(MMP_UBYTE ubSnrSel, MMPF_SENSOR_FAST_3A_OPERATION sOp)
{  
#if (ISP_EN)
    ISP_UINT32 	ulCurAEGain, ulCurAEShutter, ulCurAEVsync;
    ISP_UINT32 	ulCurAWBGain[3];    
	static MMP_UBYTE    ubStatusAE;
    static MMP_UBYTE    ubStatusAWB;

	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

	if (m_pSnrCustTable[ubSnrSel] == NULL) {
    	return MMP_SENSOR_ERR_INITIALIZE;
    }

    if (m_bLinkISPSel[ubSnrSel] == MMP_FALSE) {
        return MMP_ERR_NONE;
    }

	if (MMP_IsDscHDREnable()) {
		RTNA_DBG_Str(3, "NOT CAHNGE HDR 3A\r\n");
		return MMP_ERR_NONE;
	}

    if (sOp == MMPF_SENSOR_FAST_3A_RESERVE_SETTING)
    {
#if (PS5220_DEMO_TUNE==1) || (PS5250_DEMO_TUNE==1) || (PS5270_DEMO_TUNE==1)

        #if ISP_WAIT_LUX_EN==1
        if( 1  ) {
          
          int lux ,timeout = 0 ;
          do { 
            lux = MMPF_Sensor_GetLightSensorLux();
            MMPF_OS_Sleep(1);                
          } while( (lux == -1 ) && ( timeout++ < 200 )) ;
          printc("Lux(%d),wait %d ms\r\n",lux,timeout );
        }
        #endif

        m_AE_PreGain        = 0x180;//ISP_IF_AE_GetGain();
        m_AE_PreShutter     = 0x119;//ISP_IF_AE_GetShutter();
#else
        m_AE_PreGain        = ISP_IF_AE_GetGain();
        m_AE_PreShutter     = ISP_IF_AE_GetShutter();
#endif
        m_AE_PreVsync       = ISP_IF_AE_GetVsync();
        m_AE_PreShutterBase = ISP_IF_AE_GetShutterBase();
        m_AE_PreVsyncBase   = ISP_IF_AE_GetVsyncBase();

        m_AWB_PreGain[0] 	= ISP_IF_AWB_GetGainR();
        m_AWB_PreGain[1] 	= ISP_IF_AWB_GetGainG();
        m_AWB_PreGain[2] 	= ISP_IF_AWB_GetGainB();
        
        m_usOldPreviewMode = gsCurPreviewMode[ubSnrSel];
    }
    else if (sOp == MMPF_SENSOR_FAST_3A_CHANGE_MODE) 
    {
    	/*
         * UI may switch AE/AWB status by ISP API directly and want to keep it.
         * To avoid 3A switch on again in MMPF_SensorDrv_FastDo3AOperation(),
         * we keep AE/AWB status here and restore it.
         */
        ubStatusAE  = ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AE);
        ubStatusAWB = ISP_IF_3A_GetSwitch(ISP_3A_ALGO_AWB);

        // Enter capture mode	
        //ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AE, 0);
        //ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AWB, 0);
       if(m_AE_PreVsync < (m_pSnrCustTable[ubSnrSel]->ResTable)->usVsyncLine[gsCurPreviewMode[ubSnrSel]])
       {
           m_AE_PreVsync = (m_pSnrCustTable[ubSnrSel]->ResTable)->usVsyncLine[gsCurPreviewMode[ubSnrSel]];
           m_AE_PreShutterBase = gSnrLineCntPerSec[ubSnrSel];
           m_AE_PreVsyncBase = gSnrLineCntPerSec[ubSnrSel];
       }
        
        ulCurAEGain = m_AE_PreGain
                * (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubWBinningN[gsCurPreviewMode[ubSnrSel]] 
                * (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubHBinningN[gsCurPreviewMode[ubSnrSel]] 
                * (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubWBinningM[m_usOldPreviewMode] 
                * (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubHBinningM[m_usOldPreviewMode] 
                / (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubWBinningM[gsCurPreviewMode[ubSnrSel]]
                / (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubHBinningM[gsCurPreviewMode[ubSnrSel]]  
                / (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubWBinningN[m_usOldPreviewMode]
                / (ISP_UINT32)m_pSnrCustTable[ubSnrSel]->ResTable->ubHBinningN[m_usOldPreviewMode]; 

      	#if 1
        ulCurAEShutter = m_AE_PreShutter * gSnrLineCntPerSec[ubSnrSel] / m_AE_PreShutterBase; 
        #else
        ulCurAEShutter = m_AE_PreShutter;
        #endif

        ulCurAEVsync = m_AE_PreVsync * gSnrLineCntPerSec[ubSnrSel] / m_AE_PreVsyncBase
                * (ISP_UINT32)(m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[m_usOldPreviewMode]
                / (ISP_UINT32)(m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[gsCurPreviewMode[ubSnrSel]];

        ulCurAWBGain[0] = m_AWB_PreGain[0];
        ulCurAWBGain[1] = m_AWB_PreGain[1];
        ulCurAWBGain[2] = m_AWB_PreGain[2];

        ISP_IF_AE_SetGain(ulCurAEGain, ISP_IF_AE_GetGainBase());
        ISP_IF_AE_SetShutter(ulCurAEShutter, ISP_IF_AE_GetShutterBase());
        
        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[gsCurPreviewMode[ubSnrSel]] != 0xFF) {
            SNR_SetIQSysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[gsCurPreviewMode[ubSnrSel]]);
        }
        else {
            SNR_SetIQSysMode(0); //Set as default mode
            printc("#IQ Fast 3A set Sys.Mode 0\r\n");
        }
        
        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[gsCurPreviewMode[ubSnrSel]] != 0xFF) {
            SNR_SetAESysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[gsCurPreviewMode[ubSnrSel]]);
        }
        
        gsSensorFunction->MMPF_Sensor_DoIQOperation(ubSnrSel);
        gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, ulCurAEShutter, ulCurAEVsync);
        gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, ulCurAEGain);
        #if (BIND_SENSOR_PS5220==1) || (BIND_SENSOR_PS5250==1) || (BIND_SENSOR_PS5270==1)
        // die for 60fps
        if( (m_pSnrCustTable[ubSnrSel]->ResTable)->usTargetFpsx10[gsCurPreviewMode[ubSnrSel]] <= 300 ) {
          gsSensorFunction->MMPF_Sensor_SetReg(ubSnrSel, 0x09, 0x01); //shutter & gain update, if update in frame active next 2 frames, if blanking next frame.
        }
        #endif

        ISP_IF_IQ_SetAWBGains(ulCurAWBGain[0], ulCurAWBGain[1], ulCurAWBGain[2], ISP_IF_AWB_GetGainBase());
    }
    else if (sOp == MMPF_SENSOR_FAST_3A_RESTORE_SETTING) 
    {
        ulCurAEGain 	= m_AE_PreGain; 
        ulCurAEShutter 	= m_AE_PreShutter;
        ulCurAEVsync 	= m_AE_PreVsync * gSnrLineCntPerSec[ubSnrSel] / m_AE_PreVsyncBase;
                   
        ulCurAWBGain[0]	= m_AWB_PreGain[0];
        ulCurAWBGain[1] = m_AWB_PreGain[1];
        ulCurAWBGain[2] = m_AWB_PreGain[2];
                          
        m_usOldPreviewMode = gsCurPreviewMode[ubSnrSel];

        ISP_IF_AE_SetGain(ulCurAEGain, ISP_IF_AE_GetGainBase());
        ISP_IF_AE_SetShutter(ulCurAEShutter, ISP_IF_AE_GetShutterBase());

        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[m_usOldPreviewMode] != 0xFF) {
            SNR_SetIQSysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustIQmode[m_usOldPreviewMode]);
        }
        else {
            SNR_SetIQSysMode(1);  //Set as default mode  
            //printc("#IQ Fast 3A Restore Sys.Mode 1\r\n");
        }

        if (m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[m_usOldPreviewMode] != 0xFF) {
            SNR_SetAESysMode(m_pSnrCustTable[ubSnrSel]->ResTable->ubCustAEmode[m_usOldPreviewMode]);
        }
        
        gsSensorFunction->MMPF_Sensor_DoIQOperation(ubSnrSel);
        gsSensorFunction->MMPF_Sensor_SetShutter(ubSnrSel, ulCurAEShutter, ulCurAEVsync);
        gsSensorFunction->MMPF_Sensor_SetGain(ubSnrSel, ulCurAEGain);

        ISP_IF_IQ_SetAWBGains(ulCurAWBGain[0], ulCurAWBGain[1], ulCurAWBGain[2], ISP_IF_AWB_GetGainBase());

		// Exit capture mode, restore AE/AWB status
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AE, ubStatusAE);
        ISP_IF_3A_SetSwitch(ISP_3A_ALGO_AWB, ubStatusAWB);
	}
#endif
    return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SensorDrv_LinkISPSel
//  Description :
//------------------------------------------------------------------------------
static MMP_ERR MMPF_SensorDrv_LinkISPSel(MMP_UBYTE ubSnrSel, MMP_BOOL bLinkISP)
{
	if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
		return MMP_SENSOR_ERR_INITIALIZE;
	}

    m_bLinkISPSel[ubSnrSel] = bLinkISP;
    return MMP_ERR_NONE;
}

static MMP_ERR MMPF_SensorDrv_Switch3ASpeed(MMP_UBYTE ubSnrSel, MMP_BOOL slow)
{
  if (ubSnrSel >= VIF_SENSOR_MAX_NUM) {
  	return MMP_SENSOR_ERR_INITIALIZE;
  }
  m_pSnrCustTable[ubSnrSel]->MMPF_Cust_Switch3ASpeed(slow);  
  return MMP_ERR_NONE;
}


#if 0
void ____Sensor_Function_Table____(){ruturn;} //dummy
#endif

MMPF_SENSOR_FUNCTION  m_SensorFunction =
{
	//0
	MMPF_SensorDrv_Register,
    MMPF_SensorDrv_InitCustFunc,
    MMPF_SensorDrv_InitSensor,
    MMPF_SensorDrv_InitVIF,
    MMPF_SensorDrv_InitISP,

    //5
    MMPF_SensorDrv_PowerDown,
    MMPF_SensorDrv_ChangeMode,
    MMPF_SensorDrv_SetPreviewMode,
    MMPF_SensorDrv_SetReg,
    MMPF_SensorDrv_GetReg,

    //10
    MMPF_SensorDrv_DoAWBOperation,
    MMPF_SensorDrv_DoAEOperation_ST,
    MMPF_SensorDrv_DoAEOperation_END,
    MMPF_SensorDrv_DoAFOperation,
    MMPF_SensorDrv_DoIQOperation,

    //15
    MMPF_SensorDrv_SetExposureLimit,
    MMPF_SensorDrv_SetGain,
    MMPF_SensorDrv_SetShutter,
    MMPF_SensorDrv_SetSensorFlip,
    MMPF_SensorDrv_SetSensorRotate,
    
    //20
    MMPF_SensorDrv_GetRealFPS,
    MMPF_SensorDrv_GetSensorVifPad,
    MMPF_SensorDrv_GetScalInputRes,
    MMPF_SensorDrv_GetBayerRes,
    MMPF_SensorDrv_GetTargetFpsx10,
    
    //25
    MMPF_SensorDrv_GetResTable,
    MMPF_SensorDrv_GetSnrType,
    MMPF_SensorDrv_FastDo3AOperation,
    MMPF_SensorDrv_LinkISPSel,
    MMPF_SensorDrv_Switch3ASpeed,
};

MMPF_SENSOR_FUNCTION *SensorFuncTable = &m_SensorFunction;

#endif //(SENSOR_EN)
