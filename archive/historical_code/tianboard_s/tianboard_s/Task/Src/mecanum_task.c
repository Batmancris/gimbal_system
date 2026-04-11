#include "mecanum_task.h"
#include "can.h"
#include "can_wrapper.h"
#include "cmsis_os.h"
#include "imu.h"
#include "math.h"
#include "motor.h"
#include "param.h"
#include "pid.h"
#include "protocol.h"
#include "protocol_task.h"
#include "rc.h"
#include "rc_task.h"
#include "stdlib.h"
#include "string.h"
#include "odom.h"

osThreadId MecanumCtrlTaskHandle;
osThreadId MecanumFeedbackTaskHandle;
osThreadId MecanumPoseTaskHandle;

static void MecanumCtrlTaskEntry(void const *argument)
{
    osEvent evt;
    MotionCtrl_t *p;
    Pid_t wheelPid[4];
    float w = 0, vx = 0, vy = 0;

    float pre_w = 0, pre_vx = 0, pre_vy = 0;
    float smooth_w = 0, smooth_vx = 0, smooth_vy = 0;
    float linear_acc, angular_acc;
    if (param.time_to_max_v != 0)
    {
        linear_acc = param.max_speed / param.time_to_max_v;
    }
    if (param.time_to_max_w != 0)
    {
        angular_acc = param.max_w / param.time_to_max_w;
    }
    uint32_t old_tick = HAL_GetTick();

    int timeout = 0;
    int16_t motorCurrent[4] = {0, 0, 0, 0};
    float v[4] = {0, 0, 0, 0};

    osDelay(10);

    if (hcan1.State == HAL_CAN_STATE_READY)
    {
        CanParamInit(&hcan1);
    }
    PidInit(&wheelPid[0], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[1], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[2], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);
    PidInit(&wheelPid[3], POSITION_PID, param.pid.max_out, param.pid.i_limit, param.pid.p, param.pid.i, param.pid.d);

    for (;;)
    {
        evt = osMailGet(CtrlMail, param.ctrl_period);

        if (param.time_to_max_w != 0)
        {
            pre_w = smooth_w;
        }
        if (param.time_to_max_v != 0)
        {
            pre_vx = smooth_vx;
            pre_vy = smooth_vy;
        }

        if (evt.status == osEventMail)
        {
            timeout = 0;
            p = evt.value.p;

            vx = p->vx;
            vy = p->vy;
            w = p->w;

            osMailFree(CtrlMail, p);
        }
        else
        {
            timeout++;
            if (timeout >= 1000 / param.ctrl_period)
            {
                vx = 0;
                vy = 0;
                w = 0;
                timeout = 1000;
            }
        }

        if (vx > param.max_speed)
        {
            vx = param.max_speed;
        }
        else if (vx < -param.max_speed)
        {
            vx = -param.max_speed;
        }

        if (vy > param.max_speed)
        {
            vy = param.max_speed;
        }
        else if (vy < -param.max_speed)
        {
            vy = -param.max_speed;
        }

        if (w > param.max_w)
        {
            w = param.max_w;
        }
        else if (w < -param.max_w)
        {
            w = -param.max_w;
        }
        uint32_t new_tick = HAL_GetTick();
        if (param.time_to_max_v != 0)
        {
            if (vx - pre_vx > linear_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_vx = pre_vx + linear_acc * (new_tick - old_tick) / 1000.0f;
            }
            else if (vx - pre_vx < -linear_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_vx = pre_vx - linear_acc * (new_tick - old_tick) / 1000.0f;
            }
            else
            {
                smooth_vx = vx;
            }

            if (vy - pre_vy > linear_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_vy = pre_vy + linear_acc * (new_tick - old_tick) / 1000.0f;
            }
            else if (vy - pre_vy < -linear_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_vy = pre_vy - linear_acc * (new_tick - old_tick) / 1000.0f;
            }
            else
            {
                smooth_vy = vy;
            }
        }
        else
        {
            smooth_vx = vx;
            smooth_vy = vy;
        }
        if (param.time_to_max_w != 0)
        {
            if (w - pre_w > angular_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_w = pre_w + angular_acc * (new_tick - old_tick) / 1000.0f;
            }
            else if (w - pre_w < -angular_acc * (new_tick - old_tick) / 1000.0f)
            {
                smooth_w = pre_w - angular_acc * (new_tick - old_tick) / 1000.0f;
            }
            else
            {
                smooth_w = w;
            }
        }
        else
        {
            smooth_w = w;
        }
        old_tick = new_tick;

        v[0] = (smooth_vx - smooth_vy - smooth_w * (param.base.mecanum.base_a + param.base.mecanum.base_b)) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
        v[1] = (-(smooth_vx + smooth_vy + smooth_w * (param.base.mecanum.base_a + param.base.mecanum.base_b))) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
        v[2] = (smooth_vx + smooth_vy - smooth_w * (param.base.mecanum.base_a + param.base.mecanum.base_b)) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;
        v[3] = (-(smooth_vx - smooth_vy + smooth_w * (param.base.mecanum.base_a + param.base.mecanum.base_b))) * param.motor_reduction_ratio / (2 * PI * param.wheel_r) * 60.0f;

        motorCurrent[0] = PidCalc(&wheelPid[0], motorInfo[0].w, v[0]);
        motorCurrent[1] = PidCalc(&wheelPid[1], motorInfo[1].w, v[1]);
        motorCurrent[2] = PidCalc(&wheelPid[2], motorInfo[2].w, v[2]);
        motorCurrent[3] = PidCalc(&wheelPid[3], motorInfo[3].w, v[3]);
        SetCanMotor(motorCurrent[0], motorCurrent[1], motorCurrent[2], motorCurrent[3], &hcan1);
    }
}

