# Repository Audit

Updated: 2026-03-27

## 1. Executive Summary

This repository already contains a usable full-system mainline, but it still behaves like an integration warehouse rather than a product repository.

The current verified chain is:

```text
hik_camera -> rm_armor_detection -> rm_gimbal_bridge -> UART -> Gimbal control
```

The main structural issues are:

- active directories still use transitional names such as `dev-branch/`
- the firmware mainline directory still uses a space-containing name: `Gimbal control/`
- data capture, labeling, training, and model storage were not previously first-class top-level areas
- historical and reference material is preserved in place, but not yet fully normalized under a product-facing archive strategy

## 2. Requested Audit Items

### 2.1 Current Root Directory Structure

Key top-level directories discovered:

- `dev-branch/`
- `Gimbal control/`
- `tianboard_s/`
- `tools/`
- `_git_migration_backup/`

Newly added transition directories in this update:

- `docs/`
- `ros2_ws/`
- `firmware/`
- `datasets/`
- `models/`
- `scripts/`
- `archive/`

### 2.2 README / AGENTS / Subdirectory README Status

Existing documentation before this change:

- root `README.md`
- root `AGENTS.md`
- `dev-branch/README.md`
- `dev-branch/scripts/README.md`
- `dev-branch/hik_camera/README.md`
- `dev-branch/rm_armor_detection/README.md`
- `dev-branch/rm_gimbal_bridge/README.md`
- `Gimbal control/README.md`
- `tianboard_s/README.md`
- `tools/README.md`

Assessment:

- documentation already reflected the current mainline reasonably well
- root docs were strong on current-state description, but still needed productization, AI-agent contract detail, dataset planning, and migration structure

### 2.3 Current ROS Workspace Organization

Current real ROS2 workspace role is carried by `dev-branch/`.

Observed package structure:

- `dev-branch/hik_camera`
- `dev-branch/rm_armor_detection`
- `dev-branch/rm_gimbal_bridge`
- `dev-branch/rm_interfaces`
- `dev-branch/rm_utils`

Observed generated workspace artifacts:

- `dev-branch/build/`
- `dev-branch/install/`
- `dev-branch/log/`

Assessment:

- functionally this behaves like a ROS2 workspace root
- structurally it should eventually migrate toward `ros2_ws/src/<package>`
- the current directory name `dev-branch/` is not product-grade and should be treated as transitional

### 2.4 Current Firmware Organization

Current real firmware mainline role is carried by `Gimbal control/`.

Observed structure:

- `Src/`
- `Inc/`
- `Chassis/`
- `IMU/`
- `algorithm/`
- `USB_DEVICE/`
- `Middlewares/`
- `Drivers/`
- `Makefile`

Assessment:

- this is the current source-of-truth firmware project
- the directory name contains a space and should eventually migrate to a stable name such as `firmware/stm32_gimbal_control/`
- migration should be staged because many existing scripts and human workflows still reference the current path

### 2.5 Communication Protocol / UART / USB CDC Locations

Upper-level serial bridge:

- `dev-branch/rm_gimbal_bridge/src/serial_bridge_node.cpp`

Upper-level USB CDC diagnostics:

- `dev-branch/rm_gimbal_bridge/src/usb_cdc_ping_test.cpp`
- `dev-branch/rm_gimbal_bridge/src/usb_cdc_pitch_test.cpp`
- `dev-branch/scripts/usb_cdc_pitch_control_test.py`

Lower-level UART vision ingress:

- `Gimbal control/Src/vision_input.c`
- `Gimbal control/Src/usart.c`

Lower-level USB CDC implementation:

- `Gimbal control/USB_DEVICE/App/usbd_cdc_if.c`
- `Gimbal control/Src/usb_cdc_test.c`
- `Gimbal control/Src/usb_cdc_test.h`

Current protocol fact:

- current bridge README documents an 8-byte UART frame:
  `0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD`

### 2.6 Actual Node / Package Locations

Camera:

- package: `dev-branch/hik_camera`
- launch: `dev-branch/hik_camera/launch/hik_camera.launch.py`
- node source: `dev-branch/hik_camera/src/hik_camera_node.cpp`

Detection:

- package: `dev-branch/rm_armor_detection`
- launch: `dev-branch/rm_armor_detection/launch/rm_armor_detection.launch.py`
- main inference node: `dev-branch/rm_armor_detection/src/sample.cpp`
- visualizer node: `dev-branch/rm_armor_detection/src/visualizer.cpp`

Bridge:

