/*
 ******************************************************************************
 * adxl345.c -
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
#include "adxl345.h"


/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define DBG_MSG			0
#if (DBG_MSG == 1)
    #include <stdio.h>
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif

#define adxl345_delay(n) \
do { \
	volatile unsigned long	i; \
	for (i=0; i<(n); i++) \
		; \
} while (__LINE__ == -1)

/*
 * adxl345_select - select the accelerometer on the I2C-bus
 */
#define adxl345_select()		\
{ \
	HalI2CInit(XL345_SLAVE_ADDR, i2cClock_267KHZ); \
}


/*
 ******************************************************************************
 *
 * Local Function
 *
 ******************************************************************************
 */
static unsigned char adxl345_reg_write(unsigned char addr, unsigned char val)
{
	unsigned char	buf[2];

	buf[0] = addr;		// register address
	buf[1] = val;		// register value
	return HalI2CWrite(sizeof(buf), buf);
}

static unsigned char adxl345_reg_read(unsigned char addr, unsigned char *val)
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
char adxl345_init(void)
{
	unsigned char	devid;

	adxl345_select();

	if (!adxl345_reg_read(XL345_DEVID, &devid)) {
		dmsg(("adxl345 read devid i2c fail\n"));
		return 0;
	}
	if (devid != 0xe5) {
		dmsg(("adxl345 devid=%02x is not 0xE5\n", devid));
		return 0;
	}

	adxl345_reg_write(XL345_FIFO_CTL,	XL345_FIFO_RESET);				// FIFO reset & bypass
	adxl345_reg_write(XL345_BW_RATE,	XL345_RATE_100);				// output data rate: 100Hz
	adxl345_reg_write(XL345_DATA_FORMAT,	XL345_FULL_RESOLUTION | XL345_RANGE_2G);	// data format: +/-16g range, right justified,  256->1g
//	adxl345_reg_write(XL345_FIFO_CTL,	XL345_FIFO_MODE_FIFO | 0x0A);			// FIFO mode, samples = 10
//	adxl345_reg_write(XL345_INT_ENABLE,	XL345_DATAREADY);				// INT_Enable: water mark
//	adxl345_reg_write(XL345_INT_MAP,	XL345_DATAREADY);				// INT_Map: water mark interrupt to the INT2 pin
	adxl345_reg_write(XL345_POWER_CTL,	XL345_MEASURE);					// power control: measure mode
	return 1;
}

char adxl345_read(unsigned short *p)
{
	unsigned char	intsrc, i;
	unsigned char	x0, x1;
	unsigned char	y0, y1;
	unsigned char	z0, z1;
	char		ret;

	// check data ready
	ret = -1;

	adxl345_select();
	if (!adxl345_reg_read(XL345_INT_SOURCE, &intsrc)) {
		return ret;
	}
	if ((intsrc & XL345_DATAREADY) != XL345_DATAREADY) {	// water mark interrupt
		return ret;
	}

	// read the three registers
	ret = 1;

	if (!adxl345_reg_read(XL345_DATAX0, &x0)) {
		return ret;
	}
	if (!adxl345_reg_read(XL345_DATAX1, &x1)) {
		return ret;
	}

	if (!adxl345_reg_read(XL345_DATAY0, &y0)) {
		return ret;
	}
	if (!adxl345_reg_read(XL345_DATAY1, &y1)) {
		return ret;
	}

	if (!adxl345_reg_read(XL345_DATAZ0, &z0)) {
		return ret;
	}
	if (!adxl345_reg_read(XL345_DATAZ1, &z1)) {
		return ret;
	}

	// valid data
	ret  = 0;

	p[0] = ((unsigned short) x1) << 8 | x0;
	p[1] = ((unsigned short) y1) << 8 | y0;
	p[2] = ((unsigned short) z1) << 8 | z0;

	dmsg(("X=%04x, Y=%04x, Z=%04x\n", p[0], p[1], p[2]));

	// translate the two's complement to true binary code
	for (i=0; i<3; i++) {
		if (p[i] > 32768) {
			p[i] = (p[i] ^ 0xFFFF) + 1;
		}
	}

	return ret;
}


void adxl345_enter_sleep(void)
{
//	adxl345_reg_write(XL345_POWER_CTL, XL345_STANDBY);
}


void adxl345_exit_sleep(void)
{
	char	retry;

	for (retry=0; retry<3; retry++) {
		if (adxl345_init()) {
			break;
		}
	}

}


#define ADXL345_CALIB_NUM	16
void adxl345_self_calibration(void)
{
	unsigned char	x0, x1;
	unsigned char	y0, y1;
	unsigned char	z0, z1;
	unsigned char	intsrc, n;
	long		cx, cy, cz;

	// enter self calibration
	adxl345_select();
	adxl345_reg_write(XL345_BW_RATE,	XL345_RATE_100);				// output data rate: 100Hz
	adxl345_reg_write(XL345_DATA_FORMAT,	XL345_FULL_RESOLUTION | XL345_RANGE_2G);	// All g-ranges, full resolution,  256LSB/g
	adxl345_reg_write(XL345_OFSX,		0);
	adxl345_reg_write(XL345_OFSY,		0);
	adxl345_reg_write(XL345_OFSZ,		0);

	// get samples
	cx = 0;	cy = 0;	cz = 0;
	for (n=0; n<ADXL345_CALIB_NUM; n++) {
retry:
		adxl345_reg_read(XL345_INT_SOURCE, &intsrc);
		if ((intsrc & XL345_DATAREADY) != XL345_DATAREADY) {
			adxl345_delay(50);
			goto retry;
		}
		adxl345_reg_read(XL345_DATAX0, &x0);
		adxl345_reg_read(XL345_DATAX1, &x1);
		adxl345_reg_read(XL345_DATAY0, &y0);
		adxl345_reg_read(XL345_DATAY1, &y1);
		adxl345_reg_read(XL345_DATAZ0, &z0);
		adxl345_reg_read(XL345_DATAZ1, &z1);

		cx += ((unsigned short) x1) << 8 | x0;
		cy += ((unsigned short) y1) << 8 | y0;
		cz += ((unsigned short) z1) << 8 | z0;
	}

	cx = cx / ADXL345_CALIB_NUM;
	cy = cy / ADXL345_CALIB_NUM;
	cz = cz / ADXL345_CALIB_NUM;

	// result
	cx = (cx / 4) * (-1);
	cy = (cy / 4) * (-1);
	cz = (cz / 4) * (-1);

	adxl345_reg_write(XL345_OFSX, (char) cx);
	adxl345_reg_write(XL345_OFSY, (char) cy);
	adxl345_reg_write(XL345_OFSZ, (char) cz);

not_ready:
	adxl345_reg_read(XL345_INT_SOURCE, &intsrc);
	if ((intsrc & XL345_DATAREADY) != XL345_DATAREADY) {
		adxl345_delay(50);
		goto not_ready;
	}
	adxl345_reg_read(XL345_DATAX0, &x0);
	adxl345_reg_read(XL345_DATAX1, &x1);
	adxl345_reg_read(XL345_DATAY0, &y0);
	adxl345_reg_read(XL345_DATAY1, &y1);
	adxl345_reg_read(XL345_DATAZ0, &z0);
	adxl345_reg_read(XL345_DATAZ1, &z1);

	cx = ((unsigned short) x1) << 8 | x0;
	cy = ((unsigned short) y1) << 8 | y0;
	cz = ((unsigned short) z1) << 8 | z0;


}


