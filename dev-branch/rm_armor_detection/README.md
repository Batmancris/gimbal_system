# rm_armor_detection

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_armor_detection` 是当前项目在 RDK-X5 / TROS 环境下的 YOLOv8 装甲板检测主线包。

该包基于 D-Robotics 的 DNN / BPU 推理链路，订阅图像数据后输出检测结果，并可选接入网页可视化链路。

### 当前定位

- 检测主线：是
- 主要运行平台：RDK-X5
- 主要输入：共享内存图像 `/hbmem_img`
- 主要输出：`/dnn_node_sample`

### 当前能力

- 加载量化后的 YOLOv8 模型
- 在 BPU 上执行推理
- 解析输出框、关键点与类别
- 发布智能感知结果
- 可选启用 Web 展示链路

### 编译

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select rm_armor_detection
```

### 运行

```bash
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

如果需要网页显示：

```bash
export WEB_SHOW=TRUE
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

### 说明

- 当前 README 以项目内真实角色为准，不再单纯作为外部 demo 说明
- 当前主线更关注“检测结果接到桥接链路”，而不是单独展示 demo

## English

### Overview

`rm_armor_detection` is the current YOLOv8 armor detection mainline package for the project on RDK-X5 / TROS.

It is built around the D-Robotics DNN / BPU inference pipeline, consumes image input, publishes detection outputs, and can optionally enable a web visualization path.

### Current Role

- Active detector mainline: yes
- Primary runtime platform: RDK-X5
- Primary input: shared-memory image topic `/hbmem_img`
- Primary output: `/dnn_node_sample`

### Current Capabilities

- loads the quantized YOLOv8 model
- runs inference on the BPU
- parses boxes, keypoints, and classes
- publishes perception results
- optionally enables web visualization

### Build

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select rm_armor_detection
```

### Run

```bash
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

To enable web visualization:

```bash
export WEB_SHOW=TRUE
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

### Note

- This README now describes the package as it is used in the current project instead of only as an external demo
- The current mainline focus is the detector-to-bridge path, not just standalone demo presentation
