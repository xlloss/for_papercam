/**
 * opt3001.c - Texas Instruments OPT3001 Light Sensor
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Andreas Dannenberg <dannenberg@ti.com>
 * Based on previous work from: Felipe Balbi <balbi@ti.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 of the License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include "mmp_lib.h"
#include "misc.h"
#include "mmpf_i2cm.h"

#if SUPPORT_OPT3006==1

#define EBUSY       16
#define EINVAL      22  /* Invalid argument */
#define ETIMEDOUT   110 /* Connection timed out */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BIT(nr)			(1UL << (nr))


#define IIO_VAL_INT 1
#define IIO_VAL_INT_PLUS_MICRO 2
#define IIO_VAL_INT_PLUS_NANO 3
#define IIO_VAL_INT_PLUS_MICRO_DB 4
#define IIO_VAL_INT_MULTIPLE 5
#define IIO_VAL_FRACTIONAL 10
#define IIO_VAL_FRACTIONAL_LOG2 11
#define IIO_VAL_PRELUX  100


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef int      bool ;
#define true  1
#define false 0

#define msleep( x ) MMPF_OS_Sleep( x )

#define _S_IN_IDLE    (0)
#define _S_IN_PRELUX  (1)


enum iio_chan_type {
	IIO_VOLTAGE,
	IIO_CURRENT,
	IIO_POWER,
	IIO_ACCEL,
	IIO_ANGL_VEL,
	IIO_MAGN,
	IIO_LIGHT,
	IIO_INTENSITY,
	IIO_PROXIMITY,
	IIO_TEMP,
	IIO_INCLI,
	IIO_ROT,
	IIO_ANGL,
	IIO_TIMESTAMP,
	IIO_CAPACITANCE,
	IIO_ALTVOLTAGE,
	IIO_CCT,
	IIO_PRESSURE,
	IIO_HUMIDITYRELATIVE,
	IIO_ACTIVITY,
	IIO_STEPS,
	IIO_ENERGY,
	IIO_DISTANCE,
	IIO_VELOCITY,
	IIO_CONCENTRATION,
	IIO_RESISTANCE,
	IIO_PH,
	IIO_UVINDEX,
	IIO_ELECTRICALCONDUCTIVITY,
	IIO_COUNT,
	IIO_INDEX,
	IIO_GRAVITY,
};

enum iio_shared_by {
	IIO_SEPARATE,
	IIO_SHARED_BY_TYPE,
	IIO_SHARED_BY_DIR,
	IIO_SHARED_BY_ALL
};

enum iio_endian {
	IIO_CPU,
	IIO_BE,
	IIO_LE,
};

enum iio_available_type {
	IIO_AVAIL_LIST,
	IIO_AVAIL_RANGE,
};
enum iio_event_type {
	IIO_EV_TYPE_THRESH,
	IIO_EV_TYPE_MAG,
	IIO_EV_TYPE_ROC,
	IIO_EV_TYPE_THRESH_ADAPTIVE,
	IIO_EV_TYPE_MAG_ADAPTIVE,
	IIO_EV_TYPE_CHANGE,
};

enum iio_event_direction {
	IIO_EV_DIR_EITHER,
	IIO_EV_DIR_RISING,
	IIO_EV_DIR_FALLING,
	IIO_EV_DIR_NONE,
};

enum iio_event_info {
	IIO_EV_INFO_ENABLE,
	IIO_EV_INFO_VALUE,
	IIO_EV_INFO_HYSTERESIS,
	IIO_EV_INFO_PERIOD,
	IIO_EV_INFO_HIGH_PASS_FILTER_3DB,
	IIO_EV_INFO_LOW_PASS_FILTER_3DB,
};

