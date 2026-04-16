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
