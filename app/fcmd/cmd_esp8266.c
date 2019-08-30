#include "osapi.h"
#include "at_custom.h"
#include "user_interface.h"

#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "espconn.h"

#include "cmd_esp8266.h"

void ICACHE_FLASH_ATTR get_ip_mac(void)
{
	char hwaddr[6];
	struct ip_info ipconfig;

	wifi_get_ip_info(SOFTAP_IF, &ipconfig);
	wifi_get_macaddr(SOFTAP_IF, hwaddr);
	os_printf("soft-AP:" MACSTR " " IPSTR, MAC2STR(hwaddr), IP2STR(&ipconfig.ip));

	wifi_get_ip_info(STATION_IF, &ipconfig);
	wifi_get_macaddr(STATION_IF, hwaddr);
	os_printf("\nstation:" MACSTR " " IPSTR "\n", MAC2STR(hwaddr), IP2STR(&ipconfig.ip));
}
/*
 * 设置station_config,将会自动连接设置的ap
 */
void ICACHE_FLASH_ATTR
set_station_config(char *ssid, char *password)
{
	int len;

	// Wifi configuration
	struct station_config stationConf;

	//need not mac address
	stationConf.bssid_set = 0;
	os_memset(&stationConf, 0, sizeof(struct station_config));

	//Set ap settings, 设置要连接的ap
	len = os_strlen(ssid);
	os_memcpy(&stationConf.ssid, ssid, len > 32 ? 32 : len);

	len = os_strlen(password);
	os_memcpy(&stationConf.password, password, len > 64 ? 64 : len);
	wifi_station_set_config(&stationConf);
}

int _atoi(const char *s)
{
  int n;
  unsigned char sign = 0;

  while (isspace(*s))
  {
    s++;
  }

  if (*s == '-')
  {
    sign = 1;
    s++;
  }
  else if (*s == '+')
  {
    s++;
  }

  n=0;

  while ('0' <= *s && *s <= '9')
  {
    n = n * 10 + *s++ - '0';
  }

  return sign ? -n : n;
}


/******************************************************************************
 * fsm_os
 */
#include "fsm.h"

static os_timer_t fsm_dispatch_timer;
static os_timer_t fsm_tick_timer;

static void ICACHE_FLASH_ATTR
fsm_dispatch_func(void *timer_arg)
{
	os_dispatch();
}

static void ICACHE_FLASH_ATTR
fsm_tick_func(void *timer_arg)
{
	os_timer_update();
}

static void ICACHE_FLASH_ATTR
fsm_hook(void)
{
	os_timer_disarm(&fsm_dispatch_timer);
	os_timer_setfn(&fsm_dispatch_timer, (os_timer_func_t *)fsm_dispatch_func, 0);
	os_timer_arm(&fsm_dispatch_timer, 0, 0);
}

void ICACHE_FLASH_ATTR
fsm_init(void)
{
	os_register_hook(fsm_hook);

	//fsm 1ms tick
	os_timer_disarm(&fsm_tick_timer);
	os_timer_setfn(&fsm_tick_timer, (os_timer_func_t *)fsm_tick_func, 0);
	os_timer_arm(&fsm_tick_timer, 1, 1);

	os_init_tasks();
}


/*****************************************************************************
 * sntp(简单网络时间协议)测试
 * now sdk support SNTP_MAX_SERVERS = 3
 * 202.120.2.101 (上海交通大学网络中心NTP服务器地址）
 * 210.72.145.44 (国家授时中心服务器)
 */
sint8 sntp_get_timezone(void);
bool sntp_set_timezone(sint8 timezone);
void sntp_init(void);
void sntp_stop(void);
void sntp_setserver(unsigned char idx, ip_addr_t *addr);
void sntp_setservername(unsigned char idx, char *server);//通过域名设置授时服务器

void ICACHE_FLASH_ATTR
get_real_time(void)
{
	//查询当前距离基准时间（1970.01.01 00 ：00：00 GMT + 8）的时间戳，单位：秒
	uint32 t = sntp_get_current_timestamp();
	os_printf("sntp:%s\n", sntp_get_real_time(t));
}

void ICACHE_FLASH_ATTR
sntp_init_test(void)
{
	ip_addr_t ip;

	sntp_init();

	sntp_set_timezone(8);

	IP4_ADDR(&ip, 202, 120, 2, 101);
	sntp_setserver(0, &ip);

	IP4_ADDR(&ip, 210, 72, 145, 44);
	sntp_setserver(1, &ip);
}


