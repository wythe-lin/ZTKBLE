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
#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OSAL_Clock.h"

#include "OnBoard.h"
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
#include "devinfoservice-st.h"
#include "simplekeys.h"

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
// How often to perform sensor reads (milliseconds)
#define ACC_DEFAULT_PERIOD			20

// Constants for two-stage reading
#define TEMP_MEAS_DELAY				275	// Conversion time 250 ms
#define ACC_FSM_PERIOD				20

// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL		160

// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE		GAP_ADTYPE_FLAGS_LIMITED

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL	80

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL	800

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY		1

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
// Supervision Timeout > (1 + Slave Latency) * (Connection Interval)
#define DEFAULT_DESIRED_CONN_TIMEOUT		1000

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST		FALSE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL		8


#define INVALID_CONNHANDLE			0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN				15

// battery level
#define BATT_LEVEL_00				500	// 464	// 4.0V 1858
#define BATT_LEVEL_01				455	// 3.4V 1857
#define BATT_LEVEL_02				443	// 3.2V 1737
#define BATT_LEVEL_03				428	// 3.1V
#define BATT_LEVEL_04				414	// 3.0V
#define BATT_LEVEL_05				394	// 2.9V
#define BATT_LEVEL_06				382	// 2.8V


/*
 *****************************************************************************
 *
 * typedefs
 *
 *****************************************************************************
 */


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
 * external variables
 *
 *****************************************************************************
 */


/*
 *****************************************************************************
 *
 * external functions
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
	'Z',
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
};

// GAP GATT Attributes
#if defined(HAL_IMAGE_A)
static uint8		attDeviceName[] = "Zealtek Pedometer A";
#elif defined(HAL_IMAGE_B)
static uint8		attDeviceName[] = "Zealtek Pedometer B";
#else
static uint8		attDeviceName[] = "Zealtek Pedometer";
#endif


// sensor state variables
static bool		key1_press	   = FALSE;
static bool		screen_saving	   = TRUE;

static unsigned short	gsen_period	   = ACC_DEFAULT_PERIOD;

static unsigned char	current_adv_status = FALSE;
static unsigned char	opmode		   = MODE_NORMAL;
static unsigned short	acc[3]		   = { 0, 0, 0 };

static unsigned char	charge_icon	   = 0;
static unsigned char	charge_cnt	   = 0;

static UTCTime		time_workout	   = 0;

static struct personal_info 	pi = {
	170,					// unit: cm
	65,					// unit: kg
	(unsigned char) (170*0.415),		// unit: cm - man = weight x 0.415, woman = weight x 0.413
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
	uint8	SK_Keys = 0;

	if (keys & HAL_KEY_SW_1) {
		// find the current GAP advertising status
		GAPRole_GetParameter(GAPROLE_ADVERT_ENABLED, &current_adv_status);
		if (current_adv_status == FALSE) {
			current_adv_status = TRUE;
			GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &current_adv_status);
		}
		osal_start_timerEx(sensorTag_TaskID, ST_BLE_EVT, 10000);

		// press KEY1
		key1_press = TRUE;
		osal_start_timerEx(sensorTag_TaskID, ST_MODE_EVT,   2000);
		osal_start_timerEx(sensorTag_TaskID, ST_SLEEP_EVT,  4000);
		osal_start_timerEx(sensorTag_TaskID, ST_SYSRST_EVT, 8000);

		if (!screen_saving) {
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
		screen_saving = FALSE;
		osal_start_timerEx(sensorTag_TaskID, ST_SCREEN_EVT, 11000);	// about 10.0s

		vgm064032a1w01_exit_sleep();
		vgm064032a1w01_clr_screen();
		osal_set_event(sensorTag_TaskID, ST_DISP_EVT);

	} else {
		// release KEY1
		key1_press = FALSE;

	}


	if (keys & HAL_KEY_SW_2) {		// Carbon S2
		SK_Keys |= SK_KEY_LEFT;
	}

	if (keys & HAL_KEY_SW_3) {		// Carbon S3
		SK_Keys |= SK_KEY_RIGHT;
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
		sensorTag_HandleKeys(((keyChange_t *)pMsg)->state, ((keyChange_t *)pMsg)->keys);
		break;

	default:
		// do nothing
		break;
	}
}


/**
 * @fn      resetSensorSetup
 *
 * @brief   Turn off all sensors that are on
 *
 * @param   none
 *
 * @return  none
 */
static void resetSensorSetup(void)
{


}


/**
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gaprole_States_t newState)
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
		}
		break;

	case GAPROLE_ADVERTISING:
		HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
		break;

	case GAPROLE_CONNECTED:
		HalLedSet(HAL_LED_1, HAL_LED_MODE_FLASH);
		break;

	case GAPROLE_WAITING:
		// Link terminated intentionally: reset all sensors
		resetSensorSetup();
		break;

	default:
		break;
	}
	gapProfileState = newState;
}

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

static void sensorTag_ClockGet(UTCTimeStruct *t)
{
	osal_ConvertUTCTime(t, osal_getClock());
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
	adc = HalAdcRead(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_10) & 0xFFFC;

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

	normal.distance  =  (steps_normal * pi.stride) / 100UL;
	workout.distance = ((steps_normal - steps_workout) * pi.stride) / 100UL;
	normal.calorie   = (unsigned short) ((float) (normal.distance  / 1000UL * pi.weight) * 1.036);
	workout.calorie  = (unsigned short) ((float) (workout.distance / 1000UL * pi.weight) * 1.036);

	if ((mode & 0xF0) == MODE_SLEEP) {
		// display time
		osal_ConvertUTCTime(&time, osal_getClock());

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
			osal_ConvertUTCTime(&time, osal_getClock());

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
			osal_ConvertUTCTime(&time, osal_getClock() - time_workout);

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


/*
 *****************************************************************************
 *
 * profile callbacks
 *
 *****************************************************************************
 */
