/*
 ******************************************************************************
 * ptProfile.c -
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
#define SERVAPP_NUM_ATTR_SUPPORTED	10


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// ProTrack Profile Service 1 UUID: 0xFFE5
CONST uint8	ptProfileServ1UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV1_UUID), HI_UINT16(PTPROFILE_SERV1_UUID)
};

// Characteristic 1 UUID: 0xFFE9
CONST uint8	ptProfileChar1UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_CHAR1_UUID), HI_UINT16(PTPROFILE_CHAR1_UUID)
};

// ProTrack Profile Service 2 UUID: 0xFFE0
CONST uint8	ptProfileServ2UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_SERV2_UUID), HI_UINT16(PTPROFILE_SERV2_UUID)
};

// Characteristic 2 UUID: 0xFFE4
CONST uint8	ptProfileChar2UUID[ATT_BT_UUID_SIZE] = {
	LO_UINT16(PTPROFILE_CHAR2_UUID), HI_UINT16(PTPROFILE_CHAR2_UUID)
};


/*********************************************************************
 * EXTERNAL VARIABLES
 */
pt_t	pt;


/*
 ******************************************************************************
 * local variables
 ******************************************************************************
 */
static ptProfileCBs_t	*ptProfile_AppCBs = NULL;

/*
 ******************************************************************************
 * Profile Attributes - variables
 ******************************************************************************
 */
// ProTrack Profile Service 1 attribute
static CONST gattAttrType_t	ptProfileServ1 = { ATT_BT_UUID_SIZE, ptProfileServ1UUID };

// ProTrack Profile Characteristic 1 Properties, Value, User Description
static uint8		ptProfileChar1Props = GATT_PROP_WRITE;
static pt_t		ptProfileChar1 = { 0 };
static uint8		ptProfileChar1UserDesp[17] = "Write\0";

// ProTrack Profile Service 1 attribute
static CONST gattAttrType_t	ptProfileServ2 = { ATT_BT_UUID_SIZE, ptProfileServ2UUID };

// ProTrack Profile Characteristic 2 Properties, Value, User Description
static uint8		ptProfileChar2Props = GATT_PROP_NOTIFY;
static uint8		ptProfileChar2 = 0;
static uint8		ptProfileChar2UserDesp[17] = "Notify\0";


// ProTrack Profile Characteristic 2 Configuration Each client has its own
// instantiation of the Client Characteristic Configuration. Reads of the
// Client Characteristic Configuration only shows the configuration for
// that client and writes only affect the configuration of that client.
static gattCharCfg_t	ptProfileChar2Config[GATT_MAX_NUM_CONN];


/*
 * Profile Attributes - Table
 */
static gattAttribute_t	ptProfileServ1AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* Service 1 - 0xFFE5 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &ptProfileServ1			/* pValue	*/
	},

	/* Characteristic 1 - 0xFFE9 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&ptProfileChar1Props
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, ptProfileChar1UUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(unsigned char *) &ptProfileChar1
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		ptProfileChar1UserDesp
	},
};

static gattAttribute_t	ptProfileServ2AttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = {
	/* Service 2 - 0xFFE0 */
	{
		{ ATT_BT_UUID_SIZE, primaryServiceUUID },	/* type		*/
		GATT_PERMIT_READ,				/* permissions	*/
		0,						/* handle	*/
		(uint8 *) &ptProfileServ2			/* pValue	*/
	},

	/* Characteristic 2 - 0xFFE4 */
	// Declaration
	{
		{ ATT_BT_UUID_SIZE, characterUUID },
		GATT_PERMIT_READ,
		0,
		&ptProfileChar2Props
	},

	// Value
	{
		{ ATT_BT_UUID_SIZE, ptProfileChar2UUID },
		0,
		0,
		&ptProfileChar2
	},

	// Configuration
	{
		{ ATT_BT_UUID_SIZE, clientCharCfgUUID },
		GATT_PERMIT_READ | GATT_PERMIT_WRITE,
		0,
		(uint8 *) ptProfileChar2Config
	},

	// User Description
	{
		{ ATT_BT_UUID_SIZE, charUserDescUUID },
		GATT_PERMIT_READ,
		0,
		ptProfileChar2UserDesp
	},
};


