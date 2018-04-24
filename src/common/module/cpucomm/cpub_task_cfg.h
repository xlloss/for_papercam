/*$T /cpub_task_cfg.h GC 1.150 2016-08-25 18:31:24 */


/*$6*/


#ifndef __CPU_B_TASK_CONFIG_H__
#define __CPU_B_TASK_CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

//==============================================================================
//
//                              CPU B's task priority define
//
//==============================================================================
#define TASK_B_SYS_PRIO				(1)
#define TASK_B_AEC_PRIO				(3)
#define TASK_B_AAC_PRIO				(4)
#define TASK_B_AES_PRIO				(5)
    //redifine on all_fw.h
// #define TASK_B_MD_PRIO				(10)
#define TASK_B_I2C_PWM_TEST_PRIO    (6)
#define TASK_B_UART_TEST_PRIO		(2)
#define TASK_B_A2B_PRIO				(20)
#define TASK_B_SOCKET_PRIO			(11)
#define TASK_B_CPU_SHAREMEM_R_PRIO	(19)
#define TASK_B_CPU_SHAREMEM_S_PRIO	(18)
#define TASK_B_CPU_SHAREMEM_S1_PRIO (17)
#define TASK_B_CPU_SHAREMEM_R1_PRIO (16)

//==============================================================================
//
//                              CPU B's task stack size define
//
//==============================================================================
// #define TASK_B_SYS_STK_SIZE				(128)
#define TASK_B_I2C_PWM_TEST_STK_SIZE	(1024)
#define TASK_B_UART_TEST_STK_SIZE		(1024)
// #define TASK_B_MD_STK_SIZE				(2048)
#define TASK_B_AEC_STK_SIZE				(2048)
#define TASK_B_A2B_STK_SIZE				(1024)
#define TASK_B_AAC_STK_SIZE				(2048)
#define TASK_B_SOCKET_STK_SIZE			(2048)
#define TASK_B_AES_STK_SIZE				(1024)
#define TASK_B_CPU_SHAREMEM_R_STK_SIZE	(2048)
#define TASK_B_CPU_SHAREMEM_S_STK_SIZE	(2048)
#define TASK_B_CPU_SHAREMEM_S1_STK_SIZE (2048)
#define TASK_B_CPU_SHAREMEM_R1_STK_SIZE (2048)
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
