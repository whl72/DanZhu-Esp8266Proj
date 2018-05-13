/*
 * user_spi.c
 *
 *  Created on: 2016年11月15日
 *      Author: billwang
 */

#include "user_spi.h"
#include "osapi.h"
#include "user_interface.h"

SpiFlashOpResult user_spi_flash_write(uint32 des_addr, uint8 *src_addr, uint32 size){
	uint8 temp_arr[SECTOR_SIZE];
	//char* w_data=(char*)os_malloc(FLASH_WRITE_LEN_BYTE);
	uint32 sector_num,scetor_addr,i;

	sector_num = (des_addr/SECTOR_SIZE);//取要写数据所在的扇区号
//	//调试用打印输出
//	os_printf("要写的扇区号：%d\r\n",sector_num);
	spi_flash_read(sector_num*SECTOR_SIZE,(uint32 *)temp_arr,sizeof(temp_arr));//先读出该扇区的数据
	spi_flash_erase_sector(sector_num);//擦除要写数据的扇区
	scetor_addr = (des_addr%SECTOR_SIZE);//取要写数据所在的具体位置
//	//调试用打印输出
//	os_printf("要写的具体位置：%d\r\n",scetor_addr);
	for(i=0; i<size; i++){//将要写的数据写入缓存数组中
		temp_arr[scetor_addr+i] = *src_addr++;
	}
//	//调试用打印输出
//	os_printf("要写入的数据：%s\r\n",temp_arr);
	i = spi_flash_write(sector_num*SECTOR_SIZE,(uint32 *)temp_arr,sizeof(temp_arr));//将缓存到数组的数据写入flash
//	//调试用打印输出
//	os_printf("写入的结果：%d\r\n",i);
	return i;
}

void user_router_erase(void){
	struct softap_config user_softap_conf;

	os_memset(user_softap_conf.ssid,0,sizeof(user_softap_conf.ssid));
	os_memset(user_softap_conf.password,0,sizeof(user_softap_conf.password));
	//调试用打印输出
	os_printf("ap_ssid：%s\r\n",user_softap_conf.ssid);
	os_printf("ap_password：%s\r\n",user_softap_conf.password);
	user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+SSID_ADDR,(uint8*)user_softap_conf.ssid,sizeof(user_softap_conf.ssid));
	user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+PASSKEY_ADDR,(uint8*)user_softap_conf.password,sizeof(user_softap_conf.password));
}


