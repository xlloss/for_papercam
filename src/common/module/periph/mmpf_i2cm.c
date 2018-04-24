//==============================================================================
//
//  File        : mmpf_i2cm.c
//  Description : MMPF_I2C functions
//  Author      : Penguin Torng
//  Revision    : 1.0
//
//==============================================================================

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "includes_fw.h"
#include "lib_retina.h"
#include "mmp_reg_gbl.h"
#include "mmp_reg_pad.h"
#include "mmp_reg_i2cm.h"
#include "mmp_reg_vif.h"
#include "mmpf_pio.h"
#include "mmpf_i2cm.h"
#include "mmpf_vif.h"
#include "mmpf_sensor.h"
#include "mmpf_system.h"

#define SWI2C_EN    (0)
#define EEDID_EN    (0)
#define DMAI2C_EN   (0)
#define RD_EN       (1)
#define BURST_EN    (0)
/** @addtogroup MMPF_I2CM
@{
*/

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================

#define I2C_SCL                 VIF_SIF_SCL
#define I2C_SDA                 VIF_SIF_SDA

#define I2C_INPUT               MMP_FALSE
#define I2C_OUTPUT              MMP_TRUE

#define I2C_SET_HIGH            MMP_TRUE
#define I2C_SET_LOW             MMP_FALSE

#if defined(MINIBOOT_FW) || defined(FAT_BOOT)
#define I2CM_OS_SEM_PROTECT     (0)
#else
#define I2CM_OS_SEM_PROTECT     (1)
#endif

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static MMP_UBYTE   	m_ubRegLen[I2CM_CNT] = {0};     // If in the future, each module keep its' I2CM attribute, this array can be replaced.
static MMP_UBYTE 	m_ubDataLen[I2CM_CNT] = {0};    // If in the future, each module keep its' I2CM attribute, this array can be replaced.
MMP_UBYTE			m_ubSwI2cmSlaveAddr[I2CM_SW_CNT] = {0};  // If in the future, each module keep its' I2CM attribute, this array can be replaced.
MMP_UBYTE			m_ubSwI2cmDelayTime[I2CM_SW_CNT] = {0};  // If in the future, each module keep its' I2CM attribute, this array can be replaced.
MMPF_OS_SEMID  		gI2cmSemID[I2CM_CNT];           // Used for protection
static MMP_ULONG    m_ulI2cmRxReadTimeout[I2CM_HW_CNT];

#if (I2CM_INT_MODE_EN == 1)
MMPF_OS_SEMID  		gI2cmIntSemID[I2CM_HW_CNT];     // Used for HW I2C interrupt mode protection
#endif
  
//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern MMP_ERR MMPF_PLL_GetGroupFreq(CLK_GRP ubGroupNum, MMP_ULONG *ulGroupFreq);
#if defined(MINIBOOT_FW)
extern void MMPC_System_WaitMs(MMP_ULONG ulMs);
#endif

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//------------------------------------------------------------------------------
//  Function    : MMPF_I2CM_ISR
//  Description : This function is the I2cm interrupt service routine.
//------------------------------------------------------------------------------
/** 
 * @brief This function is the I2cm interrupt service routine.
 * 
 * @return It return void.  
 */
void MMPF_I2CM_ISR(void)
{
    #if !defined(MINIBOOT_FW)
    #if (I2CM_INT_MODE_EN == 1)
   	MMP_ULONG   i = 0;
   	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;

   	for(i = 0; i < I2CM_HW_CNT; i++) 
   	{
   		if(pI2CM->I2CMS[i].I2CM_INT_CPU_SR & I2CM_TX_DONE) {
   			pI2CM->I2CMS[i].I2CM_INT_CPU_SR &= I2CM_TX_DONE;  //Clean status
   			
			if (pI2CM->I2CMS[i].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
				RTNA_DBG_Str(0, FG_RED("I2C Error SLAVE_NO_ACK1")"\r\n");
				RTNA_DBG_PrintByte(0, pI2cmAttr->i2cmID);
				RTNA_DBG_PrintByte(0, pI2cmAttr->ubSlaveAddr);
				RTNA_DBG_Str(0, "\r\n");
		        //Clear FIFO. Otherwise, keeping for next transmission.
		        pI2CM->I2CMS[i].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
		    }
		   	MMPF_OS_ReleaseSem(gI2cmIntSemID[i]); 
		   	break;
		}
	}
 	#endif
    #endif 	
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_InitDriver
//  Description : The function initialize i2cm interrupt and semaphore.
//------------------------------------------------------------------------------
/** 
 * @brief The function initialize i2cm interrupt and semaphore.
 *    
 * @return It return the function status. 
 */
MMP_ERR MMPF_I2cm_InitDriver(void)
{
	MMP_UBYTE   i = 0;
	static MMP_BOOL m_bInitialFlag = MMP_FALSE;
    #if (I2CM_INT_MODE_EN == 1)
    AITPS_AIC   pAIC = AITC_BASE_AIC;
    #endif

	if (m_bInitialFlag == MMP_FALSE) 
	{
		#if !defined(MINIBOOT_FW)
		#if (I2CM_INT_MODE_EN == 1)
        RTNA_AIC_Open(pAIC, AIC_SRC_I2CM, i2cm_isr_a,
                    AIC_INT_TO_IRQ | AIC_SRCTYPE_HIGH_LEVEL_SENSITIVE | 7);
		RTNA_AIC_IRQ_En(pAIC, AIC_SRC_I2CM);
        #endif
		#endif
		
		/* Initial SW I2cm Attribute */
		for (i = 0; i < I2CM_SW_CNT; i++) {
			m_ubSwI2cmSlaveAddr[i] = 0;
			m_ubSwI2cmDelayTime[i] = 0;
		}

        /* Initial HW I2cm Attribute */
		for (i = 0; i < I2CM_HW_CNT; i++) {
			#if (I2CM_OS_SEM_PROTECT)
			#if (I2CM_INT_MODE_EN == 1)
			gI2cmIntSemID[i] = MMPF_OS_CreateSem(0);
			#endif
			#endif
			m_ulI2cmRxReadTimeout[i] = 0;
		}
		
		/* Initial Common I2cm Attribute */
		for (i = 0; i < I2CM_CNT; i++) {
			#if (I2CM_OS_SEM_PROTECT)
			gI2cmSemID[i]   = MMPF_OS_CreateSem(1);
			#endif
			m_ubRegLen[i]   = 0;
			m_ubDataLen[i]  = 0;
		}
		
		MMPF_SYS_EnableClock(MMPF_SYS_CLK_I2CM, MMP_TRUE);

		m_bInitialFlag = MMP_TRUE;
	}
	return MMP_ERR_NONE;
}

#if SWI2C_EN==1
//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_Start
//  Description : The function is the start signal of i2cm.
//------------------------------------------------------------------------------
/** 
 * @brief The function is the start signal of i2cm.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_Start(MMP_I2CM_ATTR *pI2cmAttr)
{
	MMP_UBYTE   ubDelay;
	MMP_ERR     status = MMP_ERR_NONE;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED))  {
        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "Error SW I2CM speed\r\n");
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;
	
	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {
		
	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, (I2C_SCL|I2C_SDA), I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
    }
    else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
    
    	status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_clk_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
		status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
		RTNA_WAIT_US(ubDelay);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
		RTNA_WAIT_US(ubDelay);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
    	RTNA_WAIT_US(ubDelay);
    }
    else {
        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n");
    	return MMP_I2CM_ERR_PARAMETER;
    }

	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_Stop
//  Description : The function is the stop signal of i2cm.
//------------------------------------------------------------------------------
/** 
 * @brief The function is the stop signal of i2cm.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.     
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_Stop(MMP_I2CM_ATTR *pI2cmAttr)
{
	MMP_UBYTE 	ubDelay;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n");
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;
	
 	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {
 	
	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
	
    	status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
		RTNA_WAIT_US(ubDelay);
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
    }
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n");
    	return MMP_I2CM_ERR_PARAMETER;
    }
	
	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_WriteData
//  Description : The function write one byte data to slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function write one byte data to slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.     
 * @param[in] ubData    : stands for the data which will write into device. 
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_WriteData(MMP_I2CM_ATTR *pI2cmAttr, MMP_UBYTE ubData)
{
    MMP_ULONG   i;
    MMP_UBYTE	ubShift;
	MMP_UBYTE 	ubDelay;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
	    MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n");
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;
	
	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) 
	{
	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);

	    ubShift = 0x80;
	    
	    for (i = 0; i < 8; i++) {

	        if (ubData & ubShift) {
	            status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_HIGH);
	            status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	        }
	        else {
	            status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA | I2C_SCL, I2C_SET_LOW);
	        }

	        RTNA_WAIT_US(ubDelay);
	        status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	        RTNA_WAIT_US(ubDelay);
	        status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	        RTNA_WAIT_US(ubDelay);
	        ubShift >>= 1;
	    }
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO)
	{
    	status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);

		ubShift = 0x80;

    	for (i = 0; i < 8; i++) {

	        if (ubData & ubShift) {
	         	status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	        }
	        else {
	           	status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	        }

			RTNA_WAIT_US(ubDelay);
	        status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	        RTNA_WAIT_US(ubDelay);
            status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
            
            if (i == 7)
            {
                status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_FALSE, pI2cmAttr->bOsProtectEn);
            }
	        RTNA_WAIT_US(ubDelay);
	        ubShift >>= 1;
    	}
    }
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n");
    	return MMP_I2CM_ERR_PARAMETER;
    }

	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_ReadData
//  Description : The function read one byte data from slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function read one byte data from slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the data which read from device. . 
 */
MMP_UBYTE MMPF_SwI2cm_ReadData(MMP_I2CM_ATTR *pI2cmAttr)
{
    MMP_ULONG   i;
    MMP_UBYTE  	ubReceiveData = 0;
    MMP_UBYTE  	ubBit_val =0 ;
	MMP_UBYTE 	ubDelay;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
		MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n");
		return 0;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;

	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {
	
	    MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_INPUT);

	    ubReceiveData = 0;
	    
	    for (i = 0; i < 8; i++) {
	        RTNA_WAIT_US(ubDelay);
	        MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
		    
		    ubBit_val = (MMP_UBYTE)MMPF_VIF_GetPIOOutput(ubVifId, I2C_SDA);
	        ubReceiveData |= ubBit_val;
	        
	        if (i < 7) {
	            ubReceiveData <<= 1;
	        }
	        RTNA_WAIT_US(ubDelay);
		    MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    }
	    return ubReceiveData;
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
    	
    	MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_FALSE, pI2cmAttr->bOsProtectEn);
	 
    	ubReceiveData = 0;
    	
    	for (i = 0; i < 8; i++) {
			MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	        MMPF_PIO_GetData(pI2cmAttr->sw_data_pin, &ubBit_val);
	        RTNA_WAIT_US(ubDelay);

	        ubReceiveData |= ubBit_val;
	        
	        if (i < 7) {
	            ubReceiveData <<= 1;
	        }
	        
	        MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
			
			if (i == 7)
			{
				MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
				MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
			}
	        RTNA_WAIT_US(ubDelay);
	     }
    }
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n");
    	return 0;
    }
	
    return ubReceiveData;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_GetACK
