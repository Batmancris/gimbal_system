# ros2_ws

This workspace contains the active RDK-X5 ROS2/TROS runtime for camera capture, detector inference, target bridging, and board-side helper scripts.

## Packages

```text
src/
|- hik_camera
|- rm_armor_detection
|- rm_vehicle_detection
|- rm_gimbal_bridge
|- rm_interfaces
\- rm_utils
```

## Current runtime chains

Armor mode:

```text
hik_camera -> /hbmem_img -> rm_armor_detection -> /dnn_node_sample -> rm_gimbal_bridge
```

Vehicle mode, package default:

```text
hik_camera -> /hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets -> rm_gimbal_bridge
```

Vehicle mode, shared downstream topic:

```text
hik_camera -> /hbmem_img -> rm_vehicle_detection -> /dnn_node_sample -> rm_gimbal_bridge
```

## Build

On the board:

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_vehicle_detection rm_gimbal_bridge --event-handlers console_direct+
source install/setup.bash
```

From the repository root:

```bash
bash scripts/build_ros2_mainline.sh
```

## Direct run

Camera:

```bash
ros2 launch hik_camera hik_camera.launch.py
```

Armor detector:

```bash
ros2 run rm_armor_detection rm_armor_detection
```

Vehicle detector:

```bash
ros2 launch rm_vehicle_detection rm_vehicle_detection.launch.py
```

Vehicle detector using the shared downstream topic:

```bash
ros2 launch rm_vehicle_detection rm_vehicle_detection.launch.py \
  output_topic:=/dnn_node_sample
```

Bridge:

```bash
ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args \
  -p input_topic:=/dnn_node_sample
```

Whole system:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py
```

Whole system in vehicle mode:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py \
  detector_type:=vehicle \
  detector_topic:=/dnn_node_sample
```

## tmux scripts

Board-side helper scripts live in `ros2_ws/scripts/`.

Common entries:

- `start_autoaim_tmux.sh`
- `start_rm_bridge_tmux.sh`
- `check_autoaim_topics.sh`
- `run_rm_det_loop.sh`
- `run_rm_bridge_loop.sh`
- `run_rm_vis_loop.sh`

Armor mode:

```bash
bash src/scripts/start_autoaim_tmux.sh
```

Vehicle mode:

```bash
export DETECTOR_TYPE=vehicle
export DETECTOR_TOPIC=/dnn_node_sample
bash src/scripts/start_autoaim_tmux.sh
```

Optional model override:

```bash
export VEHICLE_MODEL_PATH=/opt/tros/lib/rm_vehicle_detection/config/quant.bin
```

Notes:

- if `VEHICLE_MODEL_PATH` is unset, `run_rm_det_loop.sh` keeps the package default model path
- `check_autoaim_topics.sh` follows `DETECTOR_TOPIC`
- `start_autoaim_tmux.sh` clears both armor and vehicle detector processes before relaunch

## Topics

Core topics:

- `/image_raw`: normal ROS image for visualization
- `/hbmem_img`: TROS shared-memory image topic
- `/dnn_node_sample`: shared detector output topic used by armor mode and optional vehicle mode
- `/vehicle_detection/targets`: package-default vehicle detector output

## Troubleshooting

No `/hbmem_img` publisher:

```bash
ros2 topic info /hbmem_img -v
tmux -L autoaim capture-pane -pt hik_cam
```

No detector publisher:

```bash
export DETECTOR_TOPIC=/dnn_node_sample
ros2 topic info "${DETECTOR_TOPIC}" -v
tmux -L autoaim capture-pane -pt rm_det
```

Bridge gets no target:

```bash
ros2 topic echo -n 5 /dnn_node_sample
tmux -L bridge capture-pane -pt rm_bridge
```

If you keep vehicle mode on `/vehicle_detection/targets`, point the bridge `input_topic` and your checks to the same topic.

## Related docs

- [src/rm_armor_detection/README.md](src/rm_armor_detection/README.md)
- [src/rm_vehicle_detection/README.md](src/rm_vehicle_detection/README.md)
- [src/rm_gimbal_bridge/README.md](src/rm_gimbal_bridge/README.md)

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
