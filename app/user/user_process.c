/*
 * user_process.c
 *
 *  Created on: 2016��11��16��
 *      Author: billwang
 */

#include "user_process.h"
#include "user_interface.h"
#include "user_spi.h"
#include "spi_flash.h"
#include "espconn.h"
#include "osapi.h"
#include "mem.h"
#include "ip_addr.h"
#include "user_timer.h"
//#include "gpio.h"
#include "driver/uart.h"
#include "user_light.h"
#include "gpio16.h"

struct espconn user_tcp_conn;     		//�����������TCP����
struct espconn user_udp_espconn;		//udp�㲥
//struct espconn updata_tcp_conn;
static linkConType pLink[LINK_MAX];   	//�ֻ����豸����TCP����
static struct espconn *pTcpServer;		//����soft_ap sever��������
os_timer_t updata_timer;

uint8 tcp_send_buff[256];
uint8 tcp_rec_buff[1500];
static uint16_t server_timeover = 180;
static uint8 sta_count = 0;
uint8 at_buff[256] = {0};//ATָ���
char updataIP[16];//���ڴ洢����ʱ��IP
char updataPort[5];//���ڴ洢����ʱ�ĵĶ˿ں�
//���汾��.�ΰ汾��.�޶��汾��.���ڰ汾��.ϣ����ĸ�汾��
//X.Y.Z �����汾��.�ΰ汾��.�޶��ţ�
/*
 * 17.3.9 �������ݣ�1.�����˿��ƺв�ѯ״̬ʱ����ӡ���ص�״̬��Ϣ
 * 				  2.�޸�������������Ͽ�����ʱֻ���� espconn_connect
 */
const uint8_t software_ver[] = "0.1.1";
const uint8_t if_ver[] = "0.1.1";

uint8 wifi_userconfig_flag = FALSE;//�״��ϵ�����û��Զ��л����������־
uint8 wifi_connect_flag = FALSE;// ·�����ӱ�־
uint8 sever_connect_flag = FALSE;//���������ӱ�־
uint8 sever_login_flag = FALSE;//��������¼��־
uint8 tcp_sent_flag = FALSE;//TCP������ɱ�־
//uint8 rec_ping_flag = FALSE;//���յ��������ݱ�־
uint8 rec_severdata_falg = FALSE;//���յ����������ݱ�־
uint8 rec_updataseverdata_flag = FALSE;//���յ��������������ݱ�־
uint8 rec_appdata_falg = FALSE;//���յ�app���ݱ�־
uint8 updata_flag = FALSE;//�̼�������־
uint8 cb_updata_flag = FALSE;//���ƺ�������־
uint8 updata_sent_flag = FALSE;//�̼�����׼�������������ݳɹ���־
uint8 updata_start_conncet_flag = FALSE;//�̼�����ʱ��ԭ���������Ͽ���־
uint8 updata_sever_connect_flag = FALSE;//�̼�����ʱ���������������ӱ�־
uint8 tt_send_sever_flag = FALSE;//�����������͸�����ݱ�־
uint16 ping_outtime_count = 0;//δ�յ��������ݼ�ʱ
uint8 send_status_count = 0;//��MCU����״̬ʱ�����
static uint8 linkNum = 0;
//extern uint16 recon_ap_delay;
extern uint16 recon_sever_delay;
extern uint16 rejoin_sever_dalay;
extern uint8 blink_mode;
extern uint16 led_g_count;
extern os_timer_t run_timer;
extern os_timer_t check_timer;
//extern timer timers;
//extern uint8 count_app_status;
//extern timer timers;

extern uint8 WORK_MODE;
extern struct ip_addr user_sever_ip;
//extern ip_addr_t *user_sever_ip;
extern struct station_config user_ap_config;
extern uint8 user_ap_bssid[18];
extern uint8 DEV_ID[9];
extern uint8 ROU_ID[9];
extern uint8 ttcmd_buff[TT_CMD_LEN];

void ICACHE_FLASH_ATTR user_login_sever(void){
	struct ip_info info;
	uint8 mac_addr[6]={0};

	wifi_get_ip_info(STATION_IF,&info);
	wifi_get_macaddr(STATION_IF,mac_addr);

	os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
	os_sprintf(tcp_send_buff,
			"{\"Name\":\"Login\",\"DeviceId\":\"%s\",\"Mac\":\""MACSTR"\",\"SWVersion\":\"%s\",\"SWVersion\":\"%s\"}\r\n",
			DEV_ID,MAC2STR(mac_addr),software_ver,if_ver);
	espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
}

void ICACHE_FLASH_ATTR user_send_basicinfor(void){
	struct ip_info info;
	uint8 mac_addr[6]={0};

	wifi_get_ip_info(STATION_IF,&info);
	wifi_get_macaddr(STATION_IF,mac_addr);
	os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
	os_sprintf(tcp_send_buff,
			"{\"info\":{\"wifi\":\"%s\",\"bssid\":\"%s\",\"mac\":\""MACSTR"\",\"version\":\"%s\",\"ip\":\"%d.%d.%d.%d\"}}\r\n",
			user_ap_config.ssid,user_ap_bssid,MAC2STR(mac_addr),software_ver,IP2STR(&info.ip));
	espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
	os_printf("���͸��������豸��Ϣ��%s\r\n",tcp_send_buff);
}

void ICACHE_FLASH_ATTR updata_ack_sever(char result){
	os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
	os_sprintf(tcp_send_buff,"%c%c%c",result,BOOT_END1,BOOT_END2);
	//���Դ�ӡ
	os_printf("���ͷ�����%s\r\n",tcp_send_buff);
	espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
}

