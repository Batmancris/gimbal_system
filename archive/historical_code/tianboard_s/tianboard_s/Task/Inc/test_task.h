#ifndef __TEST_TASK_H__
#define __TEST_TASK_H__

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define UART_RX_BUFF_SIZE 16

#define CAN_RX_BUFF_SIZE 8

void TestTaskInit(void);

#endif
