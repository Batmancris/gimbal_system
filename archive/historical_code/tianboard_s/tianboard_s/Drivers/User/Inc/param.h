#ifndef _PARAM_H_
#define _PARAM_H_

#include "stm32f4xx_hal.h"

#define PARAM_HEAD 0xaa5555aa
#define PARAM_TAIL 0x55aaaa55

#define BASE_TYPE_MECANUM 0
#define BASE_TYPE_OMNI 1
#define BASE_TYPE_DIFF 2
#define BASE_TYPE_ACKERMANN 3

#define MOTOR_SERVO_TYPE_CAN_UART 0
#define MOTOR_SERVO_TYPE_PWM 1
#define MOTOR_SERVO_TYPE_VESC_PWM 2

#define MOTOR_DJI_CAN      0
#define MOTOR_MOTEC_CAN    1
#define MOTOR_VESC6_CAN     2

#ifndef PI
#define PI 3.1415926f
#endif

#define SIN_60 0.8660254f
#define COS_60 0.5f

#pragma pack(push)
#pragma pack(1)
/******************************
//              ^^
//              ^^
//              ||
//
//         m1 | -- | m2
//            |    |
//         m3 | -- | m4
//
//
//           \    /|
//            \  / |
//          A---   |B
//            /  \
//           /    \
*******************************/
typedef struct
{
  float base_a; // width/2 left to center
  float base_b; // length/2 front to center
} MecanumBaseParam_t;

/******************************
//              ^^
//              ^^
//              ||
//
//         m1 | -- | m2
//            |    |
//         m3 | -- | m4
//
//
//           \    /|
//            \  / |
//          A---   |B
//            /  \
//           /    \
*******************************/
typedef struct
{
  float base_a; // width/2 left to center
  float base_b; // length/2 front to center
} DiffBaseParam_t;

/******************************
//            ^^
//            ^^
//            ||
//
//             m1
//             |
//            r|
//             /\
//            /  \
//           m3   m2
*******************************/

typedef struct
{
  float base_r;
} OmniBaseParam_t;

/******************************
//              ^^
//              ^^
//              ||
//
//           \    /|
//            \  / |
//          A---   |B
//            /  \
//           /    \
*******************************/
typedef struct
{
  float base_a; // width/2 left to center
  float base_b; // length/2 front to center
  uint32_t motor_servo_type;
  float pwm_dead_zone;
  float max_steer_angle;
  float steering_offset;
  float steering_ratio; // servo angle / wheel angle
} AckermannBaseParam_t;

typedef struct
{
  float p;
  float i;
  float d;
  float max_out;
  float i_limit;
} PidParam_t;

typedef struct
{
  int32_t max;
  int32_t min;
  int32_t mid;
  uint32_t dead_zone;
} RcParam_t;

typedef struct
{
  uint32_t param_head;
  float wheel_r;
  float motor_reduction_ratio;
  float max_w;
  float max_speed;
  uint32_t base_type;
  struct
  {
    MecanumBaseParam_t mecanum;
    DiffBaseParam_t diff;
    OmniBaseParam_t omni;
    AckermannBaseParam_t acker;
  } base;
  PidParam_t pid;
  uint32_t ticks_per_lap;
  int32_t max_ticks;
  int ctrl_period;
  int feedback_period;
  int pose_calc_period;
  float mag_v;
  float mag_max_w;
  float time_to_max_v;
  float time_to_max_w;
  RcParam_t rc_param;
  int32_t motor_type;
  char desc[32];
  uint32_t param_tail;
} Param_t;
#pragma pack(pop)
extern Param_t param;
extern const Param_t DefaultParam;
extern const Param_t RoboRacerParam;
void InitParam(void);
void SaveParam(void *p);
#endif
