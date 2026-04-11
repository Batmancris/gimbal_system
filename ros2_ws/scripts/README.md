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

### 当前主链说明

- `start_autoaim_tmux.sh` 默认启动当前板端三节点：`hik_camera + rm_armor_detection + rm_armor_detection_visualizer`
- `start_rm_bridge_tmux.sh` 单独启动桥接节点：`rm_gimbal_bridge`
- `rm-autoaim.service` 现在建议仅做运行链路自启动，不在开机阶段触发编译
- `rm-bridge.service` 用于桥接链路单独自启动，并提供串口等待和 detector 就绪等待
- 可通过 `STARTUP_DELAY_SEC` 为板端显示链增加启动延时；可通过 `WAIT_FOR_SERIAL_SEC` 为桥接链增加串口等待，避免设备尚未就绪时启动失败
- 当前脚本布局把“板端实时画面观察”和“桥接到下位机”拆成两组入口，便于分开排障
- 路径迁移期间可通过 `REMOTE_SRC_DIR` 和 `REMOTE_SCRIPT_DIR` 覆盖远端源码目录和脚本目录；默认仍保持 `/home/sunrise/rm_ws/src` 与 `/home/sunrise/rm_ws/src/scripts`

示例：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
ros2 run rm_armor_detection rm_armor_detection_visualizer
```

### 使用建议

- 这些脚本服务于当前主线 `hik_camera + rm_armor_detection + rm_gimbal_bridge`
- 使用前请先确认设备地址、权限、串口名和目标环境是否与脚本假设一致
- `usb_cdc_pitch_control_test.py` 仅用于 USB CDC pitch 诊断，不替代 `rm_gimbal_bridge -> USB CDC -> STM32` 视觉主链
- 使用该脚本前，需要 STM32 侧显式打开对应测试模式

### 阅读建议

- 如果你先看到了本目录，建议回头配合 `ros2_ws/README.md` 一起看
- 若你在看桥接链，请同时参考 `rm_gimbal_bridge/README.md`
- 若你在看板端三节点显示链，请同时参考 `hik_camera/README.md` 和 `rm_armor_detection/README.md`

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

### Current Mainline Notes

- `start_autoaim_tmux.sh` starts the current board-side three-node path: `hik_camera + rm_armor_detection + rm_armor_detection_visualizer`
- `start_rm_bridge_tmux.sh` starts the bridge node separately: `rm_gimbal_bridge`
- `rm-autoaim.service` is now intended for runtime-only autostart, while `rm-bridge.service` handles the separate bridge bring-up flow
- use `STARTUP_DELAY_SEC` for the board-side display chain and `WAIT_FOR_SERIAL_SEC` for the bridge chain when devices may appear late at boot
- the current script layout intentionally separates board-side visual inspection from lower-level bridge integration for easier debugging
- during migration, `REMOTE_SRC_DIR` and `REMOTE_SCRIPT_DIR` can override the remote source and script directories; defaults stay compatible with `/home/sunrise/rm_ws/src` and `/home/sunrise/rm_ws/src/scripts`

Example:

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
ros2 run rm_armor_detection rm_armor_detection_visualizer
```

### Usage Guidance

- These scripts are intended for the current `hik_camera + rm_armor_detection + rm_gimbal_bridge` mainline
- Before using them, confirm that the device address, permissions, serial port, and target environment still match the assumptions in the scripts
- `usb_cdc_pitch_control_test.py` is only for USB CDC pitch diagnostics and does not replace the `rm_gimbal_bridge -> USB CDC -> STM32` vision mainline
- Only use the USB CDC test script when the STM32 side explicitly enables the matching test mode
- For boot-time startup, prefer runtime-only autostart instead of build-and-run, and use `STARTUP_DELAY_SEC` plus `WAIT_FOR_SERIAL_SEC` when `/dev/ttyACM0` may appear late

### Reading Guidance

- if you started from this directory, read `ros2_ws/README.md` alongside it
- for the bridge path, pair this with `rm_gimbal_bridge/README.md`
- for the board-side three-node display path, pair this with `hik_camera/README.md` and `rm_armor_detection/README.md`
