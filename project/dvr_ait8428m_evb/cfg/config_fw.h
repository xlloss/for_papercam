///@ait_only
/** @file config_fw.h

All customer-dependent compiler options go here.
The purpose is to prepare many versions of the file to different use cases,
such as different customers and different platforms. When compiling, simply replace this file and then compile the whole project.
This file could be used both in firmware projects.

This file should not be included in includes_fw.h.
Because the file is used to config and recompile the whole project,
it's better to be used only in the file which needs the config.
And it's highly recommended to use a value in the \#define instead of \#define only. That is
@code #define DOWNLOAD_METHOD 0 @endcode
is better than
@code #define USE_SD_DOWNLOAD_FW @endcode \n
Since the .c files might not include this file and get an unexpected result with any compiler warning.

@brief Configuration compiler options
@author Truman Yang
@since 10-aug-06
@version
- 1.0 Original version
*/

#ifndef _CONFIG_FW_H_
#define _CONFIG_FW_H_
#define PROJECT_SDK


//#define SCAL_FUNC_DBG
  #define MIPI_TEST_PARAM_EN      (0)

	#ifndef WIFI_PORT
	#define WIFI_PORT           		(0)
	#endif

#if 0
void ____System_Config____(){ruturn;} //dummy
#endif

/** @name Power On GPIO key map
@{ */

#define  AIT_EVB_GPIO_MAP   (0)
#define  QD_EVB_GPIO_MAP    (1)
#define  GPIO_MAP           AIT_EVB_GPIO_MAP

/** @name System
@{ */

    #define HANDLE_EVENT_BY_TASK    	(0)     // 1: All events handled by the event task

    #define	P_V2        				(1)
    #define MCR_V2      				(2)

    #define CHIP_CORE_ID_MCR_V2    	    (0x83)
    #define CHIP_CORE_ID_MCR_V2_LE      (0xC3)  // low end version

    // This chip name should be defined in MCP target.
    #ifndef CHIP
        #define	CHIP    (MCR_V2)
    #endif

    #define MCR_V2_UNDER_DBG    		(1)

    // For Dual-Core Chip only
    #define CPU_A       				(0)
    #define CPU_B       				(1)

    #ifndef CPU_ID
        #define CPU_ID  (CPU_A)
    #endif

    #define MCI_READ_QUEUE_4        	(1) // 1: Set 1 to support digital zoom;
                                            // 0: For project not need to support digital zoom

    #if defined(UPDATER_FW)||defined(MBOOT_FW)
    #define AUTO_DRAM_LOCKCORE          (1)
    #else
    #define AUTO_DRAM_LOCKCORE          (0)
    #endif

    #ifndef MEMORY_POOL_CTL
    #define MEMORY_POOL_CTL             (0)
    #endif
    // for TM to check dram timing
    // open most of clocks, pll set to higher one
    // 
    #define CONFIG_HW_FOR_DRAM_TIMIMG_TEST  (0)
    
    // support OTA 
    #define MTD_OTA_EN                   (1)
    // force the related dvide function, divide to constant
    #define USE_DIV_CONST                (0)
    #define USER_STRING                  (1)
    #define USER_LOG                     (1)
    #define uart_printc (1)
    #define ITCM_PUT (0)
    #define DTCM_PUT (0)

    #if (ITCM_PUT)
    #define ITCMFUNC __attribute__((section(".flashToItcmFlashSection")))
    #else
    #define ITCMFUNC
    #endif

    #if (DTCM_PUT)
    #define DTCMDATA __attribute__((section(".flashToDtcmFlashSection")))
    #else
    #define DTCMDATA
    #endif

/** @} */ // end of System

#if 0
void ____System_Clock_Config____(){ruturn;} //dummy
#endif

/** @name System Clock
@{ */

    #define EXT_CLK 					(24000) //for 24 MHz 

/** @} */ // end of System Clock

#if 0
void ____FrontCam_Config____(){ruturn;} //dummy
#endif

/** @name FrontCam
@{ */

    #define FRONT_CAM_TYPE_BAYER_SENSOR		(0) // Normal Sensor (VIF)
    #define FRONT_CAM_TYPE_YUV_SENSOR		(1) // Normal Sensor (VIF)
    #define FRONT_CAM_TYPE              	(FRONT_CAM_TYPE_BAYER_SENSOR)

/** @} */ // end of FrontCam

#if 0
void ____RearCam_Config____(){ruturn;} //dummy
#endif

/** @name RearCam
@{ */

    #define AIT_REAR_CAM_STRM_NV12_H264		(0) //Preview: NV12, Save file: H264 
    #define AIT_REAR_CAM_STRM_MJPEG_H264	(1)	//Preview: MJPEG, Save file: H264
    #define AIT_REAR_CAM_STRM_TYPE			(AIT_REAR_CAM_STRM_NV12_H264)

/** @} */ // end of RearCam

#if 0
void ____UART_Config____(){ruturn;} //dummy
#endif

/** @name Uart
@{ */

    /** @brief
    Define which UART to use, and UART padset for output
    */
    #if defined(ALL_FW)
    #define DEBUG_UART_NUM  			(MMP_UART_ID_1)
    #define DEBUG_UART_PAD  			(MMP_UART_PADSET_0)
    #else
    #define DEBUG_UART_NUM  			(MMP_UART_ID_0)
    #define DEBUG_UART_PAD  			(MMP_UART_PADSET_0)
    #endif

    // Debug Level: see @ref debug_level for more detail.
    #define DBG_LEVEL   				(0)

