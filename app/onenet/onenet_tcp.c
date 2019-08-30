#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"

#include "EdpKit.h"
#include "onenet_tcp.h"
#include "onenet_config.h"
#include "onenet_private.h"
#ifdef _ENCRYPT
#include "edp_openssl.h"
#endif


static void dns_check(void *timer_arg);
static void dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
static void onenet_login_check(void *timer_arg);
static void onenet_heartbeat_check(void *timer_arg);
static void reconnect_check(void *timer_arg);

/******************************************************************************
*
*/
static struct espconn *s_pespconn_tcp=0;
static os_timer_t s_login_timer;
static os_timer_t s_heartbeat_timer;
static os_timer_t s_dns_timer;
static ip_addr_t s_server_ip;
static os_timer_t s_reconnect_timer;
static os_timer_t s_disconnect_timer;

static uint8 s_tcp_server_ip[] = {183,230,40,39};
static uint8 s_tcp_domain[]=ONENET_TCP_DOMAIN;

static uint8 s_onenet_tcp_state=ONENET_TCP_BEGIN;
static uint8 s_heartbeat_miss;
static uint8 s_login_miss;
static uint8 s_reconnect_miss;
static uint8 s_is_encrypt = 0;
static uint8 s_send_data_ok;
static uint8 s_heartbeat_onoff=0;
static uint8 s_reconnect_state;
static uint8 s_disconnect_evt=0;

static pushdata_handle_callback_t s_pushdata_handle_callback;
static saveack_handle_callback_t s_saveack_handle_callback;
static cmdreq_handle_callback_t s_cmdreq_handle_callback;
static savedatabin_handle_callback_t s_savedatabin_handle_callback;
static savedatabin_field_handle_callback_t s_savedatabin_field_handle_callback;
static savedata_json_handle_callback_t s_savedata_json_handle_callback;

extern onenet_cloud_info_t s_onenet_cloud;



/******************************************************************************
*
*/
static void ICACHE_FLASH_ATTR onenet_login_check(void *timer_arg)
{
	os_timer_disarm(&s_login_timer);
	
	if (s_onenet_tcp_state == ONENET_TCP_LOGIN_SUCCESS)
	{
		s_heartbeat_miss = 0;
		s_onenet_tcp_state = ONENET_TCP_HEARTBEAT_REQUEST;

		onenet_heartbeat_check(NULL);
	}
	else
	{
		os_timer_disarm(&s_login_timer);
		os_timer_setfn(&s_login_timer, (os_timer_func_t *)onenet_login_check, NULL);
		os_timer_arm(&s_login_timer, 4000, 0);

		s_onenet_tcp_state = ONENET_TCP_LOGIN_REQUEST;
		s_login_miss++;
		// 多次登录失败，则重新连接一下
		if (s_login_miss > 4)
		{
			s_login_miss = 0;
			onenet_tcpclient_reconnect();
			return ;
		}
#ifdef _ENCRYPT
		if (s_is_encrypt)
		{
			edp_encryptreq();
		}
		else
#endif
		{
			edp_login();
		}
	}
}
static void ICACHE_FLASH_ATTR onenet_heartbeat_check(void *timer_arg)
{
	os_timer_disarm(&s_heartbeat_timer);

	if (s_onenet_tcp_state >= ONENET_TCP_LOGIN_SUCCESS)
	{
		if (s_onenet_tcp_state == ONENET_TCP_HEARTBEAT_REQUEST)
		{
			s_heartbeat_miss++;
		}
		if (s_heartbeat_miss > 3)
		{
			s_onenet_tcp_state = ONENET_TCP_LOGIN_REQUEST;
			onenet_login_check(NULL);
		}
		else
		{
			os_timer_disarm(&s_heartbeat_timer);
			os_timer_setfn(&s_login_timer, (os_timer_func_t *)onenet_heartbeat_check, NULL);
			os_timer_arm(&s_login_timer, 60000, 0);// 60s一次心跳数据包
			s_onenet_tcp_state = ONENET_TCP_HEARTBEAT_REQUEST;
			if (s_heartbeat_onoff == 0)
			{
				edp_heartbeat();
			}
		}
	}
}
static void ICACHE_FLASH_ATTR tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t msg_type;
	int ret;
	uint32 remainlen;
	uint8 highbyte, lowbyte, addrlen;
	uint8 *addr = NULL;
	uint8 b;
	char *src_devid = NULL;
	
	EdpPacket pkt;
	pkt._data = pdata;
	pkt._read_pos = 0;
	pkt._capacity = len;
	pkt._write_pos = len;

