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
