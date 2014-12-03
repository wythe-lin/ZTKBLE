/*
 ******************************************************************************
 * steps.c -
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
#include "defines.h"
#include "steps.h"


/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define DBG_MSG			1
#define DBG_TWIN		1

#if (DBG_MSG == 1)
    #include <stdio.h>
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif
#if (DBG_TWIN == 1)
    #include <stdio.h>
    #define twmsg(x)		printf x
#else
    #define twmsg(x)
#endif

// Three Axis acceleration Flag
#define	X_CHANNEL		0
#define	Y_CHANNEL		1
#define	Z_CHANNEL		2

#define REGULATION		4		// find stable rule steps
#define INVALID			2		// lose stable rule steps

#define __ACC_TWIN_MIN		GSEN_TIMEWINDOW_MIN
#define __ACC_TWIN_MAX		GSEN_TIMEWINDOW_MAX

#define __ACC_THH		GSEN_TH_HIGH
#define __ACC_THM		GSEN_TH_MEDIUM
#define __ACC_THL		GSEN_TH_LOW

#define __ACC_PCS0		GSEN_PRECISION_0
#define __ACC_PCS1		GSEN_PRECISION_1
#define __ACC_PCS2		GSEN_PRECISION_2
#define __ACC_PCS3		GSEN_PRECISION_3



/*
 *****************************************************************************
 *
 * Variable
 *
 *****************************************************************************
 */

//static unsigned short	accval[3]     = { 0, 0, 0 };		// 3-axis acceleration value
static unsigned short	*accval;				// 3-axis acceleration value
static unsigned short	_data0[3]     = { 1, 1, 1 };		// register for digital filter
static unsigned short	_data1[3]     = { 1, 1, 1 };		// register for digital filter
static unsigned short	_data2[3]     = { 0, 0, 0 };		// register for digital filter
static unsigned short	_data3[3]     = { 0, 0, 0 };		// register for digital filter
static unsigned short	_filter[3]    = { 0, 0, 0 };		// digital filter result

static unsigned char	filter_times  = 0;

static unsigned short	_max[3]       = { 0,    0,    0 };	// maximum for calculating dynamic threshold and precision
static unsigned short	_min[3]       = { 8192, 8192, 8192 };	// minimum for calculating dynamic threshold and precision

static unsigned short	_vpp[3]       = {  0,  0,  0 };		// peak-to-peak for calculating dynamic precision
static unsigned char	_bad_flag[3]  = {  0,  0,  0 };		// bad flag for peak-to-peak value less than 0.5625g
static unsigned short	_threshold[3] = {  0,  0,  0 };		// dynamic threshold
static unsigned short	_precision[3] = { 24, 24, 24 };		// dynamic precision

static unsigned short	_sample_old[3] = { 0, 0, 0 };		// old fixed value
static unsigned short	_sample_new[3] = { 0, 0, 0 };		// new fixed value

unsigned long		STEPS = 0;				// new steps

static unsigned char	samples = 0;				// record the sample times

static unsigned char	interval     = 0;			// Record the interval of a step
static unsigned char	TempSteps    = 0;			// Record the temp steps
static unsigned char	InvalidSteps = 0;			// Record the invalid steps
static unsigned char	rule         = 2;			// flag for rule
								// 2 - New begin
								// 1 - Have begin, but do not find the rule
								// 0 - Have found the rule

/*
 ******************************************************************************
 *
 * Local Functions
 *
 ******************************************************************************
 */
/* time_window -
 *	Realize the time window algorithm, only the steps in time window is valid.
 *      There should be four continuous steps at begin.
 *
 */
