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

#include <hal_board.h>
#include <hal_i2c.h>
#include <osal.h>

#include "defines.h"
#include "sensorTag.h"
#include "adxl345.h"


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
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
typedef struct {
	unsigned char	lsb;
	unsigned char	msb;
} b16_t;

typedef union {
	b16_t		u8;
	unsigned short	u16;
	signed   short	s16;
} endian_t;


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

static unsigned char adxl345_reg_multi_read(unsigned char addr, unsigned char n, unsigned char *val)
{
	/* send address we're reading from */
	if (HalI2CWrite(1, &addr) == 1) {
		/* now read data */
		return HalI2CRead(n, val);
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
 * software reset
 */
void adxl345_softrst(void)
{
	adxl345_select();

	adxl345_reg_write(XL345_INT_ENABLE,    0);
	adxl345_reg_write(XL345_OFSX,	       0);
	adxl345_reg_write(XL345_OFSY,	       0);
	adxl345_reg_write(XL345_OFSZ,	       0);
	adxl345_reg_write(XL345_THRESH_ACT,    0);
	adxl345_reg_write(XL345_ACT_INACT_CTL, 0);
	adxl345_reg_write(XL345_FIFO_CTL,      0);
	adxl345_reg_write(XL345_POWER_CTL,     0);

	// CPU I/O function
	GSINT1_SEL &= ~GSINT1_BV;
	GSINT1_DDR &= ~GSINT1_BV;

	GSINT2_SEL &= ~GSINT2_BV;
	GSINT2_DDR &= ~GSINT2_BV;

	// CPU interrupt register
	GSINT_IEN  &= ~GSINT_IENBIT;
	GSINT_ICTL &= ~(GSINT1_BV | GSINT2_BV);
	GSINT_PXIFG = ~(GSINT1_BV | GSINT2_BV);
}


/*
 * activity detection
 */
void adxl345_activity(unsigned char threshold)
{
	unsigned char	ctrl, inten;

	GSINT_IEN  &= ~GSINT_IENBIT;
	GSINT_ICTL &= ~GSINT1_BV;
	GSINT_PXIFG = ~GSINT1_BV;

	adxl345_select();
	adxl345_reg_read(XL345_ACT_INACT_CTL, &ctrl);
	adxl345_reg_read(XL345_INT_ENABLE, &inten);
	if (threshold) {
		ctrl  |= (XL345_ACT_AC | XL345_ACT_X_ENB | XL345_ACT_Y_ENB | XL345_ACT_Z_ENB);
		inten |= XL345_ACTIVITY;
		adxl345_reg_write(XL345_THRESH_ACT, threshold);		// 62.5mg/LSB
		adxl345_reg_write(XL345_ACT_INACT_CTL, ctrl);
		adxl345_reg_write(XL345_INT_ENABLE, inten);
		GSINT_ICTL |=  GSINT1_BV;
	} else {
		ctrl  &= 0x0F;
		inten &= ~XL345_ACTIVITY;
		adxl345_reg_write(XL345_INT_ENABLE, inten);
		adxl345_reg_write(XL345_ACT_INACT_CTL, ctrl);
		adxl345_reg_write(XL345_THRESH_ACT, 0);
	}
	GSINT_IEN |= GSINT_IENBIT;
}


void adxl345_sampling(unsigned char rate)
{
	unsigned char	inten;

	GSINT_IEN  &= ~GSINT_IENBIT;
	GSINT_ICTL &= ~GSINT2_BV;
	GSINT_PXIFG = ~GSINT2_BV;

	adxl345_select();
	adxl345_reg_write(XL345_FIFO_CTL,  0);
	adxl345_reg_read(XL345_INT_ENABLE, &inten);
	if (rate) {
		inten |= XL345_WATERMARK;
		adxl345_reg_write(XL345_BW_RATE, rate);
		adxl345_reg_write(XL345_INT_ENABLE, inten);
		GSINT_ICTL |=  GSINT2_BV;
	} else {
		inten &= ~XL345_WATERMARK;
		adxl345_reg_write(XL345_BW_RATE, XL345_RATE_25);					// sampling rate: 25Hz
		adxl345_reg_write(XL345_INT_ENABLE, inten);
	}
	adxl345_reg_write(XL345_INT_MAP,     XL345_WATERMARK);						// INT_MAP: water mark interrupt to the INT2 pin
	adxl345_reg_write(XL345_DATA_FORMAT, XL345_INT_LOW | XL345_FULL_RESOLUTION | XL345_RANGE_2G);	// data format: 2g range, 256->1g
	adxl345_reg_write(XL345_FIFO_CTL,    XL345_FIFO_MODE_FIFO | GSEN_SAMPLING_FREQ);		// FIFO_CTL: FIFO mode, samples = GSEN_SAMPLING_FREQ
	adxl345_reg_write(XL345_POWER_CTL,   XL345_MEASURE);						// POWER_CTL: measure mode

	GSINT_IEN |= GSINT_IENBIT;
}



/*
 * sleep
 */
void adxl345_enter_sleep(void)
{
	adxl345_sampling(0);
	adxl345_activity(16);
}

void adxl345_exit_sleep(void)
{
	adxl345_activity(0);
	adxl345_sampling(XL345_BW_50);
}

void adxl345_shutdown(void)
{
	adxl345_sampling(0);
	adxl345_activity(0);
}


/*
 *
 */
char adxl345_chk_dev(void)
{
	unsigned char	devid;

	adxl345_select();
	if (!adxl345_reg_read(XL345_DEVID, &devid)) {
		dmsg(("[adxl345]: read devid i2c fail\n"));
		return 0;
	}
	if (devid != 0xe5) {
		dmsg(("[adxl345]: devid=%02x is not 0xE5\n", devid));
		return 0;
	}
	return 1;
}


unsigned char adxl345_chk_fifo(void)
{
	unsigned char	intsrc;
	unsigned char	ret = 0;

	adxl345_select();
	adxl345_reg_read(XL345_INT_SOURCE, &intsrc);
	if ((intsrc & XL345_WATERMARK) != XL345_WATERMARK) {
		return ret;
	}

	adxl345_reg_read(XL345_FIFO_STATUS, &ret);
	return ret;
}


/*
 * read sampling
 */
void adxl345_read(unsigned short *p)
{
	endian_t	*x, *y, *z, val[3];

	// read the three registers
	adxl345_reg_multi_read(XL345_DATAX0, 6, (unsigned char *) val);

	// translate the two's complement to true binary code
	x    = &val[0];
	y    = &val[1];
	z    = &val[2];
	p[0] = (x->s16 < 0) ? (unsigned short) (x->s16 * (-1)) : x->u16;
	p[1] = (y->s16 < 0) ? (unsigned short) (y->s16 * (-1)) : y->u16;
	p[2] = (z->s16 < 0) ? (unsigned short) (z->s16 * (-1)) : z->u16;
}


/*
 * ISR
 */
void adxl345_int1_isr(void)
{
	osal_set_event(sensorTag_TaskID, EVT_GSNINT1);
//	GSINT_PXIFG = ~(GSINT1_BV);
}

void adxl345_int2_isr(void)
{
	osal_set_event(sensorTag_TaskID, EVT_GSNINT2);
//	GSINT_PXIFG = ~(GSINT2_BV);
}


/*
 * calibration
 */
#define ADXL345_CALIB_NUM	16
void adxl345_self_calibration(void)
{
	unsigned char	intsrc, n;
	long		cx, cy, cz;
	unsigned char	keep[5];
	endian_t	*x, *y, *z, val[3];

	adxl345_select();

	// keep old status
	dmsg(("[adxl345]: keep old status\n"));
	adxl345_reg_read(XL345_BW_RATE,     &keep[0]);
	adxl345_reg_read(XL345_DATA_FORMAT, &keep[1]);
	adxl345_reg_read(XL345_INT_ENABLE,  &keep[2]);
	adxl345_reg_read(XL345_FIFO_CTL,    &keep[3]);
	adxl345_reg_read(XL345_POWER_CTL,   &keep[4]);

	// enter self calibration
	dmsg(("[adxl345]: enter self calibration\n"));
	adxl345_reg_write(XL345_OFSX,	     0);
	adxl345_reg_write(XL345_OFSY,	     0);
	adxl345_reg_write(XL345_OFSZ,	     0);
	adxl345_reg_write(XL345_BW_RATE,     XL345_RATE_100);				// output data rate: 100Hz
	adxl345_reg_write(XL345_DATA_FORMAT, XL345_FULL_RESOLUTION | XL345_RANGE_2G);	// All g-ranges, full resolution,  256LSB/g
	adxl345_reg_write(XL345_INT_ENABLE,  0);
	adxl345_reg_write(XL345_FIFO_CTL,    XL345_FIFO_MODE_FIFO | 16);		// FIFO_CTL: FIFO mode, samples = 16 
	adxl345_reg_write(XL345_POWER_CTL,   XL345_MEASURE);				// POWER_CTL: measure mode

	// get samples
	dmsg(("[adxl345]: get samples\n"));
	x = &val[0];
	y = &val[1];
	z = &val[2];

wait:
	adxl345_reg_read(XL345_INT_SOURCE, &intsrc);
	if ((intsrc & XL345_WATERMARK) != XL345_WATERMARK) {
		adxl345_delay(50);
		goto wait;
	}

	cx = 0;	cy = 0;	cz = 0;
	for (n=0; n<ADXL345_CALIB_NUM; n++) {
		adxl345_reg_multi_read(XL345_DATAX0, 6, (unsigned char *) val);

		cx += (long) x->s16;
		cy += (long) y->s16;
		cz += (long) z->s16;
	}

	cx /= ADXL345_CALIB_NUM;
	cy /= ADXL345_CALIB_NUM;
	cz /= ADXL345_CALIB_NUM;
	dmsg(("[adxl345]: before - x=%0ld, y=%0ld, z=%0ld\n", cx, cy, cz));

	// result
	cx = cx * (-1L);
	cy = cy * (-1L);
	if (cz > 0) {
		cz =  256L - cz;
	} else {
		cz = -256L - cz;
	}
	dmsg(("[adxl345]: cali   - x=%0ld, y=%0ld, z=%0ld\n", cx, cy, cz));

	cx /= 4;
	cy /= 4;
	cz /= 4;
	adxl345_reg_write(XL345_OFSX, (char) cx);
	adxl345_reg_write(XL345_OFSY, (char) cy);
	adxl345_reg_write(XL345_OFSZ, (char) cz);

	adxl345_reg_write(XL345_FIFO_CTL,  0);
	adxl345_reg_write(XL345_POWER_CTL, 0);
	adxl345_delay(50);
	adxl345_reg_write(XL345_POWER_CTL, XL345_MEASURE);
not_ready:
	adxl345_reg_read(XL345_INT_SOURCE, &intsrc);
	if ((intsrc & XL345_DATAREADY) != XL345_DATAREADY) {
		adxl345_delay(50);
		goto not_ready;
	}
	adxl345_reg_multi_read(XL345_DATAX0, 6, (unsigned char *) val);
	dmsg(("[adxl345]: after  - x=%0d, y=%0d, z=%0d\n", x->s16, y->s16, z->s16));

	// restore old status
	dmsg(("[adxl345]: restore old status\n"));
	adxl345_reg_write(XL345_BW_RATE,     keep[0]);
	adxl345_reg_write(XL345_DATA_FORMAT, keep[1]);
	adxl345_reg_write(XL345_INT_ENABLE,  keep[2]);
	adxl345_reg_write(XL345_FIFO_CTL,    keep[3]);
	adxl345_reg_write(XL345_POWER_CTL,   keep[4]);
}


