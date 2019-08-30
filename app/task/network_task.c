#include "fsm.h"

#include "osapi.h"
#include "user_interface.h"
#include "string.h"

#include "network_task.h"

#include "mem.h"
#include "cmd_esp8266.h"

#include "driver_task.h"
#include "onenet_app.h"
#include "onenet_tcp.h"
#include "onenet_http.h"
#include "onenet_config.h"


#define USER_MASTER_APIKEY  "QcWE5p7=kZA6aLhxqdCWf7cqXXo="
#define USER_PRODUCT_ID "526934531"

static uint16_t s_disconnect_tick = 0;
static uint8_t s_network_state = NW_STATE_IDLE;
static uint8_t s_smartlink_state = SMART_STATE_SUCCESS;
//static uint8_t s_softap_config_state;
static uint8_t s_update_system;

uint8_t ICACHE_FLASH_ATTR network_smartlink_get_state(void)
{
	return s_smartlink_state;
}
static void ICACHE_FLASH_ATTR network_smartlink_success(void *pdata)
{
	os_post_message(NETWORK_ID, NW_SMART_OK_EVT, 0);
}

static void ICACHE_FLASH_ATTR network_smartlink_timeout(void *pdata)
{
	os_post_message(NETWORK_ID, NW_SMART_TIMEOUT_EVT, 0);
}
static void ICACHE_FLASH_ATTR network_smartlink_finding(void *pdata)
{
	s_smartlink_state = SMART_STATE_FIND;
	onenet_app_set_smart_effect(0);
}
static void ICACHE_FLASH_ATTR network_smartlink_getting(void *pdata)
{
	s_smartlink_state = SMART_STATE_GETTING;
	onenet_app_set_smart_effect(1);
}
static void ICACHE_FLASH_ATTR network_smartlink_link(void *pdata)
{
	s_smartlink_state = SMART_STATE_LINK;
	onenet_app_set_smart_effect(2);
}
uint8_t ICACHE_FLASH_ATTR network_get_state(void)
{
	return s_network_state;
}


/******************************************************************************
*  网络状态机
*/
void network_actor(event_t *e);
void network_idle(event_t *e);
void network_smart(event_t *e);
//void network_ap(event_t *e);
//void network_apsta(event_t *e);
void network_sta(event_t *e);

