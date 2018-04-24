#ifndef _CFGPARSER_H
#define _CFGPARSER_H
typedef struct _cfg_data_t
{
    char name[32];
    char *data;
    unsigned int size ;
    unsigned int cur_pos ;
} cfg_data_t ;

typedef enum 
{
    CFG_STRING,
    CFG_INTEGER
} CFG_DATA_T ;

int init_cfg_parser(char *cfg,int cfg_size ) ;
int switch_cfg_section(char *sec_name) ;
int get_cfg_data(CFG_DATA_T type,char *name, void *data);
int get_cfg_size(void) ;
int update_cfg_data(char *data,char *sect,char *name,char *val) ;
int set_cfg_data(char *name, char *data);
#endif