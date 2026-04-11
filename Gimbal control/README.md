# Gimbal control

[中文](#中文) | [English](#english)

## 中文

### 简介

`Gimbal control` 是当前项目唯一可信的下位机主线工程，运行在 STM32F407 / DJI C 板一类控制板上。

该工程主要负责：

- 云台姿态与电机控制
- CAN 总线和电机交互
- DBUS 遥控输入
- BMI088 / IST8310 IMU 与姿态解算
- 视觉输入解析
- FreeRTOS 任务组织

### 当前主线结论

- 下位机主线：是
- 当前稳定通信主链：`UART`
- 当前迁移方向：`USB CDC`
- 当前建议：继续以本目录作为唯一下位机演进基础

### 产品化迁移说明

`Gimbal control` 目前仍然是实际固件主线目录，但目录名包含空格，不适合作为长期产品仓结构。

当前仓库已经新增根目录 `firmware/` 作为迁移锚点，后续建议把本工程分步迁移到：

```text
firmware/stm32_gimbal_control/
```

在完成 Makefile、IDE、文档和外部脚本引用统一前，不建议直接重命名本目录。

### USB CDC 当前状态

截至当前工作区状态，USB CDC 已完成以下基础能力：

- STM32 端成功枚举为 USB CDC ACM 设备
- 主机端可识别为 `/dev/ttyACM0`
- 下位机可周期性发送 heartbeat：`HB\r\n`
- 主机到 STM32 的接收路径已打通
- 下位机可对收到的数据进行回显
- 已增加 pitch swing test with safety switch 诊断能力
- 已完成 `rm_gimbal_bridge -> USB CDC -> Gimbal control` 的一次成功目标跟踪联调
- P 轴视觉跟踪比例与诊断链路已补齐到和 Y 轴一致，现场联调已验证有效

这说明：

- USB CDC 传输层已经双向打通
- 但它还没有完全替代 UART 成为整机默认主链
- 当前不应贸然删除 UART 相关路径

### 关键文件

- `Src/main.c`
- `Src/freertos.c`
- `Src/gimbal_task.c`
- `Src/vision_input.c`
- `Src/usb_cdc_test.c`
- `USB_DEVICE/App/usbd_cdc_if.c`
- `USB_CDC_MIGRATION.md`

### 当前建议

1. 继续把本目录作为唯一正式下位机工程维护
2. 继续保留 UART 兼容路径直到 USB CDC 全链路验证完成
3. 若继续推进 USB CDC，请优先复用现有 `vision_input` 与 `usb_cdc_test` 入口

### 阅读建议

- 若你在看整机链路，请同时参考上位机 `dev-branch/README.md`
- 若你在看 USB CDC 迁移，请同时看 `dev-branch/scripts/README.md`

### 视觉跟随调参记录

当前下位机在视觉跟随过程中会持续填充并上送以下诊断量：

- `error_x` / `error_y`：目标相对图像中心的滤波后像素误差
- `yaw_add_mrad` / `pitch_add_mrad`：本次视觉控制实际追加的角度增量，单位毫弧度
- `pitch_set_mrad`：当前 P 轴最终设定值，单位毫弧度

当前行为：

- 视觉跟随已整理为 PID 框架，但当前默认仅启用 `P`
- `P` 轴跟随目标额外限制在 `[-pi/4, +pi/4]`
- 单次视觉追加量仍受 `VISION_MAX_ANGLE_STEP` 保护

联调建议：

1. 在上位机运行 `rm_gimbal_bridge` 时观察 `tune err=(ex,ey) add=(yaw,pitch)mrad pitch_set=...` 日志
2. 若 `error_y` 长时间很大而 `pitch_add_mrad` 很小，可增大 `VISION_PITCH_PID_KP`
3. 若 `pitch_add_mrad` 来回抖动、目标上下振荡，可减小 `VISION_PITCH_PID_KP`
4. 若 `pitch_set_mrad` 经常贴近 `+-785mrad`，说明已触发当前 `P` 轴软件限位

当前调参入口：

- `Src/gimbal_task.h`
  - `VISION_YAW_PID_KP`
  - `VISION_PITCH_PID_KP`
  - `VISION_PID_MAX_OUT`
  - `GIMBAL_PITCH_FOLLOW_MAX_ANGLE`

## English

### Overview

`Gimbal control` is the only trusted lower-level firmware mainline in the current project, targeting STM32F407 / DJI C-board class controllers.

This firmware is responsible for:

- gimbal attitude and motor control
- CAN bus and motor interaction
- DBUS remote input
- BMI088 / IST8310 IMU and attitude estimation
- visual input parsing
- FreeRTOS task organization

### Current Mainline Status

- Lower-level mainline: yes
- Current validated communication path: `UART`
- Current migration direction: `USB CDC`
- Current recommendation: continue evolving this directory as the only formal lower-level codebase

### USB CDC Status

At the current workspace state, USB CDC already provides:

- successful STM32 USB CDC ACM enumeration
- host-side device visibility as `/dev/ttyACM0`
- periodic firmware heartbeat transmission: `HB\r\n`
- verified host-to-STM32 receive path
- RX echo support on the firmware side
- pitch swing test support with a safety switch
- a verified minimal pitch control validation path from upper-level host to lower-level firmware
- the ability to switch back to the default remote/UART path after testing
- one successful end-to-end target-tracking integration with `rm_gimbal_bridge -> USB CDC -> Gimbal control`
- pitch visual tracking gain and diagnostic visibility aligned with yaw, verified in on-device integration

This validation path specifically keeps:

- PID logic unchanged
- angle/speed limiting unchanged
- yaw mainline logic unchanged

This means:

- the USB CDC transport layer is bidirectionally working
- the minimal upper-to-lower USB CDC control validation path is also working
- but it has not fully replaced UART as the default whole-system mainline
- the UART path should not be removed yet

### Key Files

- `Src/main.c`
- `Src/freertos.c`
- `Src/gimbal_task.c`
- `Src/vision_input.c`
- `Src/usb_cdc_test.c`
- `USB_DEVICE/App/usbd_cdc_if.c`
- `USB_CDC_MIGRATION.md`

### Current Recommendation

1. Keep this directory as the only formal lower-level firmware project
2. Preserve UART compatibility until USB CDC is validated across the full system
3. Reuse the existing `vision_input` and `usb_cdc_test` hooks for continued USB CDC migration

### Reading Guidance

- for the full system path, read this together with `dev-branch/README.md`
- for USB CDC migration and validation scripts, also read `dev-branch/scripts/README.md`

### Vision Follow Tuning Notes

During visual follow, the lower-level firmware now continuously reports:

- `error_x` / `error_y`: filtered pixel error relative to image center
- `yaw_add_mrad` / `pitch_add_mrad`: actual visual angle increment applied this cycle, in milliradians
- `pitch_set_mrad`: current pitch setpoint after limiting, in milliradians

Current behavior:

- the visual follow path now uses a PID-shaped control hook, with only `P` enabled by default
- the pitch follow target is additionally constrained to `[-pi/4, +pi/4]`
- each visual increment is still protected by `VISION_MAX_ANGLE_STEP`

Suggested tuning flow:

1. Watch the upper-level `rm_gimbal_bridge` log line `tune err=(ex,ey) add=(yaw,pitch)mrad pitch_set=...`
2. If `error_y` stays large while `pitch_add_mrad` is too small, increase `VISION_PITCH_PID_KP`
3. If `pitch_add_mrad` oscillates and the target hunts vertically, decrease `VISION_PITCH_PID_KP`
4. If `pitch_set_mrad` often stays close to `+-785mrad`, the current pitch software limit is being hit

Current tuning entry points:

- `Src/gimbal_task.h`
  - `VISION_YAW_PID_KP`
  - `VISION_PITCH_PID_KP`
  - `VISION_PID_MAX_OUT`
  - `GIMBAL_PITCH_FOLLOW_MAX_ANGLE`
