/**************************************************************************************************
  Filename:       wcube.c
  Revised:        $Date: 2013-08-23 11:45:31 -0700 (Fri, 23 Aug 2013) $
  Revision:       $Revision: 35100 $

  Description:    This file contains the Sensor Tag sample application
                  for use with the TI Bluetooth Low Energy Protocol Stack.

  Copyright 2012-2013  Texas Instruments Incorporated. All rights reserved.

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

/*
 *****************************************************************************
 *
 * includes
 *
 *****************************************************************************
 */
#include <bcomdef.h>
#include <OSAL.h>
#include <OSAL_PwrMgr.h>
#include <OSAL_Clock.h>
#include <OSAL_Memory.h>
#include <osal_bufmgr.h>

#include <OnBoard.h>
#include <hal_drivers.h>
#include <hal_adc.h>
#include <hal_led.h>
#include <hal_keys.h>

#include <gatt.h>
#include <hci.h>

#include <gapgattserver.h>
#include <gattservapp.h>

#include <peripheral.h>
#include <gapbondmgr.h>

#if defined(FEATURE_OAD)
   #include <oad.h>
   #include <oad_target.h>
#endif

// Services
#include "devinfo.h"
#include "battserv.h"
#include "uartserv.h"

// Sensor drivers
#include "wcube.h"
#include "gplink.h"

#include "defines.h"

#include "uart.h"
#include "plantimer.h"


/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define MSG_NONE		0
#define MSG_DBG			0x0001
#define MSG_FLOW		0x0002
#define MSG_PWMGR		0x0004
#define MSG_BLE			0x0008
#define MSG_UART		0x0010
#define MSG_BATT		0x0020

#define DEBUG_MESSAGE		\
(				\
/*	MSG_BATT	|*/	\
	MSG_UART	|	\
	MSG_BLE		|	\
	MSG_PWMGR	|	\
	MSG_FLOW	|	\
	MSG_DBG		|	\
	MSG_NONE		\
)

#if (DEBUG_MESSAGE > 0)
    #include <stdio.h>
#endif
#if (DEBUG_MESSAGE & MSG_DBG)
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_FLOW)
    #define fmsg(x)		printf x
#else
    #define fmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_BLE)
    #define btmsg(x)		printf x
#else
    #define btmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_UART)
    #define uartmsg(x)		printf x
#else
    #define uartmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_BATT)
    #define battmsg(x)		printf x
#else
    #define battmsg(x)
#endif


/*
 *****************************************************************************
 *
 * constants
 *
 *****************************************************************************
 */
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL		(160*2)

// Maximum time to remain advertising, when in Limited Discoverable mode. In seconds (default 180 seconds)
#define DEFAULT_ADVERT_MAX_TIMEOUT		(0)	// (30)

// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE		GAP_ADTYPE_FLAGS_LIMITED

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
// iOS request minimum interval > 20ms
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL	(80*1)

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
// iOS request maximum interval < 2s
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL	(800)

// Slave latency to use if automatic parameter update request is enabled
// iOS request slave latency < 4s
#define DEFAULT_DESIRED_SLAVE_LATENCY		(1)

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
// Supervision Timeout > (1 + Slave Latency) * (Connection Interval)
#define DEFAULT_DESIRED_CONN_TIMEOUT		(1000)

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST		FALSE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL		(8)



/*
 *****************************************************************************
 *
 * global variables
 *
 *****************************************************************************
 */
uint8		wCube_TaskID;		// Task ID for internal task/event processing


/*
 *****************************************************************************
 *
 * local variables
 *
 *****************************************************************************
 */

