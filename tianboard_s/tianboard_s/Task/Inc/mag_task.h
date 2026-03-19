#ifndef _MAG_TASK_H_
#define _MAG_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define MAG_MSG_BUFF_LEN	10

#define MAG_MSG_QUENE_SIZE 8

#define MAG_TIMEOUT 30

#define MAG_OUT 0
#define MAG_IN 1

typedef struct{
	uint8_t Msg[MAG_MSG_BUFF_LEN];
	uint16_t MsgLen;
}MagMsg_t;

extern osMailQId MagMail;

extern MagMsg_t *pMagMsg;

extern osThreadId MagTaskHandle;

void MagTaskInit(void);

#endif
