/**
  @file ait_utility.c
  @brief It contains the utility functions need to be ported to the customer platform.
  @author Rogers Chen, Truman

  We should implement regular functions (MMP initialization),
  callback functions (LCD, Audio CODEC initlization) &
  retarget functions (memset, strcat... etc.)

  @version
- 2.0 Add customization functions, such as Bypass pin controller, LCD Initializer
- 1.0 Original version
 */

//==============================================================================
//
//                              INCLUDE FILE
//
//==============================================================================

#include "mmp_lib.h"
#include "ait_utility.h"
#include "mmp_reg_gbl.h"

//==============================================================================
//
//                              VARIABLES
//
//==============================================================================

//==============================================================================
//                              RETARGET FUNCTIONS
//==============================================================================
/** @brief Convert Unicode encoding string to ASCII string.
*/
char *strfromUCS(char *dest, const char *src)
{
    char *dst = dest;

    while(*src != '\0') {
        *dest = *src;
        dest++;
        src+=2;
    }
    *dest = '\0';

    return dst;
}
/** @brief Convert ASCII string to Unicode encoding string.
*/
char *strtoUCS(char *dest, const char *src)
{
    char *dst = dest;

    while(*src != '\0') {
        *dest++ = *src++;
        *dest++ = '\0';
    }
    *dest++ = '\0';
    *dest = '\0';

    return dst;
}
/** @brief Copy a Unicode encoding string.

Copies the string pointed to by src (including the terminating '\0\0')
to the array pointed to by dest. The strings may not overlap,
and the destination string dest must be large enough to receive the copy.
*/
char *uniStrcpy(char *dest, const char *src)
{
    short *s = (short *)src, *d = (short *)dest;

    while(*s != 0) {
        *d++ = *s++;
    }
    *d = 0;

    return dest;
}
/** @brief Calculate the length of a Unicode encoding string.

Calculates the length (in byte) of the Unicode encoding string src,
not including the terminating '\0' character.
*/
int uniStrlen(const short *src)
{
    int ulLen=0;

    while(*src != '\0') {
        src++;
        ulLen++;
    }

    return (ulLen << 1);
}
/** @brief Concatenate two strings

Appends the Unicode encoding src2 string to the Unicode encoding str1 string
overwriting the '\0\0' at the end of dest, and then adds a terminating '\0\0'.
The strings may not overlap, and the str1 string must have enough space for the result.
*/
char *uniStrcat(char *str1, const char *str2)
{
    short *s = (short *)str2, *d = (short *)str1;

    while(*d != 0) {
        d++;
    }
    while(*s != 0) {
        *d++ = *s++;
    }
    *d = 0;

    return str1;
}


//------------------------------------------------------------------------------
//  Function    : MMPC_TransfomTime2Stamp
//  Description :
//------------------------------------------------------------------------------
void MMPC_TransfomTime2Stamp(int YY, int MM, int DD, int hh, int mm, int ss, MMP_BYTE* pDate)
{
    MEMCPY(pDate, "2016:01:01 00:00:00", 20);
}
