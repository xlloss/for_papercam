#ifndef _MMP_HIF_CMD_H_
#define _MMP_HIF_CMD_H_

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define FUNC_SHFT           (0)
#define GRP_SHFT            (5)
#define SUB_SHFT            (8)

#define FUNC(_f)            (_f << FUNC_SHFT)
#define GRP(_g)             (_g << GRP_SHFT)
#define SUB(_s)             (_s << SUB_SHFT)

#define FUNC_MASK           FUNC(0x1F)  //5 Bits
#define GRP_MASK            GRP(0x07)   //3 Bits
#define SUB_MASK            SUB(0xFF)   //8 Bits

#define INT_BYTE3(_v)       ( (MMP_UBYTE) (( _v >> 24 ) & 0x000000FF) )
#define INT_BYTE2(_v)       ( (MMP_UBYTE) (( _v >> 16 ) & 0x000000FF) )
#define INT_BYTE1(_v)       ( (MMP_UBYTE) (( _v >> 8  ) & 0x000000FF) )
#define INT_BYTE0(_v)       ( (MMP_UBYTE) (( _v       ) & 0x000000FF) ) 

#define INT_WORD1(_v)       ( (MMP_USHORT) (( _v >> 16 ) & 0x0000FFFF))
#define INT_WORD0(_v)       ( (MMP_USHORT) (( _v       ) & 0x0000FFFF))

#define SHORT_BYTE1(_v)     ( (MMP_UBYTE) (( _v >> 8 ) & 0x00FF) )
#define SHORT_BYTE0(_v)     ( (MMP_UBYTE) (( _v      ) & 0x00FF) )

/////////////////////////////////////////////////////////
//                    //              //               //
//      SUB(8Bit)     //  GRP(3Bit)   //  FUNC(5Bit)   //
//                    //              //               //
/////////////////////////////////////////////////////////

#define GRP_SYS             GRP(0x00)
#define GRP_FLOWCTL         GRP(0x01)
#define GRP_DSC             GRP(0x02)
#define GRP_VID             GRP(0x03)
#define GRP_AUD             GRP(0x04)
#define GRP_USB             GRP(0x05)
#define GRP_USR             GRP(0x06)

typedef enum _HIF_GRP_IDX
{
    GRP_IDX_SYS     = 0,
	GRP_IDX_FLOWCTL,
	GRP_IDX_DSC,
	GRP_IDX_VID,
	GRP_IDX_NUM
} HIF_GRP_IDX;

#define	MAX_HIF_ARRAY_SIZE	(6) // Unit: int

//==============================================================================
//
//                              MACRO DEFINE : COMMAND
//
//==============================================================================

/* System */
#define HIF_SYS_CMD_VERSION_CONTROL         (GRP_SYS | FUNC(0x01))
#define     FW_RELEASE_TIME                         SUB(0x02)
#define     FW_RELEASE_DATE                         SUB(0x03)
#define HIF_SYS_CMD_SET_BYPASS_MODE         (GRP_SYS | FUNC(0x02))
#define     ENTER_BYPASS_MODE                       SUB(0x01)
#define     EXIT_BYPASS_MODE                        SUB(0x02)
#define HIF_SYS_CMD_SET_PS_MODE             (GRP_SYS | FUNC(0x03))
#define     ENTER_PS_MODE                           SUB(0x01)
#define     EXIT_PS_MODE                            SUB(0x02)
#define HIF_SYS_CMD_SELF_SLEEP              (GRP_SYS | FUNC(0x04))
#define     SYSTEM_ENTER_SLEEP                      SUB(0x01)
#define     SET_WAKE_EVENT                        	SUB(0x02)
#define HIF_SYS_CMD_SYSTEM_RESET       		(GRP_SYS | FUNC(0x05))
#define     SYSTEM_SOFTWARE_RESET                   SUB(0x01)
#define HIF_SYS_CMD_CONFIG_TV               (GRP_SYS | FUNC(0x07))
#define     TV_COLORBAR                             SUB(0x01)
#define     TVENC_INIT                              SUB(0x02)
#define     TVENC_UNINIT                            SUB(0x03)
#define	HIF_SYS_CMD_GET_FW_ADDR				(GRP_SYS | FUNC(0x09))
#define     FW_END		                            SUB(0x01)
#define     FW_SRAM_END                             SUB(0x02)
#define     AUDIO_START                             SUB(0x03) //TBD
#define HIF_SYS_CMD_ECHO             		(GRP_SYS | FUNC(0x0A))
#define     GET_INTERNAL_STATUS                     SUB(0x01)
#define HIF_SYS_CMD_GET_GROUP_FREQ          (GRP_SYS | FUNC(0x0B))
#define HIF_SYS_CMD_GET_GROUP_SRC_AND_DIV   (GRP_SYS | FUNC(0x0C))
#define HIF_SYS_CMD_MEMORY_CONTROL          (GRP_SYS | FUNC(0x0D))
#define     SET_POOL_HANDLER                        SUB(0x01)
#define     GET_POOL_HANDLER                        SUB(0x02)
#define HIF_SYS_CMD_SET_AUD_FRACT           (GRP_SYS | FUNC(0x0E))

/* FlowControl : Sensor */
#define HIF_FCTL_CMD_INIT_SENSOR            (GRP_FLOWCTL | FUNC(0x00))
#define HIF_FCTL_CMD_SENSOR_CONTROL         (GRP_FLOWCTL | FUNC(0x01))
#define     SET_REGISTER                            SUB(0x80)
#define     GET_REGISTER                            SUB(0x00)
#define HIF_FCTL_CMD_SET_SENSOR_RES_MODE 	(GRP_FLOWCTL | FUNC(0x02))
#define     SENSOR_PREVIEW_MODE            			SUB(0x00)
#define     SENSOR_CAPTURE_MODE                  	SUB(0x01)
#define HIF_FCTL_CMD_SET_SENSOR_MODE        (GRP_FLOWCTL | FUNC(0x03))
#define HIF_FCTL_CMD_SET_SENSOR_FLIP        (GRP_FLOWCTL | FUNC(0x04))
#define HIF_FCTL_CMD_SET_SENSOR_ROTATE      (GRP_FLOWCTL | FUNC(0x05))
#define HIF_FCTL_CMD_POWERDOWN_SENSOR       (GRP_FLOWCTL | FUNC(0x06))
#define HIF_FCTL_CMD_GET_SENSOR_DRV_PARAM   (GRP_FLOWCTL | FUNC(0x07))
#define HIF_FCTL_CMD_CHANGE_SENSOR_RES_MODE (GRP_FLOWCTL | FUNC(0x08))