void ICACHE_FLASH_ATTR updata_romdata(char *pdata){
	static uint8 step = 0;
	static uint32 updata_addr;
	uint32 flash_ear_falg=0;//�������������falsh��־
	static uint32 arr_cnt = 0;//һ֡128byte bin���� ���湻4096byte��д��flash
	uint8 i,verify = 0;
	uint8 temp_arr[128];
	char *buff_arr;

	buff_arr = pdata;
	switch(step){
		case 0:
			if(BOOT_HEAD == buff_arr[0]){//���յ�������ͷ
				os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
				os_sprintf(tcp_send_buff,"%c%c%c",BOOT_OK,BOOT_END1,BOOT_END2);
				//���Դ�ӡ
				os_printf("�յ�����ͷ���ݣ������������ݣ�%s\r\n",tcp_send_buff);
				espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
				step++;
				//�������falsh������־
				user_spi_flash_write((USER_UPDATE_ERA_FALG_ADDR*SECTOR_SIZE),\
									(uint8*)&flash_ear_falg,1);
			}

			if(0x01000 == system_get_userbin_addr()) updata_addr = 129;
			else updata_addr = 1;
			break;
		case 1:
			switch(buff_arr[0]){
				case BOOT_GO:
					os_printf("�յ���תָ��\r\n");
					system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
					updata_ack_sever(BOOT_OK);
					system_upgrade_reboot();
					break;
				case BOOT_VERIFY:
					verify = 0;
					os_printf("�յ�У��ָ��\r\n");
					updata_ack_sever(BOOT_OK);
					break;
				case BOOT_WRITE:
					verify = 0;
					for(i=0; i<128; i++){//�����
						verify += buff_arr[i+1];
					}
					if(verify == buff_arr[129]){//У��ɹ�
//						for(i=0; i<128; i++){
//							temp_arr[arr_cnt+i] = buff_arr[i+1];
//						}
						os_memcpy(temp_arr,&buff_arr[1],sizeof(temp_arr));
						//���Դ�ӡ
						os_printf("���յ���bin���ݣ�%s\r\n",temp_arr);
						spi_flash_write((uint32)(updata_addr*SECTOR_SIZE)+arr_cnt,(uint32*)temp_arr,(uint32)128);
						arr_cnt = (arr_cnt+128);
						//д��flash��ɣ�����������
						updata_ack_sever(BOOT_OK);
					}
					else{//У��ʧ�ܣ��������������·���
						os_printf("���յ���bin���ݴ��������ط�\r\n");
						updata_ack_sever(BOOT_ERR);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

void ICACHE_FLASH_ATTR updata_tcp_recv_cb(void *arg,char *pdata,unsigned short len){
	os_printf("�յ��������������ݣ�%s\r\n",pdata);
	rec_updataseverdata_flag = TRUE;
	os_memset(tcp_rec_buff,0,sizeof(tcp_rec_buff));
	os_memcpy(tcp_rec_buff,pdata,len);
}

void ICACHE_FLASH_ATTR updata_tcp_sent_cb(void *arg){
	os_printf("�������������������ݳɹ���\r\n");
	//if(WORK_MODE == CBUPDATA_MODE) uart0_sendStr("OK\r\n");
}

void ICACHE_FLASH_ATTR updata_tcp_discon_cb(void *arg){
	os_printf("��Զ�������������Ͽ����ӳɹ���\r\n");
}

void ICACHE_FLASH_ATTR user_updata_recon_cb(void *arg, sint8 err){
	static recon_status = 0;

	//���������жϿ����ӣ�����ʧ�ܣ�������δ�ɹ���־
	//system_upgrade_flag_set(UPGRADE_FLAG_START);
	if(UPGRADE_FLAG_FINISH != system_upgrade_flag_check()){
//		updata_start_conncet_flag = TRUE;
		os_printf("Զ���������������Ӵ��󣬴������Ϊ%d\r\n",err);
		system_restart_enhance(0, system_get_userbin_addr());
	}
	else
		os_printf("Զ��������ɣ��Ͽ�����������ӣ�\r\n");
}

void ICACHE_FLASH_ATTR user_updata_connect_cb(void *arg){
	struct espconn *pespconn=arg;
	uint8 i;
	os_printf("Զ���������ӷ������ɹ���\r\n");

	updata_sever_connect_flag = TRUE;//���������ӳɹ���־��һ
//	blink_mode = BLINK_NO_WORK;
//	led_g_count = 0;

	espconn_regist_recvcb(pespconn,updata_tcp_recv_cb);
	espconn_regist_sentcb(pespconn,updata_tcp_sent_cb);
	espconn_regist_disconcb(pespconn,updata_tcp_discon_cb);
//	os_printf("ע��Զ�������������յ����ݻص������ɹ���\r\n");
	//espconn_sent(pespconn,HEAD,strlen(HEAD));
}

void ICACHE_FLASH_ATTR updata_station_init(int remote_port){
	os_printf("��ʼ����Զ�������������ṹ��\r\n");
	//espconn��������
	user_tcp_conn.proto.tcp->remote_port=remote_port;
	//ע�����ӻص������������ص�����
	espconn_regist_connectcb(&user_tcp_conn,user_updata_connect_cb);
	espconn_regist_reconcb(&user_tcp_conn,user_updata_recon_cb);
	//��������
	espconn_connect(&user_tcp_conn);
}

uint8 ICACHE_FLASH_ATTR user_updata_send_check(void){
	static uint8 i=0;

	i++;
	if(updata_sent_flag){//����ready�ɹ���׼���Ͽ���ǰ����������
		updata_sent_flag = FALSE;
		//�Ͽ���ǰ��������
		espconn_disconnect(&user_tcp_conn);//�Ͽ���ǰ����������
		//�Ͽ���ǰ�ֻ�����
		for(i=0; i<3; i++){
			//�����ã���ӡ�����Ϣ
			os_printf("�ѽ��������ӣ�%x\r\n",pLink[i].linkId);
			if(TRUE == pLink[i].linkEn){
				while(espconn_disconnect(pLink[i].pCon));
				//�����ã���ӡ�����Ϣ
				os_printf("�Ͽ����ӣ�%x\r\n",pLink[i].linkId);
			}
		}
		return 0;
	}

	if(i>500){//5s��δ���ͳɹ���Ϊʧ��
		os_printf("���ͷ�����׼��update����ʧ��!\r\n");
		system_restart();
		return 2;
	}
	return 1;
}

void ICACHE_FLASH_ATTR user_updata_set(void){
	os_sprintf(tcp_send_buff,"{\"updata\":\"ready\"}\r\n");
	espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
	os_memset(tcp_send_buff,0,strlen(tcp_send_buff));
}

void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg){
	os_printf("��������������ݳɹ���\r\n");

	if(tt_send_sever_flag){
		tt_send_sever_flag = FALSE;
		uart0_sendStr("OK\r\n");
	}

	if(updata_flag){
		updata_sent_flag = TRUE;
	}
}
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg){
	os_printf("��������Ͽ����ӳɹ���\r\n");
	if(updata_flag){
		os_printf("��ԭ�������Ͽ����ӳɹ�����������������������\r\n");
		updata_start_conncet_flag = TRUE;
	}
	//�����־
	sever_connect_flag = FALSE;
	recon_sever_delay = 6000;//��⵽�Ͽ���������������
	sever_login_flag = FALSE;
}
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg,char *pdata,unsigned short len){
	char* p_result = NULL;
	uint8 i;
	int result;

	os_printf("�յ����������ݣ�%s\r\n",pdata);
	p_result = strstr(pdata, (const char *)"Success");
	if(p_result != NULL){
		sever_login_flag = TRUE;
		rejoin_sever_dalay = 0;
		//��¼�������ɹ�������wifi��Ϣ��mac��ַ���̼��汾��
		user_send_basicinfor();
		return;
	}
	p_result = strstr(pdata, (const char *)"updata");
	if(p_result != NULL){
		os_printf("�յ�����ָ��\r\n");
//		for(i=0;i<15;i++){//���˵�updata��ip����֮ǰ������
//			if(p_result[i+9]!=0x22){//���ݲ�Ϊ"������ȡ����
//				updataIP[i]= p_result[i+9];
//			}
//			else
//				break;
//		}
//		p_result = strstr(pdata , (const char *)"port");
//		if(p_result != NULL){
//			for(i=0;i<5;i++){//���˵�updata��ip����֮ǰ������
//				if(p_result[i+7]!=0x22){//���ݲ�Ϊ"������ȡ����
//					updataIP[i]= p_result[i+7];
//				}
//				else{
//					updataPort[i] = '\0';
//					  break;
//				}
//			}
//		}
		updata_flag = TRUE;
		WORK_MODE = UPDATA_MODE;
		os_printf("����Ϊupdataģʽ\r\n");
		return;
	}
	rec_severdata_falg = TRUE;
	os_memcpy(tcp_rec_buff,pdata,strlen(pdata));
}
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err){
	struct ip_info sta_info;

	os_printf("���Ӵ��󣬴������Ϊ%d\r\n",err);
	if(updata_flag) system_restart(); //����ʱ�������ʧ�� ���粻�ȶ� ����ϵͳ

	//�����ж��Ƿ��·�ɶϿ�������
	wifi_get_ip_info(STATION_IF, &sta_info);
	if(sta_info.ip.addr == (int)0){
		wifi_connect_flag = FALSE;
		wifi_station_disconnect();
	}

	sever_connect_flag = FALSE;
	sever_login_flag = FALSE;
}

