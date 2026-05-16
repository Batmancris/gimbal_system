# rm_gimbal_bridge

`rm_gimbal_bridge` 把检测节点发布的 `ai_msgs/msg/PerceptionTargets` 转成 STM32 能接收的视觉目标坐标帧。

## 当前角色

在当前主线中，桥接节点位于：

```text
rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> USB-CDC -> STM32
```

低速跟随已经顺滑；高速目标移动时仍会出现跟不上，桥接侧后续重点是继续验证预测、发送限幅和目标稳定策略。

## 串口帧格式

当前发送给 STM32 的坐标帧：

```text
0xFA 0xFB X_L X_H Y_L Y_H 0xFC 0xFD
```

其中 `X/Y` 是图像坐标里的目标中心点。没有有效目标或目标超时后，桥接会发送图像中心点作为 neutral frame，避免下位机继续沿旧速度盲转。

## 主要功能

- 订阅检测结果话题。
- 按类型、置信度和中心区域筛选候选目标。
- 用 sticky target 逻辑避免多个目标之间来回跳。
- 以固定频率发送平滑后的目标点。
- 读取 STM32 诊断帧，确认下位机视觉开关、目标有效状态和调参数据。
- 串口写失败时自动尝试 reopen。

## 运行上下文说明

- 当前推荐入口：`bash ros2_ws/scripts/start_fast_follow_verified.sh`
- 当前 baseline 使用 `FOLLOW_PROFILE=fast_best`。
- `run_rm_bridge_loop.sh` 自身默认 profile 为 `stable`，但被 `start_fast_follow_verified.sh` 调用时会被覆盖为 `fast_best`。
- `rm_gimbal_bridge.yaml` 和 `serial_bridge_node.cpp` 中的 `declare_parameter` 默认值是 fallback / legacy 默认值，不代表当前 fast_best baseline 的最终运行值。
- 脚本传入的 `--ros-args -p` 优先级最高，是当前板端运行值的来源。

## 当前 fast_best baseline 参数

以下为 `start_fast_follow_verified.sh` 通过 `run_rm_bridge_loop.sh` 实际传入的 fast_best 运行值：

```bash
input_topic=/bear_detection/targets
allowed_target_types=['bear']
min_confidence=0.71
require_lower_vision_enabled=false
serial_port=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
follow_send_rate_hz=80.0
follow_interp_rate_hz=35.0
follow_control_mode=light_predict
follow_smoothing_alpha=0.50
follow_max_step_px=75.0
follow_deadband_px=5.0
measurement_jitter_deadband_px=10.0
fast_follow_error_px=95.0
fast_follow_smoothing_alpha=0.62
fast_follow_max_step_px=105.0
light_follow_gain=0.58
predict_alpha=0.65
predict_beta=0.03
predict_horizon_sec=0.025
target_hold_ms=240
target_switch_radius_px=200.0
target_switch_min_conf_gain=0.30
target_switch_center_gain_px=60.0
min_send_delta_px=1.0
send_keepalive_ms=20
center_gate_x_ratio=1.00
center_gate_y_ratio=1.00
enable_fixed_rate_follow=true
```

注意：`rm_gimbal_bridge.yaml` 中的值是 fallback 默认值（面向 vehicle 场景），不代表当前 bear-follow baseline 的运行参数。

## 运行

直接运行 bear 主线：

```bash
ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args \
  -p input_topic:=/bear_detection/targets \
  -p allowed_target_types:="['bear']" \
  -p min_confidence:=0.71 \
  -p serial_port:=/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00
```

通过主链路脚本启动：

```bash
bash ros2_ws/scripts/start_fast_follow_verified.sh
```

查看桥接日志：

```bash
tmux -L bridge capture-pane -pt rm_bridge
```

## 高速跟随调试说明

当前不要把高速问题归因到单一参数。建议按顺序看：

1. `/bear_detection/targets` 是否在高速下仍稳定输出目标。
2. 桥接日志里的目标中心点是否明显滞后。
3. STM32 诊断里的 `err=(x,y)`、`add=(yaw,pitch)mrad` 是否到达限幅。
4. 云台实际响应是否被机械惯量或电机限流限制。

可临时打开：

```bash
export BRIDGE_LOG_DIAG_FEEDBACK=true
export BEAR_LOG_DETECTIONS=true
```

如果只调整桥接侧，优先小步尝试：

```bash
export FAST_FOLLOW_MAX_STEP_PX=84.0
export LIGHT_FOLLOW_GAIN=0.50
```

每次只改一组参数，确认低速丝滑没有被破坏后再继续。

