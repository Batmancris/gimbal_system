#include "ackermann_task.h"
#include "cmsis_os.h"
#include "rc.h"
#include "can_wrapper.h"
#include "can.h"
#include "rc_task.h"
#include "pid.h"
#include "param.h"
#include "stdlib.h"
#include "protocol.h"
#include "protocol_task.h"
#include "string.h"
#include "imu.h"
#include "math.h"
#include "motor.h"
#include "servo.h"
#include "tim.h"
#include "usart.h"
#include "odom.h"

float steering_set;
float motor_rpm_set;
float motor_v;
osThreadId AckermannCtrlTaskHandle;
osThreadId AckermannFeedbackTaskHandle;
osThreadId AckermannPoseTaskHandle;

void Uart1Init500000(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 500000;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

static void AckermannCtrlTaskEntry(void const *argument)
{
  osEvent evt;
  MotionCtrl_t *p;
  Pid_t wheelPid[1];
  int timeout = 0;
  int16_t motorCurrent = 0;
  float v = 0;
  float steering = 0;

  osDelay(10);
  // Uart1Init500000();
  if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_CAN_UART || param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_VESC_PWM)
  {
    while (1)
    {
      if (hcan1.State == HAL_CAN_STATE_READY)
      {
        CanParamInit(&hcan1);
        break;
      }
    }
    if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_VESC_PWM)
    {
      TIM1->CCR2 = 1500;
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    }
  }
  else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_PWM)
  {
    TIM1->CCR1 = LIMIT(RACECAR_SPEED_ZERO, MOTOR_MIN, MOTOR_MAX);
    TIM1->CCR2 = LIMIT(SERVO_CAL(RACECAR_STEER_ANGLE_ZERO), SERVO_CAL(MID_STEER_ANGLE - param.base.acker.max_steer_angle), SERVO_CAL(MID_STEER_ANGLE + param.base.acker.max_steer_angle));
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
  }
  PidInit(&wheelPid[0], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
  if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_CAN_UART)
  {
    for (;;)
    {
      evt = osMailGet(CtrlMail, param.ctrl_period);
      if (evt.status == osEventMail)
      {
        timeout = 0;
        p = evt.value.p;
        v = p->vx / 2.0f / PI / param.wheel_r * param.motor_reduction_ratio * 60.0f;
        steering = p->steering_angle;
        osMailFree(CtrlMail, p);
      }
      else
      {
        timeout++;
        if (timeout >= 1000 / param.ctrl_period)
        {
          v = 0;
          steering = 0;
          timeout = 0;
        }
      }

      motorCurrent = PidCalc(&wheelPid[0], motorInfo[0].w, v);
      steering_set = steering;

      SetUartServoAngle(steering * param.base.acker.steering_ratio + param.base.acker.steering_offset);
      SetCanMotor(motorCurrent, 0, 0, 0, &hcan1);
    }
  }
  else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_PWM)
  {
    int timeout = 0;
    int motorPwm;
    float steering = 90.0;
    float v = 0;
    for (;;)
    {
      evt = osMailGet(CtrlMail, param.ctrl_period);

      if (evt.status == osEventMail)
      {
        timeout = 0;
        p = evt.value.p;
        steering = p->steering_angle;
        v = p->vx;

        motorPwm = PidCalc(&wheelPid[0], motor_v, v);
        steering += 90;

        // skip dead zone
        if (motorPwm < 0)
        {
          motorPwm -= param.base.acker.pwm_dead_zone;
        }
        else if (motorPwm > 0)
        {
          motorPwm += param.base.acker.pwm_dead_zone;
        }

        if (v == 0)
        {
          motorPwm = 0;
          PidInit(&wheelPid[0], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
        }

        TIM1->CCR1 = LIMIT(MOTOR_CAL(motorPwm), MOTOR_MIN, MOTOR_MAX);
        TIM1->CCR2 = LIMIT(SERVO_CAL(steering), SERVO_CAL(MID_STEER_ANGLE - param.base.acker.max_steer_angle), SERVO_CAL(MID_STEER_ANGLE + param.base.acker.max_steer_angle));
        osMailFree(CtrlMail, p);
      }
      else
      {
        timeout++;
        if (timeout >= 200 / param.ctrl_period)
        {
          v = 0;
          steering = RACECAR_STEER_ANGLE_ZERO;
          timeout = 0;
        }
        motorPwm = PidCalc(&wheelPid[0], motor_v, v);
        // skip dead zone
        if (motorPwm < 0)
        {
          motorPwm -= param.base.acker.pwm_dead_zone;
        }
        else if (motorPwm > 0)
        {
          motorPwm += param.base.acker.pwm_dead_zone;
        }

        if (v == 0)
        {
          motorPwm = 0;
          
        }

        TIM1->CCR1 = LIMIT(MOTOR_CAL(motorPwm), MOTOR_MIN, MOTOR_MAX);
        TIM1->CCR2 = LIMIT(SERVO_CAL(steering), SERVO_CAL(MID_STEER_ANGLE - param.base.acker.max_steer_angle), SERVO_CAL(MID_STEER_ANGLE + param.base.acker.max_steer_angle));
      }
    }
  }
  else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_VESC_PWM)
  {
    for (;;)
    {
      evt = osMailGet(CtrlMail, param.ctrl_period);
      if (evt.status == osEventMail)
      {
        timeout = 0;
        p = evt.value.p;
        v = p->vx;
        steering = p->steering_angle;
        osMailFree(CtrlMail, p);
      }
      else
      {
        timeout++;
        if (timeout >= 1000 / param.ctrl_period)
        {
          v = 0;
          steering = 0;
          timeout = 0;
        }
      }
      v = LIMIT(v, -param.max_speed, param.max_speed);
      motor_rpm_set = v / 2.0f / PI / param.wheel_r * param.motor_reduction_ratio * 60.0f;
      steering_set = LIMIT(AngleDeg2Pwm(steering), 1000, 2000);

      TIM1->CCR2 = steering_set;
      SetVescRPM(motor_rpm_set);
    }
  }
  else
  {
    for (;;)
    {
      evt = osMailGet(CtrlMail, param.ctrl_period);
      if (evt.status == osEventMail)
      {
        osMailFree(CtrlMail, evt.value.p);
      }
    }
  }
}