//  Description : The function get ACK signal from slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function get ACK signal from slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_GetACK(MMP_I2CM_ATTR *pI2cmAttr)
{
    MMP_UBYTE  	ubBit_val;
    MMP_ERR 	status  = MMP_ERR_NONE;
    MMP_ULONG 	ulcount = 0x4FFF;
	MMP_UBYTE 	ubDelay;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
		MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n");
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;

	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {
	
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_INPUT);
	    
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);

	    do {
	        ubBit_val = (MMP_UBYTE)MMPF_VIF_GetPIOOutput(ubVifId, I2C_SDA);
            ulcount--;
            if (ulcount == 0) {
                status = MMP_I2CM_ERR_SLAVE_NO_ACK;
                MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "MMP_I2CM_ERR_SLAVE_NO_ACK\r\n");
                break;
            }
	    } while(ubBit_val);

	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
		
	    status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_FALSE, pI2cmAttr->bOsProtectEn);
	    
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    
	    do {
	        status |= MMPF_PIO_GetData(pI2cmAttr->sw_data_pin, &ubBit_val);
            ulcount--;
            if (ulcount == 0) {
                status = MMP_I2CM_ERR_SLAVE_NO_ACK;
                MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error : MMP_I2CM_ERR_SLAVE_NO_ACK\r\n");        
                break;
            }
	    } while(ubBit_val);
	   
	   	// For Customer's wave-form improvement request
	    MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);    
	}
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n"); 
    	return MMP_I2CM_ERR_PARAMETER;
    }

	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_SendNACK
//  Description : The function send NACK signal to slave device for read operation.
//------------------------------------------------------------------------------
/** 
 * @brief The function send NACK signal to slave device for read operation.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_SendNACK(MMP_I2CM_ATTR *pI2cmAttr)
{
	MMP_UBYTE 	ubDelay;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
		MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n"); 
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;
	
	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {

	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_HIGH);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SDA, I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
	 	status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
	 	
	 	status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	}
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error soft I2CM ID\r\n"); 
    	return MMP_I2CM_ERR_PARAMETER;
    }
	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_SwI2cm_SendACK
//  Description : The function send ACK signal to slave device for read operation.
//------------------------------------------------------------------------------
/** 
 * @brief The function send ACK signal to slave device for read operation.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the function status. 
 */
MMP_ERR MMPF_SwI2cm_SendACK(MMP_I2CM_ATTR *pI2cmAttr)
{
	MMP_UBYTE 	ubDelay;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	
	if ((pI2cmAttr->ulI2cmSpeed == 0) || (pI2cmAttr->ulI2cmSpeed > I2CM_SW_MAX_SPEED)) {
		MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM speed\r\n"); 
		return MMP_I2CM_ERR_PARAMETER;
	}
	
	ubDelay = I2CM_SW_MAX_SPEED / pI2cmAttr->ulI2cmSpeed;
	
	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) {

	    status |= MMPF_VIF_SetPIODir(ubVifId, I2C_SDA, I2C_OUTPUT);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, (I2C_SCL | I2C_SDA), I2C_SET_LOW);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_HIGH);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_VIF_SetPIOOutput(ubVifId, I2C_SCL, I2C_SET_LOW);
	 }
    else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO) {
		status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
		
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	    status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_LOW, pI2cmAttr->bOsProtectEn);
	    RTNA_WAIT_US(ubDelay);
	}
    else {
    	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error SW I2CM ID\r\n"); 
    	return MMP_I2CM_ERR_PARAMETER;
    }

	return	status;
}
#endif

#if 0
void __I2CM_GENERAL_INTERFACE__(){}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_StartSemProtect
//  Description : The function acquire semaphore to do i2cm operation.
//------------------------------------------------------------------------------
/** 
 * @brief The function acquire semaphore to do i2cm operation.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the function status. 
 */
MMP_UBYTE MMPF_I2cm_StartSemProtect(MMP_I2CM_ATTR *pI2cmAttr) 
{
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		return MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	}
	else {
		return 0;
	}
	#else
	return 0;
	#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_EndSemProtect
