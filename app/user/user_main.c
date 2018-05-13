/*
 * user_main.c
 *
 *  Created on: 2016��11��11��
 *      Author: billwang
 */

//#include "driver/uart.h"
#include "user_main.h"
#include "mem.h"

void ICACHE_FLASH_ATTR init_net(void);

struct station_config user_ap_config;
uint8 user_ap_bssid[18]={0};
os_timer_t run_timer;
os_timer_t check_timer;
os_timer_t erase_timer;

//uint8 WORK_MODE;//�豸��ǰ����ģʽ��־
WORK_MODE_T WORK_MODE;
struct ip_addr user_sever_ip;
extern uint8 wifi_userconfig_flag;
int pre_addr = 0;//�������������������

//const uint8 DEVICE_ID[] = "6301709190000035";
uint8 DEV_ID[9] = {0};
uint8 ROU_ID[9] = {0};

void user_init(){
	uint32 temp;
	uint8 flag;

	uart_init(BIT_RATE_115200,BIT_RATE_115200);
	wifi_station_set_auto_connect(0);//�ر��ϵ��Զ�����·�ɹ���
	wifi_station_set_reconnect_policy(0);//�رմ�AP�Ͽ����Զ�����
	io_init();

//	user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+DEVICE_ID_ADDR,\
//						(uint8*)DEVICE_ID,sizeof(DEVICE_ID));
//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+SSID_ADDR,(uint32*)user_ap_config.ssid,\
//					   sizeof(user_ap_config.ssid));
//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+BSSID_ADDR,(uint32*)user_ap_bssid,\
//				   sizeof(user_ap_bssid));
//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+PASSKEY_ADDR,(uint32*)user_ap_config.password,\
//				   sizeof(user_ap_config.password));
//	user_sever_ip.addr = (uint32)0xbc841b78;//"120.27.132.188";  ��ǰ��д���ģ�����Ҫ��Ϊ�û����õ�

//	if((user_ap_config.ssid!=NULL)&&(user_ap_config.ssid[0]!=0xff)&&(user_ap_config.ssid[0]!=0x00)){
//		WORK_MODE = STATION_MODE;
//	}
//	else{
//		WORK_MODE = SOFTAP_MODE;
//		wifi_userconfig_flag = TRUE;
//	}

	init_net();

	temp = system_get_userbin_addr();
	if(0x01000==temp){
		pre_addr = 129;
		os_printf("\r\n��ǰ���г���Ϊ��0x%x\r\n",temp);
	}
	else{
		pre_addr = 1;
		os_printf("\r\n��ǰ���г���Ϊ��0x%x\r\n",temp);
	}
	//��������������Ա��û���������ʱֱ��д�루����ڷ���ָ������ʧ�ܵķ��գ�
	spi_flash_read((USER_UPDATE_ERA_FALG_ADDR*SECTOR_SIZE),\
				   (uint32*)&flag,1);
	if(1 != flag){
		//�������������в�����ռ�ô���ʱ�䣬��Щflash�����������ᵼ������        ע��flash��������
		os_timer_disarm(&erase_timer);
		os_timer_setfn(&erase_timer, earse_process , NULL);
		os_timer_arm(&erase_timer,100,1);
	}
	//ע��ģʽ������
	os_timer_disarm(&run_timer);
	os_timer_setfn(&run_timer, mode_process , NULL);
	os_timer_arm(&run_timer,10,1);
	//ע��10ms��ʱ��
	os_timer_disarm(&check_timer);
	os_timer_setfn(&check_timer, timer_process , NULL);
	os_timer_arm(&check_timer,10,1);
}

void ICACHE_FLASH_ATTR earse_process(void){
	static uint8 i;
	uint8 flag;

	if(i<CODE_SCETOR_SIZE){
		spi_flash_erase_sector(pre_addr+i);
		i++;
		os_printf("i= %d\r\n",i);
	}
	if(i == CODE_SCETOR_SIZE){
		flag = 1;
		user_spi_flash_write((USER_UPDATE_ERA_FALG_ADDR*SECTOR_SIZE),\
							(uint8*)&flag,1);
		os_timer_disarm(&erase_timer);
	}
}

void ICACHE_FLASH_ATTR io_init(void){
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,FUNC_GPIO14);//MTDI����ΪGPIO14
	gpio_output_set(BIT14, 0, BIT14, 0);//����ߵ�ƽ   LED_R
//	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);//UOTXD����ΪGPIO1 ����������
//	gpio_output_set(0, 0, 0, BIT1);//����GPIO1Ϊ����  ������
	gpio_output_set(BIT4, 0, BIT4, 0);//����ߵ�ƽ   LED_G
	gpio_output_set(0, 0, 0, BIT5);//����GPIO5Ϊ����  BUTTON2
//	gpio16_input_conf();//����GPIO16Ϊ����  IR-SENSOR  GPIO16��һ�������IO��Ҫ�õ�ר������
}

void ICACHE_FLASH_ATTR init_net(void){
	uint8 i;

//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+DEVICE_ID_ADDR,\
//								   (uint32*)DEV_ID,ID_LENGTH);
//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+ROUTER_ID_ADDR,\
//									   (uint32*)ROU_ID,ID_LENGTH);
//	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+SEVER_ADDR,\
//					   (uint32*)&user_sever_ip.addr,sizeof(user_sever_ip.addr));

//	for(i = 0; i<8; i++){
//		if((ROU_ID[i] != 0xff) || (ROU_ID[i] != 0x00))
//			break;
//	}
//
//	os_printf("\r\n test id count i = %d\r\n", i);
//	if(i == 8){
//		set_process_mode(ROUTER_CONFIG);
//		os_printf("NO ROUTER INIT !\r\n");
//		return;
//	}
//	if((user_sever_ip.addr == 0xffffffff) || (user_sever_ip.addr == 0x00)){
//		set_process_mode(SEVER_CONFIG);
//		os_printf("NO SEVER INIT !\r\n");
//		return;
//	}
	set_process_mode(NET_CONNECT);

	os_memset(&user_ap_config, 0, sizeof(&user_ap_config));
	os_sprintf(user_ap_config.ssid,"DANZLE-LAB-%s",ROU_ID);
	os_sprintf(user_ap_config.password,"DANZLE-LAB-%s",ROU_ID);

	//���Դ�ӡ
	os_printf("user_ap_config.ssid��%s\r\n",user_ap_config.ssid);
	os_printf("user_ap_config.password��%s\r\n",user_ap_config.password);
	os_printf("device_ID��%s\r\n",DEV_ID);
	os_printf("device_ID��%s\r\n",ROU_ID);
	os_printf("sever_ip��%x\r\n",user_sever_ip.addr);
}


void user_rf_pre_init(){}


