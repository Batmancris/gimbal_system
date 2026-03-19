# 云台瞄靶系统代码梳理

## 1. 当前总体链路

### 下位机

- 工程目录：`Gimbal control`
- 核心框架：STM32F407 + HAL + FreeRTOS
- 主要任务：
  - `INS_task`：IMU姿态解算
  - `gimbal_task`：云台控制闭环
  - `chassis_task`：底盘相关逻辑
  - `test_task`：当前只做视觉坐标调试
- 电机链路：
  - 云台使用 GM6020
  - 通过 CAN 总线收发电机反馈和控制电流
- 视觉输入：
  - `USART1 + DMA + IDLE 中断`
  - 接收 8 字节协议：`FA FB XL XH YL YH FC FD`

### 上位机

- 工程目录：`dev-branch`
- 已存在模块：
  - `rm_camera_driver`：大恒相机 ROS2 驱动
  - `rm_camera_driver_nv12`：适配 RDK X5 BPU 输入的 NV12 驱动
  - `armor_detector`：传统视觉装甲板检测
  - `rm_armor_detection`：RDK X5 YOLOv8 检测
- 本次新增模块：
  - `rm_gimbal_bridge`：检测结果到下位机串口协议桥接

## 2. 现阶段主要问题

### 已确认的问题

- 下位机收到视觉坐标后，还没有接入 `gimbal_task` 控制闭环
- 原 `rm_armor_detection` 把串口发送直接写在推理节点里，职责耦合过重
- 原上位机发送的是 ASCII 文本，不是下位机实际解析的二进制协议
- 下位机调试输出和视觉输入共用 `USART1`，会互相干扰

### 还没完成但后续必须补的部分

- 像素坐标到 yaw/pitch 角度误差的标定关系
- 目标筛选策略和跟踪策略
- 丢目标后的保持、回中、搜索逻辑
- 弹道补偿与开火判定

## 3. 本次改动

- 下位机串口接收链路做了基础稳固：
  - 关闭 `USART1` 上无必要的 `RXNE` 中断
  - `usart_printf` 改为有边界的 `vsnprintf`
  - `test_task` 默认不再从视觉串口打印调试信息
  - DMA 空闲中断处理时补了停止 DMA 和异常帧清空
- 上位机拆分职责：
  - `rm_armor_detection` 只保留检测与发布
  - 新增 `rm_gimbal_bridge` 专门负责选目标并通过串口发 8 字节协议
