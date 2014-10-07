/*
 ******************************************************************************
 * wbprofile.c -
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
#ifndef __WBPROFILE_H__
#define __WBPROFILE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*
 ******************************************************************************
 * Includes
 ******************************************************************************
 */


/*
 ******************************************************************************
 * Constants
 ******************************************************************************
 */

// Profile Parameters
#define SIMPLEPROFILE_CHAR1		0	// RW uint8 - Profile Characteristic 1 value
#define SIMPLEPROFILE_CHAR2		1	// RW uint8 - Profile Characteristic 2 value
#define SIMPLEPROFILE_CHAR3		2	// RW uint8 - Profile Characteristic 3 value
#define SIMPLEPROFILE_CHAR4		3	// RW uint8 - Profile Characteristic 4 value
#define SIMPLEPROFILE_CHAR5		4	// RW uint8 - Profile Characteristic 5 value
#define SIMPLEPROFILE_CHAR6		5	// RW uint8 - Profile Characteristic 6 value

// WB read service UUID
#define WBPROFILE_SERV_UUID		0xFFF0

// Key Pressed UUID
#define SIMPLEPROFILE_CHAR1_UUID	0xFFF1
#define SIMPLEPROFILE_CHAR2_UUID	0xFFF2
#define SIMPLEPROFILE_CHAR3_UUID	0xFFF3
#define SIMPLEPROFILE_CHAR4_UUID	0xFFF4
#define SIMPLEPROFILE_CHAR5_UUID	0xFFF5
#define SIMPLEPROFILE_CHAR6_UUID	0xFFF6

// Simple Keys Profile Services bit fields
#define SIMPLEPROFILE_SERVICE		0x00000001

// Length of Characteristic 5 in bytes
#define SIMPLEPROFILE_CHAR5_LEN		5

// Length of Characteristic 6 in bytes
#define SIMPLEPROFILE_CHAR6_LEN		3


/*
 ******************************************************************************
 * Typedefs
 ******************************************************************************
 */


/*
 ******************************************************************************
 * Macros
 ******************************************************************************
 */


/*
 ******************************************************************************
 * Profile Callbacks
 ******************************************************************************
 */



/*
 ******************************************************************************
 * API Functions
 ******************************************************************************
 */



#ifdef __cplusplus
}
#endif

#endif /* __WBPROFILE_H__ */


