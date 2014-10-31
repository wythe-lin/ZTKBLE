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
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OSAL_Clock.h"

#include "OnBoard.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_keys.h"
#include "hal_i2c.h"

#include "gatt.h"
#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"

#include "peripheral.h"

#include "gapbondmgr.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

// Services
#include "devinfo.h"
#include "simplekeys.h"
#include "ptprofile.h"
#include "battserv.h"

// Sensor drivers
#include "sensorTag.h"
#include "hal_sensor.h"

#include "uart.h"
#include "vgm064032a1w01.h"
#include "fonts.h"
//#include "kxti9-1001.h"
#include "adxl345.h"

#include "pedometer.h"

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
 * constants
 *
 *****************************************************************************
 */
// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL		(160*5)

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


// battery level
#define BATT_LEVEL_00				500		// 464	// 4.0V 1858
#define BATT_LEVEL_01				455		// 3.4V 1857
#define BATT_LEVEL_02				443		// 3.2V 1737
#define BATT_LEVEL_03				428		// 3.1V
#define BATT_LEVEL_04				414		// 3.0V
#define BATT_LEVEL_05				394		// 2.9V
#define BATT_LEVEL_06				382		// 2.8V

// how often to perform something (milliseconds)
#define PERIOD_MODE_SWITCH			(1000*2)	// mode switch
#define PERIOD_MODE_SLEEP			(1000*4)	// into sleep mode
#define PERIOD_SYSRST				(1000*8)	// system reset
#define PERIOD_GSENSOR				(20)		// g-sensor
#define PERIOD_DISP				(200)		// oled display
#define PERIOD_PWMGR				(1000*10)	// power saving
#define PERIOD_RTC				(1000*1)	// real time clock


#define PERIOD_SCREEN_SAVING			(5)		// screen saving

/*
 *****************************************************************************
 *
 * global variables
 *
 *****************************************************************************
 */



/*
 *****************************************************************************
 *
 * local variables
 *
 *****************************************************************************
 */
static uint8			sensorTag_TaskID;	// Task ID for internal task/event processing
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
static uint8		attDeviceName[] = "Zealtek Pedometer A";
#elif defined(HAL_IMAGE_B)
static uint8		attDeviceName[] = "Zealtek Pedometer B";
#else
static uint8		attDeviceName[] = "PT-5200";
#endif

// sensor state variables
static bool		key1_press	= FALSE;

static unsigned short	gsen_period	= PERIOD_GSENSOR;

static unsigned char	opmode		= MODE_NORMAL;
static unsigned short	acc[3]		= { 0, 0, 0 };

static unsigned char	charge_icon	= 0;
static unsigned char	charge_cnt	= 0;

static pwmgr_t		pwmgr;
static unsigned char	power_saving	= 0;


static struct personal_info 	pi = {
	170,					// unit: cm
	65,					// unit: kg
	(unsigned char) (170*0.415),		// unit: cm - man = weight x 0.415, woman = weight x 0.413
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
};

static struct sport_info	normal = {
	0,
	0,
	0,
	0,
};

static struct sport_info	workout = {
	0,
	0,
	0,
	0,
};


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
static void sensorTag_ClockSet(unsigned char *p)
{
	UTCTimeStruct	time;

	// parse time service structure to OSAL time structure
	time.year = BUILD_UINT16(p[0], p[1]);
	if (time.year == 0) {
		time.year = 2000;
	}
	p         += 2;
	time.month = *p++;
	if (time.month > 0) {
//		time.month--;
	}
	time.day = *p++;
	if (time.day > 0) {
//		time.day--;
	}
	time.hour    = *p++;
	time.minutes = *p++;
	time.seconds = *p;

	// update OSAL time
	osal_setClock(osal_ConvertUTCSecs(&time));
}

static void sensorTag_ClockGet(UTCTimeStruct *t, UTCTime delta)
{
	osal_ConvertUTCTime(t, osal_getClock() - delta);
}


/**
 * @fn      sensorTag_BattMeasure
 *
 * @brief   Measure the battery level with the ADC
 *
 * @param   none
 *
 * @return
 */
