#ifndef _NETWORK_TASK_h_
#define _NETWORK_TASK_h_

#ifdef NETWORK_DEBUG_ON
#define NETWORK_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define NETWORK_DEBUG(format, ...)
#endif

enum
{
	NW_STATE_IDLE=0,
	NW_STATE_SMART,
	NW_STATE_AP,
	NW_STATE_APSTA,
	NW_STATE_STA,
};

enum
{
	SMART_STATE_SUCCESS=0,
	SMART_STATE_TIMEOUT,
	SMART_STATE_FIND,
	SMART_STATE_GETTING,
	SMART_STATE_LINK,
};
uint8_t network_smartlink_get_state(void);
uint8_t network_get_state(void);

#endif

