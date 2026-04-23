# rm_interfaces

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_interfaces` 保存当前工作区共用的 ROS2 自定义消息与服务定义。

### 当前定位

- 主线基础包：是
- 直接运行入口：否
- 主要作用：为检测、传统视觉、控制桥接等模块提供统一接口

### 当前内容

- 装甲板与目标相关消息
- 调试消息
- 底盘 / 云台控制相关消息
- 串口接收与测量相关消息
- `SetMode` 服务定义

### 使用建议

- 如果你在看数据流接口，建议和 `rm_armor_detection`、`rm_gimbal_bridge` 一起对照
- 如果你在看单节点启动，这里通常不是直接入口，而是被其他包依赖

## English

### Overview

`rm_interfaces` contains the shared ROS2 custom messages and service definitions used in this workspace.

### Current Role

- Mainline foundation package: yes
- Standalone runtime entry: no
- Primary purpose: provide common interfaces for detection, traditional vision, and control bridge modules

### Current Contents

- armor and target related messages
- debug messages
- chassis / gimbal control related messages
- serial receive and measurement related messages
- the `SetMode` service definition

### Usage Guidance

- if you are tracing data interfaces, read this together with `rm_armor_detection` and `rm_gimbal_bridge`
- this directory is usually not a direct runtime entry; it is a shared dependency for other packages

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
