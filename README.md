# TianAim

Tianbot gimbal auto-aim workspace for ROS2 perception, STM32 gimbal control, serial communication, and the future dataset/model workflow.

> Historical repository name: `gimbal_system`

## Overview

TianAim is organized around one verified runtime chain:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> STM32 gimbal control
```

`UART` is the current stable path. `USB CDC` is kept as a migration-validation path.

## Status

| Area | Active path | Notes |
| --- | --- | --- |
| 上位机 / upper-level runtime | `ros2_ws/` | ROS2 / TROS / RDK-X5 packages and scripts |
| 下位机 / lower-level firmware | `firmware/stm32_gimbal_control/` | STM32 gimbal control firmware |
| 模型训练 / data and models | `datasets/`, `models/`, `tools/` | Capture, labeling, training, evaluation workflow |
| 历史版本 / references | `archive/historical_code/` | Reference-only unless a task says otherwise |

## Quick Start

Build the upper-level ROS2 workspace:

```bash
bash scripts/build_ros2_mainline.sh
```

Build the STM32 firmware:

```bash
bash scripts/build_firmware_mainline.sh
```

Run the bridge node:

```bash
bash scripts/run_ros2_bridge.sh
```

The wrapper scripts source `scripts/tianaim_paths.sh` and support staged migration path overrides:

- `TIANAIM_ROS_WS`
- `TIANAIM_FIRMWARE_DIR`

## Repository Layout

```text
.
├── ros2_ws/                         # 上位机: ROS2 packages and RDK scripts
├── firmware/stm32_gimbal_control/   # 下位机: STM32 firmware
├── datasets/                        # Dataset structure and manifests
├── models/                          # Model metadata and export notes
├── tools/                           # Capture, labeling, training, evaluation tools
├── scripts/                         # Top-level build/run wrappers
├── docs/                            # Architecture, migration, backlog, audit notes
└── archive/
    ├── historical_code/             # Historical ROS2, board refs, and backups
    └── repo_audit_2026-04-11.md     # Archived migration audit
```

Important package and control entry points:

- camera package: `ros2_ws/src/hik_camera/`
- detector package: `ros2_ws/src/rm_armor_detection/`
- bridge package: `ros2_ws/src/rm_gimbal_bridge/`
- serial bridge: `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- vision input: `firmware/stm32_gimbal_control/Src/vision_input.c`
- target state: `firmware/stm32_gimbal_control/Src/target_state.c`
- gimbal control: `firmware/stm32_gimbal_control/Src/gimbal_task.c`

## Development Notes

- Put new upper-level runtime work in `ros2_ws/`.
- Put new firmware work in `firmware/stm32_gimbal_control/`.
- Put dataset/model workflow helpers in `tools/`, `datasets/`, or `models/`.
- Treat `archive/historical_code/` as the historical/reference code area.
- Avoid committing large raw datasets or large model weights without explicit approval.

## Documentation Map

Active entry points:

- Root overview: `README.md`
- Upper-level runtime: `ros2_ws/README.md`
- Lower-level firmware: `firmware/stm32_gimbal_control/README.md`
- Dataset workflow: `datasets/README.md`
- Model workflow: `models/README.md`
- Tooling workflow: `tools/README.md`
- Agent contract: `AGENTS.md`

Project notes:

- Architecture: `docs/architecture.md`
- Migration plan: `docs/migration_plan.md`
- Backlog: `docs/backlog.md`
- Archived audit: `archive/repo_audit_2026-04-11.md`

README files under `archive/historical_code/` and third-party snapshots are retained references, not daily reading entry points.
