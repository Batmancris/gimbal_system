#include "usart.h"
#include "servo.h"

void SetUartServoAngle(float angle)
{
  static uint8_t Data[128];
  int i = 0;
  uint8_t bcc;
  uint16_t offset = 0;

  Data[i++] = 0x12;
  Data[i++] = 0x4C;
  Data[i++] = 0x08;                               //cmd id rotate
  Data[i++] = 0x07;                               //data len
  Data[i++] = 0x01;                               //servo id
  Data[i++] = ((int)(angle * 10)) & 0xFF;        //angle
  Data[i++] = (((int)(angle * 10)) >> 8) & 0xFF; //angle
  Data[i++] = SERVO_PERIOD & 0xFF;                //time
  Data[i++] = (SERVO_PERIOD >> 8) & 0xFF;         //time
  Data[i++] = 0x00;                               //power
  Data[i++] = 0x00;                               //power
  bcc = 0;
  for (; offset < i; offset++)
  {
    bcc += Data[offset];
  }
  Data[i++] = bcc;
  offset++;
  HAL_UART_Transmit_DMA(&huart1, Data, i);
}