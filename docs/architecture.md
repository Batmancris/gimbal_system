# Architecture And Migration Notes

## 1. Naming Direction

Preferred product naming:

- product name: `TianAim`
- future tool/module prefix: `tianaim_*`

This update does not rename active runtime packages. It prepares the repository so a future staged rename is possible without losing current operability.

## 2. Current Architecture

### Upper-Level Runtime

```text
dev-branch/
├── hik_camera
├── rm_armor_detection
├── rm_gimbal_bridge
├── rm_interfaces
├── rm_utils
└── scripts
```

### Lower-Level Runtime

```text
Gimbal control/
├── Src
├── Inc
├── Chassis
├── IMU
├── algorithm
├── USB_DEVICE
├── Drivers
├── Middlewares
└── Makefile
```

### Runtime Data Flow

```text
hik_camera
  -> image_raw / hbmem_img
  -> rm_armor_detection
  -> /dnn_node_sample
  -> rm_gimbal_bridge
  -> UART or USB CDC validation path
  -> vision_input
  -> target_state
  -> gimbal_task
```

## 3. Target Product Structure

Recommended target layout:

```text
/
├── README.md
├── AGENTS.md
├── docs/
├── firmware/
│   └── stm32_gimbal_control/
├── ros2_ws/
│   └── src/
├── datasets/
├── models/
├── tools/
├── scripts/
└── archive/
```

## 4. Why The Source Trees Were Not Moved Yet

`dev-branch/` was not physically moved because:

- existing deployment scripts still assume `dev-branch/`
- current `colcon` and remote bring-up flow already work there

`Gimbal control/` was not physically moved because:

- its Makefile and related local workflows already reference the current path
- directory rename risk is high for local IDE metadata and external references

This means the current migration approach is:

- add transition anchors first
- update docs and top-level entry scripts
- move real code later in reviewable steps

## 5. Controller Evolution Plan

Mainline recommendation:

1. keep current `P/PI`-compatible control path as the validated mainline
2. add prediction stabilization before introducing more aggressive control changes
3. treat `Kalman`, `alpha-beta`, and `LQR` as experiment lines until hardware-validated

Code entry points:

- current control application: `Gimbal control/Src/gimbal_task.c`
- current tuning macros: `Gimbal control/Src/gimbal_task.h`
- current target ingest: `Gimbal control/Src/target_state.c`

Implementation principle:

- expose explicit controller extension points
- do not rewrite the working control loop without hardware verification

## 6. Dataset Workflow Design

Recommended dataset lifecycle:

```text
camera capture
  -> datasets/raw/session_x
  -> session manifest
  -> labeling export
  -> datasets/labeled/
  -> datasets/splits/
  -> training config / model artifact
  -> evaluation report
```

Suggested image naming:

```text
<session_id>_<camera_id>_<timestamp_ns>_<frame_index>.jpg
```

Suggested session manifest fields:

- `session_id`
- `project`
- `camera.vendor`
- `camera.model`
- `capture_mode`
- `image_format`
- `environment`
- `operator`
- `labels_expected`
- `frames[]`

## 7. Archive Strategy

Recommended archive scope:

- `tianboard_s/`
- `dev-branch/armor_detector/`
- `dev-branch/rm_camera_driver/`
- `dev-branch/rm_camera_driver_nv12/`
- `dev-branch/work_handover/`
- `dev-branch/ultralytics-8.2.103/`
- `_git_migration_backup/`

These should be documented as references first, then physically moved only when all references are updated.
