# dev-branch

## 简介

`dev-branch` 是当前项目的上位机主线工作区，主要运行在 RDK-X5 等设备上，基于 ROS2。

这个目录主要负责：

- 相机驱动
- 视觉检测
- 上下位机通信桥接
- 部署脚本
- 启动脚本与联调辅助工具

## 当前主线方案

### 相机主线

当前主线相机方案为：

- `hik_camera`

原因：

- 这是当前已经实际接入并联调过的方案
- 更贴近现有海康工业相机硬件

以下目录目前不作为主线相机方案：

- `rm_camera_driver`
- `rm_camera_driver_nv12`

它们保留为历史/备用/实验参考，不作为当前主运行路径。

### 通信主线

当前上下位机主线通信方式为：

- `UART`

说明：

- `rm_gimbal_bridge` 当前主链仍按 UART 方向组织
- USB CDC 当前只保留最小测试支线，不作为当前比赛/联调主方案
- USB CDC 的后续迁移目标由下位机主工程中的文档统一管理：
  - `../Gimbal control/USB_CDC_MIGRATION.md`

## 关键目录

- `hik_camera/`
  当前主线相机驱动
- `rm_armor_detection/`
  目标检测与装甲板识别
- `rm_gimbal_bridge/`
  上下位机桥接与串口发送
- `rm_interfaces/`
  自定义消息与接口
- `rm_utils/`
  公共工具与数学模块
- `scripts/`
  部署、自启动、远端运行辅助脚本

## 当前建议

- 优先维护 `hik_camera + rm_armor_detection + rm_gimbal_bridge`
- 若继续做 USB CDC，请明确标记为实验线
- 不要把 `build/install/log` 当作源码的一部分保留

## 说明

- 本目录已经清理并忽略了常见构建产物
- 当前主线以“稳定跑通”为优先，不以实验功能为主