void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg){
	struct espconn *pespconn=arg;
	uint8 i;
	os_printf("���ӵķ�������link��%d\r\n",pespconn->link_cnt);

	sever_connect_flag = TRUE;//���������ӳɹ���־��һ
	blink_mode = BLINK_NO_WORK;
	led_g_count = 0;

	espconn_regist_recvcb(pespconn,user_tcp_recv_cb);
	espconn_regist_sentcb(pespconn,user_tcp_sent_cb);
	espconn_regist_disconcb(pespconn,user_tcp_discon_cb);
	//espconn_sent(pespconn,HEAD,strlen(HEAD));
}

void ICACHE_FLASH_ATTR my_station_init(struct ip_addr *remote_ip,struct ip_addr *local_ip,int remote_port){
	//espconn��������
	user_tcp_conn.type=ESPCONN_TCP;
	user_tcp_conn.state=ESPCONN_NONE;
	user_tcp_conn.proto.tcp=(esp_tcp *)os_zalloc(sizeof(esp_tcp));
	os_memcpy(user_tcp_conn.proto.tcp->local_ip,local_ip,4);
	os_memcpy(user_tcp_conn.proto.tcp->remote_ip,remote_ip,4);
	user_tcp_conn.proto.tcp->local_port=espconn_port();
	user_tcp_conn.proto.tcp->remote_port=remote_port;

	//�����ã���ӡ��Ҫ���ӵ���Ϣ
	os_printf("sever_ip_information��%u\r\n",user_tcp_conn.proto.tcp->remote_ip[0]);
	os_printf("sever_ip_information��%u\r\n",user_tcp_conn.proto.tcp->remote_ip[1]);
	os_printf("sever_ip_information��%u\r\n",user_tcp_conn.proto.tcp->remote_ip[2]);
	os_printf("sever_ip_information��%u\r\n",user_tcp_conn.proto.tcp->remote_ip[3]);
	os_printf("dev_ip_information��%u\r\n",user_tcp_conn.proto.tcp->local_ip[0]);
	os_printf("dev_ip_information��%u\r\n",user_tcp_conn.proto.tcp->local_ip[1]);
	os_printf("dev_ip_information��%u\r\n",user_tcp_conn.proto.tcp->local_ip[2]);
	os_printf("dev_ip_information��%u\r\n",user_tcp_conn.proto.tcp->local_ip[3]);
	os_printf("sever_port_information��%u\r\n",user_tcp_conn.proto.tcp->remote_port);

	//ע�����ӻص������������ص�����
	espconn_regist_connectcb(&user_tcp_conn,user_tcp_connect_cb);
	espconn_regist_reconcb(&user_tcp_conn,user_tcp_recon_cb);
	//��������
	espconn_connect(&user_tcp_conn);
}

void ICACHE_FLASH_ATTR user_tcpclient_recv(void *arg, char *pdata, unsigned short len){
	char* p_result = NULL;
	struct espconn *pespconn = (struct espconn *)arg;
	uint16 i,j;
	uint8 temp_sever_ip[4]={0};

	p_result = strstr(pdata, (const char *)"updata");
	if(p_result != NULL){
		os_printf("�յ�����ָ��\r\n");
		WORK_MODE = UPDATA_MODE;
		user_updata_set();
		updata_flag = TRUE;
		return;
	}
#if 1
	p_result = strstr( (const char *)pdata , (const char *)"server_ip");
	for(i=0,j=0; j<4; j++){
		while((p_result[i+12] != '.') && (p_result[i+12] != '"')){
			temp_sever_ip[j] = ((temp_sever_ip[j]*10) + (p_result[i+12]-0x30));
			i++;
//				os_printf("p_result��%d\r\n",p_result[i+12]);
//				os_printf("�ڣ�%d���ۼӣ�%d\r\n",j,temp_sever_ip[j]);
		}
		i++;//�����м�� ��'.'
//			os_printf("�յ���������ַ��%d\r\n",temp_sever_ip[j]);
	}
	IP4_ADDR(&user_sever_ip, temp_sever_ip[0], temp_sever_ip[1], temp_sever_ip[2], temp_sever_ip[3]);
#endif
	os_printf("�յ�app���ݣ�%s\r\n",pdata);
	rec_appdata_falg = TRUE;
	os_memcpy(tcp_rec_buff,pdata,strlen(pdata));
}

void ICACHE_FLASH_ATTR user_tcpserver_recon_cb(void *arg, sint8 errType){
	os_printf("�ֻ��������󣬴�����룺%d\r\n",errType);

	//�����粻�ȶ����߲���Ƶ��ʱ����ɸô��󣬳��ָô���ʱ��ֱ�ӶϿ�����
	struct espconn *pespconn = (struct espconn *)arg;
	linkConType *linkTemp = (linkConType *)pespconn->reverse;

	espconn_disconnect(pespconn);
	linkTemp->linkEn = FALSE;
	linkTemp->pCon = NULL;
}

