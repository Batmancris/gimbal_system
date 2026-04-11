# Repository Audit

Archived: 2026-04-11

This is a short migration audit summary. Use `README.md` for navigation and `docs/architecture.md` for the current structure.

## Current Source Of Truth

- 上位机 / ROS2 mainline: `ros2_ws/`
- 下位机 / STM32 firmware mainline: `firmware/stm32_gimbal_control/`
- top-level wrappers: `scripts/`
- data/model/tool workflow: `datasets/`, `models/`, `tools/`

## Current Verified Chain

```text
hik_camera -> rm_armor_detection -> rm_gimbal_bridge -> UART -> STM32 gimbal control
```

Key locations:

- camera package: `ros2_ws/src/hik_camera/`
- detector package: `ros2_ws/src/rm_armor_detection/`
- bridge package: `ros2_ws/src/rm_gimbal_bridge/`
- UART frame source: `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- firmware vision input: `firmware/stm32_gimbal_control/Src/vision_input.c`
- firmware target state: `firmware/stm32_gimbal_control/Src/target_state.c`
- firmware control logic: `firmware/stm32_gimbal_control/Src/gimbal_task.c`

## Historical Material

Treat these as reference or backup areas unless a task explicitly says otherwise:

- `archive/historical_code/dev-branch/`
- `archive/historical_code/tianboard_s/`
- `archive/`
- `archive/historical_code/_git_migration_backup/`
- `.git.BAK-*`

The many README files under `archive/historical_code/dev-branch/ultralytics-8.2.103/` are third-party historical material, not active TianAim documentation.

## Remaining Cleanup Risk

- Some old references may still mention the previous firmware path with a space-containing name.
- Historical directories are preserved in place, so search results can include non-mainline code.
- Large binary assets and old third-party README files can make the tree feel noisier than the active product structure really is.
