#ifndef _MTD_OTA_H
#define _MTD_OTA_H

#define MTD_OTA_START_TAG 0xaa55
#define MTD_TABLE_LOAD_ADDR (0xFE000)
#define MTD_TABLE_MAX_SIZE  (4096   )

#define FLAG_ACTIVE         (1<<0)
#define FLAG_ROOT           (1<<1)
#define FLAG_EARLY_MKNOD    (1<<2)

typedef struct mtd_desc_s
{
  unsigned char flag ; // 1 : active , 0 : inactive
  unsigned char size_kb[2] ; 
  unsigned char offset[4] ;
  unsigned char reserved[ 5 ] ;
  unsigned char mtd_name[20] ; 
} mtd_desc_t ;
void mtd_set_desc_table(char *addr);
mtd_desc_t *mtd_get_active_desc(char *name);
unsigned long mtd_get_active_mtd_offset(char *name);
char *build_kernel_cmd_line(void);
#endif