enum iio_chan_info_enum {
	IIO_CHAN_INFO_RAW = 0,
	IIO_CHAN_INFO_PROCESSED,
	IIO_CHAN_INFO_SCALE,
	IIO_CHAN_INFO_OFFSET,
	IIO_CHAN_INFO_CALIBSCALE,
	IIO_CHAN_INFO_CALIBBIAS,
	IIO_CHAN_INFO_PEAK,
	IIO_CHAN_INFO_PEAK_SCALE,
	IIO_CHAN_INFO_QUADRATURE_CORRECTION_RAW,
	IIO_CHAN_INFO_AVERAGE_RAW,
	IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY,
	IIO_CHAN_INFO_HIGH_PASS_FILTER_3DB_FREQUENCY,
	IIO_CHAN_INFO_SAMP_FREQ,
	IIO_CHAN_INFO_FREQUENCY,
	IIO_CHAN_INFO_PHASE,
	IIO_CHAN_INFO_HARDWAREGAIN,
	IIO_CHAN_INFO_HYSTERESIS,
	IIO_CHAN_INFO_INT_TIME,
	IIO_CHAN_INFO_ENABLE,
	IIO_CHAN_INFO_CALIBHEIGHT,
	IIO_CHAN_INFO_CALIBWEIGHT,
	IIO_CHAN_INFO_DEBOUNCE_COUNT,
	IIO_CHAN_INFO_DEBOUNCE_TIME,
	IIO_CHAN_INFO_CALIBEMISSIVITY,
	IIO_CHAN_INFO_OVERSAMPLING_RATIO,
};

#define IIO_CHAN_SOFT_TIMESTAMP(_si) {					\
	.type = IIO_TIMESTAMP,						\
	.channel = -1,							\
	.scan_index = _si,						\
	.scan_type = {							\
		.sign = 's',						\
		.realbits = 64,					\
		.storagebits = 64,					\
		},							\
}

struct iio_chan_spec {
	enum iio_chan_type	type;
	int			channel;
	int			channel2;
	unsigned long		address;
	int			scan_index;
	struct {
		char	sign;
		u8	realbits;
		u8	storagebits;
		u8	shift;
		u8	repeat;
		enum iio_endian endianness;
	} scan_type;
	long			info_mask_separate;
	long			info_mask_separate_available;
	long			info_mask_shared_by_type;
	long			info_mask_shared_by_type_available;
	long			info_mask_shared_by_dir;
	long			info_mask_shared_by_dir_available;
	long			info_mask_shared_by_all;
	long			info_mask_shared_by_all_available;
	const struct iio_event_spec *event_spec;
	unsigned int		num_event_specs;
	const struct iio_chan_spec_ext_info *ext_info;
	const char		*extend_name;
	const char		*datasheet_name;
	unsigned		modified:1;
	unsigned		indexed:1;
	unsigned		output:1;
	unsigned		differential:1;
};


struct iio_event_spec {
	enum iio_event_type type;
	enum iio_event_direction dir;
	unsigned long mask_separate;
	unsigned long mask_shared_by_type;
	unsigned long mask_shared_by_dir;
	unsigned long mask_shared_by_all;
};


#define OPT3001_RESULT		0x00
#define OPT3001_CONFIGURATION	0x01
#define OPT3001_LOW_LIMIT	0x02
#define OPT3001_HIGH_LIMIT	0x03
#define OPT3001_MANUFACTURER_ID	0x7e
#define OPT3001_DEVICE_ID	0x7f

#define OPT3001_CONFIGURATION_RN_MASK	(0xf << 12)
#define OPT3001_CONFIGURATION_RN_AUTO	(0xc << 12)

#define OPT3001_CONFIGURATION_CT	BIT(11)

#define OPT3001_CONFIGURATION_M_MASK	(3 << 9)
#define OPT3001_CONFIGURATION_M_SHUTDOWN (0 << 9)
#define OPT3001_CONFIGURATION_M_SINGLE	(1 << 9)
#define OPT3001_CONFIGURATION_M_CONTINUOUS (2 << 9) /* also 3 << 9 */

#define OPT3001_CONFIGURATION_OVF	BIT(8)
#define OPT3001_CONFIGURATION_CRF	BIT(7)
#define OPT3001_CONFIGURATION_FH	BIT(6)
#define OPT3001_CONFIGURATION_FL	BIT(5)
#define OPT3001_CONFIGURATION_L		BIT(4)
#define OPT3001_CONFIGURATION_POL	BIT(3)
#define OPT3001_CONFIGURATION_ME	BIT(2)