#ifdef GLOBAL_DEBUG_ON
	os_printf("TCP RECV:");
	print_buf(pdata, len);
	os_printf("\r\n");
#endif

	ret = IsPkgComplete(pdata, len);
	if (ret <= 0)
	{
		ONENET_DEBUG("ERROR: pkg imcomplete!\n");
		return ;
	}

	msg_type = EdpPacketType(&pkt);

#ifdef _ENCRYPT
	if (msg_type != ENCRYPTRESP)
	{
		if (s_is_encrypt)
		{
			SymmDecrypt(&pkt);
		}
	}
#endif

	switch (msg_type)
	{
	case CONNRESP://S->C
		ONENET_DEBUG("CONNRESP\r\n");
		s_login_miss = 0;
		ret = UnpackConnectResp(&pkt);
		if (ret == 0)
		{
			ONENET_DEBUG("CONNRESP OK\n");
			s_onenet_tcp_state = ONENET_TCP_LOGIN_SUCCESS;
			onenet_login_check(NULL);
		}
		else
		{
			ONENET_DEBUG("CONNRESP ERROR: %d\n", ret);
			os_timer_disarm(&s_login_timer);
		}
		break;

	case PUSHDATA:// C->S  S->C
		ONENET_DEBUG("PUSHDATA\r\n");
		char *data=0;
		uint32 data_len;
		ret = UnpackPushdata(&pkt, &src_devid, &data, &data_len);
		if (ret)
		{
			ONENET_DEBUG("PUSHDATA: %d\r\n", ret);
			return ;
		}
		// msg body callback
		if (s_pushdata_handle_callback)
		{
			s_pushdata_handle_callback(src_devid, os_strlen(src_devid), data, data_len);
		}

		if (src_devid)
		{
			os_free(src_devid);
		}
		if (data)
		{
			os_free(data);
		}
		break;

	case CONNCLOSE:// S->C
		ReadByte(&pkt, &b);
		ReadByte(&pkt, &b);
		ONENET_DEBUG("CONNCLOSE: %d\n", b);
		// 数据包出错，连接关闭，则重新登录
//		s_onenet_tcp_state = ONENET_TCP_LOGIN_REQUEST;
//		onenet_login_check(NULL);
		onenet_tcpclient_reconnect();
		break;

	case SAVEDATA:// S->C  C->S
		ONENET_DEBUG("SAVEDATA\r\n");
		uint8 datatype;
		char *filedstr = NULL;
		ret = UnpackSavedata(&pkt, &src_devid, &datatype);
		if (ret)
		{
			ONENET_DEBUG("SAVEDATA: %d\r\n", ret);
			return ;
		}
		
		switch (datatype)
		{
		case 5://,;field;field;field;field
			ReadStr(&pkt, &filedstr);
			if (s_savedatabin_field_handle_callback)
			{
				s_savedatabin_field_handle_callback(filedstr, os_strlen(filedstr));
			}
			if (filedstr)
			{
				os_free(filedstr);
			}
			break;
		case 4:
		case 3:
		case 1://jsonstring
			if (s_savedata_json_handle_callback)
			{
				char *json_str;
				UnpackSavedataJson(&pkt, &json_str);
				s_savedata_json_handle_callback(json_str);
				if (json_str)
				{
					os_free(json_str);
				}
			}
			break;
		case 2://token ds_id jsonstring bindata
			{
			uint8 *bin_data = 0;
			uint32 bin_len;
			char *json_str = 0;
			ret = UnpackSavedataBin(&pkt, &json_str, &bin_data, &bin_len);
			if (s_savedata_json_handle_callback)
			{
				s_savedata_json_handle_callback(json_str);
			}
			if (s_savedatabin_handle_callback)
			{
				s_savedatabin_handle_callback(bin_data, bin_len);
			}

			if (json_str)
			{
				os_free(json_str);
			}
			if (bin_data)
			{
				os_free(bin_data);
			}
			}
			break;
		default:
			ONENET_DEBUG("SAVEDATA: error datatype!\r\n");
			break;
		}
		
		if (src_devid)
		{
			os_free(src_devid);
		}
		break;

	case SAVEACK:
		ONENET_DEBUG("SAVEACK\r\n");
		if (ReadRemainlen(&pkt, &remainlen))
		{
			return ;
		}
		ReadByte(&pkt, &b);
		if (b & 0x80)
		{
			char *jsonstr = NULL;
			int jsonstr_len;
			ReadStr(&pkt, &jsonstr);
			jsonstr_len = os_strlen(jsonstr);
			if (s_saveack_handle_callback)
			{
				s_saveack_handle_callback(jsonstr, jsonstr_len);
			}
			os_free(jsonstr);
		}
		break;

	case CMDREQ:
		ONENET_DEBUG("CMDREQ\r\n");
		if (ReadRemainlen(&pkt, &remainlen))
		{
			ONENET_DEBUG("CMDREQ: %d\r\n", ERR_UNPACK_CMDREQ_REMAIN);
			return ;
		}

		uint16 cmdid_len;
		uint8 *cmdid = 0;
		ReadUint16(&pkt, &cmdid_len);
		ReadBytes(&pkt, &cmdid, cmdid_len);

		int cmdbody_len;
		ReadUint32(&pkt, &cmdbody_len);
		if (s_cmdreq_handle_callback)
		{
			s_cmdreq_handle_callback(cmdid, cmdid_len, pkt._data+pkt._read_pos, cmdbody_len);
		}
		if (cmdid)
		{
			os_free(cmdid);
		}
		break;

	case PINGRESP:// S->C
		ONENET_DEBUG("PINGRESP\r\n");
		s_onenet_tcp_state = ONENET_TCP_HEARTBEAR_SUCCESS;
		s_heartbeat_miss = 0;
		break;

	case ENCRYPTRESP:
		ONENET_DEBUG("ENCRYPTRESP\r\n");
#ifdef _ENCRYPT
		UnpackEncryptResp(pkt);
		edp_login();
#endif
		break;

	default:
		ONENET_DEBUG("ERROR MSG TYPE\r\n");
		break;
	}
}