- package: `dev-branch/rm_gimbal_bridge`
- launch: `dev-branch/rm_gimbal_bridge/launch/rm_gimbal_bridge.launch.py`
- whole-system launch: `dev-branch/rm_gimbal_bridge/launch/rm_autoaim_system.launch.py`
- serial node: `dev-branch/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- feedback helper: `dev-branch/rm_gimbal_bridge/src/target_feedback_node.cpp`

Lower-level control:

- control logic: `Gimbal control/Src/gimbal_task.c`
- mode manager: `Gimbal control/Src/gimbal_mode_manager.c`
- target state: `Gimbal control/Src/target_state.c`

### 2.7 Existing Data / Training / Labeling / Model Directories

Before this update:

- no productized top-level `datasets/` directory
- no productized top-level `models/` directory
- `dev-branch/ultralytics-8.2.103/` existed as a retained third-party source snapshot
- `dev-branch/armor_detector/model/` existed as historical model-related content

Assessment:

- training and model-related material existed only in legacy or retained reference locations
- there was no current top-level structure for dataset collection and model lifecycle management

### 2.8 Historical / Backup / Temporary / Irregular Directories

Historical / reference:

- `tianboard_s/`
- `dev-branch/armor_detector/`
- `dev-branch/rm_camera_driver/`
- `dev-branch/rm_camera_driver_nv12/`
- `dev-branch/work_handover/`
- `dev-branch/ultralytics-8.2.103/`
- `_git_migration_backup/`

Generated / temporary:

- `dev-branch/build/`
- `dev-branch/install/`
- `dev-branch/log/`
- `Gimbal control/build/`
- `.git.BAK-*`

Irregular naming:

- `dev-branch/`
- `Gimbal control/`

### 2.9 Actual Build / Run / Test Entry Points

ROS2 build:

- `cd dev-branch && colcon build`

ROS2 run:

- `dev-branch/scripts/start_autoaim_tmux.sh`
- `dev-branch/scripts/start_rm_bridge_tmux.sh`

Firmware build:

- `make -C "Gimbal control"`

USB CDC validation:

- `dev-branch/scripts/usb_cdc_pitch_control_test.py`
- `dev-branch/rm_gimbal_bridge/src/usb_cdc_ping_test.cpp`
- `dev-branch/rm_gimbal_bridge/src/usb_cdc_pitch_test.cpp`

### 2.10 Naming Consistency

Current naming quality:

- ROS2 package names are generally stable and conventional
- top-level workspace naming is inconsistent with product naming
- firmware top-level directory naming is inconsistent because of embedded space
- generated and reference directories are mixed into active source areas

Conclusion:

- package naming is acceptable
- repository and top-level directory naming still need a staged migration plan

## 3. Classification

### 3.1 By Responsibility

Product mainline:

- `dev-branch/`
- `Gimbal control/`

Firmware control:

- `Gimbal control/`

Vision perception:

- `dev-branch/hik_camera/`
- `dev-branch/rm_armor_detection/`

Tracking and control:

- `Gimbal control/Src/gimbal_task.c`
- `Gimbal control/Src/target_state.c`
- `Gimbal control/Src/gimbal_mode_manager.c`

Communication bridge:

- `dev-branch/rm_gimbal_bridge/`
- `Gimbal control/Src/vision_input.c`
- `Gimbal control/USB_DEVICE/`

Data capture:

- `tools/capture/`
- `datasets/raw/`
- `datasets/manifests/`

Model training:

- `tools/training/`
- `models/`

Tooling:

- `tools/`
- `scripts/`

Historical archive:

- `tianboard_s/`
- `dev-branch/armor_detector/`
- `dev-branch/rm_camera_driver/`
- `dev-branch/rm_camera_driver_nv12/`
- `dev-branch/work_handover/`
- `dev-branch/ultralytics-8.2.103/`
- `_git_migration_backup/`

Documentation:

- `README.md`
- `AGENTS.md`
- `docs/`
- directory READMEs

### 3.2 By Maturity

Stable mainline:

- `dev-branch/hik_camera/`
- `dev-branch/rm_armor_detection/`
- `dev-branch/rm_gimbal_bridge/`
- `Gimbal control/` with UART path

In-development mainline:

- data workflow under `datasets/` and `tools/capture/`
- firmware controller extension points

Experimental modules:

- USB CDC tests
- future advanced controllers

Migration-stage modules:

- USB CDC path
- top-level transition anchors `ros2_ws/` and `firmware/`

Reference but not mainline:

- `tianboard_s/`
- `dev-branch/armor_detector/`
- `dev-branch/rm_camera_driver/`
- `dev-branch/rm_camera_driver_nv12/`
- `dev-branch/ultralytics-8.2.103/`

Archive candidates:

- build/install/log outputs
- `.git.BAK-*`

### 3.3 By Risk

Low risk:

- adding docs
- adding top-level transition directories
- adding dataset/model skeletons
- adding offline capture-tool skeletons

Medium risk:

- updating launch references
- renaming active directories
- modifying bridge defaults or topic names

High risk:

- changing serial protocol bytes
- switching UART mainline to USB CDC by default
- rewriting lower-level control loops without hardware validation

### 3.4 By Priority

P0:

- productized README and AGENTS
- repository audit and architecture docs
- dataset and capture skeleton
- explicit migration plan for `dev-branch/` and `Gimbal control/`

P1:

- staged move toward `ros2_ws/src/`
- staged move toward `firmware/stm32_gimbal_control/`
- Hik SDK capture integration
- session manifest standardization across collection runs

P2:

- labeling and split tooling
- training/evaluation automation
- controller enhancement with prediction stabilization

P3:

- experimental controller branches such as Kalman/LQR
- deeper archive normalization and legacy extraction

## 4. Recommended Restructure

Recommended target structure:

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

Recommended rollout:

1. keep the current physical runtime paths in place
2. add top-level transition directories and documentation
3. unify entry scripts and developer navigation
4. move source trees only when compatibility wrappers and updated references are ready

## 5. Minimum Manual Follow-Ups

- verify `colcon build` in an environment with TROS dependencies
- verify `make -C "Gimbal control"` in an environment with `arm-none-eabi-gcc`
- validate the future directory move with remote RDK deployment scripts before any physical rename
