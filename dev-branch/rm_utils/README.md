# rm_utils

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_utils` 是当前工作区中的公共工具库，给多个 ROS2 包提供共享能力。

### 当前包含内容

- 扩展卡尔曼滤波器
- PnP 解算器
- 弹道补偿器
- 日志封装
- URL 路径解析
- 心跳发布器

### 当前定位

- 主线基础库：是
- 直接对外运行：通常否
- 主要作用：减少各包重复实现，统一公共数学与工具能力

### 适用场景

- 检测与跟踪算法共用数学模块
- 相机与视觉节点共用工具能力
- ROS2 节点复用日志、路径解析、心跳发布等基础设施

### 阅读建议

- 若你在排查日志封装，可继续看 `include/rm_utils/logger/README.md`
- 若你在看整机链路，本目录通常不是第一入口，而是被多个主线包复用

## English

### Overview

`rm_utils` is the shared utility library used across multiple ROS2 packages in this workspace.

### Current Contents

- Extended Kalman Filter
- PnP solver
- trajectory compensator
- logging wrapper
- URL path resolver
- heartbeat publisher

### Current Role

- Mainline foundation library: yes
- Standalone runtime entry: usually no
- Primary purpose: reduce duplicated implementations and centralize common math and utility capabilities

### Common Use Cases

- shared math modules for detection and tracking
- reusable utility support for camera and vision nodes
- common ROS2 infrastructure such as logging, path resolution, and heartbeat publication

### Reading Guidance

- if you are investigating the logging wrapper, continue with `include/rm_utils/logger/README.md`
- for the whole runtime path, this directory is usually a shared dependency rather than the first entry point