/* FlowControl : ISP/IQ */
#define HIF_FCTL_CMD_3A_FUNCTION            (GRP_FLOWCTL | FUNC(0x10))
#define     SET_HW_BUFFER                           SUB(0x01)
#define     GET_HW_BUFFER_SIZE                      SUB(0x02)
#define HIF_FCTL_CMD_AE_FUNC                (GRP_FLOWCTL | FUNC(0x11))
#define 	SET_PREV_GAIN						    SUB(0x01)
#define		SET_CAP_GAIN						    SUB(0x02)
#define     SET_PREV_EXP_LIMIT					    SUB(0x03)
#define		SET_CAP_EXP_LIMIT						SUB(0x04)
#define		SET_PREV_SHUTTER				        SUB(0x05)
#define		SET_CAP_SHUTTER							SUB(0x06)
#define 	SET_PREV_EXP_VAL						SUB(0x07)
#define		SET_CAP_EXP_VAL							SUB(0x08)

/* FlowControl : Preview,Misc */
#define HIF_FCTL_CMD_SET_PREVIEW_ENABLE     (GRP_FLOWCTL | FUNC(0x12))
#define     ENABLE_PREVIEW                          SUB(0x01)
#define     DISABLE_PREVIEW                         SUB(0x02)
#define     ENABLE_PIPE                             SUB(0x03)
#define     DISABLE_PIPE                            SUB(0x04)
#define HIF_FCTL_CMD_SET_ROTATE_BUF         (GRP_FLOWCTL | FUNC(0x13))
#define HIF_FCTL_CMD_GET_PREVIEW_BUF        (GRP_FLOWCTL | FUNC(0x14))
#define HIF_FCTL_CMD_SET_PREVIEW_BUF        (GRP_FLOWCTL | FUNC(0x15))
#define     BUFFER_ADDRESS                          SUB(0x00)
#define     BUFFER_COUNT                            SUB(0x01)
#define     BUFFER_WIDTH                            SUB(0x02)
#define     BUFFER_HEIGHT                           SUB(0x03)
#define HIF_FCTL_CMD_SET_IBC_LINK_MODE      (GRP_FLOWCTL | FUNC(0x16))
#define     LINK_NONE                               SUB(0x00)
#define     LINK_DISPLAY                            SUB(0x01)
#define     LINK_VIDEO                              SUB(0x02)
#define     LINK_DMA                                SUB(0x03)
#define     LINK_GRAY                               SUB(0x04)
#define     UNLINK_GRAY                             SUB(0x05)
#define     LINK_GRAPHIC                            SUB(0x06)
#define     UNLINK_GRAPHIC                          SUB(0x07)
#define     LINK_USB                                SUB(0x08)
#define     UNLINK_USB                              SUB(0x09)
#define     LINK_LDC                               	SUB(0x0A)
#define 	UNLINK_LDC								SUB(0x0B)
#define     LINK_MDTC                               SUB(0x0C)
#define 	UNLINK_MDTC								SUB(0x0D)
#define     LINK_LDWS                               SUB(0x0E)
#define 	UNLINK_LDWS							    SUB(0x0F)
#define     LINK_GRA2MJEPG                          SUB(0x10)
#define     UNLINK_GRA2MJPEG                        SUB(0x11)
#define     LINK_GRA2STILLJPEG                      SUB(0x12)
#define     UNLINK_GRA2STILLJPEG                    SUB(0x13)
#define     LINK_WIFI                               SUB(0x14)
#define     UNLINK_WIFI                             SUB(0x15)
#define     LINK_GRA2UVC                          	SUB(0x16)
#define     UNLINK_GRA2UVC                        	SUB(0x17)
#define     GET_IBC_LINK_ATTR                       SUB(0x18)
#define     LINK_IVA                                SUB(0x19)
#define     UNLINK_IVA                              SUB(0x1A)


#define HIF_FCTL_CMD_SET_RAW_PREVIEW        (GRP_FLOWCTL | FUNC(0x17))
#define 	ENABLE_RAW_PATH						    SUB(0x00)
#define     ENABLE_RAWPREVIEW                       SUB(0x01)
#define     SET_RAWSTORE_ADDR                     	SUB(0x02)
#define     SET_RAWSTORE_UADDR                     	SUB(0x03)
#define     SET_RAWSTORE_VADDR                     	SUB(0x04)
#define 	SET_RAWSTORE_ENDADDR					SUB(0x05)
#define 	SET_RAWSTORE_ENDUADDR					SUB(0x06)
#define 	SET_RAWSTORE_ENDVADDR					SUB(0x07)
#define     SET_FETCH_RANGE                         SUB(0x08)
#define     SET_RAWSTORE_ONLY                       SUB(0x09)
#define HIF_FCTL_CMD_SET_PIPE_LINKED_SNR	(GRP_FLOWCTL | FUNC(0x18))