#define OPT3001_CONFIGURATION_FC_MASK	(3 << 0)

/* The end-of-conversion enable is located in the low-limit register */
#define OPT3001_LOW_LIMIT_EOC_ENABLE	0xc000

#define OPT3001_REG_EXPONENT(n)		((n) >> 12)
#define OPT3001_REG_MANTISSA(n)		((n) & 0xfff)

#define OPT3001_INT_TIME_LONG		800000
#define OPT3001_INT_TIME_SHORT		100000

/*
 * Time to wait for conversion result to be ready. The device datasheet
 * sect. 6.5 states results are ready after total integration time plus 3ms.
 * This results in worst-case max values of 113ms or 883ms, respectively.
 * Add some slack to be on the safe side.
 */
#define OPT3001_RESULT_READY_SHORT	120 //150
#define OPT3001_RESULT_READY_LONG	1000
struct opt3001 {
	void	*client;
	void	*dev;

	//struct mutex		lock;
	bool			ok_to_ignore_lock;
	bool			result_ready;
	//wait_queue_head_t	result_ready_queue;
	u16			result;

	u32			int_time,os_time;
	u32			mode;

	u16			high_thresh_mantissa;
	u16			low_thresh_mantissa;

	u8			high_thresh_exp;
	u8			low_thresh_exp;

  u8      ctrl_flag; 
};

struct opt3001_scale {
	int	val;
	int	val2;
};

static const struct opt3001_scale opt3001_scales[] = {
	{
		.val = 40,
		.val2 = 950000,
	},
	{
		.val = 81,
		.val2 = 900000,
	},
	{
		.val = 163,
		.val2 = 800000,
	},
	{
		.val = 327,
		.val2 = 600000,
	},
	{
		.val = 655,
		.val2 = 200000,
	},
	{
		.val = 1310,
		.val2 = 400000,
	},
	{
		.val = 2620,
		.val2 = 800000,
	},
	{
		.val = 5241,
		.val2 = 600000,
	},
	{
		.val = 10483,
		.val2 = 200000,
	},
	{
		.val = 20966,
		.val2 = 400000,
	},
	{
		.val = 83865,
		.val2 = 600000,
	},
};


static struct opt3001 __opt3001 ;

#if 0
static int opt3001_find_scale(const struct opt3001 *opt, int val,
		int val2, u8 *exponent)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(opt3001_scales); i++) {
		const struct opt3001_scale *scale = &opt3001_scales[i];

		/*
		 * Combine the integer and micro parts for comparison
		 * purposes. Use milli lux precision to avoid 32-bit integer
		 * overflows.
		 */
		if ((val * 1000 + val2 / 1000) <=
				(scale->val * 1000 + scale->val2 / 1000)) {
			*exponent = i;
			return 0;
		}
	}

	return -EINVAL;
}
#endif

static void opt3001_to_iio_ret(struct opt3001 *opt, u8 exponent,
		u16 mantissa, int *val, int *val2)
{
	int lux;

	lux = 10 * (mantissa << exponent);
	*val = lux / 1000;
	*val2 = (lux - (*val * 1000)) * 1000;
}

static void opt3001_set_mode(struct opt3001 *opt, u16 *reg, u16 mode)
{
	*reg &= ~OPT3001_CONFIGURATION_M_MASK;
	*reg |= mode;
	opt->mode = mode;
}


static const struct iio_event_spec opt3001_event_spec[] = {
	{
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_RISING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE) |
			BIT(IIO_EV_INFO_ENABLE),
	},
	{
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_FALLING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE) |
			BIT(IIO_EV_INFO_ENABLE),
	},
};
/*
static const struct iio_chan_spec opt3001_channels[] = {
	{
		.type = IIO_LIGHT,
		.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED) |
		.event_spec = opt3001_event_spec,
				BIT(IIO_CHAN_INFO_INT_TIME),
		.num_event_specs = ARRAY_SIZE(opt3001_event_spec),
	},
	IIO_CHAN_SOFT_TIMESTAMP(1),
};
*/

