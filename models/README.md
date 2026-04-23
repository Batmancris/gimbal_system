# models

这里保存模型部署说明、导出记录和轻量模型元数据。大模型二进制通常放在对应 ROS2 package 的 `config/` 目录，或部署到 RDK X5 的 install 目录。

## 当前主线模型

当前默认跟随链路使用 bear 检测：

```text
ros2_ws/src/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin
```

运行时默认安装路径：

```text
/home/sunrise/rm_ws/install/rm_bear_detection/lib/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin
```

## 当前状态

- 低速跟随已经顺滑。
- 高速跟随仍然存在跟不上，不应把模型或控制链路描述成高速完成。
- 这轮主要进展不是重新训练模型，而是 RDK X5 运行链路、目标稳定筛选、桥接发送和 STM32 控制参数整理。

## 相关代码

- `ros2_ws/src/rm_bear_detection/`
- `ros2_ws/src/rm_vehicle_detection/`
- `tools/training/`

