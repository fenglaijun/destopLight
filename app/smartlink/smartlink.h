#ifndef __SMARTLINK_H__
#define __SMARTLINK_H__

typedef void (* smartlink_success_callback_t)(const void * pdata);
typedef void (* smartlink_timeout_callback_t)(const void * pdata);
typedef void (* smartlink_getting_callback_t)(const void * pdata);
typedef void (* smartlink_linking_callback_t)(const void * pdata);
typedef void (* smartlink_finding_callback_t)(const void * pdata);
void smartlink_success_callback_register(smartlink_success_callback_t smartlink_success_callback);
void smartlink_timeout_callback_register(smartlink_timeout_callback_t smartlink_timeout_callback);
void smartlink_getting_callback_register(smartlink_getting_callback_t smartlink_getting_callback);
void smartlink_linking_callback_register(smartlink_linking_callback_t smartlink_linking_callback);
void smartlink_finding_callback_register(smartlink_finding_callback_t smartlink_finding_callback);
void smartlink_start(void);
void smartlink_stop(void);
void smartlink_init(void);



#endif /* __SMARTLINK_H__ */
