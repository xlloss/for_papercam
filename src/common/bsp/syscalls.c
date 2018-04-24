/*$T /syscalls.c GC 1.150 2016-08-25 18:31:16 */

/* Copyright Statement:
 *
 * @2015 MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek Inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE.
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int __io_putchar (int ch)	__attribute__((weak));
extern int __io_getchar (void)	__attribute__((weak));

int								printc(const char *szfmt, ...);

/* */
int _close(int file)
{
	return 0;
}

/* */
int _fstat(int file, struct stat *st)
{
	return 0;
}

/* */
int _isatty(int file)
{
	return 1;
}

/* */
int _lseek(int file, int ptr, int dir)
{
	return 0;
}

/* */
int _open(const char *name, int flags, int mode)
{
	return -1;
}

/**
 * Quick hack, read bytes from console if requested. Note that 'file' is
 * standard input if its value is 0.
 */
int _read(int file, char *ptr, int len)
{
	if(file)
	{
		return 0;
	}

	if(len <= 0)
	{
		return 0;
	}

	*ptr++ = __io_getchar();

	return 1;
}

/* */
int _write(int file, char *ptr, int len)
{
	int i;

	for(i = 0; i < len; i++)
	{
		__io_putchar(*ptr++);
	}

	return len;
}

/* Register name faking - works in collusion with the linker.  */
#define UNUSED(expr) \
	do \
	{ \
		(void) (expr); \
	} while(0)

	/* */
	caddr_t _sbrk_r(struct _reent *r, int incr)
{
	UNUSED(r);
	UNUSED(incr);
	return 0;
#if 0
	extern char *end asm("end");	/* Defined by the linker.  */
	static char *heap_end = NULL;
	char		*prev_heap_end;

	if(heap_end == NULL)
	{
		heap_end = &end;
	}

	prev_heap_end = heap_end;

	/* workaround: skip the check if executing in thread mode (psp) */
	if((heap_end < stack_ptr) && (heap_end + incr > stack_ptr))
	{
		/* Some of the libstdc++-v3 tests rely upon detecting
    out of memory errors, so do not abort here.  */
#if 0
		extern void abort(void);
		_write(1, "_sbrk: Heap and stack collision\n", 32);

		abort();
#else
		//errno = ENOMEM;
		return(caddr_t) - 1;
#endif
	}

	heap_end += incr;

	return(caddr_t) prev_heap_end;
#endif
}

/* */
void _exit(int __status)
{
	printc("_exit\n");
	while(1)
	{
		;
	}
}

/* */
int _kill(int pid, int sig)
{
	printc("_kill %d\n", sig);
	return -1;
}

/* */
pid_t _getpid(void)
{
	printc("_getpid\n");
	return 0;
}

//#include "FreeRTOS.h"
//#include "task.h"
#include <sys/time.h>

/* */
int _gettimeofday(struct timeval *tv, void *ptz)
{
	int ticks = 100;	//xTaskGetTickCount();
	if(tv != NULL)
	{
		tv->tv_sec = (ticks / 1000);
		tv->tv_usec = (ticks % 1000) * 1000;
		return 0;
	}

	return -1;
}
void _fini(void)
{
    printc("%s %d \r\n", __func__, __LINE__);
}
