/*
 ******************************************************************************
 * uartserv.c - UART profile
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

/*
 ******************************************************************************
 *
 * includes
 *
 ******************************************************************************
 */
#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"
#include "peripheral.h"

#include "uartserv.h"


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
#define SERVAPP_NUM_ATTR_SUPPORTED	5


/*
 ******************************************************************************
 *
 * global variables
 *
 ******************************************************************************
 */
/* 
 * UART Service 1
 */
// Service UUID: 0xFFE5
CONST uint8	uartServ1UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(UARTSERV1_UUID), HI_UINT16(UARTSERV1_UUID)
};

// Characteristic UUID: 0xFFE9
CONST uint8	uartServ1CharUUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(UARTSERV1_CHAR_UUID), HI_UINT16(UARTSERV1_CHAR_UUID)
};

/*
 * UART Service 2
 */
// Service UUID: 0xFFE0
CONST uint8	uartServ2UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(UARTSERV2_UUID), HI_UINT16(UARTSERV2_UUID)
};

// Characteristic UUID: 0xFFE4
CONST uint8	uartServ2CharUUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(UARTSERV2_CHAR_UUID), HI_UINT16(UARTSERV2_CHAR_UUID)
};


/*
 ******************************************************************************
 *
 * local variables
 *
 ******************************************************************************
 */
static uartServ1CB_t		uartServ1_AppCBs = NULL;
static uartServ2CB_t		uartServ2_AppCBs = NULL;

/*
 * UART Service 1
 */
// Profile Attribute
static CONST gattAttrType_t	uartServ1 = { ATT_BT_UUID_SIZE, uartServ1UUID };

// Characteristic Properties, Value, User Description
static uint8			uartServ1CharProps        = GATT_PROP_WRITE;
uartpkt_t			uartpkt;
static uint8			uartServ1CharUserDesp[17] = "Write\0";

/*
 * UART Service 2
 */
// Profile Attribute
static CONST gattAttrType_t	uartServ2 = { ATT_BT_UUID_SIZE, uartServ2UUID };

// Characteristic Properties, Value, Configuration, User Description
static uint8			uartServ2CharProps        = GATT_PROP_NOTIFY;
static gattCharCfg_t		uartServ2CharCfg[GATT_MAX_NUM_CONN];
static uint8			uartServ2CharUserDesp[17] = "Notify\0";


/*
 * profile attributes - table
 */
static gattAttribute_t		uartServ1AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* UART Service 1 - 0xFFE5 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &uartServ1				/* pValue	*/
	},

	/* UART Service 1 Characteristic - 0xFFE9 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&uartServ1CharProps
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, uartServ1CharUUID },
		GATT_PERMIT_WRITE,
		0,
		(unsigned char *) &uartpkt
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		uartServ1CharUserDesp
	},
};

static gattAttribute_t		uartServ2AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* UART Service 2 - 0xFFE0 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &uartServ2				/* pValue	*/
	},

	/* UART Service 2 Characteristic - 0xFFE4 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&uartServ2CharProps
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, uartServ2CharUUID },
		GATT_PERMIT_READ,
		0,
		(unsigned char *) &uartpkt
	},

	// Configuration
	{
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(uint8 *) uartServ2CharCfg
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		uartServ2CharUserDesp
	},
};


/*
 ******************************************************************************
 *
 * UART service 1
 *
 ******************************************************************************
 */
/*
 * @fn      uartServ1_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t uartServ1_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset)
{
	bStatus_t	status = SUCCESS;

	dmsg(("\033[40;31m0xFFE9 (Write)\033[0m\n"));

	// If attribute permissions require authorization to write, return error
	if (gattPermitAuthorWrite(pAttr->permissions)) {
		// Insufficient authorization
		return (ATT_ERR_INSUFFICIENT_AUTHOR);
	}

	if (pAttr->type.len == ATT_BT_UUID_SIZE) {
		// 16-bit UUID
		uint16	uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

		switch (uuid) {
		case UARTSERV1_CHAR_UUID:
			osal_memcpy(pAttr->pValue, pValue, len);

			if (uartServ1_AppCBs) {
				uartServ1_AppCBs(UARTSERV1_CHAR);
			}
			break;

		default:
			// should never get here! (characteristic 2 do not have write permissions)
			dmsg(("err\n"));
			status = ATT_ERR_ATTR_NOT_FOUND;
			break;
		}
	} else {
		// 128-bit UUID
		status = ATT_ERR_INVALID_HANDLE;
	}

	return (status);
}

// UART Service 1 Callbacks
CONST gattServiceCBs_t		uartServ1CBs = {
	NULL,				// Read callback function pointer
	uartServ1_WriteAttrCB,		// Write callback function pointer
	NULL				// Authorization callback function pointer
};



/*
 ******************************************************************************
 *
 * UART service 2
 *
 ******************************************************************************
 */