static gaprole_States_t		gapProfileState = GAPROLE_INIT;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8		scanRspData[] = {
	// complete name
	0x08,			// length of this data
	GAP_ADTYPE_LOCAL_NAME_COMPLETE,
	'Z',
	'T',
	'-',
	'3',
	'0',
	'0',
	'0',

	// connection interval range
	0x05,			// length of this data
	GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
	LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
	LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),
	HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

	// Tx power level
	0x02,			// length of this data
	GAP_ADTYPE_POWER_LEVEL,
	0			// 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8		advertData[] = {
	// Flags; this sets the device to use limited discoverable
	// mode (advertises for 30 seconds at a time) instead of general
	// discoverable mode (advertises indefinitely)
	0x02,				// length of this data
	GAP_ADTYPE_FLAGS,
	DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	// service UUID, to notify central devices what services are included
	// in this peripheral
	0x03,				// length of this data
	GAP_ADTYPE_16BIT_MORE,		// some of the UUID's, but not all
	LO_UINT16(UARTSERV1_UUID),
	HI_UINT16(UARTSERV1_UUID),

	0x03,				// length of this data
	GAP_ADTYPE_16BIT_MORE,		// some of the UUID's, but not all
	LO_UINT16(UARTSERV2_UUID),
	HI_UINT16(UARTSERV2_UUID),
};

// GAP GATT Attributes
#if defined(HAL_IMAGE_A)
static uint8		attDeviceName[] = "ZT-3000";
#elif defined(HAL_IMAGE_B)
static uint8		attDeviceName[] = "ZT-3000";
#else
static uint8		attDeviceName[] = "ZT-3000";
#endif

// sensor state variables
static bool		key1_press = FALSE;

static unsigned char	rxpkt[128];


/*
 *****************************************************************************
 *
 * private functions
 *
 *****************************************************************************
 */
/**
 * @fn      wCube_ClockSet
 *
 * @brief   for real time clock
 *
 * @param
 *
 * @return  none
 */
static void wCube_ClockSet(UTCTimeStruct *tm)
{
	osal_setClock(osal_ConvertUTCSecs(tm));
}

static void wCube_ClockGet(UTCTimeStruct *t, UTCTime delta)
{
	osal_ConvertUTCTime(t, osal_getClock() - delta);
}


/**
 * @fn      batt_measure
 *
 * @brief   Zeller's congruence:
 *	    W = Y + D + [Y / 4] + [C / 4] + [26 x (M + 1) / 10] �V 2C - 1
 *
 * @param   none
 *
 * @return
 */
static unsigned char wCube_CalcWeek(UTCTimeStruct *t)
{
	short	century, year, month, day, week;

	dmsg((" - %04d.%02d.%02d", t->year, t->month, t->day));

	//
	if (t->month < 3) {
		t->year  -= 1;
		t->month += 12;
	}
	day     = t->day;
	month	= t->month;
	year	= t->year % 100;
	century = t->year / 100;

	//
	week    = year;
	week   += day;
	week   += (year / 4);
	week   += (century / 4);
	week   += (26 * (month + 1) / 10);
	week   -= (2 * century);
	week   -= 1;
	week   %= 7;
	week    = (week < 0) ? week+7 : week;
	week    = week % 7;

	dmsg((" (%02d)\n", (unsigned char) week));
	return (unsigned char) week;
}


/**
 * @fn      batt_measure
 *
 * @brief   Measure the battery level with the ADC
 *
 * @param   none
 *
 * @return
 */
static unsigned char batt_measure(void)
{
	unsigned short	adc;
	unsigned char	batt_level;

	// configure ADC and perform a read
	HalAdcSetReference(HAL_ADC_REF_125V);
	adc = HalAdcRead(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_14);

	if (!BATCD_SBIT) {
		batt_level = 7;	// battery charge
	} else if ((adc >= BATT_LEVEL_01)) {
		batt_level = 6;	// battery full
	} else if ((adc >= BATT_LEVEL_02) && (adc < BATT_LEVEL_01)) {
		batt_level = 5;
	} else if ((adc >= BATT_LEVEL_03) && (adc < BATT_LEVEL_02)) {
		batt_level = 4;
	} else if ((adc >= BATT_LEVEL_04) && (adc < BATT_LEVEL_03)) {
		batt_level = 3;
	} else if ((adc >= BATT_LEVEL_05) && (adc < BATT_LEVEL_04)) {
		batt_level = 2;
	} else if ((adc >= BATT_LEVEL_06) && (adc < BATT_LEVEL_05)) {
		batt_level = 1;	// battery empty
	} else {
		batt_level = 0;	// battery shutdown
	}
	battmsg(("adc=%04x, lv=%02x\n", adc, batt_level));
	return batt_level;
}

