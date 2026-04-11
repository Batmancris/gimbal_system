#include "omni_task.h"
#include "cmsis_os.h"
#include "rc.h"
#include "can_wrapper.h"
#include "can.h"
#include "rc_task.h"
#include "motor.h"
#include "pid.h"
#include "param.h"
#include "stdlib.h"
#include "protocol_task.h"
#include "protocol.h"
#include "imu.h"
#include "math.h"
#include "motor.h"
#include "odom.h"

osThreadId OmniCtrlTaskHandle;
osThreadId OmniFeedbackTaskHandle;
osThreadId OmniPoseTaskHandle;

static void OmniCtrlTaskEntry(void const *argument)
{
  osEvent evt;
  MotionCtrl_t *p;
  Pid_t wheelPid[3];
  float w, vx, vy;

  int timeout = 0;
  int16_t motorCurrent[3] = {0, 0, 0};
  int16_t v[3] = {0, 0, 0};

  osDelay(1000);

  CanParamInit(&hcan1);

  PidInit(&wheelPid[0], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
  PidInit(&wheelPid[1], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
  PidInit(&wheelPid[2], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);

  for (;;)
  {
    evt = osMailGet(CtrlMail, param.ctrl_period);
    if (evt.status == osEventMail)
    {
      timeout = 0;
      p = evt.value.p;

      vx = p->vx;
      vy = p->vy;
      w = p->w;

      v[0] = (-vy - w * param.base.omni.base_r) / (2 * PI * param.wheel_r) * 60.0f * param.motor_reduction_ratio;
      v[1] = (vy * COS_60 - vx * SIN_60 - w * param.base.omni.base_r) / (2 * PI * param.wheel_r) * 60.0f * param.motor_reduction_ratio;
      v[2] = (vy * COS_60 + vx * SIN_60 - w * param.base.omni.base_r) / (2 * PI * param.wheel_r) * 60.0f * param.motor_reduction_ratio;
      osMailFree(CtrlMail, p);
    }
    else
    {
      timeout++;
      if (timeout >= 100)
      {
        v[0] = 0;
        v[1] = 0;
        v[2] = 0;
        timeout = 0;
      }
    }
    motorCurrent[0] = PidCalc(&wheelPid[0], motorInfo[0].w, v[0]);
    motorCurrent[1] = PidCalc(&wheelPid[1], motorInfo[1].w, v[1]);
    motorCurrent[2] = PidCalc(&wheelPid[2], motorInfo[2].w, v[2]);
    SetCanMotor(motorCurrent[0], motorCurrent[1], motorCurrent[2], 0, &hcan1);
  }
}

static void OmniFeedbackTaskEntry(void const *argument)
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

static void OmniPoseTaskEntry(void const *argument)
{
  int i;
  int DeltaTick[3];
  float delta_pos[3];
  float delta_x_m, delta_y_m;
  float angle;
  memset(&pose, 0, sizeof(pose));
  for (;;)
  {
    for (i = 0; i < 3; i++)
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
      delta_pos[i] = DeltaTick[i] / param.ticks_per_lap * 2 * PI * param.wheel_r / param.motor_reduction_ratio;
    }

    pose.v_x_m = (-motorInfo[1].w + motorInfo[2].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / 3 / 60;
    pose.v_y_m = (-motorInfo[0].w * 2 + motorInfo[1].w + motorInfo[2].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / 2 / SIN_60 / 60;

    pose.wz = (-motorInfo[0].w - motorInfo[1].w - motorInfo[2].w) / 3 / param.motor_reduction_ratio * 2 * PI * param.wheel_r / 60 / param.base.omni.base_r;
    pose.yaw += (delta_pos[0] + delta_pos[1] + delta_pos[2]) / 3 / param.base.omni.base_r;

    if (pose.yaw > PI)
    {
      pose.yaw -= 2 * PI;
    }
    else if (pose.yaw < -PI)
    {
      pose.yaw += 2 * PI;
    }

    delta_x_m = -(-delta_pos[1] + delta_pos[2]) / 2.0f / SIN_60;
    delta_y_m = -(-2 * delta_pos[0] + delta_pos[1] + delta_pos[2]) / 3.0f;

    //angle = imu.yaw / RADIAN_COEF;
    angle = pose.yaw;
    pose.position_x_m += delta_x_m * cos(angle) - delta_y_m * sin(angle);
    pose.position_y_m += delta_x_m * sin(angle) + delta_y_m * cos(angle);

    osDelay(param.pose_calc_period);
  }
}

osThreadDef(OmniCtrlTask, OmniCtrlTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(OmniFeedbackTask, OmniFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(OmniPoseTask, OmniPoseTaskEntry, osPriorityAboveNormal, 0, 512);
void OmniTaskInit(void)
{
  OmniCtrlTaskHandle = osThreadCreate(osThread(OmniCtrlTask), NULL);
  OmniFeedbackTaskHandle = osThreadCreate(osThread(OmniFeedbackTask), NULL);
  OmniPoseTaskHandle = osThreadCreate(osThread(OmniPoseTask), NULL);
}
