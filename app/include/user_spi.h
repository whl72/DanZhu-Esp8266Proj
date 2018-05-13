/*
 * user_sip.h
 *
 *  Created on: 2016��11��15��
 *      Author: billwang
 */
#include "c_types.h"
#include "spi_flash.h"

#ifndef APP_INCLUDE_USER_SPI_H_
#define APP_INCLUDE_USER_SPI_H_

//#define USER_BASE_ADDR 		16

/*���ж�д������Ҫע��4byte����*/
/*124���������������������Ϣ*/
#define USER_BASE_ADDR 		124
#define SECTOR_SIZE 		4096
#define SSID_ADDR			0 //32byte
#define BSSID_ADDR			32//32byte
#define PASSKEY_ADDR		64//64byte
#define SEVER_ADDR			128//64byte
//deviceIDʵ��ռ����17���ֽ�
#define DEVICE_ID_ADDR		192//32byte
//routerIDʵ��ռ����17���ֽ�
#define ROUTER_ID_ADDR		224//32byte
/*125�����������RGB��Ϣ������״̬*/
#define USER_LIGHT_BASE_ADDR	125
#define SWITCH_ADDR				0//4byte
#define RGB_ADDR				4//4byte
#define BRI_ADDR				8//4byte
/*126����������Ŵ��������������־*/
#define USER_UPDATE_ERA_FALG_ADDR  126

SpiFlashOpResult user_spi_flash_write(uint32 des_addr, uint8 *src_addr, uint32 size);
void user_router_erase(void);

#endif /* APP_INCLUDE_USER_SPI_H_ */
