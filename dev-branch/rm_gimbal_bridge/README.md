# rm_gimbal_bridge

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_gimbal_bridge` 是当前上位机到下位机的桥接主线包。

它会把检测节点发布的 `ai_msgs/msg/PerceptionTargets` 转换为当前下位机使用的 8 字节串口协议：

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

### 当前职责

- 订阅检测结果
- 从候选目标中选择一个发送目标
- 通过串口发送目标中心点
- 提供整机自动启动 launch
- 保留 USB CDC ping / pitch test 诊断程序

### 默认行为

- 默认订阅 `/dnn_node_sample`
- 默认串口 `/dev/ttyS1`
- 默认波特率 `921600`
- 默认选择策略为最接近图像中心的目标

### 主要参数

- `input_topic`
- `serial_port`
- `baud_rate`
- `image_width`
- `image_height`
- `image_center_x`
- `image_center_y`
- `min_confidence`
- `enemy_prefix`
- `selection_mode`

### 运行

```bash
colcon build --packages-select rm_gimbal_bridge
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py
```

### 整机启动

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py
```

示例：

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py serial_port:=/dev/ttyS1 enemy_prefix:=blue_
```

## English

### Overview

`rm_gimbal_bridge` is the current upper-to-lower communication bridge mainline package.

It converts `ai_msgs/msg/PerceptionTargets` into the 8-byte serial protocol currently used by the lower-level controller:

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

### Current Responsibilities

- subscribes to detector outputs
- selects one target from all candidates
- sends the selected target center through serial
- provides a whole-system autoaim launch entry
- preserves USB CDC ping / pitch diagnostic utilities

### Default Behavior

- subscribes to `/dnn_node_sample`
- uses `/dev/ttyS1` by default
- uses `921600` baud by default
- selects the target closest to image center by default

### Main Parameters

- `input_topic`
- `serial_port`
- `baud_rate`
- `image_width`
- `image_height`
- `image_center_x`
- `image_center_y`
- `min_confidence`
- `enemy_prefix`
- `selection_mode`

### Run

```bash
colcon build --packages-select rm_gimbal_bridge
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py
```

### Whole-System Launch

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py
```

Example:

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py serial_port:=/dev/ttyS1 enemy_prefix:=blue_
```