/** @} */ // end of Uart

#if 0
void ____FlowControl_Config____(){ruturn;} //dummy
#endif

/** @name Flow Control
@{ */

	#if defined(ALL_FW)
    #define SUPPORT_LDC_RECD			(0)
	#define SUPPORT_LDC_CAPTURE			(0)
	#else
    #define SUPPORT_LDC_RECD			(0)
	#define SUPPORT_LDC_CAPTURE			(0)
	#endif

    #define VRP_DEMO_EN					(0)		// For VRP Project Demo

	#define HDR_DBG_RAW_ENABLE			(0)
	#define HDR_FOV_ENLARGE             (0)
	
	#define TV_JAGGY_WORKAROUND			(0)
	#define TV_JAGGY_1ST_OUT_W			(1920)
	#define TV_JAGGY_1ST_OUT_H			(1080)

	#define PARALLEL_FRM_STORE_EQUIRETANGLE		(1)
	#define PARALLEL_FRM_STORE_NOT_SUPPORT		(0xFF)

	#define TVDEC_SNR_STORE_YUV420  			(0)	// 1: Store YUV420, 0: Store YUV422
	#define TVDEC_SNR_USE_DMA_DEINTERLACE 		(0)
	#define TVDEC_SNR_USE_VIF_CNT_AS_FIELD_CNT 	(0)
	#define SUPPORT_ISP_TIMESHARING				(0)
	#define SUPPORT_PARALLEL_FRAME_STORE		(0)
	#define PARALLEL_FRAME_STORE_TYPE			(PARALLEL_FRM_STORE_NOT_SUPPORT)
	#define HANDLE_LDC_EVENT_BY_TASK			(0)
	#define SUPPORT_DUAL_SNR_PCAM_OUT			(0)

  #define SUPPORT_UVC_ISP_EZMODE_FUNC   (0) // EZ IQ-tuning tool
  #define SUPPORT_UVC_JPEG              (0)

/** @} */ // end of Flow Control

#if 0
void ____MMU_Table_Config____(){ruturn;} //dummy
#endif

/** @name MMU Table address
@{ */

    #define MMU_TRANSLATION_TABLE_ADDR	(0x104000)
    #define MMU_COARSEPAGE_TABLE_ADDR	(0x104800)

    #define RSVD_HEAP_SIZE     	        (0x0)

/** @} */ // end of MMU Table address

#if 0
void ____Sensor_Config____(){ruturn;} //dummy
#endif

/** @name Sensor
@{ */
    #define TOTAL_SENSOR_NUMBER     	(1)
#if defined(SENSOR_OV2710)
    #define	BIND_SENSOR_OV2710      	(1)
#endif
#if defined(SENSOR_OV2710_MIPI)
    #define	BIND_SENSOR_OV2710_MIPI		(1)
#endif
#if defined(SENSOR_OV4689)
    #define BIND_SENSOR_OV4689			(1)
#endif
// not ciq .txt file
// #if defined(SENSOR_OV5653)
    // #define BIND_SENSOR_OV5653			(1)
// #endif
#if defined(SENSOR_OV9712)
    #define BIND_SENSOR_OV9712     	 	(1)
#endif
// #if defined(SENSOR_OV10822)
    // #define BIND_SENSOR_OV10822			(1)
// #endif
#if defined(SENSOR_IMX175)
    #define BIND_SENSOR_IMX175      	(1)
#endif
#if defined(SENSOR_IMX214)
    #define BIND_SENSOR_IMX214      	(1)
#endif
#if defined(SENSOR_IMX322)
    #define BIND_SENSOR_IMX322      	(1)
#endif
#if defined(SENSOR_IMX326)
    #define BIND_SENSOR_IMX326          (1)
#endif
#if defined(SENSOR_AR0330)
    #define BIND_SENSOR_AR0330     	 	(1)
#endif
#if defined(SENSOR_AR0330_OTPM)
    #define BIND_SENSOR_AR0330_OTPM    	(1)
#endif
#if defined(SENSOR_AR0331)
    #define BIND_SENSOR_AR0331     	 	(1)
#endif
#if defined(SENSOR_AR0835)
    #define BIND_SENSOR_AR0835			(1)
#endif
#if defined(SENSOR_AR1820)
	#define BIND_SENSOR_AR1820			(1)
#endif
#if defined(SENSOR_CP8210)
    #define BIND_SENSOR_CP8210     	 	(1)
#endif
#if defined(SENSOR_H42_MIPI)
    #define BIND_SENSOR_H42_MIPI		(1)
#endif
#if defined(SENSOR_PS1210)
    #define BIND_SENSOR_PS1210			(1)
#endif
#if defined(SENSOR_JXF02)
    #define BIND_SENSOR_JXF02			(1)
#endif
#if defined(SENSOR_JXK03)
    #define BIND_SENSOR_JXK03           (1)
#endif
#if defined(SENSOR_AR0330_2ND)
    #define BIND_SENSOR_AR0330_2ND		(1)
#endif
#if defined(SENSOR_DM5150)
    #define BIND_SENSOR_DM5150          (1)
#endif
#if defined(SENSOR_BIT1603)
    #define BIND_SENSOR_BIT1603         (1)