static int i2c_smbus_write_word_swapped(void *client, u8 addr, u16 val)
{
extern int I2C_WriteToSlave(MMP_I2CM_ATTR *attr,unsigned char reg,unsigned short data);  
  int err = I2C_WriteToSlave( (MMP_I2CM_ATTR *)client,addr,val ) ;
    
  return err ;
}

static int i2c_smbus_read_word_swapped(void *client, u8 addr)
{
extern int I2C_ReadFromSlave(MMP_I2CM_ATTR *attr,unsigned char reg, unsigned char *data);
  unsigned short val ;
  int err = I2C_ReadFromSlave( (MMP_I2CM_ATTR *)client , addr ,(unsigned char *)&val );
  if( err < 0 ) {
    return err ;
  }
  return (int)val ;
}


int opt3001_get_prelux( void )
{
	int ret;
	u16 reg;
	u16 value;
	long timeout;
  struct opt3001 *opt = &__opt3001 ;

  if(opt->ctrl_flag!=_S_IN_IDLE) {
    return IIO_VAL_PRELUX ;
  }
	/* Reset data-ready indicator flag */
	opt->result_ready = false;

	/* Configure for single-conversion mode and start a new conversion */
	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}
	reg = ret;
	opt3001_set_mode(opt, &reg, OPT3001_CONFIGURATION_M_SINGLE);

	ret = i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION, reg);

	if (ret < 0) {
		printc( "failed to write register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}
	
	opt->os_time = OSTime ;
err:
	if (ret < 0)
		return ret;
	opt->ctrl_flag = _S_IN_PRELUX ;
		
	return IIO_VAL_INT_PLUS_MICRO;
}

int opt3001_get_reallux( int *val, int *val2)
{
	int ret;
	u16 mantissa;
	u16 reg;
	u8 exponent;
	u16 value;
	long timeout;
  struct opt3001 *opt = &__opt3001 ;

	if(opt->ctrl_flag == _S_IN_PRELUX) {
		/* Sleep for result ready time */
		timeout = (opt->int_time == OPT3001_INT_TIME_SHORT) ?
			OPT3001_RESULT_READY_SHORT : OPT3001_RESULT_READY_LONG;
		
		timeout = timeout - (OSTime - opt->os_time) ;	  
		msleep(timeout);
		/* Check result ready flag */
		ret = i2c_smbus_read_word_swapped(opt->client,
						  OPT3001_CONFIGURATION);
		if (ret < 0) {
			printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
			goto err;
		}
    
		if (!(ret & OPT3001_CONFIGURATION_CRF)) {
			ret = -ETIMEDOUT;
			goto err;
		}

		/* Obtain value */
		ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_RESULT);
		if (ret < 0) {
			printc( "failed to read register %02x\n",
				OPT3001_RESULT);
			goto err;
		}
		opt->result = ret;
		opt->result_ready = true;
	}

err:
  opt->ctrl_flag = _S_IN_IDLE ;
	if (ret == 0)
		return -ETIMEDOUT;
	else if (ret < 0)
		return ret;

	exponent = OPT3001_REG_EXPONENT(opt->result);
	mantissa = OPT3001_REG_MANTISSA(opt->result);

	opt3001_to_iio_ret(opt, exponent, mantissa, val, val2);

	return IIO_VAL_INT_PLUS_MICRO;
}



