/*
 ******************************************************************************
 * ssd1306.h -
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

#ifndef __SSD1306_H__
#define __SSD1306_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern unsigned char	ssd1306_command(unsigned char cmd);
extern unsigned char	ssd1306_data(unsigned char data);
extern void		ssd1306_set_pagecol(unsigned char page, unsigned char col);
extern void		ssd1306_clr_gddram(void);


#ifdef __cplusplus
};
#endif
#endif /* __SSD1306_H__ */