static unsigned char sensorTag_BattMeasure(void)
{
	unsigned short	adc;
	unsigned char	level = 1;

	// configure ADC and perform a read
	HalAdcSetReference(HAL_ADC_REF_125V);
	adc = HalAdcRead(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_10);
	dmsg(("adc=%04x\n", adc));

	if (adc >= BATT_LEVEL_00) {
		level = 7;	// battery charge
	} else if ((adc >= BATT_LEVEL_01) && (adc < BATT_LEVEL_00)) {
		level = 6;	// battery full
	} else if ((adc >= BATT_LEVEL_02) && (adc < BATT_LEVEL_01)) {
		level = 5;
	} else if ((adc >= BATT_LEVEL_03) && (adc < BATT_LEVEL_02)) {
		level = 4;
	} else if ((adc >= BATT_LEVEL_04) && (adc < BATT_LEVEL_03)) {
		level = 3;
	} else if ((adc >= BATT_LEVEL_05) && (adc < BATT_LEVEL_04)) {
		level = 2;
	} else if ((adc >= BATT_LEVEL_06) && (adc < BATT_LEVEL_05)) {
		level = 1;	// battery empty
	} else {
		level = 0;	// battery shutdown
	}
	return level;
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
		osal_pwrmgr_task_state(sensorTag_TaskID, PWRMGR_HOLD);
//		osal_set_event(sensorTag_TaskID, EVT_GSENSOR);

		// find the current GAP advertising status
		GAPRole_GetParameter(GAPROLE_ADVERT_ENABLED, &current_adv_status);
		if ((current_adv_status == FALSE) && (gapProfileState != GAPROLE_CONNECTED)) {
			current_adv_status = TRUE;
			GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &current_adv_status);
		}

		// display
		vgm064032a1w01_exit_sleep();
		vgm064032a1w01_clr_screen();
		osal_set_event(sensorTag_TaskID, EVT_DISP);

		// power management
		power_saving = PERIOD_SCREEN_SAVING;

		// press KEY1
		key1_press = TRUE;
		osal_start_timerEx(sensorTag_TaskID, EVT_MODE,   PERIOD_MODE_SWITCH);
		osal_start_timerEx(sensorTag_TaskID, EVT_SLEEP,  PERIOD_MODE_SLEEP);
		osal_start_timerEx(sensorTag_TaskID, EVT_SYSRST, PERIOD_SYSRST);

		if (pwmgr == PWMGR_S0) {
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

		pwmgr = PWMGR_S0;
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
static void sensorTag_ProcessOSALMsg(osal_event_hdr_t *pMsg)
{
	switch (pMsg->event) {
	case KEY_CHANGE:
		sensorTag_HandleKeys(((keyChange_t *) pMsg)->state, ((keyChange_t *) pMsg)->keys);
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
	vgm064032a1w01_set_font(&icon16x16);
	switch (level) {
	case 7:	// battery charge
		switch (charge_icon) {
		case 0:	vgm064032a1w01_draw_icon(2, 44,  7);	break;
		case 1:	vgm064032a1w01_draw_icon(2, 44,  8);	break;
		case 2:	vgm064032a1w01_draw_icon(2, 44,  9);	break;
		case 3:	vgm064032a1w01_draw_icon(2, 44, 10);	break;
		case 4:	vgm064032a1w01_draw_icon(2, 44, 11);	break;
		case 5:	vgm064032a1w01_draw_icon(2, 44, 12);	break;
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

	case 6:	vgm064032a1w01_draw_icon(2, 44, 12);	break;	// battery full
	case 5:	vgm064032a1w01_draw_icon(2, 44, 11);	break;
	case 4:	vgm064032a1w01_draw_icon(2, 44, 10);	break;
	case 3:	vgm064032a1w01_draw_icon(2, 44,  9);	break;
	case 2:	vgm064032a1w01_draw_icon(2, 44,  8);	break;
	case 1:	vgm064032a1w01_draw_icon(2, 44,  7);	break;	// battery empty
	case 0:	vgm064032a1w01_draw_icon(2, 44, 13);	break;	// battery shutdown
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

		vgm064032a1w01_set_font(&num8x16);
		vgm064032a1w01_draw_icon(1,  0, time.hour/10);
		vgm064032a1w01_draw_icon(1,  8, time.hour%10);
		vgm064032a1w01_draw_icon(1, 16, 10);
		vgm064032a1w01_draw_icon(1, 24, time.minutes/10);
		vgm064032a1w01_draw_icon(1, 32, time.minutes%10);

		// display sleep icon
		vgm064032a1w01_set_font(&icon16x16);
		vgm064032a1w01_draw_icon(0, 44, 6);

		// display battery
		sensorTag_BattDisp(sensorTag_BattMeasure());
		return;
	}

	switch (mode & 0x0F) {
	case MODE_TIME:
		if ((mode & 0xF0) == MODE_NORMAL) {
			// display time
			sensorTag_ClockGet(&time, 0);

			vgm064032a1w01_set_font(&num8x16);
			vgm064032a1w01_draw_icon(1,  0, time.hour/10);
			vgm064032a1w01_draw_icon(1,  8, time.hour%10);
			vgm064032a1w01_draw_icon(1, 16, 10);
			vgm064032a1w01_draw_icon(1, 24, time.minutes/10);
			vgm064032a1w01_draw_icon(1, 32, time.minutes%10);

			// display clock icon
			vgm064032a1w01_set_font(&icon16x16);
			vgm064032a1w01_draw_icon(0, 44, 14);

		} else {
			// display chronograph
			sensorTag_ClockGet(&time, workout.time);

			vgm064032a1w01_set_font(&num8x16);
			vgm064032a1w01_draw_icon(0,  0, time.hour/10);
			vgm064032a1w01_draw_icon(0,  8, time.hour%10);
			vgm064032a1w01_draw_icon(0, 16, 10);
			vgm064032a1w01_draw_icon(0, 24, time.minutes/10);
			vgm064032a1w01_draw_icon(0, 32, time.minutes%10);
			vgm064032a1w01_draw_icon(0, 40, 10);
			vgm064032a1w01_draw_icon(0, 48, time.seconds/10);
			vgm064032a1w01_draw_icon(0, 56, time.seconds%10);
		}
		// display battery
		sensorTag_BattDisp(sensorTag_BattMeasure());
		break;

	case MODE_STEP:
		// display step icon
		vgm064032a1w01_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_icon(0, 44, 0);
		} else {
			vgm064032a1w01_draw_icon(0, 44, 1);
		}
		// display step
		vgm064032a1w01_set_font(&num8x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_num(1, 0, steps_normal);
		} else {
			vgm064032a1w01_draw_num(1, 0, steps_normal - steps_workout);
		}
		break;

	case MODE_CALORIE:
		// display calorie icon
		vgm064032a1w01_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_icon(0, 44, 2);
		} else {
			vgm064032a1w01_draw_icon(0, 44, 3);
		}
		// display calorie
		vgm064032a1w01_set_font(&num8x16);
#if !defined(HAL_IMAGE_A)
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_num(1, 0, normal.calorie);
		} else {
			vgm064032a1w01_draw_num(1, 0, workout.calorie);
		}
#else
		vgm064032a1w01_draw_icon(1, 0, 0);
#endif
		break;

	case MODE_DISTANCE:
		// display distance icon
		vgm064032a1w01_set_font(&icon16x16);
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_icon(0, 44, 4);
		} else {
			vgm064032a1w01_draw_icon(0, 44, 5);
		}
		// display distance
		vgm064032a1w01_set_font(&num8x16);
#if !defined(HAL_IMAGE_A)
		if ((mode & 0xF0) == MODE_NORMAL) {
			vgm064032a1w01_draw_num(1, 0, normal.distance);
		} else {
			vgm064032a1w01_draw_num(1, 0, workout.distance);
		}
#else
		vgm064032a1w01_draw_icon(1, 0, 0);
#endif
		break;

	case MODE_DBG:
		vgm064032a1w01_set_font(&fonts7x8);
		vgm064032a1w01_gotoxy(0, 0);
		vgm064032a1w01_puts("x:");
		vgm064032a1w01_puts_04x(acc[0]);

		vgm064032a1w01_gotoxy(1, 0);
		vgm064032a1w01_puts("y:");
		vgm064032a1w01_puts_04x(acc[1]);

		vgm064032a1w01_gotoxy(2, 0);
		vgm064032a1w01_puts("z:");
		vgm064032a1w01_puts_04x(acc[2]);

		vgm064032a1w01_gotoxy(3, 0);
		vgm064032a1w01_puts("s:");
		vgm064032a1w01_puts_05u(steps_normal);
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
			dmsg(("\033[1;34m{started}\033[0m - %02x:%02x:%02x:%02x:%02x:%02x\n",
				ownAddress[0], ownAddress[1], ownAddress[2], ownAddress[3], ownAddress[4], ownAddress[5]));
		}
		break;

	case GAPROLE_ADVERTISING:
