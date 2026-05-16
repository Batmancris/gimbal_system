# AGENTS Contract

This file is the executable development contract for AI coding agents and human contributors working in this repository.

## 1. Repository Identity And Scope

- Historical repository name: `gimbal_system`
- Preferred future product name: `TianAim`
- Company context: Tianbot
- Repository role: integrated product workspace for upper-level perception, lower-level firmware, communication, tooling, and future dataset/model assets

This repository contains the active mainline code after the historical-code snapshot was removed. Use Git history or the remote historical `main` branch if old code must be inspected.

## 2. Source Of Truth

When information conflicts, use this order:

1. current source files, launch files, Makefiles, scripts, and package metadata
2. the nearest `README.md`
3. `docs/` architecture, migration, and backlog documents
4. historical handover or backup materials from Git history or the remote historical `main` branch

Do not treat old split-repository notes, backups, or legacy README variants as the primary source of truth.

## 3. Current Mainline Directories

Current physical mainline paths:

- ROS2 upper-level mainline: `ros2_ws/`
- STM32 firmware mainline: `firmware/stm32_gimbal_control/`

Current product-oriented transition anchors:

- `ros2_ws/`
- `firmware/`
- `datasets/`
- `models/`
- `tools/`
- `scripts/`
- `docs/`
- `archive/`

Important:

- `ros2_ws/` is the real ROS2 source tree today
- `firmware/stm32_gimbal_control/` is the real firmware source tree today
- the ROS2 mainline migration to `ros2_ws/` has happened
- the firmware mainline migration into `firmware/` has happened
- top-level scripts use `scripts/tianaim_paths.sh` for staged path compatibility
- `TIANAIM_ROS_WS` may override the local ROS2 workspace path
- `TIANAIM_FIRMWARE_DIR` may override the local firmware project path
- RDK scripts may override `REMOTE_SRC_DIR` and `REMOTE_SCRIPT_DIR`, but default to the current `/home/sunrise/rm_ws/src` layout

## 4. Directory Ownership

`ros2_ws/`

- owns the current ROS2 / TROS / RDK-X5 runtime mainline
- includes `src/hik_camera`, `src/rm_armor_detection`, and `src/rm_gimbal_bridge`
- owns current RDK runtime scripts under `scripts/`
- default place for upper-level code changes

`firmware/stm32_gimbal_control/`

- owns the current STM32 lower-level gimbal control firmware
- default place for lower-level control and communication-path changes

`tools/`

- owns small helper tools, diagnostics, and offline data workflows
- safe place for new capture/labeling/training/evaluation helpers

`datasets/`

- owns dataset structure, manifests, and split metadata
- do not commit large raw datasets unless explicitly intended

`models/`

- owns model metadata, export conventions, and lightweight placeholders
- avoid committing large binary weights without approval

`docs/`

- owns architecture, roadmap, and migration records
- every structural change should update relevant docs

`archive/`

- owns audit notes and recovery notes only
- do not add runtime code, historical snapshots, or backup repositories here

## 5. Communication And Interface Contracts

Current verified whole-system path:

```text
remote control
  -> DBUS / USART3 DMA
  -> STM32 remote_control
  -> gimbal mode / manual control

hik_camera
  -> /hbmem_img
  -> rm_bear_detection
  -> /bear_detection/targets
  -> rm_gimbal_bridge
  -> STM32 USB-CDC
  -> GM6020
```

Upper-level interfaces:

- camera publishes `image_raw` and `/hbmem_img`
- detector consumes `/hbmem_img`
- detector publishes `/bear_detection/targets`
- bridge consumes `ai_msgs/msg/PerceptionTargets`

Current bridge protocol:

