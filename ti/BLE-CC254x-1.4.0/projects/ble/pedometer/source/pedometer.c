/*
 ******************************************************************************
 * pedometer.h -
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

#include "pedometer.h"


/*
 *****************************************************************************
 *
 * macros
 *
 *****************************************************************************
 */
#define DBG_MSG			0
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
#define	X_CHANNEL			0
#define	Y_CHANNEL			1
#define	Z_CHANNEL			2

#define	TIMEWINDOW_MIN			10	// time window minimum 0.2s
#define TIMEWINDOW_MAX			100	// time window maximum 2s

#define REGULATION			4		// find stable rule steps
#define INVALID				2		// lose stable rule steps


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

static unsigned short	_vpp[3]       = {0, 0, 0};		// peak-to-peak for calculating dynamic precision
static unsigned char	_bad_flag[3]  = {0, 0, 0};		// bad flag for peak-to-peak value less than 0.5625g
static unsigned short	_threshold[3] = {0, 0, 0};		// dynamic threshold
static unsigned short	_precision[3] = {24, 24, 24};		// dynamic precision


static unsigned short	_sample_old[3] = {0, 0, 0};		// old fixed value
static unsigned short	_sample_new[3] = {0, 0, 0};		// new fixed value

unsigned long		STEPS = 0;				// new steps
unsigned long		steps_normal  = 0;
unsigned long		steps_workout = 0;

static unsigned char	samples = 0;				// record the sample times


static unsigned char	interval     = 0;			// Record the interval of a Step
static unsigned char	TempSteps    = 0;			// Record the Temp Steps
static unsigned char	InvalidSteps = 0;			// Record the Invalid Steps
static unsigned char	ReReg        = 2;			// Flag for Rule
								// 2 - New Begin
								// 1 - Have Begun, But Do not Find the Rule
								// 0 - Have Found the Rule

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
	
	twmsg(("\033[1;36m(%s) ", __FUNCTION__));
	switch (n) {
	case 0:	twmsg(("x: "));	break;
	case 1:	twmsg(("y: "));	break;
	case 2:	twmsg(("z: "));	break;
	}
	twmsg(("%02d - ", ReReg));

	if (ReReg == 2)	{
		// if it is the first step, add TempStep directly
		twmsg(("a"));
		TempSteps++;
		interval     = 0;
		InvalidSteps = 0;
		ReReg        = 1;

	} else {
		// if it is not the first step, process as below
		twmsg(("b"));
		if ((interval >= TIMEWINDOW_MIN) && (interval <= TIMEWINDOW_MAX)) {
			// if the step interval in the time window
			twmsg(("1"));
			InvalidSteps = 0;
			if (ReReg == 1) {
				// if still not find the rule
				twmsg(("A"));
				TempSteps++;				// make TempSteps add one
				if (TempSteps >= REGULATION) {
					// if TempSteps reach the regulation number
					STEPS     = STEPS + TempSteps;	// Update STEPS
//					STEPS     = STEPS + 1;		// Update STEPS
					TempSteps = 0;
					ReReg     = 0;			// Have found the rule
				}
				interval = 0;

			} else if (ReReg == 0) {
				// if have found the rule, Updata STEPS directly
				twmsg(("B"));
				STEPS++;
				TempSteps = 0;
				interval  = 0;
			}

		} else if (interval < TIMEWINDOW_MIN) {
			// if time interval less than the time window under threshold
			twmsg(("2"));
			if (ReReg == 0) {
				twmsg(("C"));
				// if have found the rule
				if (InvalidSteps < 255) {
					InvalidSteps++;			// make InvalidSteps add one
				}

				if (InvalidSteps >= INVALID) {
					// if InvalidSteps reach the INVALID number, search the rule again
					twmsg(("E"));
					TempSteps    = 1;
					interval     = 0;
					InvalidSteps = 0;
					ReReg        = 1;

				} else {
					// otherwise, just discard this step
					twmsg(("F"));
					interval = 0;
				}

			} else if (ReReg == 1) {
				twmsg(("D"));
				// if have not found the rule, the process looking for rule before is invalid, then search the rule again
				TempSteps    = 1;
//				interval     = 0;
				InvalidSteps = 0;
				ReReg        = 1;
			}

		} else if (interval > TIMEWINDOW_MAX) {
			// if the interval more than upper threshold, the steps is interrupted, then searh the rule again
			twmsg(("3"));
			TempSteps    = 1;
			interval     = 0;
			InvalidSteps = 0;
			ReReg        = 1;
		}
	}

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
 *
 * true binary code
 *
 */
void pedometer(unsigned short *buf)
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
	/*for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
		switch (i) {
		case X_CHANNEL:	dmsg(("x: "));	break;
		case Y_CHANNEL:	dmsg(("y: "));	break;
		case Z_CHANNEL:	dmsg(("z: "));	break;
		}
		dmsg(("max=%05d, min=%05d\n", _max[i], _min[i]));
	}*/

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

			if (_vpp[i] >= 1600) {					// 1600 --- 6.25g	(experiment value, decided by customer)
				_precision[i] = _vpp[i] / 320;			// 320  --- 1.25g	(experiment value, decided by customer)
//			} else if ((_vpp[i] >= 496) && (_vpp[i] < 1600)) {	// 496  --- 1.9375g	(experiment value, decided by customer)
			} else if ((_vpp[i] >= 72) && (_vpp[i] < 1600)) {	// 72   --- 0.288g
				_precision[i] = 48;				// 48   --- 0.1875g	(experiment value, decided by customer)
//			} else if ((_vpp[i] >= 144) && (_vpp[i] < 496))	{	// 144  --- 0.5625g	(experiment value, decided by customer)
			} else if ((_vpp[i] >= 36) && (_vpp[i] < 72))	{	// 36   --- 0.144g
				_precision[i] = 32;				// 32   --- 0.125g	(experiment value, decided by customer)
			} else {
				_precision[i] = 16;				// 16   --- 0.064g
				_bad_flag[i]  = 1;
			}
		}
		for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
			switch (i) {
			case X_CHANNEL:	dmsg(("x: "));	break;
			case Y_CHANNEL:	dmsg(("y: "));	break;
			case Z_CHANNEL:	dmsg(("z: "));	break;
			}
			dmsg(("vpp=%05d, dt=%05d, dp=%05d, badfg=%02d\n", _vpp[i], _threshold[i], _precision[i], _bad_flag[i]));
		}
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
	/*for (i=X_CHANNEL; i<=Z_CHANNEL; i++) {
		switch (i) {
		case X_CHANNEL:	dmsg(("x: "));	break;
		case Y_CHANNEL:	dmsg(("y: "));	break;
		case Z_CHANNEL:	dmsg(("z: "));	break;
		}
		dmsg(("sample old=%05d, new=%05d\n", _sample_old[i], _sample_new[i]));
	}*/

	/* judgement of dynamic threshold */
	if (interval < 0xFF) {
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

	if (steps_normal != STEPS) {
		twmsg(("\033[1;33mstep=%0lu\n\033[0m", STEPS));
	}
	steps_normal = STEPS;
}