#endif
#if defined(SENSOR_PS5220)
    #define BIND_SENSOR_PS5220          (1)
    #define PS5220_DEMO_TUNE            (1) 
#endif
#if defined(SENSOR_PS5250)
    #define BIND_SENSOR_PS5250          (1)
    #define PS5250_DEMO_TUNE            (1)
#endif
#if defined(SENSOR_PS5270)
    #define BIND_SENSOR_PS5270          (1)
    #define PS5270_DEMO_TUNE            (1)
#endif
#if defined(SENSOR_HM2140)
    #define BIND_SENSOR_HM2140          (1) 
#endif 


/** @} */ // end of Sensor

#if 0
void ____Ext_Device_Config____(){ruturn;} //dummy
#endif

/** @name Device
@{ */
    #ifndef GYROSENSOR_CONNECT_ENABLE
    #define GYROSENSOR_CONNECT_ENABLE   (0)
    #endif
    
    #define BIND_GYROSNR_ITG2020        (1)
    #define BIND_GYROSNR_ICG20660       (0)

    #define SUPPORT_CG5162              (0)
    #define SUPPORT_LM75A               (0)
    #define SUPPORT_OPT3006             (0)
    #define SUPPORT_PAS7671             (0)

/** @} */ // end of Device

#if 0
void ____ISP_Config____(){ruturn;} //dummy
#endif

/** @name ISP
@{ */
    #define ISP_USE_TASK_DO3A       	(1)

	#define ISP_CONSUMED_PIXEL_X		(2)
	#define ISP_CONSUMED_PIXEL_Y		(2)

/** @} */ // end of ISP

#if 0
void ____Display_Config____(){ruturn;} //dummy
#endif

/** @name DSPY
@{ */
    #define SUPPORT_OSD_FUNC            (1)  

    #define LCD_IF_NONE                 (0)
    #define LCD_IF_SERIAL               (1)
    #define LCD_IF_PARALLEL             (2)
    #define LCD_IF_RGB                  (3)
    #define LCD_IF          	        (LCD_IF_NONE)
    
    #define VERTICAL_LCD                (0)
    
    #define CCIR656_OUTPUT_ENABLE       (0) /* If set 1, G0 will be set to 540MHz */
    #if (CCIR656_OUTPUT_ENABLE)
    #define CCIR656_FORCE_SEL_BT601     (1)
    #else
    #define CCIR656_FORCE_SEL_BT601     (0)
    #endif

/** @} */ // end of DSPY

#if 0
void ____HDMI_Config____(){ruturn;} //dummy
#endif

/** @name HDMI
@{ */

    #define HDMI_OUTPUT_EN          	(1)

/** @} */ // end of HDMI

#if 0
void ____Video_Play_Config____(){ruturn;} //dummy
#endif
	
/** @name Video Player
@{ */
	// Debug Video playback
    #define LOG_VID_TIME                (1 << 0)
    #define LOG_VID_BUF                 (1 << 1)
    #define LOG_PARSE                   (1 << 2)
    #define DUMP_BS_BUFDATA             (1 << 3)    
    #define DEBUG_VIDEO                 (0)
    //#define DEBUG_VIDEO                 (LOG_VID_TIME | LOG_PARSE | LOG_VID_BUF)
    #define EN_SPEED_UP_GET_FRAME       (0)

    // Use software decoder for debugging or not. The SW library have to be added.
    #define SW_DECODER 					(0x10000)
    #define HW_MJPG 					(262)                  ///< Use hardware mjpg decoder
    #define SW_MJPG 					(HW_MJPG | SW_DECODER) ///< Use software mjpg decoder
    #define HW_MP4V 					(263)                  ///< Use hardware mpeg4 decoder
    #define SW_MP4V 					(HW_MP4V | SW_DECODER) ///< Use software mpeg4 decoder
    #define HW_H264 					(264)                  ///< Use hardware h.264 decoder
    #define SW_H264 					(HW_H264 | SW_DECODER) ///< Use software h.264 decoder

	/** @brief Support Rotate with DMA feature or not 0 for support. Set 1 to support.

	Set 0 to save code size. Some customer uses 823 and would never use this feature.
	@note If there is a chip ID config, replace with it.
	This could be removed later because we have CHIP_NAME now
	*/
    #ifdef VIDEO_DECODER
    
    #if (VIDEO_DECODER == SW_H264)
        #define ROTATE_DMA_SUPPORT  	(1)
    #else        
        #define ROTATE_DMA_SUPPORT  	(1) // If using 823, turn off this option to save code size.

        #define GRAPHIC_SCALER_SUPPORT  (1)
    #endif
    
        #define VIDEO_BGPARSER_EN       (0)
        #if (VIDEO_BGPARSER_EN == 1)
        #define VIDEO_HIGHPRIORITY_FS   (1)  // using high priority tak to do file read/seek
        #define VIDEO_NONPREEMPT_FS     (1)  // guarantee task will not be interrupted while it is using fs
        #else
        #define VIDEO_HIGHPRIORITY_FS   (0)
        #define VIDEO_NONPREEMPT_FS     (0)
        #endif
    #endif //VIDEO_DECODER

    #define CHANGE_AV_SPEED         	(1)
    #define VIDEO_SPEED_FIXED_RATIO     (1)  // 1: Support 2X 4X 8X. 0: Support 1~2X 5 steps.
    #define USE_SW_RV_DEC           	(0)

    #define USE_HW_WMV_DEC          	(0)
    #define USE_HW_RV_DEC           	(0)

    #define NEW_VIDEODEC_FLOW       	(0)

	#define	CHANGE_VIDEO_SPEED_NO_GAP 	(1)
	#define VIDEO_WORKINGBUF_END    	(0x7FFFFFFF)

    #define MJPG_DEC_SUPPORT    		(1)
    #define MP4ASP_DEC_SUPPORT  		(0)
    #define MP4V_DEC_SUPPORT    		(0)
    
    #define VIDEO_DEC_TO_MJPEG			(0)
    #define VIDEO_DEC_4K2K_WORKAROUND   (1) // Use for work around the 4K2K and 2.7K support.

