# scripts

[中文](#中文) | [English](#english)

## 中文

### 简介

`scripts` 目录保存当前上位机主线部署与联调脚本。

### 当前内容

- 部署到 RDK-X5 的脚本
- 远端编译并启动的脚本
- 自动启动安装脚本
- 话题检查脚本
- `systemd` 服务文件
- `tmux` 启动脚本
- `usb_cdc_pitch_control_test.py`
  用于 `/dev/ttyACM0` 的最小 pitch 控制验证脚本

### 使用建议

- 这些脚本服务于当前主线 `hik_camera + rm_armor_detection + rm_gimbal_bridge`
- 使用前请先确认设备地址、权限、串口名和目标环境是否与脚本假设一致
- `usb_cdc_pitch_control_test.py` 仅用于 USB CDC 通信验证，不替代当前 UART 主链
- 使用该脚本前，需要 STM32 侧显式打开对应测试模式

## English

### Overview

The `scripts` directory stores deployment and integration helper scripts for the current upper-level mainline.

### Current Contents

- deployment scripts for RDK-X5
- remote build-and-run helpers
- autostart installation scripts
- topic inspection scripts
- `systemd` service file
- `tmux` startup helper
- `usb_cdc_pitch_control_test.py` for minimal `/dev/ttyACM0` pitch-control validation

### Usage Guidance

- These scripts are intended for the current `hik_camera + rm_armor_detection + rm_gimbal_bridge` mainline
- Before using them, confirm that the device address, permissions, serial port, and target environment still match the assumptions in the scripts
- `usb_cdc_pitch_control_test.py` is only for USB CDC communication validation and does not replace the UART mainline
- Only use the USB CDC test script when the STM32 side explicitly enables the matching test mode