static unsigned char batt_get_level(void)
{
	unsigned char	precision;

	switch (batt_measure()) {
	case 7:	precision = 100; break;
	case 6:	precision = 100; break;
	case 5:	precision =  80; break;
	case 4:	precision =  60; break;
	case 3:	precision =  40; break;
	case 2:	precision =  20; break;
	case 1:	precision =   0; break;
	case 0:	precision =   0; break;
	}
	return precision;
}


/**
 * @fn      wCube_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void wCube_HandleKeys(uint8 shift, uint8 keys)
{
	VOID	shift;		// Intentionally unreferenced parameter

	if (keys & HAL_KEY_SW_1) {
		// press KEY1
		key1_press = TRUE;


	} else {
		// release KEY1
		key1_press = FALSE;

	}
}


/**
 * @fn      wCube_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void wCube_ProcessOSALMsg(osal_event_hdr_t *msg)
{
	switch (msg->event) {
	case KEY_CHANGE:
		wCube_HandleKeys(((keyChange_t *) msg)->state, ((keyChange_t *) msg)->keys);
		break;

	default:
		// do nothing
		break;
	}
}



/**
 * @fn      pperipheral_StateNotification
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void pperipheral_StateNotification(gaprole_States_t newState)
{
	switch (newState) {
	case GAPROLE_STARTED:
		{
			uint8	ownAddress[B_ADDR_LEN];
			uint8	systemId[DEVINFO_SYSTEM_ID_LEN];

			GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

			// use 6 bytes of device address for 8 bytes of system ID value
			systemId[0] = ownAddress[0];
			systemId[1] = ownAddress[1];
			systemId[2] = ownAddress[2];

			// set middle bytes to zero
			systemId[4] = 0x00;
			systemId[3] = 0x00;

			// shift three bytes up
			systemId[7] = ownAddress[5];
			systemId[6] = ownAddress[4];
			systemId[5] = ownAddress[3];

			DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
			btmsg(("\033[1;34m{started}\033[0m - %02x:%02x:%02x:%02x:%02x:%02x\n",
				ownAddress[0], ownAddress[1], ownAddress[2], ownAddress[3], ownAddress[4], ownAddress[5]));
		}
		break;

	case GAPROLE_ADVERTISING:
		btmsg(("\033[1;34m{advertising}\033[0m\n"));
		break;

	case GAPROLE_CONNECTED:
		btmsg(("\033[1;34m{connected}\033[0m\n"));
		break;

	case GAPROLE_WAITING:
		btmsg(("\033[1;34m{waiting}\033[0m\n"));
		break;

	case GAPROLE_CONNECTED_ADV:
		btmsg(("\033[1;34m{connected adv.}\033[0m\n"));
		break;

	case GAPROLE_WAITING_AFTER_TIMEOUT:
		btmsg(("\033[1;34m{timeout}\033[0m\n"));
		break;

	case GAPROLE_ERROR:
		btmsg(("\033[1;34m{error}\033[0m\n"));
		break;

	default:
	case GAPROLE_INIT:
		btmsg(("\033[1;34m{init}\033[0m\n"));
		break;
	}
	gapProfileState = newState;
}


/*
 ******************************************************************************
 *
 * about ser access functions
 *
 ******************************************************************************
 */
/*
 * @fn		uartServ1_PktParsing
 *
 * @brief
 *
 * @param
 * @param
 *
 * @return
 */
static unsigned char chk_p0_2(void)
{
	P2INP &= ~(1 << 5);	// p0 is pull-up
	P0SEL &= ~(1 << 2);	// p0.2 is general-purpose I/O
	P0DIR &= ~(1 << 2);	// p0.2 is input
	return P0_2;
}

static const unsigned char	pic[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,		0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,		0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,		0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
                                                                                                             
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
};

