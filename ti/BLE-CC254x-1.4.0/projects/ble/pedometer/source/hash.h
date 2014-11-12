/*
 ******************************************************************************
 * hash.h - hash table control & access
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
 *	2014.11.07	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#ifndef __HASH_H__
#define __HASH_H__

#ifdef __cplusplus
extern "C"
{
#endif



/*
 *****************************************************************************
 *
 * variables
 *
 *****************************************************************************
 */



/*
 *****************************************************************************
 *
 * public functions
 *
 *****************************************************************************
 */
extern void	hash_rst(void);
extern char	hash_is_full(void);
extern char	hash_is_empty(void);
extern void	hash_put(void *value);
extern void	*hash_get(void);
extern void	hash_set_putkey(unsigned char week, unsigned char hour, unsigned char minute);
extern void	hash_set_getkey(unsigned char week, unsigned char hour);


#ifdef __cplusplus
}
#endif

#endif /* __HASH_H__ */