void ICACHE_FLASH_ATTR user_tcpserver_discon_cb(void *arg){
	uint8 i;

	struct espconn *pespconn = (struct espconn *) arg;
	linkConType *linkTemp = (linkConType *) pespconn->reverse;
	linkTemp->linkEn = FALSE;
	linkTemp->pCon = NULL;
//	i = pespconn->link_cnt;
//	������ӵ��ֻ�TCP�ṹ��
//	os_memset(&app_tcp_arr[i],0,sizeof(struct espconn));
//	app_tcp_arr[i].link_cnt = 255;

	os_printf("�ֻ��Ͽ����ӳɹ�,linkID��%d\r\n",linkTemp->linkId);
}

void ICACHE_FLASH_ATTR user_tcpclient_sent_cb(void *arg){
	os_printf("���ֻ��������ݳɹ�\r\n");
}

void ICACHE_FLASH_ATTR user_tcpserver_listen(void *arg){
	uint8 i;

	struct espconn *pespconn = (struct espconn *)arg;
	os_printf("���ӵ�app��link��%d\r\n",pespconn->link_cnt);
	for(i=0;i<LINK_MAX;i++)
	{
		if(pLink[i].linkEn == FALSE)
		{
			pLink[i].linkEn = TRUE;
			break;
		}
	}
	if(i>=3)
	{
		return;
	}
	//д�����ӵ����ֻ�tcp�ṹ��
//	if(pespconn->link_cnt < 3){
//		i = pespconn->link_cnt;
//		os_printf("espconn�ṹ��ĳ���:%d\r\n",sizeof(struct espconn));
//		memcpy(&app_tcp_arr[i],arg,sizeof(struct espconn));
//	}
//	else
//		os_printf("link error!\r\n");
	//���²ο�ATָ���еĲ��� ��ȡlinkID
	pLink[i].teToff = FALSE;
	pLink[i].linkId = i;
	pLink[i].teType = teServer;
	pLink[i].repeaTime = 0;
	pLink[i].pCon = pespconn;
	linkNum++;
	pespconn->reverse = &pLink[i];

	os_printf("app��linkID:%d\r\n",pLink[i].linkId);
	espconn_regist_recvcb(pespconn, user_tcpclient_recv);
	espconn_regist_reconcb(pespconn, user_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, user_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, user_tcpclient_sent_cb);
}

void ICACHE_FLASH_ATTR router_connect(void){
	uint8 sta_status = 0;
	uint8 i;
	static uint32 sta_times = 0;
	struct ip_info info;
	static struct espconn *pTcpServer;

	switch(sta_count){
		case 0://����STA ����·��
			if(STATION_MODE != wifi_get_opmode()) wifi_set_opmode(STATION_MODE);
			os_printf("\r\nDevice will connect AP \r\n");

			wifi_station_disconnect();
			os_memset(&user_ap_config, 0, sizeof(&user_ap_config));
			os_sprintf(user_ap_config.ssid,"DANZLE-LAB-%s",ROU_ID);
			os_sprintf(user_ap_config.password,"DANZLE-LAB-%s",ROU_ID);

			os_printf("user_ap_config.ssid��%s\r\n",user_ap_config.ssid);
			os_printf("user_ap_config.password��%s\r\n",user_ap_config.password);

			wifi_station_set_config_current(&user_ap_config);
			wifi_station_connect();
			blink_mode = BLINK_AP_CONCT;
			led_g_count = 1;
			sta_count++;
			break;
		case 1://�������·���Ƿ�ɹ�
			sta_times++;
			sta_status = wifi_station_get_connect_status();
			if(STATION_GOT_IP == sta_status){
				wifi_connect_flag = TRUE;	//����·�ɳɹ���־
				wifi_userconfig_flag = FALSE;//���״����ñ�־
				os_printf("Wifi connect success!\r\n");
				sta_times = 0;
				sta_count = 0;
				//��������
				pTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
				pTcpServer->type = ESPCONN_TCP;
				pTcpServer->state = ESPCONN_NONE;
				pTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
				pTcpServer->proto.tcp->local_port = LISTEN_PORT;
				espconn_regist_connectcb(pTcpServer, user_tcpserver_listen);
				espconn_accept(pTcpServer);
				espconn_tcp_set_max_con_allow(pTcpServer,LINK_MAX);//��������������ӵ��ֻ��� 3
				espconn_regist_time(pTcpServer, server_timeover, 0);

				set_process_mode(SEVER_CONFIG);
			}
			else{
				if(sta_times > 3000){//30sû�����ϣ���Ϊʧ��
//					if(wifi_userconfig_flag){//������û���������
//						os_printf("Wifi connect fail��\r\n");
//						wifi_connect_flag = FALSE;
//						WORK_MODE = SOFTAP_MODE;//·����Ϣ�����л���AP��������
//					}//�����û��������ã����������ص�һ�����������Ӹ�·��
					sta_count = 0;
					sta_times = 0;
				}
			}
			break;
		default :
			break;
	}
}

void ICACHE_FLASH_ATTR sever_connect(void){
	uint8 sta_status = 0;
	uint8 i;
	static uint32 sta_times = 0;
	struct ip_info info;
	struct ip_addr *sever_addr;
	static struct espconn *pTcpServer;

	sever_addr = (struct ip_addr *)os_malloc(sizeof(struct ip_addr));
	switch(sta_count){
		case 0://�����������TCP����
			wifi_get_ip_info(STATION_IF,&info);
			//�����ã���ӡ�����Ϣ
			os_printf("device_ip_information��%u\r\n",info.ip);
			os_printf("device_mask_information��%u\r\n",info.netmask);
			os_printf("device_gw_information��%u\r\n",info.gw);
			os_printf("sever_ip_information��%u\r\n",user_sever_ip);
			//��������IP��ֵ���ṹ��
			sever_addr->addr = user_sever_ip.addr;
			my_station_init(sever_addr,&info.ip,(int)SEVER_PORT);
			blink_mode = BLINK_SEV_CONCT;
			led_g_count = 1;
			sta_count++;
			break;
		case 1://��¼������
			sta_times++;
			if(sta_times > 10){//100ms���һ��
				if(sever_connect_flag){
					os_printf("�����������TCP���ӳɹ���\r\n");
					//�����������TCP�������� 5
					if(DEFAULT_TCP_NUM != espconn_tcp_get_max_con()){
						espconn_tcp_set_max_con(DEFAULT_TCP_NUM);
					}
					//��¼������
					user_login_sever();
					sta_times = 0;
					sta_count++;
					//test ...�����õ�
					WORK_MODE = T_T_MODE;
				}
				if(sta_times > 1000){//10sδ�ܵ�¼������,ֱ�ӽ���RUNģʽ
					WORK_MODE = NORMAL_MODE;
					sta_times = 0;
					sta_count = 0;
				}
			}
			break;
		case 2://��¼�������ɹ� ����ģʽ�л���RUNģʽ
			sta_times++;
			if(sta_times > 10){
				if(TRUE == sever_login_flag){
					os_printf("��¼�������ɹ�,����������������\r\n");
					set_process_mode(NORMAL_MODE);
					blink_mode = BLINK_NO_WORK;
					led_g_count = 0;
					gpio_output_set(BIT4, 0, BIT4, 0);
					sta_times = 0;
					sta_count = 0;
				}
			}
			if(sta_times > 1000){//10sδ�ܵ�¼������,ֱ�ӽ���RUNģʽ
				WORK_MODE = NORMAL_MODE;
				sta_times = 0;
				sta_count = 0;
			}
			break;
		default:
			break;
	}

	os_free(sever_addr);
}

