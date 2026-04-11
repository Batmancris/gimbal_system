#ifndef _IMUWIT_TASK_H_
#define _IMUWIT_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define ImuWit_MSG_LEN	33 // 11*3

#ifndef PI
#define PI 3.1415926f
#endif

typedef struct{
	uint8_t Msg[ImuWit_MSG_LEN];
	uint16_t MsgLen;
}ImuWitMsg_t;

extern uint8_t ImuWitBuff[ImuWit_MSG_LEN];
extern osMailQId ImuWitMail;
extern float imuwit_accel[3]; 
extern float imuwit_gyro[3]; 
extern float imuwit_angle[3];  

void ImuWitTaskInit(void);

#endif