/* FlowControl : IVA */
#define HIF_FCTL_CMD_FDTC		           	(GRP_FLOWCTL | FUNC(0x19))
#define 	SET_FDTC_BUFFER							SUB(0x00)
#define     ACTIVATE_BUFFER							SUB(0x01)
#define     INIT_FDTC_CONFIG                      	SUB(0x02)
#define     SET_DISPLAY_INFO                        SUB(0x03)
#define     GET_FD_WORK_BUF_SIZE					SUB(0x04)
#define     GET_FD_INFO_BUF_SIZE					SUB(0x05)
#define     GET_SD_WORK_BUF_SIZE					SUB(0x06)
#define     GET_FD_SPEC                             SUB(0x07)
#define HIF_FCTL_CMD_VMD                    (GRP_FLOWCTL | FUNC(0x1A))
#define     INIT_VMD                                SUB(0x00)
#define     SET_VMD_BUF                             SUB(0x01)
#define     GET_VMD_RESOL                           SUB(0x02)
#define     REG_VMD_CALLBACK                        SUB(0x03)
#define     ENABLE_VMD                              SUB(0x04)
#define HIF_FCTL_CMD_ADAS                   (GRP_FLOWCTL | FUNC(0x1B))
#define     INIT_ADAS                               SUB(0x00)
#define		SET_ADAS_BUF							SUB(0x01)
#define		GET_ADAS_RESOL							SUB(0x02)
#define     CTL_ADAS_MODE                           SUB(0x03)
#define		ENABLE_ADAS								SUB(0x04)

/* DSC */
#define HIF_DSC_CMD_TAKE_JPEG               (GRP_DSC | FUNC(0x01))
#define		ONE_PIPE								SUB(0x01)
#define		TWO_PIPE								SUB(0x02)
#define HIF_DSC_CMD_TAKE_NEXT_JPEG          (GRP_DSC | FUNC(0x02))
#define HIF_DSC_CMD_TAKE_GRA_JPEG           (GRP_DSC | FUNC(0x03))
#define HIF_DSC_CMD_SET_JPEG_ATTR           (GRP_DSC | FUNC(0x04))
#define     SET_QSCALE_CTRL                      	SUB(0x01)
#define     SET_JPEG_RESOL                      	SUB(0x02)
#define 	SET_CONTI_SHOT_PARAM					SUB(0x03)
#define 	SET_SCALE_UP_CAPTURE					SUB(0x04)
#define     SET_QSCALE_CTRL_EX                  	SUB(0x05)
#define HIF_DSC_CMD_SET_JPEG_MEDIA          (GRP_DSC | FUNC(0x05))
#define     JPEG_FILE_NAME                          SUB(0x01)
#define HIF_DSC_CMD_DECODE_JPEG             (GRP_DSC | FUNC(0x06))
#define     GET_JPEG_INFO                           SUB(0x00)
#define     DECODE_JPEG_FILE                        SUB(0x01)
#define     GET_EXIF_INFO                           SUB(0x02)
#define     OPEN_EXIF_FILE                          SUB(0x03)
#define     CLOSE_EXIF_FILE                         SUB(0x04)
#define     SET_EXIF_WORKINGBUF                     SUB(0x05)
#define     UPDATE_EXIF_NODE                      	SUB(0x06)
#define     GET_JPG_AND_EXIF_INFO                   SUB(0x07)
#define     SET_DECODE_JPG_OFFSET                   SUB(0x08)
#define     OPEN_JPEG_FILE                          SUB(0x09)
#define     CLOSE_JPEG_FILE                         SUB(0x0A)
#define 	RESET_WORKING_BUF						SUB(0x0B)
#define 	SET_DATA_INPUT_BUF						SUB(0x0C)
#define 	DECODE_JPEG_IN_MEM						SUB(0x0D)
#define HIF_DSC_CMD_SET_IMAGE_PATH          (GRP_DSC | FUNC(0x07))
#define     CAPTURE_PATH                            SUB(0x01)
#define     DISPLAY_PATH                            SUB(0x02)
#define HIF_DSC_CMD_CARD_MODE_EXIF   		(GRP_DSC | FUNC(0x09))
#define		EXIF_INITIALIZE							SUB(0x01)
#define		EXIF_FINISH								SUB(0x02)
#define     EXIF_ENC                      			SUB(0x03)
#define     EXIF_DEC                      			SUB(0x04)
#define HIF_DSC_CMD_JPEG_SAVE_CARD        	(GRP_DSC | FUNC(0x0A))
#define HIF_DSC_CMD_SET_ROTATE_CAPTURE      (GRP_DSC | FUNC(0x0B))
#define HIF_DSC_CMD_JPEG_STABLESHOT         (GRP_DSC | FUNC(0x0C))
#define     GET_STABLESHOT_INFO                     SUB(0x01) 
#define     SET_STABLESHOT_PARAM            		SUB(0x02)
#define	HIF_DSC_CMD_RESTORE_PREVIEW			(GRP_DSC | FUNC(0x0D))
#define	HIF_DSC_CMD_SET_BUFFER		        (GRP_DSC | FUNC(0x0E))
#define     SET_ENC_BUF                             SUB(0x01)
#define     SET_DEC_BUF                             SUB(0x02)
#define     CHANGE_EXT_BUF                          SUB(0x03)
#define     SET_RING_BUF                      		SUB(0x04)
#define	HIF_DSC_RAW_OPERATION		        (GRP_DSC | FUNC(0x0F))
#define     GET_RAW_DATA                     		SUB(0x01)
#define	HIF_DSC_MJPEG_OPERATION		        (GRP_DSC | FUNC(0x10))
#define		START_MJPEG								SUB(0x01)
#define		STOP_MJPEG								SUB(0x02)
#define 	SET_FPS									SUB(0x03)
#define 	SET_VID2MJPEG_ENABLE					SUB(0x04)
#define     STREAM_JPEG_INFO                        SUB(0x05)
#define		GET_STREAM_STATUS						SUB(0x06)
#define HIF_DSC_CMD_CARD_MODE_MPF   		(GRP_DSC | FUNC(0x11))
#define		MPF_ENABLE								SUB(0x01)
#define		MPF_INITIALIZE							SUB(0x02)
#define		MPF_FINISH								SUB(0x03)
#define		MPF_UPDATE_TAG							SUB(0x04)
#define		MPF_UPDATE_ENTRY						SUB(0x05)
#define HIF_DSC_CMD_TAKE_H264_JPEG          (GRP_DSC | FUNC(0x12))
#define HIF_DSC_CMD_TAKE_LDC_JPEG          	(GRP_DSC | FUNC(0x13))

