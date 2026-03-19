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

### USB CDC 当前状态

截至当前工作区状态，USB CDC 已完成以下基础能力：

- STM32 端成功枚举为 USB CDC ACM 设备
- 主机端可识别为 `/dev/ttyACM0`
- 下位机可周期性发送 heartbeat：`HB\r\n`
- 主机到 STM32 的接收路径已打通
- 下位机可对收到的数据进行回显
- 已增加 pitch swing test with safety switch 诊断能力

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