/**
 * @fn      uartServ2WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t uartServ2WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset)
{
	bStatus_t	status = SUCCESS;
	uint16		uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

	dmsg(("\033[40;31m0xFFE0 (Notify)\033[0m\n"));

	switch (uuid) {
	case GATT_CLIENT_CHAR_CFG_UUID:
		status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
		if (status == SUCCESS) {
			uint16	charCfg = BUILD_UINT16(pValue[0], pValue[1]);

			if (uartServ2_AppCBs) {
				(*uartServ2_AppCBs)((charCfg == GATT_CFG_NO_OPERATION) ? UARTSERV2_NOTI_DISABLED : UARTSERV2_NOTI_ENABLED);
			}
		}
		break;

	default:
		status = ATT_ERR_ATTR_NOT_FOUND;
		break;
	}

	return (status);
}

// UART Service 2 Callbacks
CONST gattServiceCBs_t		uartServ2CBs = {
	NULL,				// Read callback function pointer
	uartServ2WriteAttrCB,		// Write callback function pointer
	NULL				// Authorization callback function pointer
};


/*
 *****************************************************************************
 *
 * local functions
 *
 *****************************************************************************
 */
/**
 * @fn          ptProfile_HandleConnStatusCB
 *
 * @brief       Simple Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void UartProfile_HandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
	// Make sure this is not loopback connection
	if (connHandle != LOOPBACK_CONNHANDLE) {
		// Reset Client Char Config if connection has dropped
		if ((changeType == LINKDB_STATUS_UPDATE_REMOVED) || ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) && (!linkDB_Up(connHandle)))) {
			GATTServApp_InitCharCfg(connHandle, uartServ2CharCfg);
		}
	}
}


/*
 *****************************************************************************
 *
 * global functions
 *
 *****************************************************************************
 */
/**
 * @fn      UartProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t UartProfile_AddService(void)
{
	uint8	status = SUCCESS;

	// Initialize Client Characteristic Configuration attributes
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, uartServ2CharCfg);

	// Register with Link DB to receive link status change callback
	linkDB_Register(UartProfile_HandleConnStatusCB);

	// Register GATT attribute list and CBs with GATT Server App
	GATTServApp_RegisterService(uartServ1AttrTbl, GATT_NUM_ATTRS(uartServ1AttrTbl), &uartServ1CBs);
	GATTServApp_RegisterService(uartServ2AttrTbl, GATT_NUM_ATTRS(uartServ2AttrTbl), &uartServ2CBs);
	return (status);
}


/**
 * @fn      uartServ1_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t uartServ1_RegisterAppCBs(uartServ1CB_t appCallbacks)
{
	if (appCallbacks) {
		uartServ1_AppCBs = appCallbacks;
		return (SUCCESS);
	} else {
		return (bleAlreadyInRequestedMode);
	}
}


/**
 * @fn      uartServ2_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t uartServ2_RegisterAppCBs(uartServ2CB_t appCallbacks)
{
	if (appCallbacks) {
		uartServ2_AppCBs = appCallbacks;
		return (SUCCESS);
	} else {
		return (bleAlreadyInRequestedMode);
	}
}


/**
 * @fn      uartServ1_GetParameter
 *
 * @brief   Get a Service 1 parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t uartServ1_GetParameter(uint8 param, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case UARTSERV1_CHAR:
		{
			uint8	i;
			uint8	*pValue = (uint8 *) value;

			dmsg(("[app->ble]:"));
			for (i=0; i<pValue[1]; i++) {
				dmsg((" %02x", pValue[i]));
			}
			dmsg(("\n"));
		}
		break;

	default:
		ret = INVALIDPARAMETER;
		break;
	}
	return (ret);
}


/**
 * @fn      uartServ2_SetParameter
 *
 * @brief   Set a Service 2 parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t uartServ2_SetParameter(uint8 param, uint8 len, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case UARTSERV2_CHAR:
		{
			attHandleValueNoti_t	noti;
			uint16			notiHandle;
			uint8			*pkt = (uint8 *) value;

			GAPRole_GetParameter(GAPROLE_CONNHANDLE, &notiHandle);
			noti.handle = uartServ2AttrTbl[2].handle;

			noti.len = len;
			osal_memcpy(&noti.value[0], pkt, noti.len);
			GATT_Notification(notiHandle, &noti, FALSE);

			//
			{
				uint8	i;

				dmsg(("[ble->app]:"));
				for (i=0; i<noti.len; i++) {
					dmsg((" %02x", noti.value[i]));
				}
				dmsg(("\n"));
			}
		}
		break;

	default:
		ret = INVALIDPARAMETER;
		break;
	}
	return (ret);
}






