#ifndef _ONENET_HTTP_H_
#define _ONENET_HTTP_H_

void onenet_set_device_id(const char *devid, uint8 len);
void onenet_set_master_apikey(const char *apikey, uint8 len);
void onenet_set_projectid(const char *userid, uint8 len);
void onenet_set_authinfo(const char *authinfo, uint8 len);
void onenet_set_authmethod(uint8_t method);
void onenet_set_register_flag(uint8 flag);
void onenet_set_device_name(uint8_t *name, uint8 len);
void onenet_set_device_apikey(uint8_t *apikey, uint8 len);

uint8 onenet_is_device_name_set(void);
uint8 onenet_is_register_flag_set(void);
uint8 onenet_is_master_apikey_set(void);
uint8 onenet_is_device_id_set(void);
uint8 onenet_is_device_apikey_set(void);
uint8 onenet_is_authinfo_set(void);
uint8 onenet_is_projectid_set(void);

uint8 *onenet_get_device_name(void);
uint8 *onenet_get_master_apikey(void);
uint8 *onenet_get_device_id(void);
uint8 *onenet_get_device_apikey(void);
uint8 *onenet_get_authinfo(void);
uint8 *onenet_get_projectid(void);

void onenet_device_info_print(void);
void onenet_param_restore(void);
void onenet_param_save(void);
void onenet_param_load(void);

bool onenet_httpclient_create(void);
bool onenet_httpclient_delete(void);
uint8 onenet_get_device_query_result(void);

#endif


