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

#ifndef __KXTI9_1001_H__
#define __KXTI9_1001_H__

#ifdef __cplusplus
extern "C" {
#endif


#define HAL_ACC_RANGE_8G		1
#define HAL_ACC_RANGE_4G		2
#define HAL_ACC_RANGE_2G		3


// Sensor I2C address
#define KXTI9_I2C_ADDRESS		0x0F

// KXTI9 register addresses
#define KXTI9_XOUT_HPF_L		0x00	// R
#define KXTI9_XOUT_HPF_H		0x01	// R
#define KXTI9_YOUT_HPF_L		0x02	// R
#define KXTI9_YOUT_HPF_H		0x03	// R
#define KXTI9_ZOUT_HPF_L		0x04	// R
#define KXTI9_ZOUT_HPF_H		0x05	// R
#define KXTI9_XOUT_L			0x06	// R
#define KXTI9_XOUT_H			0x07	// R
#define KXTI9_YOUT_L			0x08	// R
#define KXTI9_YOUT_H			0x09	// R
#define KXTI9_ZOUT_L			0x0A	// R
#define KXTI9_ZOUT_H			0x0B	// R
#define KXTI9_DCST_RESP			0x0C	// R
#define KXTI9_WHO_AM_I			0x0F	// R
#define KXTI9_TILT_POS_CUR		0x10	// R
#define KXTI9_TILT_POS_PRE		0x11	// R

#define KXTI9_INT_SRC_REG1		0x15	// R
#define KXTI9_INT_SRC_REG2		0x16	// R
#define KXTI9_STATUS_REG		0x18	// R
#define KXTI9_INT_REL			0x1A	// R

#define KXTI9_CTRL_REG1			0x1B	// R/W
#define KXTI9_CTRL_REG2			0x1C	// R/W
#define KXTI9_CTRL_REG3			0x1D	// R/W

#define KXTI9_INT_CTRL_REG1		0x1E	// R/W
#define KXTI9_INT_CTRL_REG2		0x1F	// R/W
#define KXTI9_INT_CTRL_REG3		0x20	// R/W
#define KXTI9_DATA_CTRL_REG		0x21	// R/W

#define KXTI9_TILT_TIMER		0x28	// R/W
#define KXTI9_WUF_TIMER			0x29	// R/W
#define KXTI9_TDT_TIMER			0x2B	// R/W
#define KXTI9_TDT_H_THRESH		0x2C	// R/W
#define KXTI9_TDT_L_THRESH		0x2D	// R/W
#define KXTI9_TDT_TAP_TIMER		0x2E	// R/W
#define KXTI9_TDT_TOTAL_TIMER		0x2F	// R/W
#define KXTI9_TDT_LATENCY_TIMER		0x30	// R/W
#define KXTI9_TDT_WINDOW_TIMER		0x31	// R/W

#define KXTI9_BUF_CTRL1			0x32	// R/W
#define KXTI9_BUF_CTRL2			0x33	// R/W
#define KXTI9_BUF_STATUS_REG1		0x34	// R
#define KXTI9_BUF_STATUS_REG2		0x35	// R/W
#define KXTI9_BUF_CLEAR			0x36	// W

#define KXTI9_SELF_TEST			0x3A	// R/W

#define KXTI9_WUF_THRESH		0x5A	// R/W
#define KXTI9_TILT_ANGLE		0x5C	// R/W
#define KXTI9_HYST_SET			0x5F	// R/W
#define KXTI9_BUF_READ			0x7F	// R/W

// Select register valies
#define REG_VAL_WHO_AM_I		0x08	// (data sheet says 0x04)

// CTRL1 BIT MASKS
#define ACC_REG_CTRL_PC			0x80	// Power Control	'1' On		'0' Off
#define ACC_REG_CTRL_RES		0x40	// Resolution		'1' High	'0' Low
#define ACC_REG_CTRL_DRDYE		0x20	// Data Ready		'1' On		'0' Off
#define ACC_REG_CTRL_GSEL_HI		0x10	// Range		'00' +/-2g	'01' +/-4g
#define ACC_REG_CTRL_GSEL_LO		0x08	//			'10' +/-8g	'11' N/A
#define ACC_REG_CTRL_GSEL_TDTE		0x04	// Directional Tap	'1' On		'0' Off
#define ACC_REG_CTRL_GSEL_WUFE		0x02	// Wake Up		'1' On		'0' Off
#define ACC_REG_CTRL_GSEL_TPE		0x01	// Tilt Position	'1' On		'0' Off

// Range +- 2G
#define ACC_REG_CTRL_ON_2G		(ACC_REG_CTRL_PC)
#define ACC_REG_CTRL_OFF_2G		(0)

// Range +- 4G
#define ACC_REG_CTRL_ON_4G		(ACC_REG_CTRL_PC | ACC_REG_CTRL_GSEL_LO)
#define ACC_REG_CTRL_OFF_4G		(ACC_REG_CTRL_GSEL_LO)

// Range +- 8G
#define ACC_REG_CTRL_ON_8G		(ACC_REG_CTRL_PC | ACC_REG_CTRL_GSEL_HI)
#define ACC_REG_CTRL_OFF_8G		(ACC_REG_CTRL_GSEL_HI)




/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern void		kxti9_set_range(unsigned char range);
extern char		kxti9_init(void);
extern char		kxti9_read(unsigned short *p);
extern unsigned char	kxti9_test(void);



#ifdef __cplusplus
};
#endif
#endif /* __KXTI9_1001_H__ */
