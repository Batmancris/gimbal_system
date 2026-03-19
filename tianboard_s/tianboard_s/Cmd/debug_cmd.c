#include "fw_update.h"
#include "param.h"
#include "protocol_task.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "version.h"
#include "odom.h"

void version_debug_cmd(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    snprintf(ret, max_ret_len, "tianboard_s v%s\r\nBuild Time: %s %s\r\nGIT commit ID: %s\r\n", __FIRMWARE_VERSION__, __DATE__, __TIME__, __GIT_COMMIT_ID__);
}

void update_debug_cmd(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    FwUpdate();
}

static void SetParam(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    if (argc != 3)
    {
        snprintf(ret, max_ret_len, "usage:\r\nparam get/set/save/reset [name] [value]\r\n");
        return;
    }

    if (strcmp("desc", argv[1]) == 0)
    {
        memset(param.desc, 0, sizeof(param.desc));
        strncpy(param.desc, argv[2], sizeof(param.desc));
    }
    else if (strcmp("base_type", argv[1]) == 0)
    {
        if (strcmp("omni", argv[2]) == 0)
        {
            param.base_type = BASE_TYPE_OMNI;
        }
        else if (strcmp("mecanum", argv[2]) == 0)
        {
            param.base_type = BASE_TYPE_MECANUM;
        }
        else if (strcmp("diff", argv[2]) == 0)
        {
            param.base_type = BASE_TYPE_DIFF;
        }
        else if (strcmp("ackermann", argv[2]) == 0)
        {
            param.base_type = BASE_TYPE_ACKERMANN;
        }
        else
        {
            snprintf(ret, max_ret_len, "support base type: mecanum/omni/diff/ackermann\r\n");
        }
    }
    else if (strcmp("base_a", argv[1]) == 0)
    {
        if (param.base_type == BASE_TYPE_MECANUM)
        {
            param.base.mecanum.base_a = atof(argv[2]);
        }
        else if (param.base_type == BASE_TYPE_DIFF)
        {
            param.base.diff.base_a = atof(argv[2]);
        }
        else if (param.base_type == BASE_TYPE_MECANUM)
        {
            param.base.acker.base_a = atof(argv[2]);
        }
        else
        {
            snprintf(ret, max_ret_len, "base type error\r\nset base type first\r\nsupport base type: mecanum/omni/diff/ackermann\r\n");
        }
    }
    else if (strcmp("base_b", argv[1]) == 0)
    {
        if (param.base_type == BASE_TYPE_MECANUM)
        {
            param.base.mecanum.base_b = atof(argv[2]);
        }
        else if (param.base_type == BASE_TYPE_DIFF)
        {
            param.base.diff.base_b = atof(argv[2]);
        }
        else if (param.base_type == BASE_TYPE_MECANUM)
        {
            param.base.acker.base_b = atof(argv[2]);
        }
        else
        {
            snprintf(ret, max_ret_len, "base type error\r\nset base type first\r\nsupport base type: mecanum/omni/diff/ackermann\r\n");
        }
    }
    else if (strcmp("motor_servo_type", argv[1]) == 0)
    {
        if (strcmp("pwm", argv[2]) == 0)
        {
            param.base.acker.motor_servo_type = MOTOR_SERVO_TYPE_PWM;
        }
        else if (strcmp("can_uart", argv[2]) == 0)
        {
            param.base.acker.motor_servo_type = MOTOR_SERVO_TYPE_CAN_UART;
        }
        else
        {
            snprintf(ret, max_ret_len, "support motor/servo type: pwm/can_uart\r\n");
        }
    }
    else if (strcmp("pwm_dead_zone", argv[1]) == 0)
    {
        param.base.acker.pwm_dead_zone = atof(argv[2]);
    }
    else if (strcmp("max_steer_angle", argv[1]) == 0)
    {
        param.base.acker.max_steer_angle = atof(argv[2]);
    }
    else if (strcmp("steering_offset", argv[1]) == 0)
    {
        param.base.acker.steering_offset = atof(argv[2]);
    }
    else if (strcmp("steering_ratio", argv[1]) == 0)
    {
        param.base.acker.steering_ratio = atof(argv[2]);
    }
    else if (strcmp("base_r", argv[1]) == 0)
    {
        param.base.omni.base_r = atof(argv[2]);
    }
    else if (strcmp("wheel_r", argv[1]) == 0)
    {
        param.wheel_r = atof(argv[2]);
    }
    else if (strcmp("motor_reduction", argv[1]) == 0)
    {
        param.motor_reduction_ratio = atof(argv[2]);
    }
    else if (strcmp("max_w", argv[1]) == 0)
    {
        param.max_w = atof(argv[2]);
    }
    else if (strcmp("max_speed", argv[1]) == 0)
    {
        param.max_speed = atof(argv[2]);
    }
    else if (strcmp("pid_p", argv[1]) == 0)
    {
        param.pid.p = atof(argv[2]);
    }
    else if (strcmp("pid_i", argv[1]) == 0)
    {
        param.pid.i = atof(argv[2]);
    }
    else if (strcmp("pid_d", argv[1]) == 0)
    {
        param.pid.d = atof(argv[2]);
    }
    else if (strcmp("pid_max_out", argv[1]) == 0)
    {
        param.pid.max_out = atof(argv[2]);
    }
    else if (strcmp("pid_i_limit", argv[1]) == 0)
    {
        param.pid.i_limit = atof(argv[2]);
    }
    else if (strcmp("ticks_per_lap", argv[1]) == 0)
    {
        param.ticks_per_lap = atoi(argv[2]);
    }
    else if (strcmp("max_ticks", argv[1]) == 0)
    {
        param.max_ticks = atoi(argv[2]);
    }
    else if (strcmp("ctrl_period", argv[1]) == 0)
    {
        param.ctrl_period = atoi(argv[2]);
    }
    else if (strcmp("feedback_period", argv[1]) == 0)
    {
        param.feedback_period = atoi(argv[2]);
    }
    else if (strcmp("pose_calc_period", argv[1]) == 0)
    {
        param.pose_calc_period = atoi(argv[2]);
    }
    else if (strcmp("mag_v", argv[1]) == 0)
    {
        param.mag_v = atof(argv[2]);
    }
    else if (strcmp("mag_max_w", argv[1]) == 0)
    {
        param.mag_max_w = atof(argv[2]);
    }
    else if (strcmp("time_to_max_v", argv[1]) == 0)
    {
        param.time_to_max_v = atof(argv[2]);
    }
    else if (strcmp("time_to_max_w", argv[1]) == 0)
    {
        param.time_to_max_w = atof(argv[2]);
    }
    else if (strcmp("rc_max", argv[1]) == 0)
    {
        param.rc_param.max = atoi(argv[2]);
    }
    else if (strcmp("rc_min", argv[1]) == 0)
    {
        param.rc_param.min = atoi(argv[2]);
    }
    else if (strcmp("rc_mid", argv[1]) == 0)
    {
        param.rc_param.mid = atoi(argv[2]);
    }
    else if (strcmp("rc_dead_zone", argv[1]) == 0)
    {
        param.rc_param.dead_zone = atoi(argv[2]);
    }
    else if (strcmp("motor_type", argv[1]) == 0)
    {
        if (strcmp("dji", argv[2]) == 0)
        {
            param.motor_type = MOTOR_DJI_CAN;
        }
        else if (strcmp("motec_canopen", argv[2]) == 0)
        {
            param.motor_type = MOTOR_MOTEC_CAN;
        }
        else
        {
            snprintf(ret, max_ret_len, "support motor type: dji/motec_canopen\r\n");
        }
    }
    else
    {
        snprintf(ret, max_ret_len, "no param %s found\r\n", argv[1]);
    }
}

