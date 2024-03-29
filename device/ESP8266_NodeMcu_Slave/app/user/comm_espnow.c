/*
 * comm_espnow.c
 *
 *  Created on: Mar 19, 2019
 *      Author: root
 */

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "mem.h"

#include "comm_pub_def.h"
#include "comm_espnow.h"
#include "espnow.h"
#include "comm_device.h"

os_timer_t esp_now_timer;
uint8 sendSerialNb;
u8 ControllerMacAddr[6] = {0xA0, 0x20, 0xA6, 0xAA, 0xAA, 0xAA};

u8 SalveMacAddrVector[ESPNOWSLAVEMAX][6] = {
		{0xA2, 0x01, 0xA6, 0x55, 0x55, 0x55},
		{0xA2, 0x02, 0xA6, 0x55, 0x55, 0x55},
		{0xA2, 0x03, 0xA6, 0x55, 0x55, 0x55},
		{0xA2, 0x04, 0xA6, 0x55, 0x55, 0x55},
		{0xA2, 0x05, 0xA6, 0x55, 0x55, 0x55},
		{0xA2, 0x06, 0xA6, 0x55, 0x55, 0x55},
};
//my esp8266 12F ap macaddr 86-0D-8E-96-5D-7A
//uint8 slave_mac[6]={0x86,0x0d,0x8E,0x96,0x5d,0x7a};

//uint8 slave_mac[6]={0x5e,0xcf,0x7f,0xc2,0x56,0x3d};


EspNowSendBuf esp_now_send_buf[ESPNOWSEDNBUFSIZE];

typedef struct {
	u8 totalCheck;
	u8 type;
	u8 sNumber;
	u8 data[5];
	u8 cursor;
	u8 mac_addr[6];
}EspNowRecvBuf;
EspNowRecvBuf esp_now_recv_buf;
typedef enum  {
	Head1,
	Head2,
	SNumber,
	Type,
	Data,
	TotalCheck
}CommandFlag;
CommandFlag CF;

int last_serial_bumber=-1;
u8 send_tmr_tick=0;

void DisplayMac(char* msg,u8 *mac){
	int i;
	char mac_str[50];
	char temp[10];
	os_memset(mac_str,0,50);
	for(i=0;i<6;i++){
		os_sprintf(temp,i==0?"%2x":"-%2x",mac[i]);
		os_strcat(mac_str,temp);
	}
	ESPNOW_DBG("dest:%s,%s",mac_str,msg);
}
void comm_esp_now_recv_cb(u8 *mac_addr, u8 *data, u8 len){
	u8 *bytes=(u8 *)os_zalloc(len);
	os_memcpy(bytes,data,len+1);
	u8 i=0;
	char str[50];
	for(i=0;i<len;i++){
		os_sprintf(str,(i==0?"%3x":"%s_%2x"),str,bytes[i]);
	}
	ESPNOW_DBG("\t\tesp now recv: %s",str);
	for(i=0;i<len;i++){
restart:
		if(CF!=TotalCheck)
			esp_now_recv_buf.totalCheck^=bytes[i];
		switch(CF){
			case Head1:
				if(bytes[i]==0xaa){
					CF=Head2;
					esp_now_recv_buf.totalCheck=bytes[i];
				}else CF=Head1;
				break;
			case Head2:
				if(bytes[i]==0xbb){
					CF=SNumber;
				}else {
					CF=Head1;
					goto restart;
				}
				break;
			case SNumber:
				CF=Type;
				esp_now_recv_buf.sNumber=bytes[i];
				break;
			case Type:
				CF=Data;
				esp_now_recv_buf.type=bytes[i];
				esp_now_recv_buf.cursor=0;
				break;
			case Data:
				if(esp_now_recv_buf.cursor<5){
					esp_now_recv_buf.data[esp_now_recv_buf.cursor]=bytes[i];
					esp_now_recv_buf.cursor++;
					if(esp_now_recv_buf.cursor==5)
						CF=TotalCheck;
				}
				break;
			case TotalCheck:
				CF=Head1;
				if(esp_now_recv_buf.totalCheck==bytes[i]){
					//pass	,data in byte 2 and 3
					if(esp_now_recv_buf.sNumber!=last_serial_bumber){
						ESPNOW_DBG("ESPNOW: Recv type:%3x, data %3x%3x%3x%3x%3x",esp_now_recv_buf.type,
								esp_now_recv_buf.data[0],esp_now_recv_buf.data[1],
								esp_now_recv_buf.data[2],esp_now_recv_buf.data[3],
								esp_now_recv_buf.data[4]);
						last_serial_bumber=esp_now_recv_buf.sNumber;
						os_memcpy(esp_now_recv_buf.mac_addr,mac_addr,6);
						comm_esp_now_recv_data();
					}else{
						ESPNOW_DBG("ESPNOW: Recv the same serial Number.");
					}
				}else{
					ESPNOW_DBG("ESPNOW: Don't pass check");
					goto restart;
				}
				break;
		}
	}
	os_free(bytes);
}