// GAP Role Callbacks
static gapRolesCBs_t	sensorTag_PeripheralCBs = {
	peripheralStateNotificationCB,	// Profile State Change Callbacks
	NULL				// When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
static gapBondCBs_t	sensorTag_BondMgrCBs = {
	NULL,				// Passcode callback (not used by application)
	NULL				// Pairing / Bonding state Callback (not used by application)
};


/*
 *****************************************************************************
 *
 * public functions
 *
 *****************************************************************************
 */
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
		current_adv_status = FALSE;

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

		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, advInt);
		GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, advInt);
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

	// Enable clock divide on halt
	// This reduces active current while radio is active and CC254x MCU
	// is halted
	HCI_EXT_ClkDivOnHaltCmd(HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT);

	// Setup a delayed profile startup
	osal_set_event(sensorTag_TaskID, ST_START_DEVICE_EVT);
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


	if (events & SYS_EVENT_MSG) {
		uint8	*pMsg;

		if ((pMsg = osal_msg_receive(sensorTag_TaskID)) != NULL) {
			dmsg(("SYS_EVENT_MSG\n"));
			sensorTag_ProcessOSALMsg((osal_event_hdr_t *) pMsg);

			// release the OSAL message
			osal_msg_deallocate(pMsg);
		}

		// return unprocessed events
		return (events ^ SYS_EVENT_MSG);
	}

	if (events & ST_START_DEVICE_EVT) {
		dmsg(("ST_START_DEVICE_EVT\n"));

		// Start the Device
		GAPRole_StartDevice(&sensorTag_PeripheralCBs);

		// Start Bond Manager
		GAPBondMgr_Register(&sensorTag_BondMgrCBs);

		vgm064032a1w01_init();
		vgm064032a1w01_clr_screen();

		adxl345_init();

		osal_set_event(sensorTag_TaskID, ST_GSENSOR_EVT);
		osal_pwrmgr_task_state(sensorTag_TaskID, PWRMGR_BATTERY);

		return (events ^ ST_START_DEVICE_EVT);
	}


	//////////////////////////////////////
	// handle system reset (long press) //
	//////////////////////////////////////
	if (events & ST_SYSRST_EVT) {
		if (key1_press) {
			dmsg(("ST_SYSRST_EVT\n"));
			HAL_SYSTEM_RESET();
		}
		return (events ^ ST_SYSRST_EVT);
	}


	////////////////
	// handle BLE //
	////////////////
	if (events & ST_BLE_EVT) {
		if (gapProfileState == GAPROLE_CONNECTED) {
			// disconnect
//			GAPRole_TerminateConnection();
		}
//		current_adv_status = FALSE;
//		GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &current_adv_status);
		return (events ^ ST_BLE_EVT);
	}


	///////////////////
	// Accelerometer //
	///////////////////
	if (events & ST_GSENSOR_EVT) {
//		dmsg(("ST_GSENSOR_EVT\n"));

		if (adxl345_read(acc)) {
			char	i;

			// translate the two's complement to true binary code
			for (i=0; i<3; i++) {
				if (acc[i] > 32768) {
					acc[i] = (acc[i] ^ 0xFFFF) + 1;
				}
			}
			pedometer(acc);

			osal_start_timerEx(sensorTag_TaskID, ST_GSENSOR_EVT, gsen_period);
		}
		return (events ^ ST_GSENSOR_EVT);
	}

	///////////////////
	// Screen Saving //
	///////////////////
	if (events & ST_SCREEN_EVT) {
		if (!screen_saving) {
			screen_saving = TRUE;
			vgm064032a1w01_enter_sleep();
			dmsg(("screen saving...\n"));
		}
		return (events ^ ST_SCREEN_EVT);
	}

	/////////////////
	// Mode Switch //
	/////////////////
	if (events & ST_MODE_EVT) {
		if (key1_press) {
			vgm064032a1w01_clr_screen();
			if ((opmode & 0xF0) == MODE_NORMAL) {
				opmode        = MODE_WORKOUT | MODE_TIME;
				steps_workout = steps_normal;
				time_workout  = osal_getClock();
				dmsg(("[workout mode]\n"));
			} else {
				opmode        = MODE_NORMAL | MODE_TIME;
				dmsg(("[normal mode]\n"));
			}
		}
		return (events ^ ST_MODE_EVT);
	}

	if (events & ST_SLEEP_EVT) {
		if (key1_press) {
			vgm064032a1w01_clr_screen();
			opmode = MODE_SLEEP;
			dmsg(("[sleep mode]\n"));
		}
		return (events ^ ST_SLEEP_EVT);
	}


	/////////////
	// Display //
	/////////////
	if (events & ST_DISP_EVT) {
		sensorTag_HandleDisp(opmode, acc);
		osal_start_timerEx(sensorTag_TaskID, ST_DISP_EVT, 100);
		return (events ^ ST_DISP_EVT);
	}

	// discard unknown events
	return 0;
}