/** @} */ // end of Video Player

#if 0
void ____Video_Record_Config____(){ruturn;} //dummy
#endif

/** @name Video Recorder
@{ */

    #define VR_ENC_EARLY_START      	(0)

    #define VR_SINGLE_CUR_FRAME     	(0) // 1: Use only 1 current frame for video encode, not support B-frame enc.
                                        	// to enable it, must make sure the encode speed is fast than sensor frame rate

    #define SHARE_REF_GEN_BUF       	(1) // 1: Share reference buffer and generate buffer to save memory, not support B-frame enc.

    #define SUPPORT_B_FRAME_ENC     	(0) // 1: Support B-frame encode
    #if (SUPPORT_B_FRAME_ENC)
        #define MAX_B_FRAME_NUMS    	(2)
    #else
        #define MAX_B_FRAME_NUMS    	(0)
    #endif

    #if (SUPPORT_B_FRAME_ENC)&&(VR_SINGLE_CUR_FRAME)
        #error With only one reference buffer, B-frame encode is not supported
    #endif
    #if (SUPPORT_B_FRAME_ENC)&&(SHARE_REF_GEN_BUF)
        #error To support B-frame encode, the reference buffer and generate buffer can not be shared
    #endif
    
    /** @brief Optional chunks for AVI container
    */
    #define AVI_IDIT_CHUNK_EN       	(0) // 1: support IDIT chunk for creation date & time
    #define AVI_ISFT_CHUNK_EN       	(0) // 1: support ISFT chunk to identify if video is recorded by AIT
    #if (AVI_ISFT_CHUNK_EN)
        #define AVI_ISFT_DATA_STR   	"ALPHA IMAGING TECH."
    #endif

    #define SUPPORT_DVS_FUNC			(0)

	#define SUPPORT_VR_THUMBNAIL        (0)

    /** @brief 3GP muxer options
    */
    #define MUXER_3GP_FIT_VID_TIME      (1) /* Truncate audio samples to fit 
                                             * video duration in fill 3GP tail
                                             */
    /** @brief Some projects strongly request that audio duration must be longer
               than video duration for every recorded files.
    To meet this request, we always record the first audio frames with silence,
    and duplicate the silence audio bitstream in post-processing.
	*/
    #define VR_AUD_TIME_ALWAYS_LONGER   (0)
    #if (MUXER_3GP_FIT_VID_TIME)&&(VR_AUD_TIME_ALWAYS_LONGER)
        #error Want to fit video time and also want audio time longer is impossible
    #endif

    #define SUPPORT_H264_WIFI_STREAM	(0) // 1: support H264 wifi streaming

    #define FPS_CTL_EN                  (1) /* Allow to dynamically adjust
                                             * encode frame rate to achieve
                                             * slow-motion or fast-action effect
                                             */
    #define VR_MIX_FPS_WITH_AUD         (1) /* Allow to record with audio even
                                             * encode frame rate is changed
                                             * dynamically. But, sound is
                                             * available only in normal record
                                             * speed, otherwise, just silence.
                                             */
    #define WORK_AROUND_EP3             (0) /* Fix H264 IC EP3 limitation
                                             */
    #define VR_START_WITH_STILL         (1)

/** @} */ // end of Video Recorder

#if 0
void ____Storage_Config____(){ruturn;} //dummy
#endif

/** @name storage
@{ */
    #define FORMAT_FREE_ENABLE          (0) // 0 = FormatFree disable, 1 = FormatFree Enable 

    #define FS_UFS              		(0)

    #define UTF8                		(0)
    #define UCS2                		(1)

    #define FS_LIB              		(FS_UFS)
    #define FS_INPUT_ENCODE     		(UTF8)
    #define	FS_UCS2_TO_UTF8     		(0)

    /** @brief
    Define which padset of serial flash is used in the project.
    For MercuryV2, there is only one padset.
    Please refer to the schematic and misc table.
    */
    #define SF_PIN_PAD_NUM				(0)

     /* Config SD card*/
    #define EN_CARD_DETECT          	(0) // 1: enable card plug-in detect function, otherwise, don't use
    #define EN_CARD_WRITEPROTECT    	(0) // 1: enable card write protect, otherwise, don't use
    #define EN_CARD_PWRCTL          	(0) // 1: enable card power control, otherwise, don't use

    /* Enable storage driver option */
    #define	USING_SD_IF	        		(1)	// 1:enable sd driver
    #define	USING_SD1_IF	        	(0)	// 1:enable sd1 driver
    #define	USING_SM_IF	        		(0)	// 1:enable sm driver
    #define	USING_MEMORY_IF	    		(0)	// 1:enable sm driver
    #define	USING_SF_IF	        		(1)	// 1:enable sf driver
    #define	USING_SN_IF	        		(0)	// 1:enable SN driver <SPI Nand Flash><SFNAND>
    
    // Command 50H for Enable Volatile write for Status Register (0)disable , (1)enable
    //#define	SIF_VOLATILE_WRITE_SR      	(0)	
    // SPI Nor Flash transfer mode setting (0)standard (1)dual (2)quad for SPI speed
    //#define	SIF_SPI_MODE              	(0)	    
    
    // It doesn't need to modify below
    #if ((USING_SM_IF && USING_SD_IF) || (USING_MEMORY_IF && USING_SD_IF))
    #define USING_DUAL_IF       		(1)
    #else
    #define USING_DUAL_IF       		(0)
    #endif
  
