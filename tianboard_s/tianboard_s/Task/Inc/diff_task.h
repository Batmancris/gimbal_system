#ifndef _DIFF_CTRL_TASK_H_
#define _DIFF_CTRL_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

extern osThreadId DiffCtrlTaskHandle;
extern osThreadId DiffPoseTaskHandle;
void DiffTaskInit(void);

#endif
