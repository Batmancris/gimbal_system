# rm_camera_driver

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_camera_driver` 是项目中保留的旧版大恒工业相机 ROS2 驱动。

它主要提供：

- `image_raw` 图像发布
- `camera_info` 发布
- 基础相机参数控制
- 录像辅助能力

### 当前定位

- 当前主线：否
- 保留状态：历史参考 / 备用方案
- 适合用途：回看旧相机链路、对照参数设计、兼容旧数据流

### 主要话题

- 发布 `image_raw`
- 发布 `camera_info`

### 常见参数

- `camera_info_url`
- `exposure_time`
- `gain`
- `resolution_width`
- `resolution_height`
- `recording`

### 说明

如果当前目标是维护整机主链，请优先使用 `hik_camera`，除非你明确需要回看旧的大恒相机路径。

## English

### Overview

`rm_camera_driver` is the legacy ROS2 Daheng industrial camera driver kept in this workspace.

It mainly provides:

- `image_raw` publication
- `camera_info` publication
- basic camera parameter control
- optional recording support

### Current Role

- Active mainline: no
- Kept as: legacy reference / backup path
- Best used for: reviewing the older camera path, comparing parameter design, or supporting older data flows

### Main Topics

- publishes `image_raw`
- publishes `camera_info`

### Common Parameters

- `camera_info_url`
- `exposure_time`
- `gain`
- `resolution_width`
- `resolution_height`
- `recording`

### Note

If your goal is to maintain the current whole-system mainline, prefer `hik_camera` unless you explicitly need the older Daheng-based path.
