#ifndef _ONENET_CONFIG_H_
#define  _ONENET_CONFIG_H_

#if defined(GLOBAL_DEBUG)
#define ONENET_DEBUG(format, ...) os_printf(format, ##__VA_ARGS__)
#else
#define ONENET_DEBUG(format, ...)
#endif

#define EDP_TCP_LOCAL_PORT   876
#define EDP_TCP_REMOTE_PORT  29876    // or 876
#define ONENET_TCP_DOMAIN    "jjfaedp.hedevice.com"
#define ONENET_TCP_IPADDR    "183.230.40.39"
#define ONENET_HTTP_DOMAIN  "api.heclouds.com"
#define ONENET_HTTP_IPADDR  "183.230.40.33"
//#define ONENET_USE_DNS

// 0x79:SAVE1  0x7A:SAVE2  0x7B:flag
#define ONENET_PARAM_START_SEC         0x79

typedef struct ds_item_s
{
	char ds_name[24];
	struct ds_item_s *next;
}ds_item_t;

typedef struct
{
	//��3������ƽ̨�Զ���������
	uint8 device_id[32];
	uint8 device_apikey[32];
	uint8 device_name[64];

	// ��4����Ҫ�û��ֶ�����, ��һ�ε�¼����ʹ��master_apikey+user_id��¼��
	char master_apikey[32];
	char project_id[32];
	char auth_info[64];
//	char sn_info[64]; // �����������к�
	uint8 auth_method;
	
	uint8 register_flag;	// 1:���豸�Ѿ�ע��
	uint8 init_flag;	// 1:������ʼ��, ��1:��ʾ��һ���������
} onenet_cloud_info_t;


#endif