//  Description : The function release semaphore to end i2cm operation.
//------------------------------------------------------------------------------
/** 
 * @brief The function release semaphore to end i2cm operation.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.      
 * @return It return the function status. 
 */
MMP_UBYTE MMPF_I2cm_EndSemProtect(MMP_I2CM_ATTR *pI2cmAttr) 
{
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		return MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);
	}
	else {
		return 0;
	} 
	#else
	return 0;
	#endif
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_SetRxTimeout
//  Description : The function set Rx operation timeout count.
//------------------------------------------------------------------------------
/** 
 * @brief The function set Rx operation timeout count.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.
 * @param[in] ulTimeOut : stands for the timeout count, 
 *                        ulTimeOut = 0 means no Timeout, otherwise the unit is ms.       
 * @return It return the function status. 
 */
MMP_ERR MMPF_I2cm_SetRxTimeout(MMP_I2CM_ATTR *pI2cmAttr, MMP_ULONG ulTimeOut)
{
	m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] = ulTimeOut;
	return MMP_ERR_NONE;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_Initialize
//  Description : The function intiailize HW/SW i2cm device.
//------------------------------------------------------------------------------
/** 
 * @brief The function intiailize HW/SW i2cm device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @return It return the function status. 
 */
MMP_ERR MMPF_I2cm_Initialize(MMP_I2CM_ATTR *pI2cmAttr)
{
	AITPS_GBL   pGBL  = AITC_BASE_GBL;
    AITPS_PAD   pPAD  = AITC_BASE_PAD;
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
	MMP_USHORT  usSckDiv = 0;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG   ulGrp0Freq = 0;
	MMP_UBYTE   ubSwIdx;
	MMP_UBYTE	ubVifId = pI2cmAttr->ubVifPioMdlId;
	#if SWI2C_EN==1
	if (pI2cmAttr->i2cmID == MMP_I2CM_SNR) 
	{
	    // Set PSNR_SDA Pad configuration
		pPAD->PAD_IO_CFG_PSNR[0x07] = PAD_OUT_DRIVING(1) | PAD_PULL_UP; 
		
		if (ubVifId == MMPF_VIF_MDL_ID0)
            pGBL->GBL_MISC_IO_CFG |= GBL_VIF0_GPIO_EN;
	    else if (ubVifId == MMPF_VIF_MDL_ID1)
	        pGBL->GBL_MISC_IO_CFG |= GBL_VIF1_GPIO_EN;

		m_ubSwI2cmSlaveAddr[1] = (pI2cmAttr->ubSlaveAddr) << 1;
		
		status |= MMPF_VIF_SetPIODir(ubVifId, VIF_SIF_SDA, I2C_OUTPUT);
		status |= MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_SDA, I2C_SET_HIGH);
		status |= MMPF_VIF_SetPIODir(ubVifId, VIF_SIF_SCL, I2C_OUTPUT);
		status |= MMPF_VIF_SetPIOOutput(ubVifId, VIF_SIF_SCL, I2C_SET_HIGH);

		ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);
		m_ubSwI2cmDelayTime[ubSwIdx] = pI2cmAttr->ubDelayTime;
	}
	else if (pI2cmAttr->i2cmID == MMP_I2CM_GPIO)
	{
        m_ubSwI2cmSlaveAddr[0] = pI2cmAttr->ubSlaveAddr;
        
        // Pull High first !! before set output mode
        status |= MMPF_PIO_SetData(pI2cmAttr->sw_clk_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);  
		status |= MMPF_PIO_SetData(pI2cmAttr->sw_data_pin, GPIO_HIGH, pI2cmAttr->bOsProtectEn);
        
		status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_clk_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);
		status |= MMPF_PIO_EnableOutputMode(pI2cmAttr->sw_data_pin, MMP_TRUE, pI2cmAttr->bOsProtectEn);

        ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);
		m_ubSwI2cmDelayTime[ubSwIdx] = pI2cmAttr->ubDelayTime;	
    }
	else
	#endif
	
	{	
		// Set I2cm Pad configuration
    	if (pI2cmAttr->i2cmID == MMP_I2CM0) {
            pGBL->GBL_I2CM_PAD_CFG &= ~(GBL_I2CM0_PAD_MASK);
            pGBL->GBL_I2CM_PAD_CFG |= GBL_I2CM0_PAD(pI2cmAttr->ubPadNum);
    	}
    	else if (pI2cmAttr->i2cmID == MMP_I2CM1) {
    	    pGBL->GBL_I2CM_PAD_CFG &= ~(GBL_I2CM1_PAD_MASK);
            pGBL->GBL_I2CM_PAD_CFG |= GBL_I2CM1_PAD(pI2cmAttr->ubPadNum);
    	}
        else if (pI2cmAttr->i2cmID == MMP_I2CM2) {
    	    pGBL->GBL_I2CM_PAD_CFG &= ~(GBL_I2CM2_PAD_MASK);
            pGBL->GBL_I2CM_PAD_CFG |= GBL_I2CM2_PAD(pI2cmAttr->ubPadNum);
    	}
        else if (pI2cmAttr->i2cmID == MMP_I2CM3) {
    	    pGBL->GBL_I2CM_PAD_CFG &= ~(GBL_I2CM3_PAD_MASK);
            pGBL->GBL_I2CM_PAD_CFG |= GBL_I2CM3_PAD(pI2cmAttr->ubPadNum);
    	}

        if (pI2cmAttr->bWfclModeEn == MMP_TRUE) {
    	    pI2cmAttr->ubRegLen = pI2cmAttr->ubDataLen;
    	}

    	// Note the SLAVE_ADDR is different among SW and HW I2CM
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR = pI2cmAttr->ubSlaveAddr;

		if (pI2cmAttr->ubRegLen == 16) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL =  I2CM_REG_16BIT_MODE
	    	    									    | I2CM_SCK_OH_MODE	// Output high by internal signal
		    	    								    | I2CM_SDA_OD_MODE  // By pull-up current, or resistor
		    	    								    | I2CM_STOP_IF_NOACK;
		} 
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL =  I2CM_REG_8BIT_MODE
													    | I2CM_SCK_OH_MODE  // Output high by internal signal
	       											    | I2CM_SDA_OD_MODE  // By pull-up current, or resistor
														| I2CM_STOP_IF_NOACK;
		}
		
        /* HDMI_I2C output from I2CM3 pad0, SCL/SDA need change to open-drain */
        if ((pI2cmAttr->i2cmID == MMP_I2CM3) && (pI2cmAttr->ubPadNum == 0)) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL &= ~(I2CM_SCK_OH_MODE | I2CM_SDA_OD_MODE); 
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= (I2CM_SCK_OD_MODE | I2CM_SDA_OD_MODE);
		}

		// Set HW Feature
		if (pI2cmAttr->bInputFilterEn == MMP_FALSE) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT |= I2CM_INPUT_FILTERN_DIS;
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT &= ~(I2CM_INPUT_FILTERN_DIS);
		}
		
		if (pI2cmAttr->bDelayWaitEn == MMP_TRUE) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT |= I2CM_DELAY_WAIT_EN;
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DELAY_WAIT_CYC = pI2cmAttr->ubDelayCycle;
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT &= ~(I2CM_DELAY_WAIT_EN);
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DELAY_WAIT_CYC = 0;
		}
		
		if (pI2cmAttr->b10BitModeEn == MMP_TRUE) {
		    /* For 10Bit mode, the I2CM_SLAVE_ADDR store addr[9:8] I2CM_SLAVE_ADDR1 store addr[7:0] */
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT |= I2CM_10BIT_SLAVEADDR_EN;
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR1 = pI2cmAttr->ubSlaveAddr1;
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT &= ~(I2CM_10BIT_SLAVEADDR_EN);
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR1 = 0;
		}
		
		if (pI2cmAttr->bClkStretchEn == MMP_TRUE) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CLK_STRETCH_EN |= I2CM_STRETCH_ENABLE;
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CLK_STRETCH_EN &= ~(I2CM_STRETCH_ENABLE);
		}
        
        // Set Clock Division/Timing
		if (pI2cmAttr->ulI2cmSpeed != 0) 
		{
		    MMPF_PLL_GetGroupFreq(CLK_GRP_GBL, &ulGrp0Freq);
		    
			usSckDiv = (ulGrp0Freq >> 1) * 1000 / pI2cmAttr->ulI2cmSpeed; // I2C module's clock = G0/2
			
			if (usSckDiv == 0) {
				MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Attention! G0 is not been initialized\r\n"); 
				usSckDiv = 48000000 / pI2cmAttr->ulI2cmSpeed;
			}
		}
		else 
		{
			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Error HW I2CM Speed Settings!\r\n"); 
			return MMP_I2CM_ERR_PARAMETER;
		}
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SCK_DIVIDER       = usSckDiv;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DATA_HOLD_CNT     = (usSckDiv >> 3);       // Must not > (usSckDiv/2). Sugguest as (usSckDiv/4)~(usSckDiv/8)
	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CMDSET_WAIT_CNT   = I2CM_SET_WAIT_CNT(20); // Wait N+1 i2cm_clk for next I2C cmd set
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SCL_DUTY_CNT      = (usSckDiv >> 1);
    }
	
    m_ubRegLen[pI2cmAttr->i2cmID]   = pI2cmAttr->ubRegLen;
	m_ubDataLen[pI2cmAttr->i2cmID]  = pI2cmAttr->ubDataLen;
	
	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_WriteReg
