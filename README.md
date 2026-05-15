# TianAim / gimbal_system

<p align="center">
  <img src="assets/tianaim_readme.svg" alt="TianAim auto-aim runtime chain" width="100%">
</p>

<p align="center">
  <a href="https://github.com/Batmancris/gimbal_system/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=stars"></a>
  <a href="https://github.com/Batmancris/gimbal_system/watchers"><img alt="GitHub watchers" src="https://img.shields.io/github/watchers/Batmancris/gimbal_system?style=for-the-badge&logo=github&label=watch"></a>
  <a href="https://github.com/Batmancris/gimbal_system/commits"><img alt="Last commit" src="https://img.shields.io/github/last-commit/Batmancris/gimbal_system?style=for-the-badge"></a>
  <a href="https://github.com/Batmancris/gimbal_system"><img alt="ROS2" src="https://img.shields.io/badge/ROS2-TROS%20%2F%20RDK--X5-39d5ff?style=for-the-badge"></a>
</p>

## 产品定位

TianAim 是 Tianbot 云台视觉跟随系统的一体化工作区，覆盖：

- RDK X5 上位机 ROS2/TROS 感知链路
- STM32F407 下位机云台控制固件
- USB-CDC 视觉通信、DBUS 遥控输入、CAN 电机控制

## 当前能力

**bear-follow baseline** - 当前唯一支持的跟随模式

- 运行基线: v1 headless fast_best stable
- 跟随目标: bear（熊）
- 跟随丝滑，偶发卡顿来自出框/丢检/bbox 突变

## 后续适配（KT）

KT 模型适配是后续工作，不是当前能力：

- KT target adaptation
- detector replacement
- target_type abstraction

## 唯一启动命令

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

启动成功判据: `Camera OK.` / `Detection OK.` / `Bridge OK.` / `FAST FOLLOW READY.` / `profile: fast_best.`

停止:

```bash
ssh rdk-x5 "tmux -L autoaim kill-server 2>/dev/null; pkill -f 'ros2 run' || true"
```

## 目录结构

```text
.
├── ros2_ws/                         # 上位机: ROS2 packages and RDK scripts
│   ├── src/
│   │   ├── hik_camera/              # [core] 海康相机驱动
│   │   ├── rm_bear_detection/       # [core] bear YOLO 检测
│   │   ├── rm_gimbal_bridge/        # [core] 云台桥接
│   │   ├── rm_interfaces/           # [core] 消息定义
│   │   ├── rm_utils/                # [core] 工具库
│   │   ├── rm_vehicle_detection/    # [legacy] vehicle 检测
│   │   └── rm_armor_detection/      # [legacy] armor 检测
│   └── scripts/                     # 启动/诊断/部署脚本
├── firmware/stm32_gimbal_control/   # 下位机: STM32 固件
├── datasets/                        # 数据集骨架
├── models/                          # 模型元数据
├── tools/                           # 采集/训练/评估工具
├── scripts/                         # 顶层构建/路径工具
├── assets/                          # README 图片
└── docs/                            # 项目文档
```

## 当前限制

- 不包含 KT 模型和检测器
- 偶发卡顿来自目标出框/丢检/bbox 突变
- 禁止依赖: `rm_vis`、`/image_raw`、`publish_image_raw:=true`

## 文档入口

- 运维手册: `docs/runbook.md`
- 架构说明: `docs/architecture.md`
- ROS2 脚本: `ros2_ws/scripts/README.md`
- ROS2 工作区: `ros2_ws/README.md`
- 云台桥接: `ros2_ws/src/rm_gimbal_bridge/README.md`
- bear 检测: `ros2_ws/src/rm_bear_detection/README.md`
- STM32 固件: `firmware/stm32_gimbal_control/README.md`
