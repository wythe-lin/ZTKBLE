/*
 ******************************************************************************
 * uartserv.h - UART profile
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
 *	2015.03.17	T.C. Chiu	frist edition
 *
 ******************************************************************************
 */
#ifndef __UARTSERV_H__
#define __UARTSERV_H__

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
// UART Service UUID
#define UARTSERV1_UUID			0xFFE5
#define UARTSERV2_UUID			0xFFE0

// Service Characteristic UUID
#define UARTSERV1_CHAR_UUID		0xFFE9
#define UARTSERV2_CHAR_UUID		0xFFE4

// Callback events
#define UARTSERV2_NOTI_ENABLED		1
#define UARTSERV2_NOTI_DISABLED		2

//
#define UARTSERV1_CHAR  		0xE9
#define UARTSERV2_CHAR  		0xE4

// opcode & response
typedef enum {
	SET_DATE			= 0x10,
	RECORD_START			= 0x01,
	RECORD_STOP			= 0x11,
	SNAPSHOT			= 0x02,
	READ_STATUS			= 0x03,
	POWER_MANAGE			= 0x00,
	INQUIRY_PIC			= 0x21,
	INQUIRY_BLOCK			= 0x22,
	GET_PIC				= 0x23,
	WRITE_GPIO			= 0x13,
	READ_GPIO			= 0x15,
	WRITE_PLAN			= 0x30,
	STATUS_PACKET			= 0x80,
	BLOCK_DATA_PACKET		= 0x20,
} uartpkt_cmd_t;

typedef struct {
	unsigned char	leading;
	unsigned char	len;
	unsigned char	command;
} uartpkt_leading_t;

typedef struct {
	unsigned char	chksum;
	unsigned char	ending;
} uartpkt_ending_t;


// packet format
typedef union {
	unsigned char		buf[200];	/* max size */
	uartpkt_leading_t	header;
} uartpkt_t;


// profile database
typedef struct {
	unsigned char	lsb;
	unsigned char	msb;
} endian_t;

typedef union {
	endian_t	b16;
	unsigned short	u16;
} ptdb_t;



/*
 ******************************************************************************
 * Variables
 ******************************************************************************
 */
extern uartpkt_t	uartpkt;



/*
 ******************************************************************************
 * Profile Callbacks
 ******************************************************************************
 */
typedef void 	(*uartServ1CB_t)(uint8 paramID);

typedef void	(*uartServ2CB_t)(uint8 evnet);



/*
 ******************************************************************************
 * API Functions
 ******************************************************************************
 */
extern bStatus_t	UartProfile_AddService(void);

extern bStatus_t	uartServ1_RegisterAppCBs(uartServ1CB_t appCallbacks);
extern bStatus_t	uartServ2_RegisterAppCBs(uartServ2CB_t appCallbacks);
                	
extern bStatus_t	uartServ1_GetParameter(uint8 param, void *value);
                	
extern bStatus_t	uartServ2_SetParameter(uint8 param, uint8 len, void *value);



#ifdef __cplusplus
}
#endif

#endif /* __UARTSERV_H__ */


