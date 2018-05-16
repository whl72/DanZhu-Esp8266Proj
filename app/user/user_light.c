/*
 * user_light.c
 *
 *  Created on: 2016年12月12日
 *      Author: billwang
 */
#include "user_light.h"
#include "string.h"
#include "stdlib.h"
#include "osapi.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_spi.h"
#include "user_process.h"
#include "user_interface.h"
#include "eagle_soc.h"
#include "stdio.h"

uint8 ttcmd_buff[TT_CMD_LEN] = {0};

extern uint8 tcp_send_buff[256];
extern struct espconn user_tcp_conn;
extern uint16 ping_outtime_count;

int16_t cmdId = none_id;//指令编号

effcet_time usual_scene_time;
timer timers;//模拟RTC


cmd_funcationType user_cmd[CMD_NUM] ={
	{"tt_download", 11, getTTData},
	{"ping", 4, ackPing}
};

static int8_t ICACHE_FLASH_ATTR
getCmdLen(char *pCmd)
{
	uint8_t n,i;

	n = 0;
	i = 128;
	while(i--)
	{
		if(*pCmd == '"')
		{
			return n;
		}
		else
		{
			pCmd++;
			n++;
		}
	}
	return -1;
}

static int16_t ICACHE_FLASH_ATTR
cmdSearch(int8_t cmdLen, char *pCmd)
{
  int16_t i;

  if(cmdLen == 0)
  {
    return 0;
  }
  else if(cmdLen > 0)
  {
    for(i=0; i<CMD_NUM; i++)
    {
//      os_printf("%d len %d\r\n", cmdLen, at_fun[i].at_cmdLen);
      if(cmdLen == user_cmd[i].cmdLen)
      {
//        os_printf("%s cmp %s\r\n", pCmd, at_fun[i].at_cmdName);
        if(os_memcmp(pCmd, user_cmd[i].cmdName, cmdLen) == 0) //think add cmp len first
        {
          return i;
        }
      }
    }
  }
  return -1;
}

void ICACHE_FLASH_ATTR
getCmd(char *pAtRcvData)
{
	char *tempStr=NULL;

	int8_t cmdLen;
	uint16_t i;
	//os_printf("开始解析指令\r\n");

	tempStr = pAtRcvData+2;//消除掉 {"
  cmdLen = getCmdLen(tempStr);
  //os_printf("指令长度: %d\r\n",cmdLen);
  if(cmdLen != -1)
  {
    cmdId = cmdSearch(cmdLen, tempStr);
    //os_printf("指令ID：%d\r\n",cmdId);
  }
  else
  {
	os_printf("指令长度错误!\r\n");
  	cmdId = -1;
  }
  if(cmdId != -1)
  {
    os_printf("cmd id: %d\r\n", cmdId);
    tempStr += (cmdLen);
//    if(*tempStr == '"')
//    {
      if(user_cmd[cmdId].exeCmd)
      {
    	  user_cmd[cmdId].exeCmd(tempStr);
      }
      else
      {
    	  os_printf("cmd error\r\n");
      }
//    }
  }
}

#if 0
void ICACHE_FLASH_ATTR
getTTData(char *pdata){
	uint8 i=0;
	uint8 temp=0;

	os_printf("收到透传数据：%s\r\n",pdata);

	pdata += 3;//消掉前面的":"
	for(i=0; i<TT_CMD_LEN; i++){//字符转换成16进制
		if((*(pdata+i*2)>='0') && (*(pdata+i*2)<='9')){
			temp = (*(pdata+i*2)-'0')<<4;
		}
		if((*(pdata+i*2)>='a') && (*(pdata+i*2)<='f')){
			temp = (*(pdata+i*2)-'a'+10)<<4;
		}
		if((*(pdata+i*2)>='A') && (*(pdata+i*2)<='F')){
			temp = (*(pdata+i*2)-'A'+10)<<4;
		}

		if((*(pdata+i*2+1)>='0') && (*(pdata+i*2+1)<='9')){
			temp += (*(pdata+i*2+1)-'0')&0xff;
		}
		if((*(pdata+i*2+1)>='a') && (*(pdata+i*2+1)<='f')){
			temp += (*(pdata+i*2+1)-'a'+10)&0xff;
		}
		if((*(pdata+i*2+1)>='A') && (*(pdata+i*2+1)<='F')){
			temp += (*(pdata+i*2+1)-'A'+10)&0xff;
		}

		ttcmd_buff[i] = temp;
		temp=0;

//		os_printf("tt_cmd：%x\r\n",ttcmd_buff[i]);
	}
}
#else
void ICACHE_FLASH_ATTR
getTTData(char *pdata){
	uint8 i=0;
	uint8 temp=0;

	os_printf("收到透传数据：%s\r\n",pdata);

	pdata += 3;//消掉前面的":"
	for(i=0; i<TT_CMD_LEN; i++){
		if(*(pdata+i) != '}')
			ttcmd_buff[i] = *(pdata+i);
		else
//			ttcmd_buff[i] = '}';
			break;
	}
}
#endif

void ICACHE_FLASH_ATTR
ackPing(char *pdata){
	uint8 i=0;

	os_printf("收到心跳数据！\r\n");
	pdata += 11;//消掉前面的":{"time":"
	timers.week = pdata[i++]-0x30;
	timers.hours = (pdata[i++]-0x30)*10 + pdata[i++]-0x30;
	timers.minutes = (pdata[i++]-0x30)*10 + pdata[i++]-0x30;
	timers.seconds = (pdata[i++]-0x30)*10 + pdata[i]-0x30;
//	os_printf("week:%d\r\n",timers.week);
//	os_printf("hours:%d\r\n",timers.hours);
//	os_printf("minutes:%d\r\n",timers.minutes);
	ping_outtime_count = 0;
}

void ICACHE_FLASH_ATTR
exeCmd(void){
	uint8 i;

	if(none_id != cmdId){//有服务器或app指令需要执行
		os_memset(tcp_send_buff,0,sizeof(tcp_send_buff));//先清空发送缓存
		switch(cmdId){
			case tt_data_id:
//				os_sprintf(tcp_send_buff,"{\"tt_data\":\"%d"}\r\n",
//							timers.hours,timers.minutes,timers.seconds,timers.week);
				break;
			case ping_get_id:
				os_sprintf(tcp_send_buff,"{\"ping\":{\"time\":\"%d:%d:%d\",\"week\":%d}}\r\n",
						timers.hours,timers.minutes,timers.seconds,timers.week);
				//调试打印
//				os_printf("反馈控制指令：%s\r\n",tcp_send_buff);
//				send_app_status(tcp_send_buff);
				espconn_sent(&user_tcp_conn,tcp_send_buff,strlen(tcp_send_buff));
				break;
			default:
				break;
		}

		cmdId = none_id;//清除指令编号
	}
}




