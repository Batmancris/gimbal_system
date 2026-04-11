#ifndef _MECANUM_CTRL_TASK_H_
#define _MECANUM_CTRL_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

extern osThreadId MecanumCtrlTaskHandle;
extern osThreadId MecanumPoseTaskHandle;
void MecanumTaskInit(void);

#endif