/** @} */ // end of storage

#if 0
void ____DRM_Config____(){ruturn;} //dummy
#endif

/** @name DRM
@{ */
    #define MSDRM_WMA           		(0) // 0: Disable WMA with MS-DRM, 1: Enable WMA with MS-DRM
    #define MSDRM_MTP					(0) // 0: Disable MTP with MS-DRM, 1: Enable MTP with MS-DRM

    #define AIT_WMDRM_SUPPORT       	(0)
    #define AIT_DIVXDRM_SUPPORT     	(0)
/** @} */ //end of DRM

#if 0
void ____USB_Config____(){ruturn;} //dummy
#endif

/** @name USB
@{ */
    #define SUPPORT_PCCAM_FUNC      	(0) // 0: Disable PCCAM, 1: Enable PCCAM
    #define SUPPORT_PCSYNC_FUNC     	(0) // 0: Disable PCSYNC, 1: Enable PCSYNC
    #define SUPPORT_MTP_FUNC        	(0) // 0: Disable MTP, 1: Enable MTP
    #define SUPPORT_DPS_FUNC        	(0) // 0: Disable DPS, 1: Enable DPS
    
    #define MSDC_WR_FLOW_TEST_EN    	(0)
	#define SUPPORT_MSDC_SCSI_AIT_CMD_MODE		(0)

    #define MSDC_SUPPORT_AIT_CUSTOMER_SCSI_CMD	(0)
    #define MSDC_SUPPORT_SECRECY_LOCK			(0)
	
    #define USE_SEC_MTP_DEV_FW      	(0)

    #if defined(ALL_FW)
    #define SUPPORT_UVC_FUNC			(1)
    #define SUPPORT_UAC					(1)

    #define SUPPORT_USB_HOST_FUNC		(0)
    #define SUPPORT_SONIX_UVC_ISO_MODE  (0)     // 0 for AIT BULK mode, 1 for SONIX ISO mode
    #endif

    #if defined(UPDATER_FW)||defined(MBOOT_FW)||defined(MINIBOOT_FW)||defined(MBOOT_EX_FW)
    #define SUPPORT_UVC_FUNC			(0)
    #define SUPPORT_UAC					(0)
    #define SUPPORT_USB_HOST_FUNC		(0)
    #define SUPPORT_SONIX_UVC_ISO_MODE  (0)     //0 for AIT BULK mode, 1 for SONIX ISO mode
    #endif

	#define SUPPORT_UVC_MDTC			(0)		// 1: Enable UVC motion detection, 0:Disable UVC motion detection
	#define SUPPORT_SYNC_UVC_TIME		(0)		// 1: Enable sync. UVC time, 0:Disable sync. UVC time

    #define CAPTURE_BAYER_RAW_ENABLE	(0)
    #define UVC_FPS_CTL             	(0)

    #define SUPPORT_PCSYNC				(0)
    #if (SUPPORT_PCSYNC)
    	#define PCSYNC_EP_ADDR			(1)	// Endpoint 2 bulk-in/out
    	#define PCSYNC_EP_MAX_PK_SIZE	(512)
    #endif

    #define DEFAULT_VIDEO_FMT_H264		(0)  // 0: YUV/MJPEG, 1: H.264

    #define SUPPORT_AUTO_FOCUS			(0)
    #define SUPPORT_DIGITAL_ZOOM        (0)
    #define SUPPORT_DIGITAL_PAN         (0)

    #define USB_UVC_SKYPE				(1)
    #define USB_UVC_BULK_EP				(0) // must be 0 for Linux
    #define USING_STILL_METHOD_1    	(1)

    #define USB_UVC_H264         		(1) // for Logitech H264
	#define ENABLED_PCAM_2M_RES			(1)

    #if (USB_UVC_BULK_EP == 1)
    #define UVC_DMA_SIZE  	 			(0x2000)
    #define UVC_DMA_3K_NUM      		(0) // two 3K for one DMA...
    #else
    #define UVC_DMA_3K_NUM      		(1) // two 3K for one DMA...
    #endif
	#define AUDIN_CHANNEL       		(2)
	#define AUDIN_SAMPLERATE        	(16000)

    #define ENABLE_JPEG_ISR             (1)
    #define ENABLE_YUV_ISR              (1)

/** @} */ //end of USB

#if 0
void ____DSC_Config____(){ruturn;} //dummy
#endif

