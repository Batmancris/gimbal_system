# scripts

Top-level convenience wrappers for the current repository.

## Path resolution

- `TIANAIM_ROS_WS` overrides the ROS2 workspace path
- `TIANAIM_FIRMWARE_DIR` overrides the firmware path
- default ROS2 workspace: `ros2_ws/`
- default firmware path: `firmware/stm32_gimbal_control/`

## Current scripts

- `build_ros2_mainline.sh`: builds `hik_camera rm_armor_detection rm_vehicle_detection rm_gimbal_bridge`
- `build_firmware_mainline.sh`: builds the STM32 firmware mainline
- `run_ros2_bridge.sh`: launches `rm_gimbal_bridge.launch.py`
- `tianaim_paths.sh`: shared path resolver used by the wrappers