static void ICACHE_FLASH_ATTR tcpclient_sent_cb(void *arg)
{
	s_send_data_ok = 1;
#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("tcpclient_sent_cb\r\n");
#endif
}
static void ICACHE_FLASH_ATTR tcpclient_discon_cb(void *arg)
{
#if 0
	if (s_reconnect_state == 1)
	{
		os_timer_disarm(&s_reconnect_timer);
		os_timer_setfn(&s_reconnect_timer, reconnect_check, 0);
		os_timer_arm(&s_reconnect_timer, 10,0);
	}
#endif
	ONENET_DEBUG("tcpclient_discon_cb\r\n");
	if (s_disconnect_evt == 1)
	{
		s_disconnect_evt = 0;
		return ;
	}
	reconnect_check(NULL);
}
static void ICACHE_FLASH_ATTR tcpclient_connect_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;

	espconn_regist_recvcb(pespconn, tcpclient_recv);
	espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
	ONENET_DEBUG("onenet_connect_cb\r\n");

#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		edp_encryptreq();
	}
	else
#endif
	{
		s_onenet_tcp_state = ONENET_TCP_LOGIN_REQUEST;
		onenet_login_check(NULL);
	}
}
static void ICACHE_FLASH_ATTR tcpclient_recon_cb(void *arg, sint8 errType)
{
	ONENET_DEBUG("tcpclient_recon_cb\r\n");
}
#ifdef ONENET_USE_DNS
static void ICACHE_FLASH_ATTR dns_check(void *timer_arg)
{
	espconn_gethostbyname(s_pespconn_tcp, s_tcp_domain, &s_server_ip, dns_found);
}
static void ICACHE_FLASH_ATTR dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;

	if (ipaddr == NULL)
	{
		ONENET_DEBUG("TCP DNS: Found, but got no ip, try to reconnect\r\n");
		os_timer_disarm(&s_dns_timer);
		os_timer_setfn(&s_dns_timer, (os_timer_func_t *)dns_check, 0);
		os_timer_arm(&s_dns_timer, 1000, 0);
		return;
	}

	ONENET_DEBUG("TCP DNS: found ip %d.%d.%d.%d\n",
	             *((uint8 *) &ipaddr->addr),
	             *((uint8 *) &ipaddr->addr + 1),
	             *((uint8 *) &ipaddr->addr + 2),
	             *((uint8 *) &ipaddr->addr + 3));

	//if (s_server_ip.addr == 0 && ipaddr->addr != 0)
	if (ipaddr->addr != 0)
	{
		s_server_ip.addr = ipaddr->addr;
		os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
		pespconn->proto.tcp->local_port = EDP_TCP_LOCAL_PORT;//espconn_port();
		pespconn->proto.tcp->remote_port = EDP_TCP_REMOTE_PORT;
		espconn_regist_connectcb(pespconn, tcpclient_connect_cb);
		espconn_regist_disconcb(pespconn, tcpclient_discon_cb);

		espconn_connect(pespconn);
	}
}
#endif
static void ICACHE_FLASH_ATTR reconnect_check(void *timer_arg)
{
	if (s_reconnect_state==0)
	{
		espconn_disconnect(s_pespconn_tcp);
		s_onenet_tcp_state = ONENET_TCP_BEGIN;
		s_reconnect_state = 1;
		ONENET_DEBUG("reconnect_check:disconnect...\n");
		
		os_timer_disarm(&s_reconnect_timer);
		os_timer_setfn(&s_reconnect_timer, reconnect_check, 0);
		os_timer_arm(&s_reconnect_timer, 10,0);
	}
	else if (s_reconnect_state == 1)
	{
		s_reconnect_state = 0;
		s_onenet_tcp_state = ONENET_TCP_LOGIN_REQUEST;
		espconn_regist_connectcb(s_pespconn_tcp, tcpclient_connect_cb);
		espconn_regist_disconcb(s_pespconn_tcp, tcpclient_discon_cb);
		espconn_connect(s_pespconn_tcp);
		ONENET_DEBUG("reconnect_check:connect...\n");
		return ;
	}
}
uint8 ICACHE_FLASH_ATTR onenet_tcpclient_reconnect(void)
{
	os_timer_disarm(&s_reconnect_timer);
	os_timer_setfn(&s_reconnect_timer, reconnect_check, 0);
	os_timer_arm(&s_reconnect_timer, 10,0);
	s_reconnect_state = 0;
	return true;
}
uint8 ICACHE_FLASH_ATTR onenet_tcpclient_send(uint8_t *buf, uint32_t len)
{
	if (s_pespconn_tcp == NULL || s_onenet_tcp_state < ONENET_TCP_LOGIN_SUCCESS)
	{
		return false;
	}
	espconn_send(s_pespconn_tcp, buf, len);
	return true;
}

