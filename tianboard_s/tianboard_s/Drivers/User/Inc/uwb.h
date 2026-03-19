#ifndef _UWB_H_
#define _UWB_H_

#include "stm32f4xx_hal.h"

#pragma pack(push)
#pragma pack(1)
typedef struct
{
    int16_t coor_x;
    int16_t coor_y;
    uint16_t yaw;
    int16_t distance[6];
    uint16_t err_mask : 14;
    uint16_t sig_level : 2;
    uint16_t reserved;

}UwbInfo_t;
#pragma pack(pop)

#define UWB_RX_DATA 0x01
extern volatile UwbInfo_t uwbInfo;
#endif
