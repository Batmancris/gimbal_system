# Architecture

Updated: 2026-04-26

This document records the current main repository architecture.

## Repository Scope

```text
gimbal_system/
├── README.md
├── AGENTS.md
├── ros2_ws/
├── firmware/
├── scripts/
├── models/
├── datasets/
├── docs/
├── assets/
└── archive/
```

训练/量化/采集工具已拆分为独立工具仓，本仓库仅保留云台主系统代码。

## Runtime Chain

```text
hik_camera
  -> /hbmem_img
  -> detection
  -> rm_gimbal_bridge
  -> STM32 USB-CDC
  -> vision_input
  -> target_state
  -> gimbal_task
  -> CAN
  -> GM6020 gimbal
```

Remote-control input remains on the lower-level firmware side:

```text
remote control
  -> DBUS / USART3 DMA
  -> remote_control
  -> gimbal mode / manual control
```

## Upper-Level Runtime

Current ROS2/TROS runtime code lives under `ros2_ws/`.

Key packages:

- `ros2_ws/src/hik_camera/`
- `ros2_ws/src/rm_bear_detection/`
- `ros2_ws/src/rm_vehicle_detection/`
- `ros2_ws/src/rm_armor_detection/`
- `ros2_ws/src/rm_gimbal_bridge/`
- `ros2_ws/src/rm_interfaces/`
- `ros2_ws/src/rm_utils/`

Primary bridge implementation:

- `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`

## Lower-Level Firmware

Current STM32 firmware lives under `firmware/stm32_gimbal_control/`.

Primary paths:

- `firmware/stm32_gimbal_control/Chassis/remote_control.c`
- `firmware/stm32_gimbal_control/USB_DEVICE/App/usbd_cdc_if.c`
- `firmware/stm32_gimbal_control/Src/vision_input.c`
- `firmware/stm32_gimbal_control/Src/target_state.c`
- `firmware/stm32_gimbal_control/Src/gimbal_task.c`

## Data And Model Policy

- `models/` keeps model metadata, reports, and current runtime configuration notes.
- `datasets/` keeps skeleton directories, manifests, and examples.
- Large datasets, training weights, exported ONNX files, and quantization outputs should stay outside this main repository unless a later release decision explicitly changes that policy.

## Maintenance Boundaries

Keep runtime changes scoped to their owning area:

- ROS2 upper-level code: `ros2_ws/`
- STM32 firmware: `firmware/stm32_gimbal_control/`
- top-level user entry scripts: `scripts/`
- repository governance, maintenance notes, and release preparation: `docs/`

Documentation-only cleanup should not mix algorithm, parameter, inference, firmware, launch, YAML, shell, C++, Python, or header changes into the same change set.