void ICACHE_FLASH_ATTR network_actor(event_t *e)
{
	switch (e->sig)
	{
	case NW_TRAN_AP_EVT:
		wifi_set_opmode(SOFTAP_MODE);//softAP配网方式
//		os_stm_tran(network_ap);
		break;
		
	case NW_TRAN_IDLE_EVT:
		os_stm_tran(network_idle);
		break;
		
	case STM_ENTRY_SIG:
		// smartlink成功和超时的回调
		os_printf("smartlink process\n");
		smartlink_success_callback_register(network_smartlink_success);
		smartlink_timeout_callback_register(network_smartlink_timeout);
		smartlink_getting_callback_register(network_smartlink_getting);
		smartlink_linking_callback_register(network_smartlink_link);
		smartlink_finding_callback_register(network_smartlink_finding);
		os_post_message(NETWORK_ID, NW_TRAN_IDLE_EVT, 0);
		break;

	case STM_EXIT_SIG:
		break;
	}
}
// 空闲，断网，错误等状态
// 该状态有15s的时间来连上路由器，15s后将会启动smartlink
void ICACHE_FLASH_ATTR network_idle(event_t *e)
{
	switch (e->sig)
	{
	case NW_TRAN_SMART_EVT:
		os_stm_tran(network_smart);
		break;
		
	case NW_TICK_EVT:

//		if (SOFTAP_MODE == wifi_get_opmode())
//		{
////			os_stm_tran(network_ap);
//		}
//		else
		if ((STATION_MODE == wifi_get_opmode()) || (STATIONAP_MODE == wifi_get_opmode()))
		{
			struct ip_info ipconfig;
			wifi_get_ip_info(STATION_IF, &ipconfig);
			if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0)
			{
				// 获取到ip了
				if (STATION_MODE == wifi_get_opmode())
				{
					os_stm_tran(network_sta);
				}
//				else if (STATIONAP_MODE == wifi_get_opmode())
//				{
//					os_stm_tran(network_apsta);
//				}
			}
			else
			{			
				s_disconnect_tick++;
			}
		}
		break;

	case STM_ENTRY_SIG:
		s_disconnect_tick = 0;
		NETWORK_DEBUG("\nnetwork_idle:entry\n");
		s_network_state = NW_STATE_IDLE;
		os_reload_timer_set(NETWORK_ID, NW_TICK_EVT, 0, 100);
		break;

	case STM_EXIT_SIG:
		os_timer_del(NETWORK_ID, NW_TICK_EVT);
		NETWORK_DEBUG("\nnetwork_idle:exit\n");
		break;
	}
}
// smart状态
void ICACHE_FLASH_ATTR network_smart(event_t *e)
{
	switch (e->sig)
	{
	case NW_SMART_OK_EVT:
		os_stm_tran(network_sta);
		break;
		
	case NW_SMART_TIMEOUT_EVT:
		wifi_set_opmode(STATIONAP_MODE);
		os_stm_tran(network_idle);//smartlink失败，跳到idle状态
		break;
		
	case STM_ENTRY_SIG:
		smartlink_start();
		NETWORK_DEBUG("\nnetwork_smart:entry\n");
		s_network_state = NW_STATE_SMART;
		onenet_app_smart_timer_start();
		break;

	case STM_EXIT_SIG:
		smartlink_stop();
		NETWORK_DEBUG("\nnetwork_smart:exit\n");

//		onenet_app_apply_settings();
//		onenet_app_smart_timer_stop();
		break;
	}
}
// sta状态
void ICACHE_FLASH_ATTR network_sta(event_t *e)
{
	switch (e->sig)
	{
	case NW_UPGRADE_EVT://程序升级
		if (s_update_system == 0)
		{
			s_update_system = 1;
			NETWORK_DEBUG("Gizwits APP: OTA update\r\n");
			update_system();
		}
		break;

	case NW_CHECK_REGISTER_EVT:
//		if (s_register_state == 0)
//		{
//			// device already register & http check device ok
//			if (onenet_is_register_flag_set() && onenet_get_device_query_result())
//			{
//				onenet_httpclient_delete();
//
//				onenet_param_load();
//				onenet_device_info_print();
//
//				onenet_callback_init();
//				onenet_tcpclient_create(0);//no encrypt
//
//				s_register_state = 1;
//			}
//		}
//		else if (s_register_state == 1)
//		{
//			if (onenet_tcpclient_get_device_state() >= ONENET_TCP_LOGIN_SUCCESS)
//			{
//				onenet_app_report_settings();
//				os_timer_del(NETWORK_ID, NW_CHECK_REGISTER_EVT);
//			}
//		}
		break;

	case STM_ENTRY_SIG:
		NETWORK_DEBUG("network_sta: set master_apikey\n");
		uint8 blank[16];
		os_memset(blank, 0, sizeof(blank));
		onenet_set_register_flag(1);
		onenet_set_device_id(blank, sizeof(blank));
		onenet_set_device_id(USER_PRODUCT_ID, os_strlen(USER_PRODUCT_ID));
		onenet_set_device_apikey(USER_MASTER_APIKEY, os_strlen(USER_MASTER_APIKEY));
		onenet_set_authmethod(AUTHMETHOD_DEVID_APIKEY);
		onenet_callback_init();
		onenet_tcpclient_create(0);//no encrypt
		os_reload_timer_set(NETWORK_ID, NW_CHECK_REGISTER_EVT, NULL, 1000);
//		s_register_state = 0;

		NETWORK_DEBUG("\nnetwork_sta:entry\n");
//		s_network_state = NW_STATE_STA;
		break;

	case STM_EXIT_SIG:
		os_timer_del(NETWORK_ID, NW_CHECK_REGISTER_EVT);
		onenet_tcpclient_delete();
		NETWORK_DEBUG("\nnetwork_sta:exit\n");
		break;
	}
}