void comm_esp_now_sendMsg(EspNowSendBuf sbuf){
	u8 ostream[10]={0};
	u8 index=0,i;
	ostream[index++]=0xaa;
	ostream[index++]=0xbb;
	ostream[index++]=sbuf.serial_nb;
	ostream[index++]=sbuf.type;
	//
	for(i=0;i<5;i++){
		ostream[index++]=sbuf.data[i];
	}
	ostream[9]=0;
	for(i=0;i<9;i++)ostream[9]^=ostream[i];
	esp_now_send(sbuf.slave_mac, ostream, 10);
	if(sbuf.failed_count==0)
		DisplayMac("esp now send msg to",sbuf.slave_mac);
}


int ICACHE_FLASH_ATTR esp_now_send_api(uint8_t *mac,uint8_t type,uint8_t *data){
	int i;
	for(i=0;i<ESPNOWSEDNBUFSIZE;i++){
		if(esp_now_send_buf[i].status!=ENSS_Success && os_memcmp(esp_now_send_buf[i].slave_mac,mac,6)==0){
			DisplayMac("esp_now_send_api has this mac",mac);
			return -1;
		}
	}
	for(i=0;i<ESPNOWSEDNBUFSIZE;i++){
		if(esp_now_send_buf[i].status==ENSS_Success){
			esp_now_send_buf[i].status=ENSS_Waitting;
			os_memcpy(esp_now_send_buf[i].slave_mac,mac,6);
			esp_now_send_buf[i].type=type;
			esp_now_send_buf[i].serial_nb=sendSerialNb++;
			esp_now_send_buf[i].failed_count = 0;
			if(data==NULL){
				os_memset(esp_now_send_buf[i].data,0,5);
			}else{
				os_memcpy(esp_now_send_buf[i].data,data,5);
			}
			DisplayMac("esp_now_send_api send",mac);
			return 1;
		}
	}
	DisplayMac("esp_now_send_api failed return -1",mac);
	return -1;
}
//buf send failed, send again;

void comm_esp_now_send_cb(u8 *mac_addr, u8 status){
	u8 i;
	for(i=0;i<ESPNOWSEDNBUFSIZE;i++){
		//DisplayMac("buf mac:",slave_mac);
		//DisplayMac("cb mac:",mac_addr);
		if(os_memcmp(esp_now_send_buf[i].slave_mac,mac_addr,6)==0){
			if(status==0){//send success
				os_memset(esp_now_send_buf[i].slave_mac,0,6);
				esp_now_send_buf[i].status=ENSS_Success;
				DisplayMac("espnow send success to ",mac_addr);
			}else{
				esp_now_send_buf[i].status=ENSS_Failed;
				esp_now_send_buf[i].failed_count++;
				if(esp_now_send_buf[i].failed_count==ESPNOWSENDERRORMAX){
					esp_now_send_buf[i].status=ENSS_Success;
					DisplayMac("espnow send failed to ",mac_addr);
				}
			}
		}
	}
}
static void comm_esp_now_send_tmr(){
	u8 i;
	for(i=0;i<ESPNOWSEDNBUFSIZE;i++){
		if(esp_now_send_buf[i].status==ENSS_Waitting){
			esp_now_send_buf[i].status=ENSS_Sending;
			comm_esp_now_sendMsg(esp_now_send_buf[i]);
		}
		 if(esp_now_send_buf[i].status==ENSS_Failed){
				esp_now_send_buf[i].status=ENSS_Sending;
			comm_esp_now_sendMsg(esp_now_send_buf[i]);
		 }
	}
}

