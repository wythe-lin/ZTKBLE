/**************************************************************************************************
  Filename:       sensorTag.h
  Revised:        $Date: 2013-03-25 07:58:08 -0700 (Mon, 25 Mar 2013) $
  Revision:       $Revision: 33575 $

  Description:    This file contains the Sensor Tag sample application
                  definitions and prototypes.

  Copyright 2012-2013 Texas Instruments Incorporated. All rights reserved.

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

#ifndef SENSORTAG_H
#define SENSORTAG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "OSAL_Clock.h"


/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Sensor Tag Task Events
#define EVT_START_DEVICE			0x0001
#define EVT_SYSRST				0x0002
#define EVT_MODE				0x0004
#define EVT_SLEEP				0x0008
#define EVT_RTC					0x0010

#define EVT_GSENSOR				0x0100
#define EVT_DISP				0x0200
#define EVT_PWMGR				0x0400


#define MODE_NORMAL				0x00
#define MODE_WORKOUT				0x10
#define MODE_SLEEP				0x20

#define MODE_TIME				0x00
#define MODE_STEP				0x01
#define MODE_CALORIE				0x02
#define MODE_DISTANCE				0x03
#define MODE_DBG				0x04


struct pi_alarm {
	unsigned char	enable;
	unsigned char	hour;
	unsigned char	minute;
	unsigned char	clock_reminding;
	unsigned char	repeat_week;
};

struct pi_alert {
	unsigned char	enable;
	unsigned char	type;
	unsigned char	start_hour;
	unsigned char	start_minute;
	unsigned char	end_hour;
	unsigned char	end_minute;
	unsigned char	repeat_week;
};

struct pi_others {
	unsigned char	gender;
	unsigned char	height;			// unit: cm
	unsigned char	weight;			// unit: kg
	unsigned char	stride;			// unit: cm
	unsigned char	running_stride;		// unit: cm
	unsigned char	sleep_start_hour;
	unsigned char	sleep_start_minute;
	unsigned char	wakeup_start_hour;
	unsigned char	wakeup_start_minute;
	unsigned char	goal_steps;
	unsigned char	bt_auto_close;
	unsigned char	bt_auto_close_time;	// unit: minute
};

struct personal_info {
	unsigned short		height;		// unit: cm
	unsigned short		weight;		// unit: kg
	unsigned short		stride;		// unit: cm - man = height x 0.415, woman = height x 0.413

	struct pi_alarm		alarm1;
	struct pi_alarm		alarm2;
	struct pi_alarm		alarm3;
	struct pi_alarm		alarm4;
	struct pi_alert		idle;
	struct pi_alert		activity;
	struct pi_others	others;
};

struct sport_info {
	unsigned long	steps;
	unsigned long	distance;	// step   x stride
	unsigned short	calorie;	// weight x distance x 1.036
	UTCTime		time;
	unsigned char	week;
};

typedef enum {
	PWMGR_S0 = 0,		// full run
	PWMGR_S1,		// oled screen saving
	PWMGR_S2,		// g-sensor active only
	PWMGR_S3,		// CPU halt
} pwmgr_t;


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */
/* Task Initialization for the BLE Application */
extern void	SensorTag_Init(uint8 task_id);

/* Task Event Processor for the BLE Application */
extern uint16	SensorTag_ProcessEvent(uint8 task_id, uint16 events);

/* Power on self test */
extern uint16	sensorTag_test(void);


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SENSORTAG_H */
