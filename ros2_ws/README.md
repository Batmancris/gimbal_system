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
