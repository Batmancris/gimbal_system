# TianAim

<p align="center">
  <img src="assets/tianaim_readme.svg" alt="TianAim auto-aim runtime chain" width="100%">
</p>

TianAim is the current integrated workspace for the RDK-X5 ROS2/TROS stack, the STM32 gimbal controller, dataset tooling, and model training assets.

The active runtime chain on board is:

```text
hik_camera
  -> /hbmem_img
  -> rm_armor_detection or rm_vehicle_detection
  -> ai_msgs/msg/PerceptionTargets
  -> rm_gimbal_bridge
  -> USB-CDC / UART
  -> STM32 vision_input
```

## Current status

- `ros2_ws/`: active ROS2 workspace for camera, detectors, bridge, launch scripts, and RDK deployment helpers
- `firmware/stm32_gimbal_control/`: active STM32F407 firmware
- `tools/`: capture, labeling, training, and evaluation helpers
- `datasets/` and `models/`: lightweight manifests, reports, and export metadata
- `archive/`: audit and recovery notes only

## Quick start

Build the ROS2 mainline packages:

```bash
bash scripts/build_ros2_mainline.sh
```

This currently builds:

```text
hik_camera rm_armor_detection rm_vehicle_detection rm_gimbal_bridge
```

Run the bridge-only launch entry:

```bash
bash scripts/run_ros2_bridge.sh
```

For the board-side full autoaim workflow, read [ros2_ws/README.md](ros2_ws/README.md).

## Repository map

```text
.
├── ros2_ws/
├── firmware/stm32_gimbal_control/
├── datasets/
├── models/
├── tools/
├── scripts/
├── assets/
└── archive/
```

Key package entry points:

- `ros2_ws/src/hik_camera/`
- `ros2_ws/src/rm_armor_detection/`
- `ros2_ws/src/rm_vehicle_detection/`
- `ros2_ws/src/rm_gimbal_bridge/`
- `firmware/stm32_gimbal_control/`

## Documentation

- ROS2 runtime and board workflow: [ros2_ws/README.md](ros2_ws/README.md)
- Bridge package notes: [ros2_ws/src/rm_gimbal_bridge/README.md](ros2_ws/src/rm_gimbal_bridge/README.md)
- Vehicle detector notes: [ros2_ws/src/rm_vehicle_detection/README.md](ros2_ws/src/rm_vehicle_detection/README.md)
- Top-level helper scripts: [scripts/README.md](scripts/README.md)

## Maintenance note

When changing package topology, launch arguments, scripts, or runtime topics, update the affected `README.md` files in the same change so the docs stay aligned with the local codebase.

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
