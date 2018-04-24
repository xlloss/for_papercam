#include "mmu.h"
#include "cfgparser.h"
#include "config_fw.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if (USER_STRING)
/* #include "ait_utility.h" */
extern void small_sprintf(char* s,char *fmt, ...);
#define sprintf small_sprintf
/* extern size_t atoi(char *p); */
/* extern int strlen (char *str); */
extern char *strcat(char* s, const char* t);
extern int strcmp(const char* s1, const char* s2);
char *strcpy(char *d, const char *s);
#endif
//#define DEF_USR_CFG_MEM_ADDR    (0x01000000 + (62 * 1024 - 16 - 260 ) * 1024)

/* extern MMP_ULONG Image$$BOOT_MENU$$Base ; */
extern unsigned char* __BOOT_START__; 
/* #define DEF_USR_CFG_MEM_ADDR (MMP_ULONG)&Image$$BOOT_MENU$$Base */
#define DEF_USR_CFG_MEM_ADDR ((MMP_ULONG) &__BOOT_START__)
#define DEF_USR_CFG_SIZE        (8*1024)

static  cfg_data_t cfg_data ;
static  cfg_data_t cur_cfg_section  ;
static  char *cfg_update_data,*cfg_update_ptr ;
static int readchar(char *data,char *c,unsigned int *cur_pos,unsigned int data_size);
static char *readline(char *data,unsigned int data_size,unsigned int *cur_pos);
static cfg_data_t *get_cur_cfg_section(void);
static  unsigned char str2hexnum(unsigned char c) ;
static  unsigned int str2hex(unsigned char *str) ;



int init_cfg_parser(char *cfg,int cfg_size )
{
    char *ptr ;
    strcpy(cfg_data.name,"[all]");
    cfg_update_ptr = 0 ;
    cfg_update_data = 0;
    if(!cfg) {
        cfg = (char *)DRAM_NONCACHE_VA(DEF_USR_CFG_MEM_ADDR );
        //printc("cfg.addr : 0x%08x\r\n", (MMP_ULONG)cfg);
        
    }
    if(!cfg_size) {
        cfg_size = DEF_USR_CFG_SIZE ;
    }
    
    /*
    Fixed bug: force to add "[end] in the buffer end
    */
    ptr = cfg + cfg_size - 16 ;
    strcpy(ptr,"\n[end]\n");
    
    if(cfg && cfg_size) {
        cfg_data.data = cfg ;
        cfg_data.size = cfg_size ;
        if( !switch_cfg_section("[end]") ) {
          cfg_data.size = cur_cfg_section.data - cfg_data.data ;
          
        }
        return 0 ;
    }
    
    cfg_data.data = (char *)0 ;
    cfg_data.size = 0 ;
    MEMSET( &cur_cfg_section,0,sizeof(cfg_data_t) ) ;
    return -1 ;
}

int get_cfg_size(void)
{
  //printf("Cfg data size:%d\n",cfg_data.size );
  return cfg_data.size ;
}

int switch_cfg_section(char *sec_name)
{
    char *line ;
    unsigned int cur_pos = 0 ;
    cfg_data_t *sect = get_cur_cfg_section(); 
    if(!cfg_data.data) {
        return -1 ;
    }
    while(	(line = (char *)readline( cfg_data.data ,cfg_data.size ,&cur_pos ))!=0 ) {
        //printc( "parsing : %s\r\n",line);
        if(cfg_update_data) {
          sprintf(cfg_update_ptr,"%s\r\n",line);
          cfg_update_ptr+=strlen(cfg_update_ptr);
        }
        if(line[0]!='#') {
            if (! strncmp(line,sec_name,strlen(sec_name) ) ) {
                memset(sect,0,sizeof(cfg_data_t) ) ;
                strcpy(sect->name,sec_name);
                sect->data = cfg_data.data + cur_pos ;
                sect->size = cfg_data.size - cur_pos ;
                return 0 ;
            }
            if( ! strncmp( line , "[end]",strlen("end]")) ) {
                
                return -2 ; // end
            }
        }
    }     
    return -1 ;   
}

    

