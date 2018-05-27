/*
 * user_timer.c
 *
 *  Created on: 2016��11��18��
 *      Author: billwang
 */

#include "user_timer.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "osapi.h"
#include "user_spi.h"
#include "user_light.h"

uint16 led_g_count = 0;
uint16 led_r_count = 0;
uint8 led_g_flag = FALSE;
uint8 led_r_flag = FALSE;
uint8 key_press_flag = FALSE;
uint8 get_time_stamp_flag = FALSE;
uint8 send_heartbeat_falg = FALSE;

uint16 key_press_cnt = 0;
uint8 blink_mode = 0;//led��˸��ģʽ��־
uint16 blink_max_cnt = 0xffff;//led��˸Ƶ�ʿ���
uint16 count_10ms = 0;//10ms����
uint16 count_1s_count = 0;//1s��ʱ���
uint16 count_timer = 0;//��������
uint16 count_up_status = 0;//��ʱ�ϴ��豸״̬����
uint8 count_app_status = 0;//��ʱ��app����״̬��Ϣ����
uint32 count_device_run_time = 0;//�豸����ʱ��
uint32 time_stamp;

uint16 recon_ap_delay = 0;//����·����ʱ
uint16 recon_sever_delay = 0;//������������ʱ
uint16 rejoin_sever_dalay = 0;//���µ�¼��������ʱ

extern uint16 ping_outtime_count;
extern uint8 WORK_MODE;
extern uint8 wifi_userconfig_flag;
extern timer timers;
extern uint8 sever_connect_flag;
extern uint8 sever_login_flag;
extern uint8 send_status_count;
extern struct espconn user_tcp_conn;

void ICACHE_FLASH_ATTR timer_process(void){
	static uint32 timer_count;

	timer_count++;
	if(count_10ms!=0xffff)count_10ms++;
	if(count_1s_count!=0xffff)count_1s_count++;
	if((led_g_count!=0x0) || (led_g_count!=0xfffff))led_g_count++;
	if(led_r_flag) led_r_count++;
	if((recon_ap_delay!=0x0) || (recon_ap_delay!=0xfffff)) recon_ap_delay++;
	if(!sever_connect_flag) recon_sever_delay++;
	if((rejoin_sever_dalay!=0x0) || (rejoin_sever_dalay!=0xfffff)) rejoin_sever_dalay++;
	if(key_press_flag) key_press_cnt++;
	if(sever_login_flag) count_up_status++;

	if(!(timer_count % 100)){//1s
		ping_outtime_count++;
		send_status_count++;
		count_device_run_time++;
		time_stamp++;

//		count_app_status++;
//		if((timers.seconds++)>=60){
//			timers.seconds=0;
//			timers.minutes++;
//		}
//		if(timers.minutes >= 60){
//			timers.minutes =0;
//			timers.hours++;
//		}
//		if(timers.hours >=24){
//			timers.hours=0;
//			timers.week++;
//		}
//		if(timers.week >6){
//			timers.week=0;
//		}
	}
	if(!(timer_count % (100*30))){
		send_heartbeat_falg = TRUE;
	}
	if(!(timer_count % (100*50))){
		get_time_stamp_flag = TRUE;
	}

	led_blink();
//	key_check();
}

void ICACHE_FLASH_ATTR led_blink(void){
	if(blink_mode != BLINK_NO_WORK){
		switch(blink_mode){
			case BLINK_AP_CONF:
				blink_max_cnt = 5;//50ms  ��˸Ƶ��10Hz
				break;
			case BLINK_AP_CONCT:
				blink_max_cnt = 10;//100ms  ��˸Ƶ�� 5Hz
				break;
			case BLINK_SEV_CONCT:
				blink_max_cnt = 25;//250ms  ��˸Ƶ�� 1Hz
				break;
			case BLINK_CODE_UPDATA:
				blink_max_cnt = 100;
				break;
			case BLINK_NO_WORK:
				blink_max_cnt = 0xffff;
				break;
			default:
				break;
		}
		if(led_g_count > blink_max_cnt){
			led_g_count = 1;
//			os_printf("������˸��\r\n");
			if(led_g_flag){
//				GPIO_OUTPUT_SET(BIT4, 1);
//				GPIO_OUTPUT_SET(BIT14, 0);
				gpio_output_set(BIT4, 0, BIT4, 0);
				led_g_flag = FALSE;
			}
			else{
//				GPIO_OUTPUT_SET(BIT4, 0);
//				GPIO_OUTPUT_SET(BIT14, 1);
				gpio_output_set(0, BIT4, BIT4, 0);
				led_g_flag = TRUE;
			}
		}
	}
	else{//�Ѿ�������������״̬����ʾ�Ʊ�֤�����״̬
		if(led_g_flag && (key_press_flag == FALSE)){
			led_g_flag = FALSE;
			gpio_output_set(BIT4, 0, BIT4, 0);
		}
	}

	if(led_r_flag){
		if(led_r_count > 100){
			led_r_flag = FALSE;
			led_r_count = 0;
			gpio_output_set(BIT14, 0, BIT14, 0);
		}
	}
}

void ICACHE_FLASH_ATTR key_check(void){
	static press_status = 0;//����״̬
	uint8 test_pin=0;

	if(!GPIO_INPUT_GET(GPIO_ID_PIN(5))){
		key_press_flag = TRUE;
	}
	else{
		if(TRUE == key_press_flag){
			if(key_press_cnt>200){
				wifi_userconfig_flag = TRUE;//�û����°���������־
				user_router_erase();
				espconn_disconnect(&user_tcp_conn);
				wifi_station_disconnect();//�Ͽ�·������
				//�����������ʱapp�˲��ܻ�ȡ��ip����
				//����process��
				//system_restart();
				WORK_MODE = DELAY_MODE;
				os_printf("����������\r\n");
			}
			else{
				os_printf("����ȡ����\r\n");
			}
			press_status = 0;
			key_press_flag = FALSE;
			key_press_cnt = 0;
		}
	}
	if(TRUE == key_press_flag){
		if(!press_status){
			if(key_press_cnt>200){
				press_status = 1;
				led_g_flag = TRUE;
				gpio_output_set(0, BIT4, BIT4, 0);
				test_pin = GPIO_INPUT_GET(GPIO_ID_PIN(4));
				os_printf("�а������£���ֵ��%d\r\n",test_pin);

				blink_mode = BLINK_NO_WORK;
			}
		}
	}
}





