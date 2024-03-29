#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 

//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
#define USART_SLOT_SIZE  		3072 	//循环队列缓冲区大小
#define EN_USART1_RX 			1		//使能（1）/禁止（0）串口1接收
#define EN_USART3_RX 			1		//使能（1）/禁止（0）串口3接收


struct USART2_RD_QUEUE{
	u8 data[USART_SLOT_SIZE];
	u32 head;
	u32 rear;
};

extern struct USART2_RD_QUEUE rdQueue;	//队列缓冲区
void USART1_Init(u32 bound);
void USART3_Init(u32 bound);
void USART1_Putc(char ch);
void USART1_Send(char *data);
void USART3_Putc(char ch);
void USART3_Send(char *data);

#endif