int get_cfg_data(CFG_DATA_T type,char *name, void *data)
{
    char *line=0 ;
    unsigned int cur_pos = 0 /*,len = 0 */;
    char item_name[64] ;
    cfg_data_t *sect = get_cur_cfg_section();
    if( !sect->data || !sect->size ) {
        return -1 ;
    }
    item_name[0] = 0 ;
    while(	(line = (char *)readline( sect->data ,sect->size ,&cur_pos )) !=0 ) {
        //printf( "parsing : %s\r\n",line);
        
        if(line[0]!='#') {
          if( line[0] == '[' ) {
              char *ptr = line+1;    
              while (*ptr) {
                  if(*ptr==']') {
                      return -2 ;
                  }
                  ptr++ ;
              }
          }
            #if (USER_STRING)
            size_t len=strlen(line);
            char localstr[len+1]; 
            strcpy(localstr, line);
            strcpy(item_name, strtok(localstr, "="));
            #else
        	/*len = */sscanf(line,"%[^=]",  item_name  );
            #endif
        	line += strlen(item_name);
        	while(   *line  ) {
        		if( (*line==' ' ) || (*line=='=' )) {
        			line++;
        			break ;
        		}
        		line++;
        	}
          if (!strncmp(item_name,name,strlen(name)) ) {
              switch(type) {
                  case CFG_INTEGER:
                  {
                    int hex = 0 /*, neg = 0*/;
                   	if(*line=='0') {
                  		if( (*(line+1)=='x' ) || (*(line+1)=='X' ) ) {
                  		    unsigned int *d = (unsigned int *)data ;
                  			line+=2 ;
                  			*d = str2hex( (unsigned char *)line );
                  			hex = 1 ;
                  		}
                  	}
                  	
                      if(!hex) {
                          int *d = (int *)data ;
                          #if 0
                          if (*line=='-') {
                              line++;
                              neg = 1 ;
                          }
                          #endif
                          *d = atoi( line );
                      }                	
                  	
                     
                  }
                  break;
                  case CFG_STRING:
                  {
                      char *ptr = (char *)data;
                      strcpy(ptr,line );
                  }
                  break;
              }
              return 0 ;
          }
        }
    }     
    return -1 ;
}

/*
 * int set_cfg_data(char *name, char *data)
 * {
 *     int set = 0 ;
 *     char *line=0 ;
 *     unsigned int cur_pos = 0 [>,len = 0 <];
 *     char item_name[64] ;
 *     cfg_data_t *sect = get_cur_cfg_section();
 *     if( !sect->data || !sect->size ) {
 *         return -1 ;
 *     }
 *     item_name[0] = 0 ;
 *     while(	(line = (char *)readline( sect->data ,sect->size ,&cur_pos )) !=0 ) {
 *         //printf( "parsing : %s\r\n",line);
 *         if(line[0]!='#') {
 *             [>len = <]sscanf(line,"%[^=]",  item_name  );
 *             line += strlen(item_name);
 *             while(   *line  ) {
 *                 if( (*line==' ' ) || (*line=='=' )) {
 *                     line++;
 *                     break ;
 *                 }
 *                 line++;
 *             }
 *           if ( !strncmp(item_name,name,strlen(name)) && !set ) {
 *               sprintf(cfg_update_ptr,"%s=%s\r\n",name,(char *)data);
 *               set = 1 ;
 *           }
 *           else {
 *             if( ! strncmp( item_name , "[end]",strlen("end]")) ) {
 *               sprintf(cfg_update_ptr,"%s\r\n",item_name);  
 *             }
 *             else {
 *               sprintf(cfg_update_ptr,"%s=%s\r\n",item_name,line);
 *             }
 *           }
 *           cfg_update_ptr+=strlen(cfg_update_ptr);
 *         }
 *         else {
 *           sprintf(cfg_update_ptr,"%s\r\n",line);
 *           cfg_update_ptr+=strlen(cfg_update_ptr);          
 *         }
 *           
 *     }     
 *     return 0 ;
 * }
 */


/*
 * int update_cfg_data(char *data,char *sect,char *name,char *val)
 * {
 *   cfg_update_data = data ;
 *   cfg_update_ptr  = data ;
 *   if( switch_cfg_section(sect) < 0 ) {
 *     //printf("can't find section : %s\n",sectn);
 *     return -1 ;
 *   }
 *   return set_cfg_data(name,val); 
 * 
 * }
 */

static cfg_data_t *get_cur_cfg_section(void)
{
    return (cfg_data_t *)&cur_cfg_section ;    
}

static int readchar(char *data,char *c,unsigned int *cur_pos,unsigned int data_size)
{
    if( *cur_pos == (data_size) ) {
        return -1 ;
    }
    *c = data[*cur_pos] ;
    *cur_pos+=1;
    return 0 ;
}

static char *readline(char *data,unsigned int data_size,unsigned int *cur_pos)
{
	static char		line[128];
	char			lc;
	unsigned int	c;
	
	c = 0;
	do {
		//if (MMPF_FS_FRead(fh, &lc, 1, &rb) != MMP_ERR_NONE) {
		if ( readchar(data, &lc ,cur_pos, data_size) ) {
			if (c != 0) {
				line[c] = 0x00;
				break;
			}
			return 0;
		}
		if (lc == 0x0d || lc == 0x0a) {
			if (c != 0) {
				line[c] = 0x00;
				break;
			}
			continue;
		}
		line[c++] = lc;
	} while (c < 127);
	line[127] = 0 ;
	return (char*)line;
}

static  unsigned char str2hexnum(unsigned char c)
{
  if (c >= '0' && c <= '9')
          return c - '0';
  if (c >= 'a' && c <= 'f')
          return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
          return c - 'A' + 10;

  return 0; /* foo */
}
 
static  unsigned int str2hex(unsigned char *str)
{
	int value = 0;
	while (*str) {
		 value = value << 4;
		value |= str2hexnum(*str++);
	}

	return value;
}

