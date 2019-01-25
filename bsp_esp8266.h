#ifndef __BSP_ESP8266_H
#define __BSP_ESP8266_H
#include "stm32f10x.h"

#define uint8_t unsigned char
#define int8_t signed char
typedef unsigned short uint16_t;

#define ESP_OK 0
#define ESP_ERR -1
#define ESP_PARA_ERR -2
#define ESP_HARDWARE_ERR -3 
#define ESP_UNKOWN_ERR -4

typedef struct ESP8266_Struct ESP8266_Type;

struct ESP8266_Struct
{
  void (*send)(uint8_t *,unsigned short);
  uint8_t *rxbuf;
  uint16_t *rxlen;
  uint16_t buf_size;
  int8_t status;//-1:硬件错误 0:没有初始化 2:成功连接网络 3:成功连接服务器
	uint8_t ssid[20];
	uint8_t secret[20];
	uint8_t server_ip[4];
	uint16_t server_port;
};
extern ESP8266_Type esp8266;

int8_t reg_ESP8266(void (*func_send)(uint8_t *,unsigned short),uint8_t *rxbuff,uint16_t *rxlen,uint16_t buff_size);
int8_t ESP_Initial(char *ssid,char *secret);
int8_t ESP_cmd(char *cmd,char *ack,uint8_t timeout);
int8_t ESP_JoinAP(void);
int8_t ESP_ConnectServer(char *host,uint16_t port);
int8_t ESP_Disconnect(void);
int8_t ESP_send(uint8_t *buf,uint16_t len);
int ESP_recv(uint8_t *buf,int len);
int8_t ESP_QueryStatus(void); 
//int8_t ESP_CallBack_Check(char *ssid,char *secret,char *host,uint16_t port);
#endif
