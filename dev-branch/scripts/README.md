# scripts

This directory contains small deployment and validation helpers for the current upper-computer workspace.

## Update Since 2026-03-19

- Added `usb_cdc_pitch_control_test.py` for the temporary USB CDC validation path
- The script opens `/dev/ttyACM0` and sends a smooth pitch sweep command for lower-computer communication testing
- This script is only for communication validation and does not replace the current UART mainline

## Notes

- Keep using `rm_gimbal_bridge` + UART as the default integration path
- Use the USB CDC script only when the STM32 side explicitly enables the test mode
