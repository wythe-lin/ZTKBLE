/*
 ******************************************************************************
 * hash.c - hash table control & access
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

#include <osal.h>

#include "hash.h"


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

/*
 *****************************************************************************
 *
 * definitions
 *
 *****************************************************************************
 */
#define HASBTBL_INTERVAL	(12)
#define HASBTBL_BLOCK		(60/HASBTBL_INTERVAL)
#define HASHTBL_MAXSZ		(7*24*HASBTBL_BLOCK)

typedef union {
	unsigned short	sa[HASHTBL_MAXSZ];
	unsigned short	da[7][HASHTBL_MAXSZ/7];
} hash_t;


/*
 *****************************************************************************
 *
 * variables
 *
 *****************************************************************************
 */
static unsigned short	hash_len;
static unsigned short	hash_in;
static unsigned short	hash_out;

hash_t			ptdb;


/*
 *****************************************************************************
 *
 * public functions
 *
 *****************************************************************************
 */
/*
 * @fn		hash_set_putkey
 *
 * @brief
 *
 * @param	week   = 1 ~ 7
 * @param	hour   = 0 ~ 23
 * @param	minute = 0 ~ 59
 *
 * @return	none
 */
void hash_set_putkey(unsigned char week, unsigned char hour, unsigned char minute)
{
	hash_in  = (week - 1) * (HASHTBL_MAXSZ/7);
	hash_in += (hour / HASBTBL_BLOCK) * (HASHTBL_MAXSZ/7/4);
	hash_in += minute / HASBTBL_INTERVAL;

	dmsg(("\033[1;37min=%02x, w=%02x, h=%02x, m=%02x\033[0m\n", hash_in, week, hour, minute));
}

void hash_set_getkey(unsigned char week, unsigned char hour)
{
	hash_out  = (week - 1) * (HASHTBL_MAXSZ/7);
	hash_out += (hour / HASBTBL_BLOCK) * (HASHTBL_MAXSZ/7/4);

	dmsg(("\033[1;37mout=%02x, w=%02x, h=%02x\033[0m\n", hash_out, week, hour));
}


/*
 * @fn		hash_rst
 *
 * @brief
 *
 * @param	
 *
 * @return	none
 */
void hash_rst(void)
{
	unsigned short	i;

	for (i=0; i<HASHTBL_MAXSZ; i++) {
//		ptdb.sa[i] = 1;
		ptdb.sa[i] = 0xc000;
	}
	hash_len = 0;
	hash_out = 0;
	hash_in  = 0;
}


char hash_is_full(void)
{
	return (hash_len >= HASHTBL_MAXSZ) ? 1 : 0;
}

char hash_is_empty(void)
{
	return (!hash_len) ? 1 : 0;
}


/*
 * @fn		hash_put
 *
 * @brief
 *
 */
void hash_put(void *value)
{
	unsigned short	*p = (unsigned short *) value;

	if (hash_len < HASHTBL_MAXSZ) {
		hash_len++;
		ptdb.sa[hash_in] = *p;
		if (++hash_in >= HASHTBL_MAXSZ) {
			hash_in = 0;
		}
		dmsg(("\033[1;37m[record]:%04x\033[0m\n", *p));
	}
}

void *hash_get(void)
{
	unsigned short	i = hash_out;

	if (++hash_out >= HASHTBL_MAXSZ) {
		hash_out = 0;
	}
	return &ptdb.sa[i];
}