static char uartServ2_PktParsing(uartpkt_t *pkt)
{
	char		i;
	unsigned char	len    = pkt->header.len;
	unsigned char	cmd    = pkt->header.command;
	unsigned char	blk    = pkt->buf[4];
	unsigned char	chksum = 0;

	// check checksum
	for (i=1; i<len-2; i++) {
		chksum += pkt->buf[i];
	}
	if (chksum != pkt->buf[len-2]) {
		return 0;
	}

	// packet parsing
	switch (cmd) {
	case SET_DATE:
	case RECORD_START:
	case RECORD_STOP:
	case SNAPSHOT:
	case POWER_MANAGE:
	case READ_STATUS:
	case INQUIRY_PIC:
	case INQUIRY_BLOCK:
	case WRITE_GPIO:
	case READ_GPIO:
	case WRITE_PLAN:
		if (chk_p0_2()) {
			dmsg(("send status packet\n"));
		} else {
			dmsg(("make status packet\n"));
		}
		len         = 10;
		pkt->buf[0] = 0xFA;					// leading
		pkt->buf[1] = len;					// length
		pkt->buf[2] = 0x80;					// command
		pkt->buf[3] = 0;					// ack
		pkt->buf[4] = 0;					// storage
		pkt->buf[5] = (cmd == RECORD_START) ? 0x01 : 0x00;	// status
		pkt->buf[6] = 0;
		switch (cmd) {
		case INQUIRY_PIC:	pkt->buf[7] = 0x01;	break;	// data
		case INQUIRY_BLOCK:	pkt->buf[7] = 0x03;	break;	// data
		case READ_GPIO:		pkt->buf[7] = 0x01;	break;	// data
		default:		pkt->buf[7] = 0x00;	break;	// data
		}
		pkt->buf[8] = 0x00;					// checksum
		pkt->buf[9] = 0xFE;					// ending
		break;

	case GET_PIC:
		if (chk_p0_2()) {
			dmsg(("send data block packet\n"));
		} else {
			dmsg(("make data block packet\n"));
		}
		len		= (blk == 2) ? 32+5 : 64+5;		// length
		pkt->buf[0]	= 0xFA;					// leading
		pkt->buf[1]	= len;					// length
		pkt->buf[2]	= 0x20;					// command
		pkt->buf[len-2] = 0x00;					// checksum
		pkt->buf[len-1] = 0xFE;					// ending

		switch (blk) {
		case 0:	osal_memcpy(&pkt->buf[3], &pic[  0], len-5);	break;
		case 1:	osal_memcpy(&pkt->buf[3], &pic[ 64], len-5);	break;
		case 2:	osal_memcpy(&pkt->buf[3], &pic[128], len-5);	break;
		}
		break;

	default:
		dmsg(("Unknown command (Serv2)\n"));
		return 0;
	}

	// calculate checksum
	for (i=1, chksum=0; i<len-2; i++) {
		chksum += pkt->buf[i];
	}
	pkt->buf[len-2] = chksum;
	return 1;
}


/*
 * @fn		uartServ1_PktParsing
 *
 * @brief
 *
 * @param
 * @param
 * @param
 *
 * @return
 */
static void gplink_send_ackpkt(uartpkt_t *pkt)
{
	if (chk_p0_2()) {
		uartServ2_PktParsing(pkt);
	}
}

static char	*resul[] = { "full HD", "HD", "VGA", "QVGA", "CIF", "QCIF" };
static char	*speed[] = { "1x", "2x", "3x", "4x" }; 
static char	*power[] = { "50Hz", "60Hz" };

