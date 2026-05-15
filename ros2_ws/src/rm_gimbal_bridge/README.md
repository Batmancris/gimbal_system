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

## 当前默认参数

板端脚本 `ros2_ws/scripts/run_rm_bridge_loop.sh` 的当前默认值：

```bash
DETECTOR_TOPIC=/bear_detection/targets
ALLOWED_TARGET_TYPES=bear
BRIDGE_MIN_CONFIDENCE=0.71
ENABLE_FIXED_RATE_FOLLOW=true
FOLLOW_SEND_RATE_HZ=50.0
FOLLOW_CONTROL_MODE=light_predict
FOLLOW_SMOOTHING_ALPHA=0.35
FOLLOW_MAX_STEP_PX=36.0
FOLLOW_DEADBAND_PX=5.0
FAST_FOLLOW_ERROR_PX=120.0
FAST_FOLLOW_SMOOTHING_ALPHA=0.55
FAST_FOLLOW_MAX_STEP_PX=72.0
LIGHT_FOLLOW_GAIN=0.45
TARGET_HOLD_MS=350
TARGET_SWITCH_RADIUS_PX=120.0
TARGET_SWITCH_MIN_CONF_GAIN=0.30
CENTER_GATE_X_RATIO=1.00
CENTER_GATE_Y_RATIO=1.00
```

注意：源码里的 `declare_parameter` 是兜底默认值，实际板端运行时通常由脚本覆盖。

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