static void AckermannFeedbackTaskEntry(void const *argument)
{
  struct odom odom;
  while (1)
  {
    odom.pose.point.x = pose.position_x_m;
    odom.pose.point.y = pose.position_y_m;
    odom.pose.point.z = 0;
    odom.pose.yaw = pose.yaw;
    odom.twist.angular.x = 0;
    odom.twist.angular.y = 0;
    odom.twist.angular.z = pose.wz;
    odom.twist.linear.x = pose.v_x_m;
    odom.twist.linear.y = pose.v_y_m;
    odom.twist.linear.z = 0;

    ProtocolSend(PACK_TYPE_ODOM_RESPONSE, (uint8_t *)&odom, sizeof(struct odom));
    osDelay(param.feedback_period);
  }
}

static void AckermannPoseTaskEntry(void const *argument)
{
  osDelay(1000);
  memset(&pose, 0, sizeof(pose));
  if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_CAN_UART)
  {
    int DeltaTick;
    int FirstFlag = 1;
    float delta_m;
    for (;;)
    {
      if (FirstFlag)
      {
        FirstFlag = 0;
        motorInfo[0].prePosition = motorInfo[0].position;
        osDelay(param.pose_calc_period);
        continue;
      }

      if ((motorInfo[0].position - motorInfo[0].prePosition) > param.max_ticks / 2) // down overflow
      {
        DeltaTick = ((motorInfo[0].prePosition + param.max_ticks - motorInfo[0].position) % param.max_ticks);
      }
      else if ((motorInfo[0].position - motorInfo[0].prePosition) < -param.max_ticks / 2) // up overflow
      {
        DeltaTick = -((motorInfo[0].position + param.max_ticks - motorInfo[0].prePosition) % param.max_ticks);
      }
      else if (motorInfo[0].position > motorInfo[0].prePosition)
      {
        DeltaTick = -(motorInfo[0].position - motorInfo[0].prePosition);
      }
      else
      {
        DeltaTick = (motorInfo[0].prePosition - motorInfo[0].position);
      }
      motorInfo[0].prePosition = motorInfo[0].position;

      delta_m = (float)-DeltaTick / param.ticks_per_lap * 2 * PI * param.wheel_r / param.motor_reduction_ratio;

      // tbd get steering angle
      float angle = steering_set / RADIAN_COEF;
      pose.wz = delta_m / param.pose_calc_period * 1000.0f * tan(angle) / 2 / param.base.acker.base_b;
      pose.yaw += delta_m * tan(angle) / 2 / param.base.acker.base_b;
      if (pose.yaw > PI)
      {
        pose.yaw -= 2 * PI;
      }
      else if (pose.yaw < -PI)
      {
        pose.yaw += 2 * PI;
      }
      // angle = imu.yaw / RADIAN_COEF;
      angle = pose.yaw;
      pose.position_x_m += delta_m * cos(angle);
      pose.position_y_m += delta_m * sin(angle);

      // pose.v_x_m = motor_v * cos(angle);
      // pose.v_y_m = motor_v * sin(angle);
      pose.v_x_m = delta_m / param.pose_calc_period * 1000.0f;
      pose.v_y_m = 0;

      osDelay(param.pose_calc_period);
    }
  }
  else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_PWM)
  {
    int DeltaTicks;
    uint16_t ticks = 0;
    uint16_t preTicks = 0;
    float delta_m;
    int ccr;
    float angle;
    for (;;)
    {
      ticks = __HAL_TIM_GET_COUNTER(&htim8);

      if (ticks - preTicks > param.max_ticks / 2) // down overflow
      {
        DeltaTicks = (int16_t)((preTicks + param.max_ticks - ticks) % param.max_ticks);
      }
      else if (ticks - preTicks < -param.max_ticks / 2) // up overflow
      {
        DeltaTicks = (int16_t)(-(ticks + param.max_ticks - preTicks) % param.max_ticks);
      }
      else if (ticks > preTicks)
      {
        DeltaTicks = (int16_t)(-(ticks - preTicks));
      }
      else
      {
        DeltaTicks = (int16_t)(preTicks - ticks);
      }
      preTicks = ticks;
      delta_m = (float)DeltaTicks / param.ticks_per_lap * 2 * PI * param.wheel_r / param.motor_reduction_ratio;
      motor_v = delta_m / param.pose_calc_period * 1000.0f; // m/s
      ccr = TIM1->CCR2;
      pose.wz = motor_v * tan(ANGLE_CAL(ccr)) / 2 / param.base.acker.base_b;
      pose.yaw += delta_m * tan(ANGLE_CAL(ccr)) / 2 / param.base.acker.base_b;
      if (pose.yaw > PI)
      {
        pose.yaw -= 2 * PI;
      }
      else if (pose.yaw < -PI)
      {
        pose.yaw += 2 * PI;
      }
      // angle = imu.yaw / RADIAN_COEF;
      angle = pose.yaw;
      pose.position_x_m += delta_m * cos(angle);
      pose.position_y_m += delta_m * sin(angle);

      // pose.v_x_m = motor_v * cos(angle);
      // pose.v_y_m = motor_v * sin(angle);
      pose.v_x_m = motor_v;
      pose.v_y_m = 0;
      osDelay(param.pose_calc_period);
    }
  }
  else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_VESC_PWM)
  {
    float delta_m, motor_v; // m, m/s
    float angle;
    int16_t RPM;
    int ccr;
    for (;;)
    {
      RPM = (int16_t)vescStatus.rpm;
      motor_v = RPM / 60.0f * 2.0f * PI * param.wheel_r / param.motor_reduction_ratio;
      delta_m = motor_v * param.pose_calc_period / 1000.0f;
      ccr = TIM1->CCR2;
      pose.wz = motor_v * tan(Pwm2AngleRad(ccr)) / 2 / param.base.acker.base_b;
      pose.yaw += delta_m * tan(Pwm2AngleRad(ccr)) / 2 / param.base.acker.base_b;
      if (pose.yaw > PI)
      {
        pose.yaw -= 2 * PI;
      }
      else if (pose.yaw < -PI)
      {
        pose.yaw += 2 * PI;
      }
      // angle = imu.yaw / RADIAN_COEF;
      angle = pose.yaw;
      pose.position_x_m += delta_m * cos(angle);
      pose.position_y_m += delta_m * sin(angle);

      // pose.v_x_m = motor_v * cos(angle);
      // pose.v_y_m = motor_v * sin(angle);
      pose.v_x_m = motor_v;
      pose.v_y_m = 0;
      osDelay(param.pose_calc_period);
      // usart_printf("%d,%.2f,%.2f,%.2f\n",RPM,motor_v,delta_m,pose.position_x_m);
    }
  }
  else
  {
    for (;;)
    {
      osDelay(param.pose_calc_period);
    }
  }
}
osThreadDef(AckermannCtrlTask, AckermannCtrlTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(AckermannFeedbackTask, AckermannFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(AckermannPoseTask, AckermannPoseTaskEntry, osPriorityAboveNormal, 0, 512);
void AckermannTaskInit(void)
{
  AckermannCtrlTaskHandle = osThreadCreate(osThread(AckermannCtrlTask), NULL);
  AckermannFeedbackTaskHandle = osThreadCreate(osThread(AckermannFeedbackTask), NULL);
  AckermannPoseTaskHandle = osThreadCreate(osThread(AckermannPoseTask), NULL);
}