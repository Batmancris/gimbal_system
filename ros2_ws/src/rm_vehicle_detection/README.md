# rm_vehicle_detection

`rm_vehicle_detection` is the YOLOv8-based vehicle detector package for the current RDK-X5 runtime chain.

## Runtime role

- subscribes to `/hbmem_img`
- runs D-Robotics `dnn_node` inference
- parses YOLOv8 detect outputs
- publishes `ai_msgs/msg/PerceptionTargets`

Default topic layout:

```text
/hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets
```

In the integrated autoaim launch and tmux scripts, the output topic can also be overridden to `/dnn_node_sample` so the bridge keeps the same downstream contract as armor mode.

## Parameters

- `image_topic`: default `/hbmem_img`
- `output_topic`: default `/vehicle_detection/targets`
- `target_type`: default `vehicle`
- `model_path`: default `/opt/tros/lib/rm_vehicle_detection/config/quant.bin`
- `box_format`: default `xyxy`, set to `cxcywh` if the deployed graph still outputs center-width-height boxes
- `score_threshold`: default `0.35`
- `nms_threshold`: default `0.5`
- `nms_top_k`: default `300`
- `task_num`: default `4`
- `log_fps`: default `false`

## Model asset

- repo copy: `ros2_ws/src/rm_vehicle_detection/config/quant.bin`
- installed copy: `/opt/tros/lib/rm_vehicle_detection/config/quant.bin`

The parser assumes a standard YOLOv8 detect-style PTQ export with:

- three output scales
- DFL bbox regression
- one class by default
- one of two explicit raw box formats:
  - `xyxy`: quantized graph already emits `[x1, y1, x2, y2, score]`
  - `cxcywh`: graph still emits `[cx, cy, w, h, score]`

If the exported tensor order or head layout changes, update `src/parser.cpp`.

## Run

Direct package launch:

```bash
ros2 launch rm_vehicle_detection rm_vehicle_detection.launch.py
```

Override the output topic for bridge compatibility:

```bash
ros2 launch rm_vehicle_detection rm_vehicle_detection.launch.py \
  output_topic:=/dnn_node_sample
```

Whole-system launch through the bridge package:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py detector_type:=vehicle
```

## Notes

- `run_rm_det_loop.sh` keeps the package default model path when `VEHICLE_MODEL_PATH` is unset
- if you want bridge and visualizer to follow the vehicle detector through tmux scripts, keep `DETECTOR_TOPIC` consistent across the scripts

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