uint32 ICACHE_FLASH_ATTR onenet_tcpclient_get_espconn(void)
{
	if (s_pespconn_tcp == 0)
	{
		return 0;
	}
	return (uint32)s_pespconn_tcp;
}

uint8 ICACHE_FLASH_ATTR onenet_tcpclient_get_send_state(void)
{
	return s_send_data_ok;
}

uint8 onenet_tcpclient_disconnect(void)
{
	espconn_disconnect(s_pespconn_tcp);
	return 0;
}

uint8 onenet_tcpclient_connect(void)
{
	espconn_connect(s_pespconn_tcp);
	return 0;
}

bool ICACHE_FLASH_ATTR onenet_tcpclient_create(uint8 encrypt)
{
	//如果设备还没有注册则不允许建立EDP协议连接
	if (s_onenet_cloud.register_flag != 1)
	{
		ONENET_DEBUG("TCP:Device Not Register!!!\r\n");
		return false;
	}

	s_is_encrypt = encrypt;

	if (s_pespconn_tcp != NULL)
	{
		return false;
	}
	s_pespconn_tcp = (struct espconn *)os_zalloc(sizeof(struct espconn));
	s_pespconn_tcp->type = ESPCONN_TCP;
	s_pespconn_tcp->state = ESPCONN_NONE;
	s_pespconn_tcp->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));

#ifdef ONENET_USE_DNS
	s_pespconn_tcp->proto.tcp->local_port = EDP_TCP_LOCAL_PORT;//espconn_port();
	s_pespconn_tcp->proto.tcp->remote_port = EDP_TCP_REMOTE_PORT;
	espconn_regist_connectcb(s_pespconn_tcp, tcpclient_connect_cb);
	espconn_regist_reconcb(s_pespconn_tcp, tcpclient_recon_cb);
	dns_check(NULL);
