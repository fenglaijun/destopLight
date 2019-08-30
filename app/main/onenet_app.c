/******************************************************************************
 * Copyright 2015 Vowstar Co., Ltd.
 *
 * FileName: gizwits_app.c
 *
 * Description: Simulate MCU in ESP8266 gokit.
 *
 * Modification history:
 *     2015/06/09, v1.0 create this file.
*******************************************************************************/
#include "user_config.h"
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"
#include "update.h"
#include "softpwm.h"
#include "softpwm_light.h"
#include "onenet_app.h"
#include "onenet_tcp.h"
#include "onenet_http.h"
#include "onenet_config.h"
#include "fsm_event.h"


LOCAL os_timer_t onenet_app_smart_timer;
LOCAL uint8_t smart_effect = 0;

static int s_manufacture_mode=0;
static int s_manufacture_timeout=0;

float ICACHE_FLASH_ATTR fast_log2(float val)
{
	int * const  exp_ptr = (int *) (&val);
	int          x = *exp_ptr;
	const int    log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;
	return (val + log_2);
}


void manufacture_control(uint8_t state)
{
	if (state == 0)
	{
		s_manufacture_mode = 0;
		s_manufacture_timeout = 0;
	}
	else
	{
		s_manufacture_mode = 1;
	}
}

void ICACHE_FLASH_ATTR onenet_app_set_smart_effect(uint8_t effect)
{
	smart_effect = effect;
}

void ICACHE_FLASH_ATTR onenet_app_smart_timer_tick(void *p)
{
	//os_timer_disarm(&onenet_app_smart_timer);
	//os_timer_setfn(&onenet_app_smart_timer, (os_timer_func_t *)onenet_app_smart_timer_tick, NULL);
	//os_timer_arm(&onenet_app_smart_timer, 20, 1);
}

void ICACHE_FLASH_ATTR onenet_app_smart_timer_start(void)
{
	os_timer_disarm(&onenet_app_smart_timer);
	os_timer_setfn(&onenet_app_smart_timer, (os_timer_func_t *)onenet_app_smart_timer_tick, NULL);
	os_timer_arm(&onenet_app_smart_timer, 20, 1);
}
void ICACHE_FLASH_ATTR onenet_app_smart_timer_stop(void)
{
	os_timer_disarm(&onenet_app_smart_timer);
}



/******************************************************************************
*  handle callback
*/
static void ICACHE_FLASH_ATTR pushdata_handle(uint8 *addr, int addrlen, uint8 *data, int datalen)
{
#ifdef GLOBAL_DEBUG
	print_bufc(addr, addrlen);
	os_printf(",");
	print_bufc(data, datalen);
	os_printf("\r\n");
#endif
}
/*
90 56 80 00 53
7b 22 65 72 72 6e 6f 22 3a 30 2c 22 65 72 72 6f 72 22 3a 22 73 75 63 63 22 2c 22
69 6e 64 65 78 22 3a 22 31 30 38 33 31 30 38 5f 31 34 36 30 33 30 30 38 32 31 33
35 38 5f 62 69 6e 31 22 2c 22 74 6f 6b 65 6e 22 3a 22 30 30 31 35 36 32 65 35 22
7d 0a
*/
static void ICACHE_FLASH_ATTR saveack_handle(char *jsonstr, int len)
{
#ifdef GLOBAL_DEBUG
	os_printf("saveack_handle:%d,", len);
	print_bufc(jsonstr, len);
	os_printf("\r\n");
#endif
}
static void ICACHE_FLASH_ATTR cmdreq_handle(uint8 *cmdid, uint16 cmdid_len, uint8 *cmdbody, int cmdbody_len)
{
	os_printf("+IOTCMD=%d,", cmdbody_len);
	uart0_tx_buffer(cmdbody, cmdbody_len);
	os_printf("\r\n");

	// r g b w s
	// +IOTCMD=5,r:220
	uint8_t buf[32];
	os_memset(buf, 0, 32);
	int value;
//	mcu_attr_flags_t *flag = (mcu_attr_flags_t *)(&buf[1]);
//	mcu_status_t *status = (mcu_status_t *)(&buf[2]);
	char *ch = cmdbody;
	if (ch[1] != ':')
	{
		os_printf("error,%s,%d\n", __FILE__,__LINE__);
		return ;
	}
	value = _atoi(ch+2);
//	switch (ch[0])
//	{
//	case 'r':
//		flag->setting_color_r = 1;
//		status->color_r = value;
//		break;
//	case 'g':
//		flag->setting_color_g = 1;
//		status->color_g = value;
//		break;
//	case 'b':
//		flag->setting_color_b = 1;
//		status->color_b = value;
//		break;
//	case 'w':
//		flag->setting_brightness = 1;
//		status->brightness = value;
//		break;
//	case 's':
//		flag->setting_on_off = 1;
//		status->on_off = value;
//		break;
//	default:
//		break;
//	}
}
static void ICACHE_FLASH_ATTR savedatabin_handle(uint8 *bindata, uint32 len)
{
#ifdef GLOBAL_DEBUG
	os_printf("savedatabin_handle:");
	print_buf(bindata, len);
	os_printf("\r\n");
#endif
}
static void ICACHE_FLASH_ATTR savedatabin_field_handle(char *filedstr, uint32 len)
{
#ifdef GLOBAL_DEBUG
	os_printf("savedatabin_field_handle:");
	print_bufc(filedstr, len);
	os_printf("\r\n");
#endif
}
static void ICACHE_FLASH_ATTR savedata_json_handle(char *json_str)
{
#ifdef GLOBAL_DEBUG
	os_printf("savedata_json_handle:");
	print_bufc(json_str, os_strlen(json_str));
	os_printf("\r\n");
#endif
}
void ICACHE_FLASH_ATTR onenet_callback_init(void)
{
	onenet_pushdata_handle_register(pushdata_handle);
	onenet_saveack_handle_register(saveack_handle);
	onenet_cmdreq_handle_register(cmdreq_handle);
	onenet_savedatabin_handle_register(savedatabin_handle);
	onenet_savedatabin_field_handle_register(savedatabin_field_handle);
	onenet_savedata_json_handle_register(savedata_json_handle);
}



