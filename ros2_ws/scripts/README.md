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

### 检测器选择 (DETECTOR_TYPE)

`run_rm_det_loop.sh` 通过 `DETECTOR_TYPE` 环境变量选择启动哪个检测节点。

| DETECTOR_TYPE | 启动节点 | 状态 |
|---|---|---|
| `bear`（默认） | `rm_bear_detection_node` | 主线推荐 |
| `vehicle` | `rm_vehicle_detection_node` | legacy/可选 |
| 其他值 | — | 脚本报错退出（FATAL），不会 fallback |

当前 `start_fast_follow_verified.sh` 硬编码 `DETECTOR_TYPE=bear`，正常启动路径不受影响。

### diagnostics

| 脚本 | 说明 |
|---|---|
| `profile_fast_follow_link.sh` | 性能采集/健康检查 |
| `check_autoaim_topics.sh` | 话题/tmux 状态检查 |
| `usb_cdc_pitch_control_test.py` | USB-CDC 协议测试工具 |

## 约束

- 不允许新增 `start_xxx_new.sh` 启动脚本
- fast_best 参数只在 `run_rm_bridge_loop.sh` 中维护
- 不使用 `/image_raw`、`rm_vis` 作为默认链路
- 不重新引入 `single_target_hold_ms` / `SelectSingleTarget`
