# ros2_ws

`ros2_ws` 是 RDK X5 上运行的 ROS2/TROS 主工作区，负责相机采集、模型推理、目标发布、云台桥接和板端启动脚本。

## 当前默认链路

当前默认调试目标是 bear，低速跟随已经比较丝滑，高速跟随仍然存在跟不上的情况：

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> STM32 USB-CDC
```

保留的兼容链路：

```text
hik_camera -> /hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets
hik_camera -> /hbmem_img -> rm_armor_detection -> /dnn_node_sample
```

## Packages

```text
src/
|-- hik_camera
|-- rm_armor_detection
|-- rm_bear_detection
|-- rm_vehicle_detection
|-- rm_gimbal_bridge
|-- rm_interfaces
`-- rm_utils
```

## 在 RDK X5 上构建

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select \
  hik_camera rm_armor_detection rm_vehicle_detection rm_bear_detection rm_gimbal_bridge \
  --event-handlers console_direct+
source install/setup.bash
```

## 直接运行

相机：

```bash
ros2 launch hik_camera hik_camera.launch.py
```

Bear 检测：

```bash
ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
  -p output_topic:=/bear_detection/targets
```

Vehicle 检测：

```bash
ros2 run rm_vehicle_detection rm_vehicle_detection_node --ros-args \
  -p output_topic:=/vehicle_detection/targets
```

云台桥接：

```bash
ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args \
  -p input_topic:=/bear_detection/targets \
  -p allowed_target_types:="['bear']" \
  -p serial_port:=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

## tmux 启动方式

板端脚本位于 `ros2_ws/scripts/`，部署到 RDK X5 后通常在 `/home/sunrise/rm_ws/src/scripts/`。

启动相机、检测和可视化：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash

export DETECTOR_TYPE=bear
export DETECTOR_TOPIC=/bear_detection/targets
bash src/scripts/start_autoaim_tmux.sh
```

启动桥接：

```bash
export DETECTOR_TOPIC=/bear_detection/targets
export ALLOWED_TARGET_TYPES=bear
bash src/scripts/start_rm_bridge_tmux.sh
```

检查状态：

```bash
bash src/scripts/check_autoaim_topics.sh
tmux -L autoaim ls
tmux -L autoaim capture-pane -pt hik_cam
tmux -L autoaim capture-pane -pt rm_det
tmux -L bridge capture-pane -pt rm_bridge
```

停止：

```bash
bash src/scripts/desktop_stop_full_stack.sh
```

## 关键话题

- `/image_raw`：普通 ROS 图像，给可视化使用。
- `/hbmem_img`：TROS/Hobot shared-memory NV12 图像，给检测节点使用。
- `/bear_detection/targets`：当前默认 bear 检测输出。
- `/vehicle_detection/targets`：vehicle 检测输出。
- `/dnn_node_sample`：armor 兼容输出或共享检测输出。

## 调试建议

没有相机输出：

```bash
ros2 topic info /hbmem_img -v
tmux -L autoaim capture-pane -pt hik_cam
```

没有检测输出：

```bash
ros2 topic info /bear_detection/targets -v
tmux -L autoaim capture-pane -pt rm_det
```

桥接没有发送：

```bash
ros2 topic echo /bear_detection/targets --once
tmux -L bridge capture-pane -pt rm_bridge
ls -l /dev/serial/by-id/
```

高速跟不上时先打开日志：

```bash
export BRIDGE_LOG_DIAG_FEEDBACK=true
export BEAR_LOG_DETECTIONS=true
bash src/scripts/start_autoaim_tmux.sh
bash src/scripts/start_rm_bridge_tmux.sh
```

## 当前限制

低速跟随已经验证为顺滑；高速跟随还没有完全解决。不要只看 ROS 节点是否运行就判断“高速完成”，需要同时看目标误差、桥接发送频率、下位机 `yaw_add_mrad/pitch_add_mrad` 和云台实际响应。

