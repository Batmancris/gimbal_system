#ifndef _RC_H_
#define _RC_H_

#include "stm32f4xx_hal.h"

#define RC_TIMEOUT 20


/*-------------------------------dbus defination--------------------*/

/* -------------------------- RC Switch Definition RC Switch Definition RC Switch Definition RC Switch Definition----------------*/
#define DBUS_RC_SW_UP ((uint16_t)1)
#define DBUS_RC_SW_MID ((uint16_t)3)
#define DBUS_RC_SW_DOWN ((uint16_t)2)
/* ----------------------- PC Key Definition PC Key Definition PC Key Definition ------------------*/
#define DBUS_KEY_PRESSED_OFFSET_W ((uint16_t)0x01 << 0)
#define DBUS_KEY_PRESSED_OFFSET_S ((uint16_t)0x01 << 1)
#define DBUS_KEY_PRESSED_OFFSET_A ((uint16_t)0x01 << 2)
#define DBUS_KEY_PRESSED_OFFSET_D ((uint16_t)0x01 << 3)
#define DBUS_KEY_PRESSED_OFFSET_Q ((uint16_t)0x01 << 4)
#define DBUS_KEY_PRESSED_OFFSET_E ((uint16_t)0x01 << 5)
#define DBUS_KEY_PRESSED_OFFSET_SHIFT ((uint16_t)0x01 << 6)
#define DBUS_KEY_PRESSED_OFFSET_CTRL ((uint16_t)0x01 << 7)
#define DBUS_RC_FRAME_LENGTH 18u

/* -----------------------Data Struct Data Struct ------------------------------------- */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
  struct
  {
    uint16_t ch0;
    uint16_t ch1;
    uint16_t ch2;
    uint16_t ch3;
    uint16_t ch4;
    uint8_t s1;
    uint8_t s2;
  } rc;
  struct
  {
    int16_t x;
    int16_t y;
    int16_t z;
    uint8_t press_l;
    uint8_t press_r;
  } mouse;
  struct
  {
    uint16_t v;
  } key;
} Dbus_RC_Ctl_t;

/*-------------------------------sbus defination--------------------*/
#define SBUS_START_FRAME 0x0F

#define SBUS_RC_FRAME_LENGTH 25u

typedef struct
{
  uint16_t ch[16];
  uint32_t rc_link;
} Sbus_RC_Ctl_t;


typedef struct
{
  float vx;
  float vy;
  float w;
  float steering_angle;
} MotionCtrl_t;

#pragma pack(pop)

void DbusRemoteDataProcess(uint8_t *pData, Dbus_RC_Ctl_t *pCtrlData);
void SbusRemoteDataProcess(uint8_t *pData, Sbus_RC_Ctl_t *pCtrlData);
#endif
