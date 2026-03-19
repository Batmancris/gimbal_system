#ifndef _RC_TASK_H_
#define _RC_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "rc.h"
#define RC_MSG_LEN	25

#define RC_MSG_QUENE_SIZE 8
#define CTRL_MSG_QUENE_SIZE 3

#define CTRL_TYPE_RC	  0
#define CTRL_TYPE_PC      1
#define CTRL_TYPE_MAG     2

#define RACECAR_MAX_SPEED 1800
#define RACECAR_MIN_SPEED 1200
#define RACECAR_MAX_OMEGA 120
#define RACECAR_MIN_OMEGA 60

#define LIMIT(x, a, b) (((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)))
#define RcValue2AngleDeg(x) \
    ((x) < 1024 ? \
    (1024 - (x)) * param.base.acker.max_steer_angle / 660 : \
    (1024 - (x)) * param.base.acker.max_steer_angle / 624) //364（左最大）~ 1024（中位）~ 1648（右最大），映射+max_steer_angle ~ 0 ~ -max_steer_angle

typedef struct{
	uint8_t Msg[RC_MSG_LEN];
	uint16_t MsgLen;
}RcMsg_t;
extern uint8_t RcBuff[RC_MSG_LEN];
extern osMailQId RcMail;
extern osMailQId CtrlMail;

extern volatile uint32_t CtrlFlag;

extern osThreadId RcTaskHandle;
extern Dbus_RC_Ctl_t CtrlData;
void RcTaskInit(void);

#endif