void comm_esp_now_recv_data(){
	switch(esp_now_recv_buf.type){
	/*has modify auto upload
	 * case RequestDht22:
		{
			u8 data[5]={0};
			dht22_temperature_read_api(data);
			dht22_humidity_read_api(data+3);
			esp_now_send_api(ControllerMacAddr,ReplyDht22,data);
		}
		break;
		*/
	case RequestRelay:
		{
			comm_relay_status_set(esp_now_recv_buf.data[0]==0?0:1);
			u8 retdata[5]={0};
			retdata[0] = esp_now_recv_buf.data[0]==0?0:1;
			esp_now_send_api(ControllerMacAddr,ReplyRelay,retdata);
		}
		break;
	/*has modify auto upload
	 * case RequestRay:
		{
			u16 ray_value=ray_read_api();
			u8 retdata[5]={ray_value&0x0f, ray_value>>8};
			esp_now_send_api(ControllerMacAddr,ReplyRay,retdata);
		}
		break;*/
	case RequestMotorMove:
		NODE_DBG("Request MotorMove");
		esp_now_recv_buf.data[3] = esp_now_recv_buf.data[3]>2?0:esp_now_recv_buf.data[3];
		motor_move_start(esp_now_recv_buf.data[0],
				esp_now_recv_buf.data[1]<<8|esp_now_recv_buf.data[2],
				esp_now_recv_buf.data[3]);
		break;
	}
}

void ICACHE_FLASH_ATTR user_esp_now_set_mac_current(void)
{
	int mac_index;
#ifdef DHT22_OPEN
	 wifi_set_macaddr(SOFTAP_IF, SalveMacAddrVector[DHT22_MACINDEX]);
	 mac_index=DHT22_MACINDEX;
#elif defined RELAY1_OPEN
	 wifi_set_macaddr(SOFTAP_IF, SalveMacAddrVector[RELAY1_MACINDEX]);
	 mac_index=RELAY1_MACINDEX;
#elif defined RELAY2_OPEN
	 wifi_set_macaddr(SOFTAP_IF, SalveMacAddrVector[RELAY2_MACINDEX]);
	 mac_index=RELAY2_MACINDEX;
#elif defined  RELAY3_OPEN
	 wifi_set_macaddr(SOFTAP_IF, SalveMacAddrVector[RELAY3_MACINDEX]);
	 mac_index=RELAY3_MACINDEX;
#elif defined RAY_OPEN
	 wifi_set_macaddr(SOFTAP_IF, SalveMacAddrVector[RAY_MACINDEX]);
	 mac_index=RAY_MACINDEX;
#endif

    wifi_set_opmode_current(SOFTAP_MODE);
    struct softap_config cfg;
    wifi_softap_get_config(&cfg);
    char ssid_str[50];
    os_memset(ssid_str,0,50);
    os_sprintf(ssid_str,"%s%d",DEFAULT_AP_SSID,mac_index);
    os_strcpy(cfg.ssid,ssid_str);
    cfg.ssid_len=os_strlen(ssid_str);
    os_strcpy(cfg.password,DEFAULT_AP_PWD);
    cfg.authmode=AUTH_WPA2_PSK;
    cfg.ssid_hidden = 0;
    wifi_softap_set_config(&cfg);
    os_printf("user_esp_now_set_mac_current mac_index:%d",mac_index);
}

char *ICACHE_FLASH_ATTR getMacAddrByIndex(uint8_t index){
	return SalveMacAddrVector[index];
}
void comm_espnow_init(){
	if (esp_now_init() == 0) {
		ESPNOW_DBG("ESPNOW: init successful\n");
		// 注册 ESP-NOW 收包的回调函数
		esp_now_register_recv_cb(comm_esp_now_recv_cb);
		// 注册发包回调函数
		esp_now_register_send_cb(comm_esp_now_send_cb);
	}
	esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
	os_timer_disarm(&esp_now_timer);
	os_timer_setfn(&esp_now_timer, comm_esp_now_send_tmr, NULL);
	os_timer_arm(&esp_now_timer, 50, 1);
}














