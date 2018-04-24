#include <stdio.h>
#include <stdarg.h>
#include "config_fw.h"
#include "mtd_ota.h"
#include "lib_retina.h"
#if (USER_STRING)
extern size_t strlen (char *str);
extern char *strcat(char* s, const char* t);
extern int strcmp(const char* s1, const char* s2);
extern void small_sprintf(char* s,char *fmt, ...);
#define sprintf small_sprintf
#else
#include <string.h>
#endif

#if MTD_OTA_EN==1
static char *mtd_table_addr = 0 ;
static mtd_desc_t *mtd_desc_addr = 0 ;

static int  mtd_num = 0;
void mtd_set_desc_table(char *addr)
{
    unsigned short *ptr ;
    mtd_table_addr = addr ;
    
    if(mtd_table_addr) {
        ptr = (unsigned short *)mtd_table_addr ;
        if( ptr[0] == MTD_OTA_START_TAG ) {
            mtd_num = ptr[1] ;
            mtd_desc_addr = (mtd_desc_t *)(ptr+2); 
        }
    }
}

mtd_desc_t *mtd_get_desc_n(int n )
{
    mtd_desc_t *desc = mtd_desc_addr ;
    return (mtd_desc_t *)(desc+n);
}

mtd_desc_t *mtd_get_active_desc(char *name)
{
    int i;
    mtd_desc_t *desc = mtd_desc_addr ;
    if(mtd_desc_addr) {
        for(i=0;i<mtd_num;i++) {
        
        
            if(!strcmp((char *)desc->mtd_name,name ) ){
                if(desc->flag & FLAG_ACTIVE) {
                    return desc ;
                }
            }
            desc++ ;
        }
    }   
    return (mtd_desc_t *)0 ;
}

unsigned long mtd_get_active_mtd_offset(char *name)
{
    unsigned long offset=0 ;
    mtd_desc_t *desc = mtd_get_active_desc(name);
    if(desc) {
        offset = ( desc->offset[0] << 0 ) | 
                 ( desc->offset[1] << 8 ) | 
                 ( desc->offset[2] << 16) |
                 ( desc->offset[3] << 24)  ;
    }
    return offset ;
}
#if defined (MBOOT_FW )
char *build_kernel_cmd_line(void)
{
extern MMP_ULONG   gCpuFreqKHz;

#define LPJ_528MHZ 1314816

#if (DRAM_SIZE <= 0x4000000)
#define NON_MTD_PART_CMD_LINE "quiet lpj=%d mem=64M console=ttyS0,115200 rootfstype=squashfs init=/init root=1f%02x mtdparts=spi0.0:"
#elif (DRAM_SIZE <= 0x8000000)
#define NON_MTD_PART_CMD_LINE "quiet lpj=%d mem=128M console=ttyS0,115200 rootfstype=squashfs init=/init root=1f%02x mtdparts=spi0.0:"
#else
#define NON_MTD_PART_CMD_LINE "quiet lpj=%d mem=240M console=ttyS0,115200 rootfstype=squashfs init=/init root=1f%02x mtdparts=spi0.0:"
#endif



static char cmd_line[384] ;
    unsigned int lpj = LPJ_528MHZ ;
    char mtd_line[384] ;
    int i ,root=2;
    mtd_desc_t *desc ;
    char mtd_name[64] ,flag_str[8] ;
    if(!mtd_num) {
        return 0 ;
    }
    mtd_line[0] = 0 ;
    for(i=0;i<mtd_num;i++) {
        desc = mtd_get_desc_n(i) ;
        if(desc) {
            flag_str[0]=0 ;
            sprintf(flag_str,"%s%s%s",(desc->flag & FLAG_ACTIVE)?"a":"",(desc->flag & FLAG_ROOT)?"r":"",(desc->flag & FLAG_EARLY_MKNOD )?"e":"" );
            
            sprintf(mtd_name,"%dK(%s%s%s),",desc->size_kb[0] | (desc->size_kb[1] << 8) , desc->mtd_name, (flag_str[0])?".":"",flag_str );
            strcat(mtd_line,mtd_name); 
            if(desc->flag  &  FLAG_ROOT ) {
                if(desc->flag  &  FLAG_ACTIVE ) {
                    root = i ;
                }
            }
        }    
    }    
    /* #if(USE_DIV_CONST) */
    /* #if (PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_168) */
	/* lpj = ( (504000/1000) * LPJ_528MHZ ) / 528 ; */
    /* #endif */
    /* #if (PLL_CONFIG==PLL_FOR_POWER)||(PLL_CONFIG==PLL_FOR_ULTRA_LOWPOWER_192) || (PLL_CONFIG==PLL_FOR_PERFORMANCE) */
	/* lpj = ( (528000/1000) * LPJ_528MHZ ) / 528 ; */
    /* #endif */
    /* #endif */
	lpj = ( (gCpuFreqKHz/1000) * LPJ_528MHZ ) / 528 ;
    
    sprintf(cmd_line,NON_MTD_PART_CMD_LINE, lpj, root);
    strcat(cmd_line,mtd_line);
    cmd_line[ strlen(cmd_line) - 1 ] = ';' ;
    
    return cmd_line ;
} 
#endif

#endif
