#ifndef __CAN_WRAPPER_H__
#define __CAN_WRAPPER_H__

#include "stm32f4xx_hal.h"

void CanParamInit(CAN_HandleTypeDef *hcan);
void CanSend(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t *data, uint8_t len);
#endif