/* Video Encode */
#define HIF_VID_CMD_MERGER_PARAMETER        (GRP_VID | FUNC(0x0C))
#define     AV_TIME_LIMIT                           SUB(0x02)
#define     AV_FILE_LIMIT                           SUB(0x03)
#define     AUDIO_ENCODE_CTL                        SUB(0x04)
#define     GET_3GP_FILE_SIZE                       SUB(0x09)
#define     GET_3GP_DATA_RECODE                     SUB(0x0A)
#define     GET_3GP_FILE_STATUS                     SUB(0x0B)
#define     GET_MERGER_STATUS                       SUB(0x0C)
#define     ENCODE_STORAGE_PATH                     SUB(0x0D)
#define     ENCODE_FILE_NAME                        SUB(0x0E)
#define     AUDIO_AAC_MODE                          SUB(0x0F)
#define     AUDIO_AMR_MODE                          SUB(0x10)
#define     GET_RECORDING_TIME                      SUB(0x12)
#define     GET_RECORDING_FRAME                     SUB(0x13)
#define     VIDEO_BUFFER_THRESHOLD                  SUB(0x14)
#define     SET_CONTAINER_TYPE                      SUB(0x15)
#define		GOP_FRAME_TYPE                          SUB(0x16)
#define     AUDIO_ADPCM_MODE                        SUB(0x18)
#define     AUDIO_MP3_MODE                          SUB(0x19)
#define     SKIP_THRESHOLD                          SUB(0x1A)
#define     SEAMLESS_MODE                           SUB(0x1B)
#define     MAKE_EXTRA_ROOM                         SUB(0x1C)
#define     AV_TIME_DYNALIMIT                       SUB(0x1D)
#define     EMERGFILE_TIME_LIMIT                    SUB(0x1E)
#define     SET_EMERGENT_PREENCTIME                 SUB(0x1F)
#define     GET_RECORDING_OFFSET                    SUB(0x20)
#define     SET_USER_DATA_ATOM                      SUB(0x21)
#define     SET_REFIXRECD_BUFFER                    SUB(0x22)
#define     SET_3GPMUX_TIMEATOM                     SUB(0x23)
#define     GET_EMERG_REC_STATUS                    SUB(0x24)
#define     GET_RECORD_DURATION                     SUB(0x25)
#define     AUDIO_PCM_MODE                          SUB(0x26)
#define     MODIFY_AVI_LIST_ATOM                    SUB(0x27)
#define     GET_COMPBUF_ADDR                        SUB(0x28)
#define HIF_VID_CMD_MERGER_OPERATION        (GRP_VID | FUNC(0x0D))
#define     MERGER_START                            SUB(0x01)
#define     MERGER_STOP                             SUB(0x02)
#define     MERGER_PAUSE                            SUB(0x03)
#define     MERGER_RESUME                           SUB(0x04)
#define     MERGER_PRECAPTURE                       SUB(0x05)
#define     MERGER_EMERGSTART                       SUB(0x06)
#define     MERGER_EMERGENABLE                      SUB(0x07)
#define     MERGER_EMERGSTOP                        SUB(0x08)
#define     MERGER_SKIPMODE_ENABLE                  SUB(0x09)
#define     MERGER_UVCRECD_START					SUB(0x0A)
#define     MERGER_UVCRECD_STOP						SUB(0x0B)
#define     MERGER_UVCRECD_INPUTFRAME               SUB(0x0C)
#define     MERGER_MULTIENC_START					SUB(0x0D)
#define     MERGER_ENCODE_FORMAT					SUB(0x0F)
#define     MERGER_ENCODE_RESOLUTION                SUB(0x10)
#define     MERGER_ENCODE_FRAMERATE					SUB(0x11)
#define     MERGER_ENCODE_GOP						SUB(0x12)
#define     MERGER_ENCODE_SPSPPSHDR                 SUB(0x13)
#define     MERGER_SET_THUMB_INFO                   SUB(0x14)
#define     MERGER_SET_THUMB_BUF                    SUB(0x15)
#define     MERGER_SEI_MODE                         SUB(0x16)
#define     MERGER_REFIX_VIDRECD                    SUB(0x18)
#define     MERGER_UVCEMERG_ENABLE                  SUB(0x19)
#define     MERGER_MULTISTREAM_USEMODE              SUB(0x1A)
#define HIF_VID_CMD_MERGER_TAILSPEEDMODE    (GRP_VID | FUNC(0x0E))
#define HIF_VID_CMD_RECD_PARAMETER          (GRP_VID | FUNC(0x0F))
#define     ENCODE_RESOLUTION                       SUB(0x01)
#define     ENCODE_CROPPING                         SUB(0x02)
#define     ENCODE_PADDING                          SUB(0x03)
#define     ENCODE_GOP                              SUB(0x04)
#define     SNR_FRAME_RATE                          SUB(0x05)
#define		ENCODE_FRAME_RATE                       SUB(0x06)
#define		ENCODE_FRAME_RATE_UPD                   SUB(0x07)
#define     ENCODE_PROFILE                          SUB(0x08)
#define     ENCODE_LEVEL                            SUB(0x09)
#define     ENCODE_ENTROPY                          SUB(0x0A)
#define     ENCODE_BITRATE                          SUB(0x0B)
#define     SET_QP_INIT                             SUB(0x0C)
#define     SET_QP_BOUNDARY                         SUB(0x0D)
#define     ENCODE_FORCE_I                          SUB(0x0E)
#define		ENCODE_RC_MODE                          SUB(0x0F)
#define		ENCODE_RC_SKIP                          SUB(0x10)
#define		ENCODE_RC_SKIPTYPE                      SUB(0x11)
#define		ENCODE_RC_LBS                           SUB(0x12)
#define     ENCODE_TNR_EN                           SUB(0x13)
#define     ENCODE_CURBUFMODE                       SUB(0x14)
#define     ENCODE_BSBUF                            SUB(0x15)
#define     ENCODE_MISCBUF                          SUB(0x16)
#define     ENCODE_REFGENBD                         SUB(0x17)
#define     ENCODE_BYTE_COUNT                       SUB(0x18)
#define     VIDRECD_STATUS                          SUB(0x19)
#define     ENCODE_CAPABILITY                       SUB(0x1A)
#define     ENCODE_BITRATE_TARGET                   SUB(0x1B)

