# rm_gimbal_bridge

`rm_gimbal_bridge` converts detector output topics into the serial target stream consumed by the STM32 gimbal controller.

## Runtime role

- subscribes to `ai_msgs/msg/PerceptionTargets`
- selects one target
- sends the target center through the configured serial device
- provides the integrated autoaim launch entry
- keeps USB CDC diagnostic utilities in the same package

Serial frame format:

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

## Default behavior

- default detector input topic: `/dnn_node_sample`
- default serial port in node config: `/dev/ttyS1`
- board-side scripts usually override the serial port to the USB-CDC by-id device
- default baud rate: `921600`
- default selection mode: `closest`

## Main parameters

- `input_topic`
- `serial_port`
- `baud_rate`
- `image_width`
- `image_height`
- `image_center_x`
- `image_center_y`
- `min_confidence`
- `enemy_prefix`
- `allowed_target_types`
- `selection_mode`
- `require_lower_vision_enabled`

## Detector integration

Armor mode:

```text
/hbmem_img -> rm_armor_detection -> /dnn_node_sample -> rm_gimbal_bridge
```

Vehicle mode can be wired in two ways:

```text
/hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets -> rm_gimbal_bridge
```

or, for a fully shared downstream contract:

```text
/hbmem_img -> rm_vehicle_detection -> /dnn_node_sample -> rm_gimbal_bridge
```

`rm_autoaim_system.launch.py` already supports detector switching with:

- `detector_type:=armor`
- `detector_type:=vehicle`
- `detector_topic:=...`
- `vehicle_model_path:=...`

## Run

Bridge only:

```bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py
```

Whole system, armor mode:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py
```

Whole system, vehicle mode:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py \
  detector_type:=vehicle \
  detector_topic:=/dnn_node_sample
```

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
