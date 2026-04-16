# TianAim RDK-X5 自瞄执行命令手册（ROS2/TROS）

## 1. 文档范围

设备：RDK-X5 上位机、海康工业相机、STM32 云台控制板

中间件：TROS Humble / ROS2

任务：部署上位机代码、编译三节点自瞄链、启动相机与识别、启动上位机到下位机桥接、安装开机自启动、排查话题与串口链路。

当前工作空间：

```text
ros2_ws/
├── scripts/
└── src/
    ├── hik_camera
    ├── rm_armor_detection
    ├── rm_gimbal_bridge
    ├── rm_vehicle_detection
    ├── rm_interfaces
    └── rm_utils
```

当前主链：

```text
hik_camera
  -> /image_raw
  -> /hbmem_img
  -> rm_armor_detection
  -> /dnn_node_sample
  -> rm_gimbal_bridge
  -> USB-CDC serial device
  -> STM32 vision_input
  -> target_state
  -> gimbal_task
```

## 2. 快速开始

在 RDK-X5 上运行相机、识别、可视化链：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
bash src/scripts/start_autoaim_tmux.sh
```

另开终端启动桥接链：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
bash src/scripts/start_rm_bridge_tmux.sh
```

检查关键话题：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
bash src/scripts/check_autoaim_topics.sh
```

如果使用当前仓库顶层封装，可以在仓库根目录运行：

```bash
cd /home/tianbot/tianbot
bash scripts/build_ros2_mainline.sh
bash scripts/run_ros2_bridge.sh
```

## 3. 环境与依赖

### 3.1 目标系统

- RDK-X5 板端用户默认：`sunrise`
- RDK-X5 工作空间默认：`/home/sunrise/rm_ws`
- RDK-X5 源码目录默认：`/home/sunrise/rm_ws/src`
- RDK-X5 脚本目录默认：`/home/sunrise/rm_ws/src/scripts`
- TROS 环境：`/opt/tros/humble/setup.bash`

### 3.2 主要 ROS2 包

当前脚本默认服务这三个上位机包：

```text
hik_camera
rm_armor_detection
rm_gimbal_bridge
```

对应编译命令：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge --event-handlers console_direct+
source install/setup.bash
```

### 3.3 串口设备

桥接脚本默认使用 USB-CDC by-id 设备：

```text
/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

如果你的板子上设备名不同，可以启动前覆盖：

```bash
export SERIAL_PORT=/dev/ttyACM0
bash src/scripts/start_rm_bridge_tmux.sh
```

也可以临时用 UART：

```bash
export SERIAL_PORT=/dev/ttyS1
bash src/scripts/start_rm_bridge_tmux.sh
```

## 4. 代码部署与路径配置

### 4.1 从本机部署到 RDK-X5

在开发机仓库目录运行：

```bash
cd /home/tianbot/tianbot/ros2_ws
export RDK_HOST=192.168.127.10
export RDK_USER=sunrise
export RDK_PORT=22
bash scripts/deploy_to_rdk_x5.sh
```

常用覆盖项：

```bash
export REMOTE_WS=/home/sunrise/rm_ws
export REMOTE_SRC_DIR=/home/sunrise/rm_ws/src
export REMOTE_SCRIPT_DIR=/home/sunrise/rm_ws/src/scripts
```

说明：

- `deploy_to_rdk_x5.sh` 会同步 ROS2 包和 `scripts/` 到 RDK-X5。
- 默认排除远端 `build/`、`install/`、`log/`。
- 迁移期间如果源码目录不在默认位置，优先用上面的环境变量覆盖。

### 4.2 远端编译并启动

在开发机仓库目录运行：

```bash
cd /home/tianbot/tianbot/ros2_ws
export RDK_HOST=192.168.127.10
export RDK_USER=sunrise
export SERIAL_PORT=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
export ENEMY_PREFIX=blue_
bash scripts/build_and_run_on_rdk_x5.sh
```

说明：

- 该脚本通过 SSH 进入 RDK-X5。
- 默认调用远端 `clean_build_and_start_on_rdk.sh`。
- 默认会清理远端 `build/ install/ log/` 后重新编译。
- 编译后会启动 `start_autoaim_tmux.sh`，也就是相机、识别、可视化链。

## 5. 编译工作空间

### 5.1 推荐顶层入口

在仓库根目录：

```bash
cd /home/tianbot/tianbot
bash scripts/build_ros2_mainline.sh
```

该入口会使用 `scripts/tianaim_paths.sh` 解析当前 ROS2 工作空间路径，默认编译：

```text
hik_camera rm_armor_detection rm_gimbal_bridge
```

### 5.2 RDK-X5 直接编译

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge --event-handlers console_direct+
source install/setup.bash
```

### 5.3 清理后重编译

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
rm -rf build install log
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge --event-handlers console_direct+
source install/setup.bash
```

也可以使用脚本：

```bash
cd /home/sunrise/rm_ws
REMOTE_WS=/home/sunrise/rm_ws bash src/scripts/clean_build_and_start_on_rdk.sh
```

## 6. 启动流程

### 6.1 相机节点

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 launch hik_camera hik_camera.launch.py
```

