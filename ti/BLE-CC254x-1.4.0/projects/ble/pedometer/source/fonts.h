/*
 ******************************************************************************
 * fonts.h -
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

#ifndef __FONTS_H__
#define __FONTS_H__

#ifdef __cplusplus
extern "C" {
#endif


struct font_def {
    unsigned char		width;			/* character width for storage */
    unsigned char		height;			/* character height for storage */
    unsigned char		firstchar;		/* the first character available */
    unsigned char		lastchar;		/* the last character available */
    const unsigned char		*tbl;			/* font table start address in memory */
};


/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern const struct font_def	*font;

extern const struct font_def	fonts7x8;
extern const struct font_def	num8x16;
extern const struct font_def	icon16x16;
extern const struct font_def	bigbatt40x32;


#ifdef __cplusplus
};
#endif
#endif /* __FONTS_H__ */

