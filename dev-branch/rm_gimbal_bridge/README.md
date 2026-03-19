# rm_gimbal_bridge

`rm_gimbal_bridge` 用来把上位机检测节点发布的 `ai_msgs/msg/PerceptionTargets` 转成下位机当前使用的 8 字节串口协议：

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

默认逻辑：

- 订阅 `/dnn_node_sample`
- 从所有目标里选取最靠近图像中心的目标
- 将目标框中心点打包后发到 `/dev/ttyS1`
- 默认波特率 `921600`

## 主要参数

- `input_topic`: 检测结果话题
- `serial_port`: 串口设备名
- `baud_rate`: 串口波特率
- `image_center_x`, `image_center_y`: 图像中心
- `min_confidence`: 最小置信度
- `enemy_prefix`: 按类别名前缀筛选目标，例如 `blue_` 或 `red_`
- `selection_mode`: `closest` 或 `highest_confidence`

## 运行

```bash
colcon build --packages-select rm_gimbal_bridge
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py
```

## 整机启动

如果你使用的是 RDK X5 + USB/V4L2 相机 + YOLO 检测 + 串口桥接，可以直接启动：

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py
```

可选参数：

```bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py serial_port:=/dev/ttyS1 enemy_prefix:=blue_ camera_device:=/dev/video0
```

## Current Note

- The recent USB CDC validation work did not modify `rm_gimbal_bridge`
- This package remains the current UART mainline bridge for integrated auto-aim runs
