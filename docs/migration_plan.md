# Migration Plan

Updated: 2026-04-11

## Current Baseline

The current runtime source trees are:

- ROS2 / TROS upper-level runtime: `ros2_ws/`
- STM32 firmware runtime: `firmware/stm32_gimbal_control/`

The next product-facing target anchor is still:

- `firmware/`

Path-compatible wrapper scripts are now in place so the repository can move in stages without breaking the current board workflow.

## Compatibility Layer

Top-level scripts source `scripts/tianaim_paths.sh`.

Default path resolution:

- ROS2 resolves to `ros2_ws/` now that packages exist under `ros2_ws/src/`; historical ROS2 code is archived under `archive/historical_code/dev-branch/`.
- Firmware resolves to `firmware/stm32_gimbal_control/` now that its `Makefile` exists there.

Override variables:

- `TIANAIM_ROS_WS`
- `TIANAIM_FIRMWARE_DIR`

RDK-side deployment and startup scripts now support:

- `REMOTE_WS`, defaulting to `/home/sunrise/rm_ws`
- `REMOTE_SRC_DIR`, defaulting to `/home/sunrise/rm_ws/src`
- `REMOTE_SCRIPT_DIR`, defaulting to `/home/sunrise/rm_ws/src/scripts`

The defaults intentionally preserve the current board-side layout.

## Phase 0: Baseline Verification

Before moving directories:

```bash
bash -n scripts/*.sh ros2_ws/scripts/*.sh
bash scripts/build_ros2_mainline.sh
bash scripts/build_firmware_mainline.sh
```

If the RDK-X5 board is available, also verify:

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 topic list
```

## Phase 1: ROS2 Migration

Status: completed for current mainline packages and active RDK scripts.

Moved current mainline ROS2 packages:

```text
dev-branch/hik_camera          -> ros2_ws/src/hik_camera
dev-branch/rm_armor_detection  -> ros2_ws/src/rm_armor_detection
dev-branch/rm_gimbal_bridge    -> ros2_ws/src/rm_gimbal_bridge
dev-branch/rm_interfaces       -> ros2_ws/src/rm_interfaces
dev-branch/rm_utils            -> ros2_ws/src/rm_utils
```

Moved active RDK scripts:

```text
dev-branch/scripts -> ros2_ws/scripts
```

Historical/reference ROS2 directories are now archived under `archive/historical_code/dev-branch/`:

- `archive/historical_code/dev-branch/armor_detector/`
- `archive/historical_code/dev-branch/rm_camera_driver/`
- `archive/historical_code/dev-branch/rm_camera_driver_nv12/`
- `archive/historical_code/dev-branch/work_handover/`
- `archive/historical_code/dev-branch/ultralytics-8.2.103/`

After this move, validate:

```bash
bash scripts/build_ros2_mainline.sh
bash -n ros2_ws/scripts/*.sh
```

RDK deployment can still target the old remote workspace layout by leaving defaults unchanged:

```bash
REMOTE_WS=/home/sunrise/rm_ws bash ros2_ws/scripts/deploy_to_rdk_x5.sh
```

## Phase 2: Firmware Migration

Status: completed for the STM32 firmware mainline.

Moved firmware mainline:

```text
Gimbal control/ -> firmware/stm32_gimbal_control/
```

Then validate:

```bash
bash scripts/build_firmware_mainline.sh
```

Keep UART as the formal stable path during this phase. Do not promote USB CDC to the default path as part of a directory rename.

## Phase 3: Archive Normalization

Status: completed by moving retained references under `archive/historical_code/`.

Archived paths:

- `archive/historical_code/tianboard_s/`
- `archive/historical_code/dev-branch/`
- `archive/historical_code/_git_migration_backup/`
- `archive/historical_code/stm32_gimbal_control.git.BAK-20260319/`

Do not add new runtime work under these archived paths.
