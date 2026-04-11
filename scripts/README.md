# scripts

Top-level convenience entry points for the current repository.

These wrappers intentionally preserve the current physical source tree while giving contributors a cleaner product-facing entry.

Path compatibility:

- `TIANAIM_ROS_WS` can override the ROS2 workspace path.
- `TIANAIM_FIRMWARE_DIR` can override the firmware project path.
- By default, ROS2 resolves to `ros2_ws/`.
- By default, firmware resolves to `firmware/stm32_gimbal_control/`.

Current scripts:

- `build_ros2_mainline.sh`
- `build_firmware_mainline.sh`
- `run_ros2_bridge.sh`
- `tianaim_paths.sh`
