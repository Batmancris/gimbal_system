# gimbal_system

[中文](#中文) | [English](#english)

## 中文

### 仓库定位

这是当前云台系统的总仓库（monorepo），用于统一管理：

- 上位机 ROS2 / TROS / RDK-X5 主线
- 下位机 STM32 云台控制主线
- 参考板级工程
- 部署、联调与迁移辅助文档

自 2026-03-19 仓库归并后，这里就是默认总入口。若历史分仓文档与当前代码状态冲突，以本仓库内主线代码和主线 README 为准。

### 当前推荐主线

当前建议维护、联调和继续演进的正式路径是：

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

当前结论：

- 上位机正式主线在 `dev-branch/`
- 下位机正式主线在 `Gimbal control/`
- 当前整机默认稳定通信主链仍然是 `UART`
- `USB CDC` 已完成基础双向通信、最小 pitch 验证和一次目标跟踪联调，但还未完全替代 UART
- `tianboard_s/` 仅作参考，不应继续承担主线开发

### 仓库结构

```text
gimbal_system/
├── README.md                 # 本文件，总仓库入口
├── Gimbal control/           # STM32 下位机主线工程
├── dev-branch/               # ROS2 / TROS / RDK-X5 上位机主线工作区
├── tianboard_s/              # 参考 / 备用板级工程
├── tools/                    # 辅助工具与诊断脚本
└── _git_migration_backup/    # monorepo 整理时保留的迁移备份
```

### 目录导航

#### `dev-branch/`

当前唯一可信的上位机主线工作区。

负责内容：

- 海康工业相机接入
- YOLOv8 / BPU 装甲板检测
- 检测结果桥接到下位机串口协议
- RDK-X5 板端实时可视化
- tmux / service / 部署脚本

入口：[`dev-branch/README.md`](./dev-branch/README.md)

#### `Gimbal control/`

当前唯一可信的下位机主线工程。

负责内容：

- 云台与电机控制
- IMU 与姿态解算
- CAN / DBUS / 视觉输入
- FreeRTOS 任务组织
- UART 主链与 USB CDC 迁移验证

入口：[`Gimbal control/README.md`](./Gimbal%20control/README.md)

#### `tianboard_s/`

参考 / 备用板级工程，不是当前整机联调主线。

更适合用于：

- 查阅历史板级实现
- 对照 USB Device 组织方式
- 迁移时复用局部底层实现

入口：[`tianboard_s/README.md`](./tianboard_s/README.md)

#### `tools/`

保存一些与主线运行分离的辅助诊断工具。

入口：[`tools/README.md`](./tools/README.md)

#### `_git_migration_backup/`

保存 monorepo 归并时的备份产物，不参与当前主线开发。

入口：[`_git_migration_backup/README.md`](./_git_migration_backup/README.md)

### 推荐阅读顺序

如果你是第一次进入这个总仓库，建议按下面顺序看：

1. 本 README：确认当前真实主线和仓库边界
2. [`dev-branch/README.md`](./dev-branch/README.md)：确认上位机运行链、主线包和历史目录定位
3. [`dev-branch/scripts/README.md`](./dev-branch/scripts/README.md)：确认 tmux / service / 部署脚本
4. [`Gimbal control/README.md`](./Gimbal%20control/README.md)：确认下位机通信主链和 USB CDC 迁移状态
5. [`tianboard_s/README.md`](./tianboard_s/README.md)：必要时查阅板级参考实现

### 文档整理原则

当前仓库的 README 已按“主入口 + 子目录入口 + 包级入口”整理。使用时建议遵循：

1. 先看当前目录下的 `README.md`
2. 如遇旧版 `Readme.md`、`README_cn.md` 一类历史文档，优先以新的双语 `README.md` 为准
3. 如文档和代码状态不一致，以当前源码入口、launch、脚本和 service 文件为准

### 当前可继续精简的结构

以下内容目前保留，但不属于应继续扩展的主线目录：

- `dev-branch/build/`
- `dev-branch/install/`
- `dev-branch/log/`
- `dev-branch/scripts/__pycache__/`
- 各目录下保留的 `.git.BAK-*`

文档层面的建议是：

- 继续把过时 README 收敛为跳转页或历史页
- 将构建产物和缓存目录视作可清理对象
- 将迁移备份统一集中在 `_git_migration_backup/`

### 当前已知边界

- monorepo 已建立，但历史目录和迁移痕迹仍然保留
- USB CDC 目前属于迁移验证路径，不是默认整机运行路径
- 一些目录保留是为了参考、对照和回溯，不代表当前应从这些目录继续开发

## English

### Repository Role

This is the current monorepo for the gimbal system. It keeps together:

- the active ROS2 / TROS / RDK-X5 upper-level workspace
- the active STM32 lower-level firmware mainline
- reference board-level projects
- deployment, integration, and migration helper documentation

Since the repository consolidation on 2026-03-19, this workspace should be treated as the default project entry. If older split-repository documents conflict with the current codebase, trust the in-repo mainline code and the current README files.

### Recommended Mainline

The recommended maintenance and integration path is:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

Current conclusions:

- the formal upper-level mainline is `dev-branch/`
- the formal lower-level mainline is `Gimbal control/`
- `UART` remains the default stable whole-system communication path
- `USB CDC` has passed transport validation, minimal pitch validation, and one end-to-end tracking test, but it has not fully replaced UART
- `tianboard_s/` is reference-only and should not carry mainline feature work

### Repository Layout

```text
gimbal_system/
├── README.md                 # this file, monorepo entry
├── Gimbal control/           # STM32 lower-level firmware mainline
├── dev-branch/               # ROS2 / TROS / RDK-X5 upper-level workspace
├── tianboard_s/              # reference / backup board-level project
├── tools/                    # helper tools and diagnostics
└── _git_migration_backup/    # migration backups kept from monorepo consolidation
```

### Directory Guide

#### `dev-branch/`

The only trusted upper-level workspace mainline.

Owns:

- Hikrobot industrial camera integration
- YOLOv8 / BPU armor detection
- bridge logic from detector output to lower-level protocol
- live on-device visualization on the RDK-X5
- tmux / service / deployment scripts

Entry: [`dev-branch/README.md`](./dev-branch/README.md)

#### `Gimbal control/`

The only trusted lower-level firmware mainline.

Owns:

- gimbal and motor control
- IMU and attitude estimation
- CAN / DBUS / vision input
- FreeRTOS task organization
- UART mainline and USB CDC migration validation

Entry: [`Gimbal control/README.md`](./Gimbal%20control/README.md)

#### `tianboard_s/`

A reference / backup board-level project, not the active full-system mainline.

Best used for:

- reviewing historical board-level implementations
- comparing USB Device organization
- selectively reusing low-level implementation details during migration

Entry: [`tianboard_s/README.md`](./tianboard_s/README.md)

#### `tools/`

Stores helper scripts that support debugging and integration but are not the main runtime path.

Entry: [`tools/README.md`](./tools/README.md)

#### `_git_migration_backup/`

Stores consolidation backups and migration artifacts. It is not part of the current development path.

Entry: [`_git_migration_backup/README.md`](./_git_migration_backup/README.md)

### Recommended Reading Order

If you are entering this monorepo for the first time, read in this order:

1. this README for current repository boundaries
2. [`dev-branch/README.md`](./dev-branch/README.md) for the upper-level mainline packages and retained legacy paths
3. [`dev-branch/scripts/README.md`](./dev-branch/scripts/README.md) for tmux / service / deployment entry points
4. [`Gimbal control/README.md`](./Gimbal%20control/README.md) for lower-level communication status and USB CDC migration notes
5. [`tianboard_s/README.md`](./tianboard_s/README.md) when reference board-level code is needed

### Documentation Policy

The repository now follows a layered README structure: root entry, directory entry, and package entry. Use this order:

1. read the `README.md` in the current directory first
2. if you encounter older `Readme.md` or `README_cn.md` files, prefer the new bilingual `README.md`
3. if documentation and code disagree, trust current source entry points, launch files, scripts, and service files

### Remaining Structure Cleanup Candidates

The following items are intentionally retained for now, but they are not active development targets:

- `dev-branch/build/`
- `dev-branch/install/`
- `dev-branch/log/`
- `dev-branch/scripts/__pycache__/`
- `.git.BAK-*` directories kept under historical subprojects

Documentation-side guidance:

- keep consolidating outdated READMEs into redirect or history pages
- treat build outputs and cache directories as cleanup candidates
- keep migration backups centralized under `_git_migration_backup/`

### Known Boundaries

- the monorepo is established, but historical directories and migration traces are intentionally retained
- USB CDC is still a migration-validation path rather than the default whole-system runtime path
- some directories are kept for reference, comparison, or rollback context and are not the places to continue current mainline development