//  Description : The function write one set data to slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function write one set data to slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address. If pI2cmAttr->bWfclModeEn is MMP_TRUE. usReg is no use.
 * @param[in] usData    : stands for the data to be sent.
 * @return It return the function status. 
 */
MMP_ERR	MMPF_I2cm_WriteReg(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT usReg, MMP_USHORT usData)
{
    return MMPF_I2cm_WriteRegSet(pI2cmAttr, &usReg, &usData, 1);
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2Cm_ReadEEDID
//  Description : The function read EDD data from slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function read EDD data from slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.
 * @param[out] ubData   : stands for 
 * @param[in] usSeg     : stands for 
 * @param[in] usOffset  : stands for 
 * @param[in] usSize    : stands for 
 * @param[in] mode    	: stands for 
 * @return It return the function status. 
 */
#if EEDID_EN==1
MMP_ERR MMPF_I2Cm_ReadEEDID(MMP_I2CM_ATTR 		*pI2cmAttr,
                            MMP_UBYTE 			*ubData,
                            MMP_USHORT 			usSeg,
                            MMP_USHORT 			usOffset, 
                            MMP_USHORT 			usSize,
                            MMP_DDC_OPERATION_TYPE mode)
{
    #if !defined(MINIBOOT_FW) // Reduce code size for miniboot
    MMP_UBYTE	ubSemStatus = 0xFF;
    MMP_ERR		status = MMP_ERR_NONE;
    MMP_ULONG   i;
    MMP_ULONG   ulI2cmTimeOut=0;
    AITPS_I2CM  pI2CM = AITC_BASE_I2CM;

    #if (I2CM_OS_SEM_PROTECT)
    if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
        ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "I2CM Semaphore TimeOut\r\n"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
    }
    #endif

    status |= MMPF_I2cm_Initialize(pI2cmAttr);
    
    if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) 
    {
		status |= MMPF_SwI2cm_Start(pI2cmAttr);
        status |= MMPF_SwI2cm_WriteData(pI2cmAttr, 0x60);
        status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        
        status |= MMPF_SwI2cm_WriteData(pI2cmAttr, usSeg);
	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
	    
	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
        status |= MMPF_SwI2cm_WriteData(pI2cmAttr, 0xA0);
        status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        
        status |= MMPF_SwI2cm_WriteData(pI2cmAttr, usOffset);
	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
	    
	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
        status |= MMPF_SwI2cm_WriteData(pI2cmAttr, 0xA1);
        status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        
        if ((usSize + usOffset) > 256) {
            usSize = 256 - usOffset;
        }
        
        for (i = 0; i < usSize; i++) {
            *(ubData + i) = MMPF_SwI2cm_ReadData(pI2cmAttr);
            if (i != (usSize - 1)) {
                status |= MMPF_SwI2cm_SendACK(pI2cmAttr);
            }
            else {
                status |= MMPF_SwI2cm_SendNACK(pI2cmAttr);
            }
        }
        status |= MMPF_SwI2cm_Stop(pI2cmAttr);
    } 
    else 
    {
		if (pI2cmAttr->i2cmID == MMP_I2CM0 || pI2cmAttr->i2cmID == MMP_I2CM1 || 
		    pI2cmAttr->i2cmID == MMP_I2CM2 || pI2cmAttr->i2cmID == MMP_I2CM3) 
		{
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR         	= 0x50;
           	if (mode == MMP_EDDC_PROTOCOL){
		    	pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_EDDC_SLAVE_ADDR    = 0x30 | I2CM_EDDC_ENABLE;
                pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_EDDC_MODE_ADDR     = usSeg;
			}
        	else if (mode == MMP_DDC2B_PROTOCOL) {
            	pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_EDDC_SLAVE_ADDR    = 0;
		    }
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_REG_ADDR      = usOffset;
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_RX_ST_ADDR         = (MMP_ULONG)ubData;
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_RX_BYTE_CNT        = usSize;
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_CTRL          = I2CM_DMA_RX_EN;
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL                    |= I2CM_REPEAT_MODE_EN | I2CM_MASTER_EN;

		    if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			    ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
    		}
    		
    		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) 
    		{
    			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
    				ulI2cmTimeOut--;

    				if (ulI2cmTimeOut == 0) {
    					status = MMP_I2CM_ERR_READ_TIMEOUT;
                        goto L_I2cmOut;
    				}
    				else {
    					//Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
    					MMPF_OS_Sleep(1);
    				}
    			}
    		}
    		
    		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
    			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "I2C Error SLAVE_NO_ACK2\r\n"); 
    			status = MMP_I2CM_ERR_SLAVE_NO_ACK;
                goto L_I2cmOut;
    		}
    		
    		#if 0
    		// HDMI E-DDC has a bug, need to use software work-around    		
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_HOST_SR = I2CM_SLAVE_NO_ACK;
    		
    		*((volatile MMP_UBYTE *)0x80005D6A) &= ~(0x04);
    		
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR        = 0x00 | I2CM_WRITE_MODE;
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_EDDC_SLAVE_ADDR   = 0x00;		
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_TX_ST_ADDR    = 0x100000; // temp write address   		
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_TX_BYTE_CNT   = 50;
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_CTRL     = I2CM_DMA_TX_EN;    		
    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL               = I2CM_CONTI_IF_NOACK | I2CM_MASTER_EN;
    		
    		while(!(pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_HOST_SR & I2CM_SLAVE_NO_ACK));
    		
    		MMPF_OS_Sleep(1);

    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_EDDC_SLAVE_ADDR |= I2CM_EDDC_ENABLE;
    		
    		if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			    ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
    		}

    		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) {
    			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
    				ulI2cmTimeOut --;
    				if (ulI2cmTimeOut == 0) {
                        status = MMP_I2CM_ERR_READ_TIMEOUT;
                        goto L_I2cmOut;
    				}
    				else {
    					//Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
    					MMPF_OS_Sleep(1);
    				}
    			}
    		}
    		#endif
		}
		else {
		    status = MMP_I2CM_ERR_PARAMETER;
		}
    }

