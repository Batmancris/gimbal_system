# hik_camera

[中文](#中文) | [English](#english)

## 中文

### 简介

`hik_camera` 是当前上位机主线使用的海康工业相机 ROS2 驱动。

它负责：

- 枚举并打开海康工业相机
- 发布标准 ROS 图像流 `image_raw`
- 发布共享内存图像 `hbmem_img`
- 加载相机内参
- 支持运行时调整曝光和增益

### 当前定位

- 相机主线：是
- 当前推荐使用：是
- 依赖：海康相机 SDK、ROS2、相关图像消息依赖

### 主要话题

- 发布 `image_raw`
- 发布 `/hbmem_img`

### 主要参数

- `camera_name`
- `frame_id`
- `exposure_time`
- `gain`
- `camera_info_url`
- `use_sensor_data_qos`

### 当前联调说明

- 当前 RM 装甲板主线更偏向较暗、高对比度画面，以突出灯条
- 目前项目内联调使用的曝光参数比默认更高，用于在现有环境下稳定看到目标灯条
- 某些海康相机型号或 SDK 组合下，`gain` 可能不会按配置成功写入，需要以实际启动日志为准
- `image_raw` 会被桌面可视化节点直接订阅，用于叠加检测框显示

### 运行

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### 说明

当前项目里虽然还保留了其他相机驱动目录，但主线优先维护本包。

### 阅读建议

- 若你在看整机链路，请同时参考 `rm_armor_detection/README.md` 和 `rm_gimbal_bridge/README.md`
- 若你只是在排查相机输入、曝光或共享内存链路，本 README 就是当前相机侧入口

## English

### Overview

`hik_camera` is the current ROS2 mainline driver for the Hikrobot industrial camera.

It is responsible for:

- enumerating and opening the Hikrobot camera
- publishing standard ROS image data on `image_raw`
- publishing shared-memory image data on `hbmem_img`
- loading camera calibration data
- supporting runtime exposure and gain updates

### Current Role

- Active camera mainline: yes
- Recommended default path: yes
- Dependencies: Hikrobot SDK, ROS2, and related image message packages

### Main Topics

- publishes `image_raw`
- publishes `/hbmem_img`

### Main Parameters

- `camera_name`
- `frame_id`
- `exposure_time`
- `gain`
- `camera_info_url`
- `use_sensor_data_qos`

### Current Integration Notes

- The current RM armor-detection workflow intentionally favors darker, higher-contrast images so the light bars stand out more clearly
- The present validation setup uses a higher exposure than the earlier default to make the target lighting easier to inspect in the current environment
- On some Hikrobot camera and SDK combinations, the configured `gain` may not be accepted at runtime, so the startup log is the source of truth
- `image_raw` is also consumed by the desktop visualizer for live overlay rendering

### Run

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### Note

Other camera driver directories still exist in the workspace, but this package is the preferred camera mainline to maintain.

### Reading Guidance

- for the full runtime chain, read this together with `rm_armor_detection/README.md` and `rm_gimbal_bridge/README.md`
- if you are only diagnosing camera bring-up, exposure, or shared-memory transport, this README is the correct camera-side entry

## 2026-04-23 Vision PID Tuning Record

This repository state records the completed RDK X5 to STM32 C-board visual closed-loop tuning pass. The model files were not changed in this pass; the work focused on bridge latency, target safety, and lower-board vision PID behavior.

Final lower-board firmware parameters are in `firmware/stm32_gimbal_control/Src/gimbal_task.h`:

- `VISION_X_DEADBAND = 14.0f`, `VISION_Y_DEADBAND = 14.0f`
- `VISION_YAW_PID_KP = 0.0000072f`, `VISION_YAW_PID_KI = 0.0f`, `VISION_YAW_PID_KD = 0.000055f`
- `VISION_PITCH_PID_KP = 0.0000060f`, `VISION_PITCH_PID_KI = 0.0f`, `VISION_PITCH_PID_KD = 0.000042f`
- `VISION_MAX_ANGLE_STEP = 0.0045f`, `VISION_FAST_ANGLE_STEP = 0.0065f`, `VISION_FAST_ERROR_THRESHOLD = 160.0f`
- `VISION_CMD_SMOOTH_ALPHA = 0.42f`, `VISION_CMD_FAST_ALPHA = 0.58f`, `VISION_CMD_BRAKE_ALPHA = 0.92f`
- `VISION_SLOWDOWN_ERROR_PX = 220.0f`, `VISION_MIN_STEP_SCALE = 0.10f`
- `VISION_FRAME_HOLD_DECAY = 0.990f`, `VISION_FRAME_BRAKE_DECAY = 0.970f`
- `TARGET_STATE_SMOOTH_ALPHA = 1.00f` in `firmware/stm32_gimbal_control/Src/target_state.h`

Final behavior summary:

- The ROS bridge sends low-latency target centers and rejects unsafe target jumps instead of falling back to a different detection on the opposite side of the image.
- When target detection is lost or tracking continuity breaks, the bridge sends a neutral center frame so the lower board clears residual velocity instead of continuing to rotate blindly.
- The lower board uses frame-triggered vision PD updates with frame-to-frame command decay, braking alpha, and quadratic slowdown near image center. This keeps the fast-follow speed while reducing hard acceleration, overshoot, and stale-command drift.
- The current tested camera/detector path remains `hik_camera -> rm_vehicle_detection -> rm_gimbal_bridge -> STM32 USB-CDC` at roughly 30 FPS without visualization.
