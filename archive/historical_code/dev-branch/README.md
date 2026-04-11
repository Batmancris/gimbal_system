# dev-branch

[中文](#中文) | [English](#english)

## 中文

### 当前定位

`dev-branch/` 已不再是当前 ROS2 主线工作区。

当前 ROS2 / TROS / RDK-X5 主线已经迁移到：

```text
ros2_ws/
├── src/
│   ├── hik_camera
│   ├── rm_armor_detection
│   ├── rm_gimbal_bridge
│   ├── rm_interfaces
│   └── rm_utils
└── scripts/
```

请优先从根目录 `README.md`、`docs/migration_plan.md` 和 `ros2_ws/README.md` 开始阅读。

### 保留内容

本目录目前保留历史、参考和生成残留内容，例如：

- `armor_detector/`
- `rm_camera_driver/`
- `rm_camera_driver_nv12/`
- `work_handover/`
- `ultralytics-8.2.103/`
- `build/`
- `install/`
- `log/`
- `.git.BAK-20260319/`

这些内容不应作为当前整机运行链的默认入口。需要引用时，请先确认它们是历史参考还是实验代码。

### 当前主链

当前主链仍是：

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

只是 ROS2 物理路径已经从 `dev-branch/` 迁到 `ros2_ws/`。

## English

### Current Role

`dev-branch/` is no longer the active ROS2 workspace.

The current ROS2 / TROS / RDK-X5 mainline has moved to:

```text
ros2_ws/
├── src/
│   ├── hik_camera
│   ├── rm_armor_detection
│   ├── rm_gimbal_bridge
│   ├── rm_interfaces
│   └── rm_utils
└── scripts/
```

Start with the root `README.md`, `docs/migration_plan.md`, and `ros2_ws/README.md`.

### Retained Content

This directory currently retains historical, reference, and generated content, including:

- `armor_detector/`
- `rm_camera_driver/`
- `rm_camera_driver_nv12/`
- `work_handover/`
- `ultralytics-8.2.103/`
- `build/`
- `install/`
- `log/`
- `.git.BAK-20260319/`

Do not treat these as the default whole-system runtime entry points.
