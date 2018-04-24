/*$T /UartShell.c GC 1.150 2016-08-25 18:31:24 */

/*******************************************************************************
 *
 *   UartShell.c
 *
 *   Implementation of UartShell.
 *
 *   Copyright 2010 by Caesar Chang.
 *
 *
 *   AUTHOR : Caesar Chang
 *
 *   VERSION: 1.0
 *
 *
*******************************************************************************/
#include "includes_fw.h"
#include "UartShell.h"
#include "mmpf_uart.h"

/*******************************************************************************
 *
 *   DEFINITIONS OF PRINTC
 *
*******************************************************************************/

// The max digits for a 32bit integer
#define DIGIT_STRING_LENGTH_OF_INT32	12
#define IS_A_DIGIT(x)					((x >= '0') && (x <= '9'))
#define MAX_PRINTF_OUTPUT_STRING		128
#define SUPPORT_CHAR_FILL

/*******************************************************************************
 *
 *   MARCOs
 *
*******************************************************************************/
#define IS_WHITE(x) ((x) == ' ' || (x) == '\t')
#define EAT_WHITE(x) \
	while(IS_WHITE(*(x))) x++;
#define EAT_NON_WHITE(x) \
	while(!IS_WHITE(*(x))) x++;
#define IS_A_DIGIT(x)	((x >= '0') && (x <= '9'))
#define EAT_REST_STRING(x) \
	while((*x) != '\0' && *(x) != '\"') x++;

/*******************************************************************************
 *
 *   FUNCTION
 *
 *      printc
 *
 *   DESCRIPTION
 *
 *      a lite printf command. Support %d , %x , %X , %u, %s, %c.
 *
 *   ARGUMENTS
 *
 *      N/A
 *
 *   RETURN
 *   
 *      N/A
 *
*******************************************************************************/
//void	__UartWrite(const char *pWrite_Str);

/* */
#if (ITCM_PUT)
/* static void printchar(char **str, int c) ITCMFUNC; */
/* static int prints(char **out, const char *string, int width, int pad) ITCMFUNC; */
/* static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase) ITCMFUNC; */
/* static int print(char **out, const char *format, va_list args) ITCMFUNC; */
int small_sprintf(char *out, const char *format, ...) ITCMFUNC;
int sprintf(char *out, const char *format, ...) ITCMFUNC;
int sprintc(char *out, const char *format, va_list args) ITCMFUNC;
void printc(char *szFormat, ...) ITCMFUNC;
#endif 

static void printchar(char **str, int c)
{
	if(str)
	{
		**str = c;
		++(*str);
	}
}

#define PAD_RIGHT	1
#define PAD_ZERO	2