- location: `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- current frame format: `0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD`

Lower-level ingest path:

- remote control input: `firmware/stm32_gimbal_control/Chassis/remote_control.c`
- USB-CDC vision input: `firmware/stm32_gimbal_control/USB_DEVICE/App/usbd_cdc_if.c`
- shared vision parser: `firmware/stm32_gimbal_control/Src/vision_input.c`
- target state update: `firmware/stm32_gimbal_control/Src/target_state.c`
- control application: `firmware/stm32_gimbal_control/Src/gimbal_task.c`

Communication rule:

- DBUS is the current remote-control path into the lower-level firmware
- USB-CDC is the current upper-to-lower vision path in active scripts and firmware hooks
- keep UART-compatible parser/framing paths unless explicit system-level validation says they can be removed

## 6. Build, Run, And Test Commands

Top-level convenience entry points:

- ROS2 build: `bash scripts/build_ros2_mainline.sh`
- firmware build: `bash scripts/build_firmware_mainline.sh`
- bridge run: `bash scripts/run_ros2_bridge.sh`

These wrappers should be preferred during migration because they preserve compatibility with both the current physical paths and the future product-oriented paths.

Direct current mainline commands:

- ROS2 build:
  ```bash
  cd ros2_ws
  source /opt/tros/humble/setup.bash
  colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
  ```
- ROS2 board-side startup (唯一推荐入口):
  ```bash
  bash ros2_ws/scripts/start_fast_follow_verified.sh
  ```
- firmware build:
  ```bash
  make -C "firmware/stm32_gimbal_control"
  ```

If a change touches:

- only docs: validate links and path references
- Python tools: run `python3 -m py_compile ...`
- ROS2 launch/scripts: run shell syntax checks where practical
- firmware control logic: at minimum compile if toolchain is available

## 7. Safe Vs. Cautious Edit Zones

Usually safe to modify directly:

- `docs/`
- `tools/`
- top-level `scripts/`
- new files under `datasets/` and `models/` that are metadata-oriented
- README and AGENTS documentation

Modify carefully and keep scope small:

- `ros2_ws/scripts/`
- ROS2 launch/config files
- `ros2_ws/src/rm_gimbal_bridge/`
- `firmware/stm32_gimbal_control/Src/gimbal_task.*`
- `firmware/stm32_gimbal_control/Src/vision_input.*`
- package names, topics, protocol bytes, serial defaults

Do not casually rewrite:

- protocol framing already validated with hardware
- DBUS remote-control defaults and USB-CDC vision defaults that are still the stable chain
- local historical-code snapshots; use Git history or the remote historical `main` branch instead

## 8. Naming Rules

- new directories should be lowercase, stable, and contain no spaces
- new Python modules and scripts should follow PEP 8
- new product-facing names should align with `TianAim` / `tianaim_*`
- do not perform wide renames of active runtime packages unless the migration plan and references are updated together

## 9. Data And Model Directory Contract

Dataset structure:

- `datasets/raw/`
- `datasets/labeled/`
- `datasets/splits/`
- `datasets/manifests/`

Tool structure:

- `tools/capture/`
- `tools/labeling/`
- `tools/training/`
- `tools/evaluation/`

Model structure:

- `models/README.md` documents storage and export expectations

Manifest expectations:

- every capture session should emit a manifest
- file naming should be timestamp-based and session-stable
- calibration, environment, and camera metadata should be recorded whenever available

## 10. Documentation Sync Requirement

Any change to one of these areas must update the nearest relevant docs in the same turn when practical:

- directory structure
- build/run commands
- protocol entry points
- dataset conventions
- naming or ownership expectations

At minimum, keep these in sync:

- `README.md`
- `AGENTS.md`
- `docs/architecture.md`
- `docs/migration_plan.md` when migration state changes
- `docs/backlog.md` when planned work changes

Historical audit records belong under `archive/`, for example `archive/repo_audit_2026-04-11.md`; do not treat archived audit files as daily documentation entry points.

## 11. Minimum Verification After Changes

After code or script changes, agents should perform the minimum practical verification:

- `python3 -m py_compile` for changed Python tools/scripts
- `bash -n` for changed shell scripts
- `colcon build --packages-select ...` for affected ROS2 packages when dependencies are available
- `make -C "firmware/stm32_gimbal_control"` for firmware changes when the ARM toolchain is available

If verification cannot be completed, state exactly what blocked it.

## 12. Migration Guidance

Preferred future structure:

```text
/
├── docs/
├── firmware/
├── ros2_ws/
├── datasets/
├── models/
├── tools/
├── scripts/
└── archive/
```

Migration principle:

- add transition layers first
- move active code only in reviewable, compatibility-preserving steps
- update README, scripts, and launch references together
- do not break the verified runtime chain for a cosmetic rename
