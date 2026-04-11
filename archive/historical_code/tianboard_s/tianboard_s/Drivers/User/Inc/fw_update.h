#ifndef __FW_UPDATE__
#define __FW_UPDATE__

#include "stm32f4xx_hal.h"

#define BOOT_ROM_FLAG 0xa5a5a5a5

void CheckBoot(void);
void FwUpdate(void);

#endif
