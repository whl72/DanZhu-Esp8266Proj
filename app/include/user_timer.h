/*
 * user_timer.h
 *
 *  Created on: 2016Äê11ÔÂ18ÈÕ
 *      Author: billwang
 */

#ifndef APP_INCLUDE_USER_TIMER_H_
#define APP_INCLUDE_USER_TIMER_H_

#define BLINK_NO_WORK       0
#define BLINK_AP_CONF 		1
#define BLINK_AP_CONCT		2
#define BLINK_SEV_CONCT		3
#define BLINK_CODE_UPDATA   4


void timer_process(void);
void led_blink(void);
void key_check(void);

#endif /* APP_INCLUDE_USER_TIMER_H_ */
