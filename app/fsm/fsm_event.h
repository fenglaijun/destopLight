#ifndef _FSM_EVENT_h_
#define _FSM_EVENT_h_


/******************************************************************************
 * �¼�����
 */
enum
{
	STM_ENTRY_SIG = 0,
	STM_EXIT_SIG,
	STM_INIT_SIG,
	
	/*
	 * driver�������¼�
	 */
	SYS_TICK_EVT,

	/*
	 * network�¼�
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
 * ����ID����
 */
enum
{
	DRIVER_ID = 0,
	NETWORK_ID,
};

#endif
