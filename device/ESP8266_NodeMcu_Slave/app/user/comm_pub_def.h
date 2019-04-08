/*
 * comm_pubDef.h
 *
 *  Created on: Mar 27, 2019
 *      Author: root
 */

#ifndef COMM_PUB_DEF_H_
#define COMM_PUB_DEF_H_


#define COMM_NOREFRESH 0
#define COMM_REFRESHING 1
#define COMM_REFRESHED 2

#define EspNowChannel 3


typedef enum {
	RequestDht22 = 0x01,
	RequestRay,
	RequestRay_MotorPos,
	RequestRay_MotorPas,
	RequestRelay,
}EspNowRequestMsgType;

typedef enum {
	ReplyDht22 = 0x81,
	ReplyRay = 0x81,
	ReplyRelay
}EspNowReplyMsgType;


#endif /* COMM_PUBDEF_H_ */