/** @name DSC
@{ */

    #define FDTC_AIT_SDTC				(0)	// The smile detection of AIT

    #define SUPPORT_TIMELAPSE          	(0)

    #define SUPPORT_SCALEUP_CAPTURE     (0)

    #define HANDLE_JPEG_EVENT_BY_QUEUE      (0)
    #define USE_H264_CUR_BUF_AS_CAPT_BUF    (0)

/** @} */ // end of DSC

#if 0
void ____Audio_Config____(){ruturn;} //dummy
#endif

/** @name AUDIO
@{ */
	#define AUDIO_SET_DB            	(0x1)  // Set Volume by the unit of DB

    #define	DEFAULT_DAC_DIGITAL_GAIN    (0x55) // 0db per channel (0x00~0x57) 12db max
    #define	DEFAULT_DAC_ANALOG_GAIN     (0xF5) // -4db both channel (0x00~0xFF) 6db max
    #define DEFAULT_DAC_LINEOUT_GAIN    (0x00) // 0db one channel (0x00~0x3E)  0dB   ~ -45dB
    #define	DEFAULT_ADC_DIGITAL_GAIN	(0x71) // 0x58 0db per channel (0x01~0xA1) -60dB ~  60dB
    #define	DEFAULT_ADC_ANALOG_GAIN    	(0x1F) // 31db per channel (0x00~0x1F) 0dB   ~  31db max
	
	#define ADC_MIC_BIAS_NONE      		(0)
    #define ADC_MIC_BIAS_0d65AVDD_MP	(0)
    #define ADC_MIC_BIAS_0d75AVDD_MP    (1)
    #define ADC_MIC_BIAS_0d85AVDD_MP    (2)
	#define ADC_MIC_BIAS_0d95AVDD_MP    (3)
	#define DEFAULT_ADC_MIC_BIAS_MP     ADC_MIC_BIAS_0d75AVDD_MP

    #define AUDIO_DEC_ENC_SHARE_WB  	(1)
    #define OMA_DRM_EN              	(0)
    #define WNR_EN                      (0) // Wind noise reduction
    #define NR_EN                       (0) // Noise reduction with EQ
    #define EXT_CODEC_SUPPORT       	(0)
    #define EXT_CODEC_TYPE          	(1) // 0: wm8971, 1: da7211

    #define SBC_SUPPORT             	(0) // 0: Disable SBC encode, 1: Enable SBC encode
    #define SRC_SUPPORT             	(1) // 0: Disable SRC encode, 1: Enable SRC encode
    #define VIDEO_SRC_SUPPORT       	(0) // 0: Disable SRC encode, 1: Enable SRC encode
    #define AUDIO_MIXER_EN          	(1) // 0: Disable software mixer 1: Enable software mixer
    #define PCM_TO_DNSE_EN          	(1) // 0: Disable software mixer 1: Enable software mixer
    #define PCM_ENC_SUPPORT         	(0)

    #define SEC_DNSE_VH_ON          	(0) // 0: Disable AC3 5.1CH,  1: Enable AC3 5.1CH

    #define AUDIO_STREAMING_EN      	(0)
    #define AUI_STREAMING_EN        	(0) // 1: Use streaming task to read bitstream from storage

    #define AGC_SUPPORT                 (0)
    #define HW_MP3D_EN              	(0) // 0 only, no MP3 HW decoder
    #define MP12DEC_EN              	(0) // 0: Disable MP12 decoding, 1: Enable MP12 decoding
    #define GAPLESS_EN              	(1) // 0: Disable gapless playback, 1: Enable gapless playback

    #define FOREIGN_SND_EFFECT  	    (0)
    #if (FOREIGN_SND_EFFECT == 1)
        #define MAX_EFFECT_PARAM    	(3)
    #endif

    #define BYPASS_FILTER_STAGE     	(0)
    #define DOWN_SAMPLE_TIMES       	(1)
    #define HIGH_SRATE_MODE         	(DOWN_SAMPLE_TIMES)

    #define AUDIO_PREVIEW_SUPPORT   	(0)
    #define AUDIO_CODEC_DUPLEX_EN   	(1) // 1: allow ADC & DAC to work at the same time

    #define I2S_CH0                 	(0x00)
    #define I2S_CH1                 	(0x01)
	#define I2S_CH2                     (0x02)
    #define I2S_CH_NUM              	(0x03)
    #define DEFAULT_I2S_CH          	(I2S_CH0)
    
/** @} */ // end of AUDIO

#if 0
void ____MultiCore_Config____(){ruturn;} //dummy
#endif
        
/** @name MultiCore
@{ */

    #define	CPU_SHAREMEM                (1)
    #define SUPPORT_V4L2                (1)
        #define V4L2_JPG                (1)
        #define V4L2_H264               (1)
            #define V4L2_H264_DBG       (0)
            #define V4L2_VIDCTL_DBG     (0)
        #define V4L2_AAC                (1)
            #define V4L2_AAC_DBG        (0)
        #define V4L2_GRAY               (1)
        #define V4L2_CPB_AS_LBS         (1)
    #define SUPPORT_ALSA                (1)
        #define ALSA_PWRON_PLAY                (1)
    #define SUPPORT_MDTC                (1)
        #define MD_USE_ROI_TYPE         (1) // roi type
        
    #define SUPPORT_AEC                 (1)
    #define CODE_FOR_DEMO               (0)
    #define CPUB_JTAG_DEBUG             (0)

    #define SUPPORT_IVA                 (0) // share pipe with MD
    
    #define INTERNAL_RTC                (1)
    #define EXTERNAL_RTC                (2)
    
    #define SUPPORT_RTC                 (0)//(INTERNAL_RTC) 
    #define SUPPORT_PWM                 (1)
    #define SUPPORT_ADC                 (1)
    #define SUPPORT_IRLED               (0)

