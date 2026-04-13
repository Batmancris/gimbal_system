# rm_armor_detection

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_armor_detection` 是当前项目在 RDK-X5 / TROS 环境下的 YOLOv8 装甲板检测主线包。

该包基于 D-Robotics 的 DNN / BPU 推理链路，负责模型推理、检测结果解析，并新增了用于板端桌面调试的实时可视化节点。

### 当前定位

- 检测主线：是
- 主要运行平台：RDK-X5
- 推理输入：共享内存图像 `/hbmem_img`
- 主要输出：`/dnn_node_sample`
- 可视化输入：`/image_raw` + `/dnn_node_sample`

### 当前能力

- 加载量化后的 YOLOv8 模型
- 在 BPU 上执行推理
- 解析输出框、关键点与类别
- 发布智能感知结果
- 通过 `rm_armor_detection_visualizer` 在 RDK-X5 屏幕上显示实时检测框
- 可选启用 Web 展示链路

### 当前联调参数

- 当前工程内检测分数阈值已放宽到更适合整链路联调的水平
- 当前视觉调试更偏向较暗、高对比度画面，用于突出 RM 装甲板灯条
- 桌面可视化节点主要用于确认“相机实时画面”和“检测结果”是否对齐

### 编译

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select rm_armor_detection
```

### 运行检测节点

```bash
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

如果需要网页显示：

```bash
export WEB_SHOW=TRUE
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

### 运行桌面可视化窗口

在 RDK-X5 图形桌面中执行：

```bash
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
ros2 run rm_armor_detection rm_armor_detection_visualizer
```

该窗口会：

- 订阅 `/image_raw`
- 订阅 `/dnn_node_sample`
- 在图上绘制检测框、类别、置信度和关键点

### 说明

- 当前 README 以项目内真实角色为准，不再单纯作为外部 demo 说明
- 当前主线更关注“检测结果接到桥接链路”，而不是单独展示 demo
- 新增的桌面可视化节点现在已经纳入 `start_autoaim_tmux.sh` 的默认板端三节点启动链
- 若只想单独排查桥接链路，可将可视化旁路保持开启，同时另行使用 `start_rm_bridge_tmux.sh` 拉起桥接节点

### 阅读建议

- 若你在看上位机总链路，请先读 `ros2_ws/README.md`
- 若你在看部署和启动入口，请看 `ros2_ws/README.md`
- 若你在看下发链路，请继续读 `rm_gimbal_bridge/README.md`

## English

### Overview

`rm_armor_detection` is the current YOLOv8 armor detection mainline package for the project on RDK-X5 / TROS.

It is built around the D-Robotics DNN / BPU inference pipeline and now also includes a live desktop visualizer for on-device debugging.

### Current Role

- Active detector mainline: yes
- Primary runtime platform: RDK-X5
- Inference input: shared-memory image topic `/hbmem_img`
- Primary output: `/dnn_node_sample`
- Visualizer input: `/image_raw` + `/dnn_node_sample`

### Current Capabilities

- loads the quantized YOLOv8 model
- runs inference on the BPU
- parses boxes, keypoints, and classes
- publishes perception results
- shows live overlay boxes on the RDK-X5 screen through `rm_armor_detection_visualizer`
- optionally enables web visualization

### Current Integration Notes

- the in-repo detector score threshold is currently relaxed for bring-up and end-to-end validation
- the present tuning intentionally favors darker, higher-contrast images so the RM armor light bars stand out more clearly
- the desktop visualizer is mainly used to confirm that the live camera stream and the detector output are aligned

### Build

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select rm_armor_detection
```

### Run The Detector

```bash
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

To enable web visualization:

```bash
export WEB_SHOW=TRUE
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

### Run The Desktop Visualizer

Run the following in the RDK-X5 graphical session:

```bash
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
ros2 run rm_armor_detection rm_armor_detection_visualizer
```

The visualizer window:

- subscribes to `/image_raw`
- subscribes to `/dnn_node_sample`
- draws boxes, class labels, confidence, and keypoints on the image

### Note

- This README now describes the package as it is used in the current project instead of only as an external demo
- The current mainline focus is the detector-to-bridge path, not just standalone demo presentation
- The desktop visualizer is now part of the default board-side three-node startup path in `start_autoaim_tmux.sh`
- If you only want to isolate the bridge path, you can keep the visualizer sidecar running and start the bridge separately via `start_rm_bridge_tmux.sh`

### Reading Guidance

- for the full upper-level chain, start with `ros2_ws/README.md`
- for deployment and startup entry points, read `ros2_ws/README.md`
- for the downstream target-delivery path, continue with `rm_gimbal_bridge/README.md`
