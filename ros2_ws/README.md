# ros2_ws

This directory is the active ROS2 / TROS / RDK-X5 workspace.

Current reality:

- the active packages live under `ros2_ws/src/`
- the active RDK deployment and runtime scripts live under `ros2_ws/scripts/`
- historical ROS2 material is retained under `archive/historical_code/dev-branch/`

Current layout:

```text
ros2_ws/
├── scripts/
└── src/
    ├── hik_camera
    ├── rm_armor_detection
    ├── rm_gimbal_bridge
    ├── rm_interfaces
    └── rm_utils
```

Build:

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
```

The top-level wrapper `bash scripts/build_ros2_mainline.sh` resolves to this workspace by default.