L_I2cmOut:
    
    #if (I2CM_OS_SEM_PROTECT)
    if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);
		}
	}
	#endif

	pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_EDDC_SLAVE_ADDR &= ~(I2CM_EDDC_ENABLE);
    return status;
    #else
	return MMP_ERR_NONE;
    #endif
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_ReadReg
//  Description : The function read one set data from slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function read one set data from slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address.
 * @param[out] usData   : stands for the received data.
 * @return It return the function status. 
 */
#if RD_EN==1
MMP_ERR	MMPF_I2cm_ReadReg(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT usReg, MMP_USHORT *usData)
{
    return MMPF_I2cm_ReadRegSet(pI2cmAttr, &usReg, usData, 1);
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_WriteRegSet
//  Description : The function write multi-set data to slave device. (FIFO mode)
//------------------------------------------------------------------------------
/** 
 * @brief The function write multi-set data to slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address. If pI2cmAttr->bWfclModeEn is MMP_TRUE. usReg is no use.
 * @param[in] usData    : stands for the data to be sent.
 * @param[in] usSetCnt  : stands for the set count to be sent.
 * @return It return the function status. 
 */
MMP_ERR	MMPF_I2cm_WriteRegSet(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT *usReg, MMP_USHORT *usData, MMP_UBYTE usSetCnt)
{
	MMP_USHORT  i;
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_UBYTE   ubSwIdx;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG   ulTimeOutTick = I2CM_WHILE_TIMEOUT;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "I2CM Semaphore TimeOut\r\n"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	#if SWI2C_EN==1
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) 
	{
	    ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);

		for (i = 0; i < usSetCnt; i++) 
		{      	
        	status |= MMPF_I2cm_Initialize(pI2cmAttr);
        	
    	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
    	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx]);
    	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
    		
    		if (pI2cmAttr->bWfclModeEn == MMP_FALSE){
        		if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
        		    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usReg[i] >> 8));
        	    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        		}
            	status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usReg[i]);
           		status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		    RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
            }

    		if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    		    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usData[i] >> 8));
    	    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		}
       		status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usData[i]);
        	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);

    	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
            if (status != MMP_ERR_NONE) {
                goto L_I2cmOut;
		    }
		}
	}
	else 
	#endif
	
	{	
		status |= MMPF_I2cm_Initialize(pI2cmAttr);
		
		// One write set contains 1 Address, 1 ByteCount and 1 Data.
		if (usSetCnt*((m_ubDataLen[pI2cmAttr->i2cmID]>>3)*2 + 1) > I2CM_TX_FIFO_DEPTH) {
			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "Attention! !! OVER I2C TX FIFO SIZE\r\n"); 

            #if (I2CM_OS_SEM_PROTECT)
			if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
				if (ubSemStatus == OS_NO_ERR) {
					MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
				}
			}
            #endif

			while (usSetCnt > 3) {
				status |= MMPF_I2cm_WriteRegSet(pI2cmAttr, usReg, usData, 3);
				usReg    = usReg + 3;
				usData   = usData + 3;
				usSetCnt = usSetCnt - 3;
			}
			status |= MMPF_I2cm_WriteRegSet(pI2cmAttr, usReg, usData, usSetCnt);
			return status;
		}
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);

	    for (i = 0; i < usSetCnt; i++) 
	    {
	        if (pI2cmAttr->bWfclModeEn == MMP_FALSE) {
    	    	if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    	    	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usReg[i] >> 8);
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg[i];
    	    	}
    		    else {
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg[i];
    	    	}
	    	
    		    if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 2;
    	    	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usData[i] >> 8);
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    	        }
    	        else {
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 1;
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    	        }
    	    }
    	    else {  //EROY CHECK: No need to set ByteCount?
    		    if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    	    	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usData[i] >> 8);
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    	        }
    	        else {
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    	        }
    	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 0;
    	    }
	    }
	    
	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = usSetCnt;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_WRITE_MODE;
		
		#if (I2CM_INT_MODE_EN == 1)
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_TX_DONE;
		#endif
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;
		
		#if (I2CM_INT_MODE_EN == 1)
		
		#if (I2CM_OS_SEM_PROTECT)
		if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
			MMPF_OS_AcquireSem(gI2cmIntSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
		}
		#endif
		
		#else
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN)
		{
    		#if (I2CM_OS_SEM_PROTECT)
    		//MMPF_OS_Sleep(1);
    		#else
    		MMPF_SYS_WaitUs(1000, MMP_FALSE, MMP_FALSE);
    		#endif

            ulTimeOutTick--;
    	    if (ulTimeOutTick == 0) {
    	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "I2CM While TimeOut\r\n"); 
    	        return MMP_I2CM_ERR_WHILE_TIMEOUT;
    	    }
        }

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
        	MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK3,ID"); 
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK3,Addr"); 
			
			// Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
			
			status = MMP_I2CM_ERR_SLAVE_NO_ACK;
			goto L_I2cmOut;
		}
		#endif
    }

L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
    #endif

    return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_ReadRegSet
//  Description : The function read multi-set data from slave device. (FIFO mode)
//------------------------------------------------------------------------------
/** 
 * @brief The function read multi-set data from slave device.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address.
 * @param[in] usData    : stands for the received data.
 * @param[in] usSetCnt  : stands for the set count to be sent.
 * @return It return the function status. 
 */
