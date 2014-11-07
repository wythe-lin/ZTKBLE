/*
 ******************************************************************************
 * oled.h -
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

#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fonts.h"


#define OLED_WIDTH		64
#define OLED_HEIGHT		32

/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern void		oled_init(void);
extern void		oled_enter_sleep(void);
extern void		oled_exit_sleep(void);
extern void		oled_set_font(const struct font_def *f);
extern void		oled_clr_screen(void);
extern unsigned char	oled_gotoxy(unsigned char row, unsigned char col);
extern void		oled_putc(char c);
extern void		oled_puts(char *s);

extern void		oled_puts_04x(unsigned short n);
extern void		oled_puts_05u(unsigned short n);

extern void		oled_draw_icon(unsigned char row, unsigned char col, unsigned char idx);
extern void		oled_draw_num(unsigned char row, unsigned char col, unsigned short n);

#ifdef __cplusplus
};
#endif
#endif /* __OLED_H__ */