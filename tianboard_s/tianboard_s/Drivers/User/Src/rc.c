#include "rc.h"

void DbusRemoteDataProcess(uint8_t *pData, Dbus_RC_Ctl_t *pCtrlData)
{
  if ((pData == NULL) || (pCtrlData == NULL))
  {
    return;
  }
  pCtrlData->rc.ch0 = ((int16_t)pData[0] | ((int16_t)pData[1] << 8)) & 0x07FF;
  pCtrlData->rc.ch1 = (((int16_t)pData[1] >> 3) | ((int16_t)pData[2] << 5)) & 0x07FF;
  pCtrlData->rc.ch2 = (((int16_t)pData[2] >> 6) | ((int16_t)pData[3] << 2) | ((int16_t)pData[4] << 10)) & 0x07FF;
  pCtrlData->rc.ch3 = (((int16_t)pData[4] >> 1) | ((int16_t)pData[5] << 7)) & 0x07FF;
  pCtrlData->rc.ch4 = ((int16_t)pData[16]) | ((int16_t)pData[17] << 8);
  pCtrlData->rc.s1 = ((pData[5] >> 6) & 0x0003);
  pCtrlData->rc.s2 = ((pData[5] >> 4) & 0x0003);
  pCtrlData->mouse.x = ((int16_t)pData[6]) | ((int16_t)pData[7] << 8);
  pCtrlData->mouse.y = ((int16_t)pData[8]) | ((int16_t)pData[9] << 8);
  pCtrlData->mouse.z = ((int16_t)pData[10]) | ((int16_t)pData[11] << 8);
  pCtrlData->mouse.press_l = pData[12];
  pCtrlData->mouse.press_r = pData[13];
  pCtrlData->key.v = ((int16_t)pData[14]) | ((int16_t)pData[15] << 8);
}

void SbusRemoteDataProcess(uint8_t *pData, Sbus_RC_Ctl_t *pCtrlData)
{
  if ((pData == NULL) || (pCtrlData == NULL))
  {
    return;
  }
  pCtrlData->ch[0] = ((int16_t)pData[1] | ((int16_t)pData[2] << 8)) & 0x07FF;
  pCtrlData->ch[1] = (((int16_t)pData[2] >> 3) | ((int16_t)pData[3] << 5)) & 0x07FF;
  pCtrlData->ch[2] = (((int16_t)pData[3] >> 6) | ((int16_t)pData[4] << 2) | ((int16_t)pData[5] << 10)) & 0x07FF;
  pCtrlData->ch[3] = (((int16_t)pData[5] >> 1) | ((int16_t)pData[6] << 7)) & 0x07FF;
  pCtrlData->ch[4] = (((int16_t)pData[6] >> 4) | ((int16_t)pData[7] << 4)) & 0x07FF;
  pCtrlData->ch[5] = (((int16_t)pData[7] >> 7) | ((int16_t)pData[8] << 1) | ((int16_t)pData[9] << 9)) & 0x07FF;
  pCtrlData->ch[6] = (((int16_t)pData[9] >> 2) | ((int16_t)pData[10] << 6)) & 0x07FF;
  pCtrlData->ch[7] = (((int16_t)pData[10] >> 5) | ((int16_t)pData[11] << 3)) & 0x07FF;
  pCtrlData->ch[8] = ((int16_t)pData[12] | ((int16_t)pData[13] << 8)) & 0x07FF;
  pCtrlData->ch[9] = (((int16_t)pData[13] >> 3) | ((int16_t)pData[14] << 5)) & 0x07FF;
  pCtrlData->ch[10] = (((int16_t)pData[14] >> 6) | ((int16_t)pData[15] << 2) | ((int16_t)pData[16] << 10)) & 0x07FF;
  pCtrlData->ch[11] = (((int16_t)pData[16] >> 1) | ((int16_t)pData[17] << 7)) & 0x07FF;
  pCtrlData->ch[12] = (((int16_t)pData[17] >> 4) | ((int16_t)pData[18] << 4)) & 0x07FF;
  pCtrlData->ch[13] = (((int16_t)pData[18] >> 7) | ((int16_t)pData[19] << 1) | ((int16_t)pData[20] << 9)) & 0x07FF;
  pCtrlData->ch[14] = (((int16_t)pData[20] >> 2) | ((int16_t)pData[21] << 6)) & 0x07FF;
  pCtrlData->ch[15] = (((int16_t)pData[21] >> 5) | ((int16_t)pData[22] << 3)) & 0x07FF;
  pCtrlData->rc_link = ((pData[23] & 0xC) == 0) ? 1 : 0;
}
