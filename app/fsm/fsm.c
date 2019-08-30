/*******************************************************************************
--事件驱动状态机
--0.01
--zrpeng
--2015-6-13
--
--note:

--history:
	0.01	2015-6-6
	在stm32上面运行正常，支持低功耗__wfe模式,运行正常，仿照CC2540的osal层写的，
	增加了有限状态机跳转操作。

	2015-11-5  运行在esp8266上面正常.

	增加os_send_message, 待测试，打算在stm中加
*******************************************************************************/
#include "fsm.h"


//状态机入口状态函数
extern void driver_actor(event_t *e);
extern void network_actor(event_t *e);


//状态机,数组偏移就是状态机的id号,自行增加状态机
static stm_handler tasks_pool[TASKS_CNT] =
{
	driver_actor,
	network_actor
};
static queue_t tasks_queue[TASKS_CNT]; /* 每个状态机对应的消息队列 */
static uint8_t stm_ret_value;          /* 状态机返回值 */
static stm_handler stm_state_temp;     /* 临时状态机变量 */
//static uint8_t message_nesting_cnt;    /* 消息嵌套计数 */

//钩子函数, 触发系统再一次进入事件循环
static void (*dispatch_hook)(void);
static uint8_t next_dispatch;	/* 即将发生下一次事件循环 */

//定时器
static timer_t timer_pool[TIMER_CNT];
//static uint32_t tick_previous;
//static uint32_t sys_tick;

//电源
#ifdef POWER_SAVING
static uint32_t power_task_state;		/* 32个位，支持32个fsm,可以扩大 */
#endif

/*
 * static function
 */
static void os_power_sleep(void);

/******************************************************************************
*       事件循环
*/
void ICACHE_FLASH_ATTR os_dispatch(void)
{
	uint8_t i;

	next_dispatch = 0;

	//查询事件并处理
	for (i = 0; i < TASKS_CNT; i++)
	{
		if (tasks_queue[i].used)
		{
			event_t e;

			PORT_SR_ALLOC();
			PORT_CPU_DISABLE();
			tasks_queue[i].used--;
			e = tasks_queue[i].buf[ tasks_queue[i].head ];
			tasks_queue[i].head++;
			if (tasks_queue[i].head >= QUEUE_SIZE)
			{
				tasks_queue[i].head = 0;
			}
			PORT_CPU_ENABLE();

			(tasks_pool[i])(&e);
			if (stm_ret_value == STM_TRAN)
			{
				stm_ret_value = 0;
				e.sig = STM_EXIT_SIG;
				(tasks_pool[i])(&e);
				e.sig = STM_ENTRY_SIG;
				stm_state_temp(&e);
				tasks_pool[i] = stm_state_temp;
			}
		}
	}
#ifdef POWER_SAVING
	// 不会有下一次os_dispatch了，可以考虑功耗了
	if (!next_dispatch)
	{
		os_power_sleep();
	}
#endif
}

void ICACHE_FLASH_ATTR os_init_tasks(void)
{
	uint8_t i;
	for (i = 0; i < TASKS_CNT; i++)
	{
		os_post_message(i, STM_ENTRY_SIG, 0);
	}
	for (i = 0; i < TIMER_CNT; i++)
	{
		timer_pool[i].task_id = INVALID_TASK_ID;
	}
}

void ICACHE_FLASH_ATTR os_register_hook(void (*func)(void))
{
	dispatch_hook = func;
}

/******************************************************************************
*   事件
*/
uint8_t ICACHE_FLASH_ATTR os_post_message(uint8_t task_id, uint8_t sig, uint32_t para)
{
	event_t e;
	e.sig = sig;
	e.para = para;
	
	PORT_SR_ALLOC();
	PORT_CPU_DISABLE();
	if (tasks_queue[task_id].used >= QUEUE_SIZE)
	{
		PORT_CPU_ENABLE();
		return QUEUE_FULLED;
	}
	tasks_queue[task_id].buf[ tasks_queue[task_id].tail ] = e;
	tasks_queue[task_id].tail++;
	if (tasks_queue[task_id].tail >= QUEUE_SIZE)
	{
		tasks_queue[task_id].tail = 0;
	}
	tasks_queue[task_id].used++;

	if (!next_dispatch)
	{
		next_dispatch = 1;
		if (dispatch_hook)
		{
			(*dispatch_hook)();
		}
	}
	PORT_CPU_ENABLE();

	return RET_SUCCESS;
}
#if 0
/*
 * notes: 与windows的PostMessage,SendMessage一样，一个是异步执行，一个是同步执行
 *        os_post_message是把消息放到队列里面，等下一次消息循环时取出消息再执行，
 *        而os_send_message是直接调用消息处理函数处理消息，消息是没有经过消息队列的
 *        os_send_message可能会产生递归，所以这里设定了最大递归层数
 *        千万不要在STM_ENTRY_SIG和STM_EXIT_SIG中使用os_send_message
 */
