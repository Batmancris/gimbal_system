# hik_camera

[中文](#中文) | [English](#english)

## 中文

### 简介

`hik_camera` 是当前上位机主线使用的海康工业相机 ROS2 驱动。

它负责：

- 枚举并打开海康工业相机
- 发布标准 ROS 图像流 `image_raw`
- 发布共享内存图像 `hbmem_img`
- 加载相机内参
- 支持运行时调整曝光和增益

### 当前定位

- 相机主线：是
- 当前推荐使用：是
- 依赖：海康相机 SDK、ROS2、相关图像消息依赖

### 主要话题

- 发布 `image_raw`
- 发布 `/hbmem_img`

### 主要参数

- `camera_name`
- `frame_id`
- `exposure_time`
- `gain`
- `camera_info_url`
- `use_sensor_data_qos`

### 运行

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### 说明

当前项目里虽然还保留了其他相机驱动目录，但主线优先维护本包。

## English

### Overview

`hik_camera` is the current ROS2 mainline driver for the Hikrobot industrial camera.

It is responsible for:

- enumerating and opening the Hikrobot camera
- publishing standard ROS image data on `image_raw`
- publishing shared-memory image data on `hbmem_img`
- loading camera calibration data
- supporting runtime exposure and gain updates

### Current Role

- Active camera mainline: yes
- Recommended default path: yes
- Dependencies: Hikrobot SDK, ROS2, and related image message packages

### Main Topics

- publishes `image_raw`
- publishes `/hbmem_img`

### Main Parameters

- `camera_name`
- `frame_id`
- `exposure_time`
- `gain`
- `camera_info_url`
- `use_sensor_data_qos`

### Run

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### Note

Other camera driver directories still exist in the workspace, but this package is the preferred camera mainline to maintain.
