#include "fsm.h"

#include "driver_task.h"
#include "osapi.h"

#include "network_task.h"
#include "onenet_app.h"
#include "update.h"

uint32_t g_sys_tick;

/******************************************************************************
*  0:Çý¶¯×´Ì¬»ú
*/
void driver_actor(event_t *e);

void ICACHE_FLASH_ATTR driver_actor(event_t *e)
{
	switch (e->sig)
	{
	case SYS_TICK_EVT:

		break;

	case STM_ENTRY_SIG:
		g_sys_tick = 0;
		os_reload_timer_set(DRIVER_ID, SYS_TICK_EVT, 0, 1000);

		os_post_message(NETWORK_ID, NW_TRAN_SMART_EVT, 0);
		break;

	case STM_EXIT_SIG:
		break;
	}
}

