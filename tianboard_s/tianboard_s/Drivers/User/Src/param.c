#include "param.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"
#include "string.h"

#define PARAM_SAVE_ADDR 0x080E0000

const Param_t DefaultParam = {
    PARAM_HEAD,
    0.0325,            // wheel_r
    36.0,              // motor_reduction_ratio
    PI,                // max_w
    1.5,               // max_speed
    BASE_TYPE_DIFF,    // base_type
    /*{base_a, base_b} {base_a, base_b} {base_r} {base_a, base_b, motor_servo_type, pwm_dead_zone, max_steer_angle, steering_offset, steering_ratio}*/
    {{0.075, 0.075}, {0.075, 0}, {0.010}, {0.135, 0.198, MOTOR_SERVO_TYPE_CAN_UART, 0, 50, 0, 1.0}},
    // {{0.130, 0.134}, {0.130, 0.134}, {0.010}, {0.135, 0.198, MOTOR_SERVO_TYPE_CAN_UART, 0, 50, 0, 1.0}},
    {15, 0.2, 0.3, 16384.0, 16384.0}, // p i d max_output i_limit
    8192,                             // tick per lap
    0x7FFFFFFF,                       // max ticks
    5,                                // ctrl period
    20,                               // feedback period
    1,                                // pose calc period
    0.2,                              // mag max v
    0.8,                              // mag max w
    0.0,                              // time to max v, unit: s
    0.0,                              // time to max w, unit: s
    {1800, 200, 1000, 40},            // rc param{max, min, mid, dead zone}
    // {1695, 352, 1024, 5},          // rc param{max, min, mid, dead zone}
    MOTOR_DJI_CAN,
    "default",
    PARAM_TAIL};

const Param_t RoboRacerParam = {
    PARAM_HEAD,
    0.0547,            // wheel_r
    10.6,              // motor_reduction_ratio 10.6
    PI,                // max_w
    8,               // max_speed
    BASE_TYPE_ACKERMANN,    // base_type //32.5L 109.32mm r*2, 25w
    /*{base_a, base_b} {base_a, base_b} {base_r} {base_a, base_b, motor_servo_type, pwm_dead_zone, max_steer_angle, steering_offset, steering_ratio}*/
    {{0.075, 0.075}, {0.075, 0}, {0.010}, {0.125, 0.163, MOTOR_SERVO_TYPE_VESC_PWM, 0, 22, 0, 1.0}},
    {15, 0.2, 0.3, 16384.0, 16384.0}, // p i d max_output i_limit
    8192,                             // tick per lap
    0x7FFFFFFF,                       // max ticks
    5,                                // ctrl period
    20,                               // feedback period
    1,                                // pose calc period
    0.2,                              // mag max v
    0.8,                              // mag max w
    0.0,                              // time to max v, unit: s
    0.0,                              // time to max w, unit: s
    {1648, 364, 1024, 0},            // rc param{max, min, mid, dead zone}
    MOTOR_VESC6_CAN,
    "RoboRacerParam",
    PARAM_TAIL};

Param_t param;

void InitParam(void)
{
  int i;
  Param_t *p = (Param_t *)PARAM_SAVE_ADDR;

  if ((p->param_head != PARAM_HEAD) || (p->param_tail != PARAM_TAIL)) // ((p->param_head != PARAM_HEAD) || (p->param_tail != PARAM_TAIL))
  {
    FLASH_EraseInitTypeDef EarseStructure;
    uint32_t SectorError = 0;
    HAL_FLASH_Unlock();
    // HAL_Delay(2000);
    EarseStructure.TypeErase = FLASH_TYPEERASE_SECTORS;
    EarseStructure.Banks = FLASH_BANK_1;
    EarseStructure.Sector = FLASH_SECTOR_11;
    EarseStructure.NbSectors = 1;
    EarseStructure.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&EarseStructure, &SectorError);
    // HAL_Delay(2000);
    for (i = 0; i < sizeof(Param_t); i++)
    {
      HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, PARAM_SAVE_ADDR + i, *(((uint8_t *)&RoboRacerParam) + i));
    }
    HAL_FLASH_Lock();
  }

  memcpy(&param, (void *)PARAM_SAVE_ADDR, sizeof(Param_t));
}

void SaveParam(void *p)
{
  int i;
  FLASH_EraseInitTypeDef EarseStructure;
  uint32_t SectorError = 0;
  HAL_FLASH_Unlock();
  // HAL_Delay(2000);
  EarseStructure.TypeErase = FLASH_TYPEERASE_SECTORS;
  EarseStructure.Banks = FLASH_BANK_1;
  EarseStructure.Sector = FLASH_SECTOR_11;
  EarseStructure.NbSectors = 1;
  EarseStructure.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGPERR);
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGSERR);
  HAL_FLASHEx_Erase(&EarseStructure, &SectorError);

  for (i = 0; i < sizeof(Param_t); i++)
  {
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, PARAM_SAVE_ADDR + i, *(((uint8_t *)p) + i));
  }
  HAL_FLASH_Lock();
}
