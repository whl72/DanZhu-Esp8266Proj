/*
 * user_process.h
 *
 *  Created on: 2016年11月16日
 *      Author: billwang
 */

#ifndef APP_INCLUDE_USER_PROCESS_H_
#define APP_INCLUDE_USER_PROCESS_H_

#include "c_types.h"
#include "user_interface.h"
//#include "espconn.h"

//struct user_app_tcp{
//	uint8 link_flag;
//	struct espconn app_tcp_conn;
//};

#define SN_LENGTH			16
#define ID_LENGTH			2

typedef enum
{
  teClient,
  teServer
}teType;

typedef struct
{
	BOOL linkEn;
	BOOL teToff;
	uint8_t linkId;
	teType teType;
	uint8_t repeaTime;
	uint8_t changType;
	uint8 remoteIp[4];
	int32_t remotePort;
	struct espconn *pCon;
}linkConType;

#define SEVER_PORT 				8787
#define UPDATA_SELF_PORT		9999   //区别于smartfurniture
#define LISTEN_PORT				9050
#define UDP_SEND_PORT			9050
#define SOFTAP_SEVER_PORT		5050
#define DEFAULT_TCP_NUM 		5
#define EARSE_TIMES				31

//升级用到的
#define BOOT_OK				0xa0
#define BOOT_ERR 			0xa1
#define BOOT_HEAD 			0xa5
#define BOOT_READ 			0xa6
#define BOOT_WRITE 			0xa7
#define BOOT_VERIFY 		0xa8
#define BOOT_GO 			0xa9
#define BOOT_START 			0xaf
#define BOOT_END1 			0xfe
#define BOOT_END2 			0xfa
#define BOOT_CBUPDATA 		0x33

#define UPGRADE_FLAG_IDLE 	0x00
#define UPGRADE_FLAG_START 	0x01
#define UPGRADE_FLAG_FINISH 0x02
#define LINK_MAX 0x03

void mode_process(void);
void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg);
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err);
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg,char *pdata,unsigned short len);
void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg);
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg);
void ICACHE_FLASH_ATTR send_app_status(uint8 *tcp_send_buff);
void ICACHE_FLASH_ATTR tt_send_app_status(uint8 *tcp_send_buff, uint8 len);
void ICACHE_FLASH_ATTR send_sever_data(uint8 *pbuff);
void ICACHE_FLASH_ATTR set_process_mode(WORK_MODE_T mode);
WORK_MODE_T ICACHE_FLASH_ATTR get_process_mode(void);
void ICACHE_FLASH_ATTR send_mcu_mode_status(void);
void ICACHE_FLASH_ATTR creat_udp_broadcast(void);

#endif /* APP_INCLUDE_USER_PROCESS_H_ */
