# TianAim

<p align="center">
  <img src="assets/tianaim_readme.svg" alt="TianAim auto-aim runtime chain" width="100%">
</p>

<p align="center">
  <a href="https://github.com/Batmancris/gimbal_system/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=stars"></a>
  <a href="https://github.com/Batmancris/gimbal_system/watchers"><img alt="GitHub watchers" src="https://img.shields.io/github/watchers/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=watch"></a>
  <a href="https://github.com/Batmancris/gimbal_system/commits"><img alt="Last commit" src="https://img.shields.io/github/last-commit/Batmancris/gimbal_system?style=for-the-badge"></a>
  <a href="https://github.com/Batmancris/gimbal_system"><img alt="ROS2" src="https://img.shields.io/badge/ROS2-TROS%20%2F%20RDK--X5-39d5ff?style=for-the-badge"></a>
</p>

TianAim 是 Tianbot 的云台自瞄一体化工作区，覆盖上位机 ROS2 感知、下位机 STM32 云台控制、USB-CDC/DBUS 通信、数据采集与后续模型流程。

历史仓库名：`gimbal_system`。当前产品化命名优先使用 `TianAim` / `tianaim_*`。

## 当前主链

```text
遥控器
  -> DBUS / USART3 DMA
  -> STM32 remote_control
  -> gimbal mode / manual control

hik_camera
  -> /hbmem_img
  -> rm_armor_detection
  -> /dnn_node_sample
  -> rm_gimbal_bridge
  -> USB-CDC serial device
  -> STM32 vision_input
  -> target_state
  -> gimbal_task
```

当前代码状态按“实际工作链路”整理：

| 模块 | 主线位置 | 当前结论 |
| --- | --- | --- |
| 上位机感知 | `ros2_ws/` | RDK-X5 / TROS / ROS2 主线，包含相机、识别、桥接与启动脚本 |
| 下位机控制 | `firmware/stm32_gimbal_control/` | STM32F407 云台控制主线，包含 DBUS、CAN、IMU、USB-CDC 与视觉跟随 |
| 遥控链路 | `firmware/stm32_gimbal_control/Chassis/remote_control.c` | 遥控器主要走 DBUS/类 SBUS 帧，经 `USART3 + DMA + IDLE` 解析 |
| 视觉链路 | `ros2_ws/src/rm_gimbal_bridge/` + `USB_DEVICE/App/usbd_cdc_if.c` | 上位机到下位机主要走 USB-CDC 设备，帧格式仍为 `0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD` |
| 数据/模型 | `datasets/`, `models/`, `tools/` | 保留轻量骨架和 manifest，不提交大体量原始数据或权重 |
| 历史参考 | Git history / remote historical `main` | 本地 `archive/historical_code/` 快照已清理，旧代码从 Git 历史查 |

> 想看谁给项目点了星：打开 [Stargazers](https://github.com/Batmancris/gimbal_system/stargazers)。README 里的 badge 会自动显示当前 star 数。

## 快速开始

构建上位机 ROS2 工作区：

```bash
bash scripts/build_ros2_mainline.sh
```

构建 STM32 固件：

```bash
bash scripts/build_firmware_mainline.sh
```

运行桥接节点：

```bash
bash scripts/run_ros2_bridge.sh
```

桥接默认参数由脚本维护；RDK 侧当前常用 USB-CDC 设备路径类似：

```text
/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

路径迁移兼容变量：

- `TIANAIM_ROS_WS`
- `TIANAIM_FIRMWARE_DIR`
- `REMOTE_WS`
- `REMOTE_SRC_DIR`
- `REMOTE_SCRIPT_DIR`

## 仓库地图

```text
.
├── ros2_ws/                         # 上位机: ROS2 packages and RDK scripts
├── firmware/stm32_gimbal_control/   # 下位机: STM32 firmware
├── datasets/                        # Dataset structure and manifests
├── models/                          # Model metadata and export notes
├── tools/                           # Capture, labeling, training, evaluation, DBUS diagnostics
├── scripts/                         # Top-level build/run wrappers
├── docs/                            # Architecture, migration, backlog
├── assets/                          # README visuals
└── archive/                         # Audit/recovery notes only
```

关键入口：

- `ros2_ws/src/hik_camera/`
- `ros2_ws/src/rm_armor_detection/`
- `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- `firmware/stm32_gimbal_control/Chassis/remote_control.c`
- `firmware/stm32_gimbal_control/Src/vision_input.c`
- `firmware/stm32_gimbal_control/Src/target_state.c`
- `firmware/stm32_gimbal_control/Src/gimbal_task.c`
- `firmware/stm32_gimbal_control/USB_DEVICE/App/usbd_cdc_if.c`

## 开发约定

- 上位机运行时代码放在 `ros2_ws/`
- 下位机控制与通信改动放在 `firmware/stm32_gimbal_control/`
- 数据、标注、训练、评估工具放在 `tools/`, `datasets/`, `models/`
- `archive/` 只放审计和恢复说明，不再放历史源码快照
- 结构、链路、协议或启动命令变更时，同步更新 `README.md`, `AGENTS.md`, `docs/architecture.md`

## 文档入口

- 架构说明：`docs/architecture.md`
- 迁移计划：`docs/migration_plan.md`
- 待办事项：`docs/backlog.md`
- 上位机说明：`ros2_ws/README.md`
- 下位机说明：`firmware/stm32_gimbal_control/README.md`
- 归档审计：`archive/repo_audit_2026-04-11.md`