#define HIF_VID_CMD_RECD_OPERATION          (GRP_VID | FUNC(0x10))
#define     VIDEO_RECORD_START                      SUB(0x01)
#define     VIDEO_RECORD_PAUSE                      SUB(0x02)
#define     VIDEO_RECORD_RESUME                     SUB(0x03)
#define     VIDEO_RECORD_STOP                       SUB(0x04)
#define     VIDEO_RECORD_PREENCODE                  SUB(0x05)
#define     VIDEO_RECORD_NONE                       SUB(0x06)

/* Audio */
#define HIF_AUD_CMD_MIDI_DEC_OP             (GRP_AUD | FUNC(0x00))
#define     DECODE_OP_PLAY_NOTE         			SUB(0x10)
#define     DECODE_OP_STOP_NOTE         			SUB(0x11)
#define     DECODE_OP_SET_PLAYTIME      			SUB(0x12)
#define     DECODE_OP_GET_PLAYTIME      			SUB(0x13)
#define HIF_AUD_CMD_DEC_OP                  (GRP_AUD | FUNC(0x01))
#define     DECODE_OP_START             			SUB(0x01)
#define     DECODE_OP_PAUSE             			SUB(0x02)
#define     DECODE_OP_RESUME            			SUB(0x03)
#define     DECODE_OP_STOP              			SUB(0x04)
#define     DECODE_OP_GET_FILEINFO      			SUB(0x05)
#define     DECODE_OP_PREDECODE         			SUB(0x06)
#define     DECODE_OP_GET_MP3FILEINFO               SUB(0x07)
#define     DECODE_OP_PREVIEW                       SUB(0x08)
#define HIF_AUD_CMD_SBC_ENCODE              (GRP_AUD | FUNC(0x03))
#define     SET_SBC_OUTPUT_BUF              		SUB(0x01)
#define     SET_SBC_ENCODE_ENABLE           		SUB(0x02)
#define     SET_SBC_CHANNEL_MODE            		SUB(0x03)
#define     SET_SBC_ALLOC_METHOD            		SUB(0x04)
#define     SET_SBC_SAMPLE_FREQ             		SUB(0x05)
#define     SET_SBC_NROF_BLOCKS             		SUB(0x06)
#define     SET_SBC_NROF_SUBBANDS           		SUB(0x07)
#define     SET_SBC_BITRATE                 		SUB(0x08)
#define     SET_SBC_HANDSHAKE_BUF           		SUB(0x09)
#define     GET_SBC_INFO                    		SUB(0x0A)
#define     SET_SBC_BITPOOL                         SUB(0x0C)
#define     GET_SBC_BITPOOL                         SUB(0x0D)
#define HIF_AUD_CMD_RA_DEC_OP               (GRP_AUD | FUNC(0x04))
#define     DECODE_OP_RA_SET_STARTTIME     	        SUB(0x11)
#define HIF_AUD_CMD_DECODE_PARAM            (GRP_AUD | FUNC(0x05))
#define     AUDIO_GET_DECODE_PCM_BUF                SUB(0x01)
#define     AUDIO_DECODE_BUF            			SUB(0x02)
#define     AUDIO_DECODE_FILE_NAME      			SUB(0x03)
#define     AUDIO_DECODE_PATH           			SUB(0x04)
#define     AUDIO_DECODE_GET_FILE_SIZE  			SUB(0x05)
#define     AUDIO_DECODE_SET_FILE_SIZE				SUB(0x06)
#define     AUDIO_DECODE_HANDSHAKE_BUF				SUB(0x07)
#define     AUDIO_DECODE_SPECTRUM_BUF				SUB(0x08)
#define     AUDIO_DECODE_FORMAT						SUB(0x09)
#define     AUDIO_DECODE_INT_THRESHOLD				SUB(0x0A)
#define     AUDIO_DECODE_DAC_PATH					SUB(0x0B)
#define     AUDIO_DECODE_SET_CALLBACK				SUB(0x0D)
#define     AUDIO_PRESET_FILE_NAME                  SUB(0x0F)
#define     AUDIO_PARSE_BUF                         SUB(0x12)
#define     AUDIO_PARSE_FILE_NAME                   SUB(0x13)
#define     AUDIO_PARSE_PATH                        SUB(0x14)
#define     AUDIO_PARSE_FORMAT                      SUB(0x19)
#define HIF_AUD_CMD_AUDIO_EQ                (GRP_AUD | FUNC(0x06))
#define     AUDIO_EQ_NONE							SUB(0x00)
#define     AUDIO_EQ_CLASSIC						SUB(0x01)
#define     AUDIO_EQ_JAZZ							SUB(0x02)
#define     AUDIO_EQ_POP							SUB(0x03)
#define     AUDIO_EQ_ROCK							SUB(0x04)
#define     AUDIO_EQ_BASS3							SUB(0x05)
#define     AUDIO_EQ_BASS9							SUB(0x06)
#define     AUDIO_EQ_LOUDNESS						SUB(0x07)
#define     AUDIO_EQ_SPK							SUB(0x08)
#define     AUDIO_EQ_VIRTUALBASS					SUB(0x09)
#define     AUDIO_HP_SURROUND           			SUB(0x10)
#define     AUDIO_SPK_SURROUND          			SUB(0x20)
#define     AUDIO_GRAPHIC_EQ                        SUB(0x30)
#define     AUDIO_ADJUST_EQ_BAND        			SUB(0x40)
#define     SET_FOREIGN_EQ                          SUB(0x50)
#define     ADJUST_FOREIGN_EQ                       SUB(0x60)
#define		SET_OAEP_PARAMETER 					    SUB(0x80)
#define HIF_AUD_CMD_SET_VOLUME              (GRP_AUD | FUNC(0x07))
#define     AUDIO_PLAY_VOLUME                       SUB(0x01)
#define     AUDIO_RECORD_VOLUME            	        SUB(0x02)
#define		AUDIO_PLAY_DIGITAL_GAIN					SUB(0x03)
#define		AUDIO_PLAY_ANALOG_GAIN					SUB(0x04)
#define		AUDIO_RECORD_DIGITAL_GAIN				SUB(0x05)
#define		AUDIO_RECORD_ANALOG_GAIN				SUB(0x06)
#define     AUDIO_RECORD_ENABLE_DUMMY				SUB(0x07)
#define     AUDIO_RECORD_HEAD_MUTE					SUB(0x08)
#define     AUDIO_RECORD_TAIL_CUT					SUB(0x09)
#define     AUDIO_RECORD_SILENCE                    SUB(0x0A)
#define HIF_AUD_CMD_WMA_DEC_OP              (GRP_AUD | FUNC(0x08))
#define     DECODE_OP_EXTRACT_TAG       			SUB(0x10)
#define     DECODE_OP_SET_STARTTIME     			SUB(0x11)
#define     DECODE_OP_GET_VERSION	     			SUB(0x12)
#define HIF_AUD_CMD_PLAY_POS                (GRP_AUD | FUNC(0x09))
#define     SET_PLAY_POS                			SUB(0x01)
#define     GET_PLAY_POS                			SUB(0x02)
#define     SET_PLAY_TIME               			SUB(0x03)
#define     GET_PLAY_TIME               			SUB(0x04)
#define HIF_AUD_CMD_AB_REPEAT_MODE          (GRP_AUD | FUNC(0x0A))
#define     ABREPEAT_MODE_ENABLE        			SUB(0x01)
#define     ABREPEAT_MODE_DISABLE       			SUB(0x02)
#define     ABREPEAT_SET_POINTA         			SUB(0x03)
#define     ABREPEAT_SET_POINTB                     SUB(0x04)
#define HIF_AUD_CMD_AUDIO_SPECTRUM          (GRP_AUD | FUNC(0x0B))
#define HIF_AUD_CMD_DECODE_STATUS           (GRP_AUD | FUNC(0x0C))
#define     PLAY_STATUS                             SUB(0x01)
#define     PREVIEW_STATUS                          SUB(0x02)
#define HIF_AUD_CMD_WRITE_STATUS            (GRP_AUD | FUNC(0x0D))
#define HIF_AUD_CMD_KARAOK		            (GRP_AUD | FUNC(0x0E))
#define 	KARAOK_VOCAL_REMOVE      				SUB(0x01)
#define 	KARAOK_MIC_VOLUME						SUB(0x02)    
#define 	KARAOK_SUPPORT    						SUB(0x03)    
#define HIF_AUD_CMD_MP3_CODE_OP			    (GRP_AUD | FUNC(0x0F))
#define		FLUSH_MP3_ZI_REGION						SUB(0x01)	
#define		INIT_MP3_ZI_REGION						SUB(0x02)	
#define		FLUSH_AUDIO_ZI_REGION					SUB(0x03)	
#define		INIT_AUDIO_ZI_REGION					SUB(0x04)	
#define		DUMMY_CODE_LINK1						SUB(0x05)	
#define		DUMMY_CODE_LINK2						SUB(0x06)
#define HIF_AUD_CMD_GAPLESS                 (GRP_AUD | FUNC(0x10))
#define     GAPLESS_SET_MODE                        SUB(0x01)
#define     GET_ALLOWED_OP                          SUB(0x02)
#define HIF_AUD_CMD_ENC_OP          	    (GRP_AUD | FUNC(0x11))
#define     ENCODE_OP_START             			SUB(0x01)
#define     ENCODE_OP_PAUSE             			SUB(0x02)
#define     ENCODE_OP_RESUME            			SUB(0x03)
#define     ENCODE_OP_STOP              			SUB(0x04)
#define     ENCODE_OP_GET_FILESIZE      			SUB(0x05)
#define     ENCODE_OP_PREENCODE         			SUB(0x06)
#define HIF_AUD_CMD_ENCODE_PARAM            (GRP_AUD | FUNC(0x12))
#define     AUDIO_ENCODE_MODE           			SUB(0x01)
#define     AUDIO_ENCODE_BUF            			SUB(0x02)
#define     AUDIO_ENCODE_FILE_LIMIT     			SUB(0x03)
#define     AUDIO_ENCODE_PATH           			SUB(0x04)
#define     AUDIO_ENCODE_FILE_NAME      			SUB(0x05)
#define     AUDIO_ENCODE_HANDSHAKE_BUF  			SUB(0x06)
#define     AUDIO_ENCODE_INT_THRESHOLD  			SUB(0x07)
#define     AUDIO_ENCODE_FORMAT         			SUB(0x09)
#define     AUDIO_ENCODE_ADC_PATH					SUB(0x0A)
#define		AUDIO_ENCODE_LINEIN_CHANNEL				SUB(0x0B)
#define     AUDIO_ENCODE_SET_AUDIO_PLL              SUB(0x0C)
#define     AUDIO_LIVEENCODE_FORMAT         		SUB(0x0D)
#define     AUDIO_LIVEENCODE_SAMPLERATE        		SUB(0x0E)
#define     AUDIO_LIVEENCODE_CB        		        SUB(0x0F)
#define     AUDIO_LIVEENCODE_MODE     		        SUB(0x10)
#define HIF_AUD_CMD_REC_STATUS		        (GRP_AUD | FUNC(0x13))
#define HIF_AUD_CMD_AUDIO_AEC          		(GRP_AUD | FUNC(0x14))
#define HIF_AUD_CMD_WAV_ENCODE              (GRP_AUD | FUNC(0x15))
#define     SET_WAV_OUTPUT_BUF              		SUB(0x01)
#define     SET_WAV_ENCODE_ENABLE                   SUB(0x02)
#define     GET_WAV_INFO                            SUB(0x03)
#define HIF_AUD_CMD_DRM                     (GRP_AUD | FUNC(0x16))
#define     SET_DRM_IV                        		SUB(0x01)
#define     SET_DRM_KEY                             SUB(0x02)
#define     SET_DRM_EN                              SUB(0x03)
#define     SET_DRM_HEADERLENGTH                    SUB(0x04)
#define HIF_AUD_CMD_BYPASS_PATH             (GRP_AUD | FUNC(0x17))
#define HIF_AUD_CMD_AUDIO_DAC               (GRP_AUD | FUNC(0x18))
#define HIF_AUD_CMD_AC3_CODE_OP             (GRP_AUD | FUNC(0x19))
#define     SET_AC3_VHPAR                           SUB(0x01)
#define HIF_AUD_CMD_I2S_CONFIG              (GRP_AUD | FUNC(0x1A))
#define     SET_MCLK_FREQ                           SUB(0x01)
#define HIF_AUD_CMD_LIVE_ENC_OP            	(GRP_AUD | FUNC(0x1B))
#define     ENCODE_OP_START             			SUB(0x01)
#define     ENCODE_OP_PAUSE             			SUB(0x02)
#define     ENCODE_OP_RESUME            			SUB(0x03)
#define     ENCODE_OP_STOP              			SUB(0x04)
#define     ENCODE_OP_GET_FILESIZE      			SUB(0x05)
#define     ENCODE_OP_PREENCODE         			SUB(0x06)
#define HIF_AUD_CMD_LIVE_REC_STATUS         (GRP_AUD | FUNC(0x1C))
#define HIF_AUD_CMD_REC_EFFECT              (GRP_AUD | FUNC(0x1D))
#define     CFG_WNR                                 SUB(0x01)
#define     SET_WNR_EN                              SUB(0x02)
#define     SET_AGC_EN                              SUB(0x03)
#define HIF_AUD_CMD_GET_ENCODE_PARAM        (GRP_AUD | FUNC(0x1E))
#define     FRAME_CNT_IN_SEC                        SUB(0x01)

