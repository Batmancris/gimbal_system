# ros2_ws

This directory is the product-facing transition anchor for the future ROS2 workspace layout.

Current reality:

- the active workspace is still `dev-branch/`

Target direction:

```text
ros2_ws/
└── src/
    ├── hik_camera
    ├── rm_armor_detection
    ├── rm_gimbal_bridge
    ├── rm_interfaces
    └── rm_utils
```

Why it is not moved yet:

- existing deployment scripts still assume `dev-branch/`
- current `colcon` and remote bring-up flow already work there
- a directory move should happen only with coordinated script and README updates
