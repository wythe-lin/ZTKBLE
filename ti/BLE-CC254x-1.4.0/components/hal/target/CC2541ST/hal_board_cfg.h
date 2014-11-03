/**************************************************************************************************
  Filename:       hal_board_cfg.h
  Revised:        $Date: 2013-02-27 11:32:02 -0800 (Wed, 27 Feb 2013) $
  Revision:       $Revision: 33315 $

  Description:    Board configuration for CC2541 Sensor Tag


  Copyright 2012  Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef HAL_BOARD_CFG_H
#define HAL_BOARD_CFG_H

/*
 *     =============================================================
 *     |            Texas Instruments CC2541 Sensor Board          |
 *     | --------------------------------------------------------- |
 *     |  mcu   : 8051 core                                        |
 *     |  clock : 32MHz                                            |
 *     =============================================================
 */

#define ACTIVE_LOW        !
#define ACTIVE_HIGH       !!    /* double negation forces result to be '1' */

/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */

#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"

/* ------------------------------------------------------------------------------------------------
 *                                       Board Indentifier
 *
 *      Define the Board Identifier to CC2541_SB
 * ------------------------------------------------------------------------------------------------
 */

#define CC2541_SB

/* ------------------------------------------------------------------------------------------------
 *                                          Clock Speed
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_CPU_CLOCK_MHZ		32
#define EXTERNAL_CRYSTAL_OSC		0x00
#define INTERNAL_RC_OSC			0x80

/*
 * If not using power management, assume that the 32kHz crystal is not
 * installed. Even if a 32kHz crystal is present on the board, it will
 * never get used since device does not ever go to sleep. By forcing
 * OSC32K_CRYSTAL_INSTALLED to FALSE, we avoid mismatches between
 * libraries built with power management off, and applications in which
 * power management is not used.
 */
#if ( !defined ( POWER_SAVING ) ) && ( !defined ( OSC32K_CRYSTAL_INSTALLED ) )
  #define OSC32K_CRYSTAL_INSTALLED	FALSE
#endif

/* 32 kHz clock source select in CLKCONCMD */
#if !defined (OSC32K_CRYSTAL_INSTALLED) || (defined (OSC32K_CRYSTAL_INSTALLED) && (OSC32K_CRYSTAL_INSTALLED == TRUE))
  #define OSC_32KHZ			EXTERNAL_CRYSTAL_OSC	/* external 32 KHz xosc */
#else
  #define OSC_32KHZ			INTERNAL_RC_OSC		/* internal 32 KHz rcosc */
#endif

// Minimum Time for Stable External 32kHz Clock (in ms)
#define MIN_TIME_TO_STABLE_32KHZ_XOSC	400

/* ------------------------------------------------------------------------------------------------
 * LED Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_NUM_LEDS		3

#define HAL_LED_BLINK_DELAY()	st( { volatile uint32 i; for (i=0; i<0x5800; i++) { }; } )

/* 1 - D1 Red */
#define LED1_BV			BV(0)
#define LED1_SBIT		P1_0
#define LED1_DDR		P1DIR
#define LED1_SEL		P1SEL
#define LED1_POLARITY		ACTIVE_LOW

/* 2 - D2 Green */
#define LED2_BV			BV(6)
#define LED2_SBIT		P1_6
#define LED2_DDR		P1DIR
#define LED2_SEL		P1SEL
#define LED2_POLARITY		ACTIVE_LOW

/* 3 - D3 Blue */
#define LED3_BV			BV(7)
#define LED3_SBIT		P1_7
#define LED3_DDR		P1DIR
#define LED3_SEL		P1SEL
#define LED3_POLARITY		ACTIVE_LOW

/* ------------------------------------------------------------------------------------------------
 * Battery Voltage Detect Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define BATT_BV			BV(7)
#define BATT_SBIT		P0_7
#define BATT_DDR		P0DIR
#define BATT_SEL		P0SEL
#define BATT_INP		P0INP
#define BATT_ACG		APCFG

/* ------------------------------------------------------------------------------------------------
 * Battery Charge Detect Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define BATCD_BV		BV(4)
#define BATCD_SBIT		P1_4
#define BATCD_DDR		P1DIR
#define BATCD_SEL		P1SEL
#define BATCD_INP		P1INP

/* ------------------------------------------------------------------------------------------------
 * Motor Configuration
 * ------------------------------------------------------------------------------------------------
 */
#define MOTOR_BV		BV(4)
#define MOTOR_SBIT		P0_4
#define MOTOR_DDR		P0DIR
#define MOTOR_SEL		P0SEL


/* ------------------------------------------------------------------------------------------------
 * Sensor Configuration
 * ------------------------------------------------------------------------------------------------
 */
///* Accelerometer interrupt */
//#define ACC_INT_BV        BV(2)
//#define ACC_INT_SBIT      P0_2
//#define ACC_INT_DDR       P0DIR
//#define ACC_INT_SEL       P0SEL

/* ------------------------------------------------------------------------------------------------
 *                                    Push Button Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* See hal_keys.h */

