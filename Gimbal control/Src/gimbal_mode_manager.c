#include "gimbal_mode_manager.h"

void GimbalModeManager_Update(gimbal_control_t *gimbal_control, gimbal_mode_command_t *command)
{
    if (gimbal_control == NULL || command == NULL)
    {
        return;
    }

    command->manual_yaw_add = 0.0f;
    command->manual_pitch_add = 0.0f;
    gimbal_behaviour_mode_set(gimbal_control);
    gimbal_behaviour_control_set(&command->manual_yaw_add, &command->manual_pitch_add, gimbal_control);
    command->behaviour = gimbal_behaviour_get();
    command->vision_enabled =
      (uint8_t)((command->behaviour == GIMBAL_RELATIVE_ANGLE) ||
                (command->behaviour == GIMBAL_ABSOLUTE_ANGLE));
}
