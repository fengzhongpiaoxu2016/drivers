#include "bsp_esp8266.h"
#include <string.h>
#include <stdio.h>

ESP8266_Type esp8266;

//�����Ҫ����ʵ�����������ʱ�����Ķ���
extern void Delay_ms(u16 dly_10ms);
#define log(...) printf(__VA_ARGS__)
uint8_t ssid[20];
uint8_t secret[20];
	
/*
* Parameter:
* send:���ڷ��ͺ���
* rxbuff:���ڽ��ջ�������ָ��
* rxlen������ʵ���յ����ַ����ȵ�ָ��
* buff_size:���ڽ��ջ������Ĵ�С
*/
int8_t reg_ESP8266( void (*func_send)(uint8_t *,unsigned short),uint8_t *rx_buf,uint16_t *rx_len,uint16_t buff_size)
{
	esp8266.send=func_send;	
	esp8266.rxbuf=rx_buf;
	esp8266.rxlen=rx_len;
	esp8266.buf_size=buff_size;
	*esp8266.rxlen=0;	
	//Delay_ms(1000); //������������ʱ  �ϵ����Ҫ��ʱ�ȴ�esp8266��ɳ�ʼ��
	return ESP_OK;
}

/*
* Parameter:
* cmd:�����͵�ATָ��
* ack:�������ص��ַ���
* timeout:��ʱʱ�䣬��λ��
*/
int8_t ESP_cmd(char *cmd,char *ack,uint8_t timeout)
{
	uint16_t cnt_10ms=0;
	if(timeout>100 || strlen((char *)cmd)>250)
		return ESP_PARA_ERR;
	memset(esp8266.rxbuf,0,esp8266.buf_size); //���������	
	*esp8266.rxlen=0;
	esp8266.send((uint8_t *)cmd,strlen((char *)cmd));
	while(cnt_10ms < (timeout*100))  //����û�г�ʱ
	{
		if(strstr((char *)esp8266.rxbuf,(char *)ack)!=NULL)
			return ESP_OK;
		cnt_10ms++;
		Delay_ms(10);
	}
	return ESP_ERR;
}

int8_t ESP_Initial(char *name,char *pwd)
{
	int8_t i=3;
	if(strlen(name)<sizeof(ssid) && strlen(pwd)<sizeof(secret))
	{
		memset(ssid,0,sizeof(ssid));
		memset(secret,0,sizeof(secret));
		memcpy(ssid,name,strlen(name));
		memcpy(secret,pwd,strlen(pwd));
		while(i--){
			if(ESP_OK==ESP_cmd("AT\r\n","OK",2))
				return ESP_OK;
			else
				Delay_ms(1000);
		}
	}	
	return ESP_ERR;
}

/*
 * Function:ESP_QueryStatus
 * Return:
 *   '2'��ESP8266 Station ������ AP����� IP ��ַ
 *   '3'��ESP8266 Station �ѽ��� TCP �� UDP ����
 *   '4'��ESP8266 Station �Ͽ���������
 *   '5'��ESP8266 Station δ���� AP 
 */
int8_t ESP_QueryStatus()
{
	char *pos;
	if(ESP_OK==ESP_cmd("AT+CIPSTATUS\r\n","OK\r\n",3))
	{
		esp8266.rxbuf[*esp8266.rxlen]=0;
		pos=strstr((char *)esp8266.rxbuf,"STATUS:");
		if(pos)
		{
			return pos[7];
		}
	}else if(ESP_OK!=ESP_cmd("AT\r\n","OK",2))
	{
		log("Please check your esp8266 circuit...\r\n");
		return ESP_HARDWARE_ERR;
	}else
	  return ESP_OK;
	return ESP_UNKOWN_ERR;
}