#else
	os_memcpy(s_pespconn_tcp->proto.tcp->remote_ip, s_tcp_server_ip, 4);
	s_pespconn_tcp->proto.tcp->local_port = EDP_TCP_LOCAL_PORT;//espconn_port();
	s_pespconn_tcp->proto.tcp->remote_port = EDP_TCP_REMOTE_PORT;
	espconn_regist_connectcb(s_pespconn_tcp, tcpclient_connect_cb);
	espconn_regist_disconcb(s_pespconn_tcp, tcpclient_discon_cb);

	espconn_connect(s_pespconn_tcp);
	ONENET_DEBUG("tcpclient connecting......\n");
#endif

	return true;
}
static void ICACHE_FLASH_ATTR disconnect_check(void *timer_arg)
{
	s_disconnect_evt = 0;
	s_onenet_tcp_state = ONENET_TCP_BEGIN;
	os_timer_disarm(&s_heartbeat_timer);
	os_timer_disarm(&s_login_timer);
	os_timer_disarm(&s_dns_timer);
	if (s_pespconn_tcp == NULL)
	{
		return ;
	}
	if (s_pespconn_tcp)
	{
		if (s_pespconn_tcp->proto.tcp)
		{
			os_free(s_pespconn_tcp->proto.tcp);
			s_pespconn_tcp->proto.tcp = 0;
		}
		os_free(s_pespconn_tcp);
		s_pespconn_tcp = 0;
	}
	ONENET_DEBUG("tcpclient deleted...\n");
}
bool ICACHE_FLASH_ATTR onenet_tcpclient_delete(void)
{
	if (s_pespconn_tcp == NULL)
	{
		return false;
	}
	if (s_pespconn_tcp)
	{
		espconn_disconnect(s_pespconn_tcp);
	}
	s_disconnect_evt = 1;
	os_timer_disarm(&s_disconnect_timer);
	os_timer_setfn(&s_disconnect_timer, disconnect_check, 0);
	os_timer_arm(&s_disconnect_timer, 10, 0);
	return true;
}

onenet_device_state_t ICACHE_FLASH_ATTR onenet_tcpclient_get_device_state(void)
{
	return s_onenet_tcp_state;
}
uint8 ICACHE_FLASH_ATTR onenet_tcpclient_get_state(void)
{
	if (s_pespconn_tcp == NULL)
	{
		return ;
	}
	return s_pespconn_tcp->state;
}

/******************************************************************************
*
*/
/*
 * 已注册的设备进行登录, 先要使用httpclient进行设备注册
 */
void ICACHE_FLASH_ATTR edp_login(void)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}

	uint8 authinfo[64];
	os_sprintf(authinfo, "{\"sn\":\"%s\"}", s_onenet_cloud.auth_info);
	if (s_onenet_cloud.auth_method == AUTHMETHOD_DEVID_APIKEY)
	{
		pkt = PacketConnect1(s_onenet_cloud.device_id, s_onenet_cloud.device_apikey);
	}
	else if (s_onenet_cloud.auth_method == AUTHMETHOD_USRID_AUTHINFO)
	{
		pkt = PacketConnect2(s_onenet_cloud.project_id, authinfo);
	}
#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		SymmEncrypt(pkt);
	}
#endif
	s_send_data_ok = 0;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);

#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("EDP:Loging\n");
#endif

	DeleteBuffer(&pkt);
}
// 发送二进制文件是要停止心跳包
void ICACHE_FLASH_ATTR edp_heartbeat_stop(void)
{
	s_heartbeat_onoff = 1;
}
void ICACHE_FLASH_ATTR edp_heartbeat_resume(void)
{
	s_heartbeat_onoff = 0;
}
void ICACHE_FLASH_ATTR edp_heartbeat(void)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}
	
	pkt = PacketPing();
#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		SymmEncrypt(pkt);
	}
#endif
	s_send_data_ok = 0;
	s_onenet_tcp_state = ONENET_TCP_HEARTBEAT_REQUEST;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);

#ifdef GLOBAL_DEBUG
	ONENET_DEBUG("EDP:Heartbeat\n");
