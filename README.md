# TianAim 云台视觉跟随系统

TianAim 是一个面向云台视觉跟随的主代码仓库。当前仓库定位为 `gimbal_system` 的单独维护主仓，只保留云台主系统相关代码和文档。

## 项目定位

本项目围绕一条可运行的云台跟随链路组织：

```text
hik_camera
  -> rm_bear_detection / rm_vehicle_detection / rm_armor_detection
  -> rm_gimbal_bridge
  -> STM32 USB-CDC
  -> GM6020 云台
```

当前主线以低速视觉跟随可用为基准。高速跟随、参数优化和进一步鲁棒性验证仍待继续。

## 当前仓库范围

本仓库保留以下内容：

- ROS2/TROS 上位机工作区：相机采集、目标检测节点、云台桥接节点、板端部署脚本。
- STM32 下位机固件：USB-CDC 视觉输入、DBUS 遥控输入、CAN 电机控制、GM6020 云台控制相关代码。
- 顶层部署脚本：从仓库根目录调用的构建和运行入口。
- 模型配置和轻量元数据：当前运行链路需要的配置、说明和报告。
- 文档：架构说明、仓库治理、部署维护和发布风险检查。

本仓库不包含或不再维护以下内容：

- 训练工具。
- 量化工具。
- 采集工具。
- 大数据集实体。
- 大模型训练权重和训练中间产物。

训练/量化/采集工具已拆分为独立工具仓，本仓库仅保留云台主系统代码。

## 当前状态

- 低速目标跟随：当前可用。
- 高速目标跟随：仍存在跟随滞后或参数不足风险，后续需要现场日志和参数优化。
- RDK X5 板端：保留部署脚本和运行说明，发布前应完成实机复核。
- 仓库治理：公开主仓不混入训练、量化、采集工具或本地环境产物。

## 主链路

```text
Hikvision camera
  -> hik_camera
  -> /hbmem_img
  -> detection
  -> /bear_detection/targets 或兼容检测话题
  -> rm_gimbal_bridge
  -> STM32 USB-CDC
  -> vision_input
  -> target_state
  -> gimbal_task
  -> CAN
  -> GM6020 gimbal motors
```

兼容检测模块仍保留在仓库中，但 README 不把未验证的新能力写成已完成结果。

## 目录结构

```text
gimbal_system/
├── README.md
├── AGENTS.md
├── ros2_ws/      # ROS2/TROS 上位机工作区
├── firmware/     # STM32 下位机固件
├── scripts/      # 顶层构建、运行入口
├── models/       # 模型配置、登记和报告
├── datasets/     # 数据集骨架、manifest 和说明
├── docs/         # 架构、部署、维护和发布风险文档
├── assets/       # README 和展示资产
└── archive/      # 可选历史说明
```

关键入口：

- `ros2_ws/README.md`
- `ros2_ws/scripts/README.md`
- `ros2_ws/src/hik_camera/`
- `ros2_ws/src/rm_bear_detection/`
- `ros2_ws/src/rm_vehicle_detection/`
- `ros2_ws/src/rm_armor_detection/`
- `ros2_ws/src/rm_gimbal_bridge/`
- `firmware/stm32_gimbal_control/README.md`
- `docs/repo_cleanup/`

## 快速开始

从仓库根目录构建 ROS2 主线：

```bash
bash scripts/build_ros2_mainline.sh
```

从仓库根目录构建 STM32 固件：

```bash
bash scripts/build_firmware_mainline.sh
```

运行云台桥接入口：

```bash
bash scripts/run_ros2_bridge.sh
```

RDK X5 常用工作区约定：

```text
/home/sunrise/rm_ws
```

常用环境变量：

- `TIANAIM_ROS_WS`
- `TIANAIM_FIRMWARE_DIR`
- `RDK_HOST`
- `RDK_USER`
- `REMOTE_WS`
- `REMOTE_SRC_DIR`
- `REMOTE_SCRIPT_DIR`

板端部署和启动脚本位于 `ros2_ws/scripts/`。实际部署前应在 RDK X5 上复核话题、服务和自启动脚本。

## GitHub 发布注意事项

发布前建议逐项确认：

- 不提交大数据集、训练权重、量化中间产物、视频和本地缓存。
- 已拆分的训练/量化/采集工具不要作为主仓组成部分重新加入。
- 当前已跟踪的二进制文件需要确认许可和发布必要性，尤其是相机 SDK 动态库、STM32 静态库和量化模型 bin。
- README、docs 和 Git 状态应与最终目录结构一致。
- 不在发布准备提交中混入算法、参数、固件或运行脚本修改。
- 发布分支、保护分支和历史重构分支的关系按 `docs/github_publish_checklist.md` 复核。

更多发布准备文档见 `docs/repo_cleanup/`。