/* ------------------------------------------------------------------------------------------------
 *                         OSAL NV implemented by internal flash pages.
 * ------------------------------------------------------------------------------------------------
 */
// Flash is partitioned into 8 banks of 32 KB or 16 pages.
#define HAL_FLASH_PAGE_PER_BANK		16
// Flash is constructed of 128 pages of 2 KB.
#define HAL_FLASH_PAGE_SIZE		2048
#define HAL_FLASH_WORD_SIZE		4

// CODE banks get mapped into the XDATA range 8000-FFFF.
#define HAL_FLASH_PAGE_MAP		0x8000

// The last 16 bytes of the last available page are reserved for flash lock bits.
// NV page definitions must coincide with segment declaration in project *.xcl file.
#if defined NON_BANKED
#define HAL_FLASH_LOCK_BITS		16
#define HAL_NV_PAGE_END			30
#else
#define HAL_FLASH_LOCK_BITS		16
#define HAL_NV_PAGE_END			126
#endif

// Re-defining Z_EXTADDR_LEN here so as not to include a Z-Stack .h file.
#define HAL_FLASH_IEEE_SIZE		8
#define HAL_FLASH_IEEE_PAGE		(HAL_NV_PAGE_END+1)
#define HAL_FLASH_IEEE_OSET		(HAL_FLASH_PAGE_SIZE - HAL_FLASH_LOCK_BITS - HAL_FLASH_IEEE_SIZE)
#define HAL_INFOP_IEEE_OSET		0xC

#define HAL_NV_PAGE_CNT			2
#define HAL_NV_PAGE_BEG			(HAL_NV_PAGE_END-HAL_NV_PAGE_CNT+1)

// Used by DMA macros to shift 1 to create a mask for DMA registers.
#define HAL_NV_DMA_CH			0
#define HAL_DMA_CH_RX			3
#define HAL_DMA_CH_TX			4

#define HAL_NV_DMA_GET_DESC()		HAL_DMA_GET_DESC0()
#define HAL_NV_DMA_SET_ADDR(a)		HAL_DMA_SET_ADDR_DESC0((a))


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */

/* ----------- Cache Prefetch control ---------- */
#define PREFETCH_ENABLE()		st( FCTL = 0x08; )
#define PREFETCH_DISABLE()		st( FCTL = 0x04; )

/* Setting Clocks */

// switch to the 16MHz HSOSC and wait until it is stable
#define SET_OSC_TO_HSOSC()                                                     \
{                                                                              \
  CLKCONCMD = (CLKCONCMD & 0x80) | CLKCONCMD_16MHZ;                            \
  while ( (CLKCONSTA & ~0x80) != CLKCONCMD_16MHZ );                            \
}

// switch to the 32MHz XOSC and wait until it is stable
#define SET_OSC_TO_XOSC()                                                      \
{                                                                              \
  CLKCONCMD = (CLKCONCMD & 0x80) | CLKCONCMD_32MHZ;                            \
  while ( (CLKCONSTA & ~0x80) != CLKCONCMD_32MHZ );                            \
}

// set 32kHz OSC and wait until it is stable
#define SET_32KHZ_OSC()                                                        \
{                                                                              \
  CLKCONCMD = (CLKCONCMD & ~0x80) | OSC_32KHZ;                                 \
  while ( (CLKCONSTA & 0x80) != OSC_32KHZ );                                   \
}

#define START_HSOSC_XOSC()                                                     \
{                                                                              \
  SLEEPCMD &= ~OSC_PD;            /* start 16MHz RCOSC & 32MHz XOSC */         \
  while (!(SLEEPSTA & XOSC_STB)); /* wait for stable 32MHz XOSC */             \
}

#define STOP_HSOSC()                                                           \
{                                                                              \
  SLEEPCMD |= OSC_PD;             /* stop 16MHz RCOSC */                       \
}
/* ----------- Board Initialization ---------- */

#define HAL_BOARD_INIT()							\
{										\
   /* Set to 16Mhz to set 32kHz OSC, then back to 32MHz */			\
	START_HSOSC_XOSC();							\
	SET_OSC_TO_HSOSC();							\
	SET_32KHZ_OSC();							\
	SET_OSC_TO_XOSC();							\
	STOP_HSOSC();								\
										\
	/* Turn on cache prefetch mode */					\
	PREFETCH_ENABLE();							\
										\
	/* LEDs */								\
	LED1_SEL   &= ~LED1_BV;							\
	LED2_SEL   &= ~LED2_BV;							\
	LED3_SEL   &= ~LED3_BV;							\
	LED1_SBIT   =  LED1_POLARITY(0);					\
	LED2_SBIT   =  LED2_POLARITY(0);					\
	LED3_SBIT   =  LED3_POLARITY(0);					\
	LED1_DDR   |=  LED1_BV;							\
	LED2_DDR   |=  LED2_BV;							\
	LED3_DDR   |=  LED3_BV;							\
										\
	/* motor control pin */							\
	MOTOR_SEL  &= ~MOTOR_BV;	/* general-purpose I/O */		\
	MOTOR_SBIT  = 0;		/* output low */			\
	MOTOR_DDR  |=  MOTOR_BV;	/* output */				\
										\
	/* battery voltage detect pin */					\
	BATT_SEL   |=  BATT_BV;		/* peripheral function */		\
	BATT_INP   |=  BATT_BV;		/* input mode 3-state */		\
	BATT_DDR   &= ~BATT_BV;		/* input mode */			\
										\
	/* battery charge detect pin */						\
	BATCD_SEL  &= ~BATCD_BV;	/* general-purpose I/O */		\
	BATCD_INP  &= ~BATCD_BV;	/* input mode pull-up */		\
	BATCD_DDR  &= ~BATCD_BV;	/* input mode */			\
}