#if RD_EN==1
MMP_ERR	MMPF_I2cm_ReadRegSet(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT *usReg, MMP_USHORT *usData, MMP_UBYTE usSetCnt)
{
	MMP_USHORT  i;
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_UBYTE   ubSwIdx;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG	ulI2cmTimeOut = 0;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	#if SWI2C_EN==1
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) 
	{
	    ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);

		for (i = 0; i < usSetCnt; i++) 
		{ 
        	status |= MMPF_I2cm_Initialize(pI2cmAttr);

    		if (pI2cmAttr->bRfclModeEn == MMP_FALSE)
    		{
        	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
        	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx]);
        	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
        	    
        		if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
        		    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usReg[i] >> 8));
        	    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        		}

            	status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usReg[i]);
            	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
        		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);

        	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
            }
            
    	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
    	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx] + 1);
    	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);

    		usData[i] = 0;
    		
    	    if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    		    usData[i] = ((MMP_USHORT)MMPF_SwI2cm_ReadData(pI2cmAttr)) << 8;
    	    	status |= MMPF_SwI2cm_SendACK(pI2cmAttr);
    	    }

    	    usData[i] |= MMPF_SwI2cm_ReadData(pI2cmAttr);
    	    status |= MMPF_SwI2cm_SendNACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
    		
    	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
            if (status != MMP_ERR_NONE){
                goto L_I2cmOut;
		    }
		}
	}
	else 
	#endif
	{		
		status |= MMPF_I2cm_Initialize(pI2cmAttr);
		
		// One read set contains 1 Data.
		if (usSetCnt*(m_ubDataLen[pI2cmAttr->i2cmID]>>3) > I2CM_RX_FIFO_DEPTH) 
		{  
			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "Attention! !! OVER I2C RX FIFO SIZE"); 
			
            #if (I2CM_OS_SEM_PROTECT)		
			if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
				if (ubSemStatus == OS_NO_ERR) {
					MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
				}
			}
            #endif

			while (usSetCnt > 4) {
				status |= MMPF_I2cm_ReadRegSet(pI2cmAttr, usReg, usData, 4);
				usReg    = usReg + 4;
				usData   = usData + 4;
				usSetCnt = usSetCnt - 4;
			}
			status |= MMPF_I2cm_ReadRegSet(pI2cmAttr, usReg, usData, usSetCnt);
			return status;
		}
		
		// Rx need to reset FIFO first.	
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;

		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);

	    for (i = 0; i < usSetCnt; i++) 
	    {
            if (pI2cmAttr->bRfclModeEn == MMP_FALSE){
    	    	if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usReg[i] >> 8);
    		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg[i];
    	    	}
    		    else {
    	    	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg[i];
    		    }
    		}
    		
	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 2;
	        }
	        else {
		        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 1;
	        }
	    }

	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = usSetCnt;

		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_READ_MODE;
		#if (I2CM_INT_MODE_EN == 1)
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_TX_DONE;
		#endif

		if (pI2cmAttr->bRepeatModeEn == MMP_FALSE) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL &= ~(I2CM_REPEAT_MODE_EN);
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= (I2CM_REPEAT_MODE_EN);
		}
		
		if (pI2cmAttr->bRfclModeEn == MMP_FALSE) {
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;
		}
		else {
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN | I2CM_RFCL_MODE_EN;
		}

		#if (I2CM_INT_MODE_EN == 1)
		
		#if (I2CM_OS_SEM_PROTECT)
		if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
			MMPF_OS_AcquireSem(gI2cmIntSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
		}
		#endif
		
		#else
		
		if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
		}
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) 
		{
			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
				ulI2cmTimeOut--;

				if (ulI2cmTimeOut == 0) {
				    status = MMP_I2CM_ERR_READ_TIMEOUT;
					goto L_I2cmOut;
				}
				else {
					// Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
					#if (I2CM_OS_SEM_PROTECT)
					MMPF_OS_Sleep(1);
					#else
					MMPC_System_WaitMs(1);
					#endif
				}
			}
		}

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK4,ID"); 
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK4,Addr");

    		if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    			RTNA_DBG_PrintShort(0, *(MMP_USHORT *) &usReg[0]);
    		}
            else {
    			RTNA_DBG_PrintByte(0, usReg[0]);
            }

			RTNA_DBG_Str(0, "\r\n");
			
			// Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
			
			status = MMP_I2CM_ERR_SLAVE_NO_ACK;
			goto L_I2cmOut;
		}
		#endif
		
		// Read Data from Rx FIFO
	    for (i = 0; i < usSetCnt; i++) {
	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
		        usData[i]  = (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0] << 8);
		        usData[i] += (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0]);
	    	}
		    else {
		        usData[i] = pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0];
	    	}
	    }
	}

L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
    if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
    	if (ubSemStatus == OS_NO_ERR) {
    		MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
    	}
    }
    #endif
    
    return status;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_WriteBurstData
//  Description : The function burst write data to slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function burst write data to slave device. (reg address will auto increament)
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address. If pI2cmAttr->bWfclModeEn is MMP_TRUE. usReg is no use.
 * @param[in] usData    : stands for the data to be sent.
 * @param[in] usDataCnt : stands for the set count to be sent.
 * @return It return the function status. 
 */
#if BURST_EN==1 
MMP_ERR MMPF_I2cm_WriteBurstData(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT usReg, MMP_USHORT *usData, MMP_UBYTE usDataCnt)
{
	MMP_USHORT  i;
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_UBYTE   ubSwIdx;
	MMP_ERR		status = MMP_ERR_NONE;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);
	
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) 
	{
	    ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);

	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx]);
	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
		
		if (pI2cmAttr->bWfclModeEn == MMP_FALSE) {
            if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    		    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usReg >> 8));
    	    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		}
    	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usReg);
    	   	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
        }

		for (i = 0; i < usDataCnt; i++) {
			if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
			    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usData[i] >> 8));
		    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
			}	    
		   	status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usData[i]);
		    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
		    RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
	    }
	    
	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
	}
	else 
	{
		if ((usDataCnt*(m_ubDataLen[pI2cmAttr->i2cmID]>>3) + 3) > I2CM_TX_FIFO_DEPTH) {  //3 for the case with 16bit register address and one byte for byte count
			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "Attention! !! OVER I2C TX FIFO SIZE"); 
            
            #if (I2CM_OS_SEM_PROTECT)	
			if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
				if (ubSemStatus == OS_NO_ERR) {
					MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
				}
			}
            #endif

			while (usDataCnt > 6) {
				status |= MMPF_I2cm_WriteBurstData(pI2cmAttr, usReg, usData, 6);
				usReg     = usReg + 6;
				usData    = usData + 6;
				usDataCnt = usDataCnt - 6;
			}
			status |= MMPF_I2cm_WriteBurstData(pI2cmAttr, usReg, usData, usDataCnt);
			return status;
		}
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);

	    if (pI2cmAttr->bWfclModeEn == MMP_FALSE) {

    	    if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    	    	pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usReg >> 8);
    		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg;
    	    }
    		else {
    		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg;
    	   	}
	    		
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (m_ubDataLen[pI2cmAttr->i2cmID] >> 3) * usDataCnt;
	        
    	    for (i = 0; i < usDataCnt; i++) {
    	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    	    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usData[i] >> 8);
    			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    		    }
    		    else {
    			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    		    }
    	   	}
	    }
	    else {

	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
	    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usData[0] >> 8);
			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[0];
		    }
		    else {
			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[0];
		    }
		    
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (m_ubDataLen[pI2cmAttr->i2cmID] >> 3) * (usDataCnt - 1);
	        
    	    for (i = 1; i < usDataCnt; i++) {
    	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
    	    		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usData[i] >> 8);
    			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    		    }
    		    else {
    			    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData[i];
    		    }
    	   	}
	    }
	    	
	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = 1;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_WRITE_MODE;
		
		#if (I2CM_INT_MODE_EN == 1)
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_TX_DONE;
		#endif
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;
		
		#if (I2CM_INT_MODE_EN == 1)
		
		#if (I2CM_OS_SEM_PROTECT)
		if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
			MMPF_OS_AcquireSem(gI2cmIntSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
		}
		#endif
		
		#else
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN);

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK5,ID\r\n"); 
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK5,Addr\r\n"); 
			
			// Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
			
			status = MMP_I2CM_ERR_SLAVE_NO_ACK;
			goto L_I2cmOut;
		}
		#endif
    }

L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
    if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
	    if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
    #endif
    	
    return status;
}
#endif
//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_ReadBurstData
//  Description : The function burst read data from slave device.
//------------------------------------------------------------------------------
/** 
 * @brief The function burst read data from slave device. (reg address will auto increament)
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address.
 * @param[in] usData    : stands for the address to stored data.
 * @param[in] usDataCnt : stands for the set count to be read.
 * @return It return the function status. 
 */
