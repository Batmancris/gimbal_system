# ros2_ws/scripts

RDK X5 板端启动、诊断和部署脚本。

当前默认主链路：

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> STM32 USB-CDC
```

## 唯一推荐启动

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

启动成功判据: `Camera OK.` / `Detection OK.` / `Bridge OK.` / `FAST FOLLOW READY.` / `profile: fast_best.`

回滚启动:

```bash
ssh rdk-x5 "FOLLOW_PROFILE=stable bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

## 脚本分类

### 唯一推荐入口

| 脚本 | 说明 |
|---|---|
| `start_fast_follow_verified.sh` | headless fast_best 一键启动，唯一推荐入口 |

### fallback 入口

| 脚本 | 说明 |
|---|---|
| `start_headless_follow_light.sh` | 被 verified 脚本调用的底层启动 |

### 底层 loop（tmux 守护）

| 脚本 | 说明 |
|---|---|
| `run_hik_cam_loop.sh` | 相机节点循环守护 |
| `run_rm_det_loop.sh` | 检测节点循环守护 |
| `run_rm_bridge_loop.sh` | 桥接节点循环守护，**fast_best 参数在此集中维护** |

### diagnostics

| 脚本 | 说明 |
|---|---|
| `profile_fast_follow_link.sh` | 性能采集/健康检查 |
| `check_autoaim_topics.sh` | 话题/tmux 状态检查 |
| `desktop_status_full_stack.sh` | 全栈状态查看 |
| `desktop_stop_full_stack.sh` | 全栈停止 |
| `usb_cdc_pitch_control_test.py` | USB-CDC 协议测试工具 |

### deployment

| 脚本 | 说明 |
|---|---|
| `deploy_to_rdk_x5.sh` | 从本机同步到板端 |
| `build_and_run_on_rdk_x5.sh` | 远程构建启动 |
| `clean_build_and_start_on_rdk.sh` | 清理重建启动 |

### legacy（不用于当前主链路）

| 脚本 | 说明 |
|---|---|
| `start_autoaim_tmux.sh` | 旧版手动 tmux 启动，含 visualizer |
| `start_rm_bridge_tmux.sh` | 旧版手动桥接 tmux 启动 |
| `desktop_start_full_stack.sh` | 旧版桌面全栈启动 |
| `desktop_start_headless_stack.sh` | 旧版桌面无头启动 |

### visualizer（不用于 headless fast_best 主链路）

| 脚本 | 说明 |
|---|---|
| `start_rm_vis_tmux.sh` | rm_vis 可视化启动 |
| `run_rm_vis_loop.sh` | rm_vis 循环守护 |

### service/autostart（当前不推荐使用）

| 脚本 | 说明 |
|---|---|
| `install_autostart_on_rdk_x5.sh` | systemd 自动启动安装 |
| `install_desktop_launchers_on_rdk_x5.sh` | 桌面快捷方式安装 |
| `rm-autoaim.service` | systemd service 文件 |
| `rm-bridge.service` | systemd service 文件 |

## 当前默认环境变量

### 检测侧

```bash
DETECTOR_TYPE=bear
DETECTOR_TOPIC=/bear_detection/targets
BEAR_SCORE_THRESHOLD=0.71
```

### 桥接侧（fast_best profile）

桥接参数集中在 `run_rm_bridge_loop.sh` 中维护，不要在其他地方重复定义。

```bash
ALLOWED_TARGET_TYPES=bear
BRIDGE_MIN_CONFIDENCE=0.71
FOLLOW_SEND_RATE_HZ=80.0
FOLLOW_CONTROL_MODE=light_predict
FAST_FOLLOW_MAX_STEP_PX=105.0
FAST_FOLLOW_ERROR_PX=95.0
FAST_FOLLOW_SMOOTHING_ALPHA=0.62
LIGHT_FOLLOW_GAIN=0.58
TARGET_HOLD_MS=240
```

### 串口

```bash
SERIAL_PORT=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

## 查看运行状态

```bash
ssh rdk-x5 "source /opt/tros/humble/setup.bash; source /home/sunrise/rm_ws/install/setup.bash; \
  ros2 node list; \
  ros2 topic info /hbmem_img; \
  ros2 topic info /bear_detection/targets; \
  tmux -L autoaim ls 2>/dev/null || true"
```

## 约束

- 不允许新增 `start_xxx_new.sh` 启动脚本
- fast_best 参数只在 `run_rm_bridge_loop.sh` 中维护
- 不使用 `/image_raw`、`rm_vis` 作为默认链路
- 不重新引入 `single_target_hold_ms` / `SelectSingleTarget`
