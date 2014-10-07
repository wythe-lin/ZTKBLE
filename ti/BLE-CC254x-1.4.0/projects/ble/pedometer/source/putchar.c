/*
 ******************************************************************************
 * putchar.c
 *
 * Copyright (c) 2014-2015 by ZealTek Electronic Co., Ltd.
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
 *	2014.06.20	T.C. Chiu
 *
 ******************************************************************************
 */
#include <iocc2541.h>
#include <stdio.h>

#define UARTPORT		0
#if (UARTPORT == 0)
    #define UXDBUF		U0DBUF
    #define UTXIF		UTX0IF
#else
    #define UXDBUF		U1DBUF
    #define UTXIF		UTX1IF
#endif

#if (__CODE_MODEL__ == __CM_BANKED__)
__near_func int putchar(int c)
#else
MEMORY_ATTRIBUTE int putchar(int c)
#endif
{
repeat:
	while (!UTXIF)
		;

	UTXIF  = 0;
	UXDBUF = c;

	if (c == '\n')  {
		c = '\r';
		goto repeat;
	}
	return c;
}

