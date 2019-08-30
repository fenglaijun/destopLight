#ifndef _ONENET_TCP_H_
#define _ONENET_TCP_H_

#include "c_types.h"

enum
{
	AUTHMETHOD_DEVID_APIKEY = 0,
	AUTHMETHOD_USRID_AUTHINFO,
};
enum
{
	SAVEDATA_DATATYPE_JSON1 = 1,
	SAVEDATA_DATATYPE_BIN,
	SAVEDATA_DATATYPE_JSON2,
	SAVEDATA_DATATYPE_JSON3,
	SAVEDATA_DATATYPE_FIELDSTRING,
};
typedef enum
{
	ONENET_TCP_BEGIN = 0,
	ONENET_TCP_LOGIN_REQUEST,
	ONENET_TCP_LOGIN_SUCCESS,
	ONENET_TCP_HEARTBEAT_REQUEST,
	ONENET_TCP_HEARTBEAR_SUCCESS,
}onenet_device_state_t;

typedef void (*pushdata_handle_callback_t)(uint8 *addr, int addrlen, uint8 *data, int datalen);
typedef void (*saveack_handle_callback_t)(char *jsonstr, int len);
typedef void (*cmdreq_handle_callback_t)(uint8 *cmdid, uint16 cmdid_len, uint8 *cmdbody, int cmdbody_len);
typedef void (*savedatabin_handle_callback_t)(uint8 *bindata, uint32 len);
typedef void (*savedatabin_field_handle_callback_t)(char *filedstr, uint32 len);
typedef void (*savedata_json_handle_callback_t)(char *json_str);


uint8 onenet_tcpclient_send(uint8_t *buf, uint32_t len);
uint8 onenet_tcpclient_get_send_state(void);
uint8 onenet_tcpclient_get_state(void);
onenet_device_state_t onenet_tcpclient_get_device_state(void);

bool onenet_tcpclient_create(uint8 encrypt);
bool onenet_tcpclient_delete(void);
void onenet_pushdata_handle_register(pushdata_handle_callback_t func);
void onenet_saveack_handle_register(saveack_handle_callback_t func);
void onenet_cmdreq_handle_register(cmdreq_handle_callback_t func);
void onenet_savedatabin_handle_register(savedatabin_handle_callback_t func);
void onenet_savedatabin_field_handle_register(savedatabin_field_handle_callback_t func);
void onenet_savedata_json_handle_register(savedata_json_handle_callback_t func);

void edp_heartbeat_stop(void);
void edp_heartbeat_resume(void);
void edp_login(void);
void edp_heartbeat(void);
void edp_pushdata(const char *dst_devid, uint8 *data, int data_len);
void edp_savedata(uint8 datatype, uint8 *dst_devid, uint8 *json_str, uint8 *bin_data, int bin_len, char *field);
void edp_cmdresp(uint8 *cmdid, uint8 *cmdbody, int cmdbody_len);

#endif

