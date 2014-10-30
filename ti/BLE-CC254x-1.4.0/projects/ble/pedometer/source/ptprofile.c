/*
 ******************************************************************************
 * ptProfile.c - ProTrack profile
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


/*
 ******************************************************************************
 * INCLUDES
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

#include "ptProfile.h"


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
// ProTrack Profile Service 1 UUID: 0xFFE5
CONST uint8	ptServ1UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV1_UUID), HI_UINT16(PTPROFILE_SERV1_UUID)
};

// Characteristic 1 UUID: 0xFFE9
CONST uint8	ptServ1CharUUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV1_CHAR_UUID), HI_UINT16(PTPROFILE_SERV1_CHAR_UUID)
};

// ProTrack Profile Service 2 UUID: 0xFFE0
CONST uint8	ptServ2UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV2_UUID), HI_UINT16(PTPROFILE_SERV2_UUID)
};

// Characteristic 2 UUID: 0xFFE4
CONST uint8	ptServ2CharUUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV2_CHAR_UUID), HI_UINT16(PTPROFILE_SERV2_CHAR_UUID)
};

/*
 ******************************************************************************
 * local variables
 ******************************************************************************
 */
static ptServ1CBs_t		*ptServ1_AppCBs = NULL;

/*
 * profile attributes - variables
 */
// ProTrack Profile Service 1 attribute
static CONST gattAttrType_t	ptServ1 = { ATT_BT_UUID_SIZE, ptServ1UUID };

// ProTrack Profile Characteristic 1 Properties, Value, User Description
static uint8			ptServ1CharProps        = GATT_PROP_WRITE;
static pt_t			ptProfileChar           = { 0 };
static uint8			ptServ1CharUserDesp[17] = "Write\0";

// ProTrack Profile Service 1 attribute
static CONST gattAttrType_t	ptServ2 = { ATT_BT_UUID_SIZE, ptServ2UUID };

// ProTrack Profile Characteristic 2 Properties, Value, User Description
static uint8			ptServ2CharProps        = GATT_PROP_NOTIFY;
static uint8			ptServ2CharUserDesp[17] = "Notify\0";
static gattCharCfg_t		ptServ2CharCfg[GATT_MAX_NUM_CONN];

/*
 * profile attributes - table
 */
