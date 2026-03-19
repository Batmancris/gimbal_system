#ifndef _UWB_TASK_H_
#define _UWB_TASK_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

extern osThreadId UwbFeedbackTaskHandle;
void UwbTaskInit(void);

#endif