/** @} */ // end of MultiCore

#if 0
void ____Misc_Config____(){ruturn;} //dummy
#endif

/** @name Misc
@{ */
    //Customer HW platform define, used in HW moudule driver

    #define ADX2002_EN              	(0x0)
    #define ADX2003_EN              	(0x0)
    #define ADX2015_EN              	(0x0)
	#define CUSTOMER_PMU_EN 			(0)

/** @} */ // end of Misc

#if 0
void ____Global_Config____(){ruturn;} //dummy
#endif

#if defined(ALL_FW)  
	#define DSC_R_EN 					(1)	
	#define DSC_P_EN 					(1)
	#define AAC_P_EN 					(0)
	#define MP3_P_EN 					(0)
	#define MP3HD_P_EN 					(0)
	#define AAC_R_EN 					(1)
	#define MP3_R_EN 					(0)
	#define WAV_R_EN 					(0)
	#define AMR_R_EN 					(0)
	#define AMR_P_EN 					(0)
	#define MIDI_EN 					(0)
	#define WMA_EN 						(0)
	#define AC3_P_EN 					(0)
	#define VAMR_R_EN 					(0)
	#define VAMR_P_EN 					(0)
	#define VAAC_R_EN 					(1)
	#define VADPCM_R_EN 				(0)
    #define VMP3_R_EN 					(0)
    #define VPCM_R_EN                   (0)
	#define VAAC_P_EN 					(0)
	#define VAC3_P_EN 					(0)
	#define USB_EN 						(0)
	#define OGG_EN 						(0)
	#define WMAPRO10_EN 				(0)
	#define RA_EN 						(0)
	#define RV_EN 						(0)
	#define VRA_P_EN 					(0)
    #define WAV_P_EN 					(0)
	#define VMP3_P_EN 					(0)
	#define VWAV_P_EN 					(0)
	#define WMV_P_EN 					(0)
	#define VWMA_P_EN 					(0)
	#define FLAC_P_EN 					(0)
	#define G711_MU_R_EN 				(0)
	#define LIVE_PCM_R_EN               (0)
	#if (DSC_R_EN)||(VAMR_R_EN)||(VAAC_R_EN)||(VADPCM_R_EN)||(VMP3_R_EN)||(VPCM_R_EN)
		#define SENSOR_EN  				(1)
	#else
		#define SENSOR_EN  				(0)
	#endif
	#if (AAC_P_EN)||(MP3_P_EN)||(MP3HD_P_EN)||(AMR_P_EN)||(MIDI_EN)||(WMA_EN)||(OGG_EN)||(WMAPRO10_EN)||(RA_EN)||(WAV_P_EN)
		#define AUDIO_P_EN 				(1) // Use for some common audio play usage. 
	#else
		#define AUDIO_P_EN 				(0) 
	#endif
	#if (AMR_R_EN)||(MP3_R_EN)||(AAC_R_EN)||(WAV_R_EN)
		#define AUDIO_R_EN 				(1) // Use for some common audio record usage. 
	#else
		#define AUDIO_R_EN 				(0) 
	#endif
	#if (AAC_R_EN)||(VAAC_R_EN)
		#define AUDIO_AAC_R_EN 			(1) // Use for some common audio AAC record usage. 
	#else
		#define AUDIO_AAC_R_EN 			(0) 
	#endif
	#if (AAC_P_EN)||(VAAC_P_EN)
		#define AUDIO_AAC_P_EN 			(1) // Use for some common audio AAC play usage. 
	#else
		#define AUDIO_AAC_P_EN 			(0)
	#endif	
    #if (MP3_R_EN)||(VMP3_R_EN)
        #define AUDIO_MP3_R_EN 			(1) // Use for some common audio MP3 record usage. 
    #else
        #define AUDIO_MP3_R_EN 			(0)
    #endif
	#if (MP3_P_EN)||(VMP3_P_EN)
		#define AUDIO_MP3_P_EN 			(1)
	#else
		#define AUDIO_MP3_P_EN 			(0)
	#endif	
	#if (AC3_P_EN)||(VAC3_P_EN)
		#define AUDIO_AC3_P_EN 			(1)
	#else
		#define AUDIO_AC3_P_EN 			(0)
	#endif
	#if (AMR_R_EN)||(VAMR_R_EN)
		#define AUDIO_AMR_R_EN 			(1) // Use for some common audio AMR record usage. 
	#else
		#define AUDIO_AMR_R_EN 			(0)
	#endif
	#if (AMR_P_EN)||(VAMR_P_EN)
		#define AUDIO_AMR_P_EN 			(1) // Use for some common audio AMR play usage. 
	#else
		#define AUDIO_AMR_P_EN 			(0)
	#endif
	#if (VAMR_R_EN)||(VAAC_R_EN)||(VADPCM_R_EN)||(VMP3_R_EN)||(VPCM_R_EN)
		#define VIDEO_R_EN 				(1) // Use for some common video record usage. 
	#else
		#define VIDEO_R_EN 				(0)
	#endif	
	#if (VAMR_P_EN)||(VAAC_P_EN)||(VMP3_P_EN)
		#define VIDEO_P_EN 				(1) // Use for some common video play usage. 
	#else
		#define VIDEO_P_EN				(0)
	#endif
