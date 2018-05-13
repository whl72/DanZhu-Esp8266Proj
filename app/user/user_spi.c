/*
 * user_spi.c
 *
 *  Created on: 2016��11��15��
 *      Author: billwang
 */

#include "user_spi.h"
#include "osapi.h"
#include "user_interface.h"

SpiFlashOpResult user_spi_flash_write(uint32 des_addr, uint8 *src_addr, uint32 size){
	uint8 temp_arr[SECTOR_SIZE];
	//char* w_data=(char*)os_malloc(FLASH_WRITE_LEN_BYTE);
	uint32 sector_num,scetor_addr,i;

	sector_num = (des_addr/SECTOR_SIZE);//ȡҪд�������ڵ�������
//	//�����ô�ӡ���
//	os_printf("Ҫд�������ţ�%d\r\n",sector_num);
	spi_flash_read(sector_num*SECTOR_SIZE,(uint32 *)temp_arr,sizeof(temp_arr));//�ȶ���������������
	spi_flash_erase_sector(sector_num);//����Ҫд���ݵ�����
	scetor_addr = (des_addr%SECTOR_SIZE);//ȡҪд�������ڵľ���λ��
//	//�����ô�ӡ���
//	os_printf("Ҫд�ľ���λ�ã�%d\r\n",scetor_addr);
	for(i=0; i<size; i++){//��Ҫд������д�뻺��������
		temp_arr[scetor_addr+i] = *src_addr++;
	}
//	//�����ô�ӡ���
//	os_printf("Ҫд������ݣ�%s\r\n",temp_arr);
	i = spi_flash_write(sector_num*SECTOR_SIZE,(uint32 *)temp_arr,sizeof(temp_arr));//�����浽���������д��flash
//	//�����ô�ӡ���
//	os_printf("д��Ľ����%d\r\n",i);
	return i;
}

void user_router_erase(void){
	struct softap_config user_softap_conf;

	os_memset(user_softap_conf.ssid,0,sizeof(user_softap_conf.ssid));
	os_memset(user_softap_conf.password,0,sizeof(user_softap_conf.password));
	//�����ô�ӡ���
	os_printf("ap_ssid��%s\r\n",user_softap_conf.ssid);
	os_printf("ap_password��%s\r\n",user_softap_conf.password);
	user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+SSID_ADDR,(uint8*)user_softap_conf.ssid,sizeof(user_softap_conf.ssid));
	user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+PASSKEY_ADDR,(uint8*)user_softap_conf.password,sizeof(user_softap_conf.password));
}


