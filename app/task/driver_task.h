#ifndef _DRIVER_TASK_h_
#define _DRIVER_TASK_h_

#ifdef DRIVER_DEBUG_ON
#define DRIVER_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define DRIVER_DEBUG(format, ...)
#endif

extern uint32_t g_sys_tick;

#endif
