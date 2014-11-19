/*
 ******************************************************************************
 * defines.h - 
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
 *	2014.11.14	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#ifndef __DEFINES_H__
#define __DEFINES_H__

#ifdef __cplusplus
extern "C"
{
#endif


/*
 *****************************************************************************
 *
 * g-sensor threshold
 *
 *****************************************************************************
 */
#define GSEN_TH_HIGH				(1600)		// x 0.004g = 6.4g
//#define GSEN_TH_MEDIUM			(496)		// x 0.004g = 1.984g
//#define GSEN_TH_LOW				(144)		// x 0.004g = 0.576g
#define GSEN_TH_MEDIUM				(256)		// x 0.004g = 1.0g
#define GSEN_TH_LOW				(72)		// x 0.004g = 0.288g

#define GSEN_PRECISION_0			(16)		// x 0.004g = 0.064g
#define GSEN_PRECISION_1                        (32)		// x 0.004g = 0.125g
#define GSEN_PRECISION_2                        (48)		// x 0.004g = 0.1875g
#define GSEN_PRECISION_3                        (320)		// x 0.004g = 1.25g

//#define GSEN_TIMEWINDOW_MIN			(10)		// time window minimum 0.2s
#define GSEN_TIMEWINDOW_MIN			(12)		// time window minimum 0.3s
#define GSEN_TIMEWINDOW_MAX			(100)		// time window maximum 2s


/*
 *****************************************************************************
 *
 * battery level threshold
 *
 *****************************************************************************
 */
#define BATT_LEVEL_01				(0x1c00)	// 3.7V
#define BATT_LEVEL_02				(0x1b28)	// 3.6V
#define BATT_LEVEL_03				(0x1a77)	// 3.5V
#define BATT_LEVEL_04				(0x1944)	// 3.4V
#define BATT_LEVEL_05				(0x1898)	// 3.3V
#define BATT_LEVEL_06				(0x16e7)	// 3.1V


/*
 *****************************************************************************
 *
 * how often to perform something (milliseconds)
 *
 *****************************************************************************
 */
#define PERIOD_MODE_SWITCH			(1000*2)	// mode switch
#define PERIOD_MODE_SLEEP			(1000*4)	// into sleep mode
#define PERIOD_CALIB				(1000*6)	// g-sensor calibration
#define PERIOD_SYSRST				(1000*8)	// system reset
#define PERIOD_DISP				(1000/5)	// oled display
#define PERIOD_RTC				(1000*1)	// real time clock
#define PERIOD_CHARGE_DEBOUNCE			(1000/1)	// battery charge detect debounce

// seconds
#define TIME_PWRON				(1)		// power on
#define TIME_OLED_OFF				(5)		// oled off
#define TIME_GSEN_OFF				(5)		// g-sensor off


/*
 *****************************************************************************
 *
 * calorie calculate
 *
 *****************************************************************************
 */
#define COEF_CALORIE				(1 + 0.036*2)




#ifdef __cplusplus
}
#endif

#endif /* __DEFINES_H__ */

