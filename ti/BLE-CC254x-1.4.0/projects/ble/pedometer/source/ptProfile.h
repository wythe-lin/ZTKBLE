/*
 ******************************************************************************
 * ptprofile.h - ProTrack profile
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
#ifndef __PTPROFILE_H__
#define __PTPROFILE_H__

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

// profile parameters
#define SIMPLEPROFILE_CHAR1		0	// RW uint8 - Profile Characteristic 1 value
#define SIMPLEPROFILE_CHAR2		1	// RW uint8 - Profile Characteristic 2 value

// ProTrack Profile Service UUID
#define PTPROFILE_SERV1_UUID		0xFFE5
#define PTPROFILE_SERV2_UUID		0xFFE0

// Key Pressed UUID
#define PTPROFILE_CHAR1_UUID		0xFFE9
#define PTPROFILE_CHAR2_UUID		0xFFE4

// Simple Keys Profile Services bit fields
#define PTPROFILE_SERVICE1		0x00000001
#define PTPROFILE_SERVICE2		0x00000002


// opcode & response
typedef enum {
	SET_BT_NAME_REQ			= 0x81,
	SET_BT_MATCH_PWD_REQ		= 0x85,
	SET_PERSONAL_INFO_REQ		= 0x83,
	GET_WEEK_SSDCG_REQ		= 0xC4,
	GET_DATE_SSDCG_REQ		= 0xC7,
	GET_DEV_DATA_REQ		= 0xC6,
	GET_DEV_DATE_STEPS_REQ		= 0xC8,
	RST_TO_DEFAULTS_REQ		= 0x87,
	CLR_DATA_REQ			= 0x88,
	SET_DEV_TIME_REQ		= 0xC2,
	GET_DEV_TIME_REQ		= 0x89
} pt_req_enum_t;

typedef enum {
	SET_BT_NAME_OK_RSP		= 0x21,
	SET_BT_NAME_NG_RSP		= 0x01,

	SET_BT_MATCH_PWD_OK_RSP		= 0x25,
	SET_BT_MATCH_PWD_NG_RSP		= 0x05,

	SET_PERSONAL_INFO_OK_RSP	= 0x23,
	SET_PERSONAL_INFO_NG_RSP	= 0x03,

	GET_WEEK_SSDCG_OK_RSP		= 0x24,
	GET_WEEK_SSDCG_NG_RSP		= 0x04,

	GET_DATE_SSDCG_OK_RSP		= 0x32,
	GET_DATE_SSDCG_NG_RSP		= 0x0B,

	GET_DEV_DATA_OK_RSP		= 0x26,
	GET_DEV_DATA_NG_RSP		= 0x06,

	GET_DEV_DATE_STEPS_OK_RSP	= 0x30,
	GET_DEV_DATE_STEPS_NG_RSP	= 0x0A,

	RST_TO_DEFAULTS_OK_RSP		= 0x27,
	RST_TO_DEFAULTS_NG_RSP		= 0x07,

	CLR_DATA_OK_RSP			= 0x28,
	CLR_DATA_NG_RSP			= 0x08,

	SET_DEV_TIME_OK_RSP		= 0x22,
	SET_DEV_TIME_NG_RSP		= 0x02,

	GET_DEV_TIME_OK_RSP		= 0x29,
	GET_DEV_TIME_NG_RSP		= 0x09
} pt_rsp_enum_t;


/*
 ******************************************************************************
 * Structures
 ******************************************************************************
 */
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	ptr[1];
} pt_header_t;


// set bluetooth name
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	name[15];
	unsigned char	chksum;
} pt_set_bt_name_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_bt_name_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_bt_name_ng_rsp_t;


// set BT match password
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	pwd[4];
	unsigned char	chksum;
} pt_set_bt_match_pwd_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_bt_match_pwd_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_bt_match_pwd_ng_rsp_t;


// read device system time
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_dev_time_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	year;
	unsigned char	month;
	unsigned char	day;
	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
	unsigned char	week;
	unsigned char	chksum;
} pt_get_dev_time_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_dev_time_ng_rsp_t;


// set device system time
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	year;
	unsigned char	month;
	unsigned char	day;
	unsigned char	hour;
	unsigned char	minute;
	unsigned char	second;
	unsigned char	week;
	unsigned char	chksum;
} pt_set_dev_time_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	pwd[4];
	unsigned char	chksum;
} pt_set_dev_time_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_dev_time_ng_rsp_t;


// set personal information
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	alarm1[5];
	unsigned char	alarm2[5];
	unsigned char	alarm3[5];
	unsigned char	alarm4[5];
	unsigned char	alarmIdle[6];
	unsigned char	alarmActivity[7];
	unsigned char	alarmOthers[14];
	unsigned char	chksum;
} pt_set_personal_info_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_personal_info_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_set_personal_info_ng_rsp_t;


