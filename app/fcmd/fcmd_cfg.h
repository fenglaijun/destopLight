#ifndef _FCMD_CFG_H_
#define _FCMD_CFG_H_
/*******************************************************************************
* 用户函数命令头文件包含，函数声明
*/

#include "cmd_mem.h"
#include "osapi.h"

#include "user_interface.h"
#include "mem.h"
#include "cmd_esp8266.h"
#include "flash_api.h"

#include "smartlink.h"

#include "onenet_app.h"
#include "onenet_tcp.h"
#include "onenet_http.h"
#include "onenet_config.h"
#include "fsm.h"


/*******************************************************************************
 * 自定义函数命令表
 */
CmdTbl_t CmdTbl[] =
{
	//系统命令, SYSTEM_CMD_NUM和系统命令个数保持一致
	"ls",  0,
	"addr", 0,
	"help", 0,


	//用户命令
	"void md(int addr, int elem_cnt, int elem_size)", (void(*)(void))md,
	"int cmp(void *addr1, void *addr2, int elem_cnt, int elem_size)", (void(*)(void))cmp,


	//system api
	"uint32 system_get_free_heap_size(void)", (void(*)(void))system_get_free_heap_size,
	"uint32 system_get_chip_id(void)", (void(*)(void))system_get_chip_id,
	"void system_restart(void)", (void(*)(void))system_restart,
	"void system_restore(void)",  (void(*)(void))system_restore,

	"void get_ip_mac(void)", (void(*)(void))get_ip_mac,

	// flash
	"uint32 spi_flash_get_id(void)",   (void(*)(void))spi_flash_get_id,
	"int sfmd(u32 addr, int elem_cnt, int elem_size)",	(void(*)(void))sfmd,
	"int sfmw(u32 writeaddr, u8 *pbuf, u32 num)",		(void(*)(void))sfmw,

	"void smartlink_start(void)", (void(*)(void))smartlink_start,
	"void smartlink_stop(void)",  (void(*)(void))smartlink_stop,

	"uint8_t wifi_station_get_connect_status(void)", (void(*)(void))wifi_station_get_connect_status,
	"bool wifi_station_connect(void)",  (void(*)(void))wifi_station_connect,
	"bool wifi_station_disconnect(void)",(void(*)(void))wifi_station_disconnect,
	"uint8 wifi_get_opmode(void)", (void(*)(void))wifi_get_opmode,
	"bool wifi_set_opmode(uint8 opmode)", (void(*)(void))wifi_set_opmode,
	
	"void set_station_config(char *ssid, char *password)", (void(*)(void))set_station_config,

	"void onenet_device_info_print(void)", (void(*)(void))onenet_device_info_print,
	"void onenet_param_restore(void)", (void(*)(void))onenet_param_restore,
	"void onenet_param_save(void)", (void(*)(void))onenet_param_save,
	"void onenet_param_load(void)", (void(*)(void))onenet_param_load,

	"void fsm_init(void)", (void(*)(void))fsm_init,

	"uint8_t os_post_message(uint8_t task_id, uint8_t sig, uint32_t para)", (void(*)(void))os_post_message,
	
};
uint8_t CmdTblSize = sizeof(CmdTbl) / sizeof(CmdTbl[0]);

#endif


