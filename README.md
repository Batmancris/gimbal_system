# gimbal_system

[中文](#中文) | [English](#english)

## 中文

### 项目概览

本仓库于 2026-03-19 整理为单一 monorepo，用来承载当前机器人云台系统相关代码。

当前工作区包含三条主要代码线：

- `Gimbal control`：STM32 下位机主线工程
- `dev-branch`：ROS2 上位机主线工作区
- `tianboard_s`：参考/备用板级工程

### 当前主线状态

当前系统已经具备一条可以继续联调和维护的主路径：

1. `hik_camera` 采集海康工业相机图像
2. `rm_armor_detection` 在 RDK-X5 / TROS 环境下做 YOLOv8 装甲板检测
3. `rm_gimbal_bridge` 将检测结果转换为下位机当前使用的串口协议
4. `Gimbal control` 通过 UART 接收视觉输入并驱动云台控制链

USB CDC 目前已经在下位机侧完成基础双向通信诊断，但还没有完全取代 UART 成为正式主链。

### 仓库结构

```text
gimbal_system/
├── Gimbal control/   # STM32 firmware mainline
├── dev-branch/       # ROS2 vision and bridge workspace
└── tianboard_s/      # Reference / backup board-level project
```

### 目录说明

#### `Gimbal control`

- 当前唯一可信的下位机主线
- 负责云台、电机、IMU、视觉输入、FreeRTOS 任务组织
- 当前稳定通信主链：`UART`
- 当前实验迁移方向：`USB CDC`

#### `dev-branch`

- 当前唯一可信的上位机主线
- 负责相机驱动、视觉检测、串口桥接、部署脚本
- 当前建议维护路径：`hik_camera + rm_armor_detection + rm_gimbal_bridge`

#### `tianboard_s`

- 不是当前比赛/联调主线
- 主要用于底层驱动、USB Device、历史实现的参考与对照

### 当前文档策略

本仓库 README 以“当前真实状态”为准，不再默认沿用旧分仓时代的描述。

如果文档与代码冲突，请优先相信：

1. 当前源码入口与 launch 文件
2. 当前主线 README
3. 历史 handover / 旧仓库文档

### 当前已知边界

- monorepo 已建立，但正式长期分支策略还没有完全收口
- USB CDC 已验证传输层，但尚未完成整机主链切换
- 一些历史/实验目录仍被保留，用于参考，不等于当前运行入口

## English

### Overview

This repository was consolidated into a single monorepo on 2026-03-19 and now serves as the current source tree for the gimbal system workspace.

It contains three primary code areas:

- `Gimbal control`: STM32 lower-level firmware mainline
- `dev-branch`: ROS2 upper-level vision and bridge workspace
- `tianboard_s`: reference / backup board-level project

### Current Mainline Status

The project now has a practical end-to-end development path:

1. `hik_camera` captures images from the Hikrobot industrial camera
2. `rm_armor_detection` runs YOLOv8-based armor detection on RDK-X5 / TROS
3. `rm_gimbal_bridge` converts detections into the serial protocol used by the controller
4. `Gimbal control` receives visual input over UART and drives the gimbal control chain

USB CDC has already been validated as a bidirectional transport on the firmware side, but it has not yet replaced UART as the formal system mainline.

### Repository Layout

```text
gimbal_system/
├── Gimbal control/   # STM32 firmware mainline
├── dev-branch/       # ROS2 vision and bridge workspace
└── tianboard_s/      # Reference / backup board-level project
```

### Directory Roles

#### `Gimbal control`

- The only trusted lower-level firmware mainline
- Owns gimbal, motor, IMU, vision input, and FreeRTOS task organization
- Current validated communication path: `UART`
- Current migration direction: `USB CDC`

#### `dev-branch`

- The only trusted upper-level workspace mainline
- Owns camera drivers, visual detection, serial bridge, and deployment scripts
- Recommended maintenance path: `hik_camera + rm_armor_detection + rm_gimbal_bridge`

#### `tianboard_s`

- Not the current competition / integration mainline
- Kept as a reference source for low-level drivers, USB Device code, and historical implementations

### Documentation Policy

The README files in this monorepo are intended to describe the current real project state instead of the old split-repository workflow.

If documentation conflicts with code, trust the following in order:

1. Current source entry points and launch files
2. Current mainline README files
3. Historical handover and legacy repository documents

### Known Boundaries

- The monorepo exists, but the long-term branch policy is not fully settled yet
- USB CDC transport is verified, but the whole-system migration is not complete
- Some historical and experimental directories are intentionally retained as references, not as the default runtime path