// read Sports & Sleep Data Curve Graph by week
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	days;
	unsigned char	hours;
	unsigned char	n_reading_hour;
	unsigned char	chksum;
} pt_get_week_ssdcg_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	date[3];
	unsigned char	info[180];
	unsigned char	chksum;
} pt_get_week_ssdcg_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_week_ssdcg_ng_rsp_t;


// read Sports & Sleep Data Curve Graph by date
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	seqno[2];
	unsigned char	chksum;
} pt_get_date_ssdcg_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	info[60];
	unsigned char	chksum;
} pt_get_date_ssdcg_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_date_ssdcg_ng_rsp_t;


// read device data
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	local_id;
	unsigned char	chksum;
} pt_get_dev_data_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	rsp[1];	/* rsp length is variable */
} pt_get_dev_data_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_dev_data_ng_rsp_t;


// read device date and total steps
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_dev_date_steps_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	rsp[1];	/* rsp length is variable */
} pt_get_dev_date_steps_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_get_dev_date_steps_ng_rsp_t;


// reset to defaults
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_rst_to_defaults_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_rst_to_defaults_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_rst_to_defaults_ng_rsp_t;


// clear data
typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_clr_data_req_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_clr_data_ok_rsp_t;

typedef struct {
	unsigned char	id;
	unsigned char	len;
	unsigned char	chksum;
} pt_clr_data_ng_rsp_t;


// opcode
typedef union {
	unsigned char			id;
	pt_header_t			header;

	pt_set_bt_name_req_t		set_bt_name;
	pt_set_bt_match_pwd_req_t	set_bt_match_pwd;
	pt_set_personal_info_req_t	set_personal_info;
	pt_get_week_ssdcg_req_t		get_ssdcg_by_week;
	pt_get_date_ssdcg_req_t		get_ssdcg_by_date;
	pt_get_dev_data_req_t		get_dev_data;
	pt_get_dev_date_steps_req_t	get_dev_date_steps;
	pt_rst_to_defaults_req_t	rst_to_defaults;
	pt_clr_data_req_t		clear_data;
	pt_set_dev_time_req_t		set_dev_time;
	pt_get_dev_time_req_t		get_dev_time;
} pt_req_t;

// response
typedef union {
	unsigned char			id;
	pt_header_t			header;

	pt_set_bt_name_ok_rsp_t		set_bt_name_ok;
	pt_set_bt_name_ng_rsp_t		set_bt_name_ng;

	pt_set_bt_match_pwd_ok_rsp_t	set_bt_match_pwd_ok;
	pt_set_bt_match_pwd_ng_rsp_t	set_bt_match_pwd_ng;

	pt_set_personal_info_ok_rsp_t	set_personal_info_ok;
	pt_set_personal_info_ng_rsp_t	set_personal_info_ng;

	pt_get_week_ssdcg_ok_rsp_t	get_week_ssdcg_ok;
	pt_get_week_ssdcg_ng_rsp_t	get_week_ssdcg_ng;

	pt_get_date_ssdcg_ok_rsp_t	get_date_ssdcg_ok;
	pt_get_date_ssdcg_ng_rsp_t	get_date_ssdcg_ng;

	pt_get_dev_data_ok_rsp_t	get_dev_data_ok;
	pt_get_dev_data_ng_rsp_t	get_dev_data_ng;

	pt_get_dev_date_steps_ok_rsp_t	get_dev_date_steps_ok;
	pt_get_dev_date_steps_ng_rsp_t	get_dev_date_steps_ng;

	pt_rst_to_defaults_ok_rsp_t	rst_to_defaults_ok;
	pt_rst_to_defaults_ng_rsp_t	rst_to_defaults_ng;

	pt_clr_data_ok_rsp_t		clr_data_ok;
	pt_clr_data_ng_rsp_t		clr_data_ng;

	pt_set_dev_time_ok_rsp_t	set_dev_time_ok;
	pt_set_dev_time_ng_rsp_t	set_dev_time_ng;

	pt_get_dev_time_ok_rsp_t	get_dev_time_ok;
	pt_get_dev_time_ng_rsp_t	get_dev_time_ng;
} pt_rsp_t;


// 
typedef union {
	unsigned char	buf[200];	/* max size */

	unsigned char	id;
	pt_header_t	header;
	pt_req_t	req;
	pt_rsp_t	rsp;
} pt_t;


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
// Callback when a characteristic value has changed
typedef void (*ptProfileChange_t)(uint8 paramID);

typedef struct {
	ptProfileChange_t	pfnptProfileChange;		// Called when characteristic value changes
} ptProfileCBs_t;



/*
 ******************************************************************************
 * API Functions
 ******************************************************************************
 */
extern bStatus_t	ptProfile_AddService(uint32 services);
extern bStatus_t	ptProfile_RegisterAppCBs(ptProfileCBs_t *appCallbacks);
extern bStatus_t	ptProfile_SetParameter(uint8 param, uint8 len, void *value);
extern bStatus_t	ptProfile_GetParameter(uint8 param, void *value);

extern void		ptProfile_PktParsing(pt_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __PTPROFILE_H__ */