int opt3001_get_lux( int *val, int *val2)
{
	int ret;
  #if 1
  ret = opt3001_get_prelux();
  if(ret > 0 ) {
    ret = opt3001_get_reallux(val,val2) ;
  }
  return ret ;
  #else
	u16 mantissa;
	u16 reg;
	u8 exponent;
	u16 value;
	long timeout;
  struct opt3001 *opt = &__opt3001 ;

	/* Reset data-ready indicator flag */
	opt->result_ready = false;

	/* Configure for single-conversion mode and start a new conversion */
	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}
	reg = ret;
	opt3001_set_mode(opt, &reg, OPT3001_CONFIGURATION_M_SINGLE);

	ret = i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION, reg);
	if (ret < 0) {
		printc( "failed to write register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}

	{
		/* Sleep for result ready time */
		timeout = (opt->int_time == OPT3001_INT_TIME_SHORT) ?
			OPT3001_RESULT_READY_SHORT : OPT3001_RESULT_READY_LONG;
		msleep(timeout);
		/* Check result ready flag */
		ret = i2c_smbus_read_word_swapped(opt->client,
						  OPT3001_CONFIGURATION);
		if (ret < 0) {
			printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
			goto err;
		}
    
		if (!(ret & OPT3001_CONFIGURATION_CRF)) {
			ret = -ETIMEDOUT;
			goto err;
		}

		/* Obtain value */
		ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_RESULT);
		if (ret < 0) {
			printc( "failed to read register %02x\n",
				OPT3001_RESULT);
			goto err;
		}
		opt->result = ret;
		opt->result_ready = true;
	}

err:

	if (ret == 0)
		return -ETIMEDOUT;
	else if (ret < 0)
		return ret;

	exponent = OPT3001_REG_EXPONENT(opt->result);
	mantissa = OPT3001_REG_MANTISSA(opt->result);

	opt3001_to_iio_ret(opt, exponent, mantissa, val, val2);

	return IIO_VAL_INT_PLUS_MICRO;
	#endif
}



static int opt3001_set_int_time(struct opt3001 *opt, int time)
{
	int ret;
	u16 reg;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		return ret;
	}

	reg = ret;

	switch (time) {
	case OPT3001_INT_TIME_SHORT:
		reg &= ~OPT3001_CONFIGURATION_CT;
		opt->int_time = OPT3001_INT_TIME_SHORT;
		break;
	case OPT3001_INT_TIME_LONG:
		reg |= OPT3001_CONFIGURATION_CT;
		opt->int_time = OPT3001_INT_TIME_LONG;
		break;
	default:
		return -EINVAL;
	}

	return i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION,
			reg);
}

#if 0 // just let build ok
static int opt3001_get_int_time(struct opt3001 *opt, int *val, int *val2)
{
	*val = 0;
	*val2 = opt->int_time;

	return IIO_VAL_INT_PLUS_MICRO;
}

static int opt3001_read_raw(struct opt3001 *iio,
		struct iio_chan_spec const *chan, int *val, int *val2,
		long mask)
{
	struct opt3001 *opt = iio ;
	int ret;

	if (opt->mode == OPT3001_CONFIGURATION_M_CONTINUOUS)
		return -EBUSY;

	if (chan->type != IIO_LIGHT)
		return -EINVAL;


	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		ret = opt3001_get_lux(opt, val, val2);
		break;
	case IIO_CHAN_INFO_INT_TIME:
		ret = opt3001_get_int_time(opt, val, val2);
		break;
	default:
		ret = -EINVAL;
	}


	return ret;
} 


static int opt3001_write_raw(struct opt3001 *iio,
		struct iio_chan_spec const *chan, int val, int val2,
		long mask)
{
	struct opt3001 *opt = iio;
	int ret;

	if (opt->mode == OPT3001_CONFIGURATION_M_CONTINUOUS)
		return -EBUSY;

	if (chan->type != IIO_LIGHT)
		return -EINVAL;

	if (mask != IIO_CHAN_INFO_INT_TIME)
		return -EINVAL;

	if (val != 0)
		return -EINVAL;

	ret = opt3001_set_int_time(opt, val2);

	return ret;
}

static int opt3001_read_event_value(struct opt3001 *iio,
		const struct iio_chan_spec *chan, enum iio_event_type type,
		enum iio_event_direction dir, enum iio_event_info info,
		int *val, int *val2)
{
	struct opt3001 *opt = iio;
	int ret = IIO_VAL_INT_PLUS_MICRO;


	switch (dir) {
	case IIO_EV_DIR_RISING:
		opt3001_to_iio_ret(opt, opt->high_thresh_exp,
				opt->high_thresh_mantissa, val, val2);
		break;
	case IIO_EV_DIR_FALLING:
		opt3001_to_iio_ret(opt, opt->low_thresh_exp,
				opt->low_thresh_mantissa, val, val2);
		break;
	default:
		ret = -EINVAL;
	}


	return ret;
}