void ICACHE_FLASH_ATTR ap_tcpserver_recon_cb(void *arg, sint8 errType){
	os_printf("��������TCP��%s\r\n",arg);
}

void ICACHE_FLASH_ATTR ap_tcpserver_discon_cb(void *arg){
	os_printf("�Ͽ����ӳɹ�����%s\r\n",arg);
}

static void ICACHE_FLASH_ATTR ap_tcpclient_sent_cb(void *arg){
	os_printf("��APP�����豸ID�ɹ�����%s\r\n",arg);

	espconn_disconnect((struct espconn *)arg);
	wifi_softap_dhcps_stop();

	WORK_MODE = STATION_MODE;
	sta_count = 0;
	//���Դ�ӡ
//	os_printf("ssid��%s\r\n",user_ap_config.ssid);
//	os_printf("password��%s\r\n",user_ap_config.password);
//	os_printf("�豸��ǰ�����ڣ�%d\r\n",WORK_MODE);
}

void ICACHE_FLASH_ATTR ap_tcp_recv_cb(void *arg,char *pdata,unsigned short len){
	char* p_result = NULL;
	uint8 i,j;
	uint8 temp_sever_ip[4]={0};
	struct espconn *pespconn = (struct espconn *)arg;

	os_printf("�յ�APP���ݣ�%s\r\n",pdata);
	p_result = strstr((char *)pdata, (char *)"password");
//	os_printf("���ص�p-result��%d\r\n",result);
	if(p_result != NULL){
		os_printf("�ɹ��յ�·���������ݣ�\r\n");
		for(i=0; i<64; i++){
			if(p_result[i+11] != 0x22){
				user_ap_config.password[i] = p_result[i+11];
			}
			else{
				user_ap_config.password[i] = 0;
				break;
			}
		}
		p_result = strstr((char *)pdata, (char *)"ssid");
		for(i=0; i<64; i++){
			if(p_result[i+7] != 0x22){
				user_ap_config.ssid[i] = p_result[i+7];
			}
			else{
				user_ap_config.ssid[i] = 0;
				break;
			}
		}
		p_result = strstr( (const char *)pdata , (const char *)"bssid");  //��ȡBSSID
		for(i=0; i<18; i++){
			if(p_result[i+8] != 0x22){
				user_ap_bssid[i] = p_result[i+8];
			}
			else{
				user_ap_bssid[i] = 0;
				break;
			}
		}
		p_result = strstr( (const char *)pdata , (const char *)"server_ip");
		for(i=0,j=0; j<4; j++){
			while((p_result[i+12] != '.') && (p_result[i+12] != '"')){
				temp_sever_ip[j] = ((temp_sever_ip[j]*10) + (p_result[i+12]-0x30));
				i++;
//				os_printf("p_result��%d\r\n",p_result[i+12]);
//				os_printf("�ڣ�%d���ۼӣ�%d\r\n",j,temp_sever_ip[j]);
			}
			i++;//�����м�� ��'.'
//			os_printf("�յ���������ַ��%d\r\n",temp_sever_ip[j]);
		}
		IP4_ADDR(&user_sever_ip, temp_sever_ip[0], temp_sever_ip[1], temp_sever_ip[2], temp_sever_ip[3]);

		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+SSID_ADDR,
				(uint8*)user_ap_config.ssid,sizeof(user_ap_config.ssid));
		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+BSSID_ADDR,
				(uint8*)user_ap_bssid,sizeof(user_ap_bssid));
		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+PASSKEY_ADDR,
				(uint8*)user_ap_config.password,sizeof(user_ap_config.password));
		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+SEVER_ADDR,
				(uint8*)temp_sever_ip,4);

//		os_printf("user_sever_ip_all: %x\r\n", user_sever_ip.addr);
//		os_printf("user_sever_ip: %d\r\n", temp_sever_ip[0]);
//		os_printf("user_sever_ip: %d\r\n", temp_sever_ip[1]);
//		os_printf("user_sever_ip: %d\r\n", temp_sever_ip[2]);
//		os_printf("user_sever_ip: %d\r\n", temp_sever_ip[3]);

//		user_ap_config.bssid_set=1;
		os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
		os_sprintf(tcp_send_buff,"{\"device_id\":\"%s\"}\r\n",DEV_ID);
		espconn_sent(pespconn,tcp_send_buff,strlen(tcp_send_buff));
	}
	else{
		os_printf("�յ�������������!\r\n");
//		rec_appdata_falg = TRUE;
		os_memcpy(tcp_rec_buff,pdata,strlen(pdata));
	}
}

void ICACHE_FLASH_ATTR user_ap_tcp_connect_cb(void *arg){
	struct espconn *pespconn = (struct espconn *)arg;

	os_printf("�ֻ������豸AP�ɹ���\r\n");
	//pespconn->reverse = &pLink[i];
	espconn_regist_recvcb(pespconn, ap_tcp_recv_cb);
	espconn_regist_reconcb(pespconn, ap_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, ap_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, ap_tcpclient_sent_cb);
}

