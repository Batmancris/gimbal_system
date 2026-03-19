# Gimbal control

## 简介

`Gimbal control` 是当前项目的下位机主线工程，运行在 STM32F407 / DJI C 板上，负责云台、电机、传感器与上下位机通信。

当前目录主要包含：

- 云台姿态与电机控制
- CAN 总线与 GM6020 电机交互
- DBUS 遥控器输入
- BMI088 / IST8310 IMU 与姿态解算
- 视觉输入链路
- FreeRTOS 任务组织

## 当前定位

- 主线代码：是
- 直接参与当前系统联调：是
- 当前稳定主链通信：UART
- 当前新增诊断链路：USB CDC

## USB CDC 当前进展

截至本次提交，`Gimbal control` 已完成一条可独立验证的 USB CDC 诊断支线：

- STM32 端已经成功枚举为 USB CDC ACM 设备
- RDK-X5 端可识别 `/dev/ttyACM0`
- STM32 可周期性发送 heartbeat：`HB\\r\\n`
- RDK-X5 -> STM32 的 USB RX 通路已经打通
- STM32 在收到数据后可将前 16 字节原样回显

这意味着：

- USB CDC 的 **双向基础通信已验证通过**
- 还没有接回完整视觉主链
- 当前仍然不修改控制律和 `gimbal_task.c`

## 关键目录

- `Src/`
  主要业务逻辑、任务和 USB CDC 诊断实现
- `Inc/`
  公共头文件和 HAL 配置
- `USB_DEVICE/`
  从参考工程移植过来的 USB Device / CDC 基础代码
- `Chassis/`
  底盘、遥控、CAN 等底层控制
- `IMU/`
  姿态与传感器相关代码
- `algorithm/`
  PID、AHRS、中间件与数学库

## 关键文件

- `Src/main.c`
- `Src/freertos.c`
- `Src/gimbal_task.c`
- `Src/vision_input.c`
- `Src/usb_cdc_test.c`
- `USB_DEVICE/App/usbd_cdc_if.c`

## 当前建议

- 若要继续推进 USB CDC，请以当前目录为唯一下位机主线继续演进
- 不建议另起一套新的 STM32 工程
- 在 USB CDC 完全替代 UART 之前，保留现有 UART 兼容路径更安全
