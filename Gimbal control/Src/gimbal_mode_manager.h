#ifndef GIMBAL_MODE_MANAGER_H
#define GIMBAL_MODE_MANAGER_H

#include "gimbal_behaviour.h"

typedef struct
{
    gimbal_behaviour_e behaviour;
    fp32 manual_yaw_add;
    fp32 manual_pitch_add;
    uint8_t vision_enabled;
} gimbal_mode_command_t;

void GimbalModeManager_Update(gimbal_control_t *gimbal_control, gimbal_mode_command_t *command);

#endif
