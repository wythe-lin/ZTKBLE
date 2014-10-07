/*
 ******************************************************************************
 * vgm064032a1w01.h -
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

#ifndef __VGM064032A1W01_H__
#define __VGM064032A1W01_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fonts.h"


#define VGM064032A1W01_WIDTH		64
#define VGM064032A1W01_HEIGHT		32

/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern void		vgm064032a1w01_init(void);
extern void		vgm064032a1w01_enter_sleep(void);
extern void		vgm064032a1w01_exit_sleep(void);
extern void		vgm064032a1w01_set_font(const struct font_def *f);
extern void		vgm064032a1w01_clr_screen(void);
extern unsigned char	vgm064032a1w01_gotoxy(unsigned char row, unsigned char col);
extern void		vgm064032a1w01_putc(char c);
extern void		vgm064032a1w01_puts(char *s);

extern void		vgm064032a1w01_puts_04x(unsigned short n);
extern void		vgm064032a1w01_puts_05u(unsigned short n);

extern void		vgm064032a1w01_draw_icon(unsigned char row, unsigned char col, unsigned char idx);
extern void		vgm064032a1w01_draw_num(unsigned char row, unsigned char col, unsigned short n);

#ifdef __cplusplus
};
#endif
#endif /* __VGM064032A1W01_H__ */