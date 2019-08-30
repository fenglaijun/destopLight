#ifndef _CMD_ESP8266_H_
#define _CMD_ESP8266_H_


void get_ip_mac(void);
int _atoi(const char *s);

void sim_network_send(char *s);
void set_station_config(char *ssid, char *password);

void change_ssid(void);
void fsm_init(void);

#endif