/* USB */
#define HIF_USB_CMD_SET_MODE                (GRP_USB | FUNC(0x00))
#define HIF_USB_CMD_SET_MSDCBUF			    (GRP_USB | FUNC(0x01))
#if (MSDC_WR_FLOW_TEST_EN  == 1)
#define     SET_MSDC_W_BUFF_QUEUE                   SUB(0x01)
#endif
#if (SUPPORT_MSDC_SCSI_AIT_CMD_MODE)  
#define     SET_DBG_BUF                   			SUB(0x02)
#endif
#define HIF_USB_CMD_SET_PCSYNCBUF           (GRP_USB | FUNC(0x02))
#define     SET_PCSYNC_IN_BUF						SUB(0x01)
#define     SET_PCSYNC_OUT_BUF              		SUB(0x02)
#define     SET_PCSYNC_HANDSHAKE_BUF        		SUB(0x03)
#define HIF_USB_CMD_SET_CONTROLBUF		    (GRP_USB | FUNC(0x03))
#define HIF_USB_CMD_SET_PCCAMBUF		    (GRP_USB | FUNC(0x04))
#define     SET_PCCAM_COMPRESS_BUF					SUB(0x01)
#define     SET_PCCAM_LINE_BUF              		SUB(0x02)
#define HIF_USB_CMD_SET_MTPBUF              (GRP_USB | FUNC(0x05))
#define     SET_MTP_EP_BUF              			SUB(0x01)
#define HIF_USB_CMD_DPS         			(GRP_USB | FUNC(0x06))
#define     DPS_START_JOB              				SUB(0x01)
#define     DPS_ABORT_JOB              				SUB(0x02)
#define     DPS_CONTINUE_JOB              			SUB(0x03)
#define     DPS_PRINTER_STATUS              		SUB(0x04)
#define     DPS_GETBUFADDR                  		SUB(0x05)
#define     DPS_GETFILEID                   		SUB(0x06)
#define HIF_USB_CMD_PCCAM_CAPTURE           (GRP_USB | FUNC(0x07))
#define HIF_USB_CMD_SET_MEMDEV_BUF			(GRP_USB | FUNC(0x08))
#define HIF_USB_CMD_MSDC_ACK                (GRP_USB | FUNC(0x09))
#define     HIF_WRITE_ACK                           SUB(0x01)
#define     HIF_READ_ACK                            SUB(0x02)
#define HIF_USB_CMD_GET_MODE				(GRP_USB | FUNC(0x0A))
#define HIF_USB_CMD_STOPDEVICE			    (GRP_USB | FUNC(0x0B))
#define HIF_USB_CMD_CONNECTDEVICE		    (GRP_USB | FUNC(0x0C))
#define HIF_USB_CMD_GET_RW_FLAG			    (GRP_USB | FUNC(0x0D))
#define		HIF_USB_GET_W_FLAG						SUB(0x01)
#define		HIF_USB_GET_R_FLAG						SUB(0x02)
#define HIF_USB_CMD_ADJUST_PHY           (GRP_USB | FUNC(0x0E))
#define		HIF_USB_PHY_VREF						SUB(0x01)
#define		HIF_USB_PHY_BIASCURRENT					SUB(0x02)
#define		HIF_USB_PHY_SIGNAL						SUB(0x03)
#define HIF_USB_CMD_SET_MSDC_UNIT           (GRP_USB | FUNC(0x0F))

