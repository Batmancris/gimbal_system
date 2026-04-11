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
- RDK-X5 桌面实时可视化
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

当前还保留一条板端桌面可视化旁路，用于直接观察推理结果：

```text
image_raw + /dnn_node_sample
  -> rm_armor_detection_visualizer
  -> X11 window on RDK-X5
```

其中：

- `hik_camera` 是当前相机主线
- `rm_armor_detection` 是当前 RDK-X5 / BPU 检测主线
- `rm_armor_detection_visualizer` 是当前实时检测框可视化窗口
- `rm_gimbal_bridge` 是当前上下位机桥接主线

### 当前目录定位

- `hik_camera/`
  当前海康工业相机主线驱动
- `rm_armor_detection/`
  当前 YOLOv8 装甲板检测主线，同时包含桌面可视化节点
- `rm_gimbal_bridge/`
  当前串口桥接主线，同时保留 USB CDC 诊断测试程序
- `rm_interfaces/`
  自定义消息与服务定义
- `rm_utils/`
  公共数学、日志与工具库
- `scripts/`
  部署、自启动、远端运行辅助脚本

### 产品化迁移说明

当前 `dev-branch/` 仍然是实际 ROS2 主线工作区，但它的目录命名不适合作为长期产品仓命名。

当前仓库已经新增：

- 根目录 `ros2_ws/`
- 根目录 `scripts/`
- 根目录 `docs/`

这表示当前推荐策略是：

1. 保持 `dev-branch/` 继续作为真实运行入口
2. 把新文档、新脚本和迁移约定收敛到根目录产品化结构
3. 等部署脚本和远端路径统一后，再分步迁移到 `ros2_ws/src/`

### 历史 / 备用目录

以下目录保留，但当前不作为主运行路径：

- `armor_detector/`：传统视觉装甲板识别实现，保留为历史算法参考
- `rm_camera_driver/`：旧版 Daheng 相机驱动，保留为历史参考
- `rm_camera_driver_nv12/`：面向 NV12 / 共享内存路径的旧相机驱动，保留为实验参考
- `ultralytics-8.2.103/`：本地保留的第三方 YOLO 代码副本，主要用于训练、调参或源码参考，不是当前运行主链
- `work_handover/`：交接和汇报性质文档，信息可参考，但不应覆盖当前主线 README

### 文档入口

建议按下面顺序阅读：

1. 本 README：确认当前上位机主线边界
2. `scripts/README.md`：确认部署、tmux、service 和 USB CDC 诊断脚本入口
3. 各包目录内的 `README.md`：确认单包职责、输入输出和运行方式

当前文档已经统一为“双语 `README.md` 为主，历史 README 为辅”的结构。

如果你看到下面这些历史文件：

- `Readme.md`
- `Readme_zh.md`
- `README_cn.md`

请优先以新的 `README.md` 为准。

### 当前进度

- 海康相机实时采集链已跑通
- YOLOv8 检测链已在 RDK-X5 上稳定运行
- 串口桥接已可将检测结果编码为当前下位机协议
- 已补充桌面实时可视化窗口，可直接显示检测框、类别和置信度
- 一键启动链已经有可维护入口

### 当前联调说明

- 当前 RM 装甲板识别参数倾向于较暗、高对比度画面，以突出灯条
- 当前主线联调时使用了较高曝光和较宽松的检测阈值，便于确认整链路是否工作
- `scripts/start_autoaim_tmux.sh` 负责当前板端三节点启动：`hik_camera + rm_armor_detection + rm_armor_detection_visualizer`
- `scripts/start_rm_bridge_tmux.sh` 负责桥接节点单独拉起：`rm_gimbal_bridge`
- 因此当前“板端画面观察”和“桥接到下位机”已经拆成两组 tmux / service 入口，便于分别排查

### 当前已知边界

- 当前正式主链仍以 UART 为准
- USB CDC 在上位机侧仍属于迁移配合阶段，不是默认运行路径
- 历史目录较多，使用前请先确认是否属于当前主线