static char uartServ1_PktParsing(uartpkt_t *pkt)
{
	char		i;
	unsigned char	chksum = 0;

	// check checksum
	for (i=1; i<pkt->header.len-2; i++) {
		chksum += pkt->buf[i];
	}
	if (chksum != pkt->buf[pkt->header.len-2]) {

		return 0;
	}

	// packet parsing
	switch (pkt->header.command) {
	case SET_DATE:
		dmsg(("command: set date - date:%04d/%02d/%02d time:%02d:%02d:%02d\n", pkt->buf[3]+2000, pkt->buf[4], pkt->buf[5], pkt->buf[6], pkt->buf[7], pkt->buf[8]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		{
			UTCTimeStruct	tm;

			tm.seconds = pkt->buf[8];	// 0-59
			tm.minutes = pkt->buf[7];	// 0-59
			tm.hour	   = pkt->buf[6];	// 0-23
			tm.day	   = pkt->buf[5];	// 0-30
			tm.month   = pkt->buf[4];	// 0-11
			tm.year	   = pkt->buf[3]+2000;	// 2000+
			osal_setClock(osal_ConvertUTCSecs(&tm));
		}
		break;

	case RECORD_START:
		dmsg(("command: record start - resolution:%s speed:%s power:%s\n", resul[pkt->buf[3]], speed[pkt->buf[4]], power[pkt->buf[5]]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case RECORD_STOP:
		dmsg(("command: record stop\n"));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case SNAPSHOT:
		dmsg(("command: snapshot - resolution:%s power:%s\n", resul[pkt->buf[3]], power[pkt->buf[4]]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case READ_STATUS:
		dmsg(("command: read status\n"));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case POWER_MANAGE:
		dmsg(("command: power manage - optin:%02d\n", pkt->buf[3]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case INQUIRY_PIC:
		dmsg(("command: inquiry pic\n"));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case INQUIRY_BLOCK:
		dmsg(("command: inquiry block - pic:%02d\n", pkt->buf[3]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		return 1;

	case GET_PIC:
		dmsg(("command: get pic - pic:%02d block:%02d\n", pkt->buf[3], pkt->buf[4]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		break;

	case WRITE_GPIO:
		dmsg(("command: write gpio - GPIO:%02d data:%02d\n", pkt->buf[3], pkt->buf[4]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);

		if (pkt->buf[3] >= 128) {

		}
		break;

	case READ_GPIO:
		dmsg(("command: read gpio - GPIO:%02d\n", pkt->buf[4]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);

		if (pkt->buf[3] >= 128) {

		}
		break;

	case WRITE_PLAN:
		dmsg(("command: write plan - id:%02d en:%02d type:%02d sHH:%02d sMM:%02d eHH:%02d eMM:%02d repeat:%02d\n",
			pkt->buf[3], pkt->buf[4], pkt->buf[5], pkt->buf[6], pkt->buf[7], pkt->buf[8], pkt->buf[9], pkt->buf[10]));
		gplink_send_ackpkt(pkt);
		gplink_send_pkt(pkt, pkt->header.len);
		{
			UTCTimeStruct	t;
			UTCTime		begin, end;

			t.seconds = 0;			// 0-59
			t.minutes = pkt->buf[7];	// 0-59
			t.hour    = pkt->buf[6];	// 0-23
			t.day     = 0;			// 0-30
			t.month   = 0;			// 0-11
			t.year    = 2000;		// 2000+
			begin	  = osal_ConvertUTCSecs(&t);

			t.minutes = pkt->buf[9];	// 0-59
			t.hour    = pkt->buf[8];	// 0-23
			end	  = osal_ConvertUTCSecs(&t);
			plantmr_update(pkt->buf[3], pkt->buf[4], pkt->buf[5], begin, end, pkt->buf[10]);
		}
		break;

	default:
		dmsg(("Unknown command (Serv1)\n"));
		return 0;
	}
	return 1;
}


/**
 * @fn      ptServ1ChgCB
 *
 * @brief   Callback from SimpleBLEProfile indicating a value change
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  none
 */
static void uartServ1ChgCB(uint8 paramID)
{
	switch (paramID) {
	case UARTSERV1_CHAR:
		uartServ1_GetParameter(UARTSERV1_CHAR, &uartpkt);
		uartServ1_PktParsing(&uartpkt);

		break;

	default:
		// should not reach here!
		break;
	}
}


/*
 *****************************************************************************
 *
 * profile functions
 *
 *****************************************************************************
 */
// GAP Role Callbacks
static gapRolesCBs_t	wCube_PeripheralCBs = {
	pperipheral_StateNotification,	// Profile State Change Callbacks
	NULL				// When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
#if !defined(GAPBONDMGR_NO_SUPPORT)
static gapBondCBs_t	wCube_BondMgrCBs = {
	NULL,				// Passcode callback (not used by application)
	NULL				// Pairing / Bonding state Callback (not used by application)
};
#endif

// ProTrack Profile Callbacks
static uartServ1CB_t	uartServ1CBs = uartServ1ChgCB;


/**
 * @fn      wCube_Init
 *
 * @brief   Initialization function for the Simple BLE Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ...).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void wCube_Init(uint8 task_id)
{
	wCube_TaskID = task_id;

	// Setup the GAP
	GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);

	// Setup the GAP Peripheral Role Profile
	{
		// Device starts advertising upon initialization
		uint8	current_adv_status = TRUE;

		// By setting this to zero, the device will go into the waiting state after
		// being discoverable for 30.72 second, and will not being advertising again
		// until the enabler is set back to TRUE
		uint16	gapRole_AdvertOffTime = 0;
		uint8	enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
		uint16	desired_min_interval  = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
		uint16	desired_max_interval  = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
		uint16	desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
		uint16	desired_conn_timeout  = DEFAULT_DESIRED_CONN_TIMEOUT;

		// Set the GAP Role Parameters
		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,  sizeof(uint8), &current_adv_status);
		GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(uint16), &gapRole_AdvertOffTime);

		GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
		GAPRole_SetParameter(GAPROLE_ADVERT_DATA,   sizeof(advertData), advertData);

		GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(uint8),  &enable_update_request);
		GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL,   sizeof(uint16), &desired_min_interval);
		GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL,   sizeof(uint16), &desired_max_interval);
		GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY,       sizeof(uint16), &desired_slave_latency);
		GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER,  sizeof(uint16), &desired_conn_timeout);
	}

	// Set the GAP Characteristics
	GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof(attDeviceName), attDeviceName);

	// Set advertising interval
	{
		uint16	advInt = DEFAULT_ADVERTISING_INTERVAL;
		uint16	advTO  = DEFAULT_ADVERT_MAX_TIMEOUT;

		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);

		GAP_SetParamValue(TGAP_LIM_ADV_TIMEOUT,      advTO);
	}

	// Setup the GAP Bond Manager
#if !defined(GAPBONDMGR_NO_SUPPORT)
	{
		uint32	passkey  = 0;	// passkey "000000"
		uint8	pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
		uint8	mitm     = TRUE;
		uint8	ioCap    = GAPBOND_IO_CAP_DISPLAY_ONLY;
		uint8	bonding  = TRUE;

		GAPBondMgr_SetParameter(GAPBOND_DEFAULT_PASSCODE, sizeof(uint32), &passkey);
		GAPBondMgr_SetParameter(GAPBOND_PAIRING_MODE,     sizeof(uint8),  &pairMode);
		GAPBondMgr_SetParameter(GAPBOND_MITM_PROTECTION,  sizeof(uint8),  &mitm);
		GAPBondMgr_SetParameter(GAPBOND_IO_CAPABILITIES,  sizeof(uint8),  &ioCap);
		GAPBondMgr_SetParameter(GAPBOND_BONDING_ENABLED,  sizeof(uint8),  &bonding);
	}
#endif

	// Add services
	GGS_AddService(GATT_ALL_SERVICES);		// GAP
	GATTServApp_AddService(GATT_ALL_SERVICES);	// GATT attributes
	DevInfo_AddService();				// Device Information Service
	Batt_AddService();				// battery profile
	UartProfile_AddService();			// UART profile

#if defined FEATURE_OAD
	OADTarget_AddService();				// OAD Profile
#endif

	// Setup the Sensor Profile Characteristic Values


	// Register for all key events - This app will handle all key events
	RegisterForKeys(wCube_TaskID);

	// makes sure LEDs are off
	HalLedSet((HAL_LED_ALL), HAL_LED_MODE_OFF);

	// Initialise UART
	uart_init();
	gplink_init();

	// Initialise sensor drivers
	plantmr_reset();

	// Register callbacks with profile
	uartServ1_RegisterAppCBs(uartServ1CBs);

        
	// Enable clock divide on halt
	// This reduces active current while radio is active and CC254x MCU
	// is halted
	HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT);

	// Setup a delayed profile startup
	osal_set_event(wCube_TaskID, EVT_START_DEVICE);
}


/**
 * @fn      wCube_ProcessEvent
 *
 * @brief   Simple BLE Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
typedef union {
	uint32	time32;
	uint16	time16[2];
	uint8	time8[4];
} osalTime_t;

typedef struct {
	void		*next;
	osalTime_t	timeout;
	uint16		event_flag;
	uint8		task_id;
	uint32		reloadTimeout;
} osalTimerRec_t;

extern osalTimerRec_t	*timerHead;



uint16 wCube_ProcessEvent(uint8 task_id, uint16 events)
{
	VOID	task_id;	// OSAL required parameter that isn't used in this function


	///////////////////////////////////////////////////////////////////////
	// system event handle                                               //
	///////////////////////////////////////////////////////////////////////
	if (events & SYS_EVENT_MSG) {
		uint8	*msg;

		if ((msg = osal_msg_receive(wCube_TaskID)) != NULL) {
			wCube_ProcessOSALMsg((osal_event_hdr_t *) msg);

			// release the OSAL message
			osal_msg_deallocate(msg);
		}

		// return unprocessed events
		return (events ^ SYS_EVENT_MSG);
	}


	///////////////////////////////////////////////////////////////////////
	// start device event                                                //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_START_DEVICE) {
		// start the device
		GAPRole_StartDevice(&wCube_PeripheralCBs);

		// start bond manager
#if !defined(GAPBONDMGR_NO_SUPPORT)
		GAPBondMgr_Register(&wCube_BondMgrCBs);
#endif
		// start peripheral device
		osal_pwrmgr_device(PWRMGR_ALWAYS_ON);

		osal_start_reload_timer(wCube_TaskID, EVT_RTC, PERIOD_RTC);

		fmsg(("\033[40;33m\n[version]: 1.0 (104s)\033[0m"));
		fmsg(("\033[40;32m\n[power on]\033[0m\n"));
		return (events ^ EVT_START_DEVICE);
	}


	///////////////////////////////////////////////////////////////////////
	// key long press handle                                             //
	///////////////////////////////////////////////////////////////////////





	///////////////////////////////////////////////////////////////////////
	// RTC handle                                                        //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_RTC) {
		// performed once per second

		dmsg(("."));
		Batt_SetLevel(batt_get_level());

		{
			UTCTimeStruct	t;
			UTCTime		curr;

			osal_ConvertUTCTime(&t, osal_getClock());
			t.seconds = 0;		// 0-59
//			t.minutes = 0;		// 0-59
//			t.hour    = 0;		// 0-23
			t.day     = 0;		// 0-30
			t.month   = 0;		// 0-11
			t.year    = 2000;	// 2000+
			curr	  = osal_ConvertUTCSecs(&t);
			plantmr_check(curr);
		}
		return (events ^ EVT_RTC);
	}


	///////////////////////////////////////////////////////////////////////
	// gplink RX handle                                                  //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_GPLINKRX) {
		static unsigned char	ofs, len, loop;
		static unsigned long	msec;

		switch (gplink_handle) { 
		case 0:	// reset & init
			gplink_rst_parse();
			gplink_handle = 1;
			return (events);

		case 1:	// get a packet to rxpkt
			switch (gplink_recv_pkt(rxpkt)) {
			case 0:	// length not enough
				return (events ^ EVT_GPLINKRX);

			case 1:	// parsing packet
				return (events);

			case 2:	// finish
				if (!chk_p0_2()) {
					uartServ2_PktParsing((uartpkt_t *) &rxpkt);	// make pseudo packet
				}
				gplink_handle = 2;
				return (events);
			}

		case 2:
			ofs    = 0;
			len    = rxpkt[1];
			loop   = len / 20;
			loop  += (len % 20) ? 1 : 0;
			gplink_handle = 3;
			return (events);

		case 3:	//  send notification to app (ble->app)
			if (len < 20) {
				uartServ2_SetParameter(UARTSERV2_CHAR, len, &rxpkt[ofs]);
			} else {
				uartServ2_SetParameter(UARTSERV2_CHAR,  20, &rxpkt[ofs]);
				len -= 20;
				ofs += 20;
			}
			if (--loop) {
				gplink_handle = 4;	// delay
				return (events);
			}
			gplink_handle = 0;		// send finish
			return (events ^ EVT_GPLINKRX);

		case 4:	// get system clock
			msec = osal_GetSystemClock();
			gplink_handle = 5;
			return (events);

		case 5:	// delay 20ms*??
			if ((osal_GetSystemClock() - msec) >= 20*4) {
				gplink_handle = 3;
			}
			return (events);
		}
	}


	///////////////////////////////////////////////////////////////////////
	// discard unknown events                                            //
	///////////////////////////////////////////////////////////////////////

	return 0;
}


void wCube_SetRxEvent(void)
{
	osal_set_event(wCube_TaskID, EVT_GPLINKRX);
}

