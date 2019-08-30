/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/

#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "smartconfig.h"
#include "smartlink.h"

// Select smartconfig method.
#define USE_ESPTOUCH 1

LOCAL os_timer_t smartlink_timer;
LOCAL smartlink_success_callback_t smartlink_success_callback_handle = NULL;
LOCAL smartlink_timeout_callback_t smartlink_timeout_callback_handle = NULL;
LOCAL smartlink_getting_callback_t smartlink_getting_callback_handle = NULL;
LOCAL smartlink_linking_callback_t smartlink_linking_callback_handle = NULL;
LOCAL smartlink_finding_callback_t smartlink_finding_callback_handle = NULL;

void ICACHE_FLASH_ATTR
smartlink_timeout(void)
{
    if (smartlink_timeout_callback_handle)
    {
        smartlink_timeout_callback_handle(NULL);
    }
    os_timer_disarm(&smartlink_timer);
}

LOCAL void ICACHE_FLASH_ATTR
smartlink_done(sc_status status, void *pdata)
{
    switch (status) {
    case SC_STATUS_WAIT:
        os_printf("SC_STATUS_WAIT\n");
        break;
    case SC_STATUS_FIND_CHANNEL:
        os_printf("SC_STATUS_FIND_CHANNEL\n");
        break;
    case SC_STATUS_GETTING_SSID_PSWD:
        os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
        if (smartlink_getting_callback_handle)
        {
            smartlink_getting_callback_handle(pdata);
        }
        break;
    case SC_STATUS_LINK:
        os_printf("SC_STATUS_LINK\n");
        if (smartlink_linking_callback_handle)
        {
            smartlink_linking_callback_handle(pdata);
        }
        struct station_config *sta_conf = pdata;

        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();
        break;
    case SC_STATUS_LINK_OVER:
        os_printf("SC_STATUS_LINK_OVER\n");

        if (pdata != NULL)
        {
            uint8 phone_ip[4] = {0};

            os_memcpy(phone_ip, (uint8*)pdata, 4);
            os_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
        }
        if (smartlink_success_callback_handle)
        {
            smartlink_success_callback_handle(pdata);
        }
        smartconfig_stop();
        break;
    }

}

void ICACHE_FLASH_ATTR
smartlink_start(void)
{
    wifi_set_opmode(STATION_MODE);
	smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS

#if defined(USE_ESPTOUCH)
    // USE ESPTOUCH
    smartconfig_start(smartlink_done);
#endif

#if defined(USE_AIRKISS)
    // USE AIRKISS
    smartconfig_start(smartlink_done);
#endif

    os_timer_disarm(&smartlink_timer);
    // os_timer_setfn(&smartlink_timer, (os_timer_func_t *)smartlink_timeout, 1);
    // os_timer_arm(&smartlink_timer, 1000 * 1000, 1);
}

void ICACHE_FLASH_ATTR
smartlink_stop(void)
{
    os_timer_disarm(&smartlink_timer);

#if defined(USE_ESPTOUCH)
    // USE ESPTOUCH
    smartconfig_stop();
#endif

#if defined(USE_AIRKISS)
    // USE AIRKISS
    smartconfig_stop();
#endif
}

void ICACHE_FLASH_ATTR
smartlink_success_callback_register(smartlink_success_callback_t smartlink_success_callback)
{
    smartlink_success_callback_handle = smartlink_success_callback;
}

void ICACHE_FLASH_ATTR
smartlink_timeout_callback_register(smartlink_timeout_callback_t smartlink_timeout_callback)
{
    smartlink_timeout_callback_handle = smartlink_timeout_callback;
}

void ICACHE_FLASH_ATTR
smartlink_getting_callback_register(smartlink_getting_callback_t smartlink_getting_callback)
{
    smartlink_getting_callback_handle = smartlink_getting_callback;
}

void ICACHE_FLASH_ATTR
smartlink_linking_callback_register(smartlink_linking_callback_t smartlink_linking_callback)
{
    smartlink_linking_callback_handle = smartlink_linking_callback;
}

void ICACHE_FLASH_ATTR
smartlink_finding_callback_register(smartlink_finding_callback_t smartlink_finding_callback)
{
    smartlink_finding_callback_handle = smartlink_finding_callback;
}