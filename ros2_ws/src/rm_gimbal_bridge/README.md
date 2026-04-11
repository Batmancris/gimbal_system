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
- 通过当前 serial 设备发送目标中心点；板端脚本主用 USB-CDC 设备路径
- 提供整机自动启动 launch
- 保留 USB CDC ping / pitch test 诊断程序

### 默认行为

- 默认订阅 `/dnn_node_sample`
- 节点代码默认串口 `/dev/ttyS1`，板端脚本通常覆盖为 USB-CDC by-id 设备
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

### 当前联调说明

- 当前工程内 `min_confidence` 已放宽，用于整链路 bring-up 时减少目标被桥接侧过早过滤
- `enemy_prefix` 仍然是颜色前缀过滤入口；留空时表示不过滤颜色前缀
- 该包仍然是当前整机自动瞄准联调时的上位机桥接主链；当前板端脚本通常把 serial 设备指向 USB-CDC
- 打开 `log_diag_feedback` 后，会在原始 `diag ...` 日志之外额外输出精简调参日志：

```text
tune err=(ex,ey) add=(yaw,pitch)mrad pitch_set=... current=(...) seq=...
```

其中：

- `err` 对应下位机视觉误差 `error_x / error_y`
- `add` 对应下位机视觉增量 `yaw_add_mrad / pitch_add_mrad`
- `pitch_set` 对应下位机当前 `pitch_set_mrad`

### 当前入口关系

- 上游检测主线：`rm_armor_detection`
- 下游通信主链：`USB-CDC serial device -> STM32 vision_input -> gimbal_task`
- 脚本入口：`scripts/start_rm_bridge_tmux.sh`

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
- sends the selected target center through the current serial device; board-side scripts usually point this at USB CDC
- provides a whole-system autoaim launch entry
- preserves USB CDC ping / pitch diagnostic utilities

### Default Behavior

- subscribes to `/dnn_node_sample`
- uses `/dev/ttyS1` as the node default, while board-side scripts usually override it with a USB-CDC by-id device
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

### Current Integration Notes

- the current in-repo `min_confidence` is relaxed for end-to-end bring-up so valid targets are less likely to be filtered too early by the bridge
- `enemy_prefix` is still the color-prefix filter; leaving it empty means no color-prefix filtering
- this package remains the active upper-to-lower bridge for full-system auto-aim integration; current board-side scripts usually route its serial device to USB CDC
- when `log_diag_feedback` is enabled, the bridge also prints a compact tuning log:

```text
tune err=(ex,ey) add=(yaw,pitch)mrad pitch_set=... current=(...) seq=...
```

where:

- `err` maps to lower-level `error_x / error_y`
- `add` maps to lower-level `yaw_add_mrad / pitch_add_mrad`
- `pitch_set` maps to lower-level `pitch_set_mrad`

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

## Current Note

- USB CDC validation did not replace the role of `rm_gimbal_bridge`
- This package remains the current bridge mainline for integrated auto-aim runs

### Current Entry Relationships

- upstream detector mainline: `rm_armor_detection`
- downstream default communication path: `USB-CDC serial device -> STM32 vision_input -> gimbal_task`
- script entry: `scripts/start_rm_bridge_tmux.sh`
