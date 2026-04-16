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
- `gamma_enable`
- `gamma`
- `camera_info_url`
- `use_sensor_data_qos`

### 当前联调说明

- 当前车辆检测联调使用短曝光加适度增益，并开启 Gamma 提亮暗部，避免高亮区域过早过曝
- 某些海康相机型号或 SDK 组合下，`gain` 可能不会按配置成功写入，需要以实际启动日志为准
- `image_raw` 会被桌面可视化节点直接订阅，用于叠加检测框显示

### 运行

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### 说明

当前项目里虽然还保留了其他相机驱动目录，但主线优先维护本包。

### 阅读建议

- 若你在看整机链路，请同时参考 `rm_armor_detection/README.md` 和 `rm_gimbal_bridge/README.md`
- 若你只是在排查相机输入、曝光或共享内存链路，本 README 就是当前相机侧入口

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
- `gamma_enable`
- `gamma`
- `camera_info_url`
- `use_sensor_data_qos`

### Current Integration Notes

- The current vehicle-detection setup uses shorter exposure, moderate gain, and Gamma to lift shadows while limiting highlight clipping
- On some Hikrobot camera and SDK combinations, the configured `gain` may not be accepted at runtime, so the startup log is the source of truth
- `image_raw` is also consumed by the desktop visualizer for live overlay rendering

### Run

```bash
ros2 launch hik_camera hik_camera.launch.py
```

### Note

Other camera driver directories still exist in the workspace, but this package is the preferred camera mainline to maintain.

### Reading Guidance

- for the full runtime chain, read this together with `rm_armor_detection/README.md` and `rm_gimbal_bridge/README.md`
- if you are only diagnosing camera bring-up, exposure, or shared-memory transport, this README is the correct camera-side entry
