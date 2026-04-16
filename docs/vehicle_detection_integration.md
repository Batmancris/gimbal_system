# Vehicle Detection Integration

This document turns the vehicle-following migration idea into a concrete phase-1
repository plan.

## Goal

Safely integrate a new vehicle detector into the existing upper-level tracking
chain without changing the STM32 protocol or breaking the current gimbal loop.

Recommended direction:

`hik_camera -> rm_vehicle_detection -> PerceptionTargets -> rm_gimbal_bridge -> STM32`

## Phase 1 scope

- add a new `rm_vehicle_detection` package
- keep the bridge input message as `ai_msgs/msg/PerceptionTargets`
- add launch-level detector switching
- keep STM32 protocol unchanged
- keep `rm_gimbal_bridge` changes small and parameter-driven

## Repository changes

- `ros2_ws/src/rm_vehicle_detection/`
- `docs/vehicle_detection_integration.md`
- `rm_autoaim_system.launch.py`
- `rm_gimbal_bridge.yaml`

## Launch contract

New `rm_autoaim_system.launch.py` arguments:

- `detector_type:=armor|vehicle`
- `detector_topic:=/dnn_node_sample|/vehicle_detection/targets`

Recommended examples:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py detector_type:=armor detector_topic:=/dnn_node_sample
```

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py detector_type:=vehicle detector_topic:=/vehicle_detection/targets
```

## Bridge contract

`rm_gimbal_bridge` continues to depend on:

- `targets[i].type`
- `targets[i].rois[0].confidence`
- ROI center

New bridge parameter:

- `allowed_target_types`: exact-match allowlist for vehicle mode or mixed detector experiments

Supported selection modes after this change:

- `closest`
- `highest_confidence`
- `largest_box`

## Vehicle detector phase-1 parameters

- `image_topic`
- `output_topic`
- `target_type`
- `backend`
- `model_path`
- `publish_empty_detections`

## Model direction

This project should use board-side inference on the RDK-X5 rather than a local PC
runtime. The currently discovered quantized model is:

`E:\research\1\yolo\hiki training\model_training\runs\vehicle_yolov8x_4090\X系列量化任务-68455350_all_results\quant.bin`

## Next implementation step

Replace the `stub` backend in `rm_vehicle_detection` with an RDK-X5 quantized-model
runner that:

1. subscribes to `/image_raw`
2. loads `quant.bin`
3. runs board-side inference with the vehicle model
4. applies confidence threshold and NMS
5. writes detections into `PerceptionTargets`
6. publishes to the configured `output_topic`