/* User Define */
#define HIF_USR_CMD_FS_ACCESS               (GRP_USR | FUNC(0x00))
#define     FS_FILE_OPEN                        SUB(0x00)
#define     FS_FILE_CLOSE                       SUB(0x01)
#define     FS_FILE_READ                        SUB(0x02)
#define     FS_FILE_WRITE                       SUB(0x03)
#define     FS_FILE_SEEK                        SUB(0x04)
#define     FS_FILE_TELL                        SUB(0x05)
#define     FS_FILE_COPY                        SUB(0x06)
#define     FS_FILE_TRUNCATE                    SUB(0x07)
#define     FS_FILE_GET_SIZE                    SUB(0x08)
#define     FS_FILE_REMOVE                      SUB(0x09)
#define     FS_FILE_DIR_GETATTRIBUTES           SUB(0x0A)
#define     FS_FILE_DIR_GETTIME                 SUB(0x0B)
#define     FS_FILE_DIR_MOVE                    SUB(0x0C)
#define     FS_FILE_DIR_RENAME                  SUB(0x0D)
#define     FS_FILE_DIR_SETATTRIBUTES           SUB(0x0E)
#define     FS_FILE_DIR_SETTIME                 SUB(0x0F)
#define     FS_DIR_CLOSE                        SUB(0x10)
#define     FS_DIR_GETATTRIBUTES                SUB(0x11)
#define     FS_DIR_GETNAME                      SUB(0x12)
#define     FS_DIR_GETSIZE                      SUB(0x13)
#define     FS_DIR_GETTIME                      SUB(0x14)
#define     FS_DIR_GETNUMFILES                  SUB(0x15)
#define     FS_DIR_CREATE                       SUB(0x16)
#define     FS_DIR_OPEN                         SUB(0x17)
#define     FS_DIR_READ                         SUB(0x18)
#define     FS_DIR_REWIND                       SUB(0x19)
#define     FS_DIR_REMOVE                       SUB(0x1A)
#define     FS_LOW_LEVEL_FORMAT                 SUB(0x1B)
#define     FS_HIGH_LEVLE_FORMAT                SUB(0x1C)
#define     FS_GET_FREE_SPACE                   SUB(0x1D)
#define     FS_GET_TOTAL_SPACE                  SUB(0x1E)
#define     FS_GET_NUM_VOLUMES                  SUB(0x1F)
#define     FS_GET_VOLUME_NAME                  SUB(0x20)
#define     FS_IS_VOLUME_MOUNTED                SUB(0x21)
#define     FS_CHECK_MEDIUM_PRESENT             SUB(0x22)
#define     FS_GET_VOLUME_INFO                  SUB(0x23)
#define     FS_GET_FILE_LIST                    SUB(0x24)
#define     FS_SET_CREATION_TIME                SUB(0x25)
#define     FS_DIR_GETNUMDIRS                   SUB(0x26)
#define     FS_INIT                             SUB(0x27)
#define     FS_MOUNT_VOLUME                     SUB(0x28)
#define     FS_UNMOUNT_VOLUME                   SUB(0x29)
#define     FS_FILE_DIR_GETINFO                 SUB(0x2A)
#define		FS_GET_MAX_DIR_COUNT				SUB(0x2B)
#define		FS_GET_MAX_FILE_COUNT				SUB(0x2C)
#define     FS_GET_FILE_LIST_DB                 SUB(0x2D)
#define HIF_USR_CMD_FS_SETADDRESS           (GRP_USR | FUNC(0x01))
#define     FS_SET_FILENAME_ADDRESS             SUB(0x01)
#define     FS_SET_SD_TMP_ADDR                  SUB(0x02)
#define     FS_SET_SN_MAP_ADDR                  SUB(0x03)
#define     FS_SET_SM_MAP_ADDR                  SUB(0x04)
#define     FS_SET_SF_MAP_ADDR                  SUB(0x05)
#define     FS_SET_FAT_GLOBALBUF_ADDR           SUB(0x06)
#define     FS_SET_DRM_IV_ADDR                  SUB(0x07)
#define     FS_GET_SDCLASS                      SUB(0x08)
#define HIF_USR_CMD_FS_STORAGE              (GRP_USR | FUNC(0x02))
#define     FS_STORAGE_CONFIG                   SUB(0x02)
#define     FS_SM_ERASE                         SUB(0x03)
#define     FS_IO_CTL                           SUB(0x05)
#define     FS_SF_STORAGE_CONFIG                SUB(0x06)

