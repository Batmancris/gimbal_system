# rm_interfaces

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_interfaces` 保存当前工作区共用的 ROS2 自定义消息与服务定义。

### 当前定位

- 主线基础包：是
- 直接运行入口：否
- 主要作用：为检测、传统视觉、控制桥接等模块提供统一接口

### 当前内容

- 装甲板与目标相关消息
- 调试消息
- 底盘 / 云台控制相关消息
- 串口接收与测量相关消息
- `SetMode` 服务定义

### 使用建议

- 如果你在看数据流接口，建议和 `rm_armor_detection`、`rm_gimbal_bridge` 一起对照
- 如果你在看单节点启动，这里通常不是直接入口，而是被其他包依赖

## English

### Overview

`rm_interfaces` contains the shared ROS2 custom messages and service definitions used in this workspace.

### Current Role

- Mainline foundation package: yes
- Standalone runtime entry: no
- Primary purpose: provide common interfaces for detection, traditional vision, and control bridge modules

### Current Contents

- armor and target related messages
- debug messages
- chassis / gimbal control related messages
- serial receive and measurement related messages
- the `SetMode` service definition

### Usage Guidance

- if you are tracing data interfaces, read this together with `rm_armor_detection` and `rm_gimbal_bridge`
- this directory is usually not a direct runtime entry; it is a shared dependency for other packages
