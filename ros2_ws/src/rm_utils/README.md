# rm_utils

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_utils` 是当前工作区中的公共工具库，给多个 ROS2 包提供共享能力。

### 当前包含内容

- 扩展卡尔曼滤波器
- PnP 解算器
- 弹道补偿器
- 日志封装
- URL 路径解析
- 心跳发布器

### 当前定位

- 主线基础库：是
- 直接对外运行：通常否
- 主要作用：减少各包重复实现，统一公共数学与工具能力

### 适用场景

- 检测与跟踪算法共用数学模块
- 相机与视觉节点共用工具能力
- ROS2 节点复用日志、路径解析、心跳发布等基础设施

### 阅读建议

- 若你在排查日志封装，可继续看 `include/rm_utils/logger/README.md`
- 若你在看整机链路，本目录通常不是第一入口，而是被多个主线包复用

## English

### Overview

`rm_utils` is the shared utility library used across multiple ROS2 packages in this workspace.

### Current Contents

- Extended Kalman Filter
- PnP solver
- trajectory compensator
- logging wrapper
- URL path resolver
- heartbeat publisher

### Current Role

- Mainline foundation library: yes
- Standalone runtime entry: usually no
- Primary purpose: reduce duplicated implementations and centralize common math and utility capabilities

### Common Use Cases

- shared math modules for detection and tracking
- reusable utility support for camera and vision nodes
- common ROS2 infrastructure such as logging, path resolution, and heartbeat publication

### Reading Guidance

- if you are investigating the logging wrapper, continue with `include/rm_utils/logger/README.md`
- for the whole runtime path, this directory is usually a shared dependency rather than the first entry point

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
