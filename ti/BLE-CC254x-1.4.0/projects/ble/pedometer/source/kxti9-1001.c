/*
 ******************************************************************************
 * kxti9-1001.h - driver for the Kionix KXTI9 accelerometer
 *
 * Copyright (c) 2014-2016 by ZealTek Electronic Co., Ltd.
 *
 * This software is copyrighted by and is the property of ZealTek
 * Electronic Co., Ltd. All rights are reserved by ZealTek Electronic
 * Co., Ltd. This software may only be used in accordance with the
 * corresponding license agreement. Any unauthorized use, duplication,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice MUST not be removed or modified without prior
 * written consent of ZealTek Electronic Co., Ltd.
 *
 * ZealTek Electronic Co., Ltd. reserves the right to modify this
 * software without notice.
 *
 * History:
 *	2014.07.22	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#include <hal_i2c.h>
#include "kxti9-1001.h"

/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define DBG_MSG			1
#if (DBG_MSG == 1)
    #include <stdio.h>
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif


#define kxti9_delay(n) \
do { \
	volatile unsigned long	i; \
	for (i=0; i<(n); i++) \
		; \
} while (__LINE__ == -1)


/*
 * kxti9_select - select the accelerometer on the I2C-bus
 */
#define kxti9_select()		\
{ \
	HalI2CInit(KXTI9_I2C_ADDRESS, i2cClock_267KHZ);	\
}


/*
 ******************************************************************************
 *
 * Local variable
 *
 ******************************************************************************
 */
static unsigned char 	kxti9_config;
static unsigned char	kxti9_off;
static unsigned char	kxti9_range;

/*
 ******************************************************************************
 *
 * Local Function
 *
 ******************************************************************************
 */
static unsigned char kxti9_reg_write(unsigned char addr, unsigned char val)
{
	unsigned char	buf[2];

	buf[0] = addr;		// register address
	buf[1] = val;		// register value
	return HalI2CWrite(sizeof(buf), buf);
}

static unsigned char kxti9_reg_read(unsigned char addr, unsigned char *val)
{
	/* send address we're reading from */
	if (HalI2CWrite(1, &addr) == 1) {
		/* now read data */
		return HalI2CRead(1, val);
	}
	return 0;
}


/*
 ******************************************************************************
 *
 * Global Function
 *
 ******************************************************************************
 */
/*
 * kxti9_set_range
 */
void kxti9_set_range(unsigned char range)
{
	kxti9_range = range;

	switch (kxti9_range) {
	case HAL_ACC_RANGE_2G:
		kxti9_config = ACC_REG_CTRL_ON_2G;
		kxti9_off    = ACC_REG_CTRL_OFF_2G;
		break;
	case HAL_ACC_RANGE_4G:
		kxti9_config = ACC_REG_CTRL_ON_4G;
		kxti9_off    = ACC_REG_CTRL_OFF_4G;
		break;
	case HAL_ACC_RANGE_8G:
		kxti9_config = ACC_REG_CTRL_ON_8G;
		kxti9_off    = ACC_REG_CTRL_OFF_8G;
		break;
	default:
		// Should not get here
		break;
	}
}


/*
 * kxti9_init
 */
char kxti9_init(void)
{
	kxti9_set_range(HAL_ACC_RANGE_8G);
	return 1;
}


/*
 * kxti9_read
 */
char kxti9_read(unsigned short *p)
{
	unsigned char	x0, x1;
	unsigned char	y0, y1;
	unsigned char	z0, z1;
	unsigned char	ret = 0;

	// select this sensor
	kxti9_select();

	// turn on sensor
	kxti9_reg_write(KXTI9_CTRL_REG1, kxti9_config);

	// wait for measurement ready (appx. 1.45 ms)
	kxti9_delay(180);

	// read the three registers
	if (kxti9_reg_read(KXTI9_XOUT_L, &x0)) {
		if (kxti9_reg_read(KXTI9_XOUT_H, &x1)) {
			if (kxti9_reg_read(KXTI9_YOUT_L, &y0)) {
				if (kxti9_reg_read(KXTI9_YOUT_H, &y1)) {
					if (kxti9_reg_read(KXTI9_ZOUT_L, &z0)) {
						if (kxti9_reg_read(KXTI9_ZOUT_H, &z1)) {
							// valid data
							ret  = 1;

							p[0] = ((unsigned short) x1) << 8 | x0;
							p[1] = ((unsigned short) y1) << 8 | y0;
							p[2] = ((unsigned short) z1) << 8 | z0;

							dmsg(("X=%04X, Y=%04X, Z=%04X\n", p[0], p[1], p[2]));
						}
					}
				}
			}
		}
	}

	// turn off sensor
	kxti9_reg_write(KXTI9_CTRL_REG1, kxti9_off);
	return ret;
}


/*
 * kxti9_test - run a sensor self-test
 *
 * return      TRUE if passed, FALSE if failed
 */
unsigned char kxti9_test(void)
{
	unsigned char	val;

	// select this sensor on the I2C bus
	kxti9_select();

	// check the DCST_RESP (pattern 0x55)
	if (!kxti9_reg_read(KXTI9_DCST_RESP, &val))
		return FALSE;

	if (!(val == 0x55))
		return FALSE;

	// check the DCST_RESP (pattern 0xAA)
	val = 0x10;	// Sets the DCST bit
	if (!kxti9_reg_write(KXTI9_CTRL_REG3, val))
		return FALSE;

	if (!kxti9_reg_read(KXTI9_DCST_RESP, &val))
		return FALSE;

	if (!(val == 0xAA))
		return FALSE;

	// check the WHO AM I register
	if (!kxti9_reg_read(KXTI9_WHO_AM_I, &val))
		return FALSE;

	if (!(val == REG_VAL_WHO_AM_I))
		return FALSE;

	return TRUE;
}