uint8_t ICACHE_FLASH_ATTR os_send_message(uint8_t task_id, uint8_t sig, uint32_t para)
{
	event_t e;
	e.sig = sig;
	e.para = para;
	
	message_nesting_cnt++;	/* ++和--要成对出现 */
	if (message_nesting_cnt > MESSAGE_NESTING_MAX)
	{
		message_nesting_cnt--;
		return NESTING_FULLED;
	}

	(tasks_pool[task_id])(&e);	/* 在这里发生递归 */
	if (stm_ret_value == STM_TRAN)
	{
		stm_ret_value = 0;
		e.sig = STM_EXIT_SIG;
		(tasks_pool[task_id])(STM_EXIT_SIG);
		e.sig = STM_ENTRY_SIG;
		stm_state_temp(STM_ENTRY_SIG);
		tasks_pool[task_id] = stm_state_temp;
	}

	message_nesting_cnt--;
	return RET_SUCCESS;
}
#endif


/******************************************************************************
*  定时器
*/
void ICACHE_FLASH_ATTR os_timer_update(void)
{
	uint8_t i;

	for (i = 0; i < TIMER_CNT; i++)
	{
		if (timer_pool[i].task_id != INVALID_TASK_ID)
		{
			if (timer_pool[i].timeout > 0)
			{
				timer_pool[i].timeout--;
			}
			else
			{
				os_post_message(timer_pool[i].task_id, timer_pool[i].sig, timer_pool[i].para);
				if (timer_pool[i].flag & TIMER_RELOAD_FLAG)
				{
					timer_pool[i].timeout = timer_pool[i].reload;
				}
				else
				{
					timer_pool[i].task_id = INVALID_TASK_ID;
				}
			}
		}
	}
}

void ICACHE_FLASH_ATTR os_timer_set(uint8_t task_id, uint8_t sig, uint32_t para, uint16_t timeout)
{
	uint8_t i;

	PORT_SR_ALLOC();
	PORT_CPU_DISABLE();
	//申请定时器
	for (i = 0; i < TIMER_CNT; i++)
	{
		if (timer_pool[i].task_id == task_id &&
		        timer_pool[i].sig == sig)		//已存在定时器,修改超时值
		{
			timer_pool[i].para = para;			//重新修改参数
			timer_pool[i].timeout = timeout;
			break;
		}
		else if (timer_pool[i].task_id == INVALID_TASK_ID)
		{
			timer_pool[i].sig = sig;
			timer_pool[i].para = para;
			timer_pool[i].timeout = timeout;
			timer_pool[i].task_id = task_id;
			timer_pool[i].flag = 0;
			break;
		}
	}
	PORT_CPU_ENABLE();
}

void ICACHE_FLASH_ATTR os_reload_timer_set(uint8_t task_id, uint8_t sig, uint32_t para, uint16_t timeout)
{
	uint8_t i;

	PORT_SR_ALLOC();
	PORT_CPU_DISABLE();
	//申请定时器
	for (i = 0; i < TIMER_CNT; i++)
	{
		if (timer_pool[i].task_id == task_id &&
		        timer_pool[i].sig == sig)		//已存在定时器,直接修改重复超时值,下一轮有效
		{
			timer_pool[i].para = para;			//重新修改参数
			timer_pool[i].reload = timeout;
			break;
		}
		else if (timer_pool[i].task_id == INVALID_TASK_ID)
		{
			timer_pool[i].sig = sig;
			timer_pool[i].para = para;
			
			timer_pool[i].timeout = timeout;
			timer_pool[i].reload = timeout;
			timer_pool[i].task_id = task_id;
			timer_pool[i].flag |= TIMER_RELOAD_FLAG;
			break;
		}
	}
	PORT_CPU_ENABLE();
}

void ICACHE_FLASH_ATTR os_timer_del(uint8_t task_id, uint8_t sig)
{
	uint8_t i;

	PORT_SR_ALLOC();
	PORT_CPU_DISABLE();
	//删除定时器
	for (i = 0; i < TIMER_CNT; i++)
	{
		if (timer_pool[i].task_id == task_id &&
		        timer_pool[i].sig == sig)		//删除存在定时器
		{
			timer_pool[i].task_id = INVALID_TASK_ID;
			timer_pool[i].flag = 0;
			break;
		}
	}
	PORT_CPU_ENABLE();
}

#ifdef POWER_SAVING
/******************************************************************************
*   电源低功耗
*/
static void ICACHE_FLASH_ATTR os_power_sleep(void)
{
	if (power_task_state == 0)
	{
		// 在这里配置MCU进入低功耗
		;
	}
}
void ICACHE_FLASH_ATTR os_power_set(uint8_t task_id, uint8_t state)
{
	if (state == POWER_HOLD)
	{
		power_task_state |= 1 << task_id;
	}
	else if (state == POWER_SLEEP)
	{
		power_task_state &= ~(1 << task_id);
	}
}
#endif

/******************************************************************************
*  有限状态机跳转
*/
void ICACHE_FLASH_ATTR os_stm_tran(stm_handler state)
{
	stm_ret_value = STM_TRAN;
	stm_state_temp = state;
}

