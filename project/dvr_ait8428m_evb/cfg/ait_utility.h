/** @file ait_utility.h

@brief Header file of the utility
@author Truman Yang
@since 10-Jul-06
@version
- 1.0 Original version
*/

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "mmps_system.h"
#include <string.h>
#include <stdlib.h>

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================
/** @name API Portability
@{ */
    #define FS_USE_ICE  (0)
    #define OMA_DRM_EN  (0)

    #if defined(ALL_FW)
    extern void printc(char *szFormat, ...);
    #define FW_DBG_EN   (0)
        #if (FW_DBG_EN)
        #define PRINTF(...) {printc(__VA_ARGS__);}
        #else
        #define PRINTF(...) {do {} while(0);}
        #endif
    #else
    #define PRINTF(...) {do {} while(0);}
    #endif
    #define SCANF(...) {do {} while(0);}

    /** @brief Implement some standard functions and retarget to it. */
    #define STRLEN strlen
    #define STRCPY strcpy
    #define STRCMP strcmp
    #define STRCAT strcat
    #ifndef MEMCPY
    #define MEMCPY memcpy
    #endif
    #ifndef MEMSET
    #define MEMSET memset
    #endif
    #define MEMCMP memcmp
    #define MEMMOVE memmove
    #ifndef MCP_MMI_FLASH
    #define GETS(c) gets(c)
    #endif
    #define RAND() rand()

/** @} */ // end of API Portability

//==============================================================================
//
//                              STRUCTURES
//
//==============================================================================

typedef MMP_BOOL AllocZeroMemCBFunc(MMP_LONG size, void **ptr);

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

char    *uniStrcpy(char *dest, const char *src);
char    *strtoUCS(char *dest, const char *src);
char    *strfromUCS(char *dest, const char *src);
int     uniStrlen(const short *src);
char    *uniStrcat(char *str1, const char *str2);

#endif  //_UTILITY_H_