/* */
static int prints(char **out, const char *string, int width, int pad)
{
	register int	pc = 0, padchar = ' ';

	if(width > 0)
	{
		register int		len = 0;
		register const char *ptr;
		for(ptr = string; *ptr; ++ptr) ++len;
		if(len >= width)
			width = 0;
		else
			width -= len;
		if(pad & PAD_ZERO) padchar = '0';
	}

	if(!(pad & PAD_RIGHT))
	{
		for(; width > 0; --width)
		{
			printchar(out, padchar);
			++pc;
		}
	}

	for(; *string; ++string)
	{
		printchar(out, *string);
		++pc;
	}

	for(; width > 0; --width)
	{
		printchar(out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN	12
unsigned divu10(unsigned n) {
 unsigned q, r;
 q = (n >> 1) + (n >> 2);
 q = q + (q >> 4);
 q = q + (q >> 8);
 q = q + (q >> 16);
 q = q >> 3;
 r = n - q*10;
 return q + ((r + 6) >> 4);
// return q + (r > 9);
}

int remu10(unsigned n) {
     static char table[16] = {0, 1, 2, 2, 3, 3, 4, 5,
          5, 6, 7, 7, 8, 8, 9, 0};
      n = (0x19999999*n + (n >> 1) + (n >> 3)) >> 28;
       return table[n];
}

/* */
static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
	char					print_buf[PRINT_BUF_LEN];
	register char			*s;
	register int			t, neg = 0, pc = 0;
	register unsigned int	u = i;

	if(i == 0)
	{
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints(out, print_buf, width, pad);
	}

	if(sg && b == 10 && i < 0)
	{
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN - 1;
	*s = '\0';

	while(u)
	{
        #if(USE_DIV_CONST)
        if(b==10) 
        {
            t = remu10(u);
            *--s = t + '0';
            u = divu10(u);
        }
        else if(b==16)
        {
            t = u & 0xf;
            if(t >= 10) t += letbase - '0' - 10;
            *--s = t + '0';
            u = u >> 4;
        }
        #else
            t = u % b;
            if(t >= 10) t += letbase - '0' - 10;
            *--s = t + '0';
            u /= b;
        #endif
	}

	if(neg)
	{
		if(width && (pad & PAD_ZERO))
		{
			printchar(out, '-');
			++pc;
			--width;
		}
		else
		{
			*--s = '-';
		}
	}

	return pc + prints(out, s, width, pad);
}

/* */
static int print(char **out, const char *format, va_list args)
{
	register int	width, pad;
	register int	pc = 0;
	char			scr[2];

	for(; *format != 0; ++format)
	{
		if(*format == '%')
		{
			++format;
			width = pad = 0;
			if(*format == '\0') break;
			if(*format == '%') goto out;
			if(*format == '-')
			{
				++format;
				pad = PAD_RIGHT;
			}

			while(*format == '0')
			{
				++format;
				pad |= PAD_ZERO;
			}

			for(; *format >= '0' && *format <= '9'; ++format)
			{
				width *= 10;
				width += *format - '0';
			}

			if(*format == 's')
			{
				register char	*s = (char *) va_arg(args, int);
				pc += prints(out, s ? s : "(null)", width, pad);
				continue;
			}

			if(*format == 'd')
			{
				pc += printi(out, va_arg(args, int), 10, 1, width, pad, 'a');
				continue;
			}

			if(*format == 'x')
			{
				pc += printi(out, va_arg(args, int), 16, 0, width, pad, 'a');
				continue;
			}

			if(*format == 'X')
			{
				pc += printi(out, va_arg(args, int), 16, 0, width, pad, 'A');
				continue;
			}

			if(*format == 'u')
			{
				pc += printi(out, va_arg(args, int), 10, 0, width, pad, 'a');
				continue;
			}

			if(*format == 'c')
			{
				/* char are converted to int then pushed on the stack */
				scr[0] = (char) va_arg(args, int);
				scr[1] = '\0';
				pc += prints(out, scr, width, pad);
				continue;
			}
		}
		else
		{
out:
			printchar(out, *format);
			++pc;
		}
	}

	if(out) **out = '\0';
	va_end(args);
	return pc;
}

int small_sprintf(char *out, const char *format, ...)
{
    va_list args;
    va_start( args, format );
    return print( &out, format, args );
}
//use for IaaAec_Init function which calling sprintf to link ourself sprintf
#if (SUPPORT_AEC) && (USER_STRING)
int sprintf(char *out, const char *format, ...)
{
    va_list args;
    va_start( args, format );
    return print( &out, format, args );
}
#endif

/* */
int sprintc(char *out, const char *format, va_list args)
{
	return print(&out, format, args);
}

/* */
void dbg_printf(char *fmt, ...)
{
	va_list arg_list;

	// Output buffer to UART
	char	szOutput[MAX_PRINTF_OUTPUT_STRING];

	va_start(arg_list, fmt);
	sprintc(szOutput, fmt, arg_list);
	va_end(arg_list);

	//__UartWrite( szOutput );//    RTNA_DBG_Str0(szOutput);
}


/* */
#if ( uart_printc == 1)
void printc(char *szFormat, ...)
{
	va_list arg_list;

	// Output buffer to UART
	char	szOutput[MAX_PRINTF_OUTPUT_STRING];

	va_start(arg_list, szFormat);
	sprintc(szOutput, szFormat, arg_list);
	va_end(arg_list);

    //__UartWrite(szOutput);	//    RTNA_DBG_Str0(szOutput);
	MMPF_Uart_Write(DEBUG_UART_NUM, szOutput);
}
#else
void printc(char *szFormat, ...)
{
    RTNA_DBG_Str0("printc null \r\n");
}
#endif

#if (USER_STRING)
/* int  strlen (const char *str)  */
/* { */
    /* int len = 0; */
    /* while (*str != '\0') { */
        /* str++; */
        /* len++; */
    /* } */
    /* return len; */
/* } */


char *strcat(char* s, const char* t)
{
  int i = 0;

  while (s[i] != '\0')
    i++;
  while (*t != '\0')
    s[i++] = *t++;
  s[i] = '\0'; //useless because already initialized with 0
  return (s);
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1==*s2))
        s1++,s2++;
    return *(const unsigned char*)s1-*(const unsigned char*)s2;
}

char *strcpy(char *d, const char *s)
{
   char *saved = d;
   while (*s)
   {
       *d++ = *s++;
   }
   *d = 0;
   return saved;
}



/* size_t atoi(char *p) { */
    /* int k = 0; */
    /* while (*p) { */
        /* k = (k<<3)+(k<<1)+(*p)-'0'; */
        /* p++; */
     /* } */
     /* return k; */
/* } */

#endif


#if (USER_LOG)
unsigned int small_log10(unsigned int v)
{
    return (v >= 1000000000u) ? 9 : (v >= 100000000u) ? 8 : 
        (v >= 10000000u) ? 7 : (v >= 1000000u) ? 6 : 
        (v >= 100000u) ? 5 : (v >= 10000u) ? 4 :
        (v >= 1000u) ? 3 : (v >= 100u) ? 2 : (v >= 10u) ? 1u : 0u; 
}
#endif

#if (SUPPORT_MDTC) && (USER_LOG)
unsigned int log10(unsigned int v)
{
    return (v >= 1000000000u) ? 9 : (v >= 100000000u) ? 8 : 
        (v >= 10000000u) ? 7 : (v >= 1000000u) ? 6 : 
        (v >= 100000u) ? 5 : (v >= 10000u) ? 4 :
        (v >= 1000u) ? 3 : (v >= 100u) ? 2 : (v >= 10u) ? 1u : 0u; 
}
#endif
