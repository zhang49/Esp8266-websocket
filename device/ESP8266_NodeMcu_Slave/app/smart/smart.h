#ifndef _SMART_H
#define _SMART_H 1

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_CHANNEL	14

#define TYPE_SUBTYPE_QOS_DATA 		0x88
#define TYPE_SUBTYPE_MASK			0xFC

#define NO_RETRY		0x41
#define DS_RETRY_MASK	0x4B

#define BSSID_ADDR	 4
#define SOURCE_ADDR	 10
#define DEST_ADDR	16
#define ADDR_LENGTH	6
#define ADDR_MATCH_START (SOURCE_ADDR)
#define ADDR_MATCH_LENGTH (ADDR_LENGTH*2)	

#define SEQ_ADDR	22
#define SEQ_LEN		2
#define SEQ_MAX		0x1000

#define BASE_LENGTH	 82


#define SSID_FLAG 	1399
#define PWD_FLAG	1459
#define L_FLAG	28
#define C_FLAG	593
#define SEP_1	3
#define SEP_2	23
#define SEP_3	24
#define SEP_4	25
#define SEP_5	26
#define SEP_NUM 2

#define NIBBLE_PER_BYTE	2

#define SEP_1_INDEX	2

#if(SEP_1_INDEX==0)
#define FLAG_NUM  (SEP_NUM+1)
#elif(SEP_1_INDEX==2)
#define FLAG_NUM  (SEP_NUM+2)
#endif
#define FLAG_MATCH_NUM  (FLAG_NUM-1)	// only need to match 2 or 3 byte to increase speed.

// #define TI_SMARTCONFIG 1

#define SSID_NIBBLE_MAX (32*NIBBLE_PER_BYTE)
#define PWD_NIBBLE_MAX (64*NIBBLE_PER_BYTE)
#define SSID_BIT_MAX (SSID_NIBBLE_MAX/8)
#define PWD_BIT_MAX (PWD_NIBBLE_MAX/8)
	
#define TIME_OUT_PER_CHANNEL	(30*1000)

#define STATION_CHECK_TIME	(2*1000)

struct _my_addr_map {
	uint8 addr[ADDR_LENGTH*3];
	uint8_t addr_len;
	uint16_t base_len;
	int16_t flag[FLAG_NUM];		
	// flag[0]: SEP_1, flag[1]: SEP_2, flag[1]: SSID_FLAG.    SEP followed by SSID_FLAG formed flag[]
	// if flag[i]==0, means skip this flag match, eg. SSID_FLAG, 0, SEP_1, SEP_2
	uint8_t flag_match_num;
	int16_t cur_base_seq;
	int8_t base_seq_valid;
	int8_t ssid_len;
	int8_t pwd_len;
};

typedef struct _my_addr_map smart_addr_map;

typedef void (* smart_succeed)(void *arg);

void smart_begin(int chnl, smart_succeed s, void *arg);
void station_check_connect(bool smart);

#ifdef __cplusplus
}
#endif

#endif
