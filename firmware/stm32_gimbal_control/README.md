# STM32 gimbal control

`firmware/stm32_gimbal_control` 是当前下位机主线工程，目标平台是 STM32F407 / DJI C 板一类控制板。它负责云台姿态、电机闭环、遥控输入、视觉输入解析和 USB CDC 通信。

## 当前真实状态

- 下位机已经接入 RDK X5 视觉链路：`rm_gimbal_bridge -> USB CDC -> vision_input -> target_state -> gimbal_task`。
- 低速目标跟随已经现场验证为顺滑。
- 高速跟随仍然存在跟不上，需要继续调试视觉增量限幅、预测、刹车平滑和机械响应。
- 当前代码保留 UART 兼容解析，不建议在确认完全不需要前删除。

## 关键文件

- `Src/vision_input.c`：接收 USB CDC/UART 喂进来的视觉帧，解析坐标。
- `Src/target_state.c`：保存最新目标状态，处理超时和滤波。
- `Src/gimbal_task.c`：把图像误差转成 yaw/pitch 角度增量，并进入云台控制闭环。
- `Src/gimbal_task.h`：视觉跟随参数、PID/PD 参数、限幅参数。
- `Src/usb_cdc_test.c`：USB CDC 诊断和测试辅助。
- `USB_DEVICE/App/usbd_cdc_if.c`：USB CDC 接收入口。

## 当前视觉控制链路

```text
RDK X5 rm_gimbal_bridge
  -> USB CDC 8-byte target frame
  -> vision_input.c parses x/y/seq
  -> target_state.c stores latest target
  -> gimbal_task.c calculates pixel error
  -> vision PID-shaped controller
  -> yaw/pitch angle increment
  -> motor angle loop + speed loop
```

## 当前主要参数

参数位于 `Src/gimbal_task.h`：

```c
#define VISION_X_DEADBAND         14.0f
#define VISION_Y_DEADBAND         14.0f
#define VISION_YAW_PID_KP         0.0000062f
#define VISION_YAW_PID_KI         0.0f
#define VISION_YAW_PID_KD         0.000055f
#define VISION_PITCH_PID_KP       0.0000060f
#define VISION_PITCH_PID_KI       0.0f
#define VISION_PITCH_PID_KD       0.000042f
#define VISION_MAX_ANGLE_STEP     0.0036f
#define VISION_FAST_ANGLE_STEP    0.0050f
#define VISION_FAST_ERROR_THRESHOLD 160.0f
#define VISION_CMD_SMOOTH_ALPHA   0.30f
#define VISION_CMD_FAST_ALPHA     0.42f
#define VISION_CMD_BRAKE_ALPHA    0.92f
#define VISION_SLOWDOWN_ERROR_PX  300.0f
#define VISION_MIN_STEP_SCALE     0.10f
#define VISION_FRAME_HOLD_DECAY   0.990f
#define VISION_FRAME_BRAKE_DECAY  0.970f
```

目标状态滤波位于 `Src/target_state.h`：

```c
#define TARGET_STATE_TIMEOUT_MS      100U
#define TARGET_STATE_SMOOTH_ALPHA    1.00f
```

## 诊断量

`GimbalVisionDiag_Get()` 会输出下位机诊断信息，桥接节点可打印：

- `vision_enabled`：下位机视觉控制是否允许。
- `target_valid`：当前目标是否有效。
- `target_seq`：目标帧序号。
- `raw_x/raw_y`：收到的原始目标中心。
- `error_x/error_y`：相对图像中心的像素误差。
- `yaw_add_mrad/pitch_add_mrad`：本周期实际追加角度，单位毫弧度。
- `yaw_given_current/pitch_given_current`：电机电流输出。

上位机打开诊断：

```bash
export BRIDGE_LOG_DIAG_FEEDBACK=true
bash src/scripts/start_rm_bridge_tmux.sh
```

## 高速跟随调试建议

当前不要直接大幅加 KP。推荐顺序：

1. 先确认高速时 RDK X5 仍连续发目标，排除检测丢失。
2. 看 `error_x/error_y` 是否持续很大，如果误差大但 `yaw_add_mrad/pitch_add_mrad` 很小，说明下位机增量或限幅偏保守。
3. 小步提高 `VISION_FAST_ANGLE_STEP`，每次烧录后都要验证低速是否仍然丝滑。
4. 如果追上后出现过冲，优先调整 `VISION_CMD_FAST_ALPHA`、`VISION_CMD_BRAKE_ALPHA` 或 `VISION_SLOWDOWN_ERROR_PX`。
5. 只有在确认低速和刹车都稳定后，再考虑继续加 KP 或引入预测。

## 当前结论

这版固件可以作为“低速跟随稳定主线”。高速跟随还不能写成完成，需要继续保留日志和参数入口，现场逐步推进。