static void GetParam(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    snprintf(ret, max_ret_len, "desc: %s\r\n", param.desc);

    if (param.base_type == BASE_TYPE_MECANUM)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_type: mecanum\r\n");
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_a: %.3f\r\n", param.base.mecanum.base_a);
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_b: %.3f\r\n", param.base.mecanum.base_b);
    }
    else if (param.base_type == BASE_TYPE_DIFF)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_type: diff\r\n");
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_a: %.3f\r\n", param.base.diff.base_a);
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_b: %.3f\r\n", param.base.diff.base_b);
    }
    else if (param.base_type == BASE_TYPE_OMNI)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_type: omni\r\n");
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_r: %.3f\r\n", param.base.omni.base_r);
    }
    else if (param.base_type == BASE_TYPE_ACKERMANN)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_type: ackermann\r\n");
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_a: %.3f\r\n", param.base.acker.base_a);
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_b: %.3f\r\n", param.base.acker.base_b);
        if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_PWM)
        {
            snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_servo_type: pwm\r\n");
            snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pwm_dead_zone: %.3f\r\n", param.base.acker.pwm_dead_zone);
        }
        else if (param.base.acker.motor_servo_type == MOTOR_SERVO_TYPE_CAN_UART)
        {
            snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_servo_type: can_uart\r\n");
        }
        else
        {
            snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_servo_type: unknown\r\n");
        }

        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "max_steer_angle: %.3f\r\n", param.base.acker.max_steer_angle);
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "steering_offset: %.3f\r\n", param.base.acker.steering_offset);
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "steering_ratio: %.3f\r\n", param.base.acker.steering_ratio);
    }
    else
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "base_type: unknown\r\n");
    }

    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "wheel_r: %.3f\r\n", param.wheel_r);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_reduction: %.3f\r\n", param.motor_reduction_ratio);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "max_w: %.3f\r\n", param.max_w);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "max_speed: %.3f\r\n", param.max_speed);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pid_p: %.3f\r\n", param.pid.p);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pid_i: %.3f\r\n", param.pid.i);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pid_d: %.3f\r\n", param.pid.d);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pid_max_out: %.3f\r\n", param.pid.max_out);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pid_i_limit: %.3f\r\n", param.pid.i_limit);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "ticks_per_lap: %ld\r\n", param.ticks_per_lap);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "max_ticks: %ld\r\n", param.max_ticks);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "ctrl_period: %d\r\n", param.ctrl_period);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "feedback_period: %d\r\n", param.feedback_period);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "pose_calc_period: %d\r\n", param.pose_calc_period);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "mag_v: %.3f\r\n", param.mag_v);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "mag_max_w: %.3f\r\n", param.mag_max_w);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "time_to_max_v: %.3f\r\n", param.time_to_max_v);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "time_to_max_w: %.3f\r\n", param.time_to_max_w);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "rc_max: %ld\r\n", param.rc_param.max);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "rc_min: %ld\r\n", param.rc_param.min);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "rc_mid: %ld\r\n", param.rc_param.mid);
    snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "rc_dead_zone: %ld\r\n", param.rc_param.dead_zone);

     if (param.motor_type == MOTOR_DJI_CAN)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_type: dji\r\n");
    }
    else if (param.motor_type == MOTOR_MOTEC_CAN)
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_type: motec_canopen\r\n");
    }
    else
    {
        snprintf(ret + strlen(ret), max_ret_len - strlen(ret), "motor_type: unknown\r\n");
    }
}

