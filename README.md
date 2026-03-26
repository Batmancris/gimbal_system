# gimbal_system

[中文](#中文) | [English](#english)

## 中文

### 仓库定位

这是当前云台系统的总仓库（monorepo），用于统一管理上下位机主线、参考板级工程以及联调脚本。

仓库在 2026-03-19 完成归并后，默认应把这里视为当前项目的总入口；如果历史分仓文档和这里冲突，以当前仓库内主线代码和主线 README 为准。

### 当前推荐主线

当前建议维护、联调和继续演进的正式路径是：

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

说明：

- 上位机主线在 `dev-branch/`
- 下位机主线在 `Gimbal control/`
- 当前整机稳定通信主链仍然是 `UART`
- `USB CDC` 已完成基础双向通信与最小 pitch 控制验证，但还没有完全替代 UART 成为默认整机主链
- 截至 2026-03-26，`rm_gimbal_bridge + Gimbal control` 的 USB CDC 目标跟踪链路已完成一次成功联调，P 轴跟踪与诊断链路已补齐到和 Y 轴一致

### 仓库结构

```text
gimbal_system/
├── README.md                 # 本文件，总仓库入口
├── Gimbal control/           # STM32 下位机主线工程
├── dev-branch/               # ROS2 / TROS / RDK-X5 上位机主线工作区
├── tianboard_s/              # 参考/备用板级工程
├── tools/                    # 辅助工具与脚本
└── _git_migration_backup/    # monorepo 整理时保留的迁移备份
```

### 目录导航

#### `Gimbal control/`

当前唯一可信的下位机主线工程。

负责内容：

- 云台与电机控制
- IMU 与姿态解算
- CAN / DBUS / 视觉输入
- FreeRTOS 任务组织
- UART 主链与 USB CDC 迁移验证

建议入口：[`Gimbal control/README.md`](./Gimbal%20control/README.md)

#### `dev-branch/`

当前唯一可信的上位机主线工作区，主要面向 ROS2 Humble / TROS / RDK-X5。

负责内容：

- 海康工业相机接入
- YOLOv8 装甲板检测
- 检测结果桥接到下位机协议
- 板端实时可视化
- 部署、自启动与联调脚本

建议入口：[`dev-branch/README.md`](./dev-branch/README.md)

#### `tianboard_s/`

参考/备用板级工程，不是当前整机联调主线。

更适合用于：

- 查阅历史板级实现
- 对照 USB Device 组织方式
- 迁移时复用局部底层实现

建议入口：[`tianboard_s/README.md`](./tianboard_s/README.md)

### 当前常用入口

如果你是第一次进入这个总仓库，建议按下面顺序看：

1. 本 README：确认当前真实主线和仓库边界
2. [`dev-branch/README.md`](./dev-branch/README.md)：确认上位机运行链、节点和脚本入口
3. [`dev-branch/scripts/README.md`](./dev-branch/scripts/README.md)：确认 tmux / service / 部署脚本
4. [`Gimbal control/README.md`](./Gimbal%20control/README.md)：确认下位机通信主链和 USB CDC 迁移状态

### 当前状态结论

- `Gimbal control` 是当前下位机正式主线
- `dev-branch` 是当前上位机正式主线
- `tianboard_s` 仅作参考，不应继续承担主线开发
- 上下位机当前推荐联调组合是：`hik_camera + rm_armor_detection + rm_gimbal_bridge + Gimbal control`
- 当前板端画面观察和桥接下位机已拆成两组脚本 / service 入口，便于分别排障

### 文档使用原则

如果文档与代码状态不一致，建议按下面顺序判断：

1. 当前源码入口、launch、脚本与 service 文件
2. 各主线目录内 README
3. 交接文档、旧仓库文档、历史说明

### 当前已知边界

- monorepo 已建立，但历史目录和迁移痕迹仍然保留
- USB CDC 目前属于迁移验证路径，不是默认整机运行路径
- 一些目录保留是为了参考、对照和回溯，不代表当前应从这些目录继续开发

## English

### Repository Role

This is the current monorepo for the gimbal system. It keeps the active upper-level workspace, the active lower-level firmware, reference board-level code, and integration scripts in one place.

Since the repository consolidation on 2026-03-19, this workspace should be treated as the main project entry. If older split-repository documentation conflicts with the current codebase, trust the current mainline code and the README files inside this monorepo.

### Recommended Mainline

The current recommended maintenance and integration path is:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

Notes:

- the active upper-level mainline is in `dev-branch/`
- the active lower-level mainline is in `Gimbal control/`
- `UART` is still the formal whole-system communication path
- `USB CDC` has passed transport-level and minimal pitch-control validation, but it has not fully replaced UART as the default system mainline

### Repository Layout

```text
gimbal_system/
├── README.md                 # this file, monorepo entry
├── Gimbal control/           # STM32 lower-level firmware mainline
├── dev-branch/               # ROS2 / TROS / RDK-X5 upper-level workspace
├── tianboard_s/              # reference / backup board-level project
├── tools/                    # helper tools and scripts
└── _git_migration_backup/    # migration backup kept from monorepo consolidation
```

### Directory Guide

#### `Gimbal control/`

The only trusted lower-level firmware mainline.

Owns:

- gimbal and motor control
- IMU and attitude estimation
- CAN / DBUS / vision input
- FreeRTOS task organization
- UART mainline and USB CDC migration validation

Entry: [`Gimbal control/README.md`](./Gimbal%20control/README.md)

#### `dev-branch/`

The only trusted upper-level workspace mainline, primarily targeting ROS2 Humble, TROS, and RDK-X5.

Owns:

- Hikrobot industrial camera integration
- YOLOv8 armor detection
- detection-to-controller bridge logic
- on-device live visualization
- deployment, autostart, and integration scripts

Entry: [`dev-branch/README.md`](./dev-branch/README.md)

#### `tianboard_s/`

A reference / backup board-level project, not the active full-system mainline.

It is mainly useful for:

- checking historical board-level implementations
- comparing USB Device organization
- selectively reusing low-level implementation details during migration

Entry: [`tianboard_s/README.md`](./tianboard_s/README.md)

### Recommended Reading Order

If you are entering this monorepo for the first time, read in this order:

1. this README for the actual current mainline and repository boundaries
2. [`dev-branch/README.md`](./dev-branch/README.md) for the active upper-level runtime path
3. [`dev-branch/scripts/README.md`](./dev-branch/scripts/README.md) for tmux / service / deployment entry points
4. [`Gimbal control/README.md`](./Gimbal%20control/README.md) for lower-level communication status and USB CDC migration notes

### Current Project Conclusions

- `Gimbal control` is the formal lower-level mainline
- `dev-branch` is the formal upper-level mainline
- `tianboard_s` is reference-only and should not continue mainline feature work
- the recommended integration stack is `hik_camera + rm_armor_detection + rm_gimbal_bridge + Gimbal control`
- board-side visualization and lower-level bridge bring-up are intentionally split into separate script/service entry points for easier debugging

### Documentation Policy

If documentation and code disagree, prefer the following in order:

1. current source entry points, launch files, scripts, and service files
2. README files in the active mainline directories
3. handover notes, legacy repository docs, and historical writeups

### Known Boundaries

- the monorepo is established, but historical directories and migration traces are intentionally retained
- USB CDC is still a migration-validation path rather than the default whole-system runtime path
- some directories are kept for reference, comparison, or rollback context and are not the places to continue current mainline development
