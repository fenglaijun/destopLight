#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"

#include "EdpKit.h"
#include "onenet_http.h"
#include "onenet_tcp.h"
#include "onenet_app.h"
#include "onenet_config.h"
#include "onenet_private.h"


#define PACK_SIZE 1024
#if 0
#define ONENET_HTTP_MASTER_APIKEY  onenet_get_user_master_apikey()
#else
#define ONENET_HTTP_MASTER_APIKEY  s_onenet_cloud.master_apikey
#endif

#define TIMESTAMP_FORMAT "yyyy-mm-ddThh:MM:ss"

#define ONENET_HTTP_DEVICE_CREATE_FRAME  "POST /devices HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define DEVICE_CREATE_JSON_FRAME "{\"title\":\"%s\",\"desc\":\"%s\",\"tags\":[%s],\"location\":{\"ele\":%s,\"lat\":%s,\"lon\":%s},\"private\":%s,\"protocol\":\"%s\",\"auth_info\":{\"sn\":\"%s\"},\"interval\":%s}"
#define ONENET_HTTP_DEVICE_QUERY_FRAME "GET /devices/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"
#define ONENET_HTTP_DEVICE_DELETE_FRAME "DELETE /devices/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"
#define ONENET_HTTP_DEVICE_UPDATE_FRAME "PUT /devices/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define DEVICE_UPDATE_JSON_FRAME "{\"title\":\"%s\"}"

#define ONENET_HTTP_DS_CREATE_FRAME "POST /devices/%s/datastreams HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define DS_CREATE_JSON_FRAME "{\"id\":\"%s\"}"
#define ONENET_HTTP_DS_QUERY_FRAME "GET /devices/%s/datastreams/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"
#define ONENET_HTTP_DS_UPDATE_FRAME "PUT /devices/%s/datastreams/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define DS_UPDATE_JSON_FRAME "{\"tags\":[\"%s\"]}"
#define ONENET_HTTP_DS_DELETE_FRAME "DELETE /devices/%s/datastreams/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"

#define ONENET_HTTP_DP_CREATE_FRAME "POST /devices/%s/datapoints HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define DP_CREATE_JSON_FRAME "{\"datastreams\":[{\"id\":\"%s\",\"datapoints\":[{\"value\":%s}]}]}"
#define ONENET_HTTP_DP_QUERY_FRAME "GET /devices/%s/datapoints?datastream_id=%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"

#define ONENET_HTTP_APIKEY_CREATE_FRAME "POST /keys HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n%s"
#define APIKEY_CREATE_JSON_FRAME "{\"title\":\"%s\",\"permissions\":[{\"resources\":[{\"dev_id\":\"%s\"}],\"access_methods\":[\"get\",\"put\",\"post\"]}]}"
#define ONENET_HTTP_APIKEY_QUERY_FRAME "GET /keys?device_id=%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"
#define ONENET_HTTP_APIKEY_DELETE_FRAME "DELETE /keys/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"

#define ONENET_HTTP_BINDATA_CREATE_FRAME "POST /bindata?device_id=%s&datastream_id=%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\nContent-Length:%d\r\n\r\n"
#define BINDATA_CREATE_ARRAY_FRAME   "XXXXXXXXXXXXXXXXXXXXXXXXX"
#define ONENET_HTTP_BINDATA_QUERY_FRAME "GET /bindata/%s HTTP/1.1\r\napi-key:%s\r\nHost:api.heclouds.com\r\n\r\n"



/******************************************************************************
*
*/
static void dns_check(void *timer_arg);
static void dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
static void httpclient_recv(void *arg, char *pdata, unsigned short len);
static void httpclient_sent_cb(void *arg);
static void httpclient_discon_cb(void *arg);
static void httpclient_connect_cb(void *arg);
static void httpclient_recon_cb(void *arg, sint8 errType);
void onenet_device_query_check(void *timer_arg);
void ICACHE_FLASH_ATTR disconnect_check(void *arg);

/******************************************************************************
*
*/
static struct espconn *s_pespconn_http=0;
static ip_addr_t s_server_ip;
static os_timer_t s_dns_timer;
static os_timer_t s_register_ckeck_timer;
static os_timer_t s_count_timer;
//static os_timer_t s_http_request_timer;
static os_timer_t s_delete_timer;

static os_timer_t s_query_check_timer;
static uint8 s_query_check_timeout;
static uint8 s_query_check_result;
static uint8 s_http_server_ip[]={183,230,40,39};
static uint8 s_http_domain[64]=ONENET_HTTP_DOMAIN;