int8_t ESP_JoinAP()
{
  char cmd[100]={0};
	ESP_cmd("AT+CWMODE_CUR=1\r\n","OK\r\n",2);//����wifi����ģʽ
	sprintf(cmd,"AT+CWJAP_CUR=\"%s\",\"%s\"\r\n",ssid,secret);
	if(ESP_OK==ESP_cmd(cmd,"WIFI GOT IP",8))
	{
		ESP_cmd("","OK\r\n",3);
	  return ESP_OK;
	}
	if(ESP_OK==ESP_cmd("AT\r\n","busy p",2))  //��һ������û��ɣ�������WIFIû�ҵ�
	{
		log("AP not found!!\r\n");
	}
	return ESP_ERR;
}

int8_t ESP_Close()  //�Ͽ�AP����
{
	ESP_cmd("AT+CWQAP\r\n","OK\r\n",1);
	return ESP_OK;
}

int8_t ESP_Disconnect() //�Ͽ�����������
{
	return ESP_cmd("AT+CIPCLOSE\r\n","OK\r\n",2);
}

int8_t ESP_ConnectServer(char *host,uint16_t port)
{
	int8_t stu=0;
  char cmd[100]={0};
	sprintf((char *)cmd,"AT+CIPSTART=\"TCP\",\"%s\",%d,7200\r\n",host,port);
	
	stu=ESP_QueryStatus();
	switch(stu)  //ע��,�˴�case�÷�ֻ��һ��break;
	{		
		case '3': //�ѽ���TCP��UDP����
			return ESP_OK;
		case '5':  //δ����AP
			if(ESP_OK!=ESP_JoinAP())
				return ESP_ERR;
		case '2': //������AP,���IP��ַ
		case '4':	//����Ͽ�
			if(ESP_OK==ESP_cmd(cmd,"CONNECT",8))
				return ESP_OK;
		default:
		break;
	}
	return ESP_ERR;
}

int8_t ESP_send( uint8_t *buf,uint16_t len)
{
	char cmd[50]={0};
	sprintf(cmd,"AT+CIPSEND=%d\r\n",len);
	if(ESP_OK==ESP_cmd(cmd,">",1))  //׼����������
	{
	  esp8266.send(buf,len);
	  return ESP_cmd("","SEND OK",5);
	}else
	  return ESP_ERR;
}

int ESP_recv(uint8_t *buf,int len)
{
	char *data;
	int16_t reallen=0;
	data=strstr((char *)esp8266.rxbuf,"+IPD,");
	if(data){
		if(sscanf(data,"+IPD,%hd:",&reallen)==1){
			data=strstr(data,":")+1;
			if(reallen>len){ //����û��ȡ��
				memcpy(buf,data,len);
				memmove(data,data+len,reallen-len);
				*esp8266.rxlen=*esp8266.rxlen-len;
				return len;
			}else{
				memcpy(buf,data,reallen);
				*esp8266.rxlen=0;
				return reallen;
			}
		}
	}
	return ESP_ERR;
}


//int8_t ESP_CallBack_Check(char *ssid,char *secret,char *host,uint16_t port)
//{
//	int8_t stu=0;
//  char cmd[100]={0};
//	stu=ESP_QueryStatus();
//	switch(stu)  //ע��,�˴�case�÷�ֻ��һ��break;
//	{
//		case '3':
//			log("case 3");
//		case '4':
//			log("case 4");
//		case '5':
//			log("case 5");
//		default:
//		break;
//	}
//	if(stu=='3')
//		return ESP_OK;
//	else if(stu<'2' || stu>'5')
//		return ESP_UNKOWN_ERR;
//	if(stu=='5'){  //δ����AP
//		if(ESP_OK!=ESP_JoinAP()){
//			return ESP_ERR;
//		}	
//	}
//	stu=ESP_QueryStatus();
//	if(stu=='2' || stu=='4'){
//		sprintf((char *)cmd,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",host,port);
//		if(ESP_OK==ESP_cmd(cmd,"CONNECT",8))
//			return ESP_OK;
//	}
//	return ESP_ERR;
//}
