/*
 ******************************************************************************
 * oled.c -  driver for OLED display based on the SSD1306 controller
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
#include <hal_board.h>
#include <hal_i2c.h>
#include <osal.h>

#include "ssd1306.h"
#include "oled.h"
#include "fonts.h"
#include "sensorTag.h"

static unsigned char		x, y;


/*
 ******************************************************************************
 *
 * Global Function
 *
 ******************************************************************************
 */
void oled_init(void)
{
	ssd1306_command(0xAE);		// set display off

	// timing & driving scheme setting
	ssd1306_command(0xD5);		// set display clock divide ratio/osc. freq.
	ssd1306_command(0xD1);		// 105Hz

	ssd1306_command(0xDB);		// VCOM deselect level mode
	ssd1306_command(0x00);		// set Vcomh = 0.65 * Vcc

	ssd1306_command(0xD9);		// set pre-charge period
	ssd1306_command(0xF1);

	// hardware configuration
	ssd1306_command(0xA8);		// set multiplex ratio
	ssd1306_command(0x1F);

	ssd1306_command(0xD3);		// set display offset
	ssd1306_command(0x00);

	ssd1306_command(0x40);		// set display start line
	ssd1306_command(0xA0 | 0x01);	// set segment remap
	ssd1306_command(0xC0 | 0x08);	// set COM output scan direction

	ssd1306_command(0xDA);		// set COM pins hardware configuration
	ssd1306_command(0x12);

	// addressing setting
	ssd1306_command(0x20);		// set memory addressing mode
	ssd1306_command(0x02);

	// charge pump
	ssd1306_command(0x8D);		// charge pump setting
	ssd1306_command(0x14);

	// fundamental
	ssd1306_command(0x81);		// set contrast control
	ssd1306_command(0xCF);

	ssd1306_command(0xA4);		// set entire display on/off
	ssd1306_command(0xA6);		// set normal display

	ssd1306_clr_gddram();

//	ssd1306_command(0xAF);		// set display on

	// initial variable
	x    = 0;
	y    = 0;
	oled_set_font(&fonts7x8);
}


/*
 *
 */
void oled_clr_screen(void)
{
	char	row, col, k;

	k = OLED_HEIGHT / 8;
	for (row=0; row<k; row++) {
		ssd1306_set_pagecol(row, 32);
		for (col=0; col<OLED_WIDTH; col++) {
			ssd1306_data(0x00);
		}
	}
}


/*
 *
 */
unsigned char oled_gotoxy(unsigned char row, unsigned char col)
{
	if (row > (OLED_HEIGHT / font->height)) {
		return 0;
	}
	if (col > (OLED_WIDTH / font->width)) {
		return 0;
	}
	y = row;
	x = col;
	ssd1306_set_pagecol(y, (x * font->width) + 32);
	return 1;
}


/*
 *
 *
 */
void oled_enter_sleep(void)
{
	if (BATCD_SBIT) {
		// charge pump
		ssd1306_command(0x8D);		// charge pump setting
		ssd1306_command(0x10);

		ssd1306_command(0xAE);		// set display off

		osal_stop_timerEx(sensorTag_TaskID, EVT_DISP);
	} else {
		oled_clr_screen();
	}
}

void oled_exit_sleep(void)
{
	// charge pump
	ssd1306_command(0x8D);		// charge pump setting
	ssd1306_command(0x14);

	ssd1306_command(0xAF);		// set display on

	oled_clr_screen();
	osal_set_event(sensorTag_TaskID, EVT_DISP);
}


/*
 *
 */
void oled_set_font(const struct font_def *f)
{
	font = f;
}


void oled_putc(char c)
{
	unsigned char		i;
	unsigned short		base;
	const unsigned char	*p = font->tbl;

	if ((c < font->firstchar) || (c > font->lastchar)) {
		c = font->lastchar;
	}

	base  = (c - font->firstchar) * font->width;
	p    += base;
	for (i=0; i<font->width; i++) {
		ssd1306_data(*p++);
	}
}

void oled_puts(char *s)
{
	while (*s) {
		oled_putc(*s);
		s++;
	}
}

void oled_puts_04x(unsigned short n)
{
	unsigned char	tmp[4];

	tmp[0] = (n      ) & 0x000F;
	tmp[1] = (n >>  4) & 0x000F;
	tmp[2] = (n >>  8) & 0x000F;
	tmp[3] = (n >> 12) & 0x000F;
	if (tmp[3] < 10) { oled_putc(tmp[3]+0x30); } else { oled_putc(tmp[3]+0x61-10); }
	if (tmp[2] < 10) { oled_putc(tmp[2]+0x30); } else { oled_putc(tmp[2]+0x61-10); }
	if (tmp[1] < 10) { oled_putc(tmp[1]+0x30); } else { oled_putc(tmp[1]+0x61-10); }
	if (tmp[0] < 10) { oled_putc(tmp[0]+0x30); } else { oled_putc(tmp[0]+0x61-10); }
}

void oled_puts_05u(unsigned short n)
{
	unsigned char	tmp[5];

	tmp[0] = n % 10;
	n      = n / 10;

	tmp[1] = n % 10;
	n      = n / 10;

	tmp[2] = n % 10;
	n      = n / 10;

	tmp[3] = n % 10;
	n      = n / 10;

	tmp[4] = n;
	oled_putc(tmp[4]+0x30);
	oled_putc(tmp[3]+0x30);
	oled_putc(tmp[2]+0x30);
	oled_putc(tmp[1]+0x30);
	oled_putc(tmp[0]+0x30);
}


void oled_draw_icon(unsigned char row, unsigned char col, unsigned char idx)
{
	unsigned char		i, k;
	const unsigned char	*p;

	k = font->height / 8;
	p = &font->tbl[idx*font->width*k];
	while (k--) {
		ssd1306_set_pagecol(row++, col + 32);
		for (i=0; i<font->width; i++) {
			ssd1306_data(*p++);
		}
	}
}

void oled_draw_num(unsigned char row, unsigned char col, unsigned short n)
{
	unsigned char	offset;
	unsigned char	tmp[5];
	unsigned char	location[] = { 0, 8, 16, 24, 32 };

	tmp[0] = n % 10;
	n      = n / 10;
	tmp[1] = n % 10;
	n      = n / 10;
	tmp[2] = n % 10;
	n      = n / 10;
	tmp[3] = n % 10;
	n      = n / 10;
	tmp[4] = n;

	offset = 0;
	if (tmp[4] != 0) {
		oled_draw_icon(row, col + location[offset++], tmp[4]);
	}
	if ((tmp[4] | tmp[3]) != 0) {
		oled_draw_icon(row, col + location[offset++], tmp[3]);
	}
	if ((tmp[4] | tmp[3] | tmp[2]) != 0) {
		oled_draw_icon(row, col + location[offset++], tmp[2]);
	}
	if ((tmp[4] | tmp[3] | tmp[2] | tmp[1]) != 0) {
		oled_draw_icon(row, col + location[offset++], tmp[1]);
	}
	oled_draw_icon(row, col + location[offset], tmp[0]);
}