#if RD_EN==1
MMP_ERR MMPF_I2cm_ReadBurstData(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT usReg, MMP_USHORT *usData, MMP_UBYTE usDataCnt)
{
	MMP_USHORT  i;
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_UBYTE   ubSwIdx;
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG	ulI2cmTimeOut = 0;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut\r\n"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);
	
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) 
	{
	    ubSwIdx = SW_I2CM_IDX(pI2cmAttr->i2cmID);

		if (pI2cmAttr->bRfclModeEn == MMP_FALSE) {
    	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
    	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx]);
    	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
    	    
    		if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    		    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)(usReg >> 8));
    	    	status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		}
    	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, (MMP_UBYTE)usReg);
    	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
    		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);

    	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
        }
        
	    status |= MMPF_SwI2cm_Start(pI2cmAttr);
	    status |= MMPF_SwI2cm_WriteData(pI2cmAttr, m_ubSwI2cmSlaveAddr[ubSwIdx] + 1);
	    status |= MMPF_SwI2cm_GetACK(pI2cmAttr);
		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);

		for (i = 0; i < usDataCnt; i++) 
		{
			usData[i] = 0;
		    if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
			    usData[i] = ((MMP_USHORT)MMPF_SwI2cm_ReadData(pI2cmAttr)) << 8;
		    	status |= MMPF_SwI2cm_SendACK(pI2cmAttr);
		    }
		    usData[i] |= MMPF_SwI2cm_ReadData(pI2cmAttr);
		    
		    if ((usDataCnt > 1) && (i != (usDataCnt -1))) {
		    	status |= MMPF_SwI2cm_SendACK(pI2cmAttr);
		    }
		    RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
	    }
	
	    status |= MMPF_SwI2cm_SendNACK(pI2cmAttr);
		RTNA_WAIT_US(m_ubSwI2cmDelayTime[ubSwIdx]);
	    
	    status |= MMPF_SwI2cm_Stop(pI2cmAttr);
	}
	else 
	{	
		if ((usDataCnt*(m_ubDataLen[pI2cmAttr->i2cmID]>>3)) > I2CM_RX_FIFO_DEPTH) {

			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "Attention! !! OVER I2C RX FIFO SIZE\r\n"); 
            
            #if (I2CM_OS_SEM_PROTECT)
			if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
				if (ubSemStatus == OS_NO_ERR) {
					MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
				}
			}
            #endif

			while (usDataCnt > 4) {
				status |= MMPF_I2cm_ReadBurstData(pI2cmAttr, usReg, usData, 4);
				usReg     = usReg + 4;
				usData    = usData + 4;
				usDataCnt = usDataCnt - 4;
			}
			status |= MMPF_I2cm_ReadBurstData(pI2cmAttr, usReg, usData, usDataCnt);
			return status;
		}
		
		// Rx need to reset FIFO first.
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;

		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);

        if (pI2cmAttr->bRfclModeEn == MMP_FALSE){
    		if (m_ubRegLen[pI2cmAttr->i2cmID] == 16) {
    			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)(usReg >> 8);
    		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg;
    	    }
    		else {
    	    	pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usReg;
    		}
	    }
	    
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (m_ubDataLen[pI2cmAttr->i2cmID]>>3) * usDataCnt;
	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = 1;
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_READ_MODE;
		#if (I2CM_INT_MODE_EN == 1)
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_TX_DONE;
		#endif

		if (pI2cmAttr->bRepeatModeEn == MMP_FALSE) {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL &= ~(I2CM_REPEAT_MODE_EN);
		}
		else {
			pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= (I2CM_REPEAT_MODE_EN);
		}
		
		if (pI2cmAttr->bRfclModeEn == MMP_FALSE) {
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;
		}
		else {
		    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN | I2CM_RFCL_MODE_EN;
		}
		
		#if (I2CM_INT_MODE_EN == 1)
		
		#if (I2CM_OS_SEM_PROTECT)
		if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
			MMPF_OS_AcquireSem(gI2cmIntSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
		}
		#endif
		
		#else
		
		if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
		}
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) 
		{
			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
				ulI2cmTimeOut--;
				
				if (ulI2cmTimeOut == 0) {
					status = MMP_I2CM_ERR_READ_TIMEOUT;
				    goto L_I2cmOut;
				}
				else {
					// Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
					#if (I2CM_OS_SEM_PROTECT)
					MMPF_OS_Sleep(1);
					#else
					MMPC_System_WaitMs(1);
					#endif
				}
			}
		}

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK6\r\n"); 
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK6\r\n"); 
			
			// Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
			
			status = MMP_I2CM_ERR_SLAVE_NO_ACK;
			goto L_I2cmOut;
		}
		#endif
		
	    for (i = 0; i < usDataCnt; i++) {
	    	if (m_ubDataLen[pI2cmAttr->i2cmID] == 16) {
		        usData[i] = (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0] << 8);
		        usData[i] += (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0]);
	    	}
		    else {
		        usData[i] = pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0];
	    	}
	    }      
	}

L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
    #endif

    return status;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_WriteNoRegMode
//  Description : The function write data to slave device without register address.
//------------------------------------------------------------------------------
/** 
 * @brief The function write data to slave device without register address.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usData    : stands for the data to be sent.
 * @return It return the function status. 
 */
MMP_ERR MMPF_I2cm_WriteNoRegMode(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT usData)
{
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG   ulTimeOutTick = I2CM_WHILE_TIMEOUT;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {	
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr,  "I2CM Semaphore TimeOut\r\n"); 
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);
	
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) {
	    // SW I2CM not support No Reg mode
	}
	else 
	{
		if (pI2cmAttr->ubRegLen != 0) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "NoRegMode only support No register address !!\r\n"); 
			status = MMP_I2CM_ERR_PARAMETER;
		    goto L_I2cmOut;
		}
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = (MMP_UBYTE)usData;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = 0; //EROY CHECK
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = 1;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_WRITE_MODE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN)
		{
    		#if (I2CM_OS_SEM_PROTECT)
    		//MMPF_OS_Sleep(1);
    		#else
    		MMPF_SYS_WaitUs(1000, MMP_FALSE, MMP_FALSE);
    		#endif

            ulTimeOutTick--;
    	    if (ulTimeOutTick == 0) {
    	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM While TimeOut\r\n"); 
    	        return MMP_I2CM_ERR_WHILE_TIMEOUT;
        	}
    	}

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK7\r\n"); 
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK7\r\n"); 
			
	        // Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
	        
	        status = MMP_I2CM_ERR_SLAVE_NO_ACK;
	        goto L_I2cmOut;
		}
	}
	
L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);
		}
	}
    #endif

	return status;
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_ReadNoRegMode
//  Description : The function read data from slave device without register address.
//------------------------------------------------------------------------------
/** 
 * @brief The function read data from slave device without register address.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[out] usData   : stands for the received data.
 * @param[in] usDataCnt : stands for the set count to be read.
 * @return It return the function status. 
 */
#if RD_EN==1
MMP_ERR MMPF_I2cm_ReadNoRegMode(MMP_I2CM_ATTR *pI2cmAttr, MMP_USHORT *usData, MMP_UBYTE usDataCnt)
{
	AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
    #if (I2CM_OS_SEM_PROTECT)
	MMP_UBYTE	ubSemStatus = 0xFF;
    #endif
	MMP_ERR		status = MMP_ERR_NONE;
	MMP_ULONG	ulI2cmTimeOut = 0, i = 0;
	
	#if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut");
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);
	
	if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) {
	    // SW I2CM not support No Reg mode
	}
	else 
	{
		if (pI2cmAttr->ubRegLen != 0) {
			MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "NoRegMode only support No register address !!\r\n");
			status =  MMP_I2CM_ERR_PARAMETER;
		    goto L_I2cmOut;
		}
		
		// Rx need to reset FIFO first.	
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);
		
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_TXFIFO_DATA.B[0] = usDataCnt; 
	    
	    pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = 1;
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_READ_MODE;	
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= (I2CM_MASTER_EN | I2CM_RFCL_MODE_EN);
		
		#if (I2CM_INT_MODE_EN == 0)
		if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
		}
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) 
		{
			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
				ulI2cmTimeOut--;
				
				if (ulI2cmTimeOut == 0) {
				    status = MMP_I2CM_ERR_READ_TIMEOUT;
					goto L_I2cmOut;
				}
				else {
					// Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
					#if (I2CM_OS_SEM_PROTECT)
					MMPF_OS_Sleep(1);
					#else
					MMPC_System_WaitMs(1);
					#endif
				}
			}
		}

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL,  pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK8\r\n");
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK8\r\n");
	        
	        // Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
	        
	        status = MMP_I2CM_ERR_SLAVE_NO_ACK;
	        goto L_I2cmOut;
		}
		#endif
		
		for (i = usDataCnt; i > 0; i--) {
			*usData = pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RXFIFO_DATA.B[0];
			usData++;
		}	
	}

