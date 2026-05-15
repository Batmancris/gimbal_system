# Architecture

Updated: 2026-05-15

## Active Layout

```text
ros2_ws/                         # 上位机: ROS2 / TROS / RDK-X5
firmware/stm32_gimbal_control/   # 下位机: STM32 gimbal firmware
datasets/                        # dataset structure and manifests
models/                          # model metadata and exports
tools/                           # capture, labeling, training, evaluation helpers
scripts/                         # top-level wrapper commands
```

## Runtime Chain

当前主链路（bear-follow baseline）:

```text
hik_camera
  -> /hbmem_img
  -> rm_bear_detection
  -> /bear_detection/targets
  -> rm_gimbal_bridge
  -> USB-CDC serial device
  -> vision_input
  -> target_state
  -> gimbal_task
```

遥控链路:

```text
remote control
  -> DBUS / USART3 DMA
  -> remote_control
  -> gimbal mode / manual control
```

## Upper-Level Runtime

```text
ros2_ws/src/hik_camera/            [core] camera driver
ros2_ws/src/rm_bear_detection/     [core] bear YOLO detection (current mainline)
ros2_ws/src/rm_gimbal_bridge/      [core] gimbal bridge
ros2_ws/src/rm_interfaces/         [core] message definitions
ros2_ws/src/rm_utils/              [core] utility library
ros2_ws/src/rm_armor_detection/    [legacy/optional]
ros2_ws/src/rm_vehicle_detection/  [legacy/optional]
ros2_ws/scripts/                   startup/diagnostics/deployment scripts
```

Primary bridge source:

- `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`

## Lower-Level Firmware

```text
firmware/stm32_gimbal_control/
├── Src/
├── Inc/
├── Chassis/
├── IMU/
├── algorithm/
├── USB_DEVICE/
├── Drivers/
├── Middlewares/
└── Makefile
```

Primary control sources:

- `firmware/stm32_gimbal_control/Chassis/remote_control.c`
- `firmware/stm32_gimbal_control/USB_DEVICE/App/usbd_cdc_if.c`
- `firmware/stm32_gimbal_control/Src/vision_input.c`
- `firmware/stm32_gimbal_control/Src/target_state.c`
- `firmware/stm32_gimbal_control/Src/gimbal_task.c`

## Data And Model Workflow

```text
datasets/raw/
datasets/labeled/
datasets/splits/
datasets/manifests/
models/
tools/capture/
tools/labeling/
tools/training/
tools/evaluation/
```

Keep small metadata, configs, and reports in Git. Do not commit large raw datasets or model weights without explicit approval.

## Migration Rule

Keep upper-level changes in `ros2_ws/`, firmware changes in `firmware/stm32_gimbal_control/`, and tooling/model workflow changes under `tools/`, `datasets/`, or `models/`.