/* Host<->Firmware Status, reserve b[15:0] for command executing status */
#define CPU_BOOT_FINISH                     (0x00000001)
#define FCTL_CMD_IN_EXEC                    (0x00000002)
#define DSC_CMD_IN_EXEC                     (0x00000004)
#define SYSTEM_CMD_IN_EXEC                  (0x00000008)
#define AUDIO_CMD_IN_EXEC                   (0x00000010)
#define MGR3GP_CMD_IN_EXEC                  (0x00000020)
#define PLAYVID_CMD_IN_EXEC                 (0x00000040)
#define FS_CMD_IN_EXEC                      (0x00000080)
#define VIDRECD_CMD_IN_EXEC                 (0x00000100)
#define VIDDEC_CMD_IN_EXEC	                (0x00000200)
#define USB_CMD_IN_EXEC                     (0x00000400)
#define MGR3GP_CANCEL_FILE_SAVE             (0x00000800)
#define MEDIA_FILE_OVERFLOW                 (0x00001000)
#define DSC_CAPTURE_BEGIN                   (0x00002000)
#define IMAGE_UNDER_ZOOM                    (0x00004000)
#define SD_CARD_NOT_EXIST                   (0x00008000)
#define FDTC_CMD_IN_EXEC                    (0x00010000)
#define MEDIA_DUALENC_OVERFLOW              (0x00020000)

#ifdef BUILD_CE
#define SYS_FLAG_FS_CMD_DONE                (0x10000000)
#define SYS_FLAG_AUD_CMD_DONE               (0x20000000)
#define SYS_FLAG_DSC_CMD_DONE               (0x40000000)
#define SYS_FLAG_SYS_CMD_DONE               (0x80000000)
#define SYS_FLAG_CE_JOB_COMMAND             (0x01000000)
#define SYS_FLAG_DPF_JPEG                   (0x02000000)
#define SYS_FLAG_DPF_UI                     (0x04000000)
#define SYS_FLAG_DPF_WAIT_INFO_RDY          (0x08000000)
#define SYS_FLAG_DPF_USB                    (0x00100000)
#define SYS_FLAG_USB_CMD_DONE               (0x00200000)
#define SYS_FLAG_VID_CMD_DONE               (0x00400000)
#define SYS_FLAG_SENSOR_CMD_DONE            (0x00800000)
#define SYS_FLAG_PANEL_TOCUH                (0x00010000)
#endif

#endif//_MMP_HIF_CMD_H_