static int opt3001_write_event_value(struct opt3001 *iio,
		const struct iio_chan_spec *chan, enum iio_event_type type,
		enum iio_event_direction dir, enum iio_event_info info,
		int val, int val2)
{
	struct opt3001 *opt = iio;
	int ret;

	u16 mantissa;
	u16 value;
	u16 reg;

	u8 exponent;

	if (val < 0)
		return -EINVAL;


	ret = opt3001_find_scale(opt, val, val2, &exponent);
	if (ret < 0) {
		printc( "can't find scale for %d.%06u\n", val, val2);
		goto err;
	}

	mantissa = (((val * 1000) + (val2 / 1000)) / 10) >> exponent;
	value = (exponent << 12) | mantissa;

	switch (dir) {
	case IIO_EV_DIR_RISING:
		reg = OPT3001_HIGH_LIMIT;
		opt->high_thresh_mantissa = mantissa;
		opt->high_thresh_exp = exponent;
		break;
	case IIO_EV_DIR_FALLING:
		reg = OPT3001_LOW_LIMIT;
		opt->low_thresh_mantissa = mantissa;
		opt->low_thresh_exp = exponent;
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	ret = i2c_smbus_write_word_swapped(opt->client, reg, value);
	if (ret < 0) {
		printc( "failed to write register %02x\n", reg);
		goto err;
	}

err:

	return ret;
}

static int opt3001_read_event_config(struct opt3001 *iio,
		const struct iio_chan_spec *chan, enum iio_event_type type,
		enum iio_event_direction dir)
{
	struct opt3001 *opt = iio;

	return opt->mode == OPT3001_CONFIGURATION_M_CONTINUOUS;
}


static int opt3001_write_event_config(struct opt3001 *iio,
		const struct iio_chan_spec *chan, enum iio_event_type type,
		enum iio_event_direction dir, int state)
{
	struct opt3001 *opt = iio;
	int ret;
	u16 mode;
	u16 reg;

	if (state && opt->mode == OPT3001_CONFIGURATION_M_CONTINUOUS)
		return 0;

	if (!state && opt->mode == OPT3001_CONFIGURATION_M_SHUTDOWN)
		return 0;

	//mutex_lock(&opt->lock);

	mode = state ? OPT3001_CONFIGURATION_M_CONTINUOUS
		: OPT3001_CONFIGURATION_M_SHUTDOWN;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}

	reg = ret;
	opt3001_set_mode(opt, &reg, mode);

	ret = i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION,
			reg);
	if (ret < 0) {
		printc( "failed to write register %02x\n",
				OPT3001_CONFIGURATION);
		goto err;
	}

err:
	//mutex_unlock(&opt->lock);

	return ret;
}
#endif
#if 0
static int opt3001_read_id(struct opt3001 *opt)
{
	char manufacturer[2];
	u16 device_id;
	int ret;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_MANUFACTURER_ID);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_MANUFACTURER_ID);
		return ret;
	}

	manufacturer[0] = ret >> 8;
	manufacturer[1] = ret & 0xff;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_DEVICE_ID);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_DEVICE_ID);
		return ret;
	}

	device_id = ret;

	printc( "\r\nFound %c%c OPT%04x - %d\r\n", manufacturer[0],
			manufacturer[1], device_id,OSTime);

	return 0;
}
#endif