void ICACHE_FLASH_ATTR ap_process(void){
	uint8 temp_arr[5]={0};
	struct softap_config user_softap_conf;
	struct ip_info softap_ip_info;
	uint8 i;

	if(SOFTAP_MODE != wifi_get_opmode()) wifi_set_opmode(SOFTAP_MODE);  //����Ϊsoft_apģʽ
//	else return;

	blink_mode = BLINK_AP_CONF;//������˸����ʾ
	led_g_count = 1;

	os_bzero(&user_softap_conf, sizeof(struct softap_config));
	spi_flash_read((USER_BASE_ADDR*SECTOR_SIZE)+DEVICE_ID_ADDR+12,(uint32 *)temp_arr,4);
	for(i=0; i<4; i++){
		if((temp_arr[i]<0x30) || (temp_arr[i]>0x39))
		{
			os_memcpy(temp_arr, "9999", 4);
			temp_arr[4] = 0;
		}
	}
	os_sprintf(user_softap_conf.ssid,"ORE_SleepCare_%s",temp_arr);
	user_softap_conf.ssid_len = 0;
	os_sprintf(user_softap_conf.password,"1234567890");
	user_softap_conf.channel = 1;
	user_softap_conf.authmode = AUTH_OPEN;
	user_softap_conf.max_connection = 4;//������㣬Ĭ��4  ���4
	//�����ô�ӡ���
	os_printf("ap_ssid��%s\r\n",user_softap_conf.ssid);
	os_printf("ssid_len��%d\r\n",user_softap_conf.ssid_len);

	wifi_softap_set_config(&user_softap_conf); //����soft_ap����
	wifi_softap_dhcps_start();
	//�����ô�ӡ���
	os_printf("Device is at AP mode\r\n");

//����ȡ������������IP
//	wifi_softap_dhcps_stop();
//	wifi_get_ip_info(0x01, &softap_ip_info);
//	IP4_ADDR(&softap_ip_info.ip, 192, 168, 10, 1);
//	IP4_ADDR(&softap_ip_info.gw, 192, 168, 10, 1);
//	IP4_ADDR(&softap_ip_info.netmask, 255, 255, 255, 0);
//	wifi_set_ip_info(SOFTAP_IF,&softap_ip_info); //����soft_ap��ip 192.168.10.1
//	wifi_softap_dhcps_start();
//	//�����ô�ӡ���
//	struct ip_info pTempIp;
//	wifi_get_ip_info(0x01, &pTempIp);
//	os_printf("%d.%d.%d.%d\r\n",
//	                 IP2STR(&pTempIp.ip));

	pTcpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	pTcpServer->type = ESPCONN_TCP;
	pTcpServer->state = ESPCONN_NONE;
	pTcpServer->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	pTcpServer->proto.tcp->local_port = SOFTAP_SEVER_PORT;
	espconn_regist_connectcb(pTcpServer, user_ap_tcp_connect_cb);
	espconn_accept(pTcpServer);
	espconn_regist_time(pTcpServer, server_timeover, 0);
	WORK_MODE = WAIT_MODE;
}

uint8 ICACHE_FLASH_ATTR connect_check(void){
//	static uint16 delay_count=0;
	struct ip_info info;
	struct ip_addr *sever_addr;

	if(FALSE == wifi_connect_flag){
		WORK_MODE = STATION_MODE;
		os_printf("��ʼ��������·�ɣ�\r\n");
		return 1;
	}
	if(FALSE == sever_connect_flag){
		if(WORK_MODE != NET_RECONNECT)
			WORK_MODE = NET_RECONNECT;
		if(recon_sever_delay > (int)6000*1){//1min
			recon_sever_delay = 0;
//			wifi_get_ip_info(STATION_IF,&info);
//			//�����ã���ӡ�����Ϣ
//			os_printf("device_ip_information��%u\r\n",info.ip);
//			os_printf("device_mask_information��%u\r\n",info.netmask);
//			os_printf("device_gw_information��%u\r\n",info.gw);
//			os_printf("sever_ip_information��%u\r\n",user_sever_ip);
//			//��������IP��ֵ���ṹ��
//			sever_addr->addr = user_sever_ip;
//			my_station_init(sever_addr,&info.ip,(int)SEVER_PORT);

//			/*
//			 * ��ȷ����ô���᲻�������ڴ��ظ�ռ�ã�����ν��иò���ʱ�����ظ�ռ���ڴ�
//			 * �Ӷ������ڴ����
//			 */
//			//ע�����ӻص������������ص�����
//			espconn_regist_connectcb(&user_tcp_conn,user_tcp_connect_cb);
//			espconn_regist_reconcb(&user_tcp_conn,user_tcp_recon_cb);
			//ȷ��·����������
			if(STATION_GOT_IP == wifi_station_get_connect_status()){
				//��������
				espconn_connect(&user_tcp_conn);
				blink_mode = BLINK_SEV_CONCT;
				led_g_count = 1;
				WORK_MODE = RUN_MODE;//�˳�͸��ģʽ���л�������ģʽ
				os_printf("��ʼ�������ӷ�������\r\n");
			}
			else{
				WORK_MODE = STATION_MODE;
			}
		}
		return 2;
	}
	if((ping_outtime_count > 1*60) && (sever_connect_flag == TRUE)){//һ����û�յ���������������
		ping_outtime_count = 0;
		if(WORK_MODE != NET_RECONNECT)
			WORK_MODE = NET_RECONNECT;
		//ȷ��·����������
		if(STATION_GOT_IP == wifi_station_get_connect_status()){
			os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
			os_sprintf(tcp_send_buff,"{\"request_ping\":{\"device_id\":\"%s\"}}\r\n",DEV_ID);
			espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
			os_printf("1minδ�յ������������������������\r\n");
		}
		else{
			WORK_MODE = STATION_MODE;
		}
		return 3;
	}
	if(FALSE == sever_login_flag){
		if(WORK_MODE != NET_RECONNECT)
			WORK_MODE = NET_RECONNECT;
		if(rejoin_sever_dalay > 6000){//1min
			rejoin_sever_dalay = 1;
			user_login_sever();
		}
		return 4;
	}
}

void ICACHE_FLASH_ATTR run_process(void){
	uint8 temp_arr[64]={0};

	//ָ���������APP��ȡ
	if(rec_appdata_falg || rec_severdata_falg){
		getCmd((char *)tcp_rec_buff);
		memset(tcp_rec_buff,0,sizeof(tcp_rec_buff));
		rec_appdata_falg = FALSE;
		rec_severdata_falg = FALSE;
		os_printf("׼��ִ��ָ��\r\n");

		//if(rec_severdata_falg) ping_outtime_count = 0;
	}
	//ִ�з�������APPָ��
	exeCmd();
//	//5s��ʱ����   ��ǰ���ӵ��ֻ�app������������
//	if(count_app_status>=5){
//		count_app_status = 0;
//		os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
//		os_sprintf(tcp_send_buff,"{\"ping\":{\"time\":\"%d:%d:%d\",\"week\":%d}}\r\n",
//								timers.hours,timers.minutes,timers.seconds,timers.week);
//		send_app_status(tcp_send_buff);
//	}
	//����״̬���
//	if(!wifi_userconfig_flag) connect_check();
}

