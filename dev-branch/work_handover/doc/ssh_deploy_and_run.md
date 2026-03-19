# RDK X5 远程部署与运行

## 1. 推荐的软件链路

上位机节点链路：

1. `hik_camera_node`
2. `rm_armor_detection`
3. `rm_gimbal_bridge_node`

数据流：

1. 海康相机发布 `/image_raw`
2. `hobot_codec` 转换 `/image_raw -> /hbmem_img`
3. 检测节点订阅 `/hbmem_img`，发布 `/dnn_node_sample`
4. 串口桥接节点订阅 `/dnn_node_sample`
5. 桥接节点通过 `/dev/ttyS1` 向 C 板发送 8 字节协议

下位机协议：

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

## 2. 首次 SSH 连接

假设 RDK X5 的 IP 是 `192.168.1.10`：

```bash
ssh root@192.168.1.10
```

如果你本机已经可以 SSH，上面一步完成后，就可以直接用脚本部署。

## 3. 从电脑部署到 RDK X5

在电脑本地执行：

```bash
cd /home/tianbot/tianbot/dev-branch
export RDK_HOST=192.168.1.10
./scripts/deploy_to_rdk_x5.sh
```

默认会同步到远端：

```bash
/home/sunrise/rm_ws/src
```

可选环境变量：

- `RDK_USER`
- `RDK_HOST`
- `RDK_PORT`
- `REMOTE_WS`

## 4. 远端编译并启动

### 方式 A：一条命令完成

```bash
cd /home/tianbot/tianbot/dev-branch
export RDK_HOST=192.168.1.10
export SERIAL_PORT=/dev/ttyS1
export ENEMY_PREFIX=blue_
./scripts/build_and_run_on_rdk_x5.sh
```

### 方式 B：登录板子后手动执行

```bash
ssh sunrise@192.168.1.10
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py serial_port:=/dev/ttyS1 enemy_prefix:=blue_
```

## 5. 关键配置文件

### 串口桥接参数

文件：

`dev-branch/rm_gimbal_bridge/config/rm_gimbal_bridge.yaml`

重点参数：

- `serial_port`
- `baud_rate`
- `image_width`
- `image_height`
- `image_center_x`
- `image_center_y`
- `enemy_prefix`
- `selection_mode`

### 相机参数

文件：

`dev-branch/hik_camera/config/camera_params.yaml`

重点参数：

- `exposure_time`
- `gain`

## 6. 运行前检查

### RDK X5 侧

- 海康 MVS SDK 可用
- `/dev/ttyS1` 存在
- `/opt/tros/humble/setup.bash` 存在
- 模型文件可访问

### C 板侧

- 程序已烧录
- USART1 接到 RDK X5 对应串口
- 波特率一致：`921600`
- GND 共地

## 7. 调试建议

### 看检测结果话题

```bash
ros2 topic echo /dnn_node_sample
```

### 看相机图像话题

```bash
ros2 topic hz /hbmem_img
```

### 看桥接节点日志

如果检测到目标，桥接节点会打印类似：

```text
target=blue_3 center=(640,512) conf=0.87 dist=12.5
```

如果桥接节点没输出，优先检查：

1. `/dnn_node_sample` 是否有数据
2. `enemy_prefix` 是否筛掉了目标
3. 串口设备名是否正确