static void time_window(char n)
{
	twmsg(("\033[1;36m[twin] %s: ", (n==0) ? "x" : (n==1) ? "y" : "z"));
	twmsg(("(rule=%0d, interval=%03d) - ", rule, interval));

	switch (rule) {
	case 2:	// if it is the first step, add TempStep directly
		twmsg(("start"));
		TempSteps++;
		InvalidSteps = 0;
		interval     = 0;
		rule         = 1;
		break;

	case 1:
		if ((interval >= __ACC_TWIN_MIN) && (interval <= __ACC_TWIN_MAX)) {
			// if still not find the rule
			twmsg(("min < t < max"));
			TempSteps++;				// make TempSteps add one
			if (TempSteps >= REGULATION) {
				// if TempSteps reach the regulation number
				twmsg((", TempSteps >= %0d", REGULATION));
				STEPS     = STEPS + TempSteps;	// Update STEPS
//				STEPS     = STEPS + 1;		// Update STEPS
				TempSteps = 0;
				rule      = 0;			// Have found the rule
			}

		} else if (interval < __ACC_TWIN_MIN) {
			// if have not found the rule, the process looking for rule before is invalid, then search the rule again
			twmsg(("t < min"));
			TempSteps = 1;

		} else if (interval > __ACC_TWIN_MAX) {
			// if the interval more than upper threshold, the steps is interrupted, then searh the rule again
			twmsg(("t > max"));
			TempSteps = 1;
		}
		InvalidSteps = 0;
		interval     = 0;
		break;

	case 0:
		if ((interval >= __ACC_TWIN_MIN) && (interval <= __ACC_TWIN_MAX)) {
			// if have found the rule, update STEPS directly
			twmsg(("min < t < max"));
			STEPS++;
			TempSteps    = 0;
			InvalidSteps = 0;

		} else if (interval < __ACC_TWIN_MIN) {
			// if have found the rule
			twmsg(("t < min"));
			InvalidSteps++;				// make InvalidSteps add one
			if (InvalidSteps >= INVALID) {
				// if InvalidSteps reach the INVALID number, search the rule again
				twmsg((", InvalidSteps >= %0d", INVALID));
				TempSteps    = 1;
				InvalidSteps = 0;
				rule         = 1;
			}

		} else if (interval > __ACC_TWIN_MAX) {
			// if the interval more than upper threshold, the steps is interrupted, then searh the rule again
			twmsg(("t > max"));
			TempSteps    = 1;
			InvalidSteps = 0;
			rule         = 1;
		}
		interval = 0;
		break;
	}


//	if (rule == 2)	{
//		// if it is the first step, add TempStep directly
//		twmsg(("(start)"));
//		TempSteps++;
//		interval     = 0;
//		InvalidSteps = 0;
//		rule         = 1;
//
//	} else {
//		// if it is not the first step, process as below
//		if ((interval >= __ACC_TWIN_MIN) && (interval <= __ACC_TWIN_MAX)) {
//			// if the step interval in the time window
//			twmsg(("(min <= interval <= max)"));
//			InvalidSteps = 0;
//			if (rule == 1) {
//				// if still not find the rule
//				twmsg(("A"));
//				TempSteps++;				// make TempSteps add one
//				if (TempSteps >= REGULATION) {
//					// if TempSteps reach the regulation number
//					STEPS     = STEPS + TempSteps;	// Update STEPS
////					STEPS     = STEPS + 1;		// Update STEPS
//					TempSteps = 0;
//					rule      = 0;			// Have found the rule
//				}
//				interval = 0;
//
//			} else if (rule == 0) {
//				// if have found the rule, Update STEPS directly
//				twmsg(("B"));
//				STEPS++;
//				TempSteps = 0;
//				interval  = 0;
//			}
//
//		} else if (interval < __ACC_TWIN_MIN) {
//			// if time interval less than the time window under threshold
//			twmsg(("interval < min"));
//			if (rule == 0) {
//				twmsg(("C"));
//				// if have found the rule
//				if (InvalidSteps < 255) {
//					InvalidSteps++;			// make InvalidSteps add one
//				}
//
//				if (InvalidSteps >= INVALID) {
//					// if InvalidSteps reach the INVALID number, search the rule again
//					twmsg(("E"));
//					TempSteps    = 1;
//					interval     = 0;
//					InvalidSteps = 0;
//					rule         = 1;
//
//				} else {
//					// otherwise, just discard this step
//					twmsg(("F"));
//					interval = 0;
//				}
//
//			} else if (rule == 1) {
//				twmsg(("D"));
//				// if have not found the rule, the process looking for rule before is invalid, then search the rule again
//				TempSteps    = 1;
//				interval     = 0;
//				InvalidSteps = 0;
//				rule         = 1;
//			}
//
//		} else if (interval > __ACC_TWIN_MAX) {
//			// if the interval more than upper threshold, the steps is interrupted, then searh the rule again
//			twmsg(("interval > max"));
//			TempSteps    = 1;
//			interval     = 0;
//			InvalidSteps = 0;
//			rule         = 1;
//		}
//	}

	twmsg(("\n\033[0m"));
}


/*
 ******************************************************************************
 *
 * Global Functions
 *
 ******************************************************************************
 */
/*
 * step algorithm
 */
unsigned long algo_step(unsigned short *buf)
{
	unsigned char	i;


	accval = buf;

	/* digital filter */
	for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
		_data3[i] = _data2[i];
		_data2[i] = _data1[i];
		_data1[i] = _data0[i];
		_data0[i] = accval[i];

		if (filter_times < 6) {
			filter_times++;
		}
		if (filter_times > 4) {
		   	_filter[i] = _data0[i] + _data1[i] + _data2[i] + _data3[i];
		  	_filter[i] = _filter[i] >> 2;		// digital filter
		}
		if (_filter[i] > _max[i]) {
			_max[i] = _filter[i];			// find the maximum of acceleration
		}
		if (_filter[i] < _min[i]) {
			_min[i] = _filter[i];			// find the minimum of acceleration
		}
	}
	samples++;						// record the sample times
