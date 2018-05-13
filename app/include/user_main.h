/*
 * user_main.h
 *
 *  Created on: 2016年11月11日
 *      Author: billwang
 */

#ifndef APP_INCLUDE_USER_MAIN_H_
#define APP_INCLUDE_USER_MAIN_H_

#include "driver/uart.h"
#include "osapi.h"
#include "eagle_soc.h"
#include "user_interface.h"
#include "user_spi.h"
#include "user_process.h"
#include "user_timer.h"
#include "at_custom.h"
#include "pwm.h"
#include "gpio16.h"
#include "gpio.h"
#include "user_light.h"
//#include "client.h"

//用户应用程序的大小    实际大小 = CODE_SCETOR_SIZE*4K
#define CODE_SCETOR_SIZE	60

void ICACHE_FLASH_ATTR Wifi_conned(void *arg);
void ICACHE_FLASH_ATTR io_init(void);
void ICACHE_FLASH_ATTR earse_process(void);


#endif /* APP_INCLUDE_USER_MAIN_H_ */
