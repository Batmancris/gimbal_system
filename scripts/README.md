# scripts

这里是仓库根目录的辅助脚本，主要用于本地构建或快速进入主线包。RDK X5 板端部署和运行脚本在 `ros2_ws/scripts/`，日常调试优先看那里。

## 当前主线

- ROS2 工作区：`ros2_ws/`
- STM32 固件：`firmware/stm32_gimbal_control/`
- 当前默认目标：bear
- 当前真实进度：低速跟随已经顺滑，高速跟随仍会跟不上，需要继续调参和现场验证。

## 脚本

- `build_ros2_mainline.sh`：构建 `hik_camera rm_bear_detection rm_gimbal_bridge`。`rm_armor_detection` 和 `rm_vehicle_detection` 属于 legacy/optional，不是当前 bear-follow baseline 的默认构建目标。
- `build_firmware_mainline.sh`：构建 STM32 固件主线。
- `run_ros2_bridge.sh`：本地运行桥接 launch。
- `tianaim_paths.sh`：路径解析辅助脚本。

## 路径覆盖

```bash
export TIANAIM_ROS_WS=/path/to/ros2_ws
export TIANAIM_FIRMWARE_DIR=/path/to/firmware/stm32_gimbal_control
```

## 推荐阅读

- `README.md`
- `ros2_ws/README.md`
- `ros2_ws/scripts/README.md`
- `firmware/stm32_gimbal_control/README.md`