static void MecanumFeedbackTaskEntry(void const *argument)
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

static void MecanumPoseTaskEntry(void const *argument)
{
    int i;
    int DeltaTick[4];
    float delta_pos[4];
    float delta_x_m, delta_y_m;
    float angle;
    osDelay(1000);
    memset(&pose, 0, sizeof(pose));
    for (;;)
    {
        for (i = 0; i < 4; i++)
        {
            if ((motorInfo[i].position - motorInfo[i].prePosition) > param.max_ticks / 2) // down overflow
            {
                DeltaTick[i] = ((motorInfo[i].prePosition + param.max_ticks - motorInfo[i].position) % param.max_ticks);
            }
            else if ((motorInfo[i].position - motorInfo[i].prePosition) < -param.max_ticks / 2) // up overflow
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
            delta_pos[i] = (float)DeltaTick[i] / param.ticks_per_lap * 2 * PI * param.wheel_r / param.motor_reduction_ratio;
        }

        delta_pos[0] = -delta_pos[0];
        delta_pos[2] = -delta_pos[2];

        pose.v_x_m = (motorInfo[0].w - motorInfo[1].w + motorInfo[2].w - motorInfo[3].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / 4 / 60;
        pose.v_y_m = (-motorInfo[0].w - motorInfo[1].w + motorInfo[2].w + motorInfo[3].w) / param.motor_reduction_ratio * 2 * PI * param.wheel_r / 4 / 60;

        pose.wz = (-motorInfo[0].w - motorInfo[1].w - motorInfo[2].w - motorInfo[3].w) / param.motor_reduction_ratio / 60 * param.wheel_r * 2 * PI / 4 / (param.base.mecanum.base_a + param.base.mecanum.base_b);
        pose.yaw += (-delta_pos[0] + delta_pos[1] - delta_pos[2] + delta_pos[3]) / 4 / (param.base.mecanum.base_a + param.base.mecanum.base_b);
        if (pose.yaw > PI)
        {
            pose.yaw -= 2 * PI;
        }
        else if (pose.yaw < -PI)
        {
            pose.yaw += 2 * PI;
        }

        delta_x_m = (delta_pos[0] + delta_pos[1] + delta_pos[2] + delta_pos[3]) / 4.0f;
        delta_y_m = (-delta_pos[0] + delta_pos[1] + delta_pos[2] - delta_pos[3]) / 4.0f;

        // angle = imu.yaw / RADIAN_COEF;
        angle = pose.yaw;
        pose.position_x_m += delta_x_m * cos(angle) - delta_y_m * sin(angle);
        pose.position_y_m += delta_x_m * sin(angle) + delta_y_m * cos(angle);

        osDelay(param.pose_calc_period);
    }
}
osThreadDef(MecanumCtrlTask, MecanumCtrlTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(MecanumFeedbackTask, MecanumFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(MecanumPoseTask, MecanumPoseTaskEntry, osPriorityAboveNormal, 0, 512);
void MecanumTaskInit(void)
{
    MecanumCtrlTaskHandle = osThreadCreate(osThread(MecanumCtrlTask), NULL);
    MecanumFeedbackTaskHandle = osThreadCreate(osThread(MecanumFeedbackTask), NULL);
    MecanumPoseTaskHandle = osThreadCreate(osThread(MecanumPoseTask), NULL);
}
