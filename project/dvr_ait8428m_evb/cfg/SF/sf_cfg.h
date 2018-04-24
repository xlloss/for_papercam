
#ifndef _SF_CFG_H_
#define _SF_CFG_H_

//==============================================================================
//
//  File        : sf_cfg.h
//  Description : SPI Nor Flash configure file
//  Author      : Rony Yeh
//  Revision    : 1.0
//==============================================================================

#include "config_fw.h"
#include "mmpf_typedef.h"

//==============================================================================
//
//                              Define
//
//==============================================================================

#if (USING_SF_IF) 
#define SPIMAXMODULENO                  (4)    
//If use new module, create a new define for the module
#define SPI_NORFLASH_DEFAULTSETTING		(1) //must be define to (1),for default parameter setting
#define SPI_NORFLASH_GD_32MB 		    (0)
#define SPI_NORFLASH_MXIC_32MB			(0)
#define SPI_NORFLASH_WBD_32MB			(1)
#define SPI_NORFLASH_WBD_16MB           (1)
#define SPI_NORFLASH_GD_16MB            (1)
#define SPI_NORFLASH_GD_10F256   		(0)
#define SPI_NORFLASH_GD_14FQ32			(0)
#define SPI_NORFLASH_WBD_KF512			(0)
#define SPI_NORFLASH_WBD_55QBG			(0)
#define SPI_NORFLASH_KH_MQB32MB			(0)
#define SPI_NORFLASH_KH_QFN64MB			(0)
#endif

//==============================================================================
//
//                              Extern Variable
//   
//==============================================================================


//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

#endif //_SF_CFG_H_
