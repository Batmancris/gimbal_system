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

## 产品定位

TianAim 是 Tianbot 云台视觉跟随系统的一体化工作区，覆盖：

- RDK X5 上位机 ROS2/TROS 感知链路
- STM32F407 下位机云台控制固件
- USB-CDC 视觉通信、DBUS 遥控输入、CAN 电机控制
- 数据采集、标注、训练、评估辅助工具

## 当前状态

**TianAim v1 headless fast_best bear-follow baseline** (2026-05-15)

- 当前运行目标：bear（熊）
- 跟随丝滑，卡顿明显减少
- 偶发卡顿主要怀疑来自目标出框/丢检/bbox 突变
- 当前基线 commit: `f8cdec2`
- 分支: `feature/formula-mini-kt-cloud-follow`（历史命名；当前运行目标是 bear，KT 适配是后续工作）

### 当前不包含

- KT 模型
- KT 检测器
- KT 专用目标输出

### 后续工作

- KT target adaptation / detector replacement / target_type abstraction

## 当前主链路

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> USB-CDC -> STM32 -> 云台
```

正确节点: `/hik_camera`, `/rm_bear_detection`, `/rm_gimbal_bridge`

正确 topic: `/hbmem_img`, `/bear_detection/targets`

### 禁止依赖（不用于主链路）

- `/image_raw`
- `rm_vis`
- `rm_armor_detection_visualizer`
- `publish_image_raw:=true`

## 快速开始

### 一键启动（推荐）

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

启动成功判据: `Camera OK.` / `Detection OK.` / `Bridge OK.` / `FAST FOLLOW READY.` / `profile: fast_best.`

### 回滚启动

```bash
ssh rdk-x5 "FOLLOW_PROFILE=stable bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

### 快速验收

```bash
ssh rdk-x5 "source /opt/tros/humble/setup.bash; \
  source /home/sunrise/rm_ws/install/setup.bash; \
  ros2 node list; \
  ros2 topic info /hbmem_img; \
  ros2 topic info /bear_detection/targets; \
  fuser -v /dev/ttyACM0 2>/dev/null || true"
```

### 性能采集

```bash
ssh rdk-x5 "cd /home/sunrise/rm_ws && DURATION=15 bash scripts/profile_fast_follow_link.sh"
```

### 停止

```bash
ssh rdk-x5 "tmux -L autoaim kill-server 2>/dev/null; pkill -f 'ros2 run' || true"
```

## 模块状态

| 模块 | 路径 | 状态 | 说明 |
|---|---|---|---|
| 相机驱动 | `ros2_ws/src/hik_camera/` | **core** | 海康相机，发布 /hbmem_img |
| bear 检测 | `ros2_ws/src/rm_bear_detection/` | **core** | YOLO bear 检测，当前主链路 |
| 云台桥接 | `ros2_ws/src/rm_gimbal_bridge/` | **core** | 选目标、抗跳变、USB-CDC 发送 |
| 消息定义 | `ros2_ws/src/rm_interfaces/` | **core** | ROS2 自定义消息 |
| 工具库 | `ros2_ws/src/rm_utils/` | **core** | logger, math, heartbeat |
| vehicle 检测 | `ros2_ws/src/rm_vehicle_detection/` | legacy/optional | 兼容保留 |
| armor 检测 | `ros2_ws/src/rm_armor_detection/` | legacy/optional | 兼容保留 |
| STM32 固件 | `firmware/stm32_gimbal_control/` | **core** | 云台控制、电机 PID |
| 启动脚本 | `ros2_ws/scripts/` | **core** | 唯一推荐入口 + 底层 loop |
| 数据工具 | `tools/`, `datasets/` | optional | 采集/训练/评估 |
| 审计/恢复 | `archive/` | optional | 历史参考 |

## 仓库地图

```text
.
├── ros2_ws/                         # 上位机: ROS2 packages and RDK scripts
│   ├── src/
│   │   ├── hik_camera/              # [core] 海康相机驱动
│   │   ├── rm_bear_detection/       # [core] bear YOLO 检测
│   │   ├── rm_gimbal_bridge/        # [core] 云台桥接
│   │   ├── rm_interfaces/           # [core] 消息定义
│   │   ├── rm_utils/                # [core] 工具库
│   │   ├── rm_vehicle_detection/    # [legacy] vehicle 检测
│   │   └── rm_armor_detection/      # [legacy] armor 检测
│   └── scripts/                     # 启动/诊断/部署脚本
├── firmware/stm32_gimbal_control/   # 下位机: STM32 固件
├── datasets/                        # 数据集骨架
├── models/                          # 模型元数据
├── tools/                           # 采集/训练/评估工具
├── scripts/                         # 顶层构建/路径工具
├── assets/                          # README 图片
├── archive/                         # 审计/恢复说明
└── docs/                            # 项目文档
```

## 关键参数

上位机桥接参数在 `ros2_ws/scripts/run_rm_bridge_loop.sh` 中维护（fast_best profile），不要在其他地方重复定义。

下位机视觉跟随参数在 `firmware/stm32_gimbal_control/Src/gimbal_task.h`。

## 文档入口

- 架构说明: `docs/architecture.md`
- ROS2 脚本: `ros2_ws/scripts/README.md`
- ROS2 工作区: `ros2_ws/README.md`
- 云台桥接: `ros2_ws/src/rm_gimbal_bridge/README.md`
- bear 检测: `ros2_ws/src/rm_bear_detection/README.md`
- STM32 固件: `firmware/stm32_gimbal_control/README.md`
- 风格规范: `docs/tianbot_style_notes.md`
- 清理计划: `docs/productization_cleanup_plan.md`
- 历史归档: `docs/archive/`

## 后续 TODO

- KT target adaptation / detector replacement / target_type abstraction（后续目标适配）
- 出框恢复策略：目标丢失后的重捕获逻辑
- 尾延迟优化：相机/检测 CPU 较高，profile 尾延迟
- 检测跳变分析：bbox 突变和多框场景专项分析
- 代码清理：标记 legacy 脚本，归档 systemd service 文件