static uint8 *s_http_send_buf;
static uint8 s_http_request_state;
//static int8 s_http_request_is_pending=-1;//-1:no http request 0:device_create 1:device_query ......
static uint8 s_http_request_result;
static uint8 s_current_create_datastream;
static uint8 s_device_query_check_state;
static uint8 s_device_query_check_substate;
static uint8 s_disconnect_state;
onenet_cloud_info_t s_onenet_cloud __attribute__((aligned(4)));
static uint8 s_httpclient_device_delete_is_self;




void ICACHE_FLASH_ATTR print_buf(uint8 *data, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		os_printf("%02x ", data[i]);
	}
}
void ICACHE_FLASH_ATTR print_bufc(uint8 *data, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		os_printf("%c", data[i]);
	}
}

/******************************************************************************
*
*/
char * ICACHE_FLASH_ATTR onenet_get_user_master_apikey(void)
{
	return "0sS65LxJsWcPvR8oaGmVTljCxqc=";//41880
}
void ICACHE_FLASH_ATTR onenet_set_device_id(const char *devid, uint8 len)
{
	if (len > 31)
		len = 31;
	os_memcpy(s_onenet_cloud.device_id, devid, len);
	s_onenet_cloud.device_id[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_master_apikey(const char *apikey, uint8 len)
{
	if (len > 31)
		len = 31;
	os_memcpy(s_onenet_cloud.master_apikey, apikey, len);
	s_onenet_cloud.master_apikey[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_projectid(const char *userid, uint8 len)
{
	if (len > 31)
		len = 31;
	os_memcpy(s_onenet_cloud.project_id, userid, len);
	s_onenet_cloud.project_id[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_authinfo(const char *authinfo, uint8 len)
{
	if (len > 63)
		len = 63;
	os_memcpy(s_onenet_cloud.auth_info, authinfo, len);
	s_onenet_cloud.auth_info[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_authmethod(uint8_t method)
{
	s_onenet_cloud.auth_method = method;
}
void ICACHE_FLASH_ATTR onenet_set_device_apikey(uint8_t *apikey, uint8 len)
{
	if (len > 31)
		len = 31;
	os_memcpy(s_onenet_cloud.device_apikey, apikey, len);
	s_onenet_cloud.device_apikey[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_device_name(uint8_t *name, uint8 len)
{
	if (len > 63)
		len = 63;
	os_memcpy(s_onenet_cloud.device_name, name, len);
	s_onenet_cloud.device_name[len] = 0;
}
void ICACHE_FLASH_ATTR onenet_set_register_flag(uint8 flag)
{
	s_onenet_cloud.register_flag = flag;
}


uint8 ICACHE_FLASH_ATTR onenet_is_device_name_set(void)
{
	return s_onenet_cloud.device_name[0] == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_register_flag_set(void)
{
	return s_onenet_cloud.register_flag == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_master_apikey_set(void)
{
	return s_onenet_cloud.master_apikey[0] == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_device_id_set(void)
{
	return s_onenet_cloud.device_id[0] == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_device_apikey_set(void)
{
	return s_onenet_cloud.device_apikey[0] == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_authinfo_set(void)
{
	return s_onenet_cloud.auth_info[0] == 0 ? 0 : 1;
}
uint8 ICACHE_FLASH_ATTR onenet_is_projectid_set(void)
{
	return s_onenet_cloud.project_id[0] == 0 ? 0 : 1;
}

uint8 *ICACHE_FLASH_ATTR onenet_get_master_apikey(void)
{
	return s_onenet_cloud.master_apikey;
}
uint8 *ICACHE_FLASH_ATTR onenet_get_device_id(void)
{
	return s_onenet_cloud.device_id;
}
uint8 *ICACHE_FLASH_ATTR onenet_get_device_apikey(void)
{
	return s_onenet_cloud.device_apikey;
}
uint8 *ICACHE_FLASH_ATTR onenet_get_authinfo(void)
{
	return s_onenet_cloud.auth_info;
}
uint8 *ICACHE_FLASH_ATTR onenet_get_projectid(void)
{
	return s_onenet_cloud.project_id;
}
uint8 *ICACHE_FLASH_ATTR onenet_get_device_name(void)
{
	return s_onenet_cloud.device_name;
}

void ICACHE_FLASH_ATTR onenet_device_info_print(void)
{
	os_printf("device_id: %s\n", s_onenet_cloud.device_id);
	os_printf("device_apikey: %s\n", s_onenet_cloud.device_apikey);
	os_printf("device_name: %s\n", s_onenet_cloud.device_name);
	os_printf("master_apikey: %s\n", s_onenet_cloud.master_apikey);
	os_printf("user_id: %s\n", s_onenet_cloud.project_id);
	os_printf("auth_info:%s\n", s_onenet_cloud.auth_info);
	os_printf("auth_method: %d\n", s_onenet_cloud.auth_method);
	os_printf("register_flag: %d\n", s_onenet_cloud.register_flag);
}

void ICACHE_FLASH_ATTR onenet_device_register_check(void)
{
	if ((s_onenet_cloud.device_id[0] != 0) &&
		(s_onenet_cloud.device_apikey[0] != 0) &&
		(s_onenet_cloud.auth_info[0] != 0) &&
		(s_onenet_cloud.project_id[0] != 0))
	{
		s_onenet_cloud.register_flag = 1;
		onenet_param_save();
		return ;
	}
	
	if (s_onenet_cloud.device_id[0] == 0)
	{
		onenet_httpclient_device_create();
	}
	else if (s_onenet_cloud.device_apikey[0] == 0)
	{
		if (s_onenet_cloud.device_id[0] != 0)
		{
			s_query_check_result &= ~0x80;
		}
		char *title = (char*)os_zalloc(128);
		if (title == NULL)
		{
			return ;
		}
		os_sprintf(title, "apikey_%08x_%08x", system_get_chip_id(), os_random());
		onenet_httpclient_apikey_create(title);
		
		os_free(title);
	}
}
uint8 ICACHE_FLASH_ATTR onenet_get_device_query_result(void)
{
	return s_query_check_result&0xC0?false:true;
}
uint8 ICACHE_FLASH_ATTR onenet_get_device_query_result2(void)
{
	return s_query_check_result;
}
void ICACHE_FLASH_ATTR onenet_device_query_check(void *timer_arg)
{
	os_timer_disarm(&s_query_check_timer);
	
	s_query_check_timeout++;
	switch(s_device_query_check_state)
	{
	case 0:
		if (s_device_query_check_substate==0)
		{
			// 查询device_id是否存在
			s_http_request_result = 0;
			s_device_query_check_substate = 1;
			s_query_check_timeout = 0;
			onenet_httpclient_device_query();
		}
		else if (s_device_query_check_substate==1)
		{
			if (s_query_check_timeout>10)
			{
				// 查询错误，device_id不存在
				ONENET_DEBUG("onenet_device_query_check: device_id error\r\n");
				s_query_check_result |= 0x80;

				s_device_query_check_state = 1;
				s_device_query_check_substate = 0;
				s_query_check_timeout = 0;
			}
		}
		else if (s_device_query_check_substate==2)
		{
			if (s_http_request_result)
			{
				// 查询错误，device_id不存在
				ONENET_DEBUG("onenet_device_query_check: device_id error\r\n");
				s_query_check_result |= 0x80;
			}
			else
			{
				// 开始查询device_apikey
				ONENET_DEBUG("onenet_device_query_check: device_id ok\r\n");
				s_query_check_result &= ~0x80;
			}
			s_device_query_check_state = 1;
			s_device_query_check_substate = 0;
			s_query_check_timeout = 0;
		}
		break;

	case 1:
		if (s_device_query_check_substate == 0)
		{
			// 查询device_apikey是否存在
			s_http_request_result = 0;
			s_device_query_check_substate = 1;
			onenet_httpclient_apikey_query();
		}
		else if (s_device_query_check_substate == 1)
		{
			if (s_query_check_timeout>10)
			{
				// device_apikey不存在
				ONENET_DEBUG("onenet_device_query_check: device_apikey error\r\n");
				s_query_check_result |= 0x40;

				if ((s_query_check_result & 0xC0) == 0xC0)
				{
					os_memset(s_onenet_cloud.device_id, 0, sizeof(s_onenet_cloud.device_id));
					os_memset(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
//					os_memset(s_onenet_cloud.auth_info, 0, sizeof(s_onenet_cloud.auth_info));
					s_onenet_cloud.register_flag = 0;
					onenet_param_save();
					onenet_device_register_check();
				}
				else if ((s_query_check_result & 0xC0) == 0x80)
				{
					os_memset(s_onenet_cloud.device_id, 0, sizeof(s_onenet_cloud.device_id));
//					os_memset(s_onenet_cloud.auth_info, 0, sizeof(s_onenet_cloud.auth_info));
					s_onenet_cloud.register_flag = 0;
					onenet_param_save();
					onenet_httpclient_apikey_delete(s_onenet_cloud.device_apikey);
				}
				else if ((s_query_check_result & 0xC0) == 0x40)
				{
					//没有查询到device_apikey，则清掉本地的device_apikey
					os_memset(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
					s_onenet_cloud.register_flag = 0;
					onenet_param_save();
					onenet_device_register_check();
				}
				return ;
			}
		}
		else if (s_device_query_check_substate == 2)
		{
			if (s_http_request_result)
			{
				// device_apikey不存在, 需要创建device_apikey
				ONENET_DEBUG("onenet_device_query_check: device_apikey error\r\n");
				s_query_check_result |= 0x40;
			}
			else
			{
				//device_apikey查询成功，设备初始化成功
				ONENET_DEBUG("onenet_device_query_check: device_apikey ok\r\n");
				s_query_check_result &= ~0x40;
			}
			s_device_query_check_state = 0;
			s_device_query_check_substate = 0;
			s_query_check_timeout = 0;

			if ((s_query_check_result & 0xC0) == 0xC0)
			{
				os_memset(s_onenet_cloud.device_id, 0, sizeof(s_onenet_cloud.device_id));
				os_memset(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
//				os_memset(s_onenet_cloud.auth_info, 0, sizeof(s_onenet_cloud.auth_info));
				s_onenet_cloud.register_flag = 0;
				onenet_param_save();
				onenet_device_register_check();
			}
			else if ((s_query_check_result & 0xC0) == 0x80)
			{
				os_memset(s_onenet_cloud.device_id, 0, sizeof(s_onenet_cloud.device_id));
//				os_memset(s_onenet_cloud.auth_info, 0, sizeof(s_onenet_cloud.auth_info));
				s_onenet_cloud.register_flag = 0;
				onenet_param_save();
				onenet_httpclient_apikey_delete(s_onenet_cloud.device_apikey);
			}
			else if ((s_query_check_result & 0xC0) == 0x40)
			{
				//没有查询到device_apikey，则清掉本地的device_apikey
				os_memset(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
				s_onenet_cloud.register_flag = 0;
				onenet_param_save();
				onenet_device_register_check();
			}
			return;
		}
		break;
	}
	
	os_timer_setfn(&s_query_check_timer, onenet_device_query_check, 0);
	os_timer_arm(&s_query_check_timer, 1000, 0);
}

uint8 ICACHE_FLASH_ATTR onenet_device_register_get_status(void)
{
	return s_onenet_cloud.register_flag;
}
void ICACHE_FLASH_ATTR onenet_param_save(void)
{
	system_param_save_with_protect(ONENET_PARAM_START_SEC,
									(void*)&s_onenet_cloud, sizeof(onenet_cloud_info_t));
}
void ICACHE_FLASH_ATTR onenet_param_load(void)
{
	system_param_load(ONENET_PARAM_START_SEC, 0,
						(void*)&s_onenet_cloud, sizeof(onenet_cloud_info_t));

	if (s_onenet_cloud.init_flag != 1)
	{
		// default param
		os_printf("onenet_param_load: set default param.\n");
		os_memset(&s_onenet_cloud, 0, sizeof(onenet_cloud_info_t));
		s_onenet_cloud.init_flag = 1;
		onenet_param_save();
	}
}
void ICACHE_FLASH_ATTR onenet_param_restore(void)
{
	// default param
	os_memset(&s_onenet_cloud, 0, sizeof(onenet_cloud_info_t));
	s_onenet_cloud.init_flag = 1;
	onenet_param_save();	
}



/******************************************************************************
*
*/
static void ICACHE_FLASH_ATTR httpclient_recv(void *arg, char *pdata, unsigned short len)
{
#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("HTTP_RECV:\r\n");
	print_bufc(pdata, len);
	ONENET_DEBUG("\r\n");
#endif
	s_device_query_check_substate = 2;
	char *pstr, *ptemp;
	char *phead, *ptail;
	int length;
	uint8 errno;
	pstr = (char*)os_strstr(pdata, "\"errno\":");
	if (pstr == NULL)
	{
		ONENET_DEBUG("http frame error!\r\n");
		s_http_request_result = 255;
		return ;
	}
	else
	{
		pstr += 8;//"errno":3
		s_http_request_result = atoi(pstr);
	}

	if (s_http_request_result)
	{
		ONENET_DEBUG("http request error: %d\n", s_http_request_result);
		return ;
	}
	pstr = os_strchr(pdata, '{');
	if (pstr == NULL)
	{
		ONENET_DEBUG("http jsonstring error!\r\n");
		return ;
	}
	
	switch (s_http_request_state)
	{
	// device
	case 0://create
		ptemp = (char*)os_strstr(pstr, "\"device_id\":");
		if (ptemp)
		{
			phead = ptemp+13;
			ptail = os_strchr(phead, '"');
			length = ptail-phead;
			if (length < 32)
			{
				os_memcpy(s_onenet_cloud.device_id, phead, length);
				s_onenet_cloud.device_id[length] = 0;
				onenet_device_register_check();
				s_query_check_result &= ~0x80;
				onenet_param_save();
				ONENET_DEBUG("HTTP:device_id is ok!\n");
			}
		}
	case 1://query
		ptemp = (char*)os_strstr(pstr, "\"auth_info\":");
		if (ptemp)
		{
			onenet_device_register_check();
//			onenet_param_save();
			ONENET_DEBUG("HTTP:auth_info is ok!\n");
		}
		break;
	case 2://delete, 删除该设备后，对应的device_apikey也要删除, 如果删除的不是自己则不需要
		if (s_httpclient_device_delete_is_self)
		{
			s_httpclient_device_delete_is_self = 0;
			os_memset(s_onenet_cloud.device_id, 0, sizeof(s_onenet_cloud.device_id));
			os_memset(s_onenet_cloud.device_name, 0, sizeof(s_onenet_cloud.device_name));
			os_memset(s_onenet_cloud.auth_info, 0, sizeof(s_onenet_cloud.auth_info));
			s_onenet_cloud.register_flag = 0;
			onenet_param_save();
			onenet_httpclient_apikey_delete(s_onenet_cloud.device_apikey);//删除该设备的device_apikey
			ONENET_DEBUG("HTTP:device_id is delete!\n");
		}
		break;
	case 3://update
		break;

	// datastream
	case 10://create
		break;
	case 11://query
		break;
	case 12://delete
		break;
	case 13://update
		break;

	// datapoint
	case 20://create
		break;
	case 21://query
		break;

	// device_apikey
	case 30://create
		ptemp = (char*)os_strstr(pstr, "\"key\":");
		if (ptemp == NULL)
		{
			break;
		}
		phead = ptemp + 7;
		ptail = os_strchr(phead, '"');
		length = ptail-phead;
		if (length < 32)
		{
			os_memcpy(s_onenet_cloud.device_apikey, phead, length);
			s_onenet_cloud.device_apikey[length] = 0;
			onenet_device_register_check();
			onenet_param_save();
			s_query_check_result &= ~0x40;
			ONENET_DEBUG("HTTP:device_apikey is ok!\n");
		}
		break;
	case 31://query
		ptemp = (char*)os_strstr(pstr, "\"key\":");
		if (ptemp == NULL)
		{
			// 没有查询到device_apikey
			s_http_request_result = 255;
#if 0
			//没有查询到device_apikey，则清掉本地的device_apikey
			os_memcpy(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
			s_onenet_cloud.register_flag = 0;
			onenet_param_save();
			onenet_device_register_check();
#endif
		}
		break;
	case 32://delete
		os_memset(s_onenet_cloud.device_apikey, 0, sizeof(s_onenet_cloud.device_apikey));
		onenet_param_save();
		onenet_device_register_check();
		ONENET_DEBUG("HTTP:device_apikey is delete!\n");
		break;

	// bindata
	case 40://create
		break;
	case 41://query
		break;
	
	default:
		ONENET_DEBUG("http_client error request!\r\n");
		break;
	}
}

static void ICACHE_FLASH_ATTR httpclient_sent_cb(void *arg)
{
	ONENET_DEBUG("httpclient_sent_cb\r\n");
}
static void ICACHE_FLASH_ATTR httpclient_discon_cb(void *arg)
{
#if 0
	espconn_regist_connectcb(s_pespconn_http, httpclient_connect_cb);
	espconn_regist_disconcb(s_pespconn_http, httpclient_discon_cb);
	espconn_connect(s_pespconn_http);
#endif
	ONENET_DEBUG("httpclient_discon_cb\r\n");
	
	if (s_disconnect_state)
	{
		s_disconnect_state = 0;
		disconnect_check(NULL);
	}
}
static void ICACHE_FLASH_ATTR httpclient_connect_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;

	espconn_regist_recvcb(pespconn, httpclient_recv);
	espconn_regist_sentcb(pespconn, httpclient_sent_cb);
	ONENET_DEBUG("httpclient_connect_cb\r\n");

	if (s_onenet_cloud.register_flag != 1)//该设备还没有注册过, 设备信息不完全
	{
		onenet_device_register_check();
		ONENET_DEBUG("onenet_device_register_check\r\n");
	}
	else
	{
		//设备已经注册过，则查询该设备确认, 查询失败的话则重新创建设备
		s_device_query_check_state = 0;
		s_device_query_check_substate = 0;
		s_query_check_timeout = 0;
		onenet_device_query_check(NULL);
		ONENET_DEBUG("onenet_device_query_check\r\n");
	}

#if 0
	// 如果有http请求挂起, 则发送http请求
	onenet_httpclient_request_check(NULL);
#endif
}
static void ICACHE_FLASH_ATTR httpclient_recon_cb(void *arg, sint8 errType)
{
	ONENET_DEBUG("httpclient_recon_cb\r\n");
}

#ifdef ONENET_USE_DNS
static void ICACHE_FLASH_ATTR dns_check(void *timer_arg)
{
	espconn_gethostbyname(s_pespconn_http, s_http_domain, &s_server_ip, dns_found);
}
static void ICACHE_FLASH_ATTR dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;

	if (ipaddr == NULL)
	{
		ONENET_DEBUG("HTTP DNS: Found, but got no ip, try to reconnect\r\n");
		os_timer_disarm(&s_dns_timer);
		os_timer_setfn(&s_dns_timer, (os_timer_func_t *)dns_check, 0);
		os_timer_arm(&s_dns_timer, 2000, 0);
		return;
	}

	ONENET_DEBUG("HTTP DNS: found ip %d.%d.%d.%d\n",
	             *((uint8 *) &ipaddr->addr),
	             *((uint8 *) &ipaddr->addr + 1),
	             *((uint8 *) &ipaddr->addr + 2),
	             *((uint8 *) &ipaddr->addr + 3));

	//if (s_server_ip.addr == 0 && ipaddr->addr != 0)
	if (ipaddr->addr != 0)
	{
		s_server_ip.addr = ipaddr->addr;
		os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
		pespconn->proto.tcp->local_port = espconn_port();
		pespconn->proto.tcp->remote_port = 80;
		espconn_regist_connectcb(pespconn, httpclient_connect_cb);
		espconn_regist_disconcb(pespconn, httpclient_discon_cb);

		espconn_connect(pespconn);
		ONENET_DEBUG("httpclient connecting...\n");
	}
}
#endif

void ICACHE_FLASH_ATTR onenet_httpclient_set_query_check(uint8 r)
{
	s_query_check_result = r;
}

bool ICACHE_FLASH_ATTR onenet_httpclient_create(void)
{
	// 没有设置master_apikey则不允许建立onenet http连接
	if (s_onenet_cloud.master_apikey[0] == 0)
	{
		ONENET_DEBUG("HTTP:master_apikey not set!!!\r\n");
		return false;
	}
	else if (s_onenet_cloud.project_id[0] == 0)
	{
		ONENET_DEBUG("HTTP:project_id not set!!!\r\n");
		return false;
	}
	else if (s_onenet_cloud.auth_info[0] == 0)
	{
		ONENET_DEBUG("HTTP:auth_info not set!!!\r\n");
		return false;
	}
	else
	{
		ONENET_DEBUG("HTTP:master_apikey && project_id && auth_info is ok!!!\r\n");
	}
	
	if (s_pespconn_http != NULL)
	{
		ONENET_DEBUG("http already create!!\n");
		return false;
	}
	s_pespconn_http = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if (s_pespconn_http == NULL)
	{
		ONENET_DEBUG("mem err!\n");
		return false;
	}
	s_pespconn_http->type = ESPCONN_TCP;
	s_pespconn_http->state = ESPCONN_NONE;
	s_pespconn_http->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	if (s_pespconn_http->proto.tcp == NULL)
	{
		os_free(s_pespconn_http);
		return false;
	}

	s_http_send_buf = (uint8*)os_malloc(PACK_SIZE);
	if (s_http_send_buf == NULL)
	{
		os_free(s_pespconn_http->proto.tcp);
		os_free(s_pespconn_http);
		return false;
	}

	s_query_check_result = 0xFF;
	
#ifdef ONENET_USE_DNS
	s_pespconn_http->proto.tcp->local_port = espconn_port();
	s_pespconn_http->proto.tcp->remote_port = 80;
	espconn_regist_connectcb(s_pespconn_http, httpclient_connect_cb);
	espconn_regist_reconcb(s_pespconn_http, httpclient_recon_cb);
	dns_check(NULL);
#else
	os_memcpy(s_pespconn_http->proto.tcp->remote_ip, s_http_server_ip, 4);
	s_pespconn_http->proto.tcp->local_port = espconn_port();
	s_pespconn_http->proto.tcp->remote_port = 80;
	espconn_regist_connectcb(s_pespconn_http, httpclient_connect_cb);
	espconn_regist_disconcb(s_pespconn_http, httpclient_discon_cb);
	espconn_connect(s_pespconn_http);
	ONENET_DEBUG("httpclient connecting...\n");
#endif

	return true;
}
void ICACHE_FLASH_ATTR disconnect_check(void *arg)
{
	os_timer_disarm(&s_delete_timer);
	if (s_pespconn_http)
	{
		if (s_pespconn_http->proto.tcp)
		{
			os_free(s_pespconn_http->proto.tcp);
			s_pespconn_http->proto.tcp = 0;
		}
		os_free(s_pespconn_http);
		s_pespconn_http = 0;
	}
	if (s_http_send_buf)
	{
		os_free(s_http_send_buf);
		s_http_send_buf = 0;
	}
	ONENET_DEBUG("httpclient deleted...\n");
}
bool ICACHE_FLASH_ATTR onenet_httpclient_delete(void)
{
	if (s_pespconn_http == NULL)
	{
		return false;
	}
	
	os_timer_disarm(&s_dns_timer);
	if (s_pespconn_http)
	{
		s_disconnect_state = 1;
		espconn_disconnect(s_pespconn_http);
	}
	os_timer_disarm(&s_delete_timer);
	os_timer_setfn(&s_delete_timer, disconnect_check, 0);
	os_timer_arm(&s_delete_timer, 5000, 0);
	return true;
}

uint32 ICACHE_FLASH_ATTR onenet_httpclient_get_espconn(void)
{
	if (s_pespconn_http)
	{
		return (uint32)s_pespconn_http;
	}
	return 0;
}

/******************************************************************************
* device
*/
void ICACHE_FLASH_ATTR onenet_httpclient_device_create(void)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}

	uint8 *http_data = (uint8*)os_zalloc(512);
	if (http_data==NULL)
		return;
	uint16 len;
	char *title = (uint8*)os_zalloc(128);
	if (title == NULL)
	{
		os_free(http_data);
	}
	os_sprintf(title, "%s_%08x_%08x", s_onenet_cloud.device_name, system_get_chip_id(), os_random());
	len = os_sprintf(http_data, DEVICE_CREATE_JSON_FRAME, title, "edpdevice", "edp", "290000", "22.606035", "113.839790", "false",
				"EDP", s_onenet_cloud.auth_info,"60");
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DEVICE_CREATE_FRAME, ONENET_HTTP_MASTER_APIKEY, len, http_data);
	s_http_request_state = 0;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);

	os_free(http_data);
	os_free(title);
	ONENET_DEBUG("http_device_create\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_device_query(void)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DEVICE_QUERY_FRAME,
				s_onenet_cloud.device_id, 
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 1;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_device_query\n");
}

void ICACHE_FLASH_ATTR onenet_httpclient_device_delete(char *devid)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}

	// 判断删除的是不是自己
	if (os_strncmp(s_onenet_cloud.device_id, devid, 32) == 0)
	{
		s_httpclient_device_delete_is_self = 1;
	}
	else
	{
		s_httpclient_device_delete_is_self = 0;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DEVICE_DELETE_FRAME,
				devid,//s_onenet_cloud.device_id,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 2;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_device_delete\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_device_update(char *json)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DEVICE_UPDATE_FRAME,
				s_onenet_cloud.device_id,
				ONENET_HTTP_MASTER_APIKEY, json);
	s_http_request_state = 3;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_device_update\n");
}
/*
 * datastreams
 */
void ICACHE_FLASH_ATTR onenet_httpclient_ds_create(char *datastream)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	uint8 http_data[128];
	int len;
	len = os_sprintf(http_data, DS_CREATE_JSON_FRAME, datastream);//{"id":"color_red"}
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DS_CREATE_FRAME,
				s_onenet_cloud.device_id,
				ONENET_HTTP_MASTER_APIKEY, len, http_data);
	s_http_request_state = 10;
	s_http_request_result = 0;
	os_strcpy(s_current_create_datastream, datastream);
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_ds_create\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_ds_query(char *datastream)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DS_QUERY_FRAME,
				s_onenet_cloud.device_id,
				datastream,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 11;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_ds_query\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_ds_update(char *datastream, char *update_datastream)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	uint8 http_data[128];
	int len;
	len = os_sprintf(http_data, DS_UPDATE_JSON_FRAME, datastream);//{"tags":["%s"]}
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DS_UPDATE_FRAME,
				s_onenet_cloud.device_id,
				datastream,//color_red
				ONENET_HTTP_MASTER_APIKEY, len, http_data);
	s_http_request_state = 12;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_ds_update\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_ds_delete(char *datastream)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DS_DELETE_FRAME,
				s_onenet_cloud.device_id,
				datastream,//color_red
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 13;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_ds_delete\n");
}
/*
 * datapoint
 */
void ICACHE_FLASH_ATTR onenet_httpclient_dp_create(char *datapoints)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DP_CREATE_FRAME,
				s_onenet_cloud.device_id,
				ONENET_HTTP_MASTER_APIKEY,
				os_strlen(datapoints), datapoints);
	s_http_request_state = 20;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_dp_create\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_dp_query(char *datastream_id)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_DP_QUERY_FRAME,
				s_onenet_cloud.device_id,
				datastream_id,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 21;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_dp_query\n");
}

/*
 * apikey
 */
void ICACHE_FLASH_ATTR onenet_httpclient_apikey_create(char *key_name)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	uint8 http_data[128];
	int len;
	len = os_sprintf(http_data, APIKEY_CREATE_JSON_FRAME, key_name, s_onenet_cloud.device_id);
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_APIKEY_CREATE_FRAME,
				ONENET_HTTP_MASTER_APIKEY,
				len, http_data);
	s_http_request_state = 30;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_apikey_create\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_apikey_query(void)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_APIKEY_QUERY_FRAME,
				s_onenet_cloud.device_id,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 31;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_apikey_query\n");
}
void ICACHE_FLASH_ATTR onenet_httpclient_apikey_delete(char *key_name)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_APIKEY_DELETE_FRAME,
				key_name,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 32;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_apikey_delete\n");
}

/*
 * bindata
 */
void ICACHE_FLASH_ATTR onenet_httpclient_bindata_create(char *datastream_id, uint8 *data, uint32 datalen)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	uint8 *buf = (uint8*)os_malloc(PACK_SIZE+datalen);
	int len;
	len = os_sprintf(buf, ONENET_HTTP_BINDATA_CREATE_FRAME,
				s_onenet_cloud.device_id, datastream_id,
				ONENET_HTTP_MASTER_APIKEY, datalen);
	os_memcpy(buf+len, data, datalen);
	s_http_request_state = 40;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, buf, len);
	ONENET_DEBUG("http_bindata_create\n");
	os_free(buf);
}
void ICACHE_FLASH_ATTR onenet_httpclient_bindata_query(char *bindata_name)
{
	if (s_pespconn_http == NULL)
	{
		return ;
	}
	
	int len;
	len = os_sprintf(s_http_send_buf, ONENET_HTTP_BINDATA_QUERY_FRAME,
				bindata_name,
				ONENET_HTTP_MASTER_APIKEY);
	s_http_request_state = 41;
	s_http_request_result = 0;
	espconn_send(s_pespconn_http, s_http_send_buf, len);
	ONENET_DEBUG("http_bindata_query\n");
}

uint8 ICACHE_FLASH_ATTR onenet_httpclient_get_state(void)
{
	os_printf("http_state:\n");
	os_printf("state:%d\n", s_pespconn_http->state);
	md(s_pespconn_http, sizeof(struct espconn), 1);
}


/******************************************************************************
* "192.168.123.234"
*/
uint8 ICACHE_FLASH_ATTR onenet_httpclient_set_server_ip(uint8 *buf_ip)
{
	int a,b,c,d;
	uint8 *pch = buf_ip;
	int len;

	len = os_strlen(buf_ip);
	if (len < 7 || len > 15)
	{
		return false;
	}

	// a
	a = atoi(pch);
	if (a < 0 || a > 255)
	{
		return false;
	}
	// .b
	pch = os_strchr(pch, '.');
	if (pch == NULL)
	{
		return false;
	}
	pch++;
	b = atoi(pch);
	if (b < 0 || b > 255)
	{
		return false;
	}
	// .c
	pch = os_strchr(pch, '.');
	if (pch == NULL)
	{
		return false;
	}
	pch++;
	c = atoi(pch);
	if (c < 0 || c > 255)
	{
		return false;
	}
	// .d
	pch = os_strchr(pch, '.');
	if (pch == NULL)
	{
		return false;
	}
	pch++;
	d = atoi(pch);
	if (d < 0 || d > 255)
	{
		return false;
	}

	// set ip
	s_http_server_ip[0]=a;
	s_http_server_ip[1]=b;
	s_http_server_ip[2]=c;
	s_http_server_ip[3]=d;
	return true;
}
uint8 ICACHE_FLASH_ATTR onenet_httpclient_set_server_domain(uint8 *buf_domain)
{
	os_strncpy(s_http_domain, buf_domain, 64);
	return true;
}


