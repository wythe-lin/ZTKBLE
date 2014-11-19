/**************************************************************************************************
  Filename:       sensorTag.c
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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
#include <hal_i2c.h>

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
#include "simplekeys.h"
#include "ptprofile.h"
#include "battserv.h"

// Sensor drivers
#include "sensorTag.h"
#include "hal_sensor.h"

#include "defines.h"

#include "uart.h"
#include "oled.h"
#include "fonts.h"
#include "adxl345.h"

#include "steps.h"
#include "hash.h"

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
#define MSG_STEPS		0x0010
#define MSG_BATT		0x0020

#define DEBUG_MESSAGE		\
(				\
/*	MSG_BATT	|*/	\
	MSG_STEPS	|	\
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
#if (DEBUG_MESSAGE & MSG_PWMGR)
    static unsigned char	power_msg;
    #define pwrmsg(x)		printf x
    #define pmsg(x)		\
    if (!power_msg) {		\
        power_msg = 1;		\
        pwrmsg(x);		\
    }
#else
    #define pmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_BLE)
    #define btmsg(x)		printf x
#else
    #define btmsg(x)
#endif
#if (DEBUG_MESSAGE & MSG_STEPS)
    #define stepmsg(x)		printf x
#else
    #define stepmsg(x)
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
#define DEFAULT_ADVERT_MAX_TIMEOUT		(30)

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
uint8		sensorTag_TaskID;	// Task ID for internal task/event processing


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
	'P',
	'T',
	'-',
	'5',
	'2',
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
	0x02,			// length of this data
	GAP_ADTYPE_FLAGS,
	DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	// service UUID, to notify central devices what services are included
	// in this peripheral
	0x03,				// length of this data
	GAP_ADTYPE_16BIT_MORE,		// some of the UUID's, but not all
	LO_UINT16(PTPROFILE_SERV1_UUID),
	HI_UINT16(PTPROFILE_SERV1_UUID),

	0x03,				// length of this data
	GAP_ADTYPE_16BIT_MORE,		// some of the UUID's, but not all
	LO_UINT16(PTPROFILE_SERV2_UUID),
	HI_UINT16(PTPROFILE_SERV2_UUID),

};

// GAP GATT Attributes
#if defined(HAL_IMAGE_A)
static uint8		attDeviceName[] = "PT-5200";
#elif defined(HAL_IMAGE_B)
static uint8		attDeviceName[] = "PT-5200";
#else
static uint8		attDeviceName[] = "PT-5200";
#endif

// sensor state variables
static bool		key1_press	= FALSE;

static unsigned char	opmode		= MODE_NORMAL;
static unsigned short	acc[3]		= { 0, 0, 0 };

static unsigned char	charge_icon	= 0;
static unsigned char	charge_cnt	= 0;

static pwmgr_t		pwmgr;
static unsigned char	power_saving;
static unsigned long	steps;
static unsigned char	batt_level;


static struct pi_others		pi = {
	0,
	170,					// unit: cm
	65,					// unit: kg
	(unsigned char) ((170*0.415) / 2),	// unit: cm - man = weight x 0.415, woman = weight x 0.413
	(unsigned char) (170*0.415),		//
	0,
	0,
	0,
	0,
	{ 0, 0, 0 },
	0,
	0,
};

static sport_record_t		normal  = { 0, 0, 0, 0 };
static sport_record_t		workout = { 0, 0, 0, 0 };
static sport_record_t		mark    = { 0, 0, 0, 0 };


/*
 *****************************************************************************
 *
 * private functions
 *
 *****************************************************************************
 */
/**
 * @fn      sensorTag_ClockSet
 *
 * @brief   for real time clock
 *
 * @param
 *
 * @return  none
 */
static void sensorTag_ClockSet(UTCTimeStruct *tm)
{
	osal_setClock(osal_ConvertUTCSecs(tm));
}

static void sensorTag_ClockGet(UTCTimeStruct *t, UTCTime delta)
{
	osal_ConvertUTCTime(t, osal_getClock() - delta);
}


/**
 * @fn      batt_measure
 *
 * @brief   Zeller's congruence:
 *	    W = Y + D + [Y / 4] + [C / 4] + [26 x (M + 1) / 10] ¡V 2C - 1
 *
 * @param   none
 *
 * @return
 */
