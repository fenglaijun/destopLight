/******************************************************************************
 * Copyright 2015-2018 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/3/06, v1.0 create this file.
*******************************************************************************/
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "upgrade.h"
#include "update.h"

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate\r\n\
Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

#define REMOTE_HOST_URI "ota.ai-thinker.com"
#define REMOTE_HOST_SUB_URI "/otabin/"
/**/


struct espconn *pespconn = NULL;
struct upgrade_server_info *upServer = NULL;

static os_timer_t at_delay_check;
static struct espconn *pTcpServer = NULL;
static ip_addr_t host_ip;
static uint8_t s_update_state;
/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
update_rsp(void *arg)
{
	struct upgrade_server_info *server = arg;

	system_set_os_print(1);

	if (server->upgrade_flag == true)
	{
		UPDATE_DEBUG("device_upgrade_success\r\n");
		UPDATE_DEBUG("OK\r\n");
		system_upgrade_reboot();
	}
	else
	{
		UPDATE_DEBUG("device_upgrade_failed\r\n");
		UPDATE_DEBUG("ERROR\r\n");
	}
	s_update_state = 0;
	os_free(server->url);
	server->url = NULL;
	os_free(server);
	server = NULL;
}
/**
  * @brief  Tcp client disconnect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
update_discon_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t idTemp = 0;

	if (pespconn->proto.tcp != NULL)
	{
		os_free(pespconn->proto.tcp);
	}
	if (pespconn != NULL)
	{
		os_free(pespconn);
	}

	UPDATE_DEBUG("disconnect\r\n");

	if (system_upgrade_start(upServer) == false)
	{
		UPDATE_DEBUG("ERROR\r\n");
	}
	else
	{
		UPDATE_DEBUG("UPDATE STATUS:4\r\n");
	}
}

/**
  * @brief  Udp server receive data callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
LOCAL void ICACHE_FLASH_ATTR
update_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;
	char temp[32] = {0};
	uint8_t user_bin[12] = {0};
	uint8_t i = 0;

	os_timer_disarm(&at_delay_check);
	UPDATE_DEBUG("UPDATE STATUS:3\r\n");

	upServer = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));

	upServer->upgrade_version[5] = '\0';

	upServer->pespconn = pespconn;

	os_memcpy(upServer->ip, pespconn->proto.tcp->remote_ip, 4);

	upServer->port = 80;

	upServer->check_cb = update_rsp;
	upServer->check_times = 60000;

	if (upServer->url == NULL)
	{
		upServer->url = (uint8 *) os_zalloc(1024);
	}

	if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1)
	{
		os_memcpy(user_bin, "user2.bin", 10);
	}
	else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2)
	{
		os_memcpy(user_bin, "user1.bin", 10);
	}

	os_sprintf(upServer->url,
	           "GET "REMOTE_HOST_SUB_URI"?action=download_rom&version=%s&filename=%s HTTP/1.1\r\nHost: "REMOTE_HOST_URI"\r\n"pheadbuffer"",
	           "20150419141355", user_bin);

	UPDATE_DEBUG("system_upgrade_start:%s\n", upServer->url);
}

LOCAL void ICACHE_FLASH_ATTR
update_wait(void *arg)
{
	struct espconn *pespconn = arg;
	os_timer_disarm(&at_delay_check);
	if (pespconn != NULL)
	{
		espconn_disconnect(pespconn);
	}
	else
	{
		UPDATE_DEBUG("ERROR\r\n");
	}
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
update_sent_cb(void *arg)
{
	struct espconn *pespconn = arg;
	os_timer_disarm(&at_delay_check);
	os_timer_setfn(&at_delay_check, (os_timer_func_t *)update_wait, pespconn);
	os_timer_arm(&at_delay_check, 5000, 0);
	UPDATE_DEBUG("update_sent_cb\r\n");
}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
update_connect_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	uint8_t user_bin[9] = {0};
	char *temp = NULL;

	UPDATE_DEBUG("UPDATE STATUS:2\r\n");


	espconn_regist_disconcb(pespconn, update_discon_cb);
	espconn_regist_recvcb(pespconn, update_recv);////////
	espconn_regist_sentcb(pespconn, update_sent_cb);

	temp = (uint8 *) os_zalloc(512);

	os_sprintf(temp, "GET "REMOTE_HOST_SUB_URI"?is_format_simple=true HTTP/1.0\r\nHost: "REMOTE_HOST_URI"\r\n"pheadbuffer"");
	UPDATE_DEBUG("update_connect_cb:%s\n", temp);

	espconn_sent(pespconn, temp, os_strlen(temp));
	os_free(temp);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
static void ICACHE_FLASH_ATTR
update_recon_cb(void *arg, sint8 errType)
{
	struct espconn *pespconn = (struct espconn *)arg;

	UPDATE_DEBUG("ERROR\r\n");
	if (pespconn->proto.tcp != NULL)
	{
		os_free(pespconn->proto.tcp);
	}
	os_free(pespconn);
	UPDATE_DEBUG("disconnect\r\n");

	if (upServer != NULL)
	{
		os_free(upServer);
		upServer = NULL;
	}
	UPDATE_DEBUG("ERROR\r\n");

}

/******************************************************************************
 * FunctionName : update_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
update_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *) arg;
//  char temp[32];

	if (ipaddr == NULL)
	{
		UPDATE_DEBUG("ERROR\r\n");
		return;
	}
	UPDATE_DEBUG("UPDATE STATUS:1\r\n");


	if (host_ip.addr == 0 && ipaddr->addr != 0)
	{
		UPDATE_DEBUG("dns_found  %d.%d.%d.%d\n",
				          *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
				          *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
		if (pespconn->type == ESPCONN_TCP)
		{
			os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
			espconn_regist_connectcb(pespconn, update_connect_cb);
			espconn_regist_reconcb(pespconn, update_recon_cb);
			espconn_connect(pespconn);
		}
	}
}

void ICACHE_FLASH_ATTR
update_system()
{
	os_printf("update_system \n\n");
	host_ip.addr = ipaddr_addr("192.168.2.105");
	uint8 server_ip[4];
	server_ip[3] = host_ip.addr >> 24;		//1
	server_ip[2] = (host_ip.addr & 0x00FF0000) >> 16;	//2
	server_ip[1] = (host_ip.addr & 0x0000FF00) >> 8;	//168
	server_ip[0] = host_ip.addr & 0xFF;	//192
	os_printf("update_system 11111\n\n");
	const char* file;
	//获取系统的目前加载的是哪个bin文件
	uint8_t userBin = system_upgrade_userbin_check();


	switch (userBin) {

	//如果检查当前的是处于user1的加载文件，那么拉取的就是user2.bin
	case UPGRADE_FW_BIN1:
		file = "user2.1024.new.2.bin";
		break;

		//如果检查当前的是处于user2的加载文件，那么拉取的就是user1.bin
	case UPGRADE_FW_BIN2:
		file = "user1.1024.new.2.bin";
		break;

		//如果检查都不是，可能此刻不是OTA的bin固件
	default:
		os_printf("Fail read system_upgrade_userbin_check! \n\n");
		return;
	}

	struct upgrade_server_info* update =
			(struct upgrade_server_info *) os_zalloc(
					sizeof(struct upgrade_server_info));
	update->pespconn = (struct espconn *) os_zalloc(sizeof(struct espconn));
	//设置服务器地址
	os_memcpy(update->ip, server_ip, 4);
	os_printf("Http Server Address:%d.%d.%d.%d", IP2STR(update->ip));
	//设置服务器端口
	update->port = 8080;
	//设置OTA回调函数
	update->check_cb = update_rsp;
	//设置定时回调时间
	update->check_times = 10000;
	//打印下請求地址
	os_printf("Http Server Address:%d.%d.%d.%d ,port: %d,filePath: %s,fileName: %s \n",
			IP2STR(update->ip), update->port, REMOTE_HOST_SUB_URI, file);
	//从 4M *1024 =4096申请内存
	update->url = (uint8 *)os_zalloc(4096);



	//拼接完整的 URL去请求服务器
	os_sprintf((char*) update->url, "GET /%s%s HTTP/1.1\r\n"
			"Host: "IPSTR":%d\r\n"
	"Connection: keep-alive\r\n"
	"\r\n", REMOTE_HOST_SUB_URI, file, IP2STR(update->ip), update->port);

	if (system_upgrade_start(update) == false) {
		os_printf(" Could not start upgrade\n");
		//释放资源
		os_free(update->pespconn);
		os_free(update->url);
		os_free(update);
	} else {
		os_printf(" Upgrading...\n");
	}
}

uint8_t ICACHE_FLASH_ATTR
update_get_state(void)
{
	return s_update_state;
}


