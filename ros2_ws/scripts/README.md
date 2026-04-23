# ros2_ws/scripts

这里是 RDK X5 板端和桌面端常用脚本。当前默认主线是 bear 低速跟随链路：

```text
hik_camera -> rm_bear_detection -> rm_gimbal_bridge -> STM32 USB-CDC
```

低速跟随已经顺滑，高速跟随仍需要继续调试。

## 从本机部署到 RDK X5

在 Windows Git Bash、ros2go 终端或 Linux 终端里执行：

```bash
cd /path/to/gimbal_system_repo_push/ros2_ws
export RDK_HOST=192.168.1.10
export RDK_USER=sunrise
bash scripts/deploy_to_rdk_x5.sh
```

部署后，RDK X5 上通常会有：

```text
/home/sunrise/rm_ws/src/
/home/sunrise/rm_ws/src/scripts/
```

## 构建并启动

从本机远程触发 RDK X5 清理、构建、启动：

```bash
export RDK_HOST=192.168.1.10
bash scripts/build_and_run_on_rdk_x5.sh
```

在 RDK X5 上手动执行：

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash

export DETECTOR_TYPE=bear
export DETECTOR_TOPIC=/bear_detection/targets
export ALLOWED_TARGET_TYPES=bear

bash src/scripts/start_autoaim_tmux.sh
bash src/scripts/start_rm_bridge_tmux.sh
```

## 常用脚本

- `deploy_to_rdk_x5.sh`：从本机同步 ROS2 包和脚本到 RDK X5。
- `build_and_run_on_rdk_x5.sh`：通过 SSH 在 RDK X5 上构建并启动。
- `clean_build_and_start_on_rdk.sh`：在 RDK X5 上清理 `build/install/log`、重新构建并启动。
- `start_autoaim_tmux.sh`：启动相机、检测、可视化 tmux 会话。
- `start_rm_bridge_tmux.sh`：单独启动桥接 tmux 会话。
- `run_hik_cam_loop.sh`：相机节点循环守护。
- `run_rm_det_loop.sh`：bear/vehicle/armor 检测循环守护。
- `run_rm_bridge_loop.sh`：桥接节点循环守护。
- `check_autoaim_topics.sh`：查看话题和 tmux 日志。
- `desktop_status_full_stack.sh`：查看 user service、tmux、进程状态。
- `desktop_stop_full_stack.sh`：停止 user service、tmux 和相关进程。

## 当前默认环境变量

检测侧：

```bash
DETECTOR_TYPE=bear
DETECTOR_TOPIC=/bear_detection/targets
BEAR_SCORE_THRESHOLD=0.71
BEAR_STABLE_REQUIRED_HITS=2
BEAR_STABLE_MATCH_RADIUS_PX=140.0
BEAR_STABLE_MAX_TRACK_AGE_MS=200
```

桥接侧：

```bash
ALLOWED_TARGET_TYPES=bear
BRIDGE_MIN_CONFIDENCE=0.71
FOLLOW_SEND_RATE_HZ=50.0
FOLLOW_CONTROL_MODE=light_predict
FOLLOW_MAX_STEP_PX=36.0
FAST_FOLLOW_MAX_STEP_PX=72.0
TARGET_HOLD_MS=350
```

串口默认使用 USB CDC by-id：

```bash
SERIAL_PORT=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

如果实际板子枚举名字不同，先查：

```bash
ls -l /dev/serial/by-id/
ls -l /dev/ttyACM*
```

## 查看运行状态

```bash
ros2 node list
ros2 topic list
ros2 topic info /hbmem_img -v
ros2 topic info /bear_detection/targets -v
ros2 topic echo /bear_detection/targets --once

tmux -L autoaim ls
tmux -L autoaim capture-pane -pt hik_cam
tmux -L autoaim capture-pane -pt rm_det
tmux -L bridge capture-pane -pt rm_bridge
```

## 高速跟随调试入口

先打开日志，不要一次改很多参数：

```bash
export BRIDGE_LOG_DIAG_FEEDBACK=true
export BEAR_LOG_DETECTIONS=true
export FOLLOW_SEND_RATE_HZ=50.0
export FAST_FOLLOW_MAX_STEP_PX=72.0
```

如果确认是桥接目标点移动太保守，可小步提高：

```bash
export FAST_FOLLOW_MAX_STEP_PX=84.0
export LIGHT_FOLLOW_GAIN=0.50
```

如果确认是下位机单次角度增量不够，需要改固件 `Src/gimbal_task.h` 里的 `VISION_FAST_ANGLE_STEP`，重新编译烧录后再测试。