关键输出：

```text
/image_raw
/hbmem_img
```

### 6.2 装甲板识别节点

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 run rm_armor_detection rm_armor_detection
```

关键输入输出：

```text
input:  /hbmem_img
output: /dnn_node_sample
```

### 6.3 可视化节点

板端有显示环境时运行：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
export XDG_RUNTIME_DIR=/run/user/1000
ros2 run rm_armor_detection rm_armor_detection_visualizer --ros-args \
  -p image_topic:=/image_raw \
  -p targets_topic:=/dnn_node_sample
```

### 6.4 桥接节点

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args \
  -p serial_port:=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00 \
  -p enemy_prefix:=blue_ \
  -p require_lower_vision_enabled:=false
```

桥接协议：

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

### 6.5 整机 launch 启动

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_autoaim_system.launch.py \
  serial_port:=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00 \
  enemy_prefix:=blue_
```

如果只启动桥接节点：

```bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py
```

## 7. tmux 脚本启动

### 7.1 自动瞄准显示链

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
bash src/scripts/start_autoaim_tmux.sh
```

该脚本会启动三个 tmux session：

```text
hik_cam  -> run_hik_cam_loop.sh
rm_det   -> run_rm_det_loop.sh
rm_vis   -> run_rm_vis_loop.sh
```

常用参数：

```bash
export STARTUP_DELAY_SEC=2
export DETECTOR_DELAY_SEC=0
export CAMERA_READY_TIMEOUT_SEC=8
export VIS_DELAY_SEC=3
export TMUX_SOCKET=autoaim
bash src/scripts/start_autoaim_tmux.sh
```

查看 tmux：

```bash
tmux -L autoaim ls
tmux -L autoaim attach -t hik_cam
tmux -L autoaim attach -t rm_det
tmux -L autoaim attach -t rm_vis
```

停止：

```bash
tmux -L autoaim kill-session -t hik_cam
tmux -L autoaim kill-session -t rm_det
tmux -L autoaim kill-session -t rm_vis
```

### 7.2 云台桥接链

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
bash src/scripts/start_rm_bridge_tmux.sh
```

该脚本会启动一个 tmux session：

```text
rm_bridge -> run_rm_bridge_loop.sh
```

常用参数：

```bash
export TMUX_SOCKET=bridge
export SERIAL_PORT=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
export ENEMY_PREFIX=blue_
export BRIDGE_DELAY_SEC=4
export WAIT_FOR_SERIAL_SEC=15
export DETECTOR_READY_TIMEOUT_SEC=10
export BRIDGE_REQUIRE_VISION_ENABLED=false
bash src/scripts/start_rm_bridge_tmux.sh
```

查看与停止：

```bash
tmux -L bridge ls
tmux -L bridge attach -t rm_bridge
tmux -L bridge kill-session -t rm_bridge
```

## 8. systemd 用户自启动

### 8.1 安装相机与识别自启动

在开发机仓库目录运行：

```bash
cd /home/tianbot/tianbot/ros2_ws
export RDK_HOST=192.168.127.10
export RDK_USER=sunrise
bash scripts/install_autostart_on_rdk_x5.sh
```

该脚本会同步并启用：

```text
~/.config/systemd/user/rm-autoaim.service
```

板端手动控制：

```bash
systemctl --user daemon-reload
systemctl --user enable rm-autoaim.service
systemctl --user start rm-autoaim.service
systemctl --user status rm-autoaim.service
systemctl --user stop rm-autoaim.service
```

### 8.2 桥接自启动

当前仓库提供 `rm-bridge.service`。如需板端启用，可将文件放到：

```text
~/.config/systemd/user/rm-bridge.service
```

然后在 RDK-X5 上运行：

```bash
systemctl --user daemon-reload
systemctl --user enable rm-bridge.service
systemctl --user start rm-bridge.service
systemctl --user status rm-bridge.service
```

桥接服务默认环境：

```text
TMUX_SOCKET=bridge
SERIAL_PORT=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
ENEMY_PREFIX=blue_
BRIDGE_DELAY_SEC=4
WAIT_FOR_SERIAL_SEC=15
DETECTOR_READY_TIMEOUT_SEC=10
BRIDGE_REQUIRE_VISION_ENABLED=false
```

## 9. 节点、话题与参数速览

### 9.1 核心节点

- `hik_camera_node`：海康相机采集，发布普通图像与共享内存图像
- `rm_armor_detection`：订阅 `/hbmem_img`，发布装甲板检测结果
- `rm_armor_detection_visualizer`：订阅 `/image_raw` 与 `/dnn_node_sample`，显示检测叠加画面
- `rm_gimbal_bridge_node`：订阅 `/dnn_node_sample`，通过串口发送目标中心点到 STM32

### 9.2 关键话题

