
#ifndef DUALCPU_I2C_H
#define DUALCPU_I2C_H
#define I2C_MAX_DEVICES 10



typedef struct i2c_info_s
{
  int  id ;
  char name[32] ;
  unsigned int slave_addr;
  unsigned short addr_len  ;  
  unsigned short data_len  ;
} i2c_info_t ;

typedef struct i2c_device_s
{
  //i2c_info_t i2c_info ;
  int id ;
  char *name ;
  unsigned short slave_addr;
  unsigned short addr_len;
  unsigned short data_len;
  int (*probe)(void *driver_data);
  int (*read)(void *driver_data ,unsigned short addr, unsigned short *data);
  int (*write)(void *driver_data ,unsigned short addr, unsigned short data); 
  void *driver_data   ;
} i2c_device_t ;


typedef struct i2c_ipc_rw_s 
{ 
    int id ;
    unsigned short addr ;
    unsigned short data ;
} i2c_ipc_rw_t ;

typedef struct i2c_ipc_info_s
{
  int devices ;
  i2c_info_t info[I2C_MAX_DEVICES] ;
} i2c_ipc_info_t ;

int I2C_Query(i2c_ipc_info_t *ipc_info) ;
int I2C_Read(i2c_ipc_rw_t *rw);
int I2C_Write(i2c_ipc_rw_t *rw);

int I2C_Register(i2c_device_t *dev) ;

#endif