/*
 * @fn		ptProfile_CalcChksum
 *
 * @brief
 *
 * @param
 * @param
 * @param
 * @param
 * @param
 * @param
 *
 * @return
 */
static unsigned char ptProfile_CalcChksum(unsigned char len, unsigned char *val)
{
	unsigned char	i;
	unsigned char	chksum;

	for (i=0, chksum=0; i<len; i++) {
		chksum ^= val[i];
	}
	return chksum;
}


/*
 * @fn          ptProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8 ptProfile_ReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 *pLen, uint16 offset, uint8 maxLen)
{
	bStatus_t	status = SUCCESS;

	dmsg(("ratt\n"));

	// If attribute permissions require authorization to read, return error
	if (gattPermitAuthorRead(pAttr->permissions)) {
		// Insufficient authorization
		return (ATT_ERR_INSUFFICIENT_AUTHOR);
	}

	// Make sure it's not a blob operation (no attributes in the profile are long)
	if (offset > 0) {
		return (ATT_ERR_ATTR_NOT_LONG);
	}

	if (pAttr->type.len == ATT_BT_UUID_SIZE) {
		// 16-bit UUID
		uint16	uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

		switch (uuid) {
		// No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
		// gattserverapp handles those reads

		// characteristics 1 and 2 have read permissions
		// characteritisc 3 does not have read permissions; therefore it is not included here
		// characteristic 4 does not have read permissions, but because it can be sent as a
		//	notification, it is included here
		case PTPROFILE_CHAR1_UUID:
			*pLen = pAttr->pValue[1];
			osal_memcpy(pValue, pAttr->pValue, pAttr->pValue[1]+3);
			break;

		case PTPROFILE_CHAR2_UUID:
			*pLen = 1;
			pValue[0] = *pAttr->pValue;
			break;

		default:
			// Should never get here! (characteristics 3 and 4 do not have read permissions)
			*pLen = 0;
			status = ATT_ERR_ATTR_NOT_FOUND;
			break;
		}
	} else {
		// 128-bit UUID
		*pLen = 0;
		status = ATT_ERR_INVALID_HANDLE;
	}
	return (status);
}

/*
 * @fn      ptProfile_WriteAttrCB
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
static bStatus_t ptProfile_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr, uint8 *pValue, uint8 len, uint16 offset)
{
	bStatus_t	status = SUCCESS;
	uint8		notifyApp = 0xFF;

	dmsg(("watt\n"));

	// If attribute permissions require authorization to write, return error
	if (gattPermitAuthorWrite(pAttr->permissions)) {
		// Insufficient authorization
		return (ATT_ERR_INSUFFICIENT_AUTHOR);
	}

	if (pAttr->type.len == ATT_BT_UUID_SIZE) {
		// 16-bit UUID
		uint16	uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

		switch (uuid) {
		case PTPROFILE_CHAR1_UUID:
			// Write the value
			if (status == SUCCESS) {
				osal_memcpy(pAttr->pValue, pValue, pValue[1]+3);
				notifyApp = SIMPLEPROFILE_CHAR1;
			}
			break;

		case PTPROFILE_CHAR2_UUID:
			dmsg(("c2\n"));
			break;

		case GATT_CLIENT_CHAR_CFG_UUID:
			dmsg(("cc\n"));
			status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
			break;

		default:
			// Should never get here! (characteristics 2 and 4 do not have write permissions)
			dmsg(("err\n"));
			status = ATT_ERR_ATTR_NOT_FOUND;
			break;
		}
	} else {
		// 128-bit UUID
		status = ATT_ERR_INVALID_HANDLE;
	}

	// If a charactersitic value changed then callback function to notify application of change
	if ((notifyApp != 0xFF) && ptProfile_AppCBs && ptProfile_AppCBs->pfnptProfileChange) {
		ptProfile_AppCBs->pfnptProfileChange(notifyApp);
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
			GATTServApp_InitCharCfg(connHandle, ptProfileChar2Config);
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
/* profile callbacks */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t		ptProfileCBs = {
	ptProfile_ReadAttrCB,			// Read callback function pointer
	ptProfile_WriteAttrCB,			// Write callback function pointer
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
bStatus_t ptProfile_AddService(uint32 services)
{
	uint8	status = SUCCESS;

	// Initialize Client Characteristic Configuration attributes
	GATTServApp_InitCharCfg(INVALID_CONNHANDLE, ptProfileChar2Config);

	// Register with Link DB to receive link status change callback
	VOID linkDB_Register(ptProfile_HandleConnStatusCB);

	// Register GATT attribute list and CBs with GATT Server App
	if (services & PTPROFILE_SERVICE2) {
		status = GATTServApp_RegisterService(ptProfileServ2AttrTbl, GATT_NUM_ATTRS(ptProfileServ2AttrTbl), &ptProfileCBs);
	}

	if (services & PTPROFILE_SERVICE1) {
		status = GATTServApp_RegisterService(ptProfileServ1AttrTbl, GATT_NUM_ATTRS(ptProfileServ1AttrTbl), &ptProfileCBs);
	}
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
bStatus_t ptProfile_RegisterAppCBs(ptProfileCBs_t *appCallbacks)
{
	if (appCallbacks) {
		ptProfile_AppCBs = appCallbacks;
		return (SUCCESS);
	} else {
		return (bleAlreadyInRequestedMode);
	}
}


/**
 * @fn      ptProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
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
bStatus_t ptProfile_SetParameter(uint8 param, uint8 len, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case SIMPLEPROFILE_CHAR1:
		{
			pt_t *p = (pt_t *) value;

			if (len == p->header.len) {
				osal_memcpy(&ptProfileChar1, value, len+3);
				{
					uint8	i;
					uint8	*pValue = (uint8 *) &ptProfileChar1;

					dmsg(("IN:"));
					for (i=0; i<len+3; i++) {
						dmsg((" %02x", pValue[i]));
					}
					dmsg(("\n"));
				}

				GATTServApp_ProcessCharCfg(NULL, (uint8 *) &ptProfileChar1, FALSE, ptProfileServ1AttrTbl, GATT_NUM_ATTRS(ptProfileServ1AttrTbl), INVALID_TASK_ID);

			} else {
				ret = bleInvalidRange;
			}
			break;
		}

	case SIMPLEPROFILE_CHAR2:
		if (len == sizeof(uint8)) {
			ptProfileChar2 = *((uint8 *) value);

			// See if Notification has been enabled
			GATTServApp_ProcessCharCfg(ptProfileChar2Config, &ptProfileChar2, FALSE, ptProfileServ2AttrTbl, GATT_NUM_ATTRS(ptProfileServ2AttrTbl), INVALID_TASK_ID);
		} else {
			ret = bleInvalidRange;
		}
//		dmsg(("s2"));
		break;

	default:
		ret = INVALIDPARAMETER;
		break;
	}
	return (ret);
}

/**
 * @fn      ptProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t ptProfile_GetParameter(uint8 param, void *value)
{
	bStatus_t	ret = SUCCESS;

	switch (param) {
	case SIMPLEPROFILE_CHAR1:
		osal_memcpy(value, &ptProfileChar1, ptProfileChar1.header.len+3);
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

	case SIMPLEPROFILE_CHAR2:
		*((uint8 *) value) = ptProfileChar2;
//		dmsg(("g2"));
		break;

	default:
		ret = INVALIDPARAMETER;
		break;
	}
	return (ret);
}



/*
 * @fn		ptProfile_PktParsing
 *
 * @brief
 *
 * @param
 * @param
 * @param
 * @param
 * @param
 * @param
 *
 * @return
 */
void ptProfile_PktParsing(pt_t *pkt)
{
	unsigned char	chksum;


	chksum = ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);

	switch (pkt->id) {
	case SET_BT_NAME_REQ:
	case SET_BT_MATCH_PWD_REQ:
		break;

	case SET_PERSONAL_INFO_REQ:
		if (chksum == pkt->req.set_personal_info.chksum) {
			pkt->rsp.set_personal_info_ok.id	= SET_PERSONAL_INFO_OK_RSP;
			pkt->rsp.set_personal_info_ok.len	= 0x00;
			pkt->rsp.set_personal_info_ok.chksum	= 0x00;
		} else {
			pkt->rsp.set_personal_info_ng.id	= SET_PERSONAL_INFO_NG_RSP;
			pkt->rsp.set_personal_info_ng.len	= 0x00;
			pkt->rsp.set_personal_info_ng.chksum	= 0x00;
		}
		dmsg(("chksum=%02x, %02x\n", chksum, pkt->req.set_personal_info.chksum));
		break;

	case GET_WEEK_SSDCG_REQ:
		break;

	case GET_DATE_SSDCG_REQ:
		break;

	case GET_DEV_DATA_REQ:
	case GET_DEV_DATE_STEPS_REQ:
		break;

	case RST_TO_DEFAULTS_REQ:
		if (chksum == pkt->req.rst_to_defaults.chksum) {
			pkt->rsp.rst_to_defaults_ok.id		= RST_TO_DEFAULTS_OK_RSP;
			pkt->rsp.rst_to_defaults_ok.len		= 0x00;
			pkt->rsp.rst_to_defaults_ok.chksum	= 0x00;
		} else {
			pkt->rsp.rst_to_defaults_ng.id		= RST_TO_DEFAULTS_NG_RSP;
			pkt->rsp.rst_to_defaults_ng.len		= 0x00;
			pkt->rsp.rst_to_defaults_ng.chksum	= 0x00;
		}
		break;

	case CLR_DATA_REQ:
		break;

	case SET_DEV_TIME_REQ:
		if (chksum == pkt->req.set_dev_time.chksum) {
			pkt->rsp.set_dev_time_ok.id	= SET_DEV_TIME_OK_RSP;
			pkt->rsp.set_dev_time_ok.len	= 0x04;
			pkt->rsp.set_dev_time_ok.pwd[0]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[1]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[2]	= 0;
			pkt->rsp.set_dev_time_ok.pwd[3]	= 0;
			pkt->rsp.set_dev_time_ok.chksum	= 0;

		} else {
			pkt->rsp.set_dev_time_ng.id	= SET_DEV_TIME_NG_RSP;
			pkt->rsp.set_dev_time_ng.len	= 0;
			pkt->rsp.set_dev_time_ng.chksum	= 0;
		}
		break;

	case GET_DEV_TIME_REQ:
		if (chksum == pkt->req.get_dev_time.chksum) {
			pkt->rsp.get_dev_time_ok.id	= GET_DEV_TIME_OK_RSP;
			pkt->rsp.get_dev_time_ok.len	= 0x07;
			pkt->rsp.get_dev_time_ok.year	= 0x0e;
			pkt->rsp.get_dev_time_ok.month	= 0x0a;
			pkt->rsp.get_dev_time_ok.day	= 0x10;
			pkt->rsp.get_dev_time_ok.hour	= 0x0f;
			pkt->rsp.get_dev_time_ok.minute	= 0x38;
			pkt->rsp.get_dev_time_ok.second	= 0x33;
			pkt->rsp.get_dev_time_ok.week	= 0x05;
			pkt->rsp.get_dev_time_ok.chksum	= ptProfile_CalcChksum(pkt->header.len, pkt->header.ptr);
		} else {
			pkt->rsp.get_dev_time_ng.id	= GET_DEV_TIME_NG_RSP;
			pkt->rsp.get_dev_time_ng.len	= 0;
			pkt->rsp.get_dev_time_ng.chksum	= 0;
		}
		break;

	default:
		dmsg(("unknown opcode\n"));
		pkt->header.len = 0;
		break;

	}
}

