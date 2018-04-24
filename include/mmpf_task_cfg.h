//==============================================================================
//
//  File        : mmpf_task_cfg.h
//  Description : Task configuration file for A-I-T MMPF source code
//  Author      : Philip Lin
//  Revision    : 1.0
//
//==============================================================================

#ifndef _MMPF_TASK_CFG_H_
#define _MMPF_TASK_CFG_H_

#include "all_fw.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
/* System task has the highest priority */
#define	TASK_SYS_PRIO				(1)

/* Task stack size for system & low priority task */
#define TASK_SYS_STK_SIZE           (1024)
#define LTASK_STK_SIZE              (128)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MMPF_TASK_CFG_H_ */
