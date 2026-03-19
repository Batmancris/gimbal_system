# armor_detector

[中文](#中文) | [English](#english)

## 中文

### 简介

`armor_detector` 是项目中保留的传统视觉装甲板识别模块。

它基于图像预处理、灯条提取、灯条配对、数字分类、PnP 解算等传统方法完成装甲板识别与位姿估计。

### 当前定位

- 当前主线：否
- 保留状态：历史算法参考
- 适合用途：传统视觉方案回看、算法对照、调试旧消息接口

### 主要输入输出

- 订阅 `image_raw`
- 订阅 `camera_info`
- 发布 `armor_detector/armors`
- 发布多类调试话题与结果图像

### 当前说明

本模块仍然具有研究和教学价值，但当前项目主线检测路径已经转向 `rm_armor_detection` 的 YOLOv8 / BPU 实现。

## English

### Overview

`armor_detector` is the preserved traditional computer-vision armor detector in this workspace.

It uses classical processing stages such as image preprocessing, light bar extraction, light matching, digit classification, and PnP solving to estimate armor targets and poses.

### Current Role

- Active mainline: no
- Kept as: historical algorithm reference
- Best used for: reviewing the traditional CV path, comparing algorithms, and debugging older message interfaces

### Main I/O

- subscribes to `image_raw`
- subscribes to `camera_info`
- publishes `armor_detector/armors`
- publishes multiple debug topics and result images

### Current Note

This module still has research and teaching value, but the active project detection path has moved to the YOLOv8 / BPU implementation in `rm_armor_detection`.