static gattAttribute_t		ptServ1AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* Service 1 - 0xFFE5 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &ptServ1				/* pValue	*/
	},

	/* Characteristic 1 - 0xFFE9 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&ptServ1CharProps
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, ptServ1CharUUID },
		GATT_PERMIT_WRITE,
		0,
		(unsigned char *) &ptProfileChar
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		ptServ1CharUserDesp
	},
};

static gattAttribute_t		ptServ2AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* Service 2 - 0xFFE0 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &ptServ2				/* pValue	*/
	},

	/* Characteristic 2 - 0xFFE4 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&ptServ2CharProps
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, ptServ2CharUUID },
		GATT_PERMIT_READ,	//0,
		0,
		(unsigned char *) &ptProfileChar
	},

	// Configuration
	{
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(uint8 *) ptServ2CharCfg
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		ptServ2CharUserDesp
	},
};


/*
 * @fn      ptServ1_WriteAttrCB
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
static unsigned char	rxlen = 0;
static unsigned char	rxofs = 0;
static bStatus_t ptServ1_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset)
{
	bStatus_t	status = SUCCESS;
	uint8		notifyApp = 0xFF;

	dmsg(("watt - \033[40;31m0xFFE9 (Write)\033[0m\n"));

	// If attribute permissions require authorization to write, return error
	if (gattPermitAuthorWrite(pAttr->permissions)) {
		// Insufficient authorization
		return (ATT_ERR_INSUFFICIENT_AUTHOR);
	}

	if (pAttr->type.len == ATT_BT_UUID_SIZE) {
		// 16-bit UUID
		uint16	uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

		switch (uuid) {
		case PTPROFILE_SERV1_CHAR_UUID:
			if (rxlen == 0) {
				rxlen = pValue[1] + 3;
				rxofs = 0;
			}
			osal_memcpy(pAttr->pValue+rxofs, pValue, len);

			rxofs += len;
			if (rxlen <= rxofs) {
				rxlen     = 0;
				notifyApp = PTPROFILE_SERV1_CHAR;
			}
			break;

		case GATT_CLIENT_CHAR_CFG_UUID:
			dmsg(("cc\n"));
			status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
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

	// If a charactersitic value changed then callback function to notify application of change
	if ((notifyApp != 0xFF) && ptServ1_AppCBs && ptServ1_AppCBs->pfnServ1Chg) {
		ptServ1_AppCBs->pfnServ1Chg(notifyApp);
	}
	return (status);
}

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
static void ptProfile_HandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
	// Make sure this is not loopback connection
	if (connHandle != LOOPBACK_CONNHANDLE) {
		// Reset Client Char Config if connection has dropped
		if ((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
		   ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) && (!linkDB_Up(connHandle)))) {
			GATTServApp_InitCharCfg(connHandle, ptServ2CharCfg);
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
// Service1 Callbacks
CONST gattServiceCBs_t		ptServ1CBs = {
	NULL,					// Read callback function pointer
	ptServ1_WriteAttrCB,			// Write callback function pointer
	NULL					// Authorization callback function pointer
};


/**
 * @fn      ptProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t ptProfile_AddService(void)
{
	uint8	status = SUCCESS;

	// Initialize Client Characteristic Configuration attributes
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, ptServ2CharCfg);

	// Register with Link DB to receive link status change callback
	linkDB_Register(ptProfile_HandleConnStatusCB);

	// Register GATT attribute list and CBs with GATT Server App
	GATTServApp_RegisterService(ptServ2AttrTbl, GATT_NUM_ATTRS(ptServ2AttrTbl), NULL);
	GATTServApp_RegisterService(ptServ1AttrTbl, GATT_NUM_ATTRS(ptServ1AttrTbl), &ptServ1CBs);
	return (status);
}


/**
 * @fn      ptProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t ptProfile_RegisterAppCBs(ptServ1CBs_t *appCallbacks)
{
	if (appCallbacks) {
		ptServ1_AppCBs = appCallbacks;
		return (SUCCESS);
	} else {
		return (bleAlreadyInRequestedMode);
	}
}


/**
 * @fn      ptServ2_SetParameter
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
bStatus_t ptServ2_SetParameter(uint8 param, uint8 len, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case PTPROFILE_SERV2_CHAR:
		{
			uint8	i;
			uint8	*pValue = (uint8 *) value;
			uint8	displen = (len < 16) ? len : 16;

			dmsg(("IN:"));
			for (i=0; i<displen; i++) {
				dmsg((" %02x", pValue[i]));
			}
			dmsg(("%s", (len < 16) ? "\n" : "...\n"));
		}
		{
			attHandleValueNoti_t	noti;
			uint16			notiHandle;
			uint8			maxlen, loop, ofs, i;
			uint8			*pkt = (uint8 *) value;

			GAPRole_GetParameter(GAPROLE_CONNHANDLE, &notiHandle);
			noti.handle = ptServ2AttrTbl[2].handle;

			maxlen = sizeof(noti.value);
			loop   = len / maxlen;
			loop  += (len % maxlen) ? 1 : 0;
			ofs    = 0;
			for (i=0; i<loop; i++) {
				if (len < maxlen) {
					noti.len = len;
					osal_memcpy(&noti.value[0], &pkt[ofs], noti.len);
				} else {
					noti.len = maxlen;
					osal_memcpy(&noti.value[0], &pkt[ofs], noti.len);
					len -= noti.len;
					ofs += noti.len;
				}
				GATT_Notification(notiHandle, &noti, FALSE);
			}
		}
		break;

	default:
		ret = INVALIDPARAMETER;
		break;
	}
	return (ret);
}

/**
 * @fn      ptServ1_GetParameter
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
bStatus_t ptServ1_GetParameter(uint8 param, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case PTPROFILE_SERV1_CHAR:
		osal_memcpy(value, &ptProfileChar, ptProfileChar.header.len+3);
		{
			uint8	i;
			uint8	*pValue = (uint8 *) value;

			dmsg(("OUT:"));
			for (i=0; i<pValue[1]+3; i++) {
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


/*
 * @fn		ptProfile_CalcChksum
 *
 * @brief
 *
 * @param
 * @param
 *
 * @return
 */
unsigned char ptProfile_CalcChksum(unsigned char len, unsigned char *val)
{
	unsigned char	i;
	unsigned char	chksum;

	for (i=0, chksum=0; i<len; i++) {
		chksum ^= val[i];
	}
	return chksum;
}

