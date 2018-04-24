#include "mmpf_i2cm.h"
#include "mmpf_pio.h"
#include "pas7671.h"

#if SUPPORT_PAS7671==1
//#define PAS7671_GPIO_INT   MMP_GPIO10

static MMP_I2CM_ATTR PAS7671_Attr ={
  MMP_I2CM0,   // I2CM interface, depend on used IO pads 
  (0x40),                          // I2CM slave address, please check device¡¦s spec.
  8,                                // I2CM address bits
  8,                                // I2CM data bits
  0,
  MMP_FALSE, 
  MMP_TRUE,
  MMP_FALSE,
  MMP_FALSE, 
  0,
  0, 
  0,                                // I2CM pad number
  250000/*250KHZ*/,    // 250KHz, up to 400KHz
  MMP_TRUE,              // Semaphore protect or not
  0,//NULL, 
  0,//NULL,
  FALSE,
  FALSE,
  FALSE,
  0
};

static int PAS7671_en = 0 ;
int  PAS7671_Status(void)
{
  MMP_USHORT val = 0 ;
  if ( PAS7671_en ) {
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x7f, 0x00 );
    MMPF_I2cm_ReadReg ( &PAS7671_Attr ,0x14, &val );
  }
  return val ;
}

void PAS7671_Ack()
{
  if(PAS7671_en) {
    printc("PAS7671_Ack\r\n");
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x7f,0x00 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x04,0x00 ); 
  }
}

void PAS7671_Init(int enable)
{
  MMP_USHORT val = 0 ;
  if(!enable) {
    return ;  
  }
  PAS7671_en = enable ;
  val  = PAS7671_Status();
  if(val==2) {
    printc("PAS7671 Alive\r\n");
    PAS7671_Ack();
  }
  else if (val==0) {
    printc("PAS7671 Init0\r\n");
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x7f,0x06 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x74,0x00 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x7f,0x00 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x0a,0x00 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x0a,0x01 );
    MMPF_OS_Sleep(100);
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x7f,0x00 );
    MMPF_I2cm_WriteReg( &PAS7671_Attr ,0x03,0x02 );
    
    //PAS7671_Ack();
  }
  else {
    printc("PAS7671 status:%d\r\n",val);
    PAS7671_Ack();
  }
}
#endif
