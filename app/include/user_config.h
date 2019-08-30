#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ICACHE_STORE_TYPEDEF_ATTR __attribute__((aligned(4),packed))
#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR

#define USE_OPTIMIZE_PRINTF

#define SOFTAP_CHANGESSID 	1
#define SOFTAP_PREFIX 		"ONENETLIGHT"
#define WDT_CLEAR()			WRITE_PERI_REG( 0x60000914, 0x73 )

#define PIN_DI 				13
#define PIN_DCKI 			15

//#define MY9291_DEBUG_ON
#define NETWORK_DEBUG_ON
#define DRIVER_DEBUG_ON
#define ONENET_APP_DEBUG_ON
#define UPDATE_DEBUG_ON


#endif