/* ----------- LED's ---------- */
#define HAL_TURN_OFF_LED1()	st( LED1_SBIT = LED1_POLARITY(0); )
#define HAL_TURN_OFF_LED2()	st( LED2_SBIT = LED2_POLARITY(0); )
#define HAL_TURN_OFF_LED3()	st( LED3_SBIT = LED3_POLARITY(0); )

#define HAL_TURN_ON_LED1()	st( LED1_SBIT = LED1_POLARITY(1); )
#define HAL_TURN_ON_LED2()	st( LED2_SBIT = LED2_POLARITY(1); )
#define HAL_TURN_ON_LED3()	st( LED3_SBIT = LED3_POLARITY(1); )

#define HAL_TOGGLE_LED1()	st( if (LED1_SBIT) { LED1_SBIT = 0; } else { LED1_SBIT = 1;} )
#define HAL_TOGGLE_LED2()	st( if (LED2_SBIT) { LED2_SBIT = 0; } else { LED2_SBIT = 1;} )
#define HAL_TOGGLE_LED3()	st( if (LED3_SBIT) { LED3_SBIT = 0; } else { LED3_SBIT = 1;} )

#define HAL_STATE_LED1()	(LED1_POLARITY(LED1_SBIT))
#define HAL_STATE_LED2()	(LED2_POLARITY(LED2_SBIT))
#define HAL_STATE_LED3()	(LED3_POLARITY(LED3_SBIT))

/* ------------------------------------------------------------------------------------------------
 *                                     Driver Configuration
 * ------------------------------------------------------------------------------------------------
 */

/* Set to TRUE enable H/W TIMER usage, FALSE disable it */
#ifndef HAL_TIMER
#define HAL_TIMER	FALSE
#endif

/* Set to TRUE enable ADC usage, FALSE disable it */
#ifndef HAL_ADC
#define HAL_ADC		FALSE
#endif

/* Set to TRUE enable DMA usage, FALSE disable it */
#ifndef HAL_DMA
#define HAL_DMA		TRUE
#endif

/* Set to TRUE enable Flash access, FALSE disable it */
#ifndef HAL_FLASH
#define HAL_FLASH	TRUE
#endif

/* Set to TRUE enable AES usage, FALSE disable it */
#ifndef HAL_AES
#define HAL_AES		FALSE
#endif

#ifndef HAL_AES_DMA
#define HAL_AES_DMA	FALSE
#endif

/* Set to TRUE enable LCD usage, FALSE disable it */
#ifndef HAL_LCD
#define HAL_LCD		FALSE
#endif

/* Set to TRUE enable LED usage, FALSE disable it */
#ifndef HAL_LED
#define HAL_LED		TRUE
#endif
#if (!defined BLINK_LEDS) && (HAL_LED == TRUE)
#define BLINK_LEDS
#endif

/* Set to TRUE enable KEY usage, FALSE disable it */
#ifndef HAL_KEY
#define HAL_KEY		TRUE
#endif

/* Set to TRUE enable UART usage, FALSE disable it */
#ifndef HAL_UART
#define HAL_UART	FALSE
#endif

#if defined(HAL_UART) && (HAL_UART == TRUE)
   // Always prefer to use DMA over ISR.
   #if defined(HAL_DMA) && (HAL_DMA == TRUE)
      #ifndef HAL_UART_DMA
         #define HAL_UART_DMA		1
      #endif
      #ifndef HAL_UART_ISR
         #define HAL_UART_ISR		0
      #endif
   #else
      #ifndef HAL_UART_ISR
         #define HAL_UART_ISR		1
      #endif
      #ifndef HAL_UART_DMA
         #define HAL_UART_DMA		0
      #endif
   #endif

   // Used to set P2 priority - USART0 over USART1 if both are defined.
   #if ((HAL_UART_DMA == 1) || (HAL_UART_ISR == 1))
      #define HAL_UART_PRIPO	0x00
   #else
      #define HAL_UART_PRIPO		0x40
   #endif
#else
   #define HAL_UART_DMA			0
   #define HAL_UART_ISR			0
#endif

#if !defined HAL_UART_SPI
#define HAL_UART_SPI			0
#endif

/*******************************************************************************************************
*/
#endif
