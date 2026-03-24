#include "gimbal_mode_manager.h"

void GimbalModeManager_Update(gimbal_control_t *gimbal_control, gimbal_mode_command_t *command)
{
    uint8_t behaviour_supports_vision;
    uint8_t rc_ready = 0U;
    uint8_t vision_requested = 0U;

    if (gimbal_control == NULL || command == NULL)
    {
        return;
    }

    command->manual_yaw_add = 0.0f;
    command->manual_pitch_add = 0.0f;
    gimbal_behaviour_mode_set(gimbal_control);
    gimbal_behaviour_control_set(&command->manual_yaw_add, &command->manual_pitch_add, gimbal_control);
    command->behaviour = gimbal_behaviour_get();

    behaviour_supports_vision =
      (uint8_t)((command->behaviour == GIMBAL_RELATIVE_ANGLE) ||
                (command->behaviour == GIMBAL_ABSOLUTE_ANGLE));

    if (gimbal_control->gimbal_rc_ctrl != NULL && !RC_data_is_error() && !toe_is_error(DBUS_TOE))
    {
        rc_ready = 1U;
        vision_requested =
          (uint8_t)switch_is_up(gimbal_control->gimbal_rc_ctrl->rc.s[GIMBAL_MODE_CHANNEL]);
    }

    command->vision_enabled = (uint8_t)(behaviour_supports_vision && rc_ready && vision_requested);

    // Keep manual control on the direct RC path. rc_ready only gates whether
    // vision is allowed to take over; it should not additionally zero a manual
    // command path that already comes from the parsed RC state.
    if (command->vision_enabled)
    {
        command->manual_yaw_add = 0.0f;
        command->manual_pitch_add = 0.0f;
    }
}
