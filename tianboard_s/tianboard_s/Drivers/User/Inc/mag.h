#ifndef _MAG_H_
#define _MAG_H_

#include "stm32f4xx_hal.h"

#define MAG_MSG_LEN 8

#define MAG_16_BIT
//#define MAG_8_BIT
int MagDataProcess(uint8_t *pData, uint16_t *pMagInfo);

#endif
