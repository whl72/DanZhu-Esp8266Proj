/*
 * user_light.h
 *
 *  Created on: 2016Äê12ÔÂ12ÈÕ
 *      Author: billwang
 */

#ifndef APP_INCLUDE_USER_LIGHT_H_
#define APP_INCLUDE_USER_LIGHT_H_

#include "c_types.h"
//#include "espconn.h"

#define TT_CMD_LEN				8
#define CMD_NUM					11

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

typedef struct
{
	char *cmdName;
	int8_t cmdLen;
  void (*exeCmd)(char *pPara);
}cmd_funcationType;

typedef struct
{
	uint8 r_value;
	uint8 g_value;
	uint8 b_value;
}type_rgb_value;

typedef struct
{
	uint8_t start_hours;
	uint8_t start_minutes;
	uint8_t end_hours;
	uint8_t end_minutes;
}effcet_time;

typedef struct{
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t week;
}timer;

typedef union{
	struct{
		uint8_t enbale;
		timer tim;
		uint8_t num;
		uint8_t state;
		uint8_t loops;
		char handler[8];
	};
	char data[16];
}alarm;

enum EFFCET_DAY{
    NOE_DAY = 0,
    TWO_DAY = 1,
	NO_USE_DAY = 9
};

enum Lamp_Mode{
    color_mode = 0,
    sunlight_mode = 1,
    cf_mode = 2,
};
enum Scene_Mode{
	default_scene = 0,
	user_scene = 1
};
enum Cmd_ID{
	tt_data_id = 0,
	ping_get_id = 1,
	none_id = -1
};

void ICACHE_FLASH_ATTR exeCmd(void);
void ICACHE_FLASH_ATTR getCmd(char *pAtRcvData);
void ICACHE_FLASH_ATTR ackPing(char *pdata);
void ICACHE_FLASH_ATTR getTTData(char *pdata);

#endif /* APP_INCLUDE_USER_LIGHT_H_ */
