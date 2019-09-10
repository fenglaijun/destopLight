/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/1/23, v1.0 create this file.
*******************************************************************************/

#include "osapi.h"
#include "user_config.h"
#include "user_interface.h"
#include "change_ssid.h"
#include "uart.h"

#include "onenet_tcp.h"
#include "onenet_http.h"
#include "onenet_config.h"

void ICACHE_FLASH_ATTR system_init_done()
{
    os_printf("\r\nSDK version:%s\r\n", system_get_sdk_version());
    os_printf("\r\nready\r\n");
    
    fsm_init();
}
void user_rf_pre_init(void)
{
	// Warning: IF YOU DON'T KNOW WHAT YOU ARE DOING, DON'T TOUCH THESE CODE

	// Control RF_CAL by esp_init_data_default.bin(0~127byte) 108 byte when wakeup
	// Will low current
	// system_phy_set_rfoption(0)£»

	// Process RF_CAL when wakeup.
	// Will high current
	system_phy_set_rfoption(1);

	// Set Wi-Fi Tx Power, Unit: 0.25dBm, Range: [0, 82]
	system_phy_set_max_tpw(82);
}


void user_init(void)
{
	// ok, Init platform
	uart_init(BIT_RATE_74880, BIT_RATE_74880);
	
	// Set Wi-Fi mode
	wifi_set_opmode(0x03);
	change_ssid();
	wifi_set_opmode(0x01);

	// Set system initialize done call back
	system_init_done_cb(system_init_done);
}
