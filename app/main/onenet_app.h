#ifndef __ONENET_APP_H__
#define __ONENET_APP_H__

#ifdef ONENET_APP_DEBUG_ON
#define ONENET_APP_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define ONENET_APP_DEBUG(format, ...)
#endif

// FLASH MAP
// 0x7c: SAVE1 0x7d: SAVE2 0x7e: FLAG
#define ONENET_APP_START_SEC   	0x7c
#define ONENET_APP_POWER_PERCENT 	100
#define ONENET_APP_POWER_LIMIT(x) 	((x) * (ONENET_APP_POWER_PERCENT) / 100)

void onenet_app_set_smart_effect(uint8_t effect);
void onenet_app_smart_timer_tick(void *param);
void onenet_app_smart_timer_start(void);
void onenet_app_smart_timer_stop(void);
void onenet_callback_init(void);


#endif /* __USER_GIZWITS_VIRTUAL_MCU_H__ */

