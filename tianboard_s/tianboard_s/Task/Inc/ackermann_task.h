#ifndef __ACKERMANN_TASK_H__
#define __ACKERMANN_TASK_H__

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define LIMIT(x, a, b) (((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)))

#define RACECAR_SPEED_ZERO 1500
#define RACECAR_STEER_ANGLE_ZERO 90

#define MID_STEER_ANGLE 90

#define SERVO_CAL(X) ((2500 - 500)*(180-X)/180+500)
#define SERVO_CAL2(X) ((X + 45) * 1000 / 90 + 1000)
#define AngleDeg2Pwm(x) (-x * 500 / (param.base.acker.max_steer_angle)+1500)
#define Pwm2AngleDeg(x) ((1500 - (x)) * (param.base.acker.max_steer_angle) / 500)
#define AngleRad2Pwm(x) (-(x) * 500 * 180 / (PI * param.base.acker.max_steer_angle) + 1500)
#define Pwm2AngleRad(x) ((1500 - (x)) * param.base.acker.max_steer_angle * PI / (500 * 180))

#define ANGLE_CAL(X) (-((X)-1500)/1000.0*PI/2.0)
#define MOTOR_CAL(X) (X + RACECAR_SPEED_ZERO)

#define MOTOR_MAX 2000
#define MOTOR_MIN 1000

extern osThreadId AckermannCtrlTaskHandle;
extern osThreadId AckermannPoseTaskHandle;
void AckermannTaskInit(void);
extern float motor_rpm_set;
#endif
