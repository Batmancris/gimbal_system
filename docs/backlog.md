# Backlog

## P0

- keep root documentation aligned with the real runtime chain
- keep UART as the formal stable communication mainline
- standardize dataset skeleton and manifest format
- integrate Hik capture through a real adapter once the SDK surface is confirmed
- keep the path compatibility layer validated after the migration landing commit

## P1

- complete board-side follow-up validation for current RDK runtime scripts under `ros2_ws/scripts/`
- continue stale path reference cleanup when old references are found in active docs or scripts
- add image import and session resume support to the capture tool
- add basic labeling export helpers
- document topic and parameter naming conventions under a `tianaim_*` vocabulary

## P2

- add split generation and dataset QA tooling
- add training runner and evaluation report generator
- add prediction-stabilized controller path while keeping the existing controller as fallback
- add hardware validation checklist for UART vs USB CDC path promotion

## P3

- evaluate Kalman / alpha-beta tracker integration
- evaluate LQR controller experiments behind explicit experimental configuration

## Completed / Follow-Up Validation

- moved current ROS2 mainline packages into `ros2_ws/src/`
- moved current RDK runtime scripts into `ros2_ws/scripts/`
- moved current STM32 firmware into `firmware/stm32_gimbal_control/`
- normalized retained historical references into `archive/historical_code/`
- added top-level build/run wrappers with staged path overrides
- follow-up validation: keep `bash -n scripts/*.sh ros2_ws/scripts/*.sh`, `bash scripts/build_ros2_mainline.sh`, and `bash scripts/build_firmware_mainline.sh` green after path-related changes
