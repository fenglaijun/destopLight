#ifndef _FSM_EVENT_h_
#define _FSM_EVENT_h_


/******************************************************************************
 * 事件定义
 */
enum
{
	STM_ENTRY_SIG = 0,
	STM_EXIT_SIG,
	STM_INIT_SIG,
	
	/*
	 * driver层任务事件
	 */
	SYS_TICK_EVT,

	/*
	 * network事件
	 */
	NW_TICK_EVT,
	NW_SMART_OK_EVT,
	NW_SMART_TIMEOUT_EVT,
	NW_AP_STA_ERR_EVT,
	NW_STA_ERR_EVT,
	NW_UPGRADE_EVT,
	NW_WAN_CONN_EVT,
	NW_WAN_DISC_EVT,
	NW_TRAN_AP_EVT,
	NW_TRAN_STA_EVT,
	NW_TRAN_AP_STA_EVT,
	NW_TRAN_SMART_EVT,
	NW_TRAN_IDLE_EVT,
	NW_CHECK_REGISTER_EVT,

};

/*
 * 任务ID定义
 */
enum
{
	DRIVER_ID = 0,
	NETWORK_ID,
};

#endif
