/*
 ******************************************************************************
 * steps.h -
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

#ifndef __STEPS_H__
#define __STEPS_H__

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long	STEPS;

/*
 ******************************************************************************
 * Functions
 ******************************************************************************
 */
extern unsigned long	algo_step(unsigned short *buf);



#ifdef __cplusplus
};
#endif
#endif /* __STEPS_H__ */