static int opt3001_configure(struct opt3001 *opt)
{
	int ret;
	u16 reg;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		return ret;
	}

	reg = ret;

	/* Enable automatic full-scale setting mode */
	reg &= ~OPT3001_CONFIGURATION_RN_MASK;
	reg |= OPT3001_CONFIGURATION_RN_AUTO;

	/* Reflect status of the device's integration time setting */

	/* Ensure device is in shutdown initially */
	opt3001_set_mode(opt, &reg, OPT3001_CONFIGURATION_M_SHUTDOWN);

	/* Configure for latched window-style comparison operation */
	reg |= OPT3001_CONFIGURATION_L ;
	reg &= ~OPT3001_CONFIGURATION_POL;
	reg &= ~OPT3001_CONFIGURATION_ME;
	reg &= ~OPT3001_CONFIGURATION_FC_MASK;

  
	ret = i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION,
			reg);
	if (ret < 0) {
		printc( "failed to write register %02x\n",
				OPT3001_CONFIGURATION);
		return ret;
	}

  opt3001_set_int_time( opt,OPT3001_INT_TIME_SHORT) ;

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_LOW_LIMIT);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_LOW_LIMIT);
		return ret;
	}

	opt->low_thresh_mantissa = OPT3001_REG_MANTISSA(ret);
	opt->low_thresh_exp = OPT3001_REG_EXPONENT(ret);

//  printc("[OPT3001] : low (%d) , (%d,%d)\r\n",ret,opt->low_thresh_mantissa , opt->low_thresh_exp );

	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_HIGH_LIMIT);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_HIGH_LIMIT);
		return ret;
	}

	opt->high_thresh_mantissa = OPT3001_REG_MANTISSA(ret);
	opt->high_thresh_exp = OPT3001_REG_EXPONENT(ret);

//printc("[OPT3001] : high (%d) , (%d,%d)\r\n",ret,opt->high_thresh_mantissa , opt->high_thresh_exp );

	return 0;
}
/*
static irqreturn_t opt3001_irq(int irq, void *_iio)
{
	struct iio_dev *iio = _iio;
	struct opt3001 *opt = iio_priv(iio);
	int ret;


	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		goto out;
	}

	if ((ret & OPT3001_CONFIGURATION_M_MASK) ==
			OPT3001_CONFIGURATION_M_CONTINUOUS) {
		if (ret & OPT3001_CONFIGURATION_FH)
			iio_push_event(iio,
					IIO_UNMOD_EVENT_CODE(IIO_LIGHT, 0,
							IIO_EV_TYPE_THRESH,
							IIO_EV_DIR_RISING),
					iio_get_time_ns(iio));
		if (ret & OPT3001_CONFIGURATION_FL)
			iio_push_event(iio,
					IIO_UNMOD_EVENT_CODE(IIO_LIGHT, 0,
							IIO_EV_TYPE_THRESH,
							IIO_EV_DIR_FALLING),
					iio_get_time_ns(iio));
	} else if (ret & OPT3001_CONFIGURATION_CRF) {
		ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_RESULT);
		if (ret < 0) {
			printc( "failed to read register %02x\n",
					OPT3001_RESULT);
			goto out;
		}
		opt->result = ret;
		opt->result_ready = true;
	}

out:

	return IRQ_HANDLED;
}
*/

int opt3001_probe(void *client)
{
	MMP_I2CM_ATTR *attr;
	MMP_UBYTE     addr = 0x44 ;
	struct opt3001 *opt= &__opt3001 ;
	//int irq = client->irq;
	int ret , i;
	opt->client = client;
  attr = (MMP_I2CM_ATTR *)opt->client ;
#if 0  
  ret = opt3001_read_id(opt);
	if (ret)
		return ret;
#else
  //printc("\r\n[OPT3001]-> %d\r\n",OSTime);
#endif  
	ret = opt3001_configure(opt);
	if (ret)
		return ret;

  opt->ctrl_flag = _S_IN_IDLE ;
	return 0;
}

int opt3001_remove(void *client)
{
	struct opt3001 *opt = &__opt3001 ;
	int ret;
	u16 reg;


	ret = i2c_smbus_read_word_swapped(opt->client, OPT3001_CONFIGURATION);
	if (ret < 0) {
		printc( "failed to read register %02x\n",
				OPT3001_CONFIGURATION);
		return ret;
	}

	reg = ret;
	opt3001_set_mode(opt, &reg, OPT3001_CONFIGURATION_M_SHUTDOWN);

	ret = i2c_smbus_write_word_swapped(opt->client, OPT3001_CONFIGURATION,
			reg);
	if (ret < 0) {
		printc( "failed to write register %02x\n",
				OPT3001_CONFIGURATION);
		return ret;
	}

	return 0;
}
#endif
