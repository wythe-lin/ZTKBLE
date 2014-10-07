/*
 ******************************************************************************
 * uart.h -
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
#include <ioCC2541.h>


/*
 * Baudrate = 115200
 * Fsys     = 32MHz
 * M = 216, E = 11
 */
//#define UART_BAUD_M		216
//#define UART_BAUD_E		11
#define UART_BAUD_M		216
#define UART_BAUD_E		14


/*
 ******************************************************************************
 * Global Function
 ******************************************************************************
 */
void uart_init(void)
{
	CLKCONCMD  &= ~(1 << 6);
	while (CLKCONSTA & (1 << 6))
		;

	CLKCONCMD  &= ~((1 << 6) | (7 << 0));
	PERCFG     |=  0x01;
	P1SEL      |=  (0x0F << 2);
	P2DIR      &= ~(0xC0);

	U0CSR      |=  (1 << 7);
	U0GCR      &= ~(0x1F);
	U0GCR      |=  UART_BAUD_E;
	U0BAUD      =  UART_BAUD_M;
	U0CSR      |=  (1 << 6);
	UTX0IF      =  1;
//	UTX0IF      =  0;
}
