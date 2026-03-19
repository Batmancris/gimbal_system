# dev-branch

[中文](#中文) | [English](#english)

## 中文

### 简介

`dev-branch` 是当前项目的上位机主线工作区，运行环境以 ROS2 Humble / TROS / RDK-X5 为主。

它负责以下能力：

- 工业相机接入
- 图像发布与共享内存链路
- 装甲板检测
- 检测结果到下位机协议的桥接
- 部署、自启动、联调脚本

### 当前推荐主线

当前建议维护和联调的主路径为：

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

其中：

- `hik_camera` 是当前相机主线
- `rm_armor_detection` 是当前 RDK-X5 / BPU 检测主线
- `rm_gimbal_bridge` 是当前上下位机桥接主线

### 当前目录定位

- `hik_camera/`
  当前海康工业相机主线驱动
- `rm_armor_detection/`
  当前 YOLOv8 装甲板检测主线
- `rm_gimbal_bridge/`
  当前串口桥接主线，同时保留 USB CDC 诊断测试程序
- `rm_interfaces/`
  自定义消息与服务定义
- `rm_utils/`
  公共数学、日志与工具库
- `scripts/`
  部署、自启动、远端运行辅助脚本

### 历史 / 备用目录

以下目录保留，但当前不作为主运行路径：

- `armor_detector/`：传统视觉装甲板识别实现，保留为历史算法参考
- `rm_camera_driver/`：旧版 Daheng 相机驱动，保留为历史参考
- `rm_camera_driver_nv12/`：面向 NV12 / 共享内存路径的旧相机驱动，保留为实验参考
- `ultralytics-8.2.103/`：本地保留的第三方 YOLO 代码副本，主要用于训练、调参或源码参考，不是当前运行主链
- `work_handover/`：交接和汇报性质文档，信息可参考，但不应覆盖当前主线 README

### 当前进度

- 上位机图像采集链已具备主线
- YOLOv8 检测链已具备可运行实现
- 串口桥接已可将检测结果编码为当前下位机协议
- 一键启动链已经有可维护入口

### 当前已知边界

- 当前正式主链仍以 UART 为准
- USB CDC 在上位机侧仍属于迁移配合阶段，不是默认运行路径
- 历史目录较多，使用前请先确认是否属于当前主线

## English

### Overview

`dev-branch` is the current upper-level workspace of the project, mainly targeting ROS2 Humble, TROS, and RDK-X5.

It is responsible for:

- industrial camera integration
- image publication and shared-memory transport
- armor detection
- conversion of detection results into controller-facing protocol data
- deployment, autostart, and integration scripts

### Recommended Mainline

The current recommended integration path is:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

In this path:

- `hik_camera` is the active camera mainline
- `rm_armor_detection` is the active RDK-X5 / BPU detector mainline
- `rm_gimbal_bridge` is the active upper-to-lower bridge mainline

### Directory Roles

- `hik_camera/`
  Current Hikrobot industrial camera mainline driver
- `rm_armor_detection/`
  Current YOLOv8 armor detection mainline
- `rm_gimbal_bridge/`
  Current serial bridge mainline, with USB CDC diagnostic utilities preserved
- `rm_interfaces/`
  Custom messages and service definitions
- `rm_utils/`
  Shared math, logging, and utility library
- `scripts/`
  Deployment, autostart, and remote run helper scripts

### Historical / Backup Paths

The following directories are kept, but they are not the current default runtime path:

- `armor_detector/`: traditional CV armor detector kept as historical algorithm reference
- `rm_camera_driver/`: older Daheng camera driver kept as legacy reference
- `rm_camera_driver_nv12/`: older NV12/shared-memory camera path kept as an experimental reference
- `ultralytics-8.2.103/`: local third-party YOLO source tree kept for training, tuning, or source reference, not the active runtime path
- `work_handover/`: handover/reporting documents that are useful for context, but should not override the current mainline README files

### Current Progress

- A workable image acquisition mainline exists
- A runnable YOLOv8 detection path exists
- The serial bridge can already encode detections into the current lower-level protocol
- A maintainable launch/deployment path is present

### Current Boundaries

- UART is still the formal whole-system communication mainline
- USB CDC is still in migration support mode on the upper-level side
- `scripts/usb_cdc_pitch_control_test.py` is now available for minimal `/dev/ttyACM0` pitch validation only
- that USB CDC test script does not replace the current UART-based `rm_gimbal_bridge` mainline
- There are multiple historical directories, so verify whether a module belongs to the current mainline before using it
