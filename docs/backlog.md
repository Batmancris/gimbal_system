# Backlog

## P0

- keep root documentation aligned with the real runtime chain
- keep UART as the formal stable communication mainline
- standardize dataset skeleton and manifest format
- integrate Hik capture through a real adapter once the SDK surface is confirmed
- stage `dev-branch/` and `Gimbal control/` migration plans before renaming directories

## P1

- add `ros2_ws/src/` compatibility migration plan with updated deployment scripts
- add `firmware/stm32_gimbal_control/` compatibility migration plan with Makefile/tooling validation
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
- normalize historical references into a physical `archive/` tree after compatibility is secured
