/*
 ******************************************************************************
 * plantimer.h - Plan Timer
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
#ifndef __PLANTIMER_H__
#define __PLANTIMER_H__

#include <OSAL_Clock.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 ******************************************************************************
 * includes
 ******************************************************************************
 */


/*
 ******************************************************************************
 * constants
 ******************************************************************************
 */
#define PLANTMR_DISABLE		(0)
#define PLANTMR_ENABLE		(1)

#define PLANTMR_SNAPSHOT	(0)
#define PLANTMR_VIDEO		(1)

#define PLANTMR_NO_REPEAT	(0)
#define PLANTMR_DAILY		(1)
#define PLANTMR_WEEKLY		(2)

struct plan_data {
	unsigned char	enable;
	unsigned char	type;
	UTCTime		begin;
	UTCTime		end;
	unsigned char	repeat;
	unsigned char	active;
};



/*
 ******************************************************************************
 * variables
 ******************************************************************************
 */


/*
 ******************************************************************************
 * API functions
 ******************************************************************************
 */
extern void	plantmr_reset(void);
extern void	plantmr_update(unsigned char id, unsigned char en, unsigned char type, UTCTime begin, UTCTime end, unsigned char repeat);
extern void	plantmr_check(UTCTime curr);


#ifdef __cplusplus
}
#endif

#endif /* __PLANTIMER_H__ */
