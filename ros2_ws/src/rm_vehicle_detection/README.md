# rm_vehicle_detection

`rm_vehicle_detection` is the vehicle-detector integration package for the existing
`hik_camera -> detector -> rm_gimbal_bridge -> STM32` pipeline.

The bridge contract remains unchanged:

- input: board-side shared-memory image topic
- output: `ai_msgs/msg/PerceptionTargets`
- bridge relies on `type`, `rois[0].confidence`, and ROI center

## Current status

This package now follows the same D-Robotics TROS pattern as `rm_armor_detection`:

- subscribe to `/hbmem_img`
- load a board-side quantized `quant.bin`
- run `dnn_node` inference on the RDK-X5
- parse YOLOv8 single-class detections
- publish `PerceptionTargets`

## Parser assumption

The current parser assumes your `quant.bin` is a standard YOLOv8 detect-style PTQ
export with:

- three output scales
- DFL bbox regression (`reg=16`)
- one class: `vehicle`

If the exported tensor order or head layout differs, the parser indexes in
`src/parser.cpp` will need a small follow-up adjustment on the board.

## Parameters

- `image_topic`: input shared-memory image topic, default `/hbmem_img`
- `output_topic`: output topic, default `/vehicle_detection/targets`
- `target_type`: published semantic label, default `vehicle`
- `model_path`: board-side quantized model path
- `score_threshold`: pre-NMS confidence threshold
- `nms_threshold`: NMS IoU threshold
- `nms_top_k`: max boxes kept by NMS
- `task_num`: DNN task queue size
- `log_fps`: whether to print runtime FPS logs

## Launch example

```bash
ros2 launch rm_vehicle_detection rm_vehicle_detection.launch.py \
  model_path:=/opt/tros/lib/rm_vehicle_detection/config/quant.bin
```

## Integration notes

- Recommended bridge input for vehicle mode: `/vehicle_detection/targets`
- Recommended bridge `allowed_target_types`: `["vehicle", "car", "KT"]`
- Recommended detector switch entry: `detector_type:=vehicle`
- Repository model asset:
  `ros2_ws/src/rm_vehicle_detection/config/quant.bin`
- Installed board-side default:
  `/opt/tros/lib/rm_vehicle_detection/config/quant.bin`
