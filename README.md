# TianAim / gimbal_system

<p align="center">
  <img src="assets/tianaim_readme.svg" alt="TianAim auto-aim runtime chain" width="100%">
</p>

<p align="center">
  <a href="https://github.com/Batmancris/gimbal_system/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=stars"></a>
  <a href="https://github.com/Batmancris/gimbal_system/watchers"><img alt="GitHub watchers" src="https://img.shields.io/github/watchers/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=watch"></a>
  <a href="https://github.com/Batmancris/gimbal_system/commits"><img alt="Last commit" src="https://img.shields.io/github/last-commit/Batmancris/gimbal_system?style=for-the-badge"></a>
  <a href="https://github.com/Batmancris/gimbal_system"><img alt="ROS2" src="https://img.shields.io/badge/ROS2-TROS%20%2F%20RDK--X5-39d5ff?style=for-the-badge"></a>
</p>

[中文](#中文) | [English](#english)

## 中文

### 产品定位

本仓库历史名称是 `gimbal_system`，当前产品化方向统一为 `TianAim`。它是 Tianbot 云台视觉跟随/自瞄系统的一体化工作区，覆盖：

- RDK X5 上位机 ROS2/TROS 感知链路。
- STM32F407 / C 板下位机云台控制固件。
- USB-CDC 视觉通信、DBUS 遥控输入、CAN 电机控制。
- 数据采集、标注、训练、评估和模型部署辅助工具。
- 面向人类开发者和 AI agent 的统一文档入口。

这版 README 融合了两个历史分支的风格：保留 `main` 分支的产品主页、badge、主链表格和仓库地图，也保留 `chore/repo-restructure` 分支的产品定位、过渡结构、快速开始和路线图表达。

### 当前真实进展

截至 2026-04-23，当前主线状态必须如实描述为：

- 低速目标跟随已经完成，现场表现很丝滑。
- 高速跟随仍然存在跟不上的情况，不能写成“高速已经完成”。
- 当前默认调试目标已经切到 bear，主链路是 `hik_camera -> rm_bear_detection -> rm_gimbal_bridge -> STM32 USB-CDC`。
- 这轮主要进展不是重新训练模型，而是 RDK X5 运行链路、目标稳定筛选、桥接发送、USB-CDC 通信和 STM32 跟随控制参数整理。
- 后续高速优化需要继续联合看检测延迟、目标跳变、桥接预测/限幅、下位机角度增量和机械响应。

### 当前主链

```text
遥控器
  -> DBUS / USART3 DMA
  -> STM32 remote_control
  -> gimbal mode / manual control

hik_camera
  -> /hbmem_img
  -> rm_bear_detection
  -> /bear_detection/targets
  -> rm_gimbal_bridge
  -> USB-CDC serial device
  -> STM32 vision_input
  -> target_state
  -> gimbal_task
```

兼容链路仍保留：

```text
rm_vehicle_detection -> /vehicle_detection/targets
rm_armor_detection   -> /dnn_node_sample
```

### 模块状态

| 模块 | 主线位置 | 当前结论 |
| --- | --- | --- |
| 上位机感知 | `ros2_ws/` | RDK X5 / TROS / ROS2 主线，包含相机、bear/vehicle/armor 检测、桥接和启动脚本 |
| 当前默认检测 | `ros2_ws/src/rm_bear_detection/` | bear 单类别检测，当前低速跟随主线 |
| 云台桥接 | `ros2_ws/src/rm_gimbal_bridge/` | 选目标、抗跳变、固定频率发送目标中心点，并读取 STM32 诊断 |
| 下位机控制 | `firmware/stm32_gimbal_control/` | STM32F407 云台控制主线，负责视觉误差转角度增量、电机闭环和诊断回传 |
| 遥控链路 | `firmware/stm32_gimbal_control/Chassis/remote_control.c` | 遥控器主要走 DBUS/类 SBUS 帧，经 `USART3 + DMA + IDLE` 解析 |
| 视觉链路 | `rm_gimbal_bridge` + `USB_DEVICE/App/usbd_cdc_if.c` | 上位机到下位机主要走 USB-CDC，坐标帧格式为 `0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD` |
| 数据/模型 | `datasets/`, `models/`, `tools/` | 保留轻量骨架、manifest 和工具，不建议直接提交大体量原始数据或权重 |
| 历史参考 | Git history / `archive/` | 旧链路仅作参考，不作为当前参数来源 |

### 仓库地图

```text
.
├── ros2_ws/                         # 上位机: ROS2 packages and RDK scripts
├── firmware/stm32_gimbal_control/   # 下位机: STM32 firmware
├── datasets/                        # Dataset structure and manifests
├── models/                          # Model metadata and export notes
├── tools/                           # Capture, labeling, training, evaluation, diagnostics
├── scripts/                         # Top-level build/run wrappers
├── assets/                          # README visuals
└── archive/                         # Audit/recovery notes only
```

关键入口：

- `ros2_ws/src/hik_camera/`
- `ros2_ws/src/rm_bear_detection/`
- `ros2_ws/src/rm_vehicle_detection/`
- `ros2_ws/src/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- `firmware/stm32_gimbal_control/Src/vision_input.c`
- `firmware/stm32_gimbal_control/Src/target_state.c`
- `firmware/stm32_gimbal_control/Src/gimbal_task.c`
- `firmware/stm32_gimbal_control/Src/gimbal_task.h`

### 快速开始

本地构建 ROS2 主线：

```bash
bash scripts/build_ros2_mainline.sh
```

本地构建 STM32 固件：

```bash
bash scripts/build_firmware_mainline.sh
```

运行桥接节点：

```bash
bash scripts/run_ros2_bridge.sh
```

RDK X5 板端常用工作区：

```text
/home/sunrise/rm_ws
```

RDK X5 板端常用 USB-CDC 设备路径：

```text
/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

路径兼容变量：

- `TIANAIM_ROS_WS`
- `TIANAIM_FIRMWARE_DIR`
- `RDK_HOST`
- `RDK_USER`
- `REMOTE_WS`
- `REMOTE_SRC_DIR`
- `REMOTE_SCRIPT_DIR`

### 从 Windows 进入 RDK X5

在 Windows PowerShell 进入本仓库：

```powershell
cd E:\research\1\yolo\gimbal_system_repo_push
```

设置板子地址，把 IP 换成实际 RDK X5 地址：

```powershell
$env:RDK_HOST="192.168.1.10"
$env:RDK_USER="sunrise"
```

SSH 登录 RDK X5：

```powershell
ssh sunrise@$env:RDK_HOST
```

上板后进入工作区并加载环境：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
```

优先查看这些目录：

```bash
ls -la
ls -la src
ls -la src/scripts
ls -la src/rm_bear_detection
ls -la src/rm_gimbal_bridge
ls -la install
```

查看进程、节点和话题：

```bash
ps -ef | grep -E 'hik_camera|rm_bear_detection|rm_vehicle_detection|rm_gimbal_bridge|tmux' | grep -v grep
ros2 node list
ros2 topic list
ros2 topic info /hbmem_img -v
ros2 topic info /bear_detection/targets -v
ros2 topic echo /bear_detection/targets --once
```

查看 tmux 后台日志：

```bash
tmux -L autoaim ls
tmux -L autoaim capture-pane -pt hik_cam
tmux -L autoaim capture-pane -pt rm_det
tmux -L bridge capture-pane -pt rm_bridge
```

### 从 Windows 部署和启动

从 Windows PowerShell 或 Git Bash 进入 `ros2_ws`：

```powershell
cd E:\research\1\yolo\gimbal_system_repo_push\ros2_ws
```

部署到 RDK X5：

```bash
bash scripts/deploy_to_rdk_x5.sh
```

远程清理、编译并启动：

```bash
bash scripts/build_and_run_on_rdk_x5.sh
```

如果想上板后手动启动：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash

export DETECTOR_TYPE=bear
export DETECTOR_TOPIC=/bear_detection/targets
export ALLOWED_TARGET_TYPES=bear

bash src/scripts/start_autoaim_tmux.sh
bash src/scripts/start_rm_bridge_tmux.sh
```

状态检查：

```bash
bash src/scripts/check_autoaim_topics.sh
bash src/scripts/desktop_status_full_stack.sh
```

停止整套服务：

```bash
bash src/scripts/desktop_stop_full_stack.sh
```

### 从 ros2go 进入 RDK X5

在 ros2go 系统终端设置板子地址：

```bash
export RDK_HOST=192.168.1.10
export RDK_USER=sunrise
```

登录 RDK X5：

```bash
ssh ${RDK_USER}@${RDK_HOST}
```

进入工作区：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
```

查看主文件和运行状态：

```bash
ls src
ls src/scripts
ros2 node list
ros2 topic list
ros2 topic info /bear_detection/targets -v
```

如果 ros2go 机器上也有本仓库，可以直接部署：

```bash
cd /path/to/gimbal_system_repo_push/ros2_ws
export RDK_HOST=192.168.1.10
bash scripts/deploy_to_rdk_x5.sh
bash scripts/build_and_run_on_rdk_x5.sh
```

只拷贝单个脚本：

```bash
scp scripts/check_autoaim_topics.sh sunrise@${RDK_HOST}:/home/sunrise/rm_ws/src/scripts/
```

### 当前关键参数

上位机桥接默认参数在 `ros2_ws/scripts/run_rm_bridge_loop.sh`：

- `DETECTOR_TOPIC=/bear_detection/targets`
- `ALLOWED_TARGET_TYPES=bear`
- `FOLLOW_SEND_RATE_HZ=50.0`
- `FOLLOW_CONTROL_MODE=light_predict`
- `FOLLOW_MAX_STEP_PX=36.0`
- `FAST_FOLLOW_MAX_STEP_PX=72.0`
- `TARGET_HOLD_MS=350`
- `BRIDGE_MIN_CONFIDENCE=0.71`
- `CENTER_GATE_X_RATIO=1.00`
- `CENTER_GATE_Y_RATIO=1.00`

下位机视觉跟随参数在 `firmware/stm32_gimbal_control/Src/gimbal_task.h`：

- `VISION_X_DEADBAND = 14.0f`
- `VISION_Y_DEADBAND = 14.0f`
- `VISION_YAW_PID_KP = 0.0000062f`
- `VISION_YAW_PID_KD = 0.000055f`
- `VISION_PITCH_PID_KP = 0.0000060f`
- `VISION_PITCH_PID_KD = 0.000042f`
- `VISION_MAX_ANGLE_STEP = 0.0036f`
- `VISION_FAST_ANGLE_STEP = 0.0050f`
- `VISION_CMD_SMOOTH_ALPHA = 0.30f`
- `VISION_CMD_FAST_ALPHA = 0.42f`
- `VISION_CMD_BRAKE_ALPHA = 0.92f`
- `VISION_SLOWDOWN_ERROR_PX = 300.0f`
- `VISION_FRAME_HOLD_DECAY = 0.990f`
- `VISION_FRAME_BRAKE_DECAY = 0.970f`

这些参数对应的是“低速丝滑，高速仍需继续优化”的当前状态。

### 高速跟随后续方向

1. 先打开 `BRIDGE_LOG_DIAG_FEEDBACK=true` 和 `BEAR_LOG_DETECTIONS=true`，采高速场景日志。
2. 先判断是检测丢失、目标跳变、桥接限速、下位机限幅还是机械响应不足。
3. 不破坏低速丝滑效果的前提下，小步调整 `FAST_FOLLOW_MAX_STEP_PX`、`LIGHT_FOLLOW_GAIN`、`VISION_FAST_ANGLE_STEP`。
4. 如果追上后过冲，优先回调刹车和平滑参数，不要盲目继续加 KP。
5. 如果确认目标明显滞后，再评估 `PREDICT_BETA` 和 `PREDICT_HORIZON_SEC`，并且必须现场验证。

### 开发约定

- 上位机运行时代码放在 `ros2_ws/`。
- 下位机控制与通信改动放在 `firmware/stm32_gimbal_control/`。
- 数据、标注、训练、评估工具放在 `tools/`, `datasets/`, `models/`。
- `archive/` 只放审计和恢复说明，不放新的运行时代码。
- 改目录结构、链路、协议、参数或启动命令时，同步更新最近的 README。

### 文档入口

- ROS2 板端运行：`ros2_ws/README.md`
- RDK 脚本：`ros2_ws/scripts/README.md`
- 云台桥接：`ros2_ws/src/rm_gimbal_bridge/README.md`
- bear 检测：`ros2_ws/src/rm_bear_detection/README.md`
- vehicle 检测：`ros2_ws/src/rm_vehicle_detection/README.md`
- STM32 固件：`firmware/stm32_gimbal_control/README.md`
- Agent 协作约定：`AGENTS.md`

### Roadmap

- P0：保持当前低速丝滑主线稳定，补齐 README 和调试入口。
- P0：继续采集高速跟随日志，明确高速跟不上的主因。
- P1：在不破坏低速效果的前提下调试高速桥接参数和下位机限幅。
- P1：补充高速运动、运动模糊、遮挡和快速横移数据。
- P2：验证轻量预测参数或 alpha-beta 预测是否能改善高速滞后。
- P2：建立更完整的检测、桥接、下位机诊断联合评估脚本。

## English

### Product Position

The historical repository name is `gimbal_system`; the product-facing direction is `TianAim`.

This repository is the integrated workspace for the Tianbot gimbal vision-follow / auto-aim stack:

- RDK X5 ROS2/TROS perception.
- STM32F407 lower-level gimbal firmware.
- USB-CDC vision communication, DBUS remote input, and CAN motor control.
- Dataset, labeling, training, evaluation, and model deployment helpers.

### Current Status

As of 2026-04-23:

- Low-speed target following is complete and smooth in field testing.
- High-speed following is not complete yet; the gimbal can still lag behind fast targets.
- The current default path is the bear pipeline:

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> STM32 USB-CDC
```

The vehicle and armor paths remain available for compatibility.

### Quick Start

Build ROS2 packages:

```bash
bash scripts/build_ros2_mainline.sh
```

Build STM32 firmware:

```bash
bash scripts/build_firmware_mainline.sh
```

Run the bridge:

```bash
bash scripts/run_ros2_bridge.sh
```

### Repository Map

```text
ros2_ws/                       ROS2/TROS packages and RDK scripts
firmware/stm32_gimbal_control/ STM32 lower-level firmware
datasets/                      Dataset manifests and structure
models/                        Model metadata and export notes
tools/                         Capture, labeling, training, evaluation
scripts/                       Top-level build/run wrappers
archive/                       Audit and recovery notes
```

### More Detail

- ROS2 runtime: `ros2_ws/README.md`
- RDK scripts: `ros2_ws/scripts/README.md`
- Bridge package: `ros2_ws/src/rm_gimbal_bridge/README.md`
- Bear detector: `ros2_ws/src/rm_bear_detection/README.md`
- Firmware: `firmware/stm32_gimbal_control/README.md`

