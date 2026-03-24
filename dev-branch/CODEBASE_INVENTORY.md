# Codebase Inventory

Updated: 2026-03-21

## Current Working Pipeline

The current verified RDK-X5 auto-aim pipeline is:

- `hik_camera`
- `rm_armor_detection`
- `rm_gimbal_bridge`
- `scripts/start_autoaim_tmux.sh`

The new checkpoint PR for the current progress is:

- <https://github.com/Batmancris/gimbal_system/pull/3>

## Local Workspace

### Keep

These are part of the current project scope or active delivery path.

- `../Gimbal control/`
  - STM32 lower-machine code.
- `../tianboard_s/`
  - board-side embedded support code.
- `./hik_camera/`
  - Hikrobot camera ROS2 package used on RDK-X5.
- `./rm_armor_detection/`
  - current armor detection package, including the new visualizer.
- `./rm_gimbal_bridge/`
  - bridge from ROS detections to STM32 serial protocol.
- `./scripts/`
  - deployment, startup, topic check, and autostart scripts used on RDK-X5.
- `./README.md`
- `./doc/`
  - keep if you still use the docs.
- `./work_handover/`
  - keep if this handover material is still useful for the team.

### Optional Reference Only

These are not part of the currently verified running chain, but may still be useful as references.

- `./armor_detector/`
  - older ROS auto-aim pipeline with visualization-related code.
- `./rm_camera_driver/`
  - older camera driver path, not used by the current Hik camera launch flow.
- `./rm_camera_driver_nv12/`
  - alternate older camera driver path, not used by the current Hik camera launch flow.
- `./rm_interfaces/`
  - mainly needed by the older `armor_detector` path.
- `./rm_utils/`
  - utility package mainly used by older packages.
- `./ultralytics-8.2.103/`
  - training/tooling side content, not needed for on-board runtime.

### Can Be Cleaned Locally

These are not source-of-truth code and can usually be removed safely after confirmation.

- `./build/`
  - local colcon build artifacts.
- `./install/`
  - local colcon install artifacts.
- `./log/`
  - local colcon logs.
- `../Gimbal control/build/`
  - STM32 local build output.
- `./.vscode/`
  - personal IDE settings.
- `../.vscode/`
  - personal IDE settings.
- `./.git.BAK-20260319/`
  - migration backup, not active code.
- `../Gimbal control/.git.BAK-20260319/`
  - migration backup, not active code.
- `../tianboard_s/.git.BAK-20260319/`
  - migration backup, not active code.
- `../_git_migration_backup/`
  - migration backup archive.

## RDK-X5 Workspace

Remote workspace checked on `192.168.127.10`:

- `/home/sunrise/rm_ws/src/`
- `/home/sunrise/rm_ws/build/`
- `/home/sunrise/rm_ws/install/`
- `/home/sunrise/rm_ws/log/`
- `/home/sunrise/rm_ws/MvSdkLog/`

### Keep On RDK-X5

Needed to run the current system.

- `/home/sunrise/rm_ws/src/hik_camera/`
- `/home/sunrise/rm_ws/src/rm_armor_detection/`
- `/home/sunrise/rm_ws/src/rm_gimbal_bridge/`
- `/home/sunrise/rm_ws/src/scripts/`
- `/home/sunrise/rm_ws/install/`
  - needed for direct runtime after build.

### Optional Reference Only On RDK-X5

Not required for the current verified runtime path.

- `/home/sunrise/rm_ws/src/armor_detector/`
- `/home/sunrise/rm_ws/src/rm_camera_driver/`
- `/home/sunrise/rm_ws/src/rm_camera_driver_nv12/`
- `/home/sunrise/rm_ws/src/rm_interfaces/`
- `/home/sunrise/rm_ws/src/rm_utils/`
- `/home/sunrise/rm_ws/src/doc/`
- `/home/sunrise/rm_ws/src/work_handover/`
- `/home/sunrise/rm_ws/src/ultralytics-8.2.103/`
- `/home/sunrise/rm_ws/src/.vscode/`
- `/home/sunrise/rm_ws/src/.git.BAK-20260319/`

### Can Be Cleaned On RDK-X5

These are generated or temporary runtime artifacts.

- `/home/sunrise/rm_ws/build/`
  - removable if you are not rebuilding immediately.
- `/home/sunrise/rm_ws/log/`
  - build/runtime logs.
- `/home/sunrise/rm_ws/MvSdkLog/`
  - camera SDK logs.

## Practical Recommendation

If the goal is to keep the repo focused on the current competition stack, the minimum long-term core is:

- lower machine: `../Gimbal control/`
- upper machine on RDK: `hik_camera`, `rm_armor_detection`, `rm_gimbal_bridge`, `scripts`
- optional docs: `README.md`, `doc/`

If the goal is to preserve historical work for later comparison, keep the optional reference packages but clearly mark them as `legacy` in a later cleanup pass instead of deleting them immediately.