### 当前可继续精简的结构

以下目录或文件属于“保留但不应继续扩张”的对象：

- `build/`
- `install/`
- `log/`
- `scripts/__pycache__/`
- `.git.BAK-20260319/`
- `Readme.md`
- `Readme_zh.md`

文档层面的整理建议是：

- 历史 README 收敛为跳转页
- 生成目录继续视作构建产物，不参与长期文档入口
- 第三方源码副本与交接文档继续保留，但要明确标注“非主线”

## English

### Overview

`dev-branch` is the current upper-level workspace of the project, mainly targeting ROS2 Humble, TROS, and RDK-X5.

It is responsible for:

- industrial camera integration
- image publication and shared-memory transport
- armor detection
- conversion of detection results into controller-facing protocol data
- live desktop visualization on the RDK-X5
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

The workspace also keeps a board-side desktop visualization sidecar for directly inspecting detector output:

```text
image_raw + /dnn_node_sample
  -> rm_armor_detection_visualizer
  -> X11 window on RDK-X5
```

In this path:

- `hik_camera` is the active camera mainline
- `rm_armor_detection` is the active RDK-X5 / BPU detector mainline
- `rm_armor_detection_visualizer` is the live on-device overlay window
- `rm_gimbal_bridge` is the active upper-to-lower bridge mainline

### Directory Roles

- `hik_camera/`
  Current Hikrobot industrial camera mainline driver
- `rm_armor_detection/`
  Current YOLOv8 armor detection mainline, including the live visualizer node
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

### Documentation Entry

Recommended reading order:

1. this README for the upper-level mainline boundary
2. `scripts/README.md` for deployment, tmux, service, and USB CDC diagnostic entry points
3. package-level `README.md` files for package-specific roles, I/O, and run commands

The documentation now follows a simple rule: bilingual `README.md` files are the source of truth, while historical README variants are secondary.

If you encounter these older files:

- `Readme.md`
- `Readme_zh.md`
- `README_cn.md`

prefer the new `README.md` in the same directory.

### Current Progress

- The Hikrobot camera acquisition path is running on the RDK-X5
- The YOLOv8 detection path is running on the BPU
- The serial bridge can already encode detections into the current lower-level protocol
- A live desktop visualizer now shows boxes, classes, and confidence directly on the RDK-X5 screen
- A maintainable launch/deployment path is present

### Current Integration Notes

- The present RM armor-detection tuning intentionally favors darker, higher-contrast images so the light bars stand out more clearly
- The current validation setup uses a higher exposure and looser detection thresholds to confirm the whole chain end-to-end
- `scripts/start_autoaim_tmux.sh` starts the current board-side three-node path: `hik_camera + rm_armor_detection + rm_armor_detection_visualizer`
- `scripts/start_rm_bridge_tmux.sh` starts the bridge node separately: `rm_gimbal_bridge`
- This means board-side visual inspection and lower-level bridge bring-up currently use two separate tmux / service entry points for easier debugging

### Current Boundaries

- UART is still the formal whole-system communication mainline
- USB CDC is still in migration support mode on the upper-level side
- `scripts/usb_cdc_pitch_control_test.py` is now available for minimal `/dev/ttyACM0` pitch validation only
- that USB CDC test script does not replace the current UART-based `rm_gimbal_bridge` mainline
- There are multiple historical directories, so verify whether a module belongs to the current mainline before using it

### Remaining Structure Cleanup Candidates

The following items are retained but should not keep growing as documentation entry points:

- `build/`
- `install/`
- `log/`
- `scripts/__pycache__/`
- `.git.BAK-20260319/`
- `Readme.md`
- `Readme_zh.md`

Documentation-side cleanup guidance:

- keep historical READMEs as redirect pages only
- keep generated directories out of the long-term navigation path
- keep third-party copies and handover docs clearly labeled as non-mainline references
