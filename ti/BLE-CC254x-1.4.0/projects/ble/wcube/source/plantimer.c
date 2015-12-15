/*
 ******************************************************************************
 * plantimer.c - Plan Timer
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
 *	2015.05.26	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */

/*
 ******************************************************************************
 *
 * includes
 *
 ******************************************************************************
 */
#include <OSAL.h>
#include "plantimer.h"


#define MSG_DBG			1

#if (MSG_DBG)
    #include <stdio.h>
    #define dmsg(x)		printf x
#else
    #define dmsg(x)
#endif

/*
 ******************************************************************************
 *
 * variables
 *
 ******************************************************************************
 */
#define MAX_PLAN_POOL	5

struct plan_data	plan_pool[MAX_PLAN_POOL];



/*
 ******************************************************************************
 *
 * local functions
 *
 ******************************************************************************
 */
#if (MSG_DBG)
   #define plantmr_pool_disp() \
   { \
	unsigned char	i; \
	for (i=0; i<MAX_PLAN_POOL; i++) { \
		dmsg(("plan[%02d] - en:%02d type:%02d begin:%08lu end:%08lu repeat:%02d\n", \
			i, \
			plan_pool[i].enable, \
			plan_pool[i].type, \
			plan_pool[i].begin, \
			plan_pool[i].end, \
			plan_pool[i].repeat)); \
	} \
   }
#else
   #define plantmr_pool_disp()
#endif



/*
 ******************************************************************************
 *
 * global functions
 *
 ******************************************************************************
 */
void plantmr_reset(void)
{
	osal_memset((void *) plan_pool, 0, sizeof(plan_pool));
}

void plantmr_update(unsigned char id, unsigned char en, unsigned char type, UTCTime begin, UTCTime end, unsigned char repeat)
{
	if (id < MAX_PLAN_POOL) {
		plan_pool[id].enable = en;
		plan_pool[id].type   = type;
		plan_pool[id].begin  = begin;
		plan_pool[id].end    = end;
		plan_pool[id].repeat = repeat;
		plan_pool[id].active = 0;
	}
	plantmr_pool_disp();

}

void plantmr_check(UTCTime curr)
{
	unsigned char	i;

	for (i=0; i<MAX_PLAN_POOL; i++) {
		if (!plan_pool[i].enable) {
			continue;
		}
		switch (plan_pool[i].active) {
		case 0:	// current status is stopped, check begin
			if (curr == plan_pool[i].begin) {
				plan_pool[i].active = 1;
				dmsg(("plan[%02d] - type:%02d, start up\n", i, plan_pool[i].type));
			}
			break;

		case 1:	// current status is started, check end
			if (curr == plan_pool[i].end) {
				plan_pool[i].active = 2;
				dmsg(("plan[%02d] - type:%02d, stop\n", i, plan_pool[i].type));
			}
			break;

		case 2:
			plan_pool[i].active = 0;
			break;

		default:
			dmsg(("plantmr_check - Unknown\n"));
			break;
		}
	}
}


