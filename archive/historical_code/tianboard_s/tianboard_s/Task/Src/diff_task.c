#include "diff_task.h"
#include "cmsis_os.h"
#include "rc.h"
#include "can_wrapper.h"
#include "can.h"
#include "rc_task.h"
#include "motor.h"
#include "pid.h"
#include "param.h"
#include "stdlib.h"
#include "protocol.h"
#include "protocol_task.h"
#include "string.h"
#include "imu.h"
#include "math.h"
#include "motor.h"
#include "odom.h"

osThreadId DiffCtrlTaskHandle;
osThreadId DiffFeedbackTaskHandle;
osThreadId DiffPoseTaskHandle;

static void DiffCtrlTaskEntry(void const *argument)
{
  osEvent evt;
  MotionCtrl_t *p;
  Pid_t wheelPid[4];
  float w, vx, vy;
  int timeout = 0;
  int16_t motorCurrent[4] = {0, 0, 0, 0};
  float v[4] = {0, 0, 0, 0};

  osDelay(10);

  if (hcan1.State == HAL_CAN_STATE_READY)
  {
    CanParamInit(&hcan1);
  }
  if (param.motor_type == MOTOR_DJI_CAN)
  {
    PidInit(&wheelPid[0], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[1], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[2], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[3], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
  }
  else if (param.motor_type == MOTOR_MOTEC_CAN)
  {
      osDelay(100);
      MotecEnterOpMode();
      osDelay(100);
      MotecInit(0);
      MotecInit(1);
  }
  for (;;)
  {
    evt = osMailGet(CtrlMail, param.ctrl_period);
    if (evt.status == osEventMail)
    {
      timeout = 0;
      p = evt.value.p;

      vx = p->vx;
      vy = 0;
      w = p->w;

      v[0] = (vx - vy - w * (param.base.diff.base_a + param.base.diff.base_b)) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
      v[1] = (-(vx + vy + w * (param.base.diff.base_a + param.base.diff.base_b))) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
      v[2] = (vx + vy - w * (param.base.diff.base_a + param.base.diff.base_b)) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
      v[3] = (-(vx - vy + w * (param.base.diff.base_a + param.base.diff.base_b))) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;

      osMailFree(CtrlMail, p);
    }
    else
    {
      timeout++;
      if (timeout >= 1000/param.ctrl_period)
      {
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
        v[3] = 0;
        timeout = 0;
      }
    }
    if (param.motor_type == MOTOR_DJI_CAN)
    {
      motorCurrent[0] = PidCalc(&wheelPid[0], motorInfo[0].w, v[0]);
      motorCurrent[1] = PidCalc(&wheelPid[1], motorInfo[1].w, v[1]);
      motorCurrent[2] = PidCalc(&wheelPid[2], motorInfo[2].w, v[2]);
      motorCurrent[3] = PidCalc(&wheelPid[3], motorInfo[3].w, v[3]);
      SetCanMotor(motorCurrent[0], motorCurrent[1], motorCurrent[2], motorCurrent[3], &hcan1);
    }
    else if (param.motor_type == MOTOR_MOTEC_CAN)
    {
        if (motecCtrlState == 0x7F)
        {
          MotecEnterOpMode();
          osDelay(10);
          MotecInit(0);
          MotecInit(1);
        }
        else
        {
          MotecSetSpeed(v[0], 0);
          MotecSetSpeed(v[1], 1);
        }
    }
  }
}

static void DiffFeedbackTaskEntry(void const *argument)
{
  struct odom odom;
  osDelay(1000);
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

static void DiffPoseTaskEntry(void const *argument)
{
  int i;
  int DeltaTick[4];
  float delta_pos[4];
  float delta_x_m;
  float angle;
  osDelay(1000);
  memset(&pose, 0, sizeof(pose));
  memset(&DeltaTick, 0, sizeof(DeltaTick));
  memset(&delta_pos, 0, sizeof(delta_pos));
  int motor_num;
  if (param.base.diff.base_b == 0)
  {
    motor_num = 2;
  }
  else
  {
    motor_num = 4;
  }
  for (;;)
  {
    for (i = 0; i < motor_num; i++)
    {
      if (param.motor_type == MOTOR_DJI_CAN)
      {
        if ((motorInfo[i].position - motorInfo[i].prePosition) > param.max_ticks / 2) //down overflow
        {
          DeltaTick[i] = ((motorInfo[i].prePosition + param.max_ticks - motorInfo[i].position) % param.max_ticks);
        }
        else if ((motorInfo[i].position - motorInfo[i].prePosition) < -param.max_ticks / 2) //up overflow
        {
          DeltaTick[i] = -((motorInfo[i].position + param.max_ticks - motorInfo[i].prePosition) % param.max_ticks);
        }
        else if (motorInfo[i].position > motorInfo[i].prePosition)
        {
          DeltaTick[i] = -(motorInfo[i].position - motorInfo[i].prePosition);
        }
        else
        {
          DeltaTick[i] = (motorInfo[i].prePosition - motorInfo[i].position);
        }
        motorInfo[i].prePosition = motorInfo[i].position;
      }
      else if (param.motor_type == MOTOR_MOTEC_CAN)
      {
        if ((motecMotorInfo[i].position - motecMotorInfo[i].prePosition) > param.max_ticks / 2) //down overflow
        {
          DeltaTick[i] = ((motecMotorInfo[i].prePosition + param.max_ticks - motecMotorInfo[i].position) % param.max_ticks);
        }
        else if ((motecMotorInfo[i].position - motecMotorInfo[i].prePosition) < -param.max_ticks / 2) //up overflow
        {
          DeltaTick[i] = -((motecMotorInfo[i].position + param.max_ticks - motecMotorInfo[i].prePosition) % param.max_ticks);
        }
        else if (motecMotorInfo[i].position > motecMotorInfo[i].prePosition)
        {
          DeltaTick[i] = -(motecMotorInfo[i].position - motecMotorInfo[i].prePosition);
        }
        else
        {
          DeltaTick[i] = (motecMotorInfo[i].prePosition - motecMotorInfo[i].position);
        }
        motecMotorInfo[i].prePosition = motecMotorInfo[i].position;
      }
      
      delta_pos[i] = (float)DeltaTick[i] / param.ticks_per_lap * 2 * PI * param.wheel_r / param.motor_reduction_ratio;
    }

    delta_pos[0] = -delta_pos[0];
    delta_pos[2] = -delta_pos[2];
    if (param.motor_type == MOTOR_DJI_CAN)
    {
      pose.v_x_m = (motorInfo[0].w - motorInfo[1].w + motorInfo[2].w - motorInfo[3].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / motor_num / 60;
      pose.wz = (-motorInfo[0].w - motorInfo[1].w - motorInfo[2].w - motorInfo[3].w) / param.motor_reduction_ratio / 60 * param.wheel_r * 2 * PI / motor_num / (param.base.diff.base_a + param.base.diff.base_b);
    }
    else if (param.motor_type == MOTOR_MOTEC_CAN)
    {
      pose.v_x_m = (motecMotorInfo[0].w - motecMotorInfo[1].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / motor_num / 60;
      pose.wz = (-motecMotorInfo[0].w - motecMotorInfo[1].w) / param.motor_reduction_ratio / 60 * param.wheel_r * 2 * PI / motor_num / (param.base.diff.base_a + param.base.diff.base_b);
    }

    pose.yaw += (-delta_pos[0] + delta_pos[1] - delta_pos[2] + delta_pos[3]) / motor_num / (param.base.diff.base_a + param.base.diff.base_b);
    pose.v_y_m = 0;
    if (pose.yaw > PI)
    {
      pose.yaw -= 2 * PI;
    }
    else if (pose.yaw < -PI)
    {
      pose.yaw += 2 * PI;
    }

    delta_x_m = (delta_pos[0] + delta_pos[1] + delta_pos[2] + delta_pos[3]) / (float)motor_num;

    //angle = imu.yaw / RADIAN_COEF;
    angle = pose.yaw;
    pose.position_x_m += delta_x_m * cos(angle);
    pose.position_y_m += delta_x_m * sin(angle);

    osDelay(param.pose_calc_period);
  }
}
osThreadDef(DiffCtrlTask, DiffCtrlTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(DiffFeedbackTask, DiffFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(DiffPoseTask, DiffPoseTaskEntry, osPriorityAboveNormal, 0, 512);
void DiffTaskInit(void)
{
  DiffCtrlTaskHandle = osThreadCreate(osThread(DiffCtrlTask), NULL);
  DiffFeedbackTaskHandle = osThreadCreate(osThread(DiffFeedbackTask), NULL);
  DiffPoseTaskHandle = osThreadCreate(osThread(DiffPoseTask), NULL);
}
