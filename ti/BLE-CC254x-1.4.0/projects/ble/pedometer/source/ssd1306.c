/*
 ******************************************************************************
 * ssd1306.c -  driver for 128x64 OLED display based on the SSD1306 controller
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
 *	2014.07.15	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */

#include <hal_i2c.h>
#include "ssd1306.h"

/*
 ******************************************************************************
 *
 * Local Function
 *
 ******************************************************************************
 */
/*
 * ssd1306_select - select the OLED controller on the I2C-bus
 */
#define ssd1306_select()	\
{ \
	HalI2CInit(0x78 >> 1, i2cClock_267KHZ); \
}

/*
 ******************************************************************************
 *
 * Global Function
 *
 ******************************************************************************
 */
unsigned char ssd1306_command(unsigned char cmd)
{
	unsigned char	buf[2];

	buf[0] = 0x00;		// [7] : continuation bit
				// [6] : data / command selection bit
	buf[1] = cmd;
	ssd1306_select();
	return HalI2CWrite(sizeof(buf), buf);
}

unsigned char ssd1306_data(unsigned char data)
{
	unsigned char	buf[2];

	buf[0] = 0x40;		// [7] : continuation bit
				// [6] : data / command selection bit
	buf[1] = data;
	ssd1306_select();
	return HalI2CWrite(sizeof(buf), buf);
}

void ssd1306_set_pagecol(unsigned char page, unsigned char col)
{
	// set col address
	ssd1306_command(0x00 + ((col     ) & 0x0F));	// low  col address
	ssd1306_command(0x10 + ((col >> 4) & 0x0F));	// high col address

	// set page address
	ssd1306_command(0xb0 + ((page    ) & 0x07));
}

void ssd1306_clr_gddram(void)
{
	unsigned char	x, y;

	for (y=0; y<8; y++) {
		ssd1306_set_pagecol(y, 0);
		for (x=0; x<128; x++) {
			ssd1306_data(0x00);
		}
	}
}


