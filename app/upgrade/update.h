#ifndef __UPDATE_H__
#define __UPDATE_H__

#define AT_USE_FIXED_IP_ADDR
#if defined(UPDATE_DEBUG_ON)
#define UPDATE_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define UPDATE_DEBUG(format, ...)
#endif

void update_system(void);
uint8_t update_get_state(void);

#endif /* __UPDATE_H__ */
