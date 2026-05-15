# ros2_ws

RDK X5 上运行的 ROS2/TROS 主工作区，负责相机采集、模型推理、目标发布、云台桥接和板端启动脚本。

## 当前默认链路

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> STM32 USB-CDC
```

当前状态: v1 headless fast_best stable baseline，跟随丝滑，偶发卡顿主要来自出框/丢检。

### 禁止依赖

- `/image_raw` 不用于主链路
- `rm_vis` 不用于主链路
- `publish_image_raw:=true` 不使用

## Packages

```text
src/
├── hik_camera/              [core] 海康相机驱动，发布 /hbmem_img
├── rm_bear_detection/       [core] bear YOLO 检测，发布 /bear_detection/targets
├── rm_gimbal_bridge/        [core] 云台桥接，订阅 targets，发送 USB-CDC
├── rm_interfaces/           [core] ROS2 自定义消息/服务定义
├── rm_utils/                [core] logger, math, heartbeat 工具库
├── rm_vehicle_detection/    [legacy/optional] vehicle 检测兼容
└── rm_armor_detection/      [legacy/optional] armor 检测兼容
```

## 在 RDK X5 上构建

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select \
  hik_camera rm_bear_detection rm_gimbal_bridge rm_interfaces rm_utils \
  --event-handlers console_direct+
source install/setup.bash
```

## 一键启动（推荐）

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

## 手动运行（仅调试用）

相机:

```bash
ros2 launch hik_camera hik_camera.launch.py
```

Bear 检测:

```bash
ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
  -p output_topic:=/bear_detection/targets
```

云台桥接:

```bash
ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args \
  -p input_topic:=/bear_detection/targets \
  -p allowed_target_types:="['bear']" \
  -p serial_port:=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

## 关键话题

| Topic | 类型 | 说明 |
|---|---|---|
| `/hbmem_img` | hbm_img_msgs/msg/HbmMsg1080P | TROS shared-memory NV12 图像，检测节点使用 |
| `/bear_detection/targets` | ai_msgs/msg/PerceptionTargets | bear 检测输出，桥接节点订阅 |

## 运行状态检查

```bash
ssh rdk-x5 "source /opt/tros/humble/setup.bash; source /home/sunrise/rm_ws/install/setup.bash; \
  ros2 node list; \
  ros2 topic info /hbmem_img; \
  ros2 topic info /bear_detection/targets; \
  fuser -v /dev/ttyACM0 2>/dev/null || true; \
  tmux -L autoaim ls 2>/dev/null || true"
```

## 性能采集

```bash
ssh rdk-x5 "cd /home/sunrise/rm_ws && DURATION=15 bash scripts/profile_fast_follow_link.sh"
```

## 文档

- 启动脚本: `ros2_ws/scripts/README.md`
- 云台桥接: `ros2_ws/src/rm_gimbal_bridge/README.md`
- bear 检测: `ros2_ws/src/rm_bear_detection/README.md`
- 体检报告: `docs/current_health_report.md`