//		HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
		dmsg(("\033[1;34m{advertising}\033[0m\n"));
		break;

	case GAPROLE_CONNECTED:
//		HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
		dmsg(("\033[1;34m{connected}\033[0m\n"));
		break;

	case GAPROLE_WAITING:
		dmsg(("\033[1;34m{waiting}\033[0m\n"));
		break;

	case GAPROLE_CONNECTED_ADV:
		dmsg(("\033[1;34m{connected adv.}\033[0m\n"));
		break;

	case GAPROLE_WAITING_AFTER_TIMEOUT:
		dmsg(("\033[1;34m{timeout}\033[0m\n"));
		break;

	case GAPROLE_ERROR:
		dmsg(("\033[1;34m{error}\033[0m\n"));
		break;

	default:
	case GAPROLE_INIT:
		dmsg(("\033[1;34m{init}\033[0m\n"));
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
const unsigned char	d1[] = {
	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,
	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,
	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x4e, 0x00, 0x13, 0x00, 0xfb, 0x00, 0xa0,
	0x00, 0x33, 0x00, 0xc5, 0x00, 0xf5, 0x00, 0xf8, 0x00, 0xf0,	0x00, 0xec, 0x00, 0xf6, 0x00, 0xa5, 0x00, 0xc6, 0x00, 0x68,
	0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0xf1,	0x00, 0x47, 0x00, 0x0c, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x00,
};

