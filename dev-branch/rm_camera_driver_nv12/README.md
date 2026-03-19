# rm_camera_driver_nv12

[中文](#中文) | [English](#english)

## 中文

### 简介

`rm_camera_driver_nv12` 是项目中保留的旧 NV12 / 共享内存方向的大恒相机驱动实现。

### 当前定位

- 当前主线：否
- 保留状态：实验 / 历史参考
- 适合用途：回看 NV12 图像链路、共享内存数据接口、旧方案对照

### 主要话题

- 发布 `hbmem_img`
- 发布 `camera_info`

### 常见参数

- `camera_info_url`
- `exposure_time`
- `gain`
- `resolution_width`
- `resolution_height`
- `recording`

### 运行

```bash
ros2 run rm_camera_driver_nv12 rm_camera_driver_nv12_node
```

### 说明

当前项目默认主线相机驱动是 `hik_camera`。本包继续保留，主要是为了参考旧的 NV12 方案，而不是作为默认入口。

## English

### Overview

`rm_camera_driver_nv12` is the preserved Daheng camera driver implementation for the older NV12 / shared-memory path.

### Current Role

- Active mainline: no
- Kept as: experimental / historical reference
- Best used for: reviewing the NV12 image path, shared-memory interfaces, and older implementation choices

### Main Topics

- publishes `hbmem_img`
- publishes `camera_info`

### Common Parameters

- `camera_info_url`
- `exposure_time`
- `gain`
- `resolution_width`
- `resolution_height`
- `recording`

### Run

```bash
ros2 run rm_camera_driver_nv12 rm_camera_driver_nv12_node
```

### Note

The current project camera mainline is `hik_camera`. This package is kept mainly for reference to the older NV12 path, not as the default entry point.