#else
	#define DSC_P_EN 					(0)
	#define DSC_R_EN					(0)
	#define AAC_P_EN 					(0)
	#define MP3_P_EN 					(0)
	#define MP3HD_P_EN 					(0)
	#define AAC_R_EN 					(0)
	#define MP3_R_EN 					(0)
	#define WAV_R_EN 					(0)
	#define AMR_R_EN 					(0)
	#define AMR_P_EN					(0)
	#define MIDI_EN						(0)
	#define WMA_EN						(0)
	#define AC3_P_EN					(0)
	#define VAMR_R_EN					(0)
	#define VAMR_P_EN					(0)
	#define VAAC_R_EN 					(0)
	#define VADPCM_R_EN 				(0)
    #define VMP3_R_EN					(0)
    #define VPCM_R_EN                   (0)
	#define VAAC_P_EN					(0)
	#define VAC3_P_EN					(0)
	#define USB_EN 						(0)
	#define AUDIO_P_EN 					(0)
	#define AUDIO_R_EN					(0)
	#define AUDIO_AAC_R_EN				(0)
	#define AUDIO_AMR_R_EN				(0)
    #define AUDIO_MP3_R_EN				(0)
	#define VIDEO_R_EN 					(0)
	#define AUDIO_AMR_P_EN				(0)
	#define VIDEO_P_EN 					(0)
	#define AUDIO_AAC_P_EN				(0)
	#define SENSOR_EN 					(0)
	#define OGG_EN						(0)
	#define WMAPRO10_EN					(0)
	#define RA_EN						(0)
    #define WAV_P_EN					(0)
	#define VMP3_P_EN					(0)
    #define RV_EN						(0)
    #define VRA_P_EN					(0)
    #define VWAV_P_EN					(0)
    #define WMV_P_EN 					(0)
    #define VWMA_P_EN					(0)
    #define FLAC_P_EN					(0)
    #define G711_MU_R_EN 				(0)
	#define LIVE_PCM_R_EN               (0)
    #define AUDIO_MP3_P_EN				(0)
    #define AUDIO_AC3_P_EN				(0)
#endif

#if defined(UPDATER_FW)
    #ifdef FS_LIB
        #undef FS_LIB
        #define FS_LIB   FS_UFS
        #undef FS_UCS2_TO_UTF8
        #define	FS_UCS2_TO_UTF8     	(0)
    #endif

    #ifdef SUPPORT_MTP_FUNC
    	#undef SUPPORT_MTP_FUNC
    	#define SUPPORT_MTP_FUNC  		(0)
    #endif
    #ifdef SUPPORT_PCSYNC_FUNC
        #undef SUPPORT_PCSYNC_FUNC
        #define SUPPORT_PCSYNC_FUNC		(0)
    #endif

    #ifdef USB_EN
    	#undef USB_EN
    	#define USB_EN					(1)
    #else
    	#define USB_EN 					(1)
    #endif
#endif

#if defined(MBOOT_FW)||defined(MBOOT_EX_FW)   
    #ifdef SUPPORT_MTP_FUNC
    	#undef SUPPORT_MTP_FUNC
    	#define SUPPORT_MTP_FUNC		(0)
    #endif
    #ifdef SUPPORT_PCSYNC_FUNC
        #undef SUPPORT_PCSYNC_FUNC
        #define SUPPORT_PCSYNC_FUNC		(0)
    #endif

    #ifdef USB_EN
    	#undef USB_EN
    	#define USB_EN					(0)
    #else
    	#define USB_EN					(0)
    #endif

#endif

#if 0
void ____PLL_Config____(){ruturn;} //dummy
#endif
// Select PLL setting by sensor size
#define PLL_FOR_POWER               (0)
#define PLL_FOR_PERFORMANCE         (1)
#define PLL_FOR_BURNING             (2)
#define PLL_FOR_ULTRA_LOWPOWER_192  (3) // 2x
#define PLL_FOR_ULTRA_LOWPOWER_168  (4) // 3x


#if (BIND_SENSOR_IMX326==1)
  #define PLL_CONFIG   PLL_FOR_BURNING
#elif (BIND_SENSOR_OV4689==1)/* ||(BIND_SENSOR_JXK03==1) */
  #define PLL_CONFIG   PLL_FOR_PERFORMANCE
#elif ((BIND_SENSOR_PS5220==1)||(BIND_SENSOR_PS5250==1) ) // for low power test
  #define PLL_CONFIG  PLL_FOR_POWER
  #define PLL_FOR_ULTRA_LOWPOWER  (1) // must set this for PS5220 60fps
#else
  #define PLL_CONFIG  PLL_FOR_POWER 
#endif

#if CONFIG_HW_FOR_DRAM_TIMIMG_TEST==1
#undef PLL_CONFIG
#define PLL_CONFIG  PLL_FOR_BURNING
#endif

// Set sensor mode
#define DEFAULT_SNR_MODE  (0)


#endif	//_CONFIG_FW_H_

///@end_ait_only