static unsigned char sensorTag_CalcWeek(UTCTimeStruct *t)
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
	return batt_level;
}

static unsigned char batt_precision(void)
{
	unsigned char	precision;

	switch (batt_get_level()) {
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

void batt_isr(void)
{
	osal_start_timerEx(sensorTag_TaskID, EVT_CHARGING, PERIOD_CHARGE_DEBOUNCE);
//	BATCD_PXIFG   = ~(BATCD_BV);
}


/**
 * @fn      pwmgr_state_change
 *
 * @brief   Handles all key events for this device.
 *
 * @param   mode -
 * @param   time -
 *
 * @return  none
 */
static void pwmgr_state_change(pwmgr_t mode, unsigned char time)
{
	pwmgr        = mode;
	power_saving = time;

#if (DEBUG_MESSAGE & MSG_PWMGR)
	power_msg    = 0;
#endif
}

static unsigned char pwmgr_saving_timer(void)
{
	if (!power_saving) {
		return 1;
	}

	power_saving--;
	return 0;
}


/**
 * @fn      
 *
 * @brief   
 *
 * @param   
 * @param
 *
 * @return
 */
// step x stride
static unsigned long calc_distance(unsigned long steps, unsigned char stride)
{
	unsigned long	distance;

	distance = (steps * stride) / 100UL;	// unit: meter
//	dmsg(("distance=%0ldm\n", distance));
	return distance;
}

// weight x distance x (1.036)
static unsigned short calc_calorie(unsigned long distance, unsigned char weight)
{
	float		tmp;
	unsigned short	calorie;

	tmp     = ((float) (distance * weight) / 1000.0) * COEF_CALORIE;
	calorie = (unsigned short) tmp;
//	dmsg(("calorie=%0d, %0.2fKcal\n", calorie, tmp));
	return calorie;
}


/**
 * @fn      sensorTag_HandleKeys
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
static void sensorTag_HandleKeys(uint8 shift, uint8 keys)
{
	VOID	shift;		// Intentionally unreferenced parameter
	uint8	current_adv_status;

	if (keys & HAL_KEY_SW_1) {
		// press KEY1
		key1_press = TRUE;

		// display
		oled_exit_sleep();

		// KEY action
		osal_start_timerEx(sensorTag_TaskID, EVT_MODE,   PERIOD_MODE_SWITCH);
		osal_start_timerEx(sensorTag_TaskID, EVT_SLEEP,  PERIOD_MODE_SLEEP);
		osal_start_timerEx(sensorTag_TaskID, EVT_SYSRST, PERIOD_SYSRST);
		if (pwmgr == PWMGR_S1) {
			switch (opmode & 0xF0) {
			case MODE_NORMAL:
				opmode++;
#if defined(HAL_IMAGE_B)
				if (opmode > (MODE_NORMAL + MODE_DISTANCE)) {
#else
				if (opmode > (MODE_NORMAL + MODE_DBG)) {
#endif
					opmode = MODE_NORMAL | MODE_TIME;
				}
				break;

			case MODE_WORKOUT:
				opmode++;
#if defined(HAL_IMAGE_B)
				if (opmode > (MODE_WORKOUT + MODE_DISTANCE)) {
#else
				if (opmode > (MODE_WORKOUT + MODE_DBG)) {
#endif
					opmode = MODE_WORKOUT | MODE_TIME;
				}
				break;

			case MODE_SLEEP:
			default:
				break;
			}
		}

		// power management
		switch (pwmgr) {
		case PWMGR_S5:
			osal_start_reload_timer(sensorTag_TaskID, EVT_RTC, PERIOD_RTC);
			pwmgr_state_change(PWMGR_S6, TIME_OLED_OFF);

		case PWMGR_S6:
			return;

		case PWMGR_S4:
			adxl345_exit_sleep();

		case PWMGR_S1:
		default:
			pwmgr_state_change(PWMGR_S1, TIME_OLED_OFF);
			break;
		}

		// find the current GAP advertising status
		Batt_SetLevel(batt_precision());
		GAPRole_GetParameter(GAPROLE_ADVERT_ENABLED, &current_adv_status);
		if ((current_adv_status == FALSE) && (gapProfileState != GAPROLE_CONNECTED)) {
			current_adv_status = TRUE;
			GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &current_adv_status);
		}

	} else {
		// release KEY1
		key1_press = FALSE;

	}
}


/**
 * @fn      sensorTag_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void sensorTag_ProcessOSALMsg(osal_event_hdr_t *msg)
{
	switch (msg->event) {
	case KEY_CHANGE:
		sensorTag_HandleKeys(((keyChange_t *) msg)->state, ((keyChange_t *) msg)->keys);
		break;

	default:
		// do nothing
		break;
	}
}


/**
 * @fn      sensorTag_BattDisp
 *
 * @brief
 *
 * @param
 *
 * @return
 */
static void sensorTag_BattDisp(unsigned char level)
{
	oled_set_font(&icon16x16);
	switch (level) {
	case 7:	// battery charge
		switch (charge_icon) {
		case 0:	oled_draw_icon(2, 44,  7);	break;
		case 1:	oled_draw_icon(2, 44,  8);	break;
		case 2:	oled_draw_icon(2, 44,  9);	break;
		case 3:	oled_draw_icon(2, 44, 10);	break;
		case 4:	oled_draw_icon(2, 44, 11);	break;
		case 5:	oled_draw_icon(2, 44, 12);	break;
		}
		if (charge_cnt < 3) {
			charge_cnt++;
		} else {
			charge_cnt = 0;
			if (++charge_icon > 5) {
				charge_icon = 0;
			}
		}
		break;

	case 6:	oled_draw_icon(2, 44, 12);	break;	// battery full
	case 5:	oled_draw_icon(2, 44, 11);	break;
	case 4:	oled_draw_icon(2, 44, 10);	break;
	case 3:	oled_draw_icon(2, 44,  9);	break;
	case 2:	oled_draw_icon(2, 44,  8);	break;
	case 1:	oled_draw_icon(2, 44,  7);	break;	// battery empty
	case 0:	oled_draw_icon(2, 44, 13);	break;	// battery shutdown
	}
}

/**
 * @fn      sensorTag_HandleDisp
 *
 * @brief
 *
 * @param
 *
 * @return
 */
static void sensorTag_HandleDisp(unsigned char mode, void *p)
{
	VOID		*p;
	UTCTimeStruct	time;


	if ((mode & 0xF0) == MODE_SLEEP) {
		// display time
		sensorTag_ClockGet(&time, 0);

		oled_set_font(&num8x16);
		oled_draw_icon(1,  0, time.hour/10);
		oled_draw_icon(1,  8, time.hour%10);
		oled_draw_icon(1, 16, 10);
		oled_draw_icon(1, 24, time.minutes/10);
		oled_draw_icon(1, 32, time.minutes%10);

		// display sleep icon
		oled_set_font(&icon16x16);
		oled_draw_icon(0, 44, 6);

		// display battery
		sensorTag_BattDisp(batt_get_level());
		return;
	}

	switch (mode & 0x0F) {
	case MODE_TIME:
		if ((mode & 0xF0) == MODE_NORMAL) {
			// display time
			sensorTag_ClockGet(&time, 0);

			oled_set_font(&num8x16);
			oled_draw_icon(1,  0, time.hour/10);
			oled_draw_icon(1,  8, time.hour%10);
			oled_draw_icon(1, 16, 10);
			oled_draw_icon(1, 24, time.minutes/10);
			oled_draw_icon(1, 32, time.minutes%10);

			// display clock icon
			oled_set_font(&icon16x16);
			oled_draw_icon(0, 44, 14);

		} else {
			// display chronograph
			sensorTag_ClockGet(&time, workout.time);

			oled_set_font(&num8x16);
			oled_draw_icon(0,  0, time.hour/10);
			oled_draw_icon(0,  8, time.hour%10);
			oled_draw_icon(0, 16, 10);
			oled_draw_icon(0, 24, time.minutes/10);
			oled_draw_icon(0, 32, time.minutes%10);
			oled_draw_icon(0, 40, 10);
			oled_draw_icon(0, 48, time.seconds/10);
			oled_draw_icon(0, 56, time.seconds%10);
		}
		// display battery
		sensorTag_BattDisp(batt_get_level());
		break;

	case MODE_STEP:
		// display step icon
		oled_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_icon(0, 44, 0);
		} else {
			oled_draw_icon(0, 44, 1);
		}
		// display step
		oled_set_font(&num8x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_num(1, 0, normal.steps);
		} else {
			oled_draw_num(1, 0, normal.steps - workout.steps);
		}
		break;

	case MODE_CALORIE:
		// display calorie icon
		oled_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_icon(0, 44, 2);
		} else {
			oled_draw_icon(0, 44, 3);
		}
		// display calorie
		oled_set_font(&num8x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_num(1, 0, normal.calorie);
		} else {
			oled_draw_num(1, 0, workout.calorie);
		}
		break;

	case MODE_DISTANCE:
		// display distance icon
		oled_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_icon(0, 44, 4);
		} else {
			oled_draw_icon(0, 44, 5);
		}
		// display distance
		oled_set_font(&num8x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			oled_draw_num(1, 0, normal.distance);
		} else {
			oled_draw_num(1, 0, workout.distance);
		}
		break;

	case MODE_DBG:
		oled_set_font(&fonts7x8);
		oled_gotoxy(0, 0);
		oled_puts("x:");
		oled_puts_04x(acc[0]);

		oled_gotoxy(1, 0);
		oled_puts("y:");
		oled_puts_04x(acc[1]);

		oled_gotoxy(2, 0);
		oled_puts("z:");
		oled_puts_04x(acc[2]);

		oled_gotoxy(3, 0);
		oled_puts("s:");
		oled_puts_05u(normal.steps);
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
 * @fn		ptProfile_PktParsing
 *
 * @brief
 *
 * @param
 * @param
 * @param
 * @param
 * @param
 * @param
 *
 * @return
 */
static char SensorTag_PktParsing(pt_t *pkt)
{
	unsigned char	chksum_recv;
	unsigned char	chksum_calc;
	UTCTimeStruct	time;
	char		ret = 1;
	unsigned char	len, i;
	unsigned short	*src;
	unsigned short	distance;	// step   x stride
	unsigned short	calorie;	// weight x distance x 1.036

	chksum_calc = ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);

	switch (pkt->id) {
	case SET_PERSONAL_INFO_REQ:
		chksum_recv = pkt->req.set_personal_info.chksum;
		if (chksum_recv == chksum_calc) {
			osal_memcpy(&pi, &pkt->req.set_personal_info.others[0], sizeof(struct pi_others));
		}
		pkt->rsp.set_personal_info_ok.id     = SET_PERSONAL_INFO_OK_RSP;
		pkt->rsp.set_personal_info_ok.len    = 0x00;
		pkt->rsp.set_personal_info_ok.chksum = 0x00;
		if (chksum_recv != chksum_calc) {
			pkt->rsp.set_personal_info_ng.id = SET_PERSONAL_INFO_NG_RSP;
			btmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case GET_WEEK_SSDCG_REQ:
		chksum_recv = pkt->req.get_ssdcg_by_week.chksum;
		if (chksum_recv == chksum_calc) {
#if 1
			pkt->rsp.get_week_ssdcg_ok.id      = GET_WEEK_SSDCG_OK_RSP;
			pkt->rsp.get_week_ssdcg_ok.len     = 0xb4;

			//
			hash_set_getkey(pkt->req.get_ssdcg_by_week.days, pkt->req.get_ssdcg_by_week.hours);
			len = pkt->rsp.get_week_ssdcg_ok.len / (3*sizeof(unsigned short));
			for (i=0; i<len; i++) {
				src = (unsigned short *) hash_get();
				if ((*src & 0xC000) != 0xC000) {
					distance = calc_distance((*src & 0x3FFF), pi.stride);
					calorie  = calc_calorie(distance, pi.weight);
//					distance = 0x0202;
//					calorie  = 0x0101;
				} else {
					*src	 = 0xc000;
					calorie  = 0x0000;
					distance = 0x0000;
//					*src	 = 0xc102;
//					calorie  = 0x0304;
//					distance = 0x0506;
				}
				pkt->rsp.get_week_ssdcg_ok.info[i*6+0] = HI_UINT16(calorie);
				pkt->rsp.get_week_ssdcg_ok.info[i*6+1] = LO_UINT16(calorie);
				pkt->rsp.get_week_ssdcg_ok.info[i*6+2] = HI_UINT16(distance);
				pkt->rsp.get_week_ssdcg_ok.info[i*6+3] = LO_UINT16(distance);
				pkt->rsp.get_week_ssdcg_ok.info[i*6+4] = HI_UINT16(*src);
				pkt->rsp.get_week_ssdcg_ok.info[i*6+5] = LO_UINT16(*src);
			}
			pkt->rsp.get_week_ssdcg_ok.chksum  = ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);
#else
			pkt->rsp.get_week_ssdcg_ok.id      = GET_WEEK_SSDCG_OK_RSP;
			pkt->rsp.get_week_ssdcg_ok.len     = 0x00;
			pkt->rsp.get_week_ssdcg_ok.date[0] = 0x00;
			ptProfile_SetParameter(PTPROFILE_SERV2_CHAR, pkt->header.len+3, (void *) pkt);
#endif
		} else {
			pkt->rsp.get_week_ssdcg_ng.id     = GET_WEEK_SSDCG_NG_RSP;
			pkt->rsp.get_week_ssdcg_ng.len    = 0x00;
			pkt->rsp.get_week_ssdcg_ng.chksum = 0x00;
			btmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case CLR_DATA_REQ:
		chksum_recv = pkt->req.clear_data.chksum;
		if (chksum_recv == chksum_calc) {
			hash_rst();

			osal_memset(&normal,  0, sizeof(sport_record_t));
			osal_memset(&workout, 0, sizeof(sport_record_t));
			osal_memset(&mark,    0, sizeof(sport_record_t));

			pkt->rsp.clr_data_ok.id     = CLR_DATA_OK_RSP;
			pkt->rsp.clr_data_ok.len    = 0x00;
			pkt->rsp.clr_data_ok.chksum = 0x00;
		} else {
			pkt->rsp.clr_data_ng.id = CLR_DATA_NG_RSP;
			pkt->rsp.clr_data_ok.len    = 0x00;
			pkt->rsp.clr_data_ok.chksum = 0x00;
			btmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case SET_DEV_TIME_REQ:
		chksum_recv = pkt->req.set_dev_time.chksum;
		if (chksum_recv == chksum_calc) {
			time.seconds = pkt->req.set_dev_time.second;
			time.minutes = pkt->req.set_dev_time.minute;
			time.hour    = pkt->req.set_dev_time.hour;
			time.day     = pkt->req.set_dev_time.day;
			time.month   = pkt->req.set_dev_time.month;
			time.year    = (unsigned short) pkt->req.set_dev_time.year + 2000;

//			normal.week  = pkt->req.set_dev_time.week;
			normal.week  = sensorTag_CalcWeek(&time);
			sensorTag_ClockSet(&time);
			hash_set_putkey(normal.week, time.hour, time.minutes);

			pkt->rsp.set_dev_time_ok.id	= SET_DEV_TIME_OK_RSP;
			pkt->rsp.set_dev_time_ok.len	= 0x04;
			pkt->rsp.set_dev_time_ok.pwd[0]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[1]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[2]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[3]	= 0;
			pkt->rsp.set_dev_time_ok.chksum	= 0;
		} else {
			pkt->rsp.set_dev_time_ng.id	= SET_DEV_TIME_NG_RSP;
			pkt->rsp.set_dev_time_ng.len	= 0;
			pkt->rsp.set_dev_time_ng.chksum	= 0;
			btmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case GET_DEV_TIME_REQ:
		chksum_recv = pkt->req.get_dev_time.chksum;
		if (chksum_recv == chksum_calc) {
			sensorTag_ClockGet(&time, 0);

			pkt->rsp.get_dev_time_ok.id	= GET_DEV_TIME_OK_RSP;
			pkt->rsp.get_dev_time_ok.len	= 0x07;
			pkt->rsp.get_dev_time_ok.year	= (unsigned char) (time.year - 2000);
			pkt->rsp.get_dev_time_ok.month	= time.month;
			pkt->rsp.get_dev_time_ok.day	= time.day;
			pkt->rsp.get_dev_time_ok.hour	= time.hour;
			pkt->rsp.get_dev_time_ok.minute	= time.minutes;
			pkt->rsp.get_dev_time_ok.second	= time.seconds;
			pkt->rsp.get_dev_time_ok.week	= normal.week;
			pkt->rsp.get_dev_time_ok.chksum	= ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);
		} else {
			pkt->rsp.get_dev_time_ng.id	= GET_DEV_TIME_NG_RSP;
			pkt->rsp.get_dev_time_ng.len	= 0;
			pkt->rsp.get_dev_time_ng.chksum	= 0;
			btmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case SET_BT_NAME_REQ:
	case SET_BT_MATCH_PWD_REQ:
	case GET_DATE_SSDCG_REQ:
	case GET_DEV_DATA_REQ:
	case GET_DEV_DATE_STEPS_REQ:
	case RST_TO_DEFAULTS_REQ:
		ret = 0;
		btmsg(("IN: unused opcode (%02x)\n", pkt->id));
		break;

	default:
		ret = 0;
		btmsg(("IN: unknown opcode (%02x)\n", pkt->id));
		break;
	}

	return ret;
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
static void ptServ1ChgCB(uint8 paramID)
{
	switch (paramID) {
	case PTPROFILE_SERV1_CHAR:
		ptServ1_GetParameter(PTPROFILE_SERV1_CHAR, &ptpkt);
		if (SensorTag_PktParsing(&ptpkt)) {
			ptServ2_SetParameter(PTPROFILE_SERV2_CHAR, ptpkt.header.len+3, (void *) &ptpkt);
		}
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
static gapRolesCBs_t	sensorTag_PeripheralCBs = {
	pperipheral_StateNotification,	// Profile State Change Callbacks
	NULL				// When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
#if !defined(GAPBONDMGR_NO_SUPPORT)
static gapBondCBs_t	sensorTag_BondMgrCBs = {
	NULL,				// Passcode callback (not used by application)
	NULL				// Pairing / Bonding state Callback (not used by application)
};
#endif

// ProTrack Profile Callbacks
static ptServ1CBs_t	ptServ1CBs = {
	ptServ1ChgCB			// Charactersitic value change callback
};


/**
 * @fn      SensorTag_Init
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
void SensorTag_Init(uint8 task_id)
{
	sensorTag_TaskID = task_id;

	// Setup the GAP
	GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL);

	// Setup the GAP Peripheral Role Profile
	{
		// Device starts advertising upon initialization
		uint8	current_adv_status = FALSE;

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
	ptProfile_AddService();				// ProTrack Profile
	Batt_AddService();				// battery profile

#if defined FEATURE_OAD
	OADTarget_AddService();				// OAD Profile
#endif

	// Setup the Sensor Profile Characteristic Values


	// Register for all key events - This app will handle all key events
	RegisterForKeys(sensorTag_TaskID);

	// makes sure LEDs are off
	HalLedSet((HAL_LED_ALL), HAL_LED_MODE_OFF);

	// Initialise UART
	uart_init();
	BATCD_SEL &= ~BATCD_BV;		// for battery charge detect pin by UART switch to peripheral function

	// Initialise sensor drivers
//	kxti9_init();

	hash_rst();

	// Register callbacks with profile
	ptProfile_RegisterAppCBs(&ptServ1CBs);

	// Enable clock divide on halt
	// This reduces active current while radio is active and CC254x MCU
	// is halted
	HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT);

	// Setup a delayed profile startup
	osal_set_event(sensorTag_TaskID, EVT_START_DEVICE);
}


/**
 * @fn      SensorTag_ProcessEvent
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



uint16 SensorTag_ProcessEvent(uint8 task_id, uint16 events)
{
	VOID	task_id;	// OSAL required parameter that isn't used in this function


	///////////////////////////////////////////////////////////////////////
	// system event handle                                               //
	///////////////////////////////////////////////////////////////////////
	if (events & SYS_EVENT_MSG) {
		uint8	*msg;

		if ((msg = osal_msg_receive(sensorTag_TaskID)) != NULL) {
			sensorTag_ProcessOSALMsg((osal_event_hdr_t *) msg);

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
		GAPRole_StartDevice(&sensorTag_PeripheralCBs);

		// start bond manager
#if !defined(GAPBONDMGR_NO_SUPPORT)
		GAPBondMgr_Register(&sensorTag_BondMgrCBs);
#endif
		// start peripheral device
		oled_init();
		adxl345_softrst();
//		adxl345_self_calibration();

		steps	     = 0;
		BATCD_PXIFG  = ~(BATCD_BV);
		BATCD_IEN   |=  BATCD_IENBIT;

		osal_start_reload_timer(sensorTag_TaskID, EVT_RTC, PERIOD_RTC);
		pwmgr_state_change(PWMGR_S0, 0);

		fmsg(("\033[40;32m\n[power on]\033[0m\n"));
		return (events ^ EVT_START_DEVICE);
	}


	///////////////////////////////////////////////////////////////////////
	// key long press handle                                             //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_MODE) {
		if (key1_press) {
			oled_clr_screen();
			if ((opmode & 0xF0) == MODE_NORMAL) {
				opmode        = MODE_WORKOUT | MODE_TIME;
				workout.steps = normal.steps;
				workout.time  = osal_getClock();
				fmsg(("\033[40;32m[workout mode]\033[0m\n"));
			} else {
				opmode        = MODE_NORMAL | MODE_TIME;
				fmsg(("\033[40;32m[normal mode]\033[0m\n"));
			}

			pwmgr_state_change(pwmgr, TIME_OLED_OFF);
		}
		return (events ^ EVT_MODE);
	}

	if (events & EVT_SLEEP) {
		if (key1_press) {
			oled_clr_screen();
			opmode = MODE_SLEEP;
			fmsg(("\033[40;32m[sleep mode]\033[0m\n"));

			pwmgr_state_change(pwmgr, TIME_OLED_OFF);
		}
		return (events ^ EVT_SLEEP);
	}

	if (events & EVT_SYSRST) {
		if (key1_press) {
			fmsg(("\033[40;32m[system reset]\033[0m\n"));
//			HAL_SYSTEM_RESET();
			adxl345_self_calibration();
		}
		return (events ^ EVT_SYSRST);
	}


	///////////////////////////////////////////////////////////////////////
	// display handle                                                    //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_DISP) {
		if (pwmgr == PWMGR_S1) {
			sensorTag_HandleDisp(opmode, acc);
		} else {
			// display battery only
			sensorTag_BattDisp(batt_get_level());
		}
		if (pwmgr != PWMGR_S6)  {
			osal_start_timerEx(sensorTag_TaskID, EVT_DISP, PERIOD_DISP);
		}
		return (events ^ EVT_DISP);
	}


	///////////////////////////////////////////////////////////////////////
	// g-sensor handle                                                   //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_GSNINT1) {
		adxl345_exit_sleep();
		pwmgr_state_change(PWMGR_S3, TIME_GSEN_OFF);
		return (events ^ EVT_GSNINT1);
	}

	if (events & EVT_GSNINT2) {
		unsigned char	sampling;
		unsigned char	i;

		sampling = adxl345_chk_fifo();
		for (i=0; i<sampling; i++) {
			adxl345_read(acc);
#if (DEBUG_MESSAGE & MSG_STEPS)
			{
				unsigned long	tmp = algo_step(acc);
				if (normal.steps != tmp) {
					stepmsg(("\033[1;33mstep=%0lu\n\033[0m", tmp));
				}
				normal.steps = tmp;
			}
#else
			normal.steps = algo_step(acc);
#endif
		}

		normal.distance  = calc_distance(normal.steps, pi.stride);
		workout.distance = calc_distance((normal.steps - workout.steps), pi.stride);
		normal.calorie   = calc_calorie(normal.distance, pi.weight);
		workout.calorie  = calc_calorie(workout.distance, pi.weight);
		return (events ^ EVT_GSNINT2);
	}

	if (events & EVT_GSENSOR) {
		adxl345_exit_sleep();
		return (events ^ EVT_GSENSOR);
	}


	///////////////////////////////////////////////////////////////////////
	// RTC handle                                                        //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_RTC) {
		// performed once per second


		// record data
		if ((pwmgr != PWMGR_S5) && (pwmgr != PWMGR_S6)) {
#if defined(HAL_IMAGE_A) || defined(HAL_IMAGE_B)
			if ((osal_getClock() - mark.time) >= (12UL*60UL)) {
#else
			if ((osal_getClock() - mark.time) >= (12UL)) {
#endif
				if (!hash_is_full()) {
					unsigned short	tmp = normal.steps - mark.steps;

					switch (opmode & 0xF0) {
					case MODE_WORKOUT: tmp |= 0x8000; break;
					case MODE_SLEEP:   tmp |= 0x4000; break;
					}
					hash_put(&tmp);
				}
				mark.time  = osal_getClock();
#if defined(HAL_IMAGE_A) || defined(HAL_IMAGE_B)
				if ((mark.time % (24UL*60UL*60UL)) <= (13UL*60UL)) {
#else
				if ((mark.time % (24UL*60UL*60UL)) <= (13UL)) {
#endif
					dmsg(("reset steps...\n"));
					normal.steps  = 0;
					workout.steps = 0;
				}
				mark.steps = normal.steps;
			}
		}

		// power management
		switch (pwmgr) {
		case PWMGR_S0:
			pmsg(("\033[40;35mS0 (power on)\033[0m\n"));
			if (pwmgr_saving_timer()) {
				adxl345_enter_sleep();

				osal_pwrmgr_device(PWRMGR_BATTERY);
				pwmgr_state_change(PWMGR_S4, 0);
			}
			break;

		case PWMGR_S1:
			pmsg(("\033[40;35mS1 (rtc+gsen+ble+oled)\033[0m\n"));
			if (pwmgr_saving_timer()) {
				oled_enter_sleep();
				osal_stop_timerEx(sensorTag_TaskID, EVT_MODE);
				osal_stop_timerEx(sensorTag_TaskID, EVT_SLEEP);
				osal_stop_timerEx(sensorTag_TaskID, EVT_SYSRST);

				pwmgr_state_change(PWMGR_S3, TIME_GSEN_OFF);
			}
			break;

		case PWMGR_S2:
			pmsg(("\033[40;35mS2 (rtc+gsen+ble)\033[0m\n"));
			if (gapProfileState == GAPROLE_WAITING) {
				// enable key interrupt mode
				InitBoard(OB_READY);
				pwmgr_state_change(PWMGR_S3, TIME_GSEN_OFF);
			}
			break;

		case PWMGR_S3:
			pmsg(("\033[40;35mS3 (rtc+gsen)\033[0m\n"));
			if (steps == normal.steps) {
				if (pwmgr_saving_timer()) {
					adxl345_enter_sleep();
					pwmgr_state_change(PWMGR_S4, 0);
				}
			} else {
				steps = normal.steps;
				pwmgr_state_change(pwmgr, TIME_GSEN_OFF);
			}
			break;

		case PWMGR_S4:
			pmsg(("\033[40;35mS4 (rtc)\033[0m\n"));
			dmsg(("$"));
			break;

		default:
		case PWMGR_S5:
			pmsg(("\033[40;35mS5 (shutdown)\033[0m\n"));
			adxl345_shutdown();
			osal_stop_timerEx(sensorTag_TaskID, EVT_RTC);
			break;

		case PWMGR_S6:
			pmsg(("\033[40;35mS6 (rtc+oled)\033[0m\n"));
			if (pwmgr_saving_timer()) {
				oled_enter_sleep();

				// enable key interrupt mode
				InitBoard(OB_READY);
				pwmgr_state_change(PWMGR_S5, 0);
			}
			break;
		}

		// battery measure
		if ((!batt_measure()) && (pwmgr != PWMGR_S6)) {
			pwmgr_state_change(PWMGR_S5, 0);
		}
		return (events ^ EVT_RTC);
	}


	///////////////////////////////////////////////////////////////////////
	// battery charge detect handle                                      //
	///////////////////////////////////////////////////////////////////////
	if (events & EVT_CHARGING) {
		if (pwmgr != PWMGR_S1) {
			if (!BATCD_SBIT) {
				dmsg(("[charging]\n"));
				oled_exit_sleep();
				if ((pwmgr == PWMGR_S5) || (pwmgr == PWMGR_S6)) {
					osal_start_reload_timer(sensorTag_TaskID, EVT_RTC, PERIOD_RTC);
					pwmgr_state_change(PWMGR_S4, 0);
				}
			} else {
				dmsg(("[no charge]\n"));
				oled_enter_sleep();
			}
		}
		return (events ^ EVT_CHARGING);
	}


	///////////////////////////////////////////////////////////////////////
	// discard unknown events                                            //
	///////////////////////////////////////////////////////////////////////

	return 0;
}