static char SensorTag_PktParsing(pt_t *pkt)
{
	unsigned char	chksum_recv;
	unsigned char	chksum_calc;
	UTCTimeStruct	time;
	char		ret = 1;


	chksum_calc = ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);

	switch (pkt->id) {
	case SET_PERSONAL_INFO_REQ:
		chksum_recv = pkt->req.set_personal_info.chksum;
		pkt->rsp.set_personal_info_ok.id     = SET_PERSONAL_INFO_OK_RSP;
		pkt->rsp.set_personal_info_ok.len    = 0x00;
		pkt->rsp.set_personal_info_ok.chksum = 0x00;
		if (chksum_recv != chksum_calc) {
			pkt->rsp.set_personal_info_ng.id = SET_PERSONAL_INFO_NG_RSP;
			dmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case GET_WEEK_SSDCG_REQ:
		chksum_recv = pkt->req.get_ssdcg_by_week.chksum;
		if (chksum_recv == chksum_calc) {
#if 1
//			sensorTag_ClockGet(&time, 0);

			pkt->rsp.get_week_ssdcg_ok.id      = GET_WEEK_SSDCG_OK_RSP;
			pkt->rsp.get_week_ssdcg_ok.len     = 0xb4;
//			pkt->rsp.get_week_ssdcg_ok.date[0] = (unsigned char) time.year;
//			pkt->rsp.get_week_ssdcg_ok.date[1] = time.month;
//			pkt->rsp.get_week_ssdcg_ok.date[2] = time.day;
			osal_memcpy(&pkt->rsp.get_week_ssdcg_ok.info[0], d1, 30*6);
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
			dmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case CLR_DATA_REQ:
		chksum_recv = pkt->req.clear_data.chksum;
		pkt->rsp.clr_data_ok.id     = CLR_DATA_OK_RSP;
		pkt->rsp.clr_data_ok.len    = 0x00;
		pkt->rsp.clr_data_ok.chksum = 0x00;
		if (chksum_recv != chksum_calc) {
			pkt->rsp.clr_data_ng.id = CLR_DATA_NG_RSP;
			dmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
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

			normal.week  = pkt->req.set_dev_time.week;
			osal_setClock(osal_ConvertUTCSecs(&time));

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
			dmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
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

			dmsg(("(%02x, %02x)\n", pkt->rsp.get_dev_time_ok.month, time.month));
			dmsg(("(%02x, %02x)\n", pkt->rsp.get_dev_time_ok.day, time.day));

		} else {
			pkt->rsp.get_dev_time_ng.id	= GET_DEV_TIME_NG_RSP;
			pkt->rsp.get_dev_time_ng.len	= 0;
			pkt->rsp.get_dev_time_ng.chksum	= 0;
			dmsg(("IN: chksum fail! (r:%02x, c:%02x)\n", chksum_recv, chksum_calc));
		}
		break;

	case SET_BT_NAME_REQ:
	case SET_BT_MATCH_PWD_REQ:
	case GET_DATE_SSDCG_REQ:
	case GET_DEV_DATA_REQ:
	case GET_DEV_DATE_STEPS_REQ:
	case RST_TO_DEFAULTS_REQ:
		ret = 0;
		dmsg(("IN: unused opcode (%02x)\n", pkt->id));
		break;

	default:
		ret = 0;
		dmsg(("IN: unknown opcode (%02x)\n", pkt->id));
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
static pt_t	pkt;

static void ptServ1ChgCB(uint8 paramID)
{
	switch (paramID) {
	case PTPROFILE_SERV1_CHAR:
		ptServ1_GetParameter(PTPROFILE_SERV1_CHAR, &pkt);
		if (SensorTag_PktParsing(&pkt)) {
			ptServ2_SetParameter(PTPROFILE_SERV2_CHAR, pkt.header.len+3, (void *) &pkt);
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
static gapBondCBs_t	sensorTag_BondMgrCBs = {
	NULL,				// Passcode callback (not used by application)
	NULL				// Pairing / Bonding state Callback (not used by application)
};

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

	// Initialise sensor drivers
//	kxti9_init();

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
uint16 SensorTag_ProcessEvent(uint8 task_id, uint16 events)
{
	VOID	task_id;	// OSAL required parameter that isn't used in this function
	uint8	current_adv_status;


	/////////////////////////
	// handle system event //
	/////////////////////////
	if (events & SYS_EVENT_MSG) {
		uint8	*pMsg;

		if ((pMsg = osal_msg_receive(sensorTag_TaskID)) != NULL) {
			sensorTag_ProcessOSALMsg((osal_event_hdr_t *) pMsg);

			// release the OSAL message
			osal_msg_deallocate(pMsg);
		}

		// return unprocessed events
		return (events ^ SYS_EVENT_MSG);
	}


	////////////////////////
	// start device event //
	////////////////////////
	if (events & EVT_START_DEVICE) {
		dmsg(("\033[40;35m\n\nS0 (power on)\033[0m\n"));

		// start the device
		GAPRole_StartDevice(&sensorTag_PeripheralCBs);

		// start bond manager
		GAPBondMgr_Register(&sensorTag_BondMgrCBs);

		// start peripheral device
		vgm064032a1w01_init();
		adxl345_init();
		adxl345_self_calibration();

		osal_pwrmgr_task_state(sensorTag_TaskID, PWRMGR_HOLD);
//		osal_set_event(sensorTag_TaskID, EVT_GSENSOR);
		osal_set_event(sensorTag_TaskID, EVT_RTC);

		power_saving = 1;
		pwmgr        = PWMGR_S0;
		return (events ^ EVT_START_DEVICE);
	}


	//////////////////////////////////
	// Mode Switch (long press key) //
	//////////////////////////////////
	if (events & EVT_MODE) {
		if (key1_press) {
			vgm064032a1w01_clr_screen();
			if ((opmode & 0xF0) == MODE_NORMAL) {
				opmode        = MODE_WORKOUT | MODE_TIME;
				steps_workout = steps_normal;
				workout.time  = osal_getClock();
				dmsg(("\033[40;32m[workout mode]\033[0m\n"));
			} else {
				opmode        = MODE_NORMAL | MODE_TIME;
				dmsg(("\033[40;32m[normal mode]\033[0m\n"));
			}

			// power management
			power_saving = PERIOD_SCREEN_SAVING;
		}
		return (events ^ EVT_MODE);
	}

	if (events & EVT_SLEEP) {
		if (key1_press) {
			vgm064032a1w01_clr_screen();
			opmode = MODE_SLEEP;
			dmsg(("\033[40;32m[sleep mode]\033[0m\n"));

			// power management
			power_saving = PERIOD_SCREEN_SAVING;
		}
		return (events ^ EVT_SLEEP);
	}

	if (events & EVT_SYSRST) {
		if (key1_press) {
			dmsg(("\033[40;32m[system reset]\033[0m\n"));
			HAL_SYSTEM_RESET();
		}
		return (events ^ EVT_SYSRST);
	}


	/////////////
	// Display //
	/////////////
	if (events & EVT_DISP) {
		sensorTag_HandleDisp(opmode, acc);
		osal_start_timerEx(sensorTag_TaskID, EVT_DISP, PERIOD_DISP);
		return (events ^ EVT_DISP);
	}


	////////////////
	// handle RTC //
	////////////////
	if (events & EVT_RTC) {
		// one second

		{
			UTCTimeStruct	t;
			UTCTime		s = 0x1c0f8759;

			osal_setClock(s);
			dmsg(("(1) s1:%08lx\n", s));

//			sensorTag_ClockGet(&t, 0);
			osal_ConvertUTCTime(&t, osal_getClock());
			dmsg(("(2) m:%02x, d:%02x\n", t.month, t.day));

			dmsg(("(3) s2:%08lx\n", osal_ConvertUTCSecs(&t)));
		}

		switch (pwmgr) {
		case PWMGR_S0:
			if (power_saving--) {
				if (!power_saving) {
					dmsg(("\033[40;35mS1 (screen saving)\033[0m\n"));
					vgm064032a1w01_enter_sleep();
					osal_stop_timerEx(sensorTag_TaskID, EVT_DISP);

					osal_stop_timerEx(sensorTag_TaskID, EVT_MODE);
					osal_stop_timerEx(sensorTag_TaskID, EVT_SLEEP);
					osal_stop_timerEx(sensorTag_TaskID, EVT_SYSRST);

					power_saving = 5;
					pwmgr        = PWMGR_S1;
				}
			}
			break;

		case PWMGR_S1:
			if (power_saving--) {
				if (!power_saving) {
					dmsg(("\033[40;35mS2 (g-sensor + RTC)\033[0m\n"));

					osal_pwrmgr_task_state(sensorTag_TaskID, PWRMGR_CONSERVE);
					power_saving = 1;
					pwmgr        = PWMGR_S2;
				}
			}
			break;

		case PWMGR_S2:
//			dmsg(("\033[40;35mS3 ()\033[0m\n"));
//			pwmgr = PWMGR_S3;
			dmsg(("$"));
			break;

		case PWMGR_S3:
			dmsg(("%"));
			break;

		default:
			break;
		}

		if (pwmgr != PWMGR_S3) {
			osal_start_timerEx(sensorTag_TaskID, EVT_RTC, PERIOD_RTC);
		}
		return (events ^ EVT_RTC);
	}


	/////////////////////////////
	// handle power management //
	/////////////////////////////
	if (events & EVT_PWMGR) {
		// GAP
		if (gapProfileState == GAPROLE_ADVERTISING) {
			GAPRole_GetParameter(GAPROLE_ADVERT_ENABLED, &current_adv_status);
			if (current_adv_status == TRUE) {
				current_adv_status = FALSE;
				GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &current_adv_status);
			}
		}

		//
		if (sensorTag_BattMeasure()) {
			dmsg(("S2 (g-sensor only)\n"));
			pwmgr = PWMGR_S2;
		} else {
			dmsg(("S3 (CPU halt)\n"));
			pwmgr = PWMGR_S3;
			osal_stop_timerEx(sensorTag_TaskID, EVT_GSENSOR);
		}

		osal_stop_timerEx(sensorTag_TaskID, EVT_MODE);
		osal_stop_timerEx(sensorTag_TaskID, EVT_SLEEP);
		osal_stop_timerEx(sensorTag_TaskID, EVT_SYSRST);

		osal_stop_timerEx(sensorTag_TaskID, EVT_DISP);
		osal_pwrmgr_task_state(sensorTag_TaskID, PWRMGR_CONSERVE);
		return (events ^ EVT_PWMGR);
	}


	//////////////
	// g-sensor //
	//////////////
	if (events & EVT_GSENSOR) {
		if (sensorTag_BattMeasure()) {
//			dmsg(("."));

			if (!adxl345_read(acc)) {
				pedometer(acc);

				normal.distance  =  (steps_normal * pi.stride) / 100UL;
				workout.distance = ((steps_normal - steps_workout) * pi.stride) / 100UL;
				normal.calorie   = (unsigned short) ((float) (normal.distance  / 1000UL * pi.weight) * 1.036);
				workout.calorie  = (unsigned short) ((float) (workout.distance / 1000UL * pi.weight) * 1.036);

				osal_start_timerEx(sensorTag_TaskID, EVT_GSENSOR, gsen_period);
			}
		}
		return (events ^ EVT_GSENSOR);
	}

	// discard unknown events
	return 0;
}


/**
 * @fn
 *
 * @brief
 *
 * @param
 *
 * @return
 */
void custom_enter_sleep(void)
{
	vgm064032a1w01_enter_sleep();
	adxl345_enter_sleep();

}

void custom_exit_sleep(void)
{
	vgm064032a1w01_exit_sleep();
	adxl345_exit_sleep();

}


