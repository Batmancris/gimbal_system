# TianAim / gimbal_system

[中文](#中文) | [English](#english)

## 中文

### 产品定位

当前仓库仍沿用历史仓库名 `gimbal_system`，但主线产品化方向已经明确收敛为 `TianAim`。

本仓库用于承载 Tianbot 云台视觉控制系统的完整产品主线，包括：

- 上位机 ROS2 / TROS / RDK-X5 感知与桥接链路
- 下位机 STM32 云台控制固件
- UART 主通信链路与 USB CDC 迁移验证链路
- 后续车辆图像数据采集、标注、训练与评估资产
- 面向人类开发者与 AI agent 的统一文档入口

当前策略不是一次性大搬家，而是先把仓库升级为“可维护的产品仓”，在不破坏现有运行链的前提下补齐产品化目录、命名规则和迁移说明。

### 当前稳定主链

当前推荐维护和联调的整机主链为：

```text
Hik camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

通信结论：

- `UART` 仍是当前默认稳定链路
- `USB CDC` 已经打通基础双向通信和最小控制验证，但仍属于迁移验证链
- 下位机视觉控制当前以 `P/PI 兼容框架` 为主线，后续再扩展预测与更复杂控制算法

### 当前仓库现状与过渡结构

当前真实主线代码仍位于：

- ROS2 上位机主线：`dev-branch/`
- STM32 固件主线：`Gimbal control/`

本次已经新增一层更产品化的过渡结构，用于统一导航和后续迁移：

```text
.
├── README.md
├── AGENTS.md
├── docs/                  # 架构、审计、分类、backlog
├── firmware/              # 固件主线迁移说明与未来命名锚点
├── ros2_ws/               # ROS2 工作区迁移说明与未来 src 锚点
├── datasets/              # 数据采集/标注/切分骨架
├── models/                # 模型与导出物约定
├── tools/                 # 采集/标注/训练/评估/诊断工具
├── scripts/               # 顶层统一入口脚本
├── archive/               # 历史目录归档说明
├── dev-branch/            # 当前 ROS2 主线物理位置
├── Gimbal control/        # 当前固件主线物理位置
├── tianboard_s/           # 参考板级工程
└── _git_migration_backup/ # 迁移备份
```

这里的关键点是：

- 已把 `ros2_ws/`、`firmware/`、`archive/`、`datasets/`、`models/`、顶层 `scripts/` 补成正式入口
- 暂未粗暴移动 `dev-branch/` 与 `Gimbal control/`，避免破坏现有脚本、Makefile、launch、远端部署路径
- 未来建议逐步把 `dev-branch/` 收敛到 `ros2_ws/src/`，把 `Gimbal control/` 收敛到 `firmware/stm32_gimbal_control/`

### 主链路说明

系统主链路建议按下面理解：

```text
Hik camera
  -> image_raw / hbmem_img
  -> rm_armor_detection
  -> /dnn_node_sample
  -> rm_gimbal_bridge
  -> UART serial frame
  -> vision_input.c
  -> target_state.c
  -> gimbal_task.c
  -> yaw / pitch control
```

当前关键节点位置：

- 相机驱动：`dev-branch/hik_camera`
- 检测节点：`dev-branch/rm_armor_detection`
- 检测可视化：`dev-branch/rm_armor_detection/src/visualizer.cpp`
- 串口桥接：`dev-branch/rm_gimbal_bridge/src/serial_bridge_node.cpp`
- 下位机视觉输入：`Gimbal control/Src/vision_input.c`
- 下位机目标状态：`Gimbal control/Src/target_state.c`
- 下位机控制主逻辑：`Gimbal control/Src/gimbal_task.c`

### 快速开始

#### 1. 阅读顺序

1. `README.md`
2. `docs/repo_audit.md`
3. `dev-branch/README.md`
4. `Gimbal control/README.md`
5. 包级 README 或工具 README

#### 2. ROS2 主线构建

```bash
bash scripts/build_ros2_mainline.sh
```

等价于：

```bash
cd dev-branch
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
```

#### 3. 固件主线构建

```bash
bash scripts/build_firmware_mainline.sh
```

等价于：

```bash
make -C "Gimbal control"
```

#### 4. 桥接节点运行

```bash
bash scripts/run_ros2_bridge.sh
```

### 环境依赖

ROS2 / 上位机：

- ROS2 Humble / TROS
- RDK-X5 运行环境
- Hikrobot / Hikvision 相机 SDK
- `colcon`, `ament_cmake`

固件 / 下位机：

- `arm-none-eabi-gcc`
- STM32F407 对应 HAL / CMSIS 依赖
- GNU Make

数据采集与训练工具：

- Python 3.10+
- 建议使用 `venv`
- 真实海康采集接入时，需要对应 Python SDK 或 C API 封装

### 构建与运行入口

当前可确认的主线入口如下：

- ROS2 构建：`dev-branch/` 下 `colcon build`
- ROS2 运行：`dev-branch/scripts/start_autoaim_tmux.sh`
- 桥接运行：`dev-branch/scripts/start_rm_bridge_tmux.sh`
- 固件构建：`Gimbal control/Makefile`
- USB CDC 最小测试：`dev-branch/scripts/usb_cdc_pitch_control_test.py`

本次新增的统一入口：

- `scripts/build_ros2_mainline.sh`
- `scripts/build_firmware_mainline.sh`
- `scripts/run_ros2_bridge.sh`

### 目录说明

- `dev-branch/`
  当前真实 ROS2 主线工作区，后续建议迁移到 `ros2_ws/src/`
- `Gimbal control/`
  当前真实 STM32 固件主线，后续建议迁移到 `firmware/stm32_gimbal_control/`
- `docs/`
  审计、架构、分类、迁移和 backlog 文档
- `datasets/`
  采集、标注、切分与 manifest 骨架
- `models/`
  模型权重、导出模型与实验记录约定
- `tools/`
  数据采集、标注、训练、评估与诊断工具
- `scripts/`
  顶层统一命令入口
- `archive/`
  历史和参考目录的归档说明，不直接替代真实目录

### 当前稳定链路与实验链路

稳定主线：

- `hik_camera -> rm_armor_detection -> rm_gimbal_bridge -> UART -> Gimbal control`

迁移中链路：

- `rm_gimbal_bridge -> USB CDC -> Gimbal control`

实验扩展方向：

- 控制算法预测稳定化
- `Kalman + PI` 实验控制器
- `LQR` 实验控制器
- 数据采集、训练和模型迭代闭环

### 数据采集链路

本次已经新增最小数据链路骨架：

```text
datasets/
├── raw/
├── labeled/
├── splits/
└── manifests/

tools/
├── capture/
├── labeling/
├── training/
└── evaluation/
```

当前最小可执行采集脚本：

```bash
python3 tools/capture/capture_session.py --config tools/capture/config.yaml
```

该脚本当前支持：

- 创建采集 session 目录
- 输出 session manifest
- 可选把已有图片导入为一组原始采集数据
- 预留真实海康 SDK 适配层接口

### 命名与迁移原则

本仓库未来优先采用 `TianAim` 产品名，但本次只做“命名就绪”和“小范围统一”，不做大规模破坏性 rename。

建议逐步迁移目标：

- 仓库产品名：`TianAim`
- 顶层工作区：`ros2_ws/`
- 固件目录：`firmware/stm32_gimbal_control/`
- 新增工具/脚本/数据目录使用无空格、全小写、下划线风格
- 新增 Python 工具与文档优先采用 `tianaim_*` 语义

### Roadmap

- P0：完成仓库产品化入口、数据采集骨架、AI-agent 导航文档
- P1：把 `dev-branch/` 迁移到 `ros2_ws/src/` 的可审查方案落地
- P1：把 `Gimbal control/` 迁移到 `firmware/stm32_gimbal_control/` 的兼容方案落地
- P1：接入真实海康采集适配器，输出规范化 manifest
- P2：建立标注、训练、评估闭环
- P2：引入 `P/PI + prediction` 控制主线增强
- P3：评估 `Kalman`、`alpha-beta`、`LQR` 实验控制线

## English

### Product Position

The repository still keeps the historical name `gimbal_system`, but the product-facing direction is now converging on `TianAim`.

This repository is the integrated product workspace for the Tianbot gimbal vision-control stack, including:

- the ROS2 / TROS / RDK-X5 upper-level pipeline
- the STM32 lower-level firmware
- UART mainline communication and USB CDC migration work
- future dataset capture, labeling, training, and evaluation assets
- unified developer and AI-agent navigation

The current strategy is not a big-bang move. Instead, we keep the verified runtime chain intact while adding product-ready structure, naming guidance, and transition entry points.

### Current Verified Pipeline

```text
Hik camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

### Transition Layout

The physical source-of-truth paths are still:

- ROS2 mainline: `dev-branch/`
- firmware mainline: `Gimbal control/`

This update adds a product-oriented transition layer:

- `docs/`
- `firmware/`
- `ros2_ws/`
- `datasets/`
- `models/`
- top-level `scripts/`
- `archive/`

These are navigation and migration anchors. They do not yet replace the active runtime paths.

### Main Runtime Chain

```text
Hik camera
  -> image_raw / hbmem_img
  -> rm_armor_detection
  -> /dnn_node_sample
  -> rm_gimbal_bridge
  -> UART serial frame
  -> vision_input.c
  -> target_state.c
  -> gimbal_task.c
  -> yaw / pitch control
```

### Quick Start

Build the ROS2 mainline:

```bash
bash scripts/build_ros2_mainline.sh
```

Build the firmware mainline:

```bash
bash scripts/build_firmware_mainline.sh
```

Run the bridge node:

```bash
bash scripts/run_ros2_bridge.sh
```

### Data Capture Skeleton

This repository now includes:

- `datasets/raw`
- `datasets/labeled`
- `datasets/splits`
- `datasets/manifests`
- `tools/capture`
- `tools/labeling`
- `tools/training`
- `tools/evaluation`

The minimal capture entry is:

```bash
python3 tools/capture/capture_session.py --config tools/capture/config.yaml
```

### Naming Direction

The preferred future product name is `TianAim`, but this update intentionally avoids a risky full rename. New top-level additions use stable, lowercase, no-space naming and prepare the path toward:

- `ros2_ws/`
- `firmware/stm32_gimbal_control/`
- `tianaim_*` for new tools and integrations

### More Detail

- audit: `docs/repo_audit.md`
- structure and migration notes: `docs/architecture.md`
- backlog: `docs/backlog.md`
