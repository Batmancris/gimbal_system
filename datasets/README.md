# datasets

[中文](#中文) | [English](#english)

## 中文

### 目录约定

本目录用于 TianAim 未来的数据闭环，不直接替代当前运行代码。

当前骨架：

```text
datasets/
├── raw/         # 原始采集图像
├── labeled/     # 标注后的数据
├── splits/      # train/val/test 切分
└── manifests/   # session manifest 与元数据
```

### 命名建议

建议图片命名格式：

```text
<session_id>_<camera_id>_<timestamp_ns>_<frame_index>.jpg
```

### 注意事项

- 不要默认把大体量原始数据直接提交到 Git
- manifest 和小体量样例可以保留在仓库中
- 真实数据建议通过对象存储或专用数据盘管理

## English

This directory is the product-facing dataset root for TianAim.

Suggested image naming:

```text
<session_id>_<camera_id>_<timestamp_ns>_<frame_index>.jpg
```

Keep metadata and small examples in Git. Large raw datasets should usually stay outside the repository.

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
