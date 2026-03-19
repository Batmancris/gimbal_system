#ifndef _CRC_H_
#define _CRC_H_

#include "stm32f4xx_hal.h"

uint32_t verify_crc16(uint8_t *pchMessage, uint32_t dwLength);
void append_crc16(uint8_t* pchMessage,uint32_t dwLength);

uint32_t verify_crc32(uint8_t *pchMessage, uint32_t dwLength);
void append_crc32(uint8_t* pchMessage,uint32_t dwLength);
#endif