void ICACHE_FLASH_ATTR updata_process(){
	static uint8 step=0;
	static uint16 recon_count = 0;
	uint32 temp;
	char TYPE_BIN;
	uint8 i;

	switch(step){
		case 0:
			user_updata_set();//����׼��������Ϣ  ready
			blink_mode = BLINK_CODE_UPDATA;
			led_g_count = 1;
			step++;
			break;
		case 1://��ⷢ�ͳɹ��󣬶Ͽ���������������
			if(!user_updata_send_check()) step++;
			break;
		case 2:
			if(updata_start_conncet_flag){//�������ķ������Ͽ����ӳɹ�
				updata_start_conncet_flag = FALSE;
				//������������� 1
				espconn_tcp_set_max_con(1);
				updata_station_init((int)UPDATA_SELF_PORT);//������������������
				step++;
			}
			break;
		case 3:
			recon_count++;
			if(updata_sever_connect_flag == TRUE){//�������������������ӳɹ�
				updata_sever_connect_flag = FALSE;
				recon_count = 0;
				step++;
				//
				if(cb_updata_flag){//����������ƺг��򣬴�������bin�ļ��ֶ�д3
					WORK_MODE = CBUPDATA_MODE;
				}
				else{
					system_upgrade_flag_set(UPGRADE_FLAG_START);
					temp = system_get_userbin_addr();
					os_printf("��ǰ�������е���ʼ��ַ��0x%x\r\n",temp);
					if(0x01000==temp){
						TYPE_BIN = 2+0x30;
					}
					else{
						TYPE_BIN = 1+0x30;
					}
					//����׼��������������ָ��
					os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
					os_sprintf(tcp_send_buff,"%c%s%c%c%c",\
							BOOT_START,DEV_ID,TYPE_BIN,BOOT_END1,BOOT_END2);
					//���Դ�ӡ
					os_printf("����׼������ָ����豸���кţ�%s\r\n",tcp_send_buff);
					espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
				}
			}
			//���10sû�����ӳɹ����ź�̫�����
			if(recon_count>1000) system_restart();
			break;
		case 4:
			recon_count++;
			//���5sû�н��յ����������ݣ��ź�̫����߷������쳣������
			if(recon_count > 500){
				step++;
				system_restart_enhance(0, system_get_userbin_addr());
			}
			if(rec_updataseverdata_flag){
				rec_updataseverdata_flag = FALSE;
				recon_count = 0;
				updata_romdata(tcp_rec_buff);
			}
			break;

		default:
			break;
	}
}

void ICACHE_FLASH_ATTR CB_updata_process(){
	if(rec_updataseverdata_flag){
		rec_updataseverdata_flag = FALSE;

		if(BOOT_HEAD == tcp_rec_buff[0]){//���յ�������ͷ
			os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
			os_sprintf(tcp_send_buff,"%c%c%c",BOOT_OK,BOOT_END1,BOOT_END2);
			//���Դ�ӡ
			os_printf("�յ�����ͷ���ݣ������������ݣ�%s\r\n",tcp_send_buff);
			espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
			memset(tcp_rec_buff,0,256);

			return;
		}
		os_printf("�յ�bin����ת��CB��%s\r\n",tcp_send_buff);
		uart0_send_AT(tcp_rec_buff,130);
		memset(tcp_rec_buff,0,256);
	}
}

uint8 ICACHE_FLASH_ATTR at_commond_parse(uint8 *pdata){
//	uint8 temp_arr[64]={0};
//	uint8 *ptemp;
	uint8 len;
	uint8 i,temp_h,temp_l;

	//����ID
	if(os_memcmp(pdata, "SETID", 5) == 0){

		os_printf("����IDָ��\r\n");
		os_memcpy(DEV_ID,&pdata[7],3);
		os_memcpy(ROU_ID,&pdata[10],3);

		os_printf("�����豸SN��%s\r\n",DEV_ID);
		os_printf("����·��SN��%s\r\n",ROU_ID);
		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+DEVICE_ID_ADDR,\
							(uint8*)DEV_ID,3);
		user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+ROUTER_ID_ADDR,\
										(uint8*)ROU_ID,SN_LENGTH);

		set_process_mode(ROUTER_CONFIG);

		uart0_sendStr("OK\r\n");
		return 0;
	}
	//͸��ָ��
	if(os_memcmp(pdata, "TRANSON", 7) == 0){
		if(get_process_mode() == NORMAL_MODE){
			set_process_mode(T_T_MODE);

			uart0_sendStr("OK\r\n");
		}
		else{
			uart0_sendStr("ERROR\r\n");
		}

		return 0;
	}
	os_printf("��Чָ��\r\n");
	return 0;
}

uint8 ICACHE_FLASH_ATTR at_process(uint8 *pdata){
	uint8 *ptemp;

	os_printf("\r\nMCU���ݣ�%s\r\n",pdata);

	if(WORK_MODE == T_T_MODE){
		if(os_memcmp(pdata, "AT+TRANSOFF", 11) == 0){
			set_process_mode(NORMAL_MODE);
			uart0_sendStr("OK\r\n");
			return 0;
		}
		else{
			os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));//����շ��ͻ���

			os_sprintf(tcp_send_buff,"{\"tt_upload\":\"%s\"}\r\n",pdata);

//				os_sprintf(tcp_send_buff,"{\"info\":\"%s\"}",pdata);
//				os_sprintf(tcp_send_buff,"{\"acknormal\":{\"k1\":\"1\"}}\r\n");
//				struct ip_info info;
//				uint8 mac_addr[6]={0};
//				wifi_get_ip_info(STATION_IF,&info);
//				wifi_get_macaddr(STATION_IF,mac_addr);
//				os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));
//				os_sprintf(tcp_send_buff,
//							"{\"infos\":{\"wifi\":\"%s\",\"bssid\":\"%s\",\"mac\":\""MACSTR"\",\"version\":\"%s\",\"ip\":\"%d.%d.%d.%d\"}}\r\n",
//							user_ap_config.ssid,user_ap_bssid,MAC2STR(mac_addr),software_ver,IP2STR(&info.ip));
//			espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
			send_sever_data(tcp_send_buff);

			os_printf("�����������͸�����ݣ�%s\r\n",tcp_send_buff);
			//�ŵ�������ɻص�������
//				uart0_sendStr("OK\r\n");
			tt_send_sever_flag = TRUE;
			return 0;
		}
	}
	else{
		if(os_memcmp(pdata, "AT+", 3) == 0){
			os_printf("����id\r\n");

			ptemp = strstr(pdata, "SETID");
			if(ptemp != NULL){

				os_printf("����IDָ��\r\n");
				os_memcpy(DEV_ID,&ptemp[6],3);
				os_memcpy(ROU_ID,&ptemp[9],3);

				os_printf("�����豸SN��%s\r\n",DEV_ID);
				os_printf("����·��SN��%s\r\n",ROU_ID);
				user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+DEVICE_ID_ADDR,\
									(uint8*)DEV_ID,3);
				user_spi_flash_write((USER_BASE_ADDR*SECTOR_SIZE)+ROUTER_ID_ADDR,\
												(uint8*)ROU_ID,SN_LENGTH);

				set_process_mode(ROUTER_CONFIG);

				uart0_sendStr("OK\r\n");
				return 0;
			}
			ptemp = strstr(pdata, "TRANSON");
			if(ptemp != NULL){
				if(get_process_mode() == NORMAL_MODE){
					set_process_mode(T_T_MODE);

					uart0_sendStr("OK\r\n");
				}
				else{
					uart0_sendStr("ERROR\r\n");
				}

				return 0;
			}
			os_printf("��Чָ��\r\n");
//			ptemp = &pdata[3];
//			at_commond_parse(pdata);
			return 0;
		}
		else{
			uart0_sendStr("ERROR\r\n");
			return 1;
		}
	}
}

