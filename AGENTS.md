# AGENTS Contract

This file is the executable development contract for AI coding agents and human contributors working in this repository.

## 1. Repository Identity And Scope

- Historical repository name: `gimbal_system`
- Preferred future product name: `TianAim`
- Company context: Tianbot
- Repository role: integrated product workspace for upper-level perception, lower-level firmware, communication, tooling, and future dataset/model assets

This repository contains both active mainline code and retained historical references. Agents must distinguish between the two before making changes.

## 2. Source Of Truth

When information conflicts, use this order:

1. current source files, launch files, Makefiles, scripts, and package metadata
2. the nearest `README.md`
3. `docs/` architecture and audit documents
4. historical handover or backup materials

Do not treat old split-repository notes, backups, or legacy README variants as the primary source of truth.

## 3. Current Mainline Directories

Current physical mainline paths:

- ROS2 upper-level mainline: `dev-branch/`
- STM32 firmware mainline: `Gimbal control/`

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

- `dev-branch/` is still the real ROS2 source tree today
- `Gimbal control/` is still the real firmware source tree today
- do not assume the migration to `ros2_ws/` or `firmware/` has already happened

## 4. Directory Ownership

`dev-branch/`

- owns the current ROS2 / TROS / RDK-X5 runtime mainline
- includes `hik_camera`, `rm_armor_detection`, and `rm_gimbal_bridge`
- default place for upper-level code changes

`Gimbal control/`

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

- owns architecture, audit, roadmap, and migration records
- every structural change should update relevant docs

`tianboard_s/`

- reference-only board-level code
- modify only for reference extraction, comparison, or archival clarification

`_git_migration_backup/`

- backup-only
- do not treat as active code

## 5. Communication And Interface Contracts

Current verified whole-system path:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

Upper-level interfaces:

- camera publishes `image_raw` and `/hbmem_img`
- detector consumes `/hbmem_img`
- detector publishes `/dnn_node_sample`
- bridge consumes `ai_msgs/msg/PerceptionTargets`

Current bridge protocol:

- location: `dev-branch/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- current frame format: `0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD`

Lower-level ingest path:

- UART vision input: `Gimbal control/Src/vision_input.c`
- target state update: `Gimbal control/Src/target_state.c`
- control application: `Gimbal control/Src/gimbal_task.c`

USB CDC rule:

- USB CDC is a migration-validation path
- do not remove or bypass UART mainline without explicit system-level validation

## 6. Build, Run, And Test Commands

Top-level convenience entry points:

- ROS2 build: `bash scripts/build_ros2_mainline.sh`
- firmware build: `bash scripts/build_firmware_mainline.sh`
- bridge run: `bash scripts/run_ros2_bridge.sh`

Direct current mainline commands:

- ROS2 build:
  ```bash
  cd dev-branch
  source /opt/tros/humble/setup.bash
  colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
  ```
- ROS2 board-side startup:
  ```bash
  bash dev-branch/scripts/start_autoaim_tmux.sh
  ```
- bridge startup:
  ```bash
  bash dev-branch/scripts/start_rm_bridge_tmux.sh
  ```
- firmware build:
  ```bash
  make -C "Gimbal control"
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

- `dev-branch/scripts/`
- ROS2 launch/config files
- `dev-branch/rm_gimbal_bridge/`
- `Gimbal control/Src/gimbal_task.*`
- `Gimbal control/Src/vision_input.*`
- package names, topics, protocol bytes, serial defaults

Do not casually rewrite:

- protocol framing already validated with hardware
- UART defaults that are still the stable chain
- historical directories that may still be referenced for recovery

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
- `docs/repo_audit.md`
- `docs/architecture.md`

## 11. Minimum Verification After Changes

After code or script changes, agents should perform the minimum practical verification:

- `python3 -m py_compile` for changed Python tools/scripts
- `bash -n` for changed shell scripts
- `colcon build --packages-select ...` for affected ROS2 packages when dependencies are available
- `make -C "Gimbal control"` for firmware changes when the ARM toolchain is available

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