/*
	dmsg(("[x]: max=%04x min=%04x, ", _max[X_CHANNEL], _min[X_CHANNEL]));
	dmsg(("[y]: max=%04x min=%04x, ", _max[Y_CHANNEL], _min[Y_CHANNEL]));
	dmsg(("[z]: max=%04x min=%04x\n", _max[Z_CHANNEL], _min[Z_CHANNEL]));
*/
	/* calculate the dynamic threshold and precision */
	if (samples == 50) {
		samples = 0;

		for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
			_vpp[i]       = _max[i] - _min[i];
			_threshold[i] = _min[i] + (_vpp[i] >> 1);

			// make the maximum register min and minimum register max, so that they can be updated at next cycle immediately
			_max[i]      = 0;
			_min[i]      = 8192;
			_bad_flag[i] = 0;

			if (_vpp[i] >= __ACC_THH) {
				_precision[i] = _vpp[i] / __ACC_PCS3;
			} else if ((_vpp[i] >= __ACC_THM) && (_vpp[i] < __ACC_THH)) {
				_precision[i] = __ACC_PCS2;
			} else if ((_vpp[i] >= __ACC_THL) && (_vpp[i] < __ACC_THM)) {
				_precision[i] = __ACC_PCS1;
			} else {
				_precision[i] = __ACC_PCS0;
				_bad_flag[i]  = 1;
			}
		}
/*
		dmsg(("[x]: vpp=%04x p=%02x fg=%02x, ", _vpp[X_CHANNEL], _precision[X_CHANNEL], _bad_flag[X_CHANNEL]));
		dmsg(("[y]: vpp=%04x p=%02x fg=%02x, ", _vpp[Y_CHANNEL], _precision[Y_CHANNEL], _bad_flag[Y_CHANNEL]));
		dmsg(("[z]: vpp=%04x p=%02x fg=%02x\n", _vpp[Z_CHANNEL], _precision[Z_CHANNEL], _bad_flag[Z_CHANNEL]));
*/
	}

	/* Linear Shift Register */
	for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
		_sample_old[i] = _sample_new[i];

	    	if (_filter[i] >= _sample_new[i]) {
	     		if ((_filter[i] - _sample_new[i]) >= _precision[i]) {
				_sample_new[i] = _filter[i];
			}
		}
		if (_filter[i] < _sample_new[i]) {
	       		if ((_sample_new[i] - _filter[i]) >= _precision[i]) {
				_sample_new[i] = _filter[i];
			}
		}
	}
/*
	dmsg(("[x]: sample old=%05d, new=%05d\n", _sample_old[X_CHANNEL], _sample_new[X_CHANNEL]));
	dmsg(("[y]: sample old=%05d, new=%05d\n", _sample_old[Y_CHANNEL], _sample_new[Y_CHANNEL]));
	dmsg(("[z]: sample old=%05d, new=%05d\n", _sample_old[Z_CHANNEL], _sample_new[Z_CHANNEL]));
*/

	/* judgement of dynamic threshold */
	if (interval < 255) {
		interval++;
	}

	if ((_vpp[X_CHANNEL] >= _vpp[Y_CHANNEL]) && (_vpp[X_CHANNEL] >= _vpp[Z_CHANNEL])) {
		if ((_sample_old[X_CHANNEL] >= _threshold[X_CHANNEL]) && (_sample_new[X_CHANNEL] < _threshold[X_CHANNEL]) && (_bad_flag[X_CHANNEL] == 0)) {
			time_window(0);
		}

	} else if ((_vpp[Y_CHANNEL] >= _vpp[X_CHANNEL]) && (_vpp[Y_CHANNEL] >= _vpp[Z_CHANNEL])) {
		if ((_sample_old[Y_CHANNEL] >= _threshold[Y_CHANNEL]) && (_sample_new[Y_CHANNEL] < _threshold[Y_CHANNEL]) && (_bad_flag[Y_CHANNEL] == 0)) {
			time_window(1);
		}

	} else if ((_vpp[Z_CHANNEL] >= _vpp[Y_CHANNEL]) && (_vpp[Z_CHANNEL] >= _vpp[X_CHANNEL])) {
		if ((_sample_old[Z_CHANNEL] >= _threshold[Z_CHANNEL]) && (_sample_new[Z_CHANNEL] < _threshold[Z_CHANNEL]) && (_bad_flag[Z_CHANNEL] == 0)) {
			time_window(2);
		}
	}

	return STEPS;
}