WORK_MODE_T ICACHE_FLASH_ATTR get_process_mode(void){
	return WORK_MODE;
}

void ICACHE_FLASH_ATTR set_process_mode(WORK_MODE_T mode){
	WORK_MODE = mode;
}

void ICACHE_FLASH_ATTR mode_process(void){
	static uint16 temp_cnt = 0;

	if(at_buff[0]){
		at_process(at_buff);
		os_memset(at_buff,0,sizeof(at_buff));
	}
	if(send_status_count > 4){
		send_status_count = 0;
		send_mcu_mode_status();
	}
	switch(WORK_MODE){
		case ROUTER_CONFIG:
			if(ROU_ID[0] != 0){
				router_connect();
			}
			break;
		case SEVER_CONFIG:
			if((user_sever_ip.addr == 0xffffffff) || (user_sever_ip.addr == 0x00)){

			}
			else{
				sever_connect();
			}
			break;
		case NET_CONNECT:
//			//TEST ...
//			sever_connect();
			break;
		case NORMAL_MODE:

			run_process();
			break;
		case T_T_MODE:
			//����ƺл�MCU���ͷ�������app�Ŀ���ָ��
			if(ttcmd_buff[0] != 0){
				//����Ҫ�ó�ֱ�ӻ�ȡ͸��ָ���strlenȻ����(������ͨ��TT_CMD_LEN)���Խ����ͬ�豸��ͬ����ָ���������
				//uart0_send_AT(ttcmd_buff,TT_CMD_LEN);
//				uart0_send_AT(ttcmd_buff,ttcmd_len);
				os_printf("׼����muc����͸������:%s\r\n",ttcmd_buff);
				os_printf("͸�����ݳ��ȣ�%d\r\n",strlen(ttcmd_buff));
				uart0_send_AT(ttcmd_buff,strlen(ttcmd_buff));
				memset(ttcmd_buff,0,TT_CMD_LEN);
			}
			else{//������棬��ֹ����͸����ֱ�ӷ���֮ǰ������
				if(ttcmd_buff[0] != 0)
					memset(ttcmd_buff,0,TT_CMD_LEN);
			}
			run_process();
			break;
		case DELAY_RST_MODE:
			if(temp_cnt > 100){
				temp_cnt = 0;
				system_restart();
			}
			break;
		default:
			break;
	}
}

void ICACHE_FLASH_ATTR send_sever_data(uint8 *pbuff){
	espconn_sent(&user_tcp_conn,pbuff,strlen(pbuff));
}

//�����ķ�����ʽ
void ICACHE_FLASH_ATTR send_app_status(uint8 *tcp_send_buff){
	uint8 i;

	for(i=0; i<LINK_MAX; i++){
		if(TRUE == pLink[i].linkEn){
			espconn_sent(pLink[i].pCon,tcp_send_buff,strlen(tcp_send_buff));
			os_printf("��app������Ϣ��app��linkID��%d\r\n",pLink[i].linkId);
		}
	}
}

//Ϊ���Frank��Э���л���0������������ʽ
void ICACHE_FLASH_ATTR tt_send_app_status(uint8 *tcp_send_buff, uint8 len){
	uint8 i;

	for(i=0; i<LINK_MAX; i++){
		if(TRUE == pLink[i].linkEn){
			espconn_sent(pLink[i].pCon,tcp_send_buff,len);
			os_printf("��app������Ϣ��app��linkID��%d\r\n",pLink[i].linkId);
		}
	}
}

void ICACHE_FLASH_ATTR send_mcu_mode_status(void){
	uint8 temp_arr[32]={0};

	os_sprintf(temp_arr,"STATUS:%d\r\n",WORK_MODE);
//	uart0_sendStr(temp_arr);
//	os_printf("SEND-MCU-STATUS��%d\r\n",WORK_MODE);

	if(WORK_MODE <= MAX_WORK_MODE)	{
		memset(temp_arr, 0, 32);
		os_sprintf(temp_arr,"STATUS:%d\r\n",WORK_MODE);
		uart0_sendStr(temp_arr);
		os_printf("SEND-MCU-STATUS��%d\r\n",WORK_MODE);
	}
	else{
		//�Ͽ�·�ɺͷ������������µ�¼
		espconn_disconnect(&user_tcp_conn);
		wifi_station_disconnect();//�Ͽ�·������
		WORK_MODE = DELAY_RST_MODE;
	}
}

void ICACHE_FLASH_ATTR user_udp_sent_cb(void *arg){//���ͻص�����
    os_printf("\r\nUDP���ͳɹ���\r\n");
//    os_timer_disarm(&test_timer);//����ʱ����
//    os_timer_setfn(&test_timer,user_udp_send,NULL);
//    os_timer_arm(&test_timer,1000,NULL);//��1���ӷ���һ��
}

void ICACHE_FLASH_ATTR user_udp_recv_cb(void *arg,
        char *pdata,
        unsigned short len){//���ջص�����
    os_printf("UDP�Ѿ��������ݣ�%s",pdata);//UDP���յ������ݴ�ӡ����
}

//void ICACHE_FLASH_ATTR creat_udp_broadcast(void){
//	wifi_set_broadcast_if(STATION_MODE);//����UDP�㲥�ķ��ͽӿ�station+soft-APģʽ����
//	user_udp_espconn.type=ESPCONN_UDP;
//	user_udp_espconn.proto.udp=(esp_udp*)os_zalloc(sizeof(esp_udp));
//	user_udp_espconn.proto.udp->local_port=LISTEN_PORT;//���ض˿�
//	user_udp_espconn.proto.udp->remote_port=UDP_SEND_PORT;//Զ�̶˿�
//
//	const char udp_remote_ip[4]={255,255,255,255};//���ڴ��Զ��IP��ַ
//	os_memcpy(&user_udp_espconn.proto.udp->remote_ip,udp_remote_ip,4);
//
//	espconn_regist_recvcb(&user_udp_espconn,user_udp_recv_cb);//���ջص�����
//	espconn_regist_sentcb(&user_udp_espconn,user_udp_sent_cb);//���ͻص�����
//	espconn_create(&user_udp_espconn);//����UDP����
//	user_udp_send(); //���ͳ�ȥ
//}




