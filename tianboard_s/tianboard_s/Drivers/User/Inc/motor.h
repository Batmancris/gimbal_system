#ifndef _MOTOR_INFO_H_
#define _MOTOR_INFO_H_

#include "stm32f4xx_hal.h"

#pragma pack(push)
#pragma pack(1)
typedef struct
{
  int16_t position;
  int16_t w;
  int16_t current;
  int8_t temperature;
  int16_t prePosition;
} MotorInfo_t;

typedef struct
{
  uint16_t status;
  int16_t w;
  int32_t position;
  int32_t prePosition;
} MotecMotorInfo_t;

typedef struct
{
  uint32_t rpm;
  uint16_t current;
  uint16_t duty;
} VescStatus_t;

#pragma pack(pop)

#define VESC_ID 0x02
// CAN commands
typedef enum {
	CAN_PACKET_SET_DUTY = 0,
	CAN_PACKET_SET_CURRENT,
	CAN_PACKET_SET_CURRENT_BRAKE,
	CAN_PACKET_SET_RPM,
	CAN_PACKET_SET_POS,
	CAN_PACKET_FILL_RX_BUFFER,
	CAN_PACKET_FILL_RX_BUFFER_LONG,
	CAN_PACKET_PROCESS_RX_BUFFER,
	CAN_PACKET_PROCESS_SHORT_BUFFER,
	CAN_PACKET_STATUS,
  CAN_PACKET_SET_CURRENT_REL,
	CAN_PACKET_SET_CURRENT_BRAKE_REL,
	CAN_PACKET_SET_CURRENT_HANDBRAKE,
	CAN_PACKET_SET_CURRENT_HANDBRAKE_REL,
} CAN_PACKET_ID;
#define VESC_CAN_PACKET_STATU_ID  ((CAN_PACKET_STATUS << 8) | (VESC_ID))

#define MOTEC_CAN_NUM (&hcan1)
#define MOTEC_CAN_OP_TIMEOUT 1000000
extern volatile MotorInfo_t motorInfo[];
extern volatile MotecMotorInfo_t motecMotorInfo[];
extern volatile uint8_t motecCtrlState;
extern volatile VescStatus_t vescStatus;
void SetCanMotor(int16_t iq1, int16_t iq2, int16_t iq3, int16_t iq4, CAN_HandleTypeDef *can);

void MotecEnterOpMode(void);
void MotecInit(int motor);
void MotecSetSpeed(int16_t rpm, int16_t motor);

void SetVescDuty(float duty);
void SetVescRPM(int32_t rpm);
#endif