| 话题 | 方向 | 说明 |
| --- | --- | --- |
| `/image_raw` | `hik_camera -> visualizer` | 标准 ROS 图像流 |
| `/hbmem_img` | `hik_camera -> rm_armor_detection` | TROS 共享内存图像 |
| `/dnn_node_sample` | `rm_armor_detection -> rm_gimbal_bridge` | AI 检测结果 |

### 9.3 桥接常用参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `input_topic` | `/dnn_node_sample` | 检测结果输入 |
| `serial_port` | `/dev/ttyS1` | 节点默认串口，脚本通常覆盖为 USB-CDC by-id |
| `baud_rate` | `921600` | 串口波特率 |
| `min_confidence` | `0.35` | 当前配置文件置信度阈值 |
| `enemy_prefix` | 空字符串 | 目标颜色前缀过滤；例如 `blue_` |
| `selection_mode` | `closest` | 默认选择最接近图像中心的目标 |
| `require_lower_vision_enabled` | `true` | 是否要求下位机视觉使能反馈；脚本可覆盖为 `false` |

## 10. USB-CDC 诊断

### 10.1 Python pitch 测试

注意：该脚本只用于 `/dev/ttyACM0` 最小 pitch 控制验证，不替代正式视觉链路。使用前需要 STM32 侧显式打开对应测试模式。

```bash
cd /home/sunrise/rm_ws
python3 src/scripts/usb_cdc_pitch_control_test.py
```

### 10.2 桥接包内诊断程序

如果已编译 `rm_gimbal_bridge`，可按实际可执行名运行：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 run rm_gimbal_bridge usb_cdc_ping_test
ros2 run rm_gimbal_bridge usb_cdc_pitch_test
```

## 11. 常见问题

### 11.1 `/hbmem_img` 没有发布者

处理：

```bash
ros2 topic info /hbmem_img -v
tmux -L autoaim capture-pane -pt hik_cam
```

检查相机是否被识别、`hik_camera_node` 是否启动、相机参数是否正确。

### 11.2 `/dnn_node_sample` 没有发布者

处理：

```bash
ros2 topic info /dnn_node_sample -v
tmux -L autoaim capture-pane -pt rm_det
```

先确认 `/hbmem_img` 正常，再看识别节点日志。

### 11.3 桥接启动失败或串口打不开

处理：

```bash
ls -l /dev/serial/by-id/
ls -l /dev/ttyACM*
tmux -L bridge capture-pane -pt rm_bridge
```

如果设备名不同，覆盖 `SERIAL_PORT` 后重启：

```bash
export SERIAL_PORT=/dev/ttyACM0
bash src/scripts/start_rm_bridge_tmux.sh
```

### 11.4 识别有结果但下位机不动

处理：

```bash
ros2 topic echo -n 5 /dnn_node_sample
tmux -L bridge capture-pane -pt rm_bridge
```

重点检查：

- `enemy_prefix` 是否过滤掉了目标
- `min_confidence` 是否过高
- `SERIAL_PORT` 是否指向当前 USB-CDC 设备
- STM32 侧是否进入视觉输入链路
- `require_lower_vision_enabled` 是否需要临时设为 `false`

### 11.5 板端可视化窗口不显示

处理：

```bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
export XDG_RUNTIME_DIR=/run/user/1000
ros2 run rm_armor_detection rm_armor_detection_visualizer --ros-args \
  -p image_topic:=/image_raw \
  -p targets_topic:=/dnn_node_sample
```

如果仍不显示，先确认板端桌面环境和权限。

### 11.6 tmux session 已存在或状态混乱

处理：

```bash
tmux -L autoaim kill-session -t hik_cam 2>/dev/null || true
tmux -L autoaim kill-session -t rm_det 2>/dev/null || true
tmux -L autoaim kill-session -t rm_vis 2>/dev/null || true
tmux -L bridge kill-session -t rm_bridge 2>/dev/null || true
```

然后重新启动：

```bash
bash src/scripts/start_autoaim_tmux.sh
bash src/scripts/start_rm_bridge_tmux.sh
```

## 12. 固定排障命令

```bash
ros2 topic list
ros2 topic info /image_raw -v
ros2 topic info /hbmem_img -v
ros2 topic info /dnn_node_sample -v
ros2 topic echo -n 5 /dnn_node_sample
tmux -L autoaim ls
tmux -L bridge ls
tmux -L autoaim capture-pane -pt hik_cam
tmux -L autoaim capture-pane -pt rm_det
tmux -L autoaim capture-pane -pt rm_vis
tmux -L bridge capture-pane -pt rm_bridge
systemctl --user status rm-autoaim.service
systemctl --user status rm-bridge.service
```

用途：

- 验证相机图像发布
- 验证共享内存图像链路
- 验证检测结果发布
- 验证桥接节点是否收到目标
- 验证 USB-CDC 串口是否可用
- 验证 systemd 自启动状态

## 13. 包级说明

包内部 README 保留为局部说明入口：

- `src/hik_camera/README.md`
- `src/rm_armor_detection/README.md`
- `src/rm_gimbal_bridge/README.md`
- `src/rm_interfaces/README.md`
- `src/rm_utils/README.md`

如果只想看执行命令，优先读本文件。