#endif

	DeleteBuffer(&pkt);
}
void ICACHE_FLASH_ATTR edp_pushdata(const char *dst_devid, uint8 *data, int data_len)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}
	
	pkt = PacketPushdata(dst_devid, data, data_len);
#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		SymmEncrypt(pkt);
	}
#endif
	s_send_data_ok = 0;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);

#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("EDP:Pushdata\n");
#endif

	DeleteBuffer(&pkt);
}
// edp_savedata(5,"777395","{"ds_id":"color_red","datastreams":["red","green"]}","0123456789",10,",;sz,56;dog")
void ICACHE_FLASH_ATTR edp_savedata(uint8 datatype, uint8 *dst_devid,
	uint8 *json_str, uint8 *bin_data, int bin_len, char *field)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}
	
	switch (datatype)
	{
	case SAVEDATA_DATATYPE_FIELDSTRING://fieldstring
		pkt = PacketSaveField(dst_devid, field);
		break;
	case SAVEDATA_DATATYPE_JSON3:
	case SAVEDATA_DATATYPE_JSON2:
	case SAVEDATA_DATATYPE_JSON1:
		pkt = PacketSaveJson(dst_devid, json_str);
		break;
	case SAVEDATA_DATATYPE_BIN:
		pkt = PacketSavedataBin(dst_devid, json_str, bin_data, bin_len);
		break;
	default:
		break;
	}
#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		SymmEncrypt(pkt);
	}
#endif
	s_send_data_ok = 0;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);
#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("EDP:Savedata\n");
#endif
	DeleteBuffer(&pkt);
}
void ICACHE_FLASH_ATTR edp_cmdresp(uint8 *cmdid, uint8 *cmdbody, int cmdbody_len)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}
	
	pkt = PacketCmdresp(cmdid, cmdbody, cmdbody_len);
#ifdef _ENCRYPT
	if (s_is_encrypt)
	{
		SymmEncrypt(pkt);
	}
#endif
	s_send_data_ok = 0;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);

#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("EDP:CmdResp\n");
#endif

	DeleteBuffer(&pkt);
}
#ifdef _ENCRYPT
void ICACHE_FLASH_ATTR edp_encryptreq(void)
{
	EdpPacket *pkt;
	if (s_pespconn_tcp==NULL)
	{
		return ;
	}
	
	pkt = PacketEncryptReq(kTypeAes);
	s_send_data_ok = 0;
	espconn_send(s_pespconn_tcp, pkt->_data, pkt->_write_pos);

#ifdef GLOBAL_DEBUG_ON
	ONENET_DEBUG("EDP:EncryptResp\n");
#endif

	DeleteBuffer(&pkt);
}
#endif

/*******************************************************************************
* onenet register
*/
void ICACHE_FLASH_ATTR onenet_pushdata_handle_register(pushdata_handle_callback_t func)
{
	s_pushdata_handle_callback = func;
}
void ICACHE_FLASH_ATTR onenet_saveack_handle_register(saveack_handle_callback_t func)
{
	s_saveack_handle_callback = func;
}
void ICACHE_FLASH_ATTR onenet_cmdreq_handle_register(cmdreq_handle_callback_t func)
{
	s_cmdreq_handle_callback = func;
}
void ICACHE_FLASH_ATTR onenet_savedatabin_handle_register(savedatabin_handle_callback_t func)
{
	s_savedatabin_handle_callback = func;
}
void ICACHE_FLASH_ATTR onenet_savedatabin_field_handle_register(savedatabin_field_handle_callback_t func)
{
	s_savedatabin_field_handle_callback = func;
}
void ICACHE_FLASH_ATTR onenet_savedata_json_handle_register(savedata_json_handle_callback_t func)
{
	s_savedata_json_handle_callback = func;
}


/******************************************************************************
* "192.168.123.234"
*/
uint8 ICACHE_FLASH_ATTR onenet_tcpclient_set_server_ip(uint8 *buf_ip)
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
	s_tcp_server_ip[0]=a;
	s_tcp_server_ip[1]=b;
	s_tcp_server_ip[2]=c;
	s_tcp_server_ip[3]=d;
	return true;
}
uint8 ICACHE_FLASH_ATTR onenet_tcpclient_set_server_domain(uint8 *buf_domain)
{
	os_strncpy(s_tcp_domain, buf_domain, 64);
	return true;
}