L_I2cmOut:
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
    #endif
    
    return status;
}
#endif

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_DMAWriteBurstData
//  Description : The function write data to slave device using DMA burst mode.
//------------------------------------------------------------------------------
/** 
 * @brief The function write data to slave device using DMA burst mode.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address.
 * @param[in] usData    : stands for the data to be sent.
 * @param[in] usDataCnt : stands for the byte count to be writed.
 * @return It return the function status. 
 */
#if DMAI2C_EN==0
MMP_ERR MMPF_I2cm_DMAWriteBurstData(MMP_I2CM_ATTR   *pI2cmAttr, 
                                    MMP_USHORT      usReg, 
                                    MMP_UBYTE       *usData,
                                    MMP_USHORT      usDataCnt)
{
    #if !defined(MINIBOOT_FW) // Reduce code size for miniboot
    AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
	MMP_UBYTE	ubSemStatus = 0xFF;
    MMP_ERR     status = MMP_ERR_NONE;
    MMP_ULONG   ulTimeOutTick = I2CM_WHILE_TIMEOUT;
    
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut\r\n");	        
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);

    if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) {
        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM DMA only support HW I2C\r\n");
    }
    else
    {
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_TX_DONE;
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);
        
        #if 1 // For Burst mode, the Reg Address is needed
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_REG_ADDR  = usReg;
        #else // For Descriptor mode, the Set Count is needed
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SET_CNT = 1;
        #endif
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_TX_BYTE_CNT    = usDataCnt;  
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_TX_ST_ADDR     = (MMP_ULONG)usData;

        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_CTRL |= I2CM_DMA_TX_EN;
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_WRITE_MODE;
        #if (I2CM_INT_MODE_EN == 1)
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_DMA_TX2FIFO_DONE;
        #endif
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;

        #if (I2CM_INT_MODE_EN == 0)
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN)
		{
            //MMPF_OS_Sleep(1);

            ulTimeOutTick--;

        	if (ulTimeOutTick == 0) {
    	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM While TimeOut\r\n");
    	        return MMP_I2CM_ERR_WHILE_TIMEOUT;
        	}
        }

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK9\r\n");
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK9\r\n");
			
	        // Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
	        
	        status = MMP_I2CM_ERR_SLAVE_NO_ACK;
		    goto L_I2cmOut;
		}
        #endif
    }
    
L_I2cmOut:
    
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
	#endif
    
    return status;
    #else
	return MMP_ERR_NONE;
    #endif    
}

//------------------------------------------------------------------------------
//  Function    : MMPF_I2cm_DMAReadBurstData
//  Description : The function read data from slave device using DMA burst mode.
//------------------------------------------------------------------------------
/** 
 * @brief The function read data from slave device using DMA burst mode.
 *
 * @param[in] pI2cmAttr : stands for the i2cm attribute.       
 * @param[in] usReg     : stands for the register sub address.
 * @param[in] usData    : stands for the buffer address to store data.
 * @param[in] usDataCnt : stands for the byte count to be read.
 * @return It return the function status. 
 */

MMP_ERR MMPF_I2cm_DMAReadBurstData(MMP_I2CM_ATTR    *pI2cmAttr,
                                   MMP_USHORT       usReg, 
                                   MMP_UBYTE        *usData,
                                   MMP_USHORT       usDataCnt)
{
    #if !defined(MINIBOOT_FW) // Reduce code size for miniboot
    AITPS_I2CM  pI2CM = AITC_BASE_I2CM;
	MMP_UBYTE	ubSemStatus = 0xFF;
    MMP_ERR     status = MMP_ERR_NONE;
    MMP_ULONG	ulI2cmTimeOut = 0;
    
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		ubSemStatus = MMPF_OS_AcquireSem(gI2cmSemID[pI2cmAttr->i2cmID], I2CM_SEM_TIMEOUT);
	    if (ubSemStatus) {
	        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM Semaphore TimeOut\r\n");
	        return MMP_I2CM_ERR_SEM_TIMEOUT;
	    }
	}
	#endif
	
	status |= MMPF_I2cm_Initialize(pI2cmAttr);

    if ((pI2cmAttr->i2cmID == MMP_I2CM_GPIO) || (pI2cmAttr->i2cmID == MMP_I2CM_SNR)) {
        MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2CM DMA only support HW I2C\r\n");
    }
    else
    {
        // Rx need to reset FIFO first.	
		pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
    
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR = I2CM_SLAVE_NO_ACK | I2CM_DMA_RX2MEM_DONE;
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR &= ~(I2CM_RW_MODE_MASK);

        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_REG_ADDR  = usReg;
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_RX_BYTE_CNT    = usDataCnt;  
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_RX_ST_ADDR     = (MMP_ULONG)usData;

        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_DMA_MODE_CTRL |= I2CM_DMA_RX_EN;
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_SLAVE_ADDR |= I2CM_READ_MODE;
        #if (I2CM_INT_MODE_EN == 1)
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_EN |= I2CM_DMA_RX2MEM_DONE;
        #endif
        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL |= I2CM_MASTER_EN;

        #if (I2CM_INT_MODE_EN == 0)
		if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
			ulI2cmTimeOut = m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID];
		}
		
		while (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_CTL & I2CM_MASTER_EN) 
		{
			if (m_ulI2cmRxReadTimeout[pI2cmAttr->i2cmID] != 0) {
				ulI2cmTimeOut--;
				
				if (ulI2cmTimeOut == 0) {
				    status = MMP_I2CM_ERR_READ_TIMEOUT;
					goto L_I2cmOut;
				}
				else {
					//Note, to use RxTimeout function, please make sure the read operations work in "task mode" instead of "ISR mode"
					#if (I2CM_OS_SEM_PROTECT)
					MMPF_OS_Sleep(1);
					#else
					MMPC_System_WaitMs(1);
					#endif
				}
			}
		}

		if (pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_INT_CPU_SR & I2CM_SLAVE_NO_ACK) {
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->i2cmID, "I2C Error SLAVE_NO_ACK10\r\n");
            MMP_PRINT_RET_ERROR(I2CM_DEBUG_LEVEL, pI2cmAttr->ubSlaveAddr, "I2C Error SLAVE_NO_ACK10\r\n");
			
	        //Clear FIFO. Otherwise, keeping for next transmission.
	        pI2CM->I2CMS[pI2cmAttr->i2cmID].I2CM_RST_FIFO_SW = I2CM_FIFO_RST;
	        
	        status = MMP_I2CM_ERR_SLAVE_NO_ACK;
	        goto L_I2cmOut;
		}
        #endif
    }

L_I2cmOut:
    
    #if (I2CM_OS_SEM_PROTECT)
	if (pI2cmAttr->bOsProtectEn == MMP_TRUE) {
		if (ubSemStatus == OS_NO_ERR) {
			MMPF_OS_ReleaseSem(gI2cmSemID[pI2cmAttr->i2cmID]);   
		}
	}
	#endif
    
    return status;
    #else
	return MMP_ERR_NONE;
    #endif    
}
#endif

/** @}*/ //end of MMPF_I2CM
