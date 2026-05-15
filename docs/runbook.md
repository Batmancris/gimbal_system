# Runbook

Updated: 2026-05-15

## Clean Boot 检查

SSH 登录 RDK-X5 后检查基础状态:

```bash
# 检查 TROS 环境
source /opt/tros/humble/setup.bash

# 检查串口设备
ls -la /dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_*

# 检查相机设备
ls -la /dev/video*
```

## 一条命令启动

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

启动成功判据: `Camera OK.` / `Detection OK.` / `Bridge OK.` / `FAST FOLLOW READY.` / `profile: fast_best.`

回滚启动（使用 stable profile）:

```bash
ssh rdk-x5 "FOLLOW_PROFILE=stable bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

## 停止链路

```bash
ssh rdk-x5 "tmux -L autoaim kill-server 2>/dev/null; pkill -f 'ros2 run' || true"
```

## 验收命令

```bash
ssh rdk-x5 "source /opt/tros/humble/setup.bash; \
  source /home/sunrise/rm_ws/install/setup.bash; \
  ros2 node list; \
  ros2 topic info /hbmem_img; \
  ros2 topic info /bear_detection/targets; \
  fuser -v /dev/ttyACM0 2>/dev/null || true"
```

性能采集:

```bash
ssh rdk-x5 "cd /home/sunrise/rm_ws && DURATION=15 bash scripts/profile_fast_follow_link.sh"
```

## 常见故障

### 相机未启动

- 检查 USB 连接和 `/dev/video*` 设备
- 检查 hik_camera 节点日志: `tmux -L autoaim attach -t cam`

### 检测未启动

- 检查模型文件: `ls /home/sunrise/rm_ws/install/rm_bear_detection/share/rm_bear_detection/models/`
- 检查检测节点日志: `tmux -L autoaim attach -t det`

### 桥接未启动

- 检查串口设备: `ls -la /dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_*`
- 检查桥接节点日志: `tmux -L autoaim attach -t bridge`

### 云台不动

- 确认 STM32 固件已烧录
- 检查 USB-CDC 连接: `fuser -v /dev/ttyACM0`
- 检查串口权限: `ls -la /dev/ttyACM0`

## 禁止依赖

以下组件不用于主链路，禁止作为默认启动:

- `rm_vis` - 可视化工具
- `/image_raw` - 原始图像话题
- `publish_image_raw:=true` - 不使用此参数