void param_debug_cmd(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    if (argc == 2)
    {
        if (strcmp("save", argv[1]) == 0)
        {
            vPortEnterCritical();
            SaveParam(&param);
            vPortExitCritical();
            snprintf(ret, max_ret_len, "cmd param save done\r\n");
        }
        else if (strcmp("get", argv[1]) == 0)
        {
            GetParam(argc - 1, &argv[1], ret, max_ret_len);
        }
        else if (strcmp("reset", argv[1]) == 0)
        {
            vPortEnterCritical();
            SaveParam((void *)&DefaultParam);
            InitParam();
            vPortExitCritical();
            snprintf(ret, max_ret_len, "cmd param reset done\r\n");
        }
        else
        {
            snprintf(ret, max_ret_len, "usage:\r\nparam get/set/save/reset [name] [value]\r\n");
        }
    }
    else if (argc == 4)
    {
        if (strcmp("set", argv[1]) == 0)
        {
            SetParam(argc - 1, &argv[1], ret, max_ret_len);
            snprintf(ret, max_ret_len, "cmd param set done\r\n");
        }
        else
        {
            snprintf(ret, max_ret_len, "usage:\r\nparam get/set/save/reset [name] [value]\r\n");
        }
    }
    else
    {
        snprintf(ret, max_ret_len, "usage:\r\nparam get/set/save/reset [name] [value]\r\n");
    }
}

void reset_debug_cmd(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    HAL_NVIC_SystemReset();
}

static void set_odom(int argc, char *argv[], char *ret, uint16_t max_ret_len)
{
    if (argc != 4)
    {
        snprintf(ret, max_ret_len, "usage:\r\nset_odom x y yaw\r\n");
        return;
    }
    pose.position_x_m = atof(argv[1]);
    pose.position_y_m = atof(argv[2]);
    pose.yaw = atof(argv[3]);
    snprintf(ret, max_ret_len, "odom set to x %f, y %f, yaw %f\r\n", pose.position_x_m, pose.position_y_m, pose.yaw);
}

ADD_DEBUG_CMD("version", version_debug_cmd, "get firmware version.")
ADD_DEBUG_CMD("update", update_debug_cmd, "reboot for fw update.")
ADD_DEBUG_CMD("param", param_debug_cmd, "get/set/save/reset parameter.")
ADD_DEBUG_CMD("reset", reset_debug_cmd, "reset board.")
ADD_DEBUG_CMD("set_odom", set_odom, "set odom x y yaw.")
