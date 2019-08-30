#ifndef _ONENET_PRIVATE_H_
#define _ONENET_PRIVATE_H_


uint8 onenet_get_device_query_result2(void);
void onenet_httpclient_set_query_check(uint8 r);

uint32 onenet_httpclient_get_espconn(void);
void onenet_httpclient_device_create(void);
void onenet_httpclient_device_query(void);
void onenet_httpclient_device_delete(char *devid);
void onenet_httpclient_device_update(char *json);
void onenet_httpclient_ds_create(char *datastream);
void onenet_httpclient_ds_query(char *datastream);
void onenet_httpclient_ds_update(char *datastream, char *update_datastream);
void onenet_httpclient_ds_delete(char *datastream);
void onenet_httpclient_dp_create(char *datapoints);
void onenet_httpclient_dp_query(char *datastream_id);
void onenet_httpclient_apikey_create(char *key_name);
void onenet_httpclient_apikey_query(void);
void onenet_httpclient_apikey_delete(char *key_name);
void onenet_httpclient_bindata_create(char *datastream_id, uint8 *data, uint32 datalen);
void onenet_httpclient_bindata_query(char *bindata_name);
uint8 onenet_httpclient_set_server_ip(uint8 *buf_ip);
uint8 onenet_httpclient_set_server_domain(uint8 *buf_domain);

void print_buf(uint8 *data, int len);
void print_bufc(uint8 *data, int len);
uint8 onenet_httpclient_get_state(void);



/**/
uint8 onenet_tcpclient_reconnect(void);
uint8 onenet_tcpclient_disconnect(void);
uint8 onenet_tcpclient_connect(void);
uint32 onenet_tcpclient_get_espconn(void);
uint8 onenet_tcpclient_set_server_ip(uint8 *buf_ip);
uint8 onenet_tcpclient_set_server_domain(uint8 *buf_domain);


#